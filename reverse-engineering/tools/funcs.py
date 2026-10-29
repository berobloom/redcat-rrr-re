#!/usr/bin/env python3
"""Find the functions of a loaded LE program and tag them with evidence.

Recursive descent from the entry point: follows calls, branches and
`jmp [reg*4+table]` jump tables.  Then every byte-scanned `call rel32` target
that is not yet covered is tried as a seed too (indirect callers such as
function-pointer tables reach code the descent cannot see), and kept only if
it decodes to a ret without running into covered code or an invalid opcode.

For each function the strings it references are listed, and a source file is
assigned when one of them is a Watcom __FILE__ string ("xxx.c").

usage: funcs.py <prog>        prints TSV: addr, calls-in, source, strings
"""
import os
import re
import struct
import sys

import capstone
from capstone import x86

sys.path.insert(0, os.path.dirname(__file__))
from lelib import Image  # noqa: E402

SRC_RE = re.compile(r'([A-Za-z0-9_]+\.(?:c|cpp|asm))', re.I)


class Descent:
    def __init__(self, img):
        self.img = img
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.md.detail = True
        self.insn = {}          # addr -> capstone insn
        self.funcs = set()
        self.calls_in = {}

    def decode(self, a):
        if a in self.insn:
            return self.insn[a]
        b = self.img.read(a, 16)
        if not b:
            return None
        for i in self.md.disasm(b, a, 1):
            return i
        return None

    def jump_table(self, ins):
        """Targets of `jmp dword ptr [reg*4 + disp]` with disp in code."""
        if len(ins.operands) != 1 or ins.operands[0].type != x86.X86_OP_MEM:
            return []
        m = ins.operands[0].mem
        if m.scale != 4 or m.base != 0 or not self.img.is_code(m.disp):
            return []
        out = []
        p = m.disp
        while True:
            t = self.img.u32(p)
            if t is None or not self.img.is_code(t):
                break
            out.append(t)
            p += 4
            if len(out) > 512:
                break
        return out

    def walk(self, start, trial=False):
        """Decode one function body; return False (and undo) on a bad trial."""
        work, seen = [start], []
        while work:
            a = work.pop()
            while True:
                if a in self.insn:
                    if trial and a not in seen:
                        if a == start:
                            break
                        # ran into existing code: fine only at a call target
                    break
                ins = self.decode(a)
                if ins is None or not self.img.is_code(a):
                    if trial:
                        for s in seen:
                            self.insn.pop(s, None)
                        return False
                    break
                self.insn[a] = ins
                seen.append(a)
                mn = ins.mnemonic
                tgt = None
                if ins.operands and ins.operands[0].type == x86.X86_OP_IMM:
                    tgt = ins.operands[0].imm
                if mn == 'call':
                    if tgt is not None and self.img.is_code(tgt):
                        self.calls_in[tgt] = self.calls_in.get(tgt, 0) + 1
                        if tgt not in self.funcs:
                            self.funcs.add(tgt)
                            self.pending.append(tgt)
                elif mn.startswith('j') or mn.startswith('loop'):
                    if tgt is not None:
                        if self.img.is_code(tgt):
                            work.append(tgt)
                    elif mn == 'jmp':
                        work.extend(self.jump_table(ins))
                    if mn == 'jmp':
                        break
                elif mn in ('ret', 'retf', 'iretd', 'iret', 'hlt'):
                    break
                a += ins.size
        return True

    def run(self, seeds, trials):
        self.pending = list(seeds)
        self.funcs |= set(seeds)
        self._drain()
        for t in sorted(trials):
            if t in self.insn or t in self.funcs:
                continue
            if self.walk(t, trial=True):
                self.funcs.add(t)
                self._drain()

    def _drain(self):
        while self.pending:
            self.walk(self.pending.pop())


def scan_calls(img):
    code, base = img.code, img.code_base
    t = set()
    for i in range(len(code) - 5):
        if code[i] == 0xE8:
            a = base + i + 5 + struct.unpack_from('<i', code, i + 1)[0]
            if base <= a < base + len(code):
                t.add(a)
    return t


def analyse(img):
    d = Descent(img)
    d.run([img.entry], scan_calls(img))
    return d


def main():
    img = Image(sys.argv[1])
    d = analyse(img)
    starts = sorted(d.funcs)
    ends = starts[1:] + [img.code_base + len(img.code)]
    covered = 0
    for a, e in zip(starts, ends):
        strs, src = [], None
        for ia in sorted(x for x in d.insn if a <= x < e):
            ins = d.insn[ia]
            covered += ins.size
            for m in re.finditer(r'0x[0-9a-f]+', ins.op_str):
                v = int(m.group(0), 16)
                if v >= img.data_base:
                    s = img.cstr(v)
                    if s and s not in strs:
                        strs.append(s)
                        mm = SRC_RE.fullmatch(s.strip())
                        if mm and not src:
                            src = mm.group(1).lower()
        print('0x%06x\t%d\t%s\t%s' % (a, d.calls_in.get(a, 0), src or '-',
              ' | '.join(repr(s)[:70] for s in strs[:6])))
    print('# %d functions, %d of %d code bytes decoded' %
          (len(starts), covered, len(img.code)), file=sys.stderr)


if __name__ == '__main__':
    main()
