#!/usr/bin/env python3
"""nmlen.py: near-miss lengths, ours (.OBJ) vs the game's (next named function start).

usage: src/check.sh | grep "differs from" | awk '{print $1, $6, $2}' | tools/wc/nmlen.py

Sorts by the difference. Ours longer means we materialise something the game keeps live; ours shorter
means statements are missing. The game's length is only as good as the name list: `None` where the next
function is unnamed, so treat a big negative as a hint, not a measurement.
"""
import re
import sys

names = {}
for line in open('reverse-engineering/analysis/rrr_names.txt'):
    m = re.search(r'(0x[0-9a-f]{4,6})\b.*?([A-Za-z_][\w$]*)\s*$', line)
    if m:
        names.setdefault(int(m.group(1), 16), m.group(2))
addrs = sorted(names)
rows = []
for line in sys.stdin:
    n, a, l = line.split()
    a = int(a, 16)
    ours = int(l)
    i = addrs.index(a) if a in names else -1
    game = (addrs[i + 1] - a) if 0 <= i < len(addrs) - 1 else None
    rows.append((ours - game if game else 0, ours, game, a, n))
for d, ours, game, a, n in sorted(rows):
    print(f"{d:>6}  ours {ours:>5}  game {str(game):>5}  0x{a:x}  {n.rstrip('_')}")
