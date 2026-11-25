#!/usr/bin/env python3
"""nmrefs.py: check what a near miss POINTS AT, which fndiff.py cannot see (it prints addresses as @).

usage: nmrefs.py PROG FILE.OBJ [-v] [NAME[=0xADDR] ...]   (no names: every open line of src/nearmiss.txt)

Both sides are reduced to the addresses they use, which survive register, slot and order
differences:
  refs      the multiset of absolute addresses (globals, string literals, called functions) the
            function's instructions use. Ours come from the .OBJ fixups: an external by its address in
            analysis/PROG_names.txt (else the 5 hex digits ending its name, v7d800 / arr72350), our own
            functions by name, CONST by the base every reference agrees on.
  branches  for each jump inside the function, the addresses used by the block it lands on (up to
            its next branch) and whether that block returns. A jump to the epilogue in one build only,
            or to the block holding the other string, shows here (both bugs were found by hand once).
consts    small immediates and [reg+disp] field offsets (not stack slots) only one side uses: a
            wrong loop bound, size or struct field (fn_1832c's 4 cards vs 3, fn_16eac's +0x20 vs +2)
            shows here; a constant one build keeps in a register shows too. Reported, not counted
            as DIFFER.
writes    the subset of those addresses the side STORES to (a memory operand capstone marks as
            written: mov dest, pop, xchg, add/sub/inc/... in place). An address the refs census sees
            on both sides can still be read-only in one build - the players_ahead bug (v7d878 never
            set) and fn_173c4's wrong field are of that class, and neither census caught them.
DIFFER (an address only one side uses: `ours only` / `game only`) is a behaviour difference until
shown otherwise; `counts` (the same addresses, used a different number of times: a cached global, a
shared address computation) usually is not. Small numeric
constants that happen to fall in the image's address range show as refs on both sides and cancel.
"""
import bisect
import collections
import difflib
import json
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.join(ROOT, 'reverse-engineering', 'tools', 'wc'))
import capstone                      # noqa: E402
import libmatch as L                 # noqa: E402
import objmatch as M                 # noqa: E402

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
md.detail = True
LO, HI = 0x10000, 0xa0000
LIB = 0x40000                        # the linked library code starts about here (clib, audio)


def norm(s):
    return re.sub(r'0x[0-9a-f]{4,}', '@', s).replace('- @', '+ @')  # masked fixup, see fndiff.norm            # the LE image's objects (code 0x10000, data 0x60000)


def fields(ins):
    """(offset in the instruction, value, kind) of its 32-bit disp / imm fields; kind 'br' a branch
    target, 'idx' a displacement used with a register (array base + folded index), 'tab' a jump
    table, 'imm' an immediate (a constant or the address constant a fixup fills), '' a plain
    memory displacement"""
    out = []
    br = ins.group(capstone.CS_GRP_JUMP) or ins.group(capstone.CS_GRP_CALL)
    if ins.disp_size == 4:
        for op in ins.operands:
            if op.type == capstone.x86.X86_OP_MEM:
                kind = 'tab' if br else 'idx' if op.mem.base or op.mem.index else ''
                out.append((ins.disp_offset, op.mem.disp & 0xFFFFFFFF, kind))
                break
    if ins.imm_size == 4 or (br and ins.imm_offset):
        for op in ins.operands:
            if op.type == capstone.x86.X86_OP_IMM:
                out.append((ins.imm_offset, op.imm & 0xFFFFFFFF, 'br' if br else 'imm'))
                break
    return out


def written(ins):
    """does the operand carrying the disp/imm that fields() picked get written? (capstone's access
    flags: mov dest, pop dest, xchg, and add/sub/or/inc/... on memory are READ|WRITE or WRITE)"""
    for op in ins.operands:
        if op.type == capstone.x86.X86_OP_MEM:
            return bool(op.access & capstone.CS_AC_WRITE)
    return False


def blocks_sig(ins_refs, idx, ends):
    """addresses used from instruction idx to the end of its block, + 'RET' if the block returns"""
    sig = []
    for k in range(idx, len(ins_refs)):
        ins, refs = ins_refs[k]
        sig.extend(refs)
        if ins.mnemonic == 'ret':
            sig.append('RET')
            break
        if ins.group(capstone.CS_GRP_JUMP) or k + 1 >= ends:
            break
    return tuple(sorted(sig, key=str))     # the order inside a block is scheduling


def main():
    args = sys.argv[1:]
    verbose = '-v' in args
    args = [a for a in args if a != '-v']
    prog, obj = args[:2]
    meta = json.load(open(os.path.join(ROOT, '.work', 'le', prog + '.json')))
    code = open(os.path.join(ROOT, '.work', 'le', prog + '.obj1.bin'), 'rb').read()
    cbase = meta['objects'][0]['base']
    names = M.names(prog)
    addrs = sorted(a for a in names.values() if a < 0x60000)
    (_, recs), = list(L.modules(open(obj, 'rb').read()))
    segs = L.code_of(recs)
    pubs = sorted(p for p in M.publics(recs) if p[0] in segs)
    pubat = {nm: (s, o) for s, o, nm in M.publics(recs)}
    fix = {}
    for seg, at, loc, selfrel, kind, nm, disp in L.fixups_of(recs):
        if loc == 9:
            if not selfrel:
                disp += struct.unpack_from('<i', bytes(segs[seg][0]), at)[0]
            fix[(seg, at)] = (selfrel, kind, nm, disp)

    def addr_of(sym):
        p = M.plain(sym)
        if p in names:
            return names[p]
        # the address ends the name, maybe with a one-letter suffix (sp7b230b, cfg77128w): of the
        # 5-digit readings take one inside the image
        for pat in (r'([0-9a-f]{5})$', r'([0-9a-f]{5})[a-z]$'):
            m = re.search(pat, p)
            if m and LO <= int(m.group(1), 16) < 0x90000:
                return int(m.group(1), 16)
        return None

    def code_target(seg, off):
        best = None
        for s, o, nm in pubs:
            if s == seg and o <= off:
                best = (o, nm)
        if not best or addr_of(best[1]) is None:
            return None
        return addr_of(best[1]) + off - best[0]

    # one base per data segment: the majority over every fixup in the object (matches and near
    # misses alike; objmatch reports the same base from the matches)
    votes = collections.defaultdict(collections.Counter)
    for (seg, at), (sr, kd, nm, dp) in fix.items():
        if kd == 'seg' and nm not in segs:
            fn = None
            for s, o, pn in pubs:
                if s == seg and o <= at:
                    fn = (o, pn)
            if fn and addr_of(fn[1]) is not None:
                gat = addr_of(fn[1]) + at - fn[0]
                votes[nm][struct.unpack_from('<I', code, gat - cbase)[0] - dp] += 1
    base = {nm: c.most_common(1)[0][0] for nm, c in votes.items()}

    def resolve(seg, at):
        sr, kd, nm, dp = fix[(seg, at)]
        if kd == 'ext' and nm in pubat:
            s2, o2 = pubat[nm]
            if s2 in segs:
                t = code_target(s2, o2)
                return None if t is None else t + dp
            kd, nm, dp = 'seg', s2, o2 + dp
        if kd == 'ext':
            t = addr_of(nm)
            return None if t is None else t + dp
        if kd == 'seg':
            if nm in segs:
                return code_target(nm, dp)
            return base[nm] + dp if nm in base else None
        return None

    if len(args) > 2:
        todo = []
        for a in args[2:]:
            nm, _, ad = a.partition('=')
            todo.append((nm, int(ad, 16) if ad else names.get(nm)))
    else:
        todo = []
        for line in open(os.path.join(ROOT, 'src', 'nearmiss.txt')):
            f = line.split()
            if len(f) >= 3 and 'MATCH' not in line and 'ASM:' not in line:
                # the address follows the row count, which may read "282 -> 276"; the name follows it
                a = next((i for i, x in enumerate(f[:5]) if re.fullmatch(r'0x[0-9a-f]{5}', x)), None)
                if a is not None and a + 1 < len(f) and re.fullmatch(r'[a-z_][a-z_0-9]*', f[a + 1]):
                    todo.append((f[a + 1], int(f[a], 16)))

    def game_end(addr, size, ours, mask):
        """the end of the game's function: its ret aligned with our last ret (the game's may be longer)"""
        A = list(md.disasm(bytes(b if not m else 0x90 for b, m in zip(ours, mask)), addr))
        B = list(md.disasm(code[addr - cbase:addr - cbase + size * 2 + 64], addr))
        rets = [k for k, i in enumerate(A) if i.mnemonic == 'ret']
        if not rets:
            return addr + size
        sm = difflib.SequenceMatcher(None, [norm(i.mnemonic + ' ' + i.op_str) for i in A],
                                     [norm(i.mnemonic + ' ' + i.op_str) for i in B], autojunk=False)
        ia = rets[-1]
        jb = 0
        for tag, i1, i2, j1, j2 in sm.get_opcodes():
            if i1 <= ia < i2:
                jb = j1 + min(ia - i1, max(j2 - j1 - 1, 0))
        for k in range(max(jb - 3, 0), len(B)):
            if B[k].mnemonic == 'ret':
                return B[k].address + B[k].size
        return addr + size

    data_names = sorted((a, n) for n, a in names.items() if a >= 0x60000)

    def array_of(v):
        """an indexed access by the named object it falls in (its displacement folds the index)"""
        k = bisect.bisect_right(data_names, (v, '~')) - 1
        return 'in ' + data_names[k][1] if k >= 0 and v - data_names[k][0] < 0x1000 else 'in 0x%x' % (v & ~0xff)

    def label(v):
        if v == 'RET':
            return 'RET'
        if isinstance(v, str):
            return v
        inv = [n for n, a in names.items() if a == v]
        return '0x%x%s' % (v, '(%s)' % inv[0] if inv else '')

    flagged = 0
    nconst = [0]
    nwrite = [0]
    for name, addr in todo:
        hit = [p for p in pubs if p[2] == name or M.plain(p[2]) == name]
        if not hit or not addr:
            print('%-20s not in the object / no address' % name)
            continue
        seg, off, pname = hit[0]
        end = next((q[1] for q in pubs if q[0] == seg and q[1] > off), len(segs[seg][0]))
        size = end - off
        gend = min(game_end(addr, size, bytes(segs[seg][0][off:end]), segs[seg][1][off:end]),
                   next((a for a in addrs if a > addr), 1 << 32))
        sides = []
        for which in ('ours', 'game'):
            if which == 'ours':
                buf = bytes(segs[seg][0][off:end])
            else:
                buf = code[addr - cbase:gend - cbase]
            insl = list(md.disasm(buf, addr))
            lim = addr + len(buf)
            ir = []
            wr = collections.Counter()
            consts = collections.Counter()
            for ins in insl:
                # small constants: immediates (loop bounds, sizes, enum values) and the displacement
                # of a [reg+disp] that is not a stack slot (a struct field); masked fixup bytes skipped
                for op in ins.operands:
                    if op.type == capstone.x86.X86_OP_IMM and not (ins.group(capstone.CS_GRP_JUMP)
                                                                   or ins.group(capstone.CS_GRP_CALL)):
                        v = op.imm & 0xFFFFFFFF
                        fo = off + ins.address - addr + ins.imm_offset
                        if v < LO and not (which == 'ours' and (seg, fo) in fix):
                            consts['#0x%x' % v] += 1
                    elif op.type == capstone.x86.X86_OP_MEM and op.mem.base and \
                            ins.reg_name(op.mem.base) not in ('esp', 'ebp') and 0 < op.mem.disp < 0x400:
                        fo = off + ins.address - addr + ins.disp_offset
                        if not (which == 'ours' and (seg, fo) in fix):   # a global's addend, not a field
                            consts['+0x%x' % op.mem.disp] += 1
                refs = []
                for fo, v, kind in fields(ins):
                    at = off + ins.address - addr + fo
                    br = kind == 'br'
                    if kind == 'tab':
                        continue                            # a jump table: placed differently
                    if which == 'ours' and (seg, at) in fix:
                        t = resolve(seg, at)
                        if t is not None and kind == 'idx':
                            t = array_of(t)
                        refs.append(t if t is not None else '?' + fix[(seg, at)][2])
                    elif kind == 'idx' and LO <= v < HI:
                        refs.append(array_of(v))
                    elif br and addr <= v < lim:
                        continue                            # a branch inside the function
                    elif br and which == 'ours':
                        t = code_target(seg, off + v - addr)  # our own function, no fixup
                        refs.append(t if t is not None else '?call')
                    elif LO <= v < HI:
                        refs.append(v)
                for fo, v, kind in fields(ins):
                    # an immediate is a value, never a written address: `mov [v7d800], @players`
                    # writes v7d800 and only READS the players address (the iter-69 false positive)
                    if kind not in ('br', 'tab', 'imm') and written(ins):
                        at = off + ins.address - addr + fo
                        if which == 'ours' and (seg, at) in fix:
                            t = resolve(seg, at)
                            if t is not None and kind == 'idx':
                                t = array_of(t)
                            wr[t if t is not None else '?' + fix[(seg, at)][2]] += 1
                        elif kind == 'idx' and LO <= v < HI:
                            wr[array_of(v)] += 1
                        elif LO <= v < HI:
                            wr[v] += 1
                ir.append((ins, refs))
            start = {ins.address: k for k, (ins, _) in enumerate(ir)}
            refs = collections.Counter(r for _, rs in ir for r in rs)
            brs = collections.Counter()
            for ins, _ in ir:
                if ins.group(capstone.CS_GRP_JUMP) and ins.operands and \
                        ins.operands[0].type == capstone.x86.X86_OP_IMM:
                    t = ins.operands[0].imm
                    if addr <= t < lim and t in start:
                        brs[(ins.mnemonic in ('jmp',), blocks_sig(ir, start[t], len(ir)))] += 1
            sides.append((refs, brs, consts, wr))
        (ro, bo, co, wo), (rg, bg, cg, wg) = sides
        c_ours, c_game = set(co) - set(cg), set(cg) - set(co)
        r_ours, r_game = ro - rg, rg - ro
        b_ours, b_game = bo - bg, bg - bo
        # a branch whose target block is just shaped differently (same addresses) is not news
        # a jump target block only one side has at all (by its addresses, jmp or conditional ignored)
        # and that uses an address or returns: a block the other build never jumps to
        so_b = {sig for (_, sig) in bo if sig}
        sg_b = {sig for (_, sig) in bg if sig}
        nb = len(so_b ^ sg_b)
        # library calls: our externals without a name in PROG_names.txt against unnamed game calls
        # above the program's own code (the C/audio library) - reported, not counted
        lib = [r for r in ro if isinstance(r, str) and r.startswith('?')]
        # unnamed addresses in [LIB, 0x60000): library calls AND unnamed CONST literals. Filter them
        # on BOTH sides - our literals resolve to the same addresses through their segment fixup, so
        # filtering only the game's side made every MATCHed function that prints a string DIFFER.
        unamed = lambda r: isinstance(r, int) and LIB <= r < 0x60000 and label(r) == '0x%x' % r
        libg = [r for r in rg if unamed(r)]
        libo = [r for r in ro if unamed(r)]
        so, sg = set(ro) - set(lib) - set(libo), set(rg) - set(libg)
        bad = so != sg                   # an address only one side uses at all
        counts = not bad and bool(r_ours or r_game)   # same addresses, used a different number of times
        flagged += bad
        print('%-20s 0x%06x  refs %4d/%4d  %s  jump-target blocks one side only: %d'
              % (name, addr, sum(ro.values()), sum(rg.values()),
                 'DIFFER' if bad else 'counts' if counts else 'same  ', nb))
        for r, c in sorted(r_ours.items(), key=str):
            if bad and (r in rg or r in lib) and not verbose:
                continue
            print('    ours %s %dx %s' % ('only ' if r not in rg else 'more ', c, label(r)))
        for r, c in sorted(r_game.items(), key=str):
            if bad and (r in ro or r in libg) and not verbose:
                continue
            print('    game %s %dx %s' % ('only ' if r not in ro else 'more ', c, label(r)))
        w_ours, w_game = set(wo) - set(wg), set(wg) - set(wo)
        if w_ours or w_game:
            nwrite[0] += 1
            print('    writes ours only: %s | game only: %s   (a store the other build never makes)'
                  % (' '.join(sorted(('?' if isinstance(x, str) and x.startswith('?') else label(x)
                                      for x in w_ours))) or '-',
                     ' '.join(sorted(label(x) for x in w_game)) or '-'))
        if c_ours or c_game:
            nconst[0] += 1
            print('    consts ours only: %s | game only: %s   (#imm, +field; a register-held constant shows too)'
                  % (' '.join(sorted(c_ours)) or '-', ' '.join(sorted(c_game)) or '-'))
        if verbose or nb:
            for sig in sorted(so_b - sg_b, key=str):
                print('    jumps ours -> %s' % ' '.join(label(x) for x in sig))
            for sig in sorted(sg_b - so_b, key=str):
                print('    jumps game -> %s' % ' '.join(label(x) for x in sig))
        if verbose:
            for (j, s), c in sorted(b_ours.items(), key=str):
                print('    branch ours %s-> %s' % ('jmp ' if j else '', ' '.join(label(x) for x in s) or '(no addresses)'))
            for (j, s), c in sorted(b_game.items(), key=str):
                print('    branch game %s-> %s' % ('jmp ' if j else '', ' '.join(label(x) for x in s) or '(no addresses)'))
    print('%d of %d functions reference different addresses; %d use constants only one side has; '
          '%d store to an address the other never stores to'
          % (flagged, len(todo), nconst[0], nwrite[0]))


if __name__ == '__main__':
    main()
