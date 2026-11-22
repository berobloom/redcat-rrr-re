#!/usr/bin/env python3
"""variants.py: score many C variants of one function against the game in one compile.

usage: variants.py FILE.OBJ PROG 0xADDR 0xEND NAME [NAME...]

Compile one probe file holding every variant (several copies of the function under different names,
same -3r -s -ox options as src/check.sh), then list their publics here: each prints its size and
how many instructions differ from the game's bytes in [ADDR,END), best first. Addresses that fixups
fill are ignored, and absolute addresses in operands are compared as placeholders, so the answer is
about structure, not about where our data landed.
"""
import json, os, re, sys, difflib
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import capstone
import libmatch as L
import objmatch as M
import fndiff

ROOT = L.ROOT
obj, prog, addr, endg = sys.argv[1], sys.argv[2], int(sys.argv[3], 16), int(sys.argv[4], 16)
want = sys.argv[5:]
meta = json.load(open(os.path.join(ROOT, '.work', 'le', prog + '.json')))
code = open(os.path.join(ROOT, '.work', 'le', prog + '.obj1.bin'), 'rb').read()
cbase = meta['objects'][0]['base']
(_, recs), = list(L.modules(open(obj, 'rb').read()))
segs = L.code_of(recs)
pubs = sorted(p for p in M.publics(recs) if p[0] in segs)
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


def norm(s):
    return re.sub(r'0x[0-9a-f]{4,}', '@', s).replace('- @', '+ @')  # masked fixup, see fndiff.norm


def dis(buf, vma, upto):
    return [(i.address, i.size, norm(i.mnemonic + ' ' + i.op_str))
            for i in md.disasm(buf, vma) if i.address < upto]


game = dis(code[addr - cbase:addr - cbase + (endg - addr)], addr, endg)
res = []
for name in want:
    hit = [p for p in pubs if p[2] == name or M.plain(p[2]) == name]
    if not hit:
        print('%-10s not in obj' % name)
        continue
    seg, off, pname = hit[0]
    end = next((q[1] for q in pubs if q[0] == seg and q[1] > off), len(segs[seg][0]))
    buf, mask = segs[seg]
    ours = bytes(b if not mask[off + k] else 0x90 for k, b in enumerate(buf[off:end]))
    A = dis(ours, addr, endg)          # cut both sides at END, as the docstring says (iter 16)
    ops = fndiff.align([t[2] for t in A], [t[2] for t in game])    # better of autojunk on/off (iter 39)
    n = fndiff.rows(ops)
    sz = sum(1 for tag, i1, i2, j1, j2 in ops if tag == 'equal'
             for x, y in zip(A[i1:i2], game[j1:j2]) if x[1] != y[1])
    res.append((n, sz, len(ours), name))
for n, sz, ln, name in sorted(res)[:12]:
    print('%-10s %d bytes  instrdiffs=%d sizediffs=%d' % (name, ln, n, sz))
