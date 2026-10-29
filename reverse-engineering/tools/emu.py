#!/usr/bin/env python3
"""Run single functions of a loaded LE program under Unicorn.

    from emu import Emu
    e = Emu('reken')
    e.stub(0x22bd2, e.watcom_rand)          # Python in place of a callee
    e.stub(0x1a1ee, e.sprintf)
    e.seed = 1
    e.call(0x1122e, eax=0)                  # Watcom: args in eax, edx, ebx, ecx
    e.cstr(0x5371c), e.u32(0x537e4)

The objects are mapped at their LE image addresses (as leparse.py fixed them
up), so globals keep their addresses. A stub runs when execution reaches its
address; it may set registers and returns as `ret` would. Callers rely on
Watcom's convention that a callee keeps every register except eax, which the
stubs respect.
"""
import os
import re
import struct
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
                               UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI,
                               UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP)

sys.path.insert(0, os.path.dirname(__file__))
from lelib import Image  # noqa: E402

REGS = {'eax': UC_X86_REG_EAX, 'ebx': UC_X86_REG_EBX, 'ecx': UC_X86_REG_ECX,
        'edx': UC_X86_REG_EDX, 'esi': UC_X86_REG_ESI, 'edi': UC_X86_REG_EDI,
        'ebp': UC_X86_REG_EBP}
STACK_TOP = 0x00f00000
SENTINEL = 0x00eff000        # return address that ends a call()


class Emu:
    def __init__(self, prog):
        self.img = Image(prog)
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        for o, (base, b, _) in zip(self.img.meta['objects'], self.img.objs):
            size = (max(o['vsize'], len(b)) + 0xfff) & ~0xfff
            self.uc.mem_map(base, size)
            self.uc.mem_write(base, b)
        self.uc.mem_map(STACK_TOP - 0x100000, 0x100000)
        self.stubs = {}
        self.seed = 1
        self.uc.hook_add(UC_HOOK_CODE, self._hook)

    def stub(self, addr, fn):
        self.stubs[addr] = fn

    def reg(self, name, value=None):
        if value is None:
            return self.uc.reg_read(REGS[name])
        self.uc.reg_write(REGS[name], value & 0xffffffff)

    def u32(self, a):
        return struct.unpack('<I', self.uc.mem_read(a, 4))[0]

    def i32(self, a):
        return struct.unpack('<i', self.uc.mem_read(a, 4))[0]

    def put32(self, a, v):
        self.uc.mem_write(a, struct.pack('<I', v & 0xffffffff))

    def cstr(self, a, n=256):
        b = bytes(self.uc.mem_read(a, n))
        return b.split(b'\0')[0].decode('cp437')

    def arg(self, i):
        """The i-th stack argument of a cdecl callee (0 = first)."""
        return self.u32(self.uc.reg_read(UC_X86_REG_ESP) + 4 + 4 * i)

    def _hook(self, uc, addr, size, _):
        fn = self.stubs.get(addr)
        if fn is None:
            return
        fn(self)
        esp = uc.reg_read(UC_X86_REG_ESP)
        uc.reg_write(UC_X86_REG_EIP, self.u32(esp))
        uc.reg_write(UC_X86_REG_ESP, esp + 4)

    def call(self, addr, **regs):
        esp = STACK_TOP - 0x100
        self.put32(esp, SENTINEL)
        self.uc.reg_write(UC_X86_REG_ESP, esp)
        for k, v in regs.items():
            self.reg(k, v)
        self.uc.emu_start(addr, SENTINEL, count=10_000_000)
        return self.reg('eax')

    # common stubs

    def watcom_rand(self, e):
        """Watcom rand() (REKEN 0x22bd2, RRR's too): 15-bit LCG."""
        self.seed = (self.seed * 0x41c64e6d + 0x3039) & 0xffffffff
        self.reg('eax', (self.seed >> 16) & 0x7fff)

    def sprintf(self, e):
        """sprintf(buf, fmt, ...) for %d, %Nd, %N.Nd, %s and %%."""
        buf, fmt = self.arg(0), self.cstr(self.arg(1))
        args = iter(range(2, 64))

        def conv(m):
            spec = m.group(0)
            if spec == '%%':
                return '%'
            v = self.arg(next(args))
            if spec[-1] == 's':
                return spec % self.cstr(v)
            return spec % struct.unpack('<i', struct.pack('<I', v))[0]
        out = re.sub(r'%%|%[-0-9.]*[ds]', conv, fmt)
        self.uc.mem_write(buf, out.encode('cp437') + b'\0')
        self.reg('eax', len(out))
