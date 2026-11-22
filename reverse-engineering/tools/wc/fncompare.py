#!/usr/bin/env python3
"""fncompare.py: our compiled .OBJ function next to the game's, aligned instruction by instruction.

usage: fncompare.py PROG NAME 0xSTART 0xEND [FILE.OBJ] [--dump]

tools/wc/fndiff.py finds the first differing byte; this answers the two questions it cannot, which is
how to read a near miss that is *shorter* than the game's:
  * how many instructions each side has in the range (a missing instruction - e.g. an -oe merged
    store - versus one that differs only in its register), and
  * which pairs read the same but encode differently (rel8 vs rel32, AL's 5-byte `a0`/`a2`
    against `8a 15`/`88 15`).
Both listings are normalized: absolute addresses, displacements and the bytes fixups fill print as
[@]. The game's side is cut at END so one function never bleeds into the next. --dump prints only
our function's listing (what `tools/le/disasm.py` gives for the game). Known noise: an *immediate*
the linker fills (e.g. `mov edi, mnames`) still reads 0 on our side against the game's address, so a
block of those alone is not a difference - `src/check.sh`'s fixup lines already check those targets.
"""
import difflib
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(os.path.dirname(HERE), 'le'))
import capstone              # noqa: E402
import libmatch as L         # noqa: E402
import objmatch as M         # noqa: E402

ROOT = L.ROOT


def ours(obj, name):
    """lines 'ADDR BYTES MNEMONIC OPS', bytes fixups fill printed as .."""
    (_, recs), = list(L.modules(open(os.path.join(ROOT, obj), 'rb').read()))
    segs = L.code_of(recs)
    pubs = sorted(p for p in M.publics(recs) if p[0] in segs)
    for i, (seg, off, pub) in enumerate(pubs):
        if pub.rstrip('_') != name.rstrip('_'):
            continue
        buf, mask = segs[seg]
        nxt = [o for s, o, _ in pubs[i + 1:] if s == seg]
        end = len(buf) if not nxt else nxt[0]
        code = bytes(buf[off:end])
        out = []
        for ins in capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32).disasm(code, off):
            o = ins.address - off
            bs = ''.join('..' if mask[o + q] else '%02x' % code[o + q] for q in range(len(ins.bytes)))
            text = (ins.mnemonic + ' ' + ins.op_str).strip()
            # an immediate the linker fills is 0 here and an address in the game: both are '@'.
            # only 4-byte tails are checked (ponytail: a masked 2-byte immediate would print raw)
            tail = o + len(ins.bytes) - 4
            if len(ins.bytes) >= 4 and all(mask[tail:]) and '..' in bs:
                text = re.sub(r'(0x)?[0-9a-f]+\Z', '@', text)
            out.append('%06x %s %s' % (ins.address, bs, text))
        return out
    print('%s: no such public in %s' % (name, obj))
    return []


def norm(lines, end):
    out = []
    for line in lines:
        f = line.split(None, 2)
        if len(f) < 3:
            continue
        addr, body = f[0], ' '.join(re.split(r'\s+;', f[2])[0].split())
        nbytes = len(f[1]) // 2
        body = re.sub(r'0x[0-9a-f]+', '@', body)
        body = re.sub(r'\[\.\.[^\]]*\]', '[@]', body)
        body = re.sub(r'\[\d+\]', '[@]', body)
        if int(addr, 16) < end:
            out.append((addr, body, nbytes))
    return out


def main():
    pos = [a for a in sys.argv[1:] if not a.startswith('--')]
    if len(pos) < 4:
        print(__doc__)
        return 2
    prog, name, start, end = pos[0], pos[1], int(pos[2], 16), int(pos[3], 16)
    obj = pos[4] if len(pos) > 4 else '.work/src/RRR.OBJ'
    mine = ours(obj, name)
    if not mine:
        return 1
    if '--dump' in sys.argv:
        print('\n'.join(mine))
        return 0
    game = subprocess.run([os.path.join(ROOT, '.work', 'venv', 'bin', 'python'),
                           os.path.join(ROOT, 'reverse-engineering', 'tools', 'disasm.py'), prog, hex(start),
                           str(end - start)], capture_output=True, text=True).stdout.splitlines()
    game, mine = norm(game, end), norm(mine, end)
    sm = difflib.SequenceMatcher(None, [b for _, b, _ in game], [b for _, b, _ in mine])
    blocks = nlen = 0
    for op, i1, i2, j1, j2 in sm.get_opcodes():
        if op == 'equal':
            for k in range(i2 - i1):
                if game[i1 + k][2] != mine[j1 + k][2]:
                    nlen += 1
                    print('LEN %s %-2d %s %-2d  %s'
                          % (game[i1+k][0], game[i1+k][2], mine[j1+k][0], mine[j1+k][2],
                             game[i1+k][1]))
            continue
        blocks += 1
        print('@@ %s' % (game[i1][0] if i1 < len(game) else '?'))
        for _, b, _ in game[i1:i2]:
            print('  GAME %s' % b)
        for _, b, _ in mine[j1:j2]:
            print('  OURS %s' % b)
    print('%s: game %d insns, ours %d insns, %d diff blocks, %d length-only diffs'
          % (name, len(game), len(mine), blocks, nlen))
    return 0


if __name__ == '__main__':
    sys.exit(main())
