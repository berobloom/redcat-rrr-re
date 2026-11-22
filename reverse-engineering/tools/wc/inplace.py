#!/usr/bin/env python3
"""inplace.py: score spellings of one function compiled IN PLACE in its module of src/ (probe copies
can score differently: shared tails, other scheduling; GOTCHAS "Probes").

usage: inplace.py NAME 0xADDR FILE.c [FILE.c ...]

Each FILE.c holds one full definition of NAME. For each, the src/*.c module that defines NAME is
copied with that definition (from its signature line to the next `/* ---- 0x` marker, or the end of
the file) replaced by FILE.c, compiled with src/rrr.h and the game's options into .work/inplace/
(IP.C, IP.OBJ), and fndiff's row count is printed. src/ is not changed.
"""
import os
import glob
import re
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))


def main():
    name, addr, files = sys.argv[1], sys.argv[2], sys.argv[3:]
    pat = re.compile(r'^[^\n;]*\b%s\([^;{]*\)\s*\{' % re.escape(name), re.M)
    for mod in sorted(glob.glob(os.path.join(ROOT, 'src', '*.c'))):
        src = open(mod).read()
        m = pat.search(src)
        if m:
            break
    else:
        sys.exit('no definition of %s in src/*.c' % name)
    start = src.rfind('\n', 0, m.start()) + 1
    end = src.find('/* ---- 0x', m.end())
    if end < 0:
        end = len(src)
    w = os.path.join(ROOT, '.work', 'inplace')
    os.makedirs(w, exist_ok=True)
    shutil.copy(os.path.join(ROOT, 'src', 'rrr.h'), os.path.join(w, 'RRR.H'))
    for f in files:
        open(os.path.join(w, 'IP.C'), 'w').write(src[:start] + open(f).read() + '\n' + src[end:])
        obj = os.path.join(w, 'IP.OBJ')
        if os.path.exists(obj):
            os.remove(obj)
        subprocess.run([os.path.join(ROOT, 'reverse-engineering', 'tools', 'wc', 'wcc.sh'), w, 'IP.C',
                        '-bt=dos', '-3r', '-s', '-ox', '-fo=IP.OBJ'], capture_output=True, text=True)
        if not os.path.exists(obj):
            print('%-30s compile error: %s' % (f, open(os.path.join(w, 'WCC.OUT')).read()[-300:]))
            continue
        r = subprocess.run([os.path.join(ROOT, '.work', 'venv', 'bin', 'python'),
                            os.path.join(ROOT, 'reverse-engineering', 'tools', 'wc', 'fndiff.py'),
                            'rrr', obj, name, addr], capture_output=True, text=True).stdout.strip().split('\n')[-1]
        print('%-30s %s' % (os.path.basename(f), r))


if __name__ == '__main__':
    main()
