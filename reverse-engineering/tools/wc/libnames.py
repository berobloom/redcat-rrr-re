#!/usr/bin/env python3
"""libnames.py: names-file lines for every public of an OMF library module found in a program.

usage: libnames.py PROG LIB [LIB ...] [--min N]

A module counts when its code segment occurs exactly once in PROG's code object (fixup bytes
masked) and at least N of its bytes (default 32) are compared: short modules match anywhere
(GOTCHAS, "libmatch's short hits"). Each public code symbol of the module is printed at the module's
address + the symbol's offset, as `0xADDR name  # CONFIRMED: <lib> module <mod>, N bytes`, with
Watcom's `_` decoration removed. Paste what is new into reverse-engineering/analysis/PROG_names.txt.
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import libmatch as L  # noqa: E402
import objmatch as M  # noqa: E402


def main():
    args = [a for a in sys.argv[1:]]
    need = 32
    if '--min' in args:
        i = args.index('--min')
        need = int(args[i + 1])
        del args[i:i + 2]
    if len(args) < 2:
        print(__doc__)
        return 2
    prog = args[0]
    le = os.path.join(L.ROOT, '.work', 'le')
    cbase = json.load(open(os.path.join(le, prog + '.json')))['objects'][0]['base']
    code = open(os.path.join(le, prog + '.obj1.bin'), 'rb').read()
    out = []
    for lib in args[1:]:
        for mod, recs in L.modules(open(lib, 'rb').read()):
            segs = L.code_of(recs)
            text = [(s, b, m) for s, (b, m) in segs.items() if len(b)]
            if len(text) != 1:
                continue
            seg, b, m = text[0]
            if len(b) - sum(1 for x in m if x) < need:
                continue
            hits = L.find(code, b, m)
            if len(hits) != 1:
                continue
            base = hits[0] + cbase
            for s, off, name in M.publics(recs):
                if s == seg:
                    out.append((base + off, M.plain(name), os.path.basename(lib), mod, len(b)))
    for a, n, lib, mod, size in sorted(out):
        print('0x%05x %-20s # CONFIRMED: %s module %s (%d bytes) found byte for byte here' % (a, n, lib, mod, size))
    return 0


if __name__ == '__main__':
    sys.exit(main())
