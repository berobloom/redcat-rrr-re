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
from unicorn import UcError  # noqa: E402
from unicorn.x86_const import UC_X86_REG_EIP  # noqa: E402

SIM = sys.argv[1]
N = int(sys.argv[2]) if len(sys.argv) > 2 else 500

GENERATORS = {0: 0x11496, 1: 0x11619, 2: 0x1122e, 3: 0x1179e}  # x : + -
TEXT, ANSWER = 0x5371c, 0x537e4
FUEL, REPAIR, BOOSTS, ROCKETS = 0x46938, 0x468fc, 0x46910, 0x46924
PRIZE, SPEECH = 0x46c86, 0x46c87


def new_emu():
    e = Emu('reken')
    e.stub(0x22bd2, e.watcom_rand)
    e.stub(0x1a1ee, e.sprintf)
    e.stub(0x18e0b, lambda e: None)   # shows the prize picture
    e.stub(0x1d8f3, lambda e: None)   # debug printf
    e.uc.mem_write(SPEECH, b'\0')     # no sound calls
    return e


e = new_emu()


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

# module 2: three choices at 0x537ec + 100 i, their values at 0x53be4 + 4 i
CHOICE, VALUE, LEVEL = 0x537ec, 0x53be4, 0x46c84
crashes = []
for level in range(6):
    port = sim('gen2', level, 1, N)
    e.uc.mem_write(LEVEL, bytes([level]))
    for i in range(N):
        e.seed = 1 + i
        try:
            e.call(0x12844, eax=level)
        except UcError:
            # 0x123cf divides by zero when the span of quotients is empty;
            # the fault leaves the machine mid-call, so start a fresh one
            crashes.append((level, 1 + i, hex(e.uc.reg_read(UC_X86_REG_EIP))))
            e = new_emu()
            e.uc.mem_write(LEVEL, bytes([level]))
            continue
        orig = ''.join('%s|%d|' % (e.cstr(CHOICE + 100 * k), e.i32(VALUE + 4 * k))
                       for k in range(3)) + str(e.seed)
        if orig != port[i]:
            bad += 1
            if bad <= 10:
                print('module 2 level %d seed %d: original %r, port %r'
                      % (level, 1 + i, orig, port[i]))

# module 3: the deal, 0x14d5a with the play (0x14967) and the SMD record
# stubbed; cards at 0x58f98, 58 bytes: text, value +0x32, kind +0x36
GRID = [(4, 5), (5, 6), (5, 6), (3, 4), (4, 4), (4, 5)]
CARDS, CELLS = 0x58f98, 0x457ac
e.stub(0x14967, lambda e: None)
e.stub(0x19f07, lambda e: None)
for level in range(6):
    port = sim('gen3', level, 1, N)
    e.uc.mem_write(LEVEL, bytes([level]))
    cells = GRID[level][0] * GRID[level][1]
    for i in range(N):
        e.seed = 1 + i
        e.put32(CELLS, cells)
        e.call(0x14d5a)
        orig = ''.join('%s/%d/%d|' % (e.cstr(CARDS + 58 * k, 50),
                                      e.i32(CARDS + 58 * k + 0x32),
                                      e.i32(CARDS + 58 * k + 0x36))
                       for k in range(cells)) + str(e.seed)
        if orig != port[i]:
            bad += 1
            if bad <= 10:
                print('module 3 level %d seed %d:\n  original %r\n  port     %r'
                      % (level, 1 + i, orig, port[i]))

# module 4: 0x17c57's eight rounds with the ask (0x17b95) stubbed to
# read the numbers 0x5bf94 and the target [0x5c104]
COUNT, NUMS, TARGET = 0x45c30, 0x5bf94, 0x5c104
rounds = []
e.stub(0x17b95, lambda e: rounds.append(
    ' '.join(str(e.i32(NUMS + 4 * j)) for j in range(e.i32(COUNT)))
    + ' = %d|' % e.i32(TARGET)))
for level in range(6):
    port = sim('gen4', level, 1, N)
    e.uc.mem_write(LEVEL, bytes([level]))
    e.put32(COUNT, 3 if level in (4, 5) else 4)
    for i in range(N):
        e.seed = 1 + i
        rounds.clear()
        e.call(0x17c57)
        orig = ''.join(rounds) + str(e.seed)
        if orig != port[i]:
            bad += 1
            if bad <= 10:
                print('module 4 level %d seed %d:\n  original %r\n  port     %r'
                      % (level, 1 + i, orig, port[i]))

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

cases = len(GENERATORS) * 6 * N + 3 * 6 * N + len(STATES) * N
print('%d of %d cases differ' % (bad, cases))
if crashes:
    print('the original faults in %d module 2 cases, e.g. level %d seed %d at %s'
          % ((len(crashes),) + crashes[0]))
sys.exit(bad != 0)
