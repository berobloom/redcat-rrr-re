#!/usr/bin/env python3
"""lecmp.py: compare two LE executables region by region.

usage: lecmp.py OURS.EXE GAME.EXE [-v]

Regions: the MZ stub, the LE header, the object table, the object page map, the resident names
table, the fixup page table, the fixup records, then each object's pages (as the file stores them,
before fixups). Per region: bytes identical / bytes, and the first difference. Then the fixups as
a set of (source address, target address) pairs, which says whether the relocations agree even
where the fixup records are ordered or encoded differently. -v lists up to 20 differing header
fields and fixup pairs. The last line is the total: `LINK: N of M file bytes identical`.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import leparse  # noqa: E402

HDR = [('magic', 0x00, 'H'), ('byte/word order', 0x02, 'H'), ('format level', 0x04, 'I'),
       ('cpu', 0x08, 'H'), ('os', 0x0a, 'H'), ('module version', 0x0c, 'I'), ('module flags', 0x10, 'I'),
       ('pages', 0x14, 'I'), ('eip object', 0x18, 'I'), ('eip', 0x1c, 'I'), ('esp object', 0x20, 'I'),
       ('esp', 0x24, 'I'), ('page size', 0x28, 'I'), ('last page bytes', 0x2c, 'I'),
       ('fixup section size', 0x30, 'I'), ('fixup checksum', 0x34, 'I'), ('loader section size', 0x38, 'I'),
       ('loader checksum', 0x3c, 'I'), ('object table', 0x40, 'I'), ('objects', 0x44, 'I'),
       ('page map', 0x48, 'I'), ('iter pages', 0x4c, 'I'), ('resource table', 0x50, 'I'),
       ('resources', 0x54, 'I'), ('resident names', 0x58, 'I'), ('entry table', 0x5c, 'I'),
       ('directives', 0x60, 'I'), ('directives n', 0x64, 'I'), ('fixup page table', 0x68, 'I'),
       ('fixup records', 0x6c, 'I'), ('import modules', 0x70, 'I'), ('import modules n', 0x74, 'I'),
       ('import procs', 0x78, 'I'), ('page checksums', 0x7c, 'I'), ('data pages', 0x80, 'I'),
       ('preload pages', 0x84, 'I'), ('nonres names', 0x88, 'I'), ('nonres names len', 0x8c, 'I'),
       ('nonres checksum', 0x90, 'I'), ('auto data object', 0x94, 'I'), ('debug info', 0x98, 'I'),
       ('debug len', 0x9c, 'I'), ('instance preload', 0xa0, 'I'), ('instance demand', 0xa4, 'I'),
       ('heap size', 0xa8, 'I')]


def regions(d):
    le = leparse.LE(d)
    h = le.hdr
    g = lambda o: struct.unpack_from('<I', d, h + o)[0]
    r = [('MZ stub', 0, h), ('LE header', h, h + 0xac),
         ('object table', h + g(0x40), h + g(0x40) + 24 * g(0x44)),
         ('object page map', h + g(0x48), h + g(0x48) + 4 * g(0x14)),
         ('resident names', h + g(0x58), h + g(0x68)),
         ('fixup page table', h + g(0x68), h + g(0x6c)),
         ('fixup records', h + g(0x6c), h + g(0x70))]
    for o in le.objects:
        s, _ = le.page_file_offset(o['page_index'])
        e, _ = le.page_file_offset(o['page_index'] + o['page_count'] - 1)
        e += le.page_len(o['page_index'] + o['page_count'] - 1)
        r.append(('object %d pages' % o['num'], s, e))
    return le, r


def pairs(le):
    out = set()
    for o in le.objects:
        for k in range(o['page_count']):
            for stype, so, tobj, toff in le.fixups(o['page_index'] + k):
                out.add((o['base'] + k * le.page_size + so, stype, le.objects[tobj - 1]['base'] + toff))
    return out


def main():
    a, b = open(sys.argv[1], 'rb').read(), open(sys.argv[2], 'rb').read()
    verbose = '-v' in sys.argv
    la, ra = regions(a)
    lb, rb = regions(b)
    same_total = 0
    for (n, s1, e1), (_, s2, e2) in zip(ra, rb):
        x, y = a[s1:e1], b[s2:e2]
        n_eq = sum(1 for i in range(min(len(x), len(y))) if x[i] == y[i])
        first = next((i for i in range(min(len(x), len(y))) if x[i] != y[i]), None)
        same_total += n_eq if (s1, e1) == (s2, e2) else 0
        size = '%d' % len(y) if len(x) == len(y) else '%d (ours %d)' % (len(y), len(x))
        where = '' if (s1, e1) == (s2, e2) else '  [ours at %#x, game at %#x]' % (s1, s2)
        if first is None and len(x) == len(y):
            print('%-18s %8s bytes  IDENTICAL%s' % (n, size, where))
        else:
            print('%-18s %8s bytes  %d identical, first difference at +%#x%s'
                  % (n, size, n_eq, first if first is not None else min(len(x), len(y)), where))
    ha, hb = la.hdr, lb.hdr
    diffs = []
    for name, off, f in HDR:
        va, vb = struct.unpack_from('<' + f, a, ha + off)[0], struct.unpack_from('<' + f, b, hb + off)[0]
        if va != vb:
            diffs.append('  header %-20s ours %#x  game %#x' % (name, va, vb))
    print('LE header fields differing: %d' % len(diffs))
    for line in diffs[:20 if verbose else 8]:
        print(line)
    pa, pb = pairs(la), pairs(lb)
    print('fixups: %d in ours, %d in the game, %d identical (source, type, target)'
          % (len(pa), len(pb), len(pa & pb)))
    if verbose:
        for p in sorted(pb - pa)[:20]:
            print('  only game %#x type %#x -> %#x' % p)
        for p in sorted(pa - pb)[:20]:
            print('  only ours %#x type %#x -> %#x' % p)
    n = sum(1 for i in range(min(len(a), len(b))) if a[i] == b[i])
    print('LINK: %d of %d file bytes identical%s' % (n, len(b), '' if len(a) == len(b) else ' (ours %d bytes)' % len(a)))


if __name__ == '__main__':
    main()
