#!/usr/bin/env python3
"""blob.py: carve an LE program back into OMF objects that WLINK links into the same program, with
real objects (library modules, src/ objects) in place of the ranges they fill.

usage: blob.py EXE OUTDIR [--obj FILE.OBJ@ADDR ...] [--lib LIB ...] [--dseg NAME@ADDR[-LEN][:CLASS] ...]

Without --obj it writes OUTDIR/CODEBLOB_0.OBJ (the code object) and OUTDIR/DATABLOB.OBJ (the data
object): every LE fixup site written back as an OMF 32-bit offset fixup (frame FLAT, the target's
offset kept in the data bytes, the way Watcom's own objects carry the addend), in the order the
original objects gave them to the linker (input_order). DATABLOB is group DGROUP: the initialised
bytes as one SEGDEF per --dseg block, STACK (class STACK) = the rest (the LE ESP is the object's end).

`--dseg NAME@ADDR[-LEN][:CLASS]` starts a data-object segment block: WLINK merges every segment of
one name and class into one run of the data object, in input order, so the original data object is
[run of CONST][CONST2][_DATA][XI][YI][DATA32]... - see tools/link/dataobj.py. Naming a data blob
piece after the block it covers is what lets a real object's same-named data segment merge into it
at its measured address, and it is how WLINK gets the Watcom bookkeeping symbols right: it defines
XIB/XIE (and YIB/YIE) itself as the bounds of the merged XI (YI) run, so with no XI run in the link
the four XIB/XIE/YIB/YIE fixups of clib's inirt386 silently aim at the end of DGROUP. Each block
runs to the next --dseg address (the last one to the end of the initialised bytes) unless -LEN ends
it there and leaves the rest to WLINK's 4-byte run alignment; blocks are named _DATA unless given a
name.

`--obj FILE.OBJ@ADDR` puts a real object's _TEXT at code address ADDR (hex). Its range is left out
of the code blob, which is cut into CODEBLOB_<n>.OBJ pieces (public `blob_<addr>` at each start):
  - each external the object needs is read back from the game's bytes at the object's own fixup
    sites and defined as a public in the blob piece (or DATABLOB) that holds that address;
  - a blob fixup into the object's range goes through the object's nearest public plus an addend.
`--obj FILE.OBJ@ADDR,SEG=DADDR,...` also places the object's data segments (tools/link/dataplace.py
measures DADDR); a _BSS given an address needs a `--dseg _BSS@ADDR:BSS` block, which then runs to the
first 4-byte boundary after the last placed _BSS, where STACK starts. OUTDIR/FILES.LNK gets the
`file` directives in link order.

`--lib LIB` lets WLINK's undefined externals be resolved: an OMF module lists every external it
*MIGHT* use, and WLINK demands a definition even for the dead entries (Fastgraph 4.03's ASM modules
list DATA.ASM globals they never call). Such a name gets a public at the address of its defining
module's segment, anchored by the game addresses already read back for sibling names in that segment
from real fixup sites (`--obj` objects' `needs`).
"""
import os
import struct
import sys
from bisect import bisect_right

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


def emit_ordered(o, segof, base, img, lo, hi, sites, target_of, flat, order, bounds=()):
    """the bytes lo..hi without fixup sites as plain LEDATA (never across a `bounds` address), then
    one LEDATA+FIXUPP per fixup site in `order` (a list of source addresses; a site in two pages
    comes once, at its first place), each in the segment that segof(addr) -> (segment, segment base)
    covers"""
    cut = sorted(s for s in sites if lo <= s < hi)
    a = lo
    for s in cut + [hi]:
        while a < min(s, hi):
            e = min(s, hi, a + 1024)
            k = bisect_right(bounds, a)
            if k < len(bounds):
                e = min(e, bounds[k])
            seg, slo = segof(a)
            if seg is None:                       # a hole between blocks: WLINK's own padding
                a = slo
                continue
            o.ledata(seg, a - slo, bytes(img[a - base:e - base]), [], flat)
            a = e
        a = max(a, s + 4)
    done = set()
    for s in order:
        if s in done or not lo <= s < hi or s not in sites:
            continue
        done.add(s)
        tseg, toff = target_of(s)
        seg, slo = segof(s)
        o.ledata(seg, s - slo, struct.pack('<I', toff), [(0, tseg)], flat)
    missing = [s for s in cut if s not in done]
    if missing:
        raise SystemExit('%d fixup sites not in the page order, first %#x' % (len(missing), missing[0]))




class RealObj:
    def __init__(self, path, addr, dat={}):
        self.path, self.addr, self.dat = path, addr, dat
        segs, pubs, exts = info(path)
        self.exts = set(exts)
        texts = [s for s in segs if s[1].upper() == 'CODE']
        if len(texts) != 1:
            raise SystemExit('%s: %d code segments, want 1' % (path, len(texts)))
        self.text, self.size = texts[0][0], texts[0][2]
        self.pubs = sorted((self.addr + o, n) for s, o, n in pubs if s == self.text)
        self.names = {n for s, o, n in pubs} | info.local     # locals resolve inside the object
        # data segments: _BSS is zero bytes and only its length matters (it eats STACK space); every
        # other non-empty segment needs the address it sits at in the game's data object
        self.datas, self.bss, self.dpubs = [], 0, {}
        for nm, cls, size in segs:
            if cls.upper() == 'CODE' or not size:
                continue
            if cls.upper() == 'BSS' and nm not in dat:
                self.bss += size
                continue
            if nm not in dat:
                raise SystemExit("%s: segment %s has %d bytes, give it an address: --obj %s@%x,%s=ADDR"
                                 % (path, nm, size, os.path.basename(path), addr, nm))
            self.datas.append((nm, cls, size, dat[nm]))
            self.dpubs[nm] = sorted((dat[nm] + o, n) for s, o, n in pubs if s == nm)
        (_, self.recs), = list(L.modules(open(path, 'rb').read()))

    def needs(self, code, cbase, data, dbase):
        """{external name: game address} from the game's bytes at this object's fixup sites"""
        bufs = L.code_of(self.recs, only_code=False)
        where = {self.text: (code, cbase, self.addr)}
        for nm, cls, size, at in self.datas:
            where[nm] = (data, dbase, at)
        out = {}
        for seg, at, loc, selfrel, kind, nm, disp in L.fixups_of(self.recs, only_code=False):
            if kind != 'ext' or loc != 9 or nm in self.names or seg not in where:
                continue
            img, base, oaddr = where[seg]
            game_at = oaddr + at
            v = struct.unpack_from('<I', img, game_at - base)[0]
            if selfrel:
                tgt = (game_at + 4 + v) & 0xFFFFFFFF
            else:
                tgt = (v - struct.unpack_from('<i', bytes(bufs[seg][0]), at)[0]) & 0xFFFFFFFF
            tgt = (tgt - disp) & 0xFFFFFFFF
            if out.setdefault(nm, tgt) != tgt:
                raise SystemExit('%s: %s is %#x and %#x' % (self.path, nm, out[nm], tgt))
        return out


def main():
    exe, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    objs, libs, dsegs = [], [], []
    for i, a in enumerate(sys.argv):
        if a == '--obj':
            p, _, at = sys.argv[i + 1].rpartition('@')
            at, _, rest = at.partition(',')
            dat = dict((kv.split('=') + [''])[:2] for kv in rest.split(',') if kv)
            objs.append(RealObj(p, int(at, 16), {k: int(v, 16) for k, v in dat.items()}))
        elif a == '--lib':
            libs.append(sys.argv[i + 1])
        elif a == '--dseg':
            nm, _, at = sys.argv[i + 1].rpartition('@')
            at, _, cls2 = at.partition(':')          # NAME@ADDR[-LEN][:CLASS], or NAME:CLASS@ADDR
            nm, _, cls = nm.partition(':')
            cls = cls or cls2
            at, _, ln = at.partition('-')
            dsegs.append((nm, cls or 'DATA', int(at, 16), int(ln, 16) if ln else None))
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
    # uninitialised data: with a --dseg _BSS@ADDR:BSS block, placed real _BSS segments sit at their
    # addresses inside it, blob BSS pieces (a length, no bytes) fill the gaps, and STACK starts at the
    # first 4-byte boundary after the last placed one. Without it every real _BSS is appended and eats
    # STACK space - the game's own BSS then moves.
    bssplaced = [at + size for o in objs for nm, cls, size, at in o.datas if cls.upper() == 'BSS']
    sbase = dbase + dinit
    if any(c.upper() == 'BSS' for n, c, a, l in dsegs):
        sbase = (max(bssplaced + [dbase + dinit]) + 3) & ~3
    elif bssplaced:
        raise SystemExit('a real _BSS placed without a --dseg _BSS@ADDR:BSS block')
    stack = dend - sbase - sum(o.bss for o in objs)

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

    # the data object as one SEGDEF per segment-name block (--dseg NAME@ADDR[-LEN][:CLASS]; the block
    # runs to the next block's address unless a length ends it there and leaves the rest to WLINK's
    # 4-byte run alignment). WLINK merges every segment of one name into one run of the data object,
    # in its fixed DGROUP order CONST, CONST2, _DATA, XIB, XI, XIE, YIB, YI, YIE, DATA32, _BSS, STACK
    # (its own map lists those runs), and defines XIB/XIE/YIB/YIE as the bounds of the XI and YI runs:
    # with no XI/YI run of the game's length in the link, inirt386's four fixups aim at the end of
    # DGROUP. Inside a run the segments come in FILES order, so every gap between two placed data
    # segments gets its own object, fed between them (for the library modules data order is code
    # order, tools/link/dataobj.py, so the same FILES sequence fixes both objects).
    dsegs.sort(key=lambda b: b[2])
    if len({b[2] for b in dsegs}) != len(dsegs):
        raise SystemExit('--dseg addresses must be distinct')
    regs, prev = [], dbase
    for i, (nm, cls, at, ln) in enumerate(dsegs):
        end = at + ln if ln else (dsegs[i + 1][2] if i + 1 < len(dsegs) else
                                  sbase if cls.upper() == 'BSS' else dbase + dinit)
        if at > prev and any(data[prev - dbase:min(at, dbase + dinit) - dbase]):
            regs.append(('_DATA', 'DATA', prev, at))   # an unnamed block; a hole of zeros is padding
        if end > at:
            regs.append((nm, cls, at, end))
        prev = max(prev, end)
    if prev < dbase + dinit:
        regs.append(('_DATA', 'DATA', prev, dbase + dinit))
    for nm, cls, lo, hi in regs:
        if cls.upper() == 'BSS':
            if lo < dbase + dinit or hi > sbase:
                raise SystemExit('--dseg block %s@%x:BSS outside the uninitialised bytes' % (nm, lo))
        elif hi <= dbase or lo >= dbase + dinit:
            raise SystemExit('--dseg block %s@%x outside the initialised data bytes' % (nm, lo))

    dsup = sorted((at, at + size, o, nm) for o in objs for nm, cls, size, at in o.datas)
    for at, end, o, nm in dsup:
        if not any(n == nm and lo <= at and end <= hi for n, c, lo, hi in regs):
            raise SystemExit('%s: %s at %#x needs a --dseg %s block around it (dataobj.py measures '
                             'where it sits)' % (o.path, nm, at, nm))
    # the pieces the blobs must supply: a block a real object's data segment sits inside is cut there
    # and every gap becomes its own object (two SEGDEFs of one name in one object always merge), a
    # block no real object touches stays whole in one blob object
    dblobs = []                                    # (lo, hi, name, class) gaps, in address order
    whole = []                                     # blocks nothing is placed inside
    heads = {}                                     # gap head -> the real object it must follow

    def head(a, b, n, c, after):
        """A gap that starts off a 4-byte boundary, right after a placed segment: its bytes up to the
        boundary are fed straight after that object. The gap's body goes late (below), and every
        module fed in between declares its empty CONST/_DATA/... with dword alignment, which WLINK
        honours: the run would be padded up to the boundary before the body (dosret + SETMODE.ASM:
        2 bytes). With the head in place the body starts on the boundary and nothing pads."""
        e = min(b, (a + 3) & ~3)
        if after is None or e == a or os.environ.get('BLOB_NOHEAD'):    # the mutation switch
            return a
        g = (a, e, n, c)
        dblobs.append(g)
        heads[g] = after
        return e
    for n, c, lo, hi in regs:
        inside = [(at, end, o, nm) for at, end, o, nm in dsup if lo <= at < hi]
        if not inside:
            whole.append((lo, hi, n, c))
            dblobs.append((lo, hi, n, c))
            continue
        a, last = lo, None
        for at, end, o, nm in inside:
            if nm != n:
                raise SystemExit('--dseg block %s@%x..%x would hold %s of %s' % (n, lo, hi, nm, o.path))
            if at > a:
                a = head(a, at, n, c, last)
                if at > a:
                    dblobs.append((a, at, n, c))
            a, last = max(a, end), o
        if a < hi:
            a = head(a, hi, n, c, last)
            if a < hi:
                dblobs.append((a, hi, n, c))
    split = [g for g in dblobs if g not in whole and g not in heads]

    def owner(t):
        """('piece', p) / ('dpiece', g) / ('obj', o) / ('dat', (o, seg)) / ('data', None)"""
        for p in pieces:
            if p[0] <= t < p[1]:
                return 'piece', p
        for g in dblobs:
            if g[0] <= t < g[1]:
                return 'dpiece', g
        for o in objs:
            if o.addr <= t < o.addr + o.size:
                return 'obj', o
            for nm, cls, size, at in o.datas:
                if at <= t < at + size:
                    return 'dat', (o, nm)
        if sbase <= t <= dend:                       # uninitialised: the STACK segment's run
            return 'data', None
        if t == cend:
            return 'piece', pieces[-1]
        raise SystemExit('target %#x is in no object (a data gap WLINK pads?)' % t)

    # publics the blobs must define for the real objects
    need = {}
    for o in objs:
        for nm, t in o.needs(code, cbase, data, dbase).items():
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
            if kind == 'dat':                      # inside another object's data segment
                oo, seg = x
                pubs = oo.dpubs.get(seg)
                if not pubs:
                    raise SystemExit('%s: a data fixup at %#x aims inside %s.%s, which has no public '
                                     'in that segment to reach it from' % (own, t, oo.path, seg))
                pa, pn = min(pubs, key=lambda q: (abs(t - q[0]), q[0] > t))
                return ext(pn), t - pa
            if kind == 'dpiece':
                if own == x:
                    return ('seg', t)
                return ext('blob_%x' % x[0]), t - x[0]
            if kind == 'data':                       # uninitialised data, in the STACK run
                return ext('blob_tail'), t - sbase
            if kind == 'piece':
                if own == x:
                    return ('seg', t)
                return ext('blob_%x' % x[0]), t - x[0]
            pa, pn = min(x.pubs, key=lambda q: (abs(t - q[0]), q[0] > t))
            return ext(pn), t - pa
        return go

    # FILES order fixes both merged objects. A data blob goes as late as the run allows (that is where
    # the game's own data modules sit relative to the fixup records, and feeding a block early costs
    # thousands of fixup bytes), but it has to precede the first placed data segment that follows it in
    # that run, and it has to follow the first module that names the run: inirt386 names XIB/XIE/YIB/YIE,
    # and WLINK only lets those bookkeeping symbols wrap the XI/YI runs when it heard of them before the
    # run was laid out, otherwise they land at the end of DGROUP.
    segrefs = {}                                   # segment name -> objects naming it in a fixup
    for o in objs:
        for _, _, _, _, kind, nm, _ in L.fixups_of(o.recs, only_code=False):
            if kind == 'seg':
                segrefs.setdefault(nm, set()).add(id(o))
    oidx = {id(o): i for i, (k, o) in enumerate(order) if k == 'obj'}

    def window(parts):
        """(after, before): the FILES positions the parts may be fed between"""
        n, after, before = len(order), -1, len(order)
        for lo, hi, name, cls in parts:
            for at, end, o, nm in dsup:
                if nm != name:
                    continue
                if at >= hi:
                    before = min(before, oidx[id(o)])
                elif end <= lo:
                    after = max(after, oidx[id(o)])
            for nm in (name + 'B', name + 'E'):   # XIB/XIE-style names, not the run's own name
                if nm in segrefs:
                    after = max(after, min(oidx[i] for i in segrefs[nm] if i in oidx))
        if after >= before:
            raise SystemExit('--dseg %s@%x cannot reach the link order: it must follow %s but precede %s'
                             % (parts[0][2], parts[0][0], after, before))
        return after, before

    def slot(parts):
        return window(parts)[1]                     # as late as the run allows

    blobs = {}                                     # FILES position -> data blob objects fed there
    wg = {}
    for g in whole:                                # blocks nothing is placed inside travel together
        wg.setdefault(slot([g]), []).append(g)
    for a in wg:
        blobs.setdefault(a, []).append(sorted(wg[a], key=lambda g: g[0]))
    # Pieces of different segment names share one object wherever their windows overlap, fed as late
    # as the narrowest allows: emit_ordered then interleaves their fixups the way the game's page
    # lists do, where one blob module brought XI and YI entries together (page 0x6c000: three such
    # modules ahead of isdbcs; and the _DATA tail's fixups with XI/YI/DATA32's).
    groups = []                                    # [after, before, parts]
    for g in sorted(split, key=lambda g: window([g])[1]):
        a, b = window([g])
        for gr in groups:
            if all(q[2] != g[2] for q in gr[2]) and max(a, gr[0]) < min(b, gr[1]):
                gr[0], gr[1] = max(a, gr[0]), min(b, gr[1])
                gr[2].append(g)
                break
        else:
            groups.append([a, b, [g]])
    for a, b, ps in groups:
        blobs.setdefault(b, []).append(sorted(ps, key=lambda q: q[0]))
    for g, o in heads.items():                     # straight after the segment the head continues
        blobs.setdefault(oidx[id(o)] + 1, []).insert(0, [g])
    main = max((ps for ps in sum(blobs.values(), [])), key=lambda ps: ps[-1][0])   # STACK travels here

    def write_dblob(parts, main=False):
        lo0, hi_last = parts[0][0], parts[-1][1]
        ob = Obj('datablob_%x' % lo0)
        segidx = [ob.segdef(n, c, hi - lo) for lo, hi, n, c in parts]
        sseg = ob.segdef('STACK', 'STACK', stack, combine=5) if main else None
        ob.grpdef('DGROUP', segidx + ([sseg] if sseg else []))
        flat = ob.grpdef('FLAT', [])
        extmap = {}
        go = resolver(ob, parts if len(parts) == 1 else None, flat, extmap)
        sites = {s for s in fix for lo, hi, n, c in parts if lo <= s < hi}
        targets = {s: go(s) for s in sorted(sites)}
        if extmap:
            ob.extdef([nm for nm, i in sorted(extmap.items(), key=lambda q: q[1])])
        for i, (lo, hi, n, c) in enumerate(parts):
            pub = [('blob_%x' % lo, 0)] + sorted((nm, t - lo) for nm, t in need.items() if lo <= t < hi)
            if pub:
                ob.pubdef(0, segidx[i], pub)
        if sseg:                                   # uninitialised data: publics in the tail segment
            tail = sorted((nm, t - sbase) for nm, t in need.items() if sbase <= t <= dend)
            ob.pubdef(0, sseg, [('blob_tail', 0)] + tail)

        def segof(t):
            for i, (lo, hi, n, c) in enumerate(parts):
                if lo <= t < hi:
                    if c.upper() == 'BSS':          # a length only: no LEDATA
                        return None, hi
                    return segidx[i], lo
            for lo, hi, n, c in parts:             # a hole between blocks: nothing to emit up to lo
                if t < lo:
                    return None, lo
            return (sseg, sbase) if sseg else (segidx[-1], hi_last)

        def tgt(s):
            r = targets[s]
            if r[0] == 'seg':
                i, slo = segof(r[1])
                return segidx[i], r[1] - slo
            return r
        emit_ordered(ob, segof, dbase, data, lo0, hi_last, sites, tgt, flat, dorder,
                     bounds=sorted({x for ps in parts for x in ps[:2]}))
        ob.modend()
        fn = 'DATABLOB_%x.OBJ' % lo0
        ob.write(os.path.join(out, fn))
        return fn

    files = []
    for n, (kind, x) in enumerate(order):
        if kind == 'obj':                          # copied next to the blobs: WLINK runs there under Wine
            fn = 'O_%x.OBJ' % x.addr
            open(os.path.join(out, fn), 'wb').write(open(x.path, 'rb').read())
            files.append(fn)
        else:
            lo, hi = x
            ob = Obj('codeblob_%x' % lo)
            text = ob.segdef('_TEXT', 'CODE', hi - lo)
            flat = ob.grpdef('FLAT', [])
            extmap = {}
            go = resolver(ob, x, flat, extmap)
            sites = {s for s in fix if lo <= s < hi}
            targets = {s: go(s) for s in sorted(sites)}
            if extmap:
                ob.extdef([nm for nm, i in sorted(extmap.items(), key=lambda q: q[1])])
            pubs = [('blob_%x' % lo, 0)] + sorted((nm, t - lo) for nm, t in need.items() if lo <= t < hi)
            ob.pubdef(0, text, pubs)

            def tgt(s, text=text):
                r = targets[s]
                return (text, r[1] - lo) if r[0] == 'seg' else r
            emit_ordered(ob, lambda t, text=text, lo=lo: (text, lo), cbase, code, lo, hi, sites, tgt,
                         flat, corder)
            ob.modend((flat, text, le.eip - (lo - cbase)) if lo <= cbase + le.eip < hi else None)
            fn = 'CODEBLOB_%x.OBJ' % lo
            ob.write(os.path.join(out, fn))
            files.append(fn)
        for ps in blobs.get(n + 1, []):
            files.append(write_dblob(ps, ps is main))
    open(os.path.join(out, 'FILES.LNK'), 'w').write(''.join('file %s\n' % f for f in files))
    print('%d code pieces, %d real objects (%d bytes, %d data segments), %d publics defined for them;'
          ' data %#x in %d blocks %s, %d data pieces, stack %#x'
          % (len(pieces), len(objs), sum(o.size for o in objs), len(dsup), len(need), dinit, len(regs),
             ','.join('%s@%x' % (n, lo) for n, c, lo, hi in regs), len(dblobs), stack))


if __name__ == '__main__':
    main()
