#!/usr/bin/env python3
"""libmap.py: give every byte of RRR.EXE's library range (0x1B916-0x4142F) an owner.

usage: libmap.py [--tsv OUT] [--gaps]

Sources, each a byte-for-byte match with fixups masked:
  1. tools/modmatch.py REKEN.EXE RRR.EXE --funcs: REKEN's Watcom debug block names its modules
     (C:\\PROTMOD\\WINLIB\\..., C:\\PROTMOD\\SOUNDLIB\\..., Fastgraph's *.ASM, clib) and their
     functions; each piece found in RRR takes REKEN's module name.
  2. tools/wc/libmatch.py -l with Watcom 10.6's CLIB3R, MATH387R, EMU387 and MATH3R.
  3. Smacker: SMACKER below, two ranges REKEN lacks and INTRO, RRRMENU and install.exe share
     byte for byte; the first is called only from Davilex's smacker.c wrapper (SMACKER_C), the
     second only from the first. The script re-checks the callers (every E8 into either range
     from outside both must come from SMACKER_C) and stops if that ever fails. STRONGLY SUPPORTED:
     no RAD string names the code.
  4. What is still unmatched but sits in a function whose strings name a Davilex library source
     file (rrr_funcs.tsv's source column: rctools.c, fileio.c, smacker.c, ...) is owned by
     `davilex-diff`: that module's code, compiled differently from REKEN's copy (not byte-matched).
The TSV's evidence column says which of these gave the owner: libmatch (the Watcom 10.6 LIB itself),
reken (the same bytes as REKEN's module of that name: Watcom start-up and emu387 are only that,
10.6's EMU387.LIB and cstrt386 _TEXT do not match), smacker, strings, or - (unknown).
A range found by both 1 and 2 keeps the libmatch name (the library module is the stronger
evidence). What nothing finds is `unknown`. Owners are classed:
  clib, math, emu (Watcom), winlib, soundlib (Davilex, C:\\PROTMOD, byte-matched to REKEN),
  fastgraph (the upper-case *.ASM modules: Ted Gruber's Fastgraph is assembly), davilex-asm
  (doscalls.asm, cpy.asm), startup (cstrt386 and its helpers), smacker, davilex-diff, unknown.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOOLS = os.path.join(ROOT, 'reverse-engineering', 'tools')
PY = os.path.join(ROOT, '.work', 'venv', 'bin', 'python')
WLIB = os.path.join(ROOT, '.work', 'watcom', 'dos', 'C106', 'WATCOM', 'LIB386')
LIBS = [('clib', os.path.join(WLIB, 'DOS', 'CLIB3R.LIB')), ('math', os.path.join(WLIB, 'MATH387R.LIB')),
        ('emu', os.path.join(WLIB, 'DOS', 'EMU387.LIB')), ('math', os.path.join(WLIB, 'MATH3R.LIB'))]
# Fastgraph's 32-bit libraries, when present (not in git; research/discoveries.md "Executables" says
# how they were made): 4.03 = 4.00 from FGL400-2.ZIP + Fastgraph's own updat401/402/403 RTPatches,
# which match every Fastgraph module in RRR; else 4.00, which matches the 13 that 4.01-4.03 left alone.
FG403 = os.path.join(ROOT, '.work', 'dl', 'fgchain', 'W')
FG400 = os.path.join(ROOT, '.work', 'dl', 'dm', 'fg400')
for d in (FG403, FG400):
    if os.path.exists(os.path.join(d, 'FG32.LIB')):
        LIBS += [('fastgraph', os.path.join(d, 'FG32.LIB')), ('fastgraph', os.path.join(d, 'FG32DPMI.LIB'))]
        break
LIB_START, CODE_END = 0x1B916, 0x10000 + 0x31430
STARTUP = {'cstrt386', 'cmain386', 'inirt386', 'initargv', 'stack386', 'cinit', 'argcv386', 'seg386',
           'dosinite', 'chk8087', 'ini87386', 'hook387', 'inirmsel', 'enterdb', 'set386', 'chint386'}


SMACKER = [(0x2ecb0, 0x31dd9), (0x3d500, 0x41430)]
SMACKER_C = (0x21a67, 0x22ab5)
DAVILEX_SRC = {'rctools.c', 'rcascii.c', 'rcbutton.c', 'rcpcx.c', 'rcobj.c', 'rcimgret.c', 'fileio.c', 'read.c',
               'easy.c', 'commands.c', 'mouse.c', 'irq7.c', 'audio.c', 'sndbuf.c', 'smacker.c', 'stdlib.c',
               'filepath.c', 'pal.c', 'beep.c', 'int9v2.c', 'automato.c', 'tracker.c', 'fxs.c'}


def check_smacker():
    sys.path.insert(0, TOOLS)
    from lelib import Image
    im = Image('rrr')
    code, base = im.code, im.code_base
    inside = lambda a: any(s <= a < e for s, e in SMACKER)
    n = 0
    for i in range(len(code) - 5):
        if code[i] == 0xE8:
            t = (base + i + 5 + int.from_bytes(code[i + 1:i + 5], 'little', signed=True)) & 0xffffffff
            if inside(t) and not inside(base + i):
                if not SMACKER_C[0] <= base + i < SMACKER_C[1]:
                    raise SystemExit('Smacker range called from %#x, outside smacker.c' % (base + i))
                n += 1
    return n


def klass(name, lib=None):
    if lib:
        return 'startup' if name in STARTUP else lib
    mod = name.split()[0]
    if 'WINLIB' in mod:
        return 'winlib'
    if 'SOUNDLIB' in mod:
        return 'soundlib'
    if mod.endswith('.ASM'):
        return 'fastgraph'
    if mod.endswith('.asm'):
        return 'davilex-asm'
    if mod in STARTUP:
        return 'startup'
    if mod == 'emu387':
        return 'emu'
    return 'clib'


def main():
    spans = []          # (start, end, class, name, source)
    out = subprocess.run([PY, os.path.join(TOOLS, 'modmatch.py'), os.path.join(ROOT, '.work/iso/REKEN.EXE'),
                          os.path.join(ROOT, '.work/iso/RRR.EXE'), '--funcs', '--tsv'],
                         capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        if line.startswith('#'):
            continue
        src, size, hits, name = line.split('\t')
        size = int(size, 16)
        if hits == '-' or ',' in hits:
            continue
        a = int(hits, 16)
        if a >= LIB_START:
            spans.append((a, a + size, klass(name), name, 'reken'))
    for cls, lib in LIBS:
        out = subprocess.run([PY, os.path.join(TOOLS, 'wc', 'libmatch.py'), '-l', 'rrr', lib],
                             capture_output=True, text=True).stdout
        for line in out.splitlines():
            m = re.match(r'\s+0x([0-9a-f]+)\s+(\d+)\s+(\S+)$', line)
            if m:
                a, n = int(m.group(1), 16), int(m.group(2))
                if a >= LIB_START:
                    spans.append((a, a + n, klass(m.group(3), cls), m.group(3), 'libmatch'))
    calls = check_smacker()
    for s, e in SMACKER:
        spans.append((s, e, 'smacker', 'Smacker player (called from smacker.c %d times)' % calls, 'smacker'))
    # one owner per byte: libmatch first, then reken, then smacker
    owner = {}
    order = {'libmatch': 0, 'reken': 1, 'smacker': 2}
    for s, e, c, n, src in sorted(spans, key=lambda t: order[t[4]]):
        for a in range(s, e):
            owner.setdefault(a, (c, n, src))
    # unmatched bytes in a function whose strings name a Davilex source file
    funcs = []
    for line in open(os.path.join(ROOT, 'reverse-engineering', 'analysis', 'rrr_funcs.tsv')):
        f = line.rstrip('\n').split('\t')
        funcs.append((int(f[0], 16), f[2]))
    funcs.sort()
    for i, (a, src) in enumerate(funcs):
        if src in DAVILEX_SRC and a >= LIB_START:
            e = funcs[i + 1][0] if i + 1 < len(funcs) else CODE_END
            for x in range(a, e):
                if x in owner:
                    break
                owner[x] = ('davilex-diff', src, 'strings')
    rows, a = [], LIB_START
    while a < CODE_END:
        o = owner.get(a, ('unknown', '', '-'))
        b = a
        while b < CODE_END and owner.get(b, ('unknown', '', '-')) == o:
            b += 1
        rows.append((a, b) + o)
        a = b
    tot = {}
    for s, e, c, n, v in rows:
        tot[c] = tot.get(c, 0) + e - s
    if '--tsv' in sys.argv:
        path = sys.argv[sys.argv.index('--tsv') + 1]
        with open(path, 'w') as f:
            f.write('# start\tend\tbytes\towner\tevidence\tname   (generated by tools/libmap.py; do not edit)\n')
            for s, e, c, n, v in rows:
                f.write('%#07x\t%#07x\t%d\t%s\t%s\t%s\n' % (s, e, e - s, c, v, n))
    if '--gaps' in sys.argv:
        for s, e, c, n, v in rows:
            if c == 'unknown':
                print('%#07x-%#07x %6d' % (s, e, e - s))
    for c in sorted(tot, key=lambda k: -tot[k]):
        print('%-12s %7d bytes' % (c, tot[c]))
    print('%-12s %7d bytes' % ('total', sum(tot.values())))


if __name__ == '__main__':
    main()
