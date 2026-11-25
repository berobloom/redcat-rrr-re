#!/usr/bin/env python3
"""blob.py: carve an LE program back into OMF objects that WLINK links into the same program, with
real objects (library modules, src/ objects) in place of the ranges they fill.

usage: blob.py EXE OUTDIR [--obj FILE.OBJ@ADDR ...] [--lib LIB ...]

Without --obj it writes OUTDIR/CODEBLOB_0.OBJ (the code object) and OUTDIR/DATABLOB.OBJ (the data
object): every LE fixup site written back as an OMF 32-bit offset fixup (frame FLAT, the target's
offset kept in the data bytes, the way Watcom's own objects carry the addend), in the order the
original objects gave them to the linker (input_order). DATABLOB is group DGROUP: _DATA (class DATA)
= the initialised bytes, STACK (class STACK) = the rest (the LE ESP is the object's end).

`--obj FILE.OBJ@ADDR` puts a real object's _TEXT at code address ADDR (hex). Its range is left out
of the code blob, which is cut into CODEBLOB_<n>.OBJ pieces (public `blob_<addr>` at each start):
  - each external the object needs is read back from the game's bytes at the object's own fixup
    sites and defined as a public in the blob piece (or DATABLOB) that holds that address;
  - a blob fixup into the object's range goes through the object's nearest public plus an addend.
Only objects whose data segments are all empty are taken for now (data placement needs the original
segment order inside the data object). OUTDIR/FILES.LNK gets the `file` directives in link order.

`--lib LIB` lets WLINK's undefined externals be resolved: an OMF module lists every external it
*MIGHT* use, and WLINK demands a definition even for the dead entries (Fastgraph 4.03's ASM modules
list DATA.ASM globals they never call). Such a name gets a public at the address of its defining
module's segment, anchored by the game addresses already read back for sibling names in that segment
from real fixup sites (`--obj` objects' `needs`).
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'wc'))
import leparse  # noqa: E402
import libmatch as L  # noqa: E402
from libobj import info, info_bytes, raw_modules  # noqa: E402


def rec(t, body):
    body = bytes(body)
    return bytes([t]) + struct.pack('<H', len(body) + 1) + body + b'\0'


def name(s):
    s = s.encode('latin1')
    return bytes([len(s)]) + s


class Obj:
    def __init__(self, modname):
        self.out = [rec(0x80, name(modname))]
        self.lnames = ['']
        self.nseg = 0
        self.ngrp = 0

    def lname(self, s):
        if s not in self.lnames:
            self.lnames.append(s)
            self.out.append(rec(0x96, name(s)))
        return self.lnames.index(s)

    def segdef(self, seg, cls, length, align=1, combine=2):
        sn, cn = self.lname(seg), self.lname(cls)
        acbp = (align << 5) | (combine << 2) | 1          # use32
        self.out.append(rec(0x99, bytes([acbp]) + struct.pack('<I', length) + bytes([sn, cn, 1])))
        self.nseg += 1
        return self.nseg

    def grpdef(self, grp, segs):
        gn = self.lname(grp)
        self.out.append(rec(0x9A, bytes([gn]) + b''.join(bytes([0xFF, s]) for s in segs)))
        self.ngrp += 1
        return self.ngrp

    def pubdef(self, grp, seg, items):
        body = bytes([grp, seg]) + b''.join(name(n) + struct.pack('<I', o) + b'\0' for n, o in items)
        self.out.append(rec(0x91, body))

    def extdef(self, names):
        self.out.append(rec(0x8C, b''.join(name(n) + b'\0' for n in names)))

    def ledata(self, seg, off, data, fixups, flat):
        """fixups: [(offset in data, target segment index or ('ext', n))]"""
        self.out.append(rec(0xA1, bytes([seg]) + struct.pack('<I', off) + data))
        if fixups:
            body = b''
            for at, tgt in fixups:
                locat = 0x80 | 0x40 | (9 << 2) | (at >> 8)        # segment-relative, offset32
                if isinstance(tgt, tuple):                       # target: EXTDEF index, no displacement
                    fixdat = (1 << 4) | (1 << 2) | 2
                    body += bytes([locat, at & 0xFF, fixdat, flat, tgt[1]])
                else:                                            # target: SEGDEF index
                    fixdat = (1 << 4) | (1 << 2) | 0
                    body += bytes([locat, at & 0xFF, fixdat, flat, tgt])
            self.out.append(rec(0x9D, body))

    def modend(self, start=None):
        if start is None:
            self.out.append(rec(0x8B, b'\0'))
        else:
            flat, seg, disp = start
            self.out.append(rec(0x8B, bytes([0xC1, 0x10, flat, seg]) + struct.pack('<I', disp)))

    def write(self, path):
        open(path, 'wb').write(b''.join(self.out))


def fixup_size(toff):
    return 9 if toff > 0xFFFF else 7


def input_order(le, obj):
    """{page: [source address, ...]} in the order the linker received the fixups. WLINK keeps each
    page's fixups in blocks of up to 512 bytes, filled in input order, and writes the blocks newest
    first; so the file's list is B_k ... B_1 with every block but the newest full (the next record
    would not have fitted). Split the list that way (a small search) and read the blocks backwards."""
    out = {}
    for k in range(obj['page_count']):
        page = obj['page_index'] + k
        recs = [(obj['base'] + k * le.page_size + so, fixup_size(toff)) for st, so, tobj, toff in le.fixups(page)]
        n = len(recs)
        if not n:
            continue
        pre = [0]
        for _, z in recs:
            pre.append(pre[-1] + z)
        size = lambda i, j: pre[j] - pre[i]
        memo = {}

        def split(i, first_of_newer):
            """partitions of recs[i:] into full blocks; first_of_newer: size of the first record of
            the block before (newer in time, earlier in the file) - the one that did not fit"""
            if i == n:
                return []
            key = (i, first_of_newer)
            if key in memo:
                return memo[key]
            res = None
            for j in range(i + 1, n + 1):
                if size(i, j) > 512:
                    break
                if size(i, j) + first_of_newer > 512:
                    rest = split(j, recs[i][1])
                    if rest is not None:
                        res = [(i, j)] + rest
                        break
            memo[key] = res
            return res
        best = None
        for j in range(1, n + 1):                  # the newest block: any size up to 512
            if size(0, j) > 512:
                break
            rest = split(j, recs[0][1]) if j < n else []
            if rest is not None:
                best = [(0, j)] + rest
                break
        if best is None:
            raise SystemExit('page %d: fixup list does not split into 512-byte blocks' % page)
        out[page] = [recs[x][0] for i, j in reversed(best) for x in range(i, j)]
    return out


def emit_ordered(o, seg, base, img, lo, hi, sites, target_of, flat, order):
    """the bytes lo..hi without fixup sites as plain LEDATA, then one LEDATA+FIXUPP per fixup site in
    `order` (a list of source addresses; a site in two pages comes once, at its first place)"""
    cut = sorted(s for s in sites if lo <= s < hi)
    a = lo
    for s in cut + [hi]:
        while a < min(s, hi):
            e = min(s, hi, a + 1024)
            o.ledata(seg, a - lo, bytes(img[a - base:e - base]), [], flat)
            a = e
        a = max(a, s + 4)
    done = set()
    for s in order:
        if s in done or not lo <= s < hi or s not in sites:
            continue
        done.add(s)
        tseg, toff = target_of(s)
        o.ledata(seg, s - lo, struct.pack('<I', toff), [(0, tseg)], flat)
    missing = [s for s in cut if s not in done]
    if missing:
        raise SystemExit('%d fixup sites not in the page order, first %#x' % (len(missing), missing[0]))




class RealObj:
    def __init__(self, path, addr):
        self.path, self.addr = path, addr
        segs, pubs, exts = info(path)
        self.exts = set(exts)
        texts = [s for s in segs if s[1].upper() == 'CODE']
        if len(texts) != 1:
            raise SystemExit('%s: %d code segments, want 1' % (path, len(texts)))
        for s in segs:
            if s[1].upper() != 'CODE' and s[2]:
                raise SystemExit('%s: data segment %s has %d bytes; data placement is not supported yet'
                                 % (path, s[0], s[2]))
        self.text, self.size = texts[0][0], texts[0][2]
        self.pubs = sorted((self.addr + o, n) for s, o, n in pubs if s == self.text)
        self.names = {n for s, o, n in pubs} | info.local     # locals resolve inside the object
        (_, self.recs), = list(L.modules(open(path, 'rb').read()))

    def needs(self, code, cbase):
        """{external name: game address} from the game's bytes at this object's fixup sites"""
        buf = L.code_of(self.recs)[self.text][0]
        out = {}
        for seg, at, loc, selfrel, kind, nm, disp in L.fixups_of(self.recs):
            if kind != 'ext' or loc != 9 or nm in self.names:
                continue
            game_at = self.addr + at
            v = struct.unpack_from('<I', code, game_at - cbase)[0]
            if selfrel:
                tgt = (game_at + 4 + v) & 0xFFFFFFFF
            else:
                tgt = (v - struct.unpack_from('<i', bytes(buf), at)[0]) & 0xFFFFFFFF
            tgt = (tgt - disp) & 0xFFFFFFFF
            if out.setdefault(nm, tgt) != tgt:
                raise SystemExit('%s: %s is %#x and %#x' % (self.path, nm, out[nm], tgt))
        return out


def main():
    exe, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    objs, libs = [], []
    for i, a in enumerate(sys.argv):
        if a == '--obj':
            p, _, at = sys.argv[i + 1].rpartition('@')
            objs.append(RealObj(p, int(at, 16)))
        elif a == '--lib':
            libs.append(sys.argv[i + 1])
    objs.sort(key=lambda o: o.addr)
    le = leparse.LE(open(exe, 'rb').read())
    imgs, _ = le.load()
    cobj, dobj = le.objects
    cbase, dbase = cobj['base'], dobj['base']
    cend, dend = cbase + cobj['vsize'], dbase + dobj['vsize']
    code, data = bytes(imgs[1]), bytes(imgs[2])
    fix = {}
    for o in le.objects:
        for k in range(o['page_count']):
            for stype, so, tobj, toff in le.fixups(o['page_index'] + k):
                if stype != 0x07:
                    raise SystemExit('fixup type %#x not handled' % stype)
                at = o['base'] + k * le.page_size + so
                if o['base'] <= at < o['base'] + o['vsize']:
                    fix[at] = le.objects[tobj - 1]['base'] + toff
    dpages = dobj['page_count']
    last = le.last_page if dobj['page_index'] + dpages - 1 == le.num_pages else le.page_size
    dinit = (dpages - 1) * le.page_size + last
    for a in fix:                                   # a pointer can end past the stored bytes
        if dbase + dinit - 4 < a < dbase + dinit:
            dinit = a - dbase + 4
    stack = dobj['vsize'] - dinit

    # code pieces between the real objects
    pieces, a = [], cbase
    for o in objs:
        if o.addr < a:
            raise SystemExit('%s overlaps the previous object' % o.path)
        if o.addr > a:
            pieces.append((a, o.addr))
        a = o.addr + o.size
    if a < cend:
        pieces.append((a, cend))
    order = [('piece', p) for p in pieces] + [('obj', o) for o in objs]
    order.sort(key=lambda t: t[1][0] if t[0] == 'piece' else t[1].addr)

    def owner(t):
        """('piece', (lo, hi)) / ('obj', RealObj) / ('data', None) for a target address"""
        for p in pieces:
            if p[0] <= t < p[1]:
                return 'piece', p
        for o in objs:
            if o.addr <= t < o.addr + o.size:
                return 'obj', o
        if dbase <= t <= dend:
            return 'data', None
        if t == cend:
            return 'piece', pieces[-1]
        raise SystemExit('target %#x is in no object' % t)

    # publics the blobs must define for the real objects
    need = {}
    for o in objs:
        for nm, t in o.needs(code, cbase).items():
            if any(nm in x.names for x in objs):
                continue
            if need.setdefault(nm, t) != t:
                raise SystemExit('%s needed at %#x and %#x' % (nm, need[nm], t))

    # externals the real objects LIST but no fixup references: WLINK still wants them defined.
    # Take the address from the LIB module that defines the name, anchoring that module's segment
    # with the game addresses already read back for its sibling names in the same segment.
    listed = set().union(*(o.exts for o in objs)) if objs else set()
    dead = sorted(nm for nm in listed if nm not in need and not any(nm in x.names for x in objs))
    if dead:
        byname, bymod = {}, {}
        for lib in libs:
            for mod, raw in raw_modules(open(lib, 'rb').read()):
                if not mod:
                    continue
                try:
                    segs, pubs, _ = info_bytes(raw)
                except Exception:
                    continue
                for sn, off, nm in pubs:
                    byname.setdefault(nm, []).append(((os.path.basename(lib), mod), sn, off))
                    bymod.setdefault((os.path.basename(lib), mod), []).append((nm, sn, off))
        for nm in dead:
            cands = byname.get(nm, [])
            if len(cands) != 1:
                raise SystemExit('%s: no fixup references it and %d LIB modules define it' % (nm, len(cands)))
            key, sn, off = cands[0]
            bases = {need[n2] - o2 for n2, s2, o2 in bymod[key] if s2 == sn and n2 in need}
            if not bases:
                raise SystemExit('%s: %s of %s is not anchored by any sibling the link reads back'
                                 % (nm, key[1], sn))
            if len(bases) != 1:
                raise SystemExit('%s: %s/%s anchored at %d bases: %s'
                                 % (nm, key[0], key[1], len(bases), sorted(hex(b) for b in bases)))
            need[nm] = bases.pop() + off
            print('dead EXTDEF %s: %s/%s %s+%#x -> %#x' % (nm, key[0], key[1], sn, off, need[nm]))
    corder = [s for p, l in sorted(input_order(le, cobj).items()) for s in l]
    dorder = [s for p, l in sorted(input_order(le, dobj).items()) for s in l]

    def resolver(ob, own, flat, extmap):
        """target address -> (target index or ('ext', n), offset to keep in the bytes)"""
        def ext(nm):
            if nm not in extmap:
                extmap[nm] = len(extmap) + 1
            return ('ext', extmap[nm])

        def go(s):
            t = fix[s]
            kind, x = owner(t)
            if kind == 'data':
                if own == 'data':
                    return ('seg', t)
                return ext('blob_data'), t - dbase
            if kind == 'piece':
                if own == x:
                    return ('seg', t)
                return ext('blob_%x' % x[0]), t - x[0]
            pa, pn = min(x.pubs, key=lambda q: (abs(t - q[0]), q[0] > t))
            return ext(pn), t - pa
        return go

    files = []
    for kind, x in order:
        if kind == 'obj':                          # copied next to the blobs: WLINK runs there under Wine
            fn = 'O_%x.OBJ' % x.addr
            open(os.path.join(out, fn), 'wb').write(open(x.path, 'rb').read())
            files.append(fn)
            continue
        lo, hi = x
        ob = Obj('codeblob_%x' % lo)
        text = ob.segdef('_TEXT', 'CODE', hi - lo)
        flat = ob.grpdef('FLAT', [])
        extmap = {}
        go = resolver(ob, x, flat, extmap)
        sites = {s for s in fix if lo <= s < hi}
        targets = {s: go(s) for s in sorted(sites)}
        if extmap:
            ob.extdef([n for n, i in sorted(extmap.items(), key=lambda q: q[1])])
        pubs = [('blob_%x' % lo, 0)] + sorted((n, t - lo) for n, t in need.items() if lo <= t < hi)
        ob.pubdef(0, text, pubs)

        def tgt(s, text=text):
            r = targets[s]
            if r[0] == 'seg':
                return text, r[1] - lo
            return r
        emit_ordered(ob, text, cbase, code, lo, hi, sites, tgt, flat, corder)
        ob.modend((flat, text, le.eip - (lo - cbase)) if lo <= cbase + le.eip < hi else None)
        fn = 'CODEBLOB_%x.OBJ' % lo
        ob.write(os.path.join(out, fn))
        files.append(fn)

    od = Obj('datablob')
    dseg = od.segdef('_DATA', 'DATA', dinit)
    sseg = od.segdef('STACK', 'STACK', stack, combine=5)
    od.grpdef('DGROUP', [dseg, sseg])
    flat = od.grpdef('FLAT', [])
    extmap = {}
    go = resolver(od, 'data', flat, extmap)
    dsites = {s for s in fix if dbase <= s < dend}
    targets = {s: go(s) for s in sorted(dsites)}
    if extmap:
        od.extdef([n for n, i in sorted(extmap.items(), key=lambda q: q[1])])
    od.pubdef(0, dseg, [('blob_data', 0)] + sorted((n, t - dbase) for n, t in need.items() if dbase <= t < dbase + dinit))
    tail = sorted((n, t - dbase - dinit) for n, t in need.items() if dbase + dinit <= t <= dend)
    if tail:                                        # uninitialised data: publics in the tail segment
        od.pubdef(0, sseg, tail)

    def dtgt(s):
        r = targets[s]
        if r[0] == 'seg':
            t = r[1]
            return (dseg, t - dbase) if t < dbase + dinit else (sseg, t - dbase - dinit)
        return r
    emit_ordered(od, dseg, dbase, data, dbase, dbase + dinit, dsites, dtgt, flat, dorder)
    od.modend()
    od.write(os.path.join(out, 'DATABLOB.OBJ'))
    files.append('DATABLOB.OBJ')
    open(os.path.join(out, 'FILES.LNK'), 'w').write(''.join('file %s\n' % f for f in files))
    print('%d code pieces, %d real objects (%d bytes), %d publics defined for them; data %#x + stack %#x'
          % (len(pieces), len(objs), sum(o.size for o in objs), len(need), dinit, stack))


if __name__ == '__main__':
    main()
