#!/usr/bin/env python3
"""entries.py: is a funcs.py start really a function entry?

usage: entries.py <prog> [--all]

funcs.py seeds its search with call targets and then tries every byte-scanned
call target it has not reached yet, so its list also holds addresses that are
just basic blocks inside another function (a branch lands there and the trial
happens to decode to a ret).  This tool separates them:

  call   - the target of a `call` in the game's code
  table  - the target of a `jmp [reg*4 + disp]` jump table
  branch - reached only by jumps/branches, and every such jump sits inside the
           body of an earlier start (so the address is a basic block of it)
  entry  - the image entry point
  loose  - reached by a jump that is not inside an earlier body (a tail jmp
           between modules, a fixup from data, ...) and not called

`call`, `table`, `entry` and `loose` are functions; `branch` is not, and
progress.py should not count it.  With --all print every start, otherwise only
the ones src/ does not define.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from lelib import Image          # noqa: E402
from funcs import analyse       # noqa: E402

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'wc'))


def names_of(root):
    named = {}
    for line in open(os.path.join(root, 'reverse-engineering', 'analysis', 'rrr_names.txt')):
        m = re.match(r'^0x([0-9a-f]+) (\S+)', line)
        if m:
            named.setdefault(int(m.group(1), 16), m.group(2))
    return named


def sweep(img, start, stop, limit=4000):
    """Decode the body at start: follow branches, do not follow calls.
    Returns the decoded (addr, size) list and the branch targets it reaches."""
    import capstone
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    ins, work, seen, todo = {}, [start], set(), []
    while work:
        a = work.pop()
        while a not in seen:
            if a in ins:
                break
            b = img.read(a, 16)
            if not b or not img.is_code(a) or a >= stop:
                break
            i = next(md.disasm(b, a), None)
            if i is None:
                break
            ins[a] = i
            seen.add(a)
            if len(ins) > limit:
                return ins, todo
            t = i.operands[0].imm if (i.operands and i.operands[0].type == 2) else None
            if i.mnemonic == 'call':
                pass
            elif i.mnemonic.startswith('j') or i.mnemonic.startswith('loop'):
                if t is not None and img.is_code(t):
                    todo.append(t)
                    if t not in ins:
                        work.append(t)
                if i.mnemonic == 'jmp':
                    break
            elif i.mnemonic in ('ret', 'retf', 'hlt', 'iretd'):
                break
            a += i.size
    return ins, todo


def main():
    prog = sys.argv[1]
    img = Image(prog)
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    d = analyse(img)

    # the game's callbacks are reached from tables in the data objects, which the
    # descent cannot see: walk those pointers too, so calls_in and coverage count,
    # but do not let them join the function list (data holds dwords that point into
    # the middle of instructions).  A pointer counts when the names file calls the
    # address a function or its first instruction is a prologue (push reg / sub esp).
    named = names_of(root)
    roots = set()
    # the game installs its callbacks with `mov [slot], offset fn`, so the pointers
    # live as immediates in code, and some as dwords in the data objects (they are
    # not in the file as fixups here)
    cand = set()
    for ins in d.insn.values():
        for op in ins.operands:
            if op.type == 2 and img.is_code(op.imm):
                cand.add(op.imm)
    for base, data, fl in img.objs[1:]:
        for off in range(0, len(data) - 3, 4):
            v = int.from_bytes(data[off:off + 4], 'little')
            if img.is_code(v):
                cand.add(v)
    for v in cand:
        if v in d.insn:
            continue
        b = img.read(v, 1)[0]
        if v in named or 0x50 <= b <= 0x57 or (b == 0x83 and img.read(v + 1, 1)[0] == 0xEC):
            roots.add(v)
    for v in sorted(roots):
        d.funcs.add(v)
        d.walk(v)
    d._drain()

    starts = sorted(d.funcs)
    ends = starts[1:] + [img.code_base + len(img.code)]
    body_of = {}                 # address -> start of the body it lies in
    for a, e in zip(starts, ends):
        for x in range(a, e):
            body_of[x] = a

    # who jumps where
    jumped = {}
    tables = set()
    for a, ins in d.insn.items():
        if not ins.mnemonic.startswith('j') and not ins.mnemonic.startswith('loop'):
            continue
        if ins.operands and ins.operands[0].type == 2 and ins.operands[0].imm:
            t = ins.operands[0].imm
            if img.is_code(t):
                jumped.setdefault(t, []).append(a)
        elif ins.mnemonic == 'jmp':
            for t in d.jump_table(ins):
                tables.add(t)

    # addresses that appear as a dword in the data objects: function-pointer tables
    ptr = set()
    for base, data, fl in img.objs[1:]:
        for off in range(0, len(data) - 3, 4):
            v = int.from_bytes(data[off:off + 4], 'little')
            if v in d.funcs:
                ptr.add(v)

    # functions src/ defines
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    src = ''
    for f in sorted(os.listdir(os.path.join(root, 'src'))):
        if f.endswith('.c'):
            src += open(os.path.join(root, 'src', f)).read()
    named = names_of(root)
    for line in open(os.path.join(root, 'reverse-engineering', 'analysis', 'rrr_names.txt')):
        m = re.match(r'^0x([0-9a-f]+) (\S+)', line)
        if m:
            named.setdefault(int(m.group(1), 16), m.group(2))

    C_START, LIB_START = 0x10010, 0x1B916
    for a, e in zip(starts, ends):
        if '--all' not in sys.argv and not C_START <= a < LIB_START:
            continue
        nm = named.get(a, 'fn_%x' % a)
        if not '--all' in sys.argv and (re.search(r'\b%s\b' % re.escape(nm), src) or nm in src):
            continue
        why = []
        if a == img.entry:
            why.append('entry')
        if d.calls_in.get(a, 0):
            why.append('call x%d' % d.calls_in[a])
        if a in tables:
            why.append('table')
        if a in ptr:
            why.append('pointer in data')
        src_addrs = jumped.get(a, [])
        inside = [s for s in src_addrs if body_of.get(s) not in (a, None)]
        if not why:
            if src_addrs and len(inside) == len(src_addrs):
                why.append('branch only, from 0x%x' % inside[0])
                verdict = 'branch'
            else:
                why.append('loose' if src_addrs else 'no direct ref')
                verdict = 'loose'
        else:
            verdict = 'func'
        # does the body before this address already own it?  A real entry is never
        # mid-instruction nor a branch target of the preceding body.
        prev = [p for p in starts if p < a and (d.calls_in.get(p, 0) or p in named)]
        if prev:
            p = prev[-1]
            nxt = [q for q in starts if q > a]
            ins, todo = sweep(img, p, nxt[0] if nxt else img.code_base + len(img.code))
            if a in todo:
                why.append('block of 0x%x' % p)
            else:
                span = [x for x in ins if x < a < x + ins[x].size]
                why.append('mid-instruction of 0x%x' % p if span else 'own boundary')
        first = d.insn.get(a)
        head = '%s %s' % (first.mnemonic, first.op_str) if first else '?'
        print('%-6s 0x%06x %-22s calls-in %-2d %-26s | %-6s %s' %
              (verdict, a, nm, d.calls_in.get(a, 0), '; '.join(why),
               img.read(a, 4).hex(), head[:32]))


if __name__ == '__main__':
    main()
