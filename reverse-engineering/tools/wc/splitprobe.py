#!/usr/bin/env python3
"""splitprobe.py: does a basic-block split decide this near miss? Pads one statement position of a
function at a time with dead stores and scores each compile in place (like inplace.py).

usage: splitprobe.py NAME 0xADDR [--n 6,14,20] [--var NAME] [--only TEXT]

Why: Watcom's code generator ends a basic block at the next statement start once 75 instruction
ids (INS_PER_BLOCK) have been generated since the block began (BlkTooBig, WCC386 10.6 0xa11ee5 in
the loaded image; Open Watcom bld/cg makeblk.c). Ids are counted when the front end GENERATES an
instruction, so copies, folded address arithmetic and dead stores count even though the optimiser
deletes them later. The scheduler cannot move an instruction across that boundary, and register
allocation sees the two halves apart, so where the split falls can decide a near miss that no
listing shows. CONFIRMED on init_objects 0x148db (research/open-questions.md 35, 2026-11-28).

For every statement start in NAME's body (not declarations, not continuation lines) and every
count in --n, the variant puts `v = 0; v = 1; ...` (n dead stores) in front of that statement and
prints fndiff's row count. `v` is --var (a local the function already has and that is dead at
that point), or a new `int sp_probe_;` declared first in the body. Any row count below the control
row means the miss depends on where a block split falls: find the NATURAL source that generates
those extra (or fewer) ids - typically an expression the compiler folds away, such as
`stats[bodies[2].stats].fuel` right after `bodies[2].stats = 1;` (constant propagation turns it
into `stats[1].fuel`, same bytes, more ids). The dead stores themselves are a probe, never the fix.

src/ is not changed: each compile is .work/splitprobe/IP.C.
"""
import argparse
import glob
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
DECL = re.compile(r'^\s*(register\s+|static\s+|const\s+|volatile\s+)*'
                  r'(unsigned|signed|int|short|long|char|struct\s+\w+|void|float|double)\b[^=(]*'
                  r'(\[[^\]]*\])?\s*(=[^;]*)?;')


def find_function(name):
    pat = re.compile(r'^[^\n;]*\b%s\([^;{]*\)\s*\{' % re.escape(name), re.M)
    for mod in sorted(glob.glob(os.path.join(ROOT, 'src', '*.c'))):
        src = open(mod).read()
        m = pat.search(src)
        if m:
            start = src.rfind('\n', 0, m.start()) + 1
            end = src.find('/* ---- 0x', m.end())
            return mod, src, start, len(src) if end < 0 else end
    sys.exit('no definition of %s in src/*.c' % name)


def statement_starts(lines):
    """Indexes of lines that begin a statement inside the function body (line 0 holds the `{`)."""
    out, prev, in_comment = [], '{', False
    for k, line in enumerate(lines):
        s = line.strip()
        if k == 0 or not s:
            continue
        if in_comment:
            in_comment = '*/' not in s
            continue
        if s.startswith('/*'):
            in_comment = '*/' not in s
            continue
        if s.startswith('//'):
            continue
        cont = not (prev.endswith(';') or prev.endswith('{') or prev.endswith('}') or prev.endswith(':')
                    or prev.endswith('*/'))
        prev = s
        if cont or s[0] in '{}' or s.startswith(('case ', 'default', 'else', '#')) or DECL.match(line):
            continue
        out.append(k)
    return out


def score(w, text, name, addr):
    open(os.path.join(w, 'IP.C'), 'w').write(text)
    obj = os.path.join(w, 'IP.OBJ')
    if os.path.exists(obj):
        os.remove(obj)
    subprocess.run([os.path.join(ROOT, 'reverse-engineering', 'tools', 'wc', 'wcc.sh'), w, 'IP.C',
                    '-bt=dos', '-3r', '-s', '-ox', '-fo=IP.OBJ'], capture_output=True, text=True)
    if not os.path.exists(obj):
        return None, 'compile error: ' + open(os.path.join(w, 'WCC.OUT')).read()[-200:].replace('\n', ' ')
    r = subprocess.run([os.path.join(ROOT, '.work', 'venv', 'bin', 'python'),
                        os.path.join(ROOT, 'reverse-engineering', 'tools', 'wc', 'fndiff.py'),
                        'rrr', obj, name, addr], capture_output=True, text=True).stdout.strip().split('\n')[-1]
    m = re.search(r'(\d+) instruction diffs', r)
    return (int(m.group(1)) if m else None), r


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('name')
    ap.add_argument('addr')
    ap.add_argument('--n', default='6,14,20')
    ap.add_argument('--var')
    ap.add_argument('--only', help='probe only statements whose text contains this')
    a = ap.parse_args()
    ns = [int(x) for x in a.n.split(',')]
    mod, src, start, end = find_function(a.name)
    fn = src[start:end]
    brace = fn.index('{')
    head, body = fn[:brace], fn[brace:]
    lines = body.split('\n')
    var = a.var or 'sp_probe_'
    if not a.var:
        lines[0] = lines[0] + '\n    int sp_probe_;'
        lines = '\n'.join(lines).split('\n')
    starts = statement_starts(lines)
    if a.only:
        starts = [k for k in starts if a.only in lines[k]]
    w = os.path.join(ROOT, '.work', 'splitprobe')
    os.makedirs(w, exist_ok=True)
    shutil.copy(os.path.join(ROOT, 'src', 'rrr.h'), os.path.join(w, 'RRR.H'))
    ctl, ctl_line = score(w, src[:start] + head + '\n'.join(lines) + src[end:], a.name, a.addr)
    print('%s in %s, %d statement starts, n = %s' % (a.name, os.path.relpath(mod, ROOT), len(starts), a.n))
    print('control (probe variable declared, no stores): %s' % ctl_line)
    if ctl is None:
        sys.exit(1)
    best = []
    for k in starts:
        ind = re.match(r'\s*', lines[k]).group(0)
        for n in ns:
            pad = ind + ' '.join('%s = %d;' % (var, j) for j in range(n))
            var_lines = lines[:k] + [pad] + lines[k:]
            rows, line = score(w, src[:start] + head + '\n'.join(var_lines) + src[end:], a.name, a.addr)
            mark = '  <== below control' if rows is not None and rows < ctl else ''
            print('%3s rows  n=%-2d before line %-4d %s%s' % (rows, n, k, lines[k].strip()[:60], mark), flush=True)
            if rows is not None and rows < ctl:
                best.append((rows, n, k, lines[k].strip()))
    print()
    if best:
        best.sort()
        print('BELOW CONTROL (%d): a block split decides this function. Best: %d rows, n=%d before `%s`'
              % (len(best), best[0][0], best[0][1], best[0][3][:70]))
    else:
        print('no variant below control (%d rows): no block split in reach of these counts' % ctl)


if __name__ == '__main__':
    main()
