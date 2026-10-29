#!/usr/bin/env python3
"""Find code that references an address or a string.

usage: xref.py <prog> 0xADDR          references to that linear address
       xref.py <prog> 'text'          every string containing text, and
                                      the references to each

A reference is any decoded instruction whose operand holds the value (the
decoded set from funcs.py's recursive descent), so it finds `push offset`,
`mov reg, offset` and `[disp]` alike.  Each hit names the function it is in.
"""
import bisect
import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from lelib import Image  # noqa: E402
import funcs  # noqa: E402


def main():
    prog, what = sys.argv[1], sys.argv[2]
    img = Image(prog)
    d = funcs.analyse(img)
    starts = sorted(d.funcs)
    targets = {}
    if re.fullmatch(r'0x[0-9a-fA-F]+', what):
        targets[int(what, 16)] = what
    else:
        needle = what.encode('cp437')
        p = 0
        while True:
            p = img.data.find(needle, p)
            if p < 0:
                break
            s = p
            while s > 0 and img.data[s - 1] != 0:
                s -= 1
            va = img.data_base + s
            targets[va] = img.cstr(va) or needle.decode('cp437')
            p += 1
    refs = {t: [] for t in targets}
    for a, ins in d.insn.items():
        for m in re.finditer(r'0x[0-9a-f]+', ins.op_str):
            v = int(m.group(0), 16)
            if v in refs:
                refs[v].append(a)
    for t in sorted(targets):
        print('0x%x %r' % (t, targets[t]))
        for a in sorted(refs[t]):
            f = starts[bisect.bisect_right(starts, a) - 1]
            print('    0x%x  in func 0x%x  %s %s' % (
                a, f, d.insn[a].mnemonic, d.insn[a].op_str))
        # pointers to it stored in the data object (string/function tables)
        needle = t.to_bytes(4, 'little')
        p = img.data.find(needle)
        while p >= 0:
            if p % 4 == 0:
                print('    data pointer at 0x%x' % (img.data_base + p))
            p = img.data.find(needle, p + 1)


if __name__ == '__main__':
    main()
