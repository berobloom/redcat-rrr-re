#!/usr/bin/env python3
"""psweep.py: score declaration orders of one function in parallel, in place in its src/ module
(inplace.py's scoring, 8 compiles at a time in .work/psweep/<pid>/; about 0.11 s an order).

usage: psweep.py NAME 0xADDR TEMPLATE.c N|all SEED < DECLS.txt > out.txt

TEMPLATE.c is the function's full definition with the line `@DECLS@` where its declarations go;
DECLS.txt holds one declaration per line. Every permutation is tried (`all`), or N of them in an
order shuffled by SEED. Each output line is `ROWS decl | decl | ...`; `sort -n out.txt | head`."""
import itertools, multiprocessing as mp, os, random, re, shutil, subprocess, sys, glob

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))  # repo root
name, addr, tpl, n, seed = sys.argv[1:6]
decls = [l.rstrip('\n') for l in sys.stdin if l.strip()]
body = open(tpl).read()
pat = re.compile(r'^[^\n;]*\b%s\([^;{]*\)\s*\{' % re.escape(name), re.M)
for mod in sorted(glob.glob(os.path.join(ROOT, 'src', '*.c'))):
    src = open(mod).read()
    m = pat.search(src)
    if m:
        break
start = src.rfind('\n', 0, m.start()) + 1
end = src.find('/* ---- 0x', m.end())
end = len(src) if end < 0 else end


def score(order):
    w = os.path.join(ROOT, '.work', 'psweep', str(os.getpid()))
    if not os.path.isdir(w):
        os.makedirs(w)
        shutil.copy(os.path.join(ROOT, 'src', 'rrr.h'), os.path.join(w, 'RRR.H'))
    text = body.replace('@DECLS@', '\n'.join(order))
    open(os.path.join(w, 'IP.C'), 'w').write(src[:start] + text + '\n' + src[end:])
    obj = os.path.join(w, 'IP.OBJ')
    if os.path.exists(obj):
        os.remove(obj)
    subprocess.run([os.path.join(ROOT, 'reverse-engineering/tools/wc/wcc.sh'), w, 'IP.C', '-bt=dos', '-3r',
                    '-s', '-ox', '-fo=IP.OBJ'], capture_output=True)
    if not os.path.exists(obj):
        return (99999, order)
    r = subprocess.run([os.path.join(ROOT, '.work/venv/bin/python'),
                        os.path.join(ROOT, 'reverse-engineering/tools/wc/fndiff.py'), 'rrr', obj, name, addr],
                       capture_output=True, text=True).stdout.strip().split('\n')[-1]
    return (int(r.split(',')[1].split()[0]), order)


if __name__ == '__main__':
    perms = list(itertools.permutations(decls))
    if n != 'all':
        random.Random(int(seed)).shuffle(perms)
        perms = perms[:int(n)]
    best = 99999
    with mp.Pool(8) as p:
        for i, (s, o) in enumerate(p.imap_unordered(score, perms, chunksize=4)):
            print(s, ' | '.join(x.strip() for x in o), flush=True)
            if s < best:
                best = s
                print('BEST', s, file=sys.stderr, flush=True)
