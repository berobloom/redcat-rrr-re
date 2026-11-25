#!/usr/bin/env python3
"""libplace.py: the library modules that can be linked as themselves in place of their blob range.

usage: libplace.py OUTDIR LIB ...      prints `--obj OUTDIR/<lib>_<module>.OBJ@<addr> ... --lib LIB ...`

For every LIB, tools/wc/libmatch.py -l says where each of its modules sits in RRR's code. A module
is taken when it is found exactly once, in the library range (>= 0x1B916: chance hits of tiny
modules inside the game's C are not), and all its data segments are empty (a module with data
needs its segment addresses: tools/link/dataplace.py). Each module taken is copied out of
its LIB (libobj.py) into OUTDIR. The reason for every module left out goes to stderr.
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
from libobj import info, raw_modules  # noqa: E402

PY = os.path.join(os.path.dirname(ROOT), '.work', 'venv', 'bin', 'python')
LIB_START = 0x1B916


def main():
    out, libs = sys.argv[1], sys.argv[2:]
    os.makedirs(out, exist_ok=True)
    args, taken = [], set()
    for lib in libs:
        res = subprocess.run([PY, os.path.join(ROOT, 'tools', 'wc', 'libmatch.py'), '-l', 'rrr', lib],
                             capture_output=True, text=True).stdout
        hits = {}
        for line in res.splitlines():
            m = re.match(r'\s+0x([0-9a-f]+)\s+(\d+)\s+(\S+)$', line)
            if m:
                hits.setdefault(m.group(3), []).append(int(m.group(1), 16))
        raw = {n.lower(): r for n, r in raw_modules(open(lib, 'rb').read()) if n}
        tag = os.path.splitext(os.path.basename(lib))[0].lower()
        for mod, addrs in sorted(hits.items(), key=lambda t: t[1][0]):
            if len(addrs) != 1 or addrs[0] < LIB_START:
                print('skip %s/%s: at %s' % (tag, mod, ' '.join('%#x' % a for a in addrs)), file=sys.stderr)
                continue
            if addrs[0] in taken:
                print('skip %s/%s: %#x already taken' % (tag, mod, addrs[0]), file=sys.stderr)
                continue
            fn = os.path.join(out, '%s_%s.OBJ' % (tag, re.sub(r'[^A-Za-z0-9]', '_', mod)))
            open(fn, 'wb').write(raw[mod.lower()])
            segs = info(fn)[0]
            full = [(s, n) for s, c, n in segs if c.upper() != 'CODE' and n]
            if full:
                print('skip %s/%s: data %s' % (tag, mod, full), file=sys.stderr)
                continue
            taken.add(addrs[0])
            args.append('--obj %s@%x' % (fn, addrs[0]))
    for lib in libs:
        args.append('--lib %s' % lib)
    print(' '.join(args))


if __name__ == '__main__':
    main()
