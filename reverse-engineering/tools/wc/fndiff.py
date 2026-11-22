#!/usr/bin/env python3
"""fndiff.py: one function of our .OBJ vs the game's, aligned instruction by instruction.

usage: fndiff.py PROG FILE.OBJ NAME [0xADDR]

Columns: OURS (the .OBJ) on the LEFT, the game on the RIGHT (iter 20 - an overnight prompt got this
backwards; check a full disassembly of one side before writing prose about a diff).

Like tools/wc/variants.py this ignores the bytes fixups fill and prints absolute addresses as '@',
but it lists the differing pairs instead of only counting them, and it aligns the two listings with
difflib so a 1-byte drift does not shift the whole function (the better of autojunk on and off: align()). A pair on one line means one side has
an instruction the other lacks.
"""
import difflib
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.join(ROOT, 'reverse-engineering', 'tools'))
sys.path.insert(0, os.path.join(ROOT, 'reverse-engineering', 'tools', 'wc'))
import capstone                      # noqa: E402
import libmatch as L                 # noqa: E402
import objmatch as M                 # noqa: E402

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


def norm(s):
    # A masked fixup (0x90 filler) reads as `[reg - 0x6f6f6f70]` where the game has `[reg + addr]`: the
    # same instruction, not a row (iter 39: 63 of car_laps' 65 rows). Real displacements are short
    # and never become @, so folding `- @` into `+ @` cannot hide a real difference.
    return re.sub(r'0x[0-9a-f]{4,}', '@', s).replace('- @', '+ @')


def dis(buf, vma, upto):
    return [(i.address, i.size, norm(i.mnemonic + ' ' + i.op_str)) for i in md.disasm(buf, vma)
            if i.address < upto]


def rows(ops):
    return sum(max(i2 - i1, j2 - j1) for t, i1, i2, j1, j2 in ops if t != 'equal')


def align(a, b):
    """difflib opcodes for the two instruction-text lists, the better of two alignments. difflib's
    default autojunk ignores lines that make up over 1% of a 200+ line list (`call @`, `mov eax,@`), and
    on long functions that misaligns whole blocks (iter 39: fn_17928 171 rows with it, 35 without);
    without it a few short functions align worse (netkeys_in 29 vs 48). Both are real alignments, so
    each count is an upper bound: keep the smaller."""
    best = None
    for junk in (True, False):
        ops = difflib.SequenceMatcher(None, a, b, autojunk=junk).get_opcodes()
        if best is None or rows(ops) < rows(best):
            best = ops
    return best


def main():
    if len(sys.argv) < 4:
        print(__doc__)
        return 2
    prog, obj, name = sys.argv[1:4]
    meta = json.load(open(os.path.join(L.ROOT, '.work', 'le', prog + '.json')))
    code = open(os.path.join(L.ROOT, '.work', 'le', prog + '.obj1.bin'), 'rb').read()
    cbase = meta['objects'][0]['base']
    names = M.names(prog)
    addr = int(sys.argv[4], 16) if len(sys.argv) > 4 else names.get(M.plain(name), 0)
    (_, recs), = list(L.modules(open(obj, 'rb').read()))
    segs = L.code_of(recs)
    pubs = sorted(p for p in M.publics(recs) if p[0] in segs)
    hit = [p for p in pubs if p[2] == name or M.plain(p[2]) == name]
    if not hit or not addr:
        print('%s not in obj / not named in %s_names.txt' % (name, prog))
        return 1
    seg, off, pname = hit[0]
    end = next((q[1] for q in pubs if q[0] == seg and q[1] > off), len(segs[seg][0]))
    buf, mask = segs[seg]
    ours = bytes(b if not mask[off + k] else 0x90 for k, b in enumerate(buf[off:end]))
    A = dis(ours, addr, addr + len(ours))
    B = dis(code[addr - cbase:addr - cbase + len(ours)], addr, addr + len(ours))
    n = 0
    for tag, i1, i2, j1, j2 in align([t[2] for t in A], [t[2] for t in B]):
        if tag == 'equal':
            continue
        n += max(i2 - i1, j2 - j1)
        for k in range(max(i2 - i1, j2 - j1)):
            a = A[i1 + k] if i1 + k < i2 else None
            b = B[j1 + k] if j1 + k < j2 else None
            print('%-8s %-30s | %-8s %s' % (hex(a[0]) if a else '', (a[2] if a else ''),
                                            hex(b[0]) if b else '', (b[2] if b else '')))
    print('%s: %d bytes, %d instruction diffs (addresses as @, fixup bytes masked)' % (name, len(ours), n))


if __name__ == '__main__':
    main()
