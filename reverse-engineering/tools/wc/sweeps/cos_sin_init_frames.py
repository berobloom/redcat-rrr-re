#!/usr/bin/env python3
"""cos_sin_init_frames.py: the sweep that matched cos_sin_init 0x13c34 (2 rows -> MATCH, 2026-11-27).

usage: .work/venv/bin/python reverse-engineering/tools/wc/sweeps/cos_sin_init_frames.py

Every spelling of one shape: the loop's two int results held in a local array element plus a scalar,
over array size x which element takes the cos value x every declaration order of i, a, y and the
array (2 x 3 + 3 x 3 + 4 x 3 = 216 compiles of the function alone). For each it prints the frame and
the slots from `stackmap.py`, and flags the ones whose frame is the game's: frame 0x20 with i at
[esp+1Ch], a at [esp+18h], y at [esp+14h] and the cos value at [esp+8] - the game's 12-byte object at
[esp+0..0xB], below the gap WCC leaves under its scalar band. Two spellings hit, `int i, y; float a;
int v[3];` (what src/rrr.c has) and the same list with y and a transposed; `long v[3]` and a local
3-int struct give the same bytes.

The frame map is only the filter - within one shape the slot SET does not depend on declaration order,
so every spelling with the right shape passes it. Each frame hit is therefore scored instruction by
instruction against the game with `variants.py` (0x13c34-0x13cba, the function's own end - one byte
past it and the neighbour's head reads as 2 phantom rows). Of the 216 spellings, 24 give the game's
frame map and 2 score 0: `int i; int y; float a; int d[3];` (what src/rrr.c has, read as v[2]) and
`int d[3]; int i; float a; int y;`. The whole table is in reverse-engineering/analysis/
cos_sin_init_frames.txt.

What made the search work: no `double t` local (the angle expression is written twice and WCC CSEs
it, which is what the game's code does), and cos read from the array element. Spellings that keep x
as a plain local can never reach [esp+8] - that is the frame, not the order (discoveries "Toolchain:
how WCC lays out a function's stack frame").
"""
import itertools, os, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)) + '/..')
import libmatch as L

ROOT = L.ROOT
W = os.path.join(ROOT, '.work/probe/framesweep')
PY = os.path.join(ROOT, '.work/venv/bin/python')
STACKMAP = os.path.join(ROOT, 'reverse-engineering/tools/wc/stackmap.py')
VARIANTS = os.path.join(ROOT, 'reverse-engineering/tools/wc/variants.py')
E = '(a + 8.0) * -0.19634954087500001'
WANT = 'frame=0x20 pushes=0  slots: 1c 18 14 8 '
CTX = ('#include <math.h>\n'
       'extern int cos_tbl[32], sin_tbl[32];\n')


def slots(src):
    open(W + '/SW.C', 'w').write(CTX + src + '\n')
    subprocess.run(['rm', '-f', W + '/SW.OBJ'])
    subprocess.run([os.path.join(ROOT, 'reverse-engineering/tools/wc/wcc.sh'), W, 'SW.C',
                    '-bt=dos', '-3r', '-s', '-ox', '-fo=SW.OBJ'], capture_output=True)
    if not os.path.exists(W + '/SW.OBJ'):
        return 'no obj'
    out = subprocess.run([PY, STACKMAP, W + '/SW.OBJ', 'rrr', 'cos_sin_init_'],
                         capture_output=True, text=True).stdout.strip()
    return out.split('cos_sin_init_')[-1].strip()


def score(obj):
    out = subprocess.run([PY, VARIANTS, obj, 'rrr', '0x13c34', '0x13cba', 'cos_sin_init_'],
                         capture_output=True, text=True).stdout.strip()
    return out.split('instrdiffs=')[-1].strip()


def main():
    os.makedirs(W, exist_ok=True)
    hits = 0
    for size in (2, 3, 4):
        for cosd in range(size):
            for ds in itertools.permutations(['int i;', 'float a;', 'int y;', 'int d[%d];' % size]):
                src = ('void cos_sin_init(void)\n{\n' + ''.join('    ' + d + '\n' for d in ds)
                       + '    for (a = 0; a < 32; a++) {\n'
                       + '        d[%d] = cos(%s) * 65536.0;\n' % (cosd, E)
                       + '        i = a;\n        y = sin(%s) * 65536.0;\n' % E
                       + '        cos_tbl[i] = d[%d];\n        sin_tbl[i] = y;\n' % cosd
                       + '    }\n}\n')
                got = slots(src)
                if WANT in got:
                    hits += 1
                    print('HIT  d[%d] cos=d[%d] | %s | %s | score=%s'
                          % (size, cosd, ' '.join(d.rstrip(';') for d in ds), got,
                             score(W + '/SW.OBJ')), flush=True)
                elif 'slots: 1c 18' in got or 'slots: 18 1c' in got:
                    print('near d[%d] cos=d[%d] | %s | %s'
                          % (size, cosd, ' '.join(d.rstrip(';') for d in ds), got), flush=True)
    print('done hits=%d' % hits)


if __name__ == '__main__':
    main()
