#!/usr/bin/env python3
"""dataplace.py: the data-object address of every data segment of real objects, for blob.py --obj.

usage: dataplace.py FILE.OBJ@CODEADDR ...     prints `--obj FILE.OBJ@CODEADDR,SEG=ADDR,...` per object

A module's code reaches its own data segments through segment-relative fixups, and Watcom keeps the
fixup's addend in the object's bytes. At such a site the game's code holds segment address + addend,
so the segment's address is the LE fixup's target there (the game's own relocation record for that
site) minus the addend - the object's dword, plus the record's displacement for the Fastgraph ASM
modules, which keep it there: exact, whatever the segment holds
(a 4-byte seed, all zeros, pointers). Every site that reaches a segment has to give the same address,
or the object is refused. Watcom C reaches a module's own data publics through an EXTDEF of the same
name (clib exit's ___atexit), and those count the same way. A segment only reached from another data
segment of the same module is read once that segment is placed (strerror: _DATA's pointers into
CONST). The other direction: a segment whose own fixups aim at something placed - an XI/YI entry
pointing at the module's code, noefgfmt's two pointers to _no_support_loaded_ - sits where the
game's LE fixups to those targets agree on one start (the candidate starts of every such fixup,
intersected). A segment no fixup reaches falls back to tools/link/dataobj.py's content search, used
only when it finds one place. Anything else is reported and the object left out.

Per segment stderr says how it was placed: `self N` (N agreeing fixup sites), `pointers`, `content`
or `none`.
_BSS is placed the same way (blob.py then needs a --dseg _BSS@ADDR:BSS block).
"""
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(os.path.dirname(HERE), 'wc'))
sys.path.insert(0, os.path.dirname(HERE))
import leparse                           # noqa: E402
import libmatch as L                     # noqa: E402
from libobj import info                  # noqa: E402
from dataobj import game_image, search   # noqa: E402


def le_targets():
    """{source address: target address} of every LE fixup of RRR.EXE"""
    from dataobj import EXE
    le = leparse.LE(open(EXE, 'rb').read())
    out = {}
    for o in le.objects:
        for k in range(o['page_count']):
            for st, so, tobj, toff in le.fixups(o['page_index'] + k):
                out[o['base'] + k * le.page_size + so] = le.objects[tobj - 1]['base'] + toff
    return out


def place(path, caddr, tgt, dimg, dmask, dbase):
    segs, pubs, _ = info(path)
    (_, recs), = list(L.modules(open(path, 'rb').read()))
    bufs = L.code_of(recs, only_code=False)
    cls = {n: c for n, c, s in segs}
    want = [n for n, c, s in segs if c.upper() != 'CODE' and s]
    text = [n for n, c, s in segs if c.upper() == 'CODE'][0]
    own = {n: (s, o) for s, o, n in pubs + info.local_items}   # its own publics, reached as externals
    at = {text: caddr}
    how = {}
    fix = L.fixups_of(recs, only_code=False)
    bysite = {}                                      # target address -> LE fixup sites aiming there
    for s, t in tgt.items():
        if dbase <= s < dbase + len(dimg):
            bysite.setdefault(t, []).append(s)
    while True:
        votes, rev = {}, {}
        for seg, off, loc, selfrel, kind, nm, disp in fix:
            if loc != 9 or selfrel:
                continue
            if kind == 'seg':
                tseg, toff = nm, 0
            elif kind == 'ext' and nm in own:
                tseg, toff = own[nm]
            else:
                continue
            add = struct.unpack_from('<i', bytes(bufs[seg][0]), off)[0] + disp
            if seg in at and tseg in want and tseg not in at:      # forward: the site is placed
                site = at[seg] + off
                if site not in tgt:
                    raise SystemExit('%s: no LE fixup at %#x, where its %s fixup is' % (path, site, nm))
                votes.setdefault(tseg, []).append((tgt[site] - add - toff) & 0xFFFFFFFF)
            elif tseg in at and seg in want and seg not in at:      # reverse: XI/YI entries, pointers
                cand = {x - off for x in bysite.get((at[tseg] + toff + add) & 0xFFFFFFFF, [])}
                rev[seg] = rev[seg] & cand if seg in rev else cand
        for nm, cand in rev.items():                 # every pointer of the segment has to agree
            if len(cand) == 1 and nm not in votes:
                votes[nm] = sorted(cand)
                how[nm] = 'pointers'
        if not votes:
            break
        for nm, vs in votes.items():
            if len(set(vs)) != 1:
                raise SystemExit('%s: %s reached at %d addresses: %s'
                                 % (path, nm, len(set(vs)), ' '.join('%#x' % v for v in sorted(set(vs)))))
            if not dbase <= vs[0] < dbase + len(dimg):
                raise SystemExit('%s: %s at %#x, outside the data object' % (path, nm, vs[0]))
            at[nm] = vs[0]
            how.setdefault(nm, 'self %d' % len(vs))
    for nm in want:
        if nm in at:
            continue
        buf, omask = bufs.get(nm, (b'', b''))
        h = search(dimg, dmask, buf, omask) if len(buf) else []
        if len(h) == 1:
            at[nm] = dbase + h[0]
            how[nm] = 'content'
        else:
            how[nm] = 'none (%d content hits)' % len(h)
    return {nm: at[nm] for nm in want if nm in at}, how, want, cls


def main():
    tgt = le_targets()
    dbase, dimg, dmask = game_image(2)
    out = []
    for arg in sys.argv[1:]:
        path, ca = arg.rsplit('@', 1)
        addrs, how, want, cls = place(path, int(ca, 16), tgt, dimg, dmask, dbase)
        for nm in want:
            print('%-28s %-7s %s  %s' % (os.path.basename(path), nm,
                                         '%#x' % addrs[nm] if nm in addrs else '?', how[nm]), file=sys.stderr)
        if len(addrs) != len(want):
            print('  left out: %s' % os.path.basename(path), file=sys.stderr)
            continue
        out.append('--obj %s@%s%s' % (path, ca, ''.join(',%s=%x' % (n, addrs[n]) for n in want)))
    print(' '.join(out))


if __name__ == '__main__':
    main()
