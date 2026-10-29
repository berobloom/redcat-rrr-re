#!/usr/bin/env python3
"""Disassemble a loaded LE program at a linear address.

usage: disasm.py <prog> <addr> [count]      e.g. disasm.py rrr 0x26c1c 60
       disasm.py <prog> <addr> --func       stop at the first ret/jmp-out

Immediates and memory operands that point at a string are annotated with it;
call targets are annotated with names from analysis/<prog>_names.txt
(lines "0xADDR name").
"""
import os
import re
import sys

import capstone

sys.path.insert(0, os.path.dirname(__file__))
from lelib import Image, ROOT  # noqa: E402


def load_names(prog):
    p = os.path.join(ROOT, 'reverse-engineering', 'analysis',
                     prog + '_names.txt')
    names = {}
    if os.path.exists(p):
        for line in open(p):
            line = line.split('#')[0].split()
            if len(line) >= 2:
                names[int(line[0], 16)] = line[1]
    return names


def annotate(img, names, op_str):
    notes = []
    for m in re.finditer(r'0x[0-9a-f]+', op_str):
        v = int(m.group(0), 16)
        if v in names:
            notes.append(names[v])
        elif v >= img.data_base:
            s = img.cstr(v)
            if s:
                notes.append(repr(s[:60]))
    return notes


def main():
    prog, addr = sys.argv[1], int(sys.argv[2], 0)
    func = '--func' in sys.argv
    count = 4000 if func else int(sys.argv[3]) if len(sys.argv) > 3 else 40
    img = Image(prog)
    names = load_names(prog)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    code = img.read(addr, 0x10000)
    far = addr                       # furthest branch target inside the func
    n = 0
    for ins in md.disasm(code, addr):
        if ins.address in names and ins.address != addr:
            print('%s:' % names[ins.address])
        notes = annotate(img, names, ins.op_str)
        print('%08x  %-24s %-7s %s%s' % (
            ins.address, ins.bytes.hex(), ins.mnemonic, ins.op_str,
            ('   ; ' + ', '.join(notes)) if notes else ''))
        n += 1
        if func:
            if ins.mnemonic.startswith('j') and ins.op_str.startswith('0x'):
                t = int(ins.op_str, 16)
                if addr <= t < addr + 0x4000:
                    far = max(far, t)
            if ins.mnemonic in ('ret', 'retf', 'iretd') or \
                    (ins.mnemonic == 'jmp' and ins.address >= far):
                if ins.address >= far:
                    break
        if n >= count:
            break


if __name__ == '__main__':
    main()
