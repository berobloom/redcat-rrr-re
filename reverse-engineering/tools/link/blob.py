#!/usr/bin/env python3
"""blob.py: carve an LE program back into OMF objects that WLINK can link into the same program.

usage: blob.py EXE OUTDIR [--keep-out START-END ...]

Writes OUTDIR/CODEBLOB.OBJ and OUTDIR/DATABLOB.OBJ:
  CODEBLOB  segment _TEXT (class CODE, byte-aligned, use32): the whole code object, every LE fixup
            site written back as an OMF 32-bit offset fixup (frame FLAT, target the segment that
            holds the target address, the target's offset in that segment kept in the data bytes,
            the way Watcom's own objects carry the addend). MODEND names the LE entry point.
  DATABLOB  group DGROUP: _DATA (class DATA) = the data object's initialised bytes (what the LE file
            stores), _BSS (class BSS) = the rest up to the stack, STACK (class STACK) = the stack
            (the LE ESP is the object's end). Fixups in data are written the same way.

`--keep-out A-B` (hex, code addresses) leaves a range of the code object out: the blob is then cut
into pieces (segment _TEXT in CODEBLOB_<n>.OBJ, each with a public `blob_<addr>` at its start) so
that real objects can go in between, in link order. A fixup whose target falls in a left-out range
must be resolved through a public of the object that fills it: not supported yet (the script stops).

The stack's start inside the zero tail is not known from the LE file; --stack N sets its size
(default: everything after the initialised data is STACK, which links to the same bytes).
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import leparse  # noqa: E402


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


def emit(o, seg, base, img, lo, hi, sites, target_of, flat):
    """LEDATA/FIXUPP records for image bytes lo..hi (addresses), cut so no fixup straddles a record"""
    a = lo
    while a < hi:
        e = min(hi, a + 1024)
        for s in sites:                                          # do not cut through a fixup site
            if a < s < e < s + 4:
                e = s
        chunk = bytearray(img[a - base:e - base])
        fx = []
        for s in sites:
            if a <= s < e:
                tseg, toff = target_of(s)
                struct.pack_into('<I', chunk, s - a, toff)
                fx.append((s - a, tseg))
        o.ledata(seg, a - lo, bytes(chunk), fx, flat)
        a = e


def main():
    exe, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    le = leparse.LE(open(exe, 'rb').read())
    imgs, _ = le.load()
    cobj, dobj = le.objects
    cbase, dbase = cobj['base'], dobj['base']
    code, data = bytes(imgs[1]), bytes(imgs[2])
    # every fixup site, with its target address
    fix = {}
    for o in le.objects:
        for k in range(o['page_count']):
            page = o['page_index'] + k
            for stype, so, tobj, toff in le.fixups(page):
                if stype != 0x07:
                    raise SystemExit('fixup type %#x not handled' % stype)
                at = o['base'] + k * le.page_size + so
                if o['base'] <= at < o['base'] + o['vsize']:
                    fix[at] = le.objects[tobj - 1]['base'] + toff
    # initialised length of the data object = what the file stores
    dpages = dobj['page_count']
    last = le.last_page if dobj['page_index'] + dpages - 1 == le.num_pages else le.page_size
    dinit = (dpages - 1) * le.page_size + last
    # WLINK does not store the zero bytes that end the last page, so the stored length can stop inside
    # initialised data (a pointer at its end, say): reach past the last fixup site that lies there
    for a in fix:
        if dbase + dinit - 4 < a < dbase + dinit:
            dinit = a - dbase + 4
    stack = int(sys.argv[sys.argv.index('--stack') + 1], 0) if '--stack' in sys.argv else dobj['vsize'] - dinit
    bss = dobj['vsize'] - dinit - stack

    corder = [a for p, l in sorted(input_order(le, cobj).items()) for a in l]
    dorder = [a for p, l in sorted(input_order(le, dobj).items()) for a in l]
    csites = sorted(a for a in fix if cbase <= a < cbase + cobj['vsize'])
    dsites = sorted(a for a in fix if dbase <= a < dbase + dobj['vsize'])

    # CODEBLOB
    oc = Obj('codeblob')
    flat_c = None
    text = oc.segdef('_TEXT', 'CODE', cobj['vsize'])
    flat_c = oc.grpdef('FLAT', [])
    oc.extdef(['blob_data'])                                     # DATABLOB's _DATA start
    def tcode(s):
        t = fix[s]
        if cbase <= t < cbase + cobj['vsize']:
            return text, t - cbase
        return ('ext', 1), t - dbase                             # data target: blob_data + offset
    oc.pubdef(0, text, [('blob_text', 0)])
    emit_ordered(oc, text, cbase, code, cbase, cbase + cobj['vsize'], set(csites), tcode, flat_c, corder)
    oc.modend((flat_c, text, le.eip))
    oc.write(os.path.join(out, 'CODEBLOB.OBJ'))

    # DATABLOB
    od = Obj('datablob')
    dseg = od.segdef('_DATA', 'DATA', dinit)
    bseg = od.segdef('_BSS', 'BSS', bss) if bss else None
    sseg = od.segdef('STACK', 'STACK', stack, combine=5)
    od.grpdef('DGROUP', [s for s in (dseg, bseg, sseg) if s])
    flat_d = od.grpdef('FLAT', [])
    od.extdef(['blob_text'])
    def tdata(s):
        t = fix[s]
        if dbase <= t < dbase + dinit:
            return dseg, t - dbase
        if bseg and dbase + dinit <= t < dbase + dinit + bss:
            return bseg, t - dbase - dinit
        if dbase + dinit + bss <= t <= dbase + dobj['vsize']:
            return sseg, t - dbase - dinit - bss
        if cbase <= t < cbase + cobj['vsize']:
            return ('ext', 1), t - cbase
        raise SystemExit('data fixup at %#x targets %#x, outside both objects' % (s, t))
    od.pubdef(0, dseg, [('blob_data', 0)])
    emit_ordered(od, dseg, dbase, data, dbase, dbase + dinit, set(dsites), tdata, flat_d, dorder)
    od.modend()
    od.write(os.path.join(out, 'DATABLOB.OBJ'))
    print('code %#x bytes, %d fixups; data %#x initialised + %#x bss + %#x stack, %d fixups; entry %#x'
          % (cobj['vsize'], len(csites), dinit, bss, stack, len(dsites), le.eip))


if __name__ == '__main__':
    main()
