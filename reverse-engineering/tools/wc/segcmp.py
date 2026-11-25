#!/usr/bin/env python3
"""segcmp.py A.OBJ B.OBJ: do two objects agree, ignoring the bytes the linker fills?

cmp/md5 on .OBJ files is useless across compiler builds: the OMF records and the fixup bytes differ
even when the code is identical. This rebuilds each module's segments with libmatch.code_of and
compares them with the linker-filled bytes masked out. Exit 0 when they agree.
"""
import sys

sys.path.insert(0, __import__('os').path.dirname(__import__('os').path.abspath(__file__)))
import libmatch as L


def segs(path):
    out = {}
    for _, recs in L.modules(open(path, 'rb').read()):
        for name, (buf, mask) in L.code_of(recs, only_code=False).items():
            out[name] = (bytes(buf), mask)
    return out


a, b = segs(sys.argv[1]), segs(sys.argv[2])
bad = 0
for s in sorted(set(a) | set(b)):
    if s not in a or s not in b:
        print(f"  {s}: only in one object"); bad += 1; continue
    (ba, ma), (bb, mb) = a[s], b[s]
    if len(ba) != len(bb):
        print(f"  {s}: {len(ba)} vs {len(bb)} bytes"); bad += 1; continue
    d = [i for i in range(len(ba)) if ba[i] != bb[i] and not ma[i] and not mb[i]]
    print(f"  {s}: {len(ba)} bytes, {len(d)} diffs" + (f" at {d[:6]}" if d else ''))
    bad += len(d)
print('IDENTICAL' if not bad else f'{bad} diffs')
sys.exit(1 if bad else 0)
