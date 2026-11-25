#!/usr/bin/env python3
"""dataobj.py: where each data-carrying module's data segments sit in RRR's data object.

usage: dataobj.py [-v] TARGET ...      TARGET: a .LIB (every module) or a .OBJ

For every module of every TARGET, each non-code segment holding LEDATA bytes is searched for in the
game's data object with the linker-filled bytes masked out on both sides (the module's own FIXUPP
sites and RRR's LE fixup sites). One hit gives the segment's address; the code segment is searched
the same way in the code object, which says where the module linked. (The 160 modules libplace.py
extracts take about six minutes.)

The table is sorted by data address. Columns: address, segment name, class, declared length, the
LEDATA extent that was matched, module, its code address. -v lists the pinned segments per name.

What it showed on 2026-11-25: the 23 segments it pins land in code-address order inside each segment
name (CONST 10, CONST2 2, _DATA 9, DATA32 2, none out of order), so WLINK's data object is one run
per segment name and inside a run the modules come in the order they were fed - the same order as
the code object. WLINK's own map (RRR.map) names the runs and their order (the order WLINK first
heard each name in, 2026-11-26; blob.py's first object names them):

    CONST CONST2 _DATA XIB XI XIE YIB YI YIE DATA32 _BSS STACK

XIB/XIE and YIB/YIE are zero-size runs WLINK defines itself at the ends of the XI and YI runs, which
is why clib's cinit needs the XI/YI-carrying modules linked to get its four bookkeeping fixups right
(blob.py --dseg). Segments whose bytes are all pointers or zeros (the XI/YI segments, small _DATA
initialized by other modules) cannot be pinned here; use the run order and the neighbour addresses
to infer them, and say so in any map built from this.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'wc'))
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import leparse                                  # noqa: E402
import libmatch as L                            # noqa: E402
from libobj import info_bytes, raw_modules      # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
EXE = os.path.join(ROOT, '.work', 'iso', 'RRR.EXE')


def game_image(objnum):
    d = open(EXE, 'rb').read()
    le = leparse.LE(d)
    obj = le.objects[objnum - 1]
    img = bytearray(leparse.LE(d).object_image(obj))
    mask = bytearray(len(img))
    for k in range(obj['page_count']):
        for st, so, tobj, toff in le.fixups(le.objects[objnum - 1]['page_index'] + k):
            at = k * le.page_size + so
            for i in range(4):
                if at + i < len(mask):
                    mask[at + i] = 1
    return obj['base'], img, mask


def search(img, gmask, buf, omask, off=0, limit=20):
    """positions in img where buf occurs, bytes masked on either side skipped; returns img offsets"""
    anc, aoff = None, 0
    start = 0
    while start < len(buf):
        end = start
        while end < len(buf) and not omask[end]:
            end += 1
        if end - start >= 6 and len(set(buf[start:end])) > 1:
            n = min(end - start, 24)
            if anc is None or n > len(anc):
                anc, aoff = bytes(buf[start:start + n]), start
        start = end + 1
    if anc is None:
        return []
    img, hits, p = bytes(img), [], 0
    while len(hits) < limit:
        p = img.find(anc, p)
        if p < 0:
            break
        q = p - aoff
        ok = q >= 0 and q + len(buf) <= len(img)
        if ok:
            for i in range(len(buf)):
                if omask[i] or gmask[q + i]:
                    continue
                if buf[i] != img[q + i]:
                    ok = False
                    break
        if ok:
            hits.append(q - off)
        p += 1
    return hits


def modules_of(target):
    if os.path.splitext(target)[1].upper() == '.LIB':
        return list(raw_modules(open(target, 'rb').read()))
    return [(os.path.splitext(os.path.basename(target))[0], None)]


def main():
    verbose = '-v' in sys.argv
    targets = [a for a in sys.argv[1:] if a != '-v']
    cbase, cimg, cmask = game_image(1)          # the code object, its LE fixup sites masked
    dbase, dimg, dmask = game_image(2)
    rows, loose = [], []
    for target in targets:
        tag = os.path.splitext(os.path.basename(target))[0].lower()
        for mod, raw in modules_of(target):
            if not mod:
                continue
            data = raw if raw is not None else open(target, 'rb').read()
            (_, recs), = list(L.modules(data))
            segs = L.code_of(recs, only_code=False)
            cls = {s[0]: s[1] for s in info_bytes(data)[0]}
            size = {s[0]: s[2] for s in info_bytes(data)[0]}
            code = segs.get('_TEXT', (b'', b''))[0]
            at = L.find(cimg, code, L.code_of(recs)['_TEXT'][1]) if len(code) > 8 else []
            codeat = at[0] + cbase if len(at) == 1 else None
            for sn, (buf, omask) in segs.items():
                if cls.get(sn, '').upper() == 'CODE' or not len(buf):
                    continue
                h = search(dimg, dmask, buf, omask)
                if len(h) == 1:
                    rows.append((dbase + h[0], sn, cls.get(sn, '?'), size.get(sn, len(buf)), len(buf), mod, tag, codeat))
                else:
                    loose.append((sn, cls.get(sn, '?'), size.get(sn, len(buf)), len(buf), mod, tag, codeat, h))
    rows.sort()
    print('%-9s %-8s %-5s %7s %7s  %-16s %-9s %s' % ('data', 'seg', 'class', 'decl', 'bytes', 'module', 'code', 'lib'))
    for a, sn, cl, dec, n, mod, tag, codeat in rows:
        print('%#09x %-8s %-5s %7d %7d  %-16s %-9s %s' % (a, sn, cl, dec, n, mod, '%#x' % codeat if codeat else '?', tag))
    print('\nnot pinned (%d):' % len(loose))
    for sn, cl, dec, n, mod, tag, codeat, h in sorted(loose, key=lambda r: (r[6] or 0, r[0])):
        print('%-8s %-5s %7d %7d  %-16s %-9s %s  %s' % (sn, cl, dec, n, mod, '%#x' % codeat if codeat else '?', tag,
                                                        ' '.join('%#x' % (dbase + x) for x in h) or 'no hit'))

    # the merge-order check: per segment name, are the pinned segments in code order?
    print('\nper segment name, data order vs code order:')
    for sn in sorted({r[1] for r in rows}):
        seq = [(r[7], r[0], r[5]) for r in rows if r[1] == sn and r[7]]
        bad = sum(1 for i in range(1, len(seq)) if seq[i][0] < seq[i - 1][0])
        print('  %-8s %2d pinned, %d out of code order' % (sn, len(seq), bad))
        if verbose:
            for c, d, m in seq:
                print('      code %#x  data %#x  %s' % (c, d, m))


if __name__ == '__main__':
    main()
