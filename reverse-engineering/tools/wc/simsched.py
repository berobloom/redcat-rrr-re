#!/usr/bin/env python3
"""simsched.py: replay Watcom's instruction scheduler (the -or pass; Open Watcom's
bld/cg/c/inssched.c, which descends from 10.x) on one basic block of an unscheduled compile.

usage: simsched.py OBJ FUNC list              number the instructions of FUNC
       simsched.py OBJ FUNC START END [order]  schedule instructions START..END (one block,
                                              without its branch) and print the result
  OBJ is compiled with `-oeilm -s` instead of `-ox` (`-oeilmr -s` gives -ox's code, CONFIRMED on
  init_objects): the code before scheduling. 'order': a comma list of indexes into the block for
  a different input order. SIMDEBUG=<text> prints every decision where an instruction containing
  <text> is ready; SIMKEY/SIMHEIGHT pick variant tie-breaks (see schedule()).

Model (from inssched.c, redefby.c, f.h and 386table.c; the approximations are LIKELY, not checked):
  - the block is scheduled bottom-up; among ready instructions the lowest StallCost against the
    ones already placed below wins, then the greater height (longest chain of ancestors, each
    adding its opnd_stall), then the greater 'stallable' (indexed operand 3, register 2, memory 1),
    then the later source position;
  - units of f.h's 486 table: integer ops ALUX (shifts ALU1) 1/2, imul 11/11, idiv 24/24,
    calls 0/0; FP is not modelled;
  - dependencies: registers from capstone, memory by redefby.c's ZapsTheOp (two different globals
    never alias, a pointer store aliases everything, the same base and index with disjoint
    offsets alias only if the index changes in between); a call reads its parameter registers
    (eax, edx, ebx, ecx by src/rrr.h's prototype) and writes eax, and orders all memory;
  - `xor r,r` is MOV 0 while scheduling (no flags); an instruction whose flags are used (the
    compare feeding the block's branch) has stall cost -1.
  On src/ (2026-11-27) it reproduces 450 of 530 blocks; 314 of them are left as they were by -or.
"""
import os
import sys

import capstone
from capstone import x86

DEBUG = os.environ.get('SIMDEBUG')
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, ROOT + '/reverse-engineering/tools')
sys.path.insert(0, ROOT + '/reverse-engineering/tools/wc')
import libmatch as L   # noqa: E402
import objmatch as M   # noqa: E402

REG32 = {}
for r32, subs in (('eax', 'eax ax al ah'), ('ebx', 'ebx bx bl bh'), ('ecx', 'ecx cx cl ch'),
                  ('edx', 'edx dx dl dh'), ('esi', 'esi si'), ('edi', 'edi di'),
                  ('ebp', 'ebp bp'), ('esp', 'esp sp')):
    for s in subs.split():
        REG32[s] = r32


class Ins:
    pass


def load(obj, func):
    data = open(obj, 'rb').read()
    (_, recs), = list(L.modules(data))
    segs = L.code_of(recs)
    pubs = sorted(p for p in M.publics(recs) if p[0] in segs)
    seg, off, name = [p for p in pubs if M.plain(p[2]) == func][0]
    end = next((q[1] for q in pubs if q[0] == seg and q[1] > off), len(segs[seg][0]))
    fix = {}
    for fseg, foff, loc, selfrel, kind, fname, disp in L.fixups_of(recs):
        if fseg == seg and not selfrel:
            fix[foff] = fname
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    out = []
    for x in md.disasm(bytes(segs[seg][0][off:end]), off):
        i = Ins()
        i.text = '%s %s' % (x.mnemonic, x.op_str)
        i.mn = x.mnemonic
        i.addr = x.address
        sym = fix.get(x.address + x.disp_offset) if x.disp_offset else None
        regs_r, regs_w = x.regs_access()
        i.rd = {REG32.get(x.reg_name(r), x.reg_name(r)) for r in regs_r} - {'eflags'}
        i.wr = {REG32.get(x.reg_name(r), x.reg_name(r)) for r in regs_w} - {'eflags'}
        i.flags_w = any(x.reg_name(r) == 'eflags' for r in regs_w)
        ops = x.op_str.split(', ')
        if x.mnemonic in ('xor', 'sub') and len(ops) == 2 and ops[0] == ops[1]:
            i.flags_w = False                    # MOV 0 => REG, made an xor when generated
            i.rd = set()
        i.flags_r = any(x.reg_name(r) == 'eflags' for r in regs_r)
        i.mem_r, i.mem_w = [], []
        for k, op in enumerate(x.operands):
            if op.type == x86.X86_OP_MEM:
                base = x.reg_name(op.mem.base) if op.mem.base else None
                index = x.reg_name(op.mem.index) if op.mem.index else None
                regs = {REG32[r] for r in (base, index) if r}
                m = (sym, regs, op.mem.disp, op.size, (base, index, op.mem.scale))
                # capstone: operand 0 of a mov/arith is the destination
                if k == 0 and x.mnemonic not in ('cmp', 'test', 'push'):
                    i.mem_w.append(m)
                    if x.mnemonic not in ('mov', 'lea'):
                        i.mem_r.append(m)
                elif x.mnemonic != 'lea':
                    i.mem_r.append(m)
        i.stall_noreg = sum(3 for k, op in enumerate(x.operands) if op.type == x86.X86_OP_MEM and (op.mem.base or op.mem.index)) + \
            sum(1 for k, op in enumerate(x.operands) if op.type == x86.X86_OP_MEM and k > 0 and not (op.mem.base or op.mem.index))
        i.call = x.mnemonic == 'call'
        i.callee = fix.get(x.address + 1) if i.call else None
        i.fp = x.mnemonic.startswith('f')
        i.jump = x.mnemonic.startswith('j')
        i.alu1 = x.mnemonic in ('shl', 'shr', 'sar', 'rol', 'ror', 'sal', 'shld', 'shrd')
        i.stallable = 0
        for k, op in enumerate(x.operands):
            if op.type == x86.X86_OP_MEM and op.mem.base or op.type == x86.X86_OP_MEM and op.mem.index:
                i.stallable += 3
            elif op.type == x86.X86_OP_MEM and k > 0:
                i.stallable += 1
            elif op.type == x86.X86_OP_REG and (k > 0 or x.mnemonic not in ('mov', 'lea')):
                i.stallable += 2
        out.append(i)
    return out


def zaps(r, op):
    """ZapsTheOp for two memory names (sym, regs, disp, size, scale_key): 'T', 'F' or 'M'.
    An absolute operand has no registers (N_MEMORY); otherwise it is N_INDEXED with the symbol
    as its base (None: a pointer, base NULL)."""
    rsym, rregs, rdisp, rsize, rkey = r
    osym, oregs, odisp, osize, okey = op
    over = rdisp < odisp + osize and odisp < rdisp + rsize
    if not oregs:                                   # op N_MEMORY: ZapsMemory(result, op)
        if not rregs:
            return 'T' if rsym == osym and over else 'F'
        if rsym is None:                            # result indexed, base NULL
            return 'T'
        return 'T' if rsym == osym else 'F'         # for_index: no overlap test
    # op N_INDEXED: ZapsIndexed
    if not rregs:                                   # result N_MEMORY
        if osym is None:
            return 'T'
        return 'T' if rsym == osym else 'F'
    if rsym == osym and rkey == okey and not over:
        return 'M'                                  # no overlap if the index is the same
    if rsym is None or osym is None:
        return 'T'
    return 'T' if rsym == osym else 'F'


def mem_redef(i, j_mem, index_redef_between):
    """ReallyDefinedBy for memory operand j_mem and the memory results of i"""
    for r in i.mem_w:
        z = zaps(r, j_mem)
        if z == 'T':
            return True
        if z == 'M' and index_redef_between(j_mem[1]):
            return True
    return False


def data_dep(i, j, block, linked):
    """DataDependant(i, j): does i redefine an operand or the result of j?"""
    ii, jj = block.index(i), block.index(j)
    lo, hi = min(ii, jj), max(ii, jj)

    def redef_between(regs):
        if not linked:
            return True
        return any(block[k].wr & regs for k in range(lo, hi))
    # index registers of j's memory operands
    for m in j.mem_r + j.mem_w:
        if i.wr & m[1]:
            return 'index'
    if i.wr & (j.rd | j.wr):
        return 'op'
    for m in j.mem_r + j.mem_w:
        if mem_redef(i, m, redef_between):
            return 'op'
    return None


def order_dep(i, j, block):
    if i.jump:
        return True
    if i.call and (j.mem_w or j.wr):
        return True
    if j.call and (i.mem_w or i.mem_r):
        return True
    if data_dep(i, j, block, True) or data_dep(j, i, block, True):
        return True
    if i.flags_r and j.flags_w or j.flags_r and i.flags_w:
        return True
    return False


def unit(i):
    """(good_fu, unit_stall, opnd_stall) of f.h's 486 table"""
    if i.call:
        return 0, 0, 0
    if i.mn == 'imul':
        return 1, 11, 11
    if i.mn in ('idiv', 'div'):
        return 1, 24, 24
    return (1 if i.alu1 else 3), 1, 2


def stall_cost(ins, below, block):
    """StallCost(ins, top): below = the instructions already scheduled, top first"""
    if getattr(ins, 'cc_used', False):
        return -1                                # its condition codes are used: schedule now
    if not below:
        return 0
    avail, ustall, ostall = unit(ins)
    k = 0
    while ustall > 0 or ostall > 0:
        curr = below[k]
        if ostall > 0:
            ostall -= 1
            d = data_dep(ins, curr, block, False)
            if d == 'op':
                return ostall * 2
            if d == 'index':
                return ostall * 2 + 1
        if ustall > 0:
            ustall -= 1
            good = unit(curr)[0]
            ov = avail & good
            if ov:
                ov &= ov - 1
                avail &= ov
                if avail == 0:
                    return ustall * 2
        k += 1
        if k >= len(below):
            break
    return 0


def schedule(block):
    n = len(block)
    for k, i in enumerate(block):
        i.id = k
        i.deps = [block[j] for j in range(k) if order_dep(i, block[j], block)]
        i.anc = 0
    for i in block:
        for p in i.deps:
            p.anc += 1
    hm = os.environ.get('SIMHEIGHT', 'ow')
    if hm == 'ow':                               # height: longest chain of ancestors
        for i in block:
            i.height = max((p.height for p in i.deps), default=0) + unit(i)[2]
    elif hm == 'pred':                           # the predecessor's own stall counts
        for i in block:
            i.height = max((p.height + unit(p)[2] for p in i.deps), default=0)
    elif hm == 'desc':                           # longest chain of dependants (to the end)
        succ = {id(i): [] for i in block}
        for i in block:
            for p in i.deps:
                succ[id(p)].append(i)
        for i in reversed(block):
            i.height = max((q.height for q in succ[id(i)]), default=0) + unit(i)[2]
    elif hm == 'datadep':                        # only register/data edges count
        for i in block:
            i.height = max((p.height for p in i.deps if (p.wr & i.rd) or (i.wr & p.rd)), default=0) + unit(i)[2]
    ready = [i for i in block if i.anc == 0]
    out = []
    top = None
    scheduled_after = {}
    while ready:
        best = None
        for c in ready:
            cost = stall_cost(c, out, block)
            mode = os.environ.get('SIMKEY', 'ow')
            if mode == 'ow':
                key = (-cost, c.height, c.stallable, c.id)
            elif mode == 'nostall':
                key = (-cost, c.height, c.id)
            elif mode == 'stall_noreg':
                key = (-cost, c.height, c.stall_noreg, c.id)
            elif mode == 'stall_first':
                key = (-cost, c.stallable, c.height, c.id)
            elif mode == 'lowid':
                key = (-cost, c.height, c.stallable, -c.id)
            if best is None or key > best[0]:
                best = (key, c)
        b = best[1]
        if DEBUG and any(DEBUG in c.text for c in ready):
            print('-- below: %s' % (out[0].text if out else None))
            for c in sorted(ready, key=lambda c: -c.id):
                print('   %-34s cost=%d height=%d stall=%d id=%d%s' % (c.text, stall_cost(c, out, block),
                      c.height, c.stallable, c.id, '  <== picked' if c is b else ''))
        ready.remove(b)
        out.insert(0, b)
        top = b
        for p in b.deps:
            p.anc -= 1
            if p.anc == 0:
                ready.append(p)
    return out


def set_cc_used(block):
    """INS_CC_USED: a flag-writing instruction whose flags a later one reads"""
    exit_cc = getattr(block[-1], 'feeds_exit', False) if block else False
    for k, i in enumerate(block):
        i.cc_used = False
        if i.flags_w:
            for j in block[k + 1:]:
                if j.flags_r:
                    i.cc_used = True
                    break
                if j.flags_w:
                    break
            else:
                i.cc_used = exit_cc


PARMS = None


def parm_count(callee):
    """the number of register parameters of callee from src/rrr.h's prototypes (Watcom's default
    convention: eax, edx, ebx, ecx); None when unknown or variadic"""
    global PARMS
    if PARMS is None:
        import re
        PARMS = {}
        txt = open(ROOT + '/src/rrr.h').read()
        for m in re.finditer(r'^[\w \*]*?\b(\w+)\s*\(([^;{}]*)\)\s*;', txt, re.M):
            args = m.group(2).strip()
            if '...' in args:
                PARMS[m.group(1)] = 0
            elif args in ('', 'void'):
                PARMS[m.group(1)] = 0
            else:
                PARMS[m.group(1)] = args.count(',') + 1
    if callee is None:
        return None
    return PARMS.get(callee.rstrip('_'))


def set_call_args(block):
    """a call reads its parameter registers and writes eax (its result)"""
    for i in block:
        if i.call:
            n = parm_count(i.callee)
            regs = ['eax', 'edx', 'ebx', 'ecx']
            i.rd |= set(regs[:4 if n is None else min(n, 4)])
            i.wr |= {'eax'}


def main():
    obj, func = sys.argv[1], sys.argv[2]
    ins = load(obj, func)
    if sys.argv[3] == 'list':
        for k, i in enumerate(ins):
            print(k, hex(i.addr), i.text)
        return
    a, b = int(sys.argv[3]), int(sys.argv[4])
    block = ins[a:b + 1]
    set_call_args(block)
    set_cc_used(block)
    if len(sys.argv) > 5:
        block = [block[int(x)] for x in sys.argv[5].split(',')]
    for i in schedule(block):
        print(i.text)


if __name__ == '__main__':
    main()
