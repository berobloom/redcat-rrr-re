#!/usr/bin/env python3
"""stackmap.py: what our compiled function does with its stack frame - the slots it touches and the
holes it leaves.

usage: stackmap.py FILE.OBJ PROG NAME [NAME...] [-v]

A near miss whose rows are only stack displacements (two slot numbers swapped, or one slot moved) is
a frame-shape question, not a spelling question: WCC puts the 4-byte scalars in a band at the top of
the frame, then a gap (its own FP temporaries / alignment), then the things it will not put in that
band - local arrays and 8-byte objects - at the next 8-byte-aligned offset. Unused locals, dead
stores, propagated constants and `register` cost no frame space. See discoveries "Toolchain: how WCC
lays out a function's stack frame" (that map is what solved cos_sin_init 0x13c34, 2 rows -> MATCH).

Prints per public: frame size, the 4-byte slots it touches (high to low) and the holes below the top
of the band. With -v, also the instruction that touches each slot. Fixup bytes are masked as in
fndiff/objmatch, so a `mov eax,@` that loads a global is not mistaken for a frame access.
"""
import os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import libmatch as L
import objmatch as M
import fndiff as F

obj, prog = sys.argv[1], sys.argv[2]
want = [n for n in sys.argv[3:] if not n.startswith('-')]
verbose = '-v' in sys.argv

(_, recs), = list(L.modules(open(obj, 'rb').read()))
segs = L.code_of(recs)
pubs = sorted(p for p in M.publics(recs) if p[0] in segs)

SLOT = re.compile(r'\[esp \+ (0x[0-9a-f]+|[0-9]+)\]')

for name in want:
    hit = [p for p in pubs if p[2] == name or M.plain(p[2]) == name]
    if not hit:
        print('%-24s not in %s' % (name, os.path.basename(obj)))
        continue
    seg, off, pname = hit[0]
    end = next((q[1] for q in pubs if q[0] == seg and q[1] > off), len(segs[seg][0]))
    buf, mask = segs[seg]
    ours = bytes(b if not mask[off + k] else 0x90 for k, b in enumerate(buf[off:end]))
    txt = [t for _, _, t in F.dis(ours, 0x1000, 0x1000 + len(ours))]
    frames = [int(m.group(1), 0) for t in txt for m in [re.match(r'sub esp, (0x[0-9a-f]+|[0-9]+)$', t)] if m]
    pushes = sum(1 for t in txt if re.match(r'push ', t)) - sum(1 for t in txt if re.match(r'pop ', t))
    used = {}
    for t in txt:
        for m in SLOT.finditer(t):
            used.setdefault(int(m.group(1), 0) & ~3, []).append(t)
    if not used:
        print('%-24s frame=%s pushes=%d  no esp-relative slots (globals/registers only)' %
              (pname, hex(frames[0]) if frames else '-', pushes))
        continue
    top = max(used)
    holes = [s for s in range(top - 4, -1, -4) if s not in used]
    runs, cur = [], []
    for s in sorted(holes, reverse=True):
        if cur and s == cur[-1] - 4:
            cur.append(s)
        else:
            if cur:
                runs.append((cur[0], cur[-1]))
            cur = [s]
    if cur:
        runs.append((cur[0], cur[-1]))
    print('%-24s frame=%s pushes=%d  slots: %s  holes: %s' %
          (pname, hex(frames[0]) if frames else '-', pushes,
           ' '.join('%x' % s for s in sorted(used, reverse=True)),
           ' '.join('%x-%x' % r for r in runs) or '-'))
    if verbose:
        for s in sorted(used, reverse=True):
            for t in used[s]:
                print('    [esp+%-4s] %s' % (hex(s)[2:], t))
