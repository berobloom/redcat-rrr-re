#!/usr/bin/env python3
"""nmclass.py: what kind of rows are a near miss's?

usage: nmclass.py PROG FILE.OBJ NAME 0xADDR [NAME 0xADDR ...]

Per function it splits fndiff's rows into:
  home  - same instruction with different registers/operands (a Watcom register or slot choice)
  shape - same mnemonic, different operand count or instruction (a real code difference)
  more  - an instruction only we have
  less  - an instruction only the game has
`home` rows say the C is right and the allocator differs; `less`/`more` rows say a statement or an
expression is spelled differently, which is the part worth spending C cycles on.
"""
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import capstone                     # noqa: E402
import fndiff as F                  # noqa: E402
import libmatch as L                # noqa: E402
import objmatch as M                # noqa: E402

REG = r'\b(eax|ecx|edx|ebx|esp|ebp|esi|edi|ax|cx|dx|bx|si|di|al|cl|dl|bl|ah|ch|dh|bh)\b'
DISP = r'(-?\s*0x[0-9a-f]+|@|\d+)'


def norm_regs(s):
    """register/slot/offset homes folded away: only the shape of the instruction is left."""
    s = re.sub(REG, 'R', s)
    s = re.sub(r'\[R [+-] @\]', '[R+off]', s)
    s = re.sub(r'\[R \+ R\]', '[R+R]', s)
    s = re.sub(r'\[R\*\d+ \+ @\]', '[R*+off]', s)
    s = re.sub(DISP, 'N', s)
    return re.sub(r'\s+', '', s)


def main():
    prog, obj = sys.argv[1], sys.argv[2]
    args = sys.argv[3:]
    meta = json.load(open(os.path.join(L.ROOT, '.work', 'le', prog + '.json')))
    code = open(os.path.join(L.ROOT, '.work', 'le', prog + '.obj1.bin'), 'rb').read()
    cbase = meta['objects'][0]['base']
    (_, recs), = list(L.modules(open(obj, 'rb').read()))
    segs = L.code_of(recs)
    pubs = sorted(p for p in M.publics(recs) if p[0] in segs)
    for name, addr in zip(args[::2], args[1::2]):
        addr = int(addr, 16)
        hit = [p for p in pubs if p[2] == name or M.plain(p[2]) == name]
        if not hit:
            print(f'{name}: not in {obj}')
            continue
        seg, off, pname = hit[0]
        end = next((q[1] for q in pubs if q[0] == seg and q[1] > off), len(segs[seg][0]))
        buf, mask = segs[seg]
        ours = bytes(b if not mask[off + k] else 0x90 for k, b in enumerate(buf[off:end]))
        A = F.dis(ours, addr, addr + len(ours))
        B = F.dis(code[addr - cbase:addr - cbase + len(ours)], addr, addr + len(ours))
        kind = {'home': 0, 'shape': 0, 'more': 0, 'less': 0}
        for tag, i1, i2, j1, j2 in F.align([t[2] for t in A], [t[2] for t in B]):
            if tag == 'equal':
                continue
            for k in range(max(i2 - i1, j2 - j1)):
                a = A[i1 + k] if i1 + k < i2 else None
                b = B[j1 + k] if j1 + k < j2 else None
                if a and b:
                    na, nb = norm_regs(a[2]), norm_regs(b[2])
                    kind['home' if na == nb else 'shape'] += 1
                else:
                    kind['more' if a else 'less'] += 1
        sizes = {k: 0 for k in kind}
        for tag, i1, i2, j1, j2 in F.align([t[2] for t in A], [t[2] for t in B]):
            if tag == 'equal':
                continue
            if i1 < i2:
                sizes['more' if i1 >= i2 or j1 >= j2 else 'shape'] += sum(t[1] for t in A[i1:i2])
            if j1 < j2:
                sizes['less' if i1 >= i2 or j1 >= j2 else 'shape'] += sum(t[1] for t in B[j1:j2])
        tot = sum(kind.values())
        print(f"{M.plain(name):20} {tot:>4} rows: home {kind['home']:>4}  shape {kind['shape']:>4}"
              f"  only-ours {kind['more']:>3} ({sizes['more']} B)  only-game {kind['less']:>3} ({sizes['less']} B)")


if __name__ == '__main__':
    main()
