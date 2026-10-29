"""Shared access to a loaded LE program (see leparse.py).

Image('rrr') reads .work/le/rrr.{json,obj*.bin}, built by
    leparse.py .work/iso/RRR.EXE .work/le/rrr
and answers reads by linear address.
"""
import json
import os
import struct

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
LEDIR = os.path.join(ROOT, '.work', 'le')


class Image:
    def __init__(self, name):
        self.name = name
        meta = json.load(open(os.path.join(LEDIR, name + '.json')))
        self.entry = meta['entry']
        self.objs = []
        for o in meta['objects']:
            b = open(os.path.join(LEDIR, '%s.obj%d.bin' % (name, o['num'])),
                     'rb').read()
            self.objs.append((o['base'], b, o['flags']))
        self.code_base, self.code = self.objs[0][0], self.objs[0][1]
        self.data_base, self.data = self.objs[1][0], self.objs[1][1]

    def obj_of(self, va):
        for base, b, fl in self.objs:
            if base <= va < base + len(b):
                return base, b
        return None, None

    def read(self, va, n):
        base, b = self.obj_of(va)
        if b is None:
            return None
        return b[va - base:va - base + n]

    def u32(self, va):
        r = self.read(va, 4)
        return struct.unpack('<I', r)[0] if r and len(r) == 4 else None

    def cstr(self, va, maxlen=200):
        """The printable NUL-terminated string at va, else None."""
        base, b = self.obj_of(va)
        if b is None:
            return None
        o = va - base
        end = b.find(b'\0', o, o + maxlen)
        if end <= o:
            return None
        s = b[o:end]
        if all(32 <= c < 127 or c in (9, 10, 13) or c >= 0x80 for c in s) \
                and sum(32 <= c < 127 for c in s) >= max(2, len(s) * 3 // 4):
            return s.decode('cp437')
        return None

    def is_code(self, va):
        return self.code_base <= va < self.code_base + len(self.code)
