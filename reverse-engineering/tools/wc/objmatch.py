#!/usr/bin/env python3
"""objmatch.py: compare the functions of a compiled OMF .OBJ with an LE program, function by function.

usage: objmatch.py PROG FILE.OBJ [0xADDR ...]

Each public code symbol's bytes (up to the next public at a higher offset) are searched in PROG's code object with the bytes that
fixups fill masked. With addresses (one per public, in offset order; 0 skips one; none given: each public's address in
analysis/PROG_names.txt, by its name without Watcom's _ decoration) each function is compared at
its address: MATCH, or the first differing byte with both sides shown, so a near miss reads like a diff.
Otherwise it is searched for (needs a run of 4 bytes no fixup touches): FOUND at the address(es), or not found.

Then the fixups of the matched functions are checked against the game's bytes (32-bit offsets, absolute or
self-relative): an external named in analysis/PROG_names.txt must point at that address; one not named is
listed as a proposed name; references into one of the module's own segments must agree on one base address
per segment (the module's _TEXT base follows from the function addresses).

A near rel32 call or jump between two functions of the same object carries no fixup (the compiler
resolves it), so its 4 bytes depend on where our build places the target. Such a function is a MATCH
(the line says how many branches were compared by target) when every other byte agrees and each
differing rel32 branch lands on the same function + offset in both builds: ours resolved through the
object's publics, the game's through the address that function is compared at (iter 74, reccmp's rule;
demo_rec, pic_show and fade_out had only these bytes differing, iter 65 checked their targets by hand).

Last, the data segments (CONST, CONST2, _DATA, ...): each is compared with the game's data object at
the base its references give (the segment bases the fixup check derives, or a _DATA public's address
from the names file), fixup bytes masked; a segment counts only when every byte agrees, and the last
line is `data: N of M bytes identical in place` (src/check.sh sums it; src/data.baseline holds it).
"""
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import libmatch as L  # noqa: E402


def publics(recs):
    lnames, segs, out = [None], [None], []
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
                ln, p = L.index(b, p0 + lsize)
                cl, p = L.index(b, p)
                if 0 < ln < len(lnames) and 0 < cl < len(lnames) and p + 1 == len(b):
                    break
            segs.append(lnames[ln])
        elif t in (0x90, 0x91):
            g, p = L.index(b, 0)
            s, p = L.index(b, p)
            if g == 0 and s == 0:
                p += 2
            while p < len(b):
                n = b[p]
                name = b[p + 1:p + 1 + n].decode('latin1')
                p += 1 + n
                off = struct.unpack_from('<I' if t == 0x91 else '<H', b, p)[0]
                p += 4 if t == 0x91 else 2
                _, p = L.index(b, p)
                out.append((segs[s], off, name))
    return out


def names(prog):
    out = {}
    try:
        for line in open(os.path.join(L.ROOT, 'reverse-engineering', 'analysis', prog + '_names.txt')):
            f = line.split('#')[0].split()
            if len(f) >= 2 and f[0].startswith('0x'):
                out[f[1]] = int(f[0], 16)
    except OSError:
        pass
    return out


def plain(sym):
    """Watcom's register-convention decoration: name_ for functions, _name for data"""
    return sym[:-1] if sym.endswith('_') else sym[1:] if sym.startswith('_') else sym


def check_fixups(recs, pubs, placed, code, cbase, known):
    """placed: {public name: game address} for the functions that matched"""
    pubat = {name: (seg, off) for seg, off, name in publics(recs)}
    funcs = sorted((off, seg, name) for seg, off, name in pubs)
    bases, bad, new, ok = {}, [], {}, 0
    for name, addr in placed.items():
        seg, off = pubat[name]
        bases.setdefault(seg, {}).setdefault(addr - off, []).append(name)
    segbytes = L.code_of(recs)
    for seg, at, loc, selfrel, kind, nm, disp in L.fixups_of(recs):
        if loc != 9:
            continue
        if not selfrel:                            # Watcom keeps the addend in the data bytes
            disp += struct.unpack_from('<i', bytes(segbytes[seg][0]), at)[0]
        fn = None
        for off, s2, name in funcs:
            if s2 == seg and off <= at:
                fn = (off, name)
        if not fn or fn[1] not in placed:
            continue
        game_at = placed[fn[1]] + at - fn[0]
        v = struct.unpack_from('<I', code, game_at - cbase)[0]
        tgt = (game_at + 4 + v) & 0xFFFFFFFF if selfrel else v
        if kind == 'ext' and nm in pubat:
            kind, s2, o2 = 'seg', pubat[nm][0], pubat[nm][1]
            bases.setdefault(s2, {}).setdefault(tgt - disp - o2, []).append('%s@0x%x' % (fn[1], game_at))
            continue
        if kind == 'seg':
            bases.setdefault(nm, {}).setdefault(tgt - disp, []).append('%s@0x%x' % (fn[1], game_at))
            continue
        want = known.get(plain(nm))
        if want is None:
            new.setdefault(plain(nm), set()).add(tgt - disp)
        elif want + disp == tgt:
            ok += 1
        else:
            bad.append('%s@0x%x: %s+%d should be 0x%x, game has 0x%x' % (fn[1], game_at, nm, disp, want + disp, tgt))
    print('fixups: %d named targets agree' % ok)
    for b in bad:
        print('  WRONG ' + b)
    for seg, bs in sorted(bases.items()):
        if len(bs) == 1:
            print('  segment %-8s base 0x%06x (%d references agree)' % (seg, next(iter(bs)), len(next(iter(bs.values())))))
        else:
            print('  segment %-8s DISAGREES: %s' % (seg, ', '.join('0x%x (%s)' % (b, ' '.join(v[:3])) for b, v in bs.items())))
    for nm, addrs in sorted(new.items()):
        print('  unnamed %-16s %s' % (nm, ' '.join('0x%06x' % a for a in sorted(addrs))))
    return bases


def compare_data(recs, bases, prog, known):
    """Each data segment (CONST, CONST2, _DATA, ...) against the game's data object at the base its
    references give (bases from check_fixups; a _DATA public named in the names file gives one too).
    Fixup bytes are masked. One line per segment: MATCH, or how many bytes agree at the best base and
    the first difference; a segment nobody references is searched for. Returns (identical, total)."""
    le = os.path.join(L.ROOT, '.work', 'le')
    meta = json.load(open(os.path.join(le, prog + '.json')))
    data = open(os.path.join(le, prog + '.obj2.bin'), 'rb').read()
    dbase = meta['objects'][1]['base']
    segs = L.code_of(recs, only_code=False)
    code = L.code_of(recs)
    pubs = publics(recs)
    same = tot = 0
    for seg in sorted(segs):
        if seg in code:
            continue
        buf, mask = segs[seg]
        cand = dict((b, len(v)) for b, v in bases.get(seg, {}).items())
        for sg, off, name in pubs:
            a = known.get(plain(name))
            if sg == seg and a is not None:
                cand[a - off] = cand.get(a - off, 0) + 1
        n = len(buf)
        tot += n

        def score(b):
            o = b - dbase
            if o < 0 or o + n > len(data):
                return -1, None
            eq = [mask[k] or buf[k] == data[o + k] for k in range(n)]
            return sum(eq), next((k for k in range(n) if not eq[k]), None)
        if not cand:
            hits = L.find(data, buf, mask)
            if len(hits) == 1:
                cand[hits[0] + dbase] = 0
        if not cand:
            print('  data %-8s %5d bytes  no reference, not found' % (seg, n))
            continue
        best = max(cand, key=lambda b: (score(b)[0], cand[b]))
        k, first = score(best)
        k = max(k, 0)
        same += k if first is None else 0
        extra = '' if len(cand) == 1 else ' (%d bases referenced)' % len(cand)
        if first is None:
            print('  data %-8s %5d bytes  IDENTICAL at 0x%06x%s' % (seg, n, best, extra))
        else:
            o = best - dbase
            print('  data %-8s %5d bytes  differs at 0x%06x: %d of %d agree, first at +%d%s\n'
                  '      ours  %s\n      game  %s' % (
                      seg, n, best, k, n, first, extra,
                      ' '.join('..' if mask[j] else '%02x' % buf[j] for j in range(max(0, first - 4), min(n, first + 12))),
                      ' '.join('%02x' % data[o + j] for j in range(max(0, first - 4), min(n, first + 12)))
                      if 0 <= o else '-'))
    return same, tot


def rel32_by_target(b, m, code, a, cbase, seg, off, pubs, addr_of):
    """Number of rel32 branches that differ only in displacement but land on the same function +
    offset (see the module doc), or None when anything else differs."""
    import capstone
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    game = code[a:a + len(b)]
    ours = list(md.disasm(bytes(b), off))
    theirs = list(md.disasm(game, a + cbase))
    if sum(i.size for i in ours) != len(b) or len(ours) != len(theirs):
        return None
    n, k = 0, 0
    starts = sorted((o, nm) for sg, o, nm in pubs if sg == seg)
    for i, j in zip(ours, theirs):
        if i.size != j.size:
            return None
        diff = [x for x in range(i.size) if not m[k + x] and b[k + x] != game[k + x]]
        if diff:
            op = bytes(b[k:k + i.size])
            imm = 1 if op[0] in (0xe8, 0xe9) else 2 if op[0] == 0x0f and 0x80 <= op[1] <= 0x8f else 0
            if not imm or i.size != imm + 4 or op[:imm] != game[k:k + imm] or min(diff) < imm:
                return None
            t_ours = i.address + i.size + struct.unpack_from('<i', op, imm)[0]
            t_game = j.address + j.size + struct.unpack_from('<i', game, k + imm)[0]
            f = [(o, nm) for o, nm in starts if o <= t_ours]
            if not f or not addr_of(f[-1][1]) or addr_of(f[-1][1]) + t_ours - f[-1][0] != t_game:
                return None
            n += 1
        k += i.size
    return n


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    prog, obj = sys.argv[1], sys.argv[2]
    want = [int(a, 16) for a in sys.argv[3:]]
    le = os.path.join(L.ROOT, '.work', 'le')
    meta = json.load(open(os.path.join(le, prog + '.json')))
    code = open(os.path.join(le, prog + '.obj1.bin'), 'rb').read()
    cbase = meta['objects'][0]['base']
    (_, recs), = list(L.modules(open(obj, 'rb').read()))
    segs = L.code_of(recs)
    pubs = sorted(p for p in publics(recs) if p[0] in segs)
    known = names(prog)
    if not want:                                   # no addresses given: the names file's, by function name
        want = [known.get(plain(name), 0) for _, _, name in pubs]
    fi = 0
    placed = {}
    for i, (seg, off, name) in enumerate(pubs):
        buf, mask = segs[seg]
        # the next public at a higher offset (the compiler gives functions with identical code one body)
        end = next((q[1] for q in pubs[i + 1:] if q[0] == seg and q[1] > off), len(buf))
        b, m = buf[off:end], mask[off:end]
        a = want[fi] - cbase if fi < len(want) and want[fi] else None
        d = None if a is None else next((k for k in range(len(b)) if not m[k] and code[a + k] != b[k]), None)
        hits = L.find(code, b, m)
        r = None
        if a is not None and d is not None:
            r = rel32_by_target(b, m, code, a, cbase, seg, off, pubs,
                                lambda nm: want[[q[2] for q in pubs].index(nm)] if nm in [q[2] for q in pubs] else None)
        if r:
            placed[name] = want[fi]
            print('%-20s %3d bytes  MATCH at 0x%06x (fixup bytes not compared; %d rel32 branch%s compared by target)'
                  % (name, len(b), want[fi], r, '' if r == 1 else 'es'))
        elif a is not None and d is None:
            placed[name] = want[fi]
            print('%-20s %3d bytes  MATCH at 0x%06x (fixup bytes not compared)' % (name, len(b), want[fi]))
        elif hits:
            if len(hits) == 1:
                placed[name] = hits[0] + cbase
            print('%-20s %3d bytes  FOUND at 0x%06x%s' % (name, len(b), hits[0] + cbase,
                                                        '' if len(hits) == 1 else ' (+%d more)' % (len(hits) - 1)))
        elif a is None:
            print('%-20s %3d bytes  not found%s' % (name, len(b), '' if b and len(b) > 8 else ' (too short to search)'))
        else:
            print('%-20s %3d bytes  differs from 0x%06x at +%d:\n      ours  %s\n      game  %s' % (
                name, len(b), want[fi], d,
                ' '.join('..' if m[k] else '%02x' % b[k] for k in range(max(0, d - 4), min(len(b), d + 12))),
                ' '.join('%02x' % code[a + k] for k in range(max(0, d - 4), min(len(b), d + 12)))))
        fi += 1
    bases = check_fixups(recs, pubs, placed, code, cbase, known) if placed else {}
    same, tot = compare_data(recs, bases, prog, known)
    print('data: %d of %d bytes identical in place' % (same, tot))
    return 0


if __name__ == '__main__':
    sys.exit(main())
