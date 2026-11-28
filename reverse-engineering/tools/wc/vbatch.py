#!/usr/bin/env python3
"""vbatch.py: score a batch of edits to one function, compiled in its own module, fixups included.

usage: vbatch.py NAME 0xADDR VARIANTS.py [--base FILE.c]

VARIANTS.py defines `VARIANTS = [('label', [('old text', 'new text'), ...]), ...]`. Each edit's old
text must occur exactly once in the module (src/<module>.c that defines NAME, or --base, a full
copy of that module to start from). For the control (no edits) and each variant the module is
compiled with the game's options into .work/vbatch/ and scored three ways:
  rows   fndiff's instruction rows (fixup bytes masked)
  match  objmatch.py's verdict for NAME (MATCH = bytes identical, fixup bytes not compared)
  wrong  objmatch.py's WRONG lines for NAME: fixups whose target is another field or symbol than
         the game's. A MATCH with wrong > 0 is NOT a match - the statements that build those
         stores are in a different order (or name different fields) than the original.
src/ is never changed. One line per variant, then the best (wrong first, then rows).
"""
import argparse
import glob
import os
import re
import runpy
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
PY = os.path.join(ROOT, '.work', 'venv', 'bin', 'python')
WC = os.path.join(ROOT, 'reverse-engineering', 'tools', 'wc')


def module_of(name):
    pat = re.compile(r'^[^\n;]*\b%s\([^;{]*\)\s*\{' % re.escape(name), re.M)
    for mod in sorted(glob.glob(os.path.join(ROOT, 'src', '*.c'))):
        if pat.search(open(mod).read()):
            return mod
    sys.exit('no definition of %s in src/*.c' % name)


def score(w, mod_upper, text, name, addr):
    c = os.path.join(w, mod_upper + '.C')
    obj = os.path.join(w, mod_upper + '.OBJ')
    open(c, 'w').write(text)
    if os.path.exists(obj):
        os.remove(obj)
    subprocess.run([os.path.join(WC, 'wcc.sh'), w, mod_upper + '.C', '-bt=dos', '-3r', '-s', '-ox',
                    '-fo=%s.OBJ' % mod_upper], capture_output=True, text=True)
    if not os.path.exists(obj):
        return None, None, None, 'compile error: ' + open(os.path.join(w, 'WCC.OUT')).read()[-200:].replace('\n', ' ')
    fd = subprocess.run([PY, os.path.join(WC, 'fndiff.py'), 'rrr', obj, name, addr],
                        capture_output=True, text=True).stdout.strip().split('\n')[-1]
    m = re.search(r'(\d+) instruction diffs', fd)
    rows = int(m.group(1)) if m else None
    om = subprocess.run([PY, os.path.join(WC, 'objmatch.py'), 'rrr', obj], capture_output=True, text=True).stdout
    verdict = next((l for l in om.split('\n') if l.startswith(name + '_ ') or l.startswith(name + ' ')), '')
    wrong = sum(1 for l in om.split('\n') if 'WRONG %s_@' % name in l or 'WRONG %s@' % name in l)
    return rows, 'MATCH' in verdict, wrong, fd


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('name')
    ap.add_argument('addr')
    ap.add_argument('variants')
    ap.add_argument('--base')
    a = ap.parse_args()
    mod = module_of(a.name)
    base = open(a.base).read() if a.base else open(mod).read()
    mod_upper = os.path.basename(mod)[:-2].upper()
    variants = runpy.run_path(a.variants)['VARIANTS']
    w = os.path.join(ROOT, '.work', 'vbatch')
    os.makedirs(w, exist_ok=True)
    shutil.copy(os.path.join(ROOT, 'src', 'rrr.h'), os.path.join(w, 'RRR.H'))
    print('%s in %s%s, %d variants' % (a.name, os.path.relpath(mod, ROOT), ' (base %s)' % a.base if a.base else '', len(variants)))
    res = []
    for label, edits in [('control', [])] + list(variants):
        text, bad = base, None
        for old, new in edits:
            n = text.count(old)
            if n != 1:
                bad = 'pattern found %d times, not once: %r' % (n, old[:60])
                break
            text = text.replace(old, new)
        if bad:
            print('%-40s %s' % (label[:40], bad), flush=True)
            continue
        rows, match, wrong, line = score(w, mod_upper, text, a.name, a.addr)
        if rows is None and match is None:
            print('%-40s %s' % (label[:40], line), flush=True)
            continue
        print('%-40s rows=%-4s %-5s wrong=%s' % (label[:40], rows, 'MATCH' if match else '-', wrong), flush=True)
        if label != 'control':
            res.append(((0 if match and wrong == 0 else 1), wrong, rows if rows is not None else 9999, label))
    if res:
        res.sort()
        b = res[0]
        print('\nbest: %s (rows=%s wrong=%d)%s' % (b[3], b[2], b[1], '  <== MATCH, no WRONG fixup' if b[0] == 0 else ''))


if __name__ == '__main__':
    main()
