#!/usr/bin/env python3
"""Compare the port's sum generators and prize choice with REKEN.EXE's own
code, run under Unicorn (reverse-engineering/tools/emu.py), seed by seed.

usage: check_reken.py <reken_sim binary> [seeds per case]
Needs .work/le/reken.* (see AGENTS.md "Setup") and unicorn.
"""
import os
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'reverse-engineering', 'tools'))
from emu import Emu  # noqa: E402

SIM = sys.argv[1]
N = int(sys.argv[2]) if len(sys.argv) > 2 else 500

GENERATORS = {0: 0x11496, 1: 0x11619, 2: 0x1122e, 3: 0x1179e}  # x : + -
TEXT, ANSWER = 0x5371c, 0x537e4
FUEL, REPAIR, BOOSTS, ROCKETS = 0x46938, 0x468fc, 0x46910, 0x46924
PRIZE, SPEECH = 0x46c86, 0x46c87

e = Emu('reken')
e.stub(0x22bd2, e.watcom_rand)
e.stub(0x1a1ee, e.sprintf)
e.stub(0x18e0b, lambda e: None)       # shows the prize picture
e.uc.mem_write(SPEECH, b'\0')         # no sound calls


def sim(*args):
    out = subprocess.run([SIM, *map(str, args)], capture_output=True,
                         text=True, check=True).stdout
    return out.splitlines()


bad = 0
for t, addr in GENERATORS.items():
    for level in range(6):
        port = sim('gen', t, level, 1, N)
        for i in range(N):
            e.seed = 1 + i
            e.uc.mem_write(TEXT, b'\0')
            e.put32(ANSWER, 0)
            e.call(addr, eax=level)
            orig = '%s|%d|%d' % (e.cstr(TEXT), e.i32(ANSWER), e.seed)
            if orig != port[i]:
                bad += 1
                if bad <= 10:
                    print('type %d level %d seed %d: original %r, port %r'
                          % (t, level, 1 + i, orig, port[i]))

STATES = [(75, 100, 5, 10), (40, 100, 5, 10), (120, 70, 5, 10),
          (100, 0, 5, 10), (100, 100, 99, 10), (100, 100, 5, 99),
          (30, 100, 99, 99), (150, 99, 0, 0)]
for st in STATES:
    port = sim('prize', *st, 1, N)
    for i in range(N):
        for a, v in zip((FUEL, REPAIR, BOOSTS, ROCKETS), st):
            e.put32(a, v)
        e.uc.mem_write(PRIZE, b'\0')
        e.seed = 1 + i
        e.call(0x18e7f)
        orig = '%d|%d' % (e.uc.mem_read(PRIZE, 1)[0], e.seed)
        if orig != port[i]:
            bad += 1
            if bad <= 10:
                print('prize %s seed %d: original %s, port %s'
                      % (st, 1 + i, orig, port[i]))

cases = len(GENERATORS) * 6 * N + len(STATES) * N
print('%d of %d cases differ' % (bad, cases))
sys.exit(bad != 0)
