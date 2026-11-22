#!/usr/bin/env python3
"""objdiff.py: one function of a compiled OMF .OBJ next to the same function in an LE program, as two listings.

usage: objdiff.py PROG FILE.OBJ NAME [0xADDR]

NAME is the public as the .OBJ has it (crcupdate_) or without Watcom's decoration; ADDR defaults to the name's
address in analysis/PROG_names.txt. Lines are disassembled with objdump; bytes that fixups fill print as ..
on our side. A '|' marks lines whose bytes differ (fixup bytes ignored).
"""
import json
import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import libmatch as L  # noqa: E402
import objmatch as M  # noqa: E402


def listing(buf, vma):
    with tempfile.NamedTemporaryFile(suffix='.bin', delete=False) as f:
        f.write(bytes(buf))
    try:
        out = subprocess.run(['objdump', '-D', '-b', 'binary', '-mi386', '--adjust-vma=0x%x' % vma, f.name],
                             capture_output=True, text=True).stdout
    finally:
        os.unlink(f.name)
    rows = []
    for line in out.split('\n'):
        parts = line.split('\t')
        if len(parts) >= 3 and parts[0].strip().endswith(':'):
            rows.append((int(parts[0].strip()[:-1], 16), parts[1].split(), parts[2].strip()))
    return rows


def main():
    if len(sys.argv) < 4:
        print(__doc__)
        return 2
    prog, obj, name = sys.argv[1:4]
    le = os.path.join(L.ROOT, '.work', 'le')
    meta = json.load(open(os.path.join(le, prog + '.json')))
    code = open(os.path.join(le, prog + '.obj1.bin'), 'rb').read()
    cbase = meta['objects'][0]['base']
    (_, recs), = list(L.modules(open(obj, 'rb').read()))
    segs = L.code_of(recs)
    pubs = sorted(p for p in M.publics(recs) if p[0] in segs)
    hit = [p for p in pubs if p[2] == name or M.plain(p[2]) == name]
    if not hit:
        print('no public %s in %s' % (name, obj))
        return 1
    seg, off, pname = hit[0]
    end = next((q[1] for q in pubs if q[0] == seg and q[1] > off), len(segs[seg][0]))
    addr = int(sys.argv[4], 16) if len(sys.argv) > 4 else M.names(prog).get(M.plain(pname))
    if addr is None:
        print('no address for %s: give one' % pname)
        return 1
    buf, mask = segs[seg]
    ours = listing(buf[off:end], addr)
    game = listing(code[addr - cbase:addr - cbase + (end - off) + 16], addr)
    o = [(a, ' '.join('..' if mask[off + a - addr + k] else b for k, b in enumerate(bs)), t) for a, bs, t in ours]
    n = max(len(o), len(game))
    for i in range(n):
        left = '%06x %-20s %-30s' % (o[i][0], o[i][1][:20], o[i][2][:30]) if i < len(o) else ' ' * 58
        right = '%06x %-20s %s' % (game[i][0], ' '.join(game[i][1])[:20], game[i][2][:30]) if i < len(game) else ''
        diff = i >= len(o) or i >= len(game) or any(x != '..' and x != y for x, y in
                                                    zip(o[i][1].split(), game[i][1])) or \
            len(o[i][1].split()) != len(game[i][1])
        print(left + (' | ' if diff else '   ') + right)
        if i >= len(o) and i > len(o) + 3:
            break
    return 0


if __name__ == '__main__':
    sys.exit(main())
