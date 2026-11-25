#!/usr/bin/env python3
"""libobj.py: copy one module out of an OMF library as a stand-alone .OBJ, and describe objects.

usage: libobj.py LIB MODULE OUT.OBJ     write the module's records unchanged (WLIB's `*` extract)
       libobj.py --info FILE.OBJ ...    segments (name, class, size), publics, externals
info() also leaves the module-local publics (LPUBDEF) in info.local.

The module's records are copied byte for byte from THEADR to MODEND, so the object is the one the
original link pulled from the library.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'wc'))
import libmatch as L  # noqa: E402


def raw_modules(data):
    """(name, bytes) per module, the records as they are in the file"""
    page = 16
    p = 0
    if data[0] == 0xF0:
        page = struct.unpack_from('<H', data, 1)[0] + 3
        p = page
    while p < len(data) and data[p] == 0x80:
        q, name = p, None
        while q + 3 <= len(data):
            t = data[q]
            n = struct.unpack_from('<H', data, q + 1)[0]
            if t == 0x80:
                name = data[q + 4:q + 4 + data[q + 3]].decode('latin1')
            q += 3 + n
            if t in (0x8A, 0x8B):
                break
        yield name, data[p:q]
        p = q
        if data[0] == 0xF0:
            p = (p + page - 1) // page * page
        if p < len(data) and data[p] == 0xF1:
            break


def info(path):
    (_, recs), = list(L.modules(open(path, 'rb').read()))
    lnames, segs, exts, pubs, local = [None], [None], [None], [], set()
    for t, b in recs:
        if t == 0x96:
            p = 0
            while p < len(b):
                n = b[p]
                lnames.append(b[p + 1:p + 1 + n].decode('latin1'))
                p += 1 + n
        elif t in (0x98, 0x99):                      # Watcom writes some 0x98 with a 4-byte length
            p0 = 1 + (3 if b[0] >> 5 == 0 else 0)
            for ln in ((4, 2) if t == 0x99 else (2, 4)):
                sn, p = L.index(b, p0 + ln)
                cn, p = L.index(b, p)
                if 0 < sn < len(lnames) and 0 < cn < len(lnames) and p + 1 == len(b):
                    break
            size = struct.unpack_from('<I' if ln == 4 else '<H', b, p0)[0]
            segs.append((lnames[sn], lnames[cn], size))
        elif t in (0x8C, 0xB4):
            p = 0
            while p < len(b):
                n = b[p]
                exts.append(b[p + 1:p + 1 + n].decode('latin1'))
                p += 1 + n
                _, p = L.index(b, p)
        elif t in (0x90, 0x91, 0xB6, 0xB7):          # ... and some 0x90 with 4-byte offsets
            g, p0 = L.index(b, 0)
            s, p0 = L.index(b, p0)
            if g == 0 and s == 0:
                p0 += 2
            for w in ((4, 2) if t == 0x91 else (2, 4)):
                items, p, ok = [], p0, True
                while p < len(b):
                    n = b[p]
                    nm = b[p + 1:p + 1 + n].decode('latin1')
                    p += 1 + n
                    if p + w + 1 > len(b):
                        ok = False
                        break
                    off = struct.unpack_from('<I' if w == 4 else '<H', b, p)[0]
                    p += w
                    _, p = L.index(b, p)
                    items.append((segs[s][0] if s else None, off, nm))
                if ok and p == len(b):
                    break
            if t in (0xB6, 0xB7):                    # LPUBDEF: module-local, not visible outside
                local.update(nm for _, _, nm in items)
            else:
                pubs.extend(items)
    info.local = local
    return segs[1:], pubs, exts[1:]


def main():
    if sys.argv[1] == '--info':
        for f in sys.argv[2:]:
            segs, pubs, exts = info(f)
            print('%s: segs %s pubs %s exts %s' % (os.path.basename(f), segs, [(n, o) for s, o, n in pubs], exts))
        return
    lib, mod, out = sys.argv[1:4]
    for name, raw in raw_modules(open(lib, 'rb').read()):
        if name and name.lower() == mod.lower():
            open(out, 'wb').write(raw)
            print('%s: %d bytes' % (name, len(raw)))
            return
    raise SystemExit('%s: no module %s' % (lib, mod))


if __name__ == '__main__':
    main()
