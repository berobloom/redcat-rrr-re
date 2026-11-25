#!/usr/bin/env python3
"""modmatch.py: find the modules of a program with Watcom debug info inside another LE program.

usage: modmatch.py SRC.EXE DST.EXE [--min N] [--tsv] [--funcs]

SRC must carry a Watcom debug block (REKEN.EXE does; tools/wdbg.py decodes it), which gives each
module's address range in SRC's code object. Each module's bytes are taken from SRC with these
masked: every LE fixup site (4 bytes; absolute addresses differ between programs) and the rel32
operand of every call/jmp/jcc whose target is outside the module (another module's address).
The masked pattern is then searched for in DST's code object. A hit counts only when DST's own
fixup sites inside the hit are exactly the masked fixup sites of SRC - two programs that share a
module have their fixups at the same offsets in it, so this is the check that a chance byte match
fails.

Output per module (in SRC's order): the DST address(es), size, and the module name. `--min N`
leaves out modules shorter than N bytes (default 8): a short module matches by chance.
`--funcs` cuts each module further at SRC's code globals (the debug block's public names) and
searches for every piece on its own: a module rebuilt with other options or other headers for
DST can still share most of its functions. A piece runs from its global to the next one, so it
carries the static functions that follow it.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import capstone  # noqa: E402
import leparse  # noqa: E402
import wdbg  # noqa: E402


def load(path):
    le = leparse.LE(open(path, 'rb').read())
    imgs, _ = le.load()
    code = le.objects[0]
    sites = set()
    for k in range(code['page_count']):
        page = code['page_index'] + k
        for stype, so, tobj, toff in le.fixups(page):
            at = k * le.page_size + so
            if 0 <= at < code['vsize']:
                sites.add(code['base'] + at)
    return code['base'], bytes(imgs[1]), sites


MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


def pattern(base, img, sites, start, size):
    """bytes and mask (True = compare) of the module at start..start+size"""
    b = img[start - base:start - base + size]
    mask = [True] * size
    fix = []
    for s in sites:
        if start <= s < start + size:
            fix.append(s - start)
            for i in range(4):
                if s - start + i < size:
                    mask[s - start + i] = False
    # rel32 branches leaving the module; decode linearly, resyncing over masked data
    off = 0
    while off < size:
        if not mask[off]:
            off += 1
            continue
        insn = next(MD.disasm(b[off:off + 16], start + off), None)
        if insn is None:
            off += 1
            continue
        if insn.mnemonic in ('call', 'jmp') or (insn.mnemonic.startswith('j') and insn.size >= 5):
            if insn.size >= 5 and insn.op_str.startswith('0x'):
                t = int(insn.op_str, 16)
                if not start <= t < start + size:
                    for i in range(insn.size - 4, insn.size):
                        if off + i < size:
                            mask[off + i] = False
        off += insn.size
    return b, mask, sorted(fix)


def anchor(b, mask):
    """longest run of compared bytes: (offset, length)"""
    best, cur, at = (0, 0), 0, 0
    for i, m in enumerate(mask + [False]):
        if m:
            if cur == 0:
                at = i
            cur += 1
        else:
            if cur > best[1]:
                best = (at, cur)
            cur = 0
    return best


def find(b, mask, fix, dbase, dimg, dsites):
    aoff, alen = anchor(b, mask)
    if alen < 4:
        return []
    key = b[aoff:aoff + alen]
    hits, p = [], dimg.find(key)
    while p >= 0:
        s = p - aoff
        if s >= 0 and s + len(b) <= len(dimg):
            seg = dimg[s:s + len(b)]
            if all(not m or x == y for x, y, m in zip(b, seg, mask)):
                dfix = sorted(x - (dbase + s) for x in dsites if dbase + s <= x < dbase + s + len(b))
                if dfix == fix:
                    hits.append(dbase + s)
        p = dimg.find(key, p + 1)
    return hits


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    minlen = 8
    if '--min' in sys.argv:
        minlen = int(sys.argv[sys.argv.index('--min') + 1])
        args = [a for a in args if a != str(minlen)]
    tsv = '--tsv' in sys.argv
    src, dst = args
    _, _, sections = wdbg.decode(src)
    mods = sections[0][1]
    ranges = sorted((off, size, m) for seg, off, size, m in sections[0][3] if seg == 1)
    names = {}
    if '--funcs' in sys.argv:
        gbls = sorted((off, n) for seg, off, m, kind, n in sections[0][2] if seg == 1)
        pieces = []
        for off, size, m in ranges:
            cuts = [(o, n) for o, n in gbls if off < o < off + size]
            starts = [(off, None)] + cuts
            for i, (o, n) in enumerate(starts):
                e = starts[i + 1][0] if i + 1 < len(starts) else off + size
                pieces.append((o, e - o, m))
                names[o] = n
        firsts = {o: n for o, n in gbls}
        for o, _, _ in pieces:
            if names.get(o) is None:
                names[o] = firsts.get(o, '')
        ranges = pieces
    sbase, simg, ssites = load(src)
    dbase, dimg, dsites = load(dst)
    found = total = 0
    for off, size, m in ranges:
        name = mods[m][1] + ((' ' + names[off]) if names.get(off) else '')
        if size < minlen:
            continue
        b, mask, fix = pattern(sbase, simg, ssites, sbase + off, size)
        hits = find(b, mask, fix, dbase, dimg, dsites)
        total += size
        if hits:
            found += size
        if tsv:
            print('%#x\t%#x\t%s\t%s' % (sbase + off, size, ','.join('%#x' % h for h in hits) or '-', name))
        else:
            print('%#08x %6d  %-24s %s' % (sbase + off, size, ','.join('%#x' % h for h in hits) or '-', name))
    print('# %d of %d bytes of %s code modules (>= %d B) found in %s'
          % (found, total, os.path.basename(src), minlen, os.path.basename(dst)), file=sys.stderr)


if __name__ == '__main__':
    main()
