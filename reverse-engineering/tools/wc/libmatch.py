#!/usr/bin/env python3
"""libmatch.py: which modules of a Watcom (OMF) library occur in an LE program's code, byte for byte.

usage: libmatch.py [-l] PROG LIB [LIB ...]   PROG: a program loaded by tools/le/leparse.py (.work/le/PROG.*)
                                             -l: also list each module found, by address

For every module of every library the code segment's bytes are rebuilt from its LEDATA records with the bytes
that fixups fill in masked out; the module is then searched for in the program's code object. Per library: how
many modules occur, and their bytes. Run it with the same library from several compiler versions: the version
whose modules match is the one that linked the program. Modules that match in one version and not another are
listed (the evidence). Also usable for any OMF .OBJ.
"""
import json
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
FIXSIZE = {0: 1, 1: 2, 2: 2, 3: 4, 4: 1, 5: 2, 9: 4, 11: 6, 13: 4}


def index(b, p):
    if b[p] & 0x80:
        return ((b[p] & 0x7F) << 8) | b[p + 1], p + 2
    return b[p], p + 1


def records(data, p=0):
    while p + 3 <= len(data):
        t = data[p]
        n = struct.unpack_from('<H', data, p + 1)[0]
        yield t, p, data[p + 3:p + 3 + n - 1]
        p += 3 + n


def modules(data):
    """split a library (or a single object) into (name, [(type, body)])"""
    page = 16
    if data[0] == 0xF0:
        page = struct.unpack_from('<H', data, 1)[0] + 3
        p = page
    else:
        p = 0
    while p < len(data) and data[p] == 0x80:
        recs, name = [], None
        q = p
        for t, at, body in records(data, p):
            q = at + 3 + len(body) + 1
            if t == 0x80:
                name = body[1:1 + body[0]].decode('latin1')
            recs.append((t, body))
            if t in (0x8A, 0x8B):
                break
        yield name, recs
        p = q
        if data[0] == 0xF0:
            p = (p + page - 1) // page * page
        if p < len(data) and data[p] == 0xF1:
            break


def code_of(recs, only_code=True):
    """the module's segments as {segname: (bytearray, mask)}; mask 1 = a byte the linker fills.
    only_code=False keeps the data segments too (their group class is the second name of 0x98/0x99)."""
    lnames, segs, out = [None], [None], {}
    last = None
    for t, b in recs:
        if t == 0x96:
            p = 0
            while p < len(b):
                n = b[p]
                lnames.append(b[p + 1:p + 1 + n].decode('latin1'))
                p += 1 + n
        elif t in (0x98, 0x99):
            p0 = 1 + (3 if b[0] >> 5 == 0 else 0)         # ACBP, [frame, offset], length, names
            for lsize in ((4, 2) if t == 0x99 else (2, 4)):   # Watcom writes some 0x98 with a 4-byte length
                ln, p = index(b, p0 + lsize)
                cl, p = index(b, p)
                if 0 < ln < len(lnames) and 0 < cl < len(lnames) and p + 1 == len(b):
                    break
            segs.append((lnames[ln] if ln < len(lnames) else '?', lnames[cl] if cl < len(lnames) else '?'))
        elif t in (0xA0, 0xA1):
            si, p = index(b, 0)
            off = struct.unpack_from('<I' if t == 0xA1 else '<H', b, p)[0]
            p += 4 if t == 0xA1 else 2
            name, cls = segs[si]
            if only_code and (cls or '').upper() != 'CODE':
                last = None
                continue
            buf, mask = out.setdefault(name, (bytearray(), bytearray()))
            data = b[p:]
            if len(buf) < off + len(data):
                buf.extend(bytes(off + len(data) - len(buf)))
                mask.extend(bytes(off + len(data) - len(mask)))
            buf[off:off + len(data)] = data
            last = (buf, mask, off)
        elif t in (0x9C, 0x9D) and last:
            buf, mask, base = last
            p = 0
            while p < len(b):
                c = b[p]
                if not c & 0x80:                          # THREAD
                    method = (c >> 2) & 7
                    p += 1
                    if method < 3 or (c & 0x40 and method < 3):
                        _, p = index(b, p)
                    continue
                loc = (c >> 2) & 15
                ofs = ((c & 3) << 8) | b[p + 1]
                p += 2
                fd = b[p]
                p += 1
                if not fd & 0x80 and ((fd >> 4) & 7) < 3:
                    _, p = index(b, p)
                if not fd & 0x08:
                    _, p = index(b, p)
                if not fd & 0x04:
                    p += 4 if t == 0x9D else 2
                for k in range(FIXSIZE.get(loc, 4)):
                    if base + ofs + k < len(mask):
                        mask[base + ofs + k] = 1
        elif t not in (0x9C, 0x9D):
            if t not in (0x88, 0x8C, 0x90, 0x91, 0xB0, 0xB6, 0xB7):
                pass
    return out


def fixups_of(recs):
    """the fixups of a module's code segments: [(seg, offset, loc, selfrel, kind, name, disp)];
    kind 'seg' (name = segment), 'grp' (name = group) or 'ext' (name = external symbol)"""
    lnames, segs, grps, exts, out = [None], [None], [None], [None], []
    threads = {}
    last = None
    for t, b in recs:
        if t == 0x96:
            p = 0
            while p < len(b):
                n = b[p]
                lnames.append(b[p + 1:p + 1 + n].decode('latin1'))
                p += 1 + n
        elif t in (0x98, 0x99):
            p0 = 1 + (3 if b[0] >> 5 == 0 else 0)
            for lsize in ((4, 2) if t == 0x99 else (2, 4)):
                ln, p = index(b, p0 + lsize)
                cl, p = index(b, p)
                if 0 < ln < len(lnames) and 0 < cl < len(lnames) and p + 1 == len(b):
                    break
            segs.append((lnames[ln] if ln < len(lnames) else '?', lnames[cl] if cl < len(lnames) else '?'))
        elif t == 0x9A:
            gi, _ = index(b, 0)
            grps.append(lnames[gi] if gi < len(lnames) else '?')
        elif t in (0x8C, 0xB4):                          # EXTDEF, LEXTDEF
            p = 0
            while p < len(b):
                n = b[p]
                exts.append(b[p + 1:p + 1 + n].decode('latin1'))
                p += 1 + n
                _, p = index(b, p)
        elif t in (0xB0, 0xB8):                          # COMDEF, LCOMDEF share the external indexes
            p = 0
            while p < len(b):
                n = b[p]
                exts.append(b[p + 1:p + 1 + n].decode('latin1'))
                p += 1 + n
                _, p = index(b, p)
                kind = b[p]
                p += 1
                for _ in range(2 if kind == 0x61 else 1):
                    v = b[p]
                    p += {0x81: 3, 0x84: 4, 0x88: 5}.get(v, 1)
        elif t in (0xA0, 0xA1):
            si, p = index(b, 0)
            off = struct.unpack_from('<I' if t == 0xA1 else '<H', b, p)[0]
            last = (segs[si][0], off) if (segs[si][1] or '').upper() == 'CODE' else None
        elif t in (0x9C, 0x9D):
            p = 0
            while p < len(b):
                c = b[p]
                if not c & 0x80:                          # THREAD: remember target threads
                    method, num, frame = (c >> 2) & 7, c & 3, c & 0x40
                    p += 1
                    idx = None
                    if method < 3:
                        idx, p = index(b, p)
                    if not frame:
                        threads[num] = (method, idx)
                    continue
                selfrel = not c & 0x40
                loc = (c >> 2) & 15
                ofs = ((c & 3) << 8) | b[p + 1]
                p += 2
                fd = b[p]
                p += 1
                if not fd & 0x80 and ((fd >> 4) & 7) < 3:
                    _, p = index(b, p)
                if fd & 0x08:
                    method, idx = threads.get(fd & 3, (None, None))
                else:
                    method = fd & 3
                    idx, p = index(b, p)
                disp = 0
                if not fd & 0x04:
                    disp = struct.unpack_from('<i' if t == 0x9D else '<h', b, p)[0]
                    p += 4 if t == 0x9D else 2
                if last is None:
                    continue
                kind, name = {0: ('seg', segs), 1: ('grp', grps), 2: ('ext', exts)}.get(method & 3, ('?', None))
                nm = name[idx] if name and idx is not None and idx < len(name) else '?'
                if kind == 'seg':
                    nm = nm[0]
                out.append((last[0], last[1] + ofs, loc, selfrel, kind, nm, disp))
    return out


def find(code, buf, mask):
    """all positions where buf occurs in code, the masked bytes free"""
    n = len(buf)
    if n == 0:
        return []
    # the longest unmasked run as the anchor
    best, run_at, run = (0, 0), None, 0
    for i in range(n):
        if mask[i]:
            run_at, run = None, 0
            continue
        if run_at is None:
            run_at = i
        run += 1
        if run > best[1]:
            best = (run_at, run)
    a, alen = best
    if alen < 4:
        return []
    anchor = bytes(buf[a:a + alen])
    hits, p = [], code.find(anchor)
    while p >= 0:
        s = p - a
        if s >= 0 and s + n <= len(code) and all(mask[i] or code[s + i] == buf[i] for i in range(n)):
            hits.append(s)
        p = code.find(anchor, p + 1)
    return hits


def main():
    args = sys.argv[1:]
    listing = args[:1] == ['-l']
    args = args[1:] if listing else args
    if len(args) < 2:
        print(__doc__)
        return 2
    prog = args[0]
    le = os.path.join(ROOT, '.work', 'le')
    meta = json.load(open(os.path.join(le, prog + '.json')))
    code = open(os.path.join(le, prog + '.obj1.bin'), 'rb').read()
    cbase = meta['objects'][0]['base']
    found = {}
    for lib in args[1:]:
        data = open(lib, 'rb').read()
        n = hit = nbytes = 0
        found[lib] = {}
        for name, recs in modules(data):
            segs = code_of(recs)
            size = sum(len(b) for b, _ in segs.values())
            if not size:
                continue
            n += 1
            ok = all(find(code, b, m) for b, m in segs.values() if len(b))
            where = [find(code, b, m)[:1] for b, m in segs.values() if len(b)]
            found[lib][name] = (ok, size, where[0][0] + cbase if ok and where and where[0] else None)
            if ok:
                hit += 1
                nbytes += size
        print('%-60s modules with code %4d, found %4d (%d bytes)' % (lib[-60:], n, hit, nbytes))
        if listing:
            for nm, (ok, size, addr) in sorted(found[lib].items(), key=lambda kv: kv[1][2] or 0):
                if ok:
                    print('  0x%06x %5d  %s' % (addr or 0, size, nm))
    libs = list(found)
    if len(libs) > 1:
        names = sorted(set().union(*[set(found[l]) for l in libs]))
        print('\nmodules found in some versions but not all (name: found per library, address):')
        for nm in names:
            r = [found[l].get(nm) for l in libs]
            oks = [bool(x and x[0]) for x in r]
            if any(oks) and not all(oks):
                addr = next(x[2] for x in r if x and x[0])
                print('  %-24s %s  0x%06x' % (nm, ' '.join('Y' if o else '-' for o in oks), addr))
    return 0


if __name__ == '__main__':
    sys.exit(main())
