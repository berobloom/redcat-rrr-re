#!/usr/bin/env python3
"""progress.py: writes PROGRESS.md, how much of RRR.EXE is rebuilt from C.

usage: reverse-engineering/tools/progress.py [--out FILE]

Builds src/ with src/check.sh and compares it the same way: a function counts as done only when
our compiled bytes equal the game's (fixup bytes masked, fixup targets checked). Near misses get
their remaining instruction rows from tools/wc/fndiff.py. Functions of the game's own C that src/rrr.c
does not have yet come from tools/funcs.py's list (reverse-engineering/analysis/rrr_funcs.tsv) and the
names file, named from reverse-engineering/analysis/rrr_names.txt. Subsystems come from
reverse-engineering/analysis/rrr_subsystems.txt. The library rows come from tools/libmap.py, which
it reruns to regenerate reverse-engineering/analysis/rrr_libmap.tsv (Watcom 10.6's libraries via
tools/wc/libmatch.py, REKEN's debug-info modules via tools/modmatch.py, the Smacker player).

It also writes docs/RRRPROGRESS.HTML (every function, searchable and filterable, with its instruction
diff against the game; the layout of wacky-re's report, template tools/progress.html) and
docs/RRRPROGRESS.SVG (the progress badge the README shows).
"""
import base64
import difflib
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOOLS = os.path.join(ROOT, 'reverse-engineering', 'tools')
ANALYSIS = os.path.join(ROOT, 'reverse-engineering', 'analysis')
sys.path.insert(0, os.path.join(TOOLS, 'wc'))
sys.path.insert(0, TOOLS)
from lelib import Image      # noqa: E402
from entries import sweep    # noqa: E402
PY = os.path.join(ROOT, '.work', 'venv', 'bin', 'python')
WLIB = os.path.join(ROOT, '.work', 'watcom', 'dos', 'C', 'WATCOM', 'LIB386')

# RRR.EXE's code object (MATCHING.md, "The target"): BEGTEXT, the game's own modules, then the
# library modules in the order WLINK pulled them in
C_START, LIB_START, CODE_END = 0x10010, 0x1B916, 0x10000 + 0x31430


def run(args, **kw):
    return subprocess.run(args, cwd=ROOT, capture_output=True, text=True, **kw).stdout


def names_file():
    out = {}
    for line in open(os.path.join(ANALYSIS, 'rrr_names.txt')):
        f = line.split('#')[0].split()
        if len(f) >= 2 and f[0].startswith('0x'):
            out.setdefault(int(f[0], 16), f[1])
    return out


def nearmiss_notes():
    """Game address -> the hand-kept status text of its src/nearmiss.txt line (MATCH/SKIP/LAYOUT/ASM)."""
    out, cur = {}, None
    for line in open(os.path.join(ROOT, 'src', 'nearmiss.txt')):
        if line[:1].isspace():                                   # wrapped status text
            if cur is not None and line.strip():
                out[cur] += ' ' + line.strip()
            continue
        f = line.split()
        at = next((i for i in range(min(5, len(f))) if re.fullmatch(r'0x[0-9a-f]+', f[i])), None)
        if at is not None and len(f) > at + 2:
            cur = int(f[at], 16)
            out[cur] = ' '.join(f[at + 2:])
        else:
            cur = None
    return out


def subsystems():
    order, of = [], {}
    cur = None
    for line in open(os.path.join(ANALYSIS, 'rrr_subsystems.txt')):
        line = line.strip()
        if line.startswith('## '):
            cur = line[3:].strip()
            order.append(cur)
            continue
        line = line.split('#')[0].strip()
        if line and cur:
            for n in line.split():
                of[n] = cur
    return order, of


def align_all(near):
    """For each near miss {addr: name}: our instructions and the game's aligned as fndiff.py does,
    the count of differing rows, the similarity ratio and diff hunks with 3 rows of context."""
    import fndiff as F
    import libmatch as L
    import objmatch as M
    meta = json.load(open(os.path.join(ROOT, '.work', 'le', 'rrr.json')))
    code = open(os.path.join(ROOT, '.work', 'le', 'rrr.obj1.bin'), 'rb').read()
    cbase = meta['objects'][0]['base']
    import glob
    objs = []
    for path in sorted(glob.glob(os.path.join(ROOT, '.work', 'src', '*.OBJ'))):
        (_, recs), = list(L.modules(open(path, 'rb').read()))
        segs = L.code_of(recs)
        objs.append((segs, sorted(p for p in M.publics(recs) if p[0] in segs)))
    out = {}
    for addr, name in near.items():
        hit = [(segs, pubs, p) for segs, pubs in objs for p in pubs if p[2] == name or M.plain(p[2]) == name]
        if not hit:
            continue
        segs, pubs, (seg, off, _) = hit[0]
        end = next((q[1] for q in pubs if q[0] == seg and q[1] > off), len(segs[seg][0]))
        buf, mask = segs[seg]
        ours = bytes(b if not mask[off + k] else 0x90 for k, b in enumerate(buf[off:end]))
        A = F.dis(ours, addr, addr + len(ours))
        B = F.dis(code[addr - cbase:addr - cbase + len(ours)], addr, addr + len(ours))
        ops = F.align([t[2] for t in B], [t[2] for t in A])
        d = F.rows(ops)
        eq = sum(i2 - i1 for t, i1, i2, j1, j2 in ops if t == 'equal')
        sm = difflib.SequenceMatcher(None, [], [])
        sm.get_opcodes = lambda: ops
        hunks = []
        for grp in sm.get_grouped_opcodes(3):
            h = []
            for t, i1, i2, j1, j2 in grp:
                if t == 'equal':
                    h += [[' ', B[i][0], A[j][0], B[i][2]] for i, j in zip(range(i1, i2), range(j1, j2))]
                else:
                    h += [['-', B[i][0], 0, B[i][2]] for i in range(i1, i2)]
                    h += [['+', 0, A[j][0], A[j][2]] for j in range(j1, j2)]
            hunks.append(h)
        out[addr] = {'d': d, 'i': eq + d, 'r': round(2.0 * eq / (len(A) + len(B)), 4), 'h': hunks}
    return out


def logo_png():
    """RedCat on his rocket kart, the menu's Start! button (docs/media/menu.jpg), as PNG."""
    import io
    from PIL import Image
    buf = io.BytesIO()
    Image.open(os.path.join(ROOT, 'docs', 'media', 'menu.jpg')).convert('RGB').crop((85, 205, 285, 375)).save(buf, 'PNG')
    return base64.b64encode(buf.getvalue()).decode()


def write_html(path, allf, sub_of, order, al, head, notes):
    fs = []
    for a, n, s in allf:
        f = {'a': a, 'n': n, 's': s, 'sub': sub_of.get(n, 'unsorted')}
        if s == 'asm':
            f['t'] = notes.get(a, '')
        if s == 'near' and a in al:
            f.update(al[a])
        elif s == 'near':
            f.update({'d': 0, 'i': 0, 'r': 0.0, 'h': []})
        fs.append(f)
    subs = [g for g in order if any(f['sub'] == g for f in fs)]
    subs += ['unsorted'] if any(f['sub'] == 'unsorted' for f in fs) else []
    data = json.dumps({'commit': head, 'subsystems': subs, 'functions': fs}, separators=(',', ':'))
    t = open(os.path.join(TOOLS, 'progress.html')).read()
    t = t.replace('/*ICON*/', logo_png()).replace('/*DATA*/', data.replace('</', '<\\/'))
    open(path, 'w').write(t)


def write_svg(path, done, total):
    """The README badge: RedCat's kart, RRR.EXE, and a bar filled to the byte-identical share."""
    pct = 100.0 * done / total
    w = 290 * pct / 100
    svg = (
        '<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" '
        'width="320" height="240" viewBox="0 0 320 240">\n'
        '<rect width="320" height="240" rx="8" fill="#01162b"/>\n'
        '<image x="85" y="8" width="150" height="128" style="image-rendering:pixelated" '
        'xlink:href="data:image/png;base64,%s"/>\n'
        '<text x="160" y="176" text-anchor="middle" font-family="monospace" font-weight="bold" font-size="22" '
        'fill="#e8c030">RRR.EXE</text>\n'
        '<clipPath id="fill"><rect x="15" y="196" width="%.2f" height="24"/></clipPath>\n'
        '<rect x="15" y="196" width="290" height="24" fill="#000" stroke="#eef2f8" stroke-width="1.5"/>\n'
        '<text x="160" y="213" text-anchor="middle" font-family="monospace" font-weight="bold" font-size="14" '
        'fill="#eef2f8">%.1f%%</text>\n'
        '<g clip-path="url(#fill)"><rect x="15" y="196" width="290" height="24" fill="#e8c030"/>\n'
        '<text x="160" y="213" text-anchor="middle" font-family="monospace" font-weight="bold" font-size="14" '
        'fill="#01162b">%.1f%%</text></g>\n'
        '</svg>\n') % (logo_png(), w, pct, pct)
    open(path, 'w').write(svg)



def main():
    out = os.path.join(ROOT, 'PROGRESS.md')
    if '--out' in sys.argv:
        out = sys.argv[sys.argv.index('--out') + 1]

    chk = run(['src/check.sh'])
    funcs = {}                                   # game address -> (name, size, matched)
    for name, size, kind, addr in re.findall(
            r'^(\S+)\s+(\d+) bytes  (MATCH at|differs from) (0x[0-9a-f]+)', chk, re.M):
        funcs[int(addr, 16)] = (name.rstrip('_'), int(size), kind == 'MATCH at')
    if not funcs:
        sys.exit('src/check.sh printed no comparisons:\n' + chk[-2000:])

    # the game's C functions src/rrr.c does not have: funcs.py's starts and the names file's
    # functions in the C range that fall inside none of ours
    named = names_file()
    calls_in = {}
    for line in open(os.path.join(ANALYSIS, 'rrr_funcs.tsv')):
        if line.startswith('0x'):
            f = line.split('\t')
            calls_in[int(f[0], 16)] = int(f[1])
    found = set(calls_in) | set(named)
    spans = sorted((a, a + s) for a, (_, s, _) in funcs.items())
    # funcs.py also seeds from a byte scan, so some of its starts are not entries at
    # all: the body before them runs straight through the address. Drop those, see
    # tools/entries.py. A start is an entry when something calls it, when the names
    # file has it, or when the code before it ends before it starts.
    img = Image('rrr')
    entries = {a for a in found if a in named or calls_in.get(a)} | set(funcs)
    missing = {}
    for a in sorted(found):
        if not C_START <= a < LIB_START or any(s <= a < e for s, e in spans):
            continue
        below = [e for e in entries if e < a]
        above = [e for e in found | set(funcs) if e > a]
        if below and above:
            ins, _blocks = sweep(img, max(below), min(above))
            if any(x < a < x + ins[x].size for x in ins):
                continue
        missing[a] = named.get(a, 'fn_%x' % a)

    notes = nearmiss_notes()          # an unwritten function can already carry a status (ASM:)
    al = align_all({a: n for a, (n, _, ok) in funcs.items() if not ok})
    rows = {a: v['d'] for a, v in al.items()}

    libmap = os.path.join(ANALYSIS, 'rrr_libmap.tsv')
    run([PY, os.path.join(TOOLS, 'libmap.py'), '--tsv', libmap], timeout=1200)
    owners, watcom_bytes = {}, 0
    for line in open(libmap):
        if line.startswith('0x'):
            f = line.rstrip('\n').split('\t')
            owners[f[3]] = owners.get(f[3], 0) + int(f[2])
            if f[4] == 'libmatch':
                watcom_bytes += int(f[2])

    order, sub_of = subsystems()
    allf = [(a, n, 'match' if ok else 'near') for a, (n, _, ok) in funcs.items()]
    allf += [(a, n, 'asm' if notes.get(a, '').startswith('ASM') else 'none')
             for a, n in missing.items()]
    allf.sort()
    total = len(allf)
    done = sum(1 for _, _, s in allf if s == 'match')
    near = sum(1 for _, _, s in allf if s == 'near')
    asm = sum(1 for _, _, s in allf if s == 'asm')
    unwritten = total - done - near - asm
    head = run(['git', 'rev-parse', '--short', 'HEAD']).strip()

    def pct(a, b):
        return '%.1f%%' % (100.0 * a / b) if b else '-'

    def num(x):
        return '{:,}'.format(x)

    c_bytes = sum(sz for _, (_, sz, ok) in funcs.items() if ok)
    whole = CODE_END - C_START
    overall = c_bytes + watcom_bytes

    L = []
    w = L.append
    w('# Progress')
    w('')
    w('How much of `RRR.EXE` (the race) is rebuilt from C. Generated by `reverse-engineering/tools/progress.py`')
    w('(do not edit by hand), measured at commit `%s`.' % head)
    w('')
    w('**%d of the %d functions known in the game\'s own C compile to the original\'s exact bytes (%s).**'
      % (done, total, pct(done, total)))
    w('')
    w('Of RRR.EXE\'s code object after `BEGTEXT` (%s bytes), %s bytes (%s) are rebuilt byte for byte or '
      'identified as byte-identical Watcom library code.' % (num(whole), num(overall), pct(overall, whole)))
    w('')
    w('A function counts only when our build of `src/` (Watcom C/C++ 10.5, `-3r -s -ox`) gives')
    w('the same bytes as the game at the same address, with every address it refers to checked as')
    w('well. A near miss compiles but its code still differs: the number is how many instructions')
    w('differ (`reverse-engineering/tools/wc/fndiff.py`). The list of functions not written yet comes')
    w('from a recursive-descent scan that finds about 60% of the code, so the total will grow.')
    w('')
    w('To search and filter the functions and see each near miss\'s diff, open')
    w('[`docs/RRRPROGRESS.HTML`](docs/RRRPROGRESS.HTML) in a browser.')
    w('')
    w('## RRR.EXE part by part')
    w('')
    w('| Part | Code | Status |')
    w('|---|---|---|')
    w('| The game\'s own C (sndman.c, main.c, new2io.c, order.c, shade.c, init.c, raceanim.c, heap.c, map.c, ...), `src/rrr.c` '
      '| 0x%05X-0x%05X, %s bytes | %d of %d functions byte-identical (%s), %d near misses, '
      '%d not written yet, %d assembly |'
      % (C_START, LIB_START - 1, num(LIB_START - C_START), done, total, pct(done, total), near,
         unwritten, asm))
    LIBROWS = [
        ('clib', 'Watcom C library (`CLIB3R.LIB` 10.6)', 'byte for byte against the LIB (a few small modules only via REKEN\'s copy); linked from the library, nothing to rewrite'),
        ('startup', 'Watcom start-up (cstrt386, cmain386, ...)', 'byte for byte against the LIB except cstrt386\'s _TEXT (725 B), which only matches REKEN\'s copy'),
        ('math', 'Watcom math library (`MATH387R.LIB`, `MATH3R.LIB` 10.6)', 'byte for byte'),
        ('emu', 'Watcom 387 emulator (emu387)', 'the same bytes as REKEN\'s emu387 module; 10.6\'s `EMU387.LIB` does not match it, so not counted above'),
        ('soundlib', 'Davilex SOUNDLIB (`C:\\PROTMOD\\SOUNDLIB\\wc32\\mod`: dma6.c, sndbuf.c, audio.c, read.c, ...)',
         'byte for byte the same as REKEN\'s copy, named by REKEN\'s debug info; C to rebuild'),
        ('winlib', 'Davilex WINLIB (`C:\\PROTMOD\\WINLIB\\mgl2\\fg2`: rctools.c, rcascii.c, mouse.c, ...)',
         'byte for byte the same as REKEN\'s copy, named by REKEN\'s debug info; C to rebuild'),
        ('davilex-diff', 'Davilex WINLIB/SOUNDLIB functions that differ from REKEN\'s copy',
         'owned by their source-file strings, not byte-matched; C to rebuild'),
        ('davilex-asm', 'Davilex assembly (doscalls.asm, cpy.asm)', 'byte for byte the same as REKEN\'s copy'),
        ('fastgraph', 'Fastgraph V4.03 (the upper-case *.ASM modules)', 'byte for byte the same as REKEN\'s copy; commercial, wanted as its LIB'),
        ('smacker', 'Smacker player (RAD)', 'shared byte for byte with INTRO, RRRMENU and install, called only from smacker.c; commercial, wanted as its LIB'),
        ('unknown', 'Not identified yet', 'nothing matched'),
    ]
    for k, what, how in LIBROWS:
        w('| %s | inside 0x%05X-0x%05X | %s bytes, %s |' % (what, LIB_START, CODE_END - 1, num(owners.get(k, 0)), how))
    w('')
    w('## The game\'s own C by subsystem')
    w('')
    w('The original\'s modules are not known function by function, so the subsystems are groups of')
    w('functions, not files. The grouping is ours: `reverse-engineering/analysis/rrr_subsystems.txt`.')
    w('')
    w('| Subsystem | Functions | Byte-identical | Near misses | Not written | Assembly | Done |')
    w('|---|---:|---:|---:|---:|---:|---:|')
    groups = order + (['unsorted'] if any(n not in sub_of for _, n, _ in allf) else [])
    for g in groups:
        fs = [s for _, n, s in allf if sub_of.get(n, 'unsorted') == g]
        if not fs:
            continue
        m_, n_, x_, a_ = (fs.count('match'), fs.count('near'), fs.count('none'), fs.count('asm'))
        w('| %s | %d | %d | %d | %d | %d | %s |' % (g, len(fs), m_, n_, x_, a_, pct(m_, len(fs))))
    w('| **All** | **%d** | **%d** | **%d** | **%d** | **%d** | **%s** |'
      % (total, done, near, unwritten, asm, pct(done, total)))
    w('')
    w('## Every function')
    w('')
    w('In address order. Names without a meaning yet keep their address (`fn_10010`); the evidence for')
    w('each name is in `reverse-engineering/analysis/rrr_names.txt`.')
    w('')
    w('| Address | Function | Subsystem | Status |')
    w('|---|---|---|---|')
    for a, n, s in allf:
        st = {'match': 'byte-identical', 'none': 'not in src/ yet',
              'asm': 'assembly, not C (ASM note in src/nearmiss.txt)'}.get(s)
        if s == 'near':
            st = 'near miss, %s instructions differ' % rows[a] if rows.get(a) is not None else 'near miss'
        w('| 0x%05X | `%s` | %s | %s |' % (a, n, sub_of.get(n, 'unsorted'), st))
    w('')
    open(out, 'w').write('\n'.join(L))
    write_html(os.path.join(ROOT, 'docs', 'RRRPROGRESS.HTML'), allf, sub_of, order, al, head, notes)
    write_svg(os.path.join(ROOT, 'docs', 'RRRPROGRESS.SVG'), done, total)
    print('%s: %d of %d byte-identical (%s), %d near misses, %d not written, %d assembly'
          % (os.path.relpath(out, ROOT), done, total, pct(done, total), near, unwritten, asm))


if __name__ == '__main__':
    main()
