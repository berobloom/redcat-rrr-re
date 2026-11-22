#!/usr/bin/env python3
"""inplace.py: score spellings of one function compiled IN PLACE in src/rrr.c (probe copies can score
differently: shared tails, other scheduling; GOTCHAS "Probes").

usage: inplace.py NAME 0xADDR FILE.c [FILE.c ...]

Each FILE.c holds one full definition of NAME. For each, src/rrr.c is copied with NAME's definition
(from its signature line to the next `/* ---- 0x` marker) replaced by FILE.c, compiled with the game's
options into .work/inplace/, and fndiff's row count is printed. src/rrr.c is not changed.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))


def main():
    name, addr, files = sys.argv[1], sys.argv[2], sys.argv[3:]
    src = open(os.path.join(ROOT, 'src', 'rrr.c')).read()
    m = re.search(r'^[^\n;]*\b%s\([^;{]*\)\s*\{' % re.escape(name), src, re.M)
    if not m:
        sys.exit('no definition of %s in src/rrr.c' % name)
    start = src.rfind('\n', 0, m.start()) + 1
    # include a /* differs: ... */ note directly above
    end = src.index('/* ---- 0x', m.end())
    w = os.path.join(ROOT, '.work', 'inplace')
    os.makedirs(w, exist_ok=True)
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
