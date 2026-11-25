#!/usr/bin/env python3
"""wdbg.py: decode the Watcom debug-info block (format 3.x) appended to an executable.

usage: wdbg.py EXE [modules|globals|addrs|all]

The block ends the file: a 14-byte master header (signature 0x8386, versions, language-table and
segment-table sizes, total size) sits in the last 14 bytes and `debug_size` counts back from the
end of the file to the block's start, where the language table and segment table come first and
then the sections, each with a
section header followed by its module, global and address tables (Open Watcom's wdbginfo.h,
format 3). Addresses are (segment, offset) with segment = the LE object number.

  modules: index, language, name
  globals: segment:offset, module, name
  addrs:   segment:offset size module   (the code/data each module contributed, in address order)
"""
import struct
import sys

MASTER = '<HBBBBHHI'


def pstr(b, p):
    n = b[p]
    return b[p + 1:p + 1 + n].decode('latin1'), p + 1 + n


def decode(path):
    data = open(path, 'rb').read()
    sig, emaj, emin, omaj, omin, lang_size, seg_size, dbg_size = struct.unpack_from(MASTER, data, len(data) - 14)
    if sig != 0x8386:
        raise SystemExit('%s: no Watcom debug trailer (signature %#x)' % (path, sig))
    start = len(data) - dbg_size
    p = start
    langs = [s.decode('latin1') for s in data[p:p + lang_size].split(b'\0') if s]
    p += lang_size
    p += seg_size
    end = len(data) - 14
    sections = []
    while p < end:
        mod_off, gbl_off, addr_off, sec_size, sec_id = struct.unpack_from('<IIIIH', data, p)
        if sec_size == 0:
            break
        base = p
        mods, p2 = [], base + mod_off
        while p2 < base + gbl_off:
            lang, = struct.unpack_from('<H', data, p2)
            p2 += 2 + 3 * 6
            name, p2 = pstr(data, p2)
            mods.append((langs[lang] if lang < len(langs) else lang, name))
        gbls, p2 = [], base + gbl_off
        while p2 < base + addr_off:
            off, seg, mod, kind = struct.unpack_from('<IHHB', data, p2)
            name, p2 = pstr(data, p2 + 9)
            gbls.append((seg, off, mod, kind, name))
        addrs, p2 = [], base + addr_off
        while p2 < base + sec_size:
            off, seg, num = struct.unpack_from('<IHH', data, p2)
            p2 += 8
            a = off
            for _ in range(num):
                size, mod = struct.unpack_from('<IH', data, p2)
                p2 += 6
                addrs.append((seg, a, size, mod))
                a += size
        sections.append((sec_id, mods, gbls, addrs))
        p = base + sec_size
    return (emaj, emin, omaj, omin), langs, sections


def main():
    path = sys.argv[1]
    what = sys.argv[2] if len(sys.argv) > 2 else 'all'
    ver, langs, sections = decode(path)
    print('# format %d.%d obj %d.%d, languages %s, %d section(s)' % (ver + (langs, len(sections))))
    for sec_id, mods, gbls, addrs in sections:
        if what in ('modules', 'all'):
            for i, (lang, name) in enumerate(mods):
                print('mod\t%d\t%s\t%s' % (i, lang, name))
        if what in ('globals', 'all'):
            for seg, off, mod, kind, name in sorted(gbls):
                print('gbl\t%d:%#07x\t%d\t%s\t%s' % (seg, off, mod, mods[mod][1] if mod < len(mods) else '?', name))
        if what in ('addrs', 'all'):
            for seg, off, size, mod in sorted(addrs):
                print('addr\t%d:%#07x\t%#x\t%d\t%s' % (seg, off, size, mod, mods[mod][1] if mod < len(mods) else '?'))


if __name__ == '__main__':
    main()
