#!/usr/bin/env python3
"""Linear Executable (LE, DOS/4GW) loader.

Reads the LE image embedded in a bound Watcom executable (RRR.EXE, REKEN.EXE,
RRRMENU.EXE, INTRO.EXE, install.exe), lays the objects out at their
relocation base addresses, applies the internal fixups and writes:

  <out>.elf   ELF32 i386 executable, one PT_LOAD per object, entry = LE EIP,
              so REA/Ghidra/objdump see the code at the addresses the game ran
              at (DOS/4GW flat model: no relocation beyond the fixups here).
  <out>.json  header, object table and fixup statistics.

Selector fixups (type 2) are patched with the object number, which is enough
to tell the code and data segments apart when reading disassembly.

usage: leparse.py <exe> [<out-prefix>]   (no prefix: print the summary only)
"""
import json
import struct
import sys


def u8(b, o): return b[o]
def u16(b, o): return struct.unpack_from('<H', b, o)[0]
def u32(b, o): return struct.unpack_from('<I', b, o)[0]


class LE:
    def __init__(self, data):
        self.data = data
        if data[:2] != b'MZ':
            raise ValueError('not an MZ file')
        lfa = u32(data, 0x3c)
        if data[lfa:lfa + 2] != b'LE':
            raise ValueError('no LE header at e_lfanew 0x%x' % lfa)
        self.hdr = h = lfa
        g = lambda o: u32(data, h + o)
        self.num_pages = g(0x14)
        self.eip_obj, self.eip = g(0x18), g(0x1c)
        self.esp_obj, self.esp = g(0x20), g(0x24)
        self.page_size = g(0x28)
        self.last_page = g(0x2c)
        self.obj_tab = h + g(0x40)
        self.num_objs = g(0x44)
        self.page_map = h + g(0x48)
        self.fixup_pages = h + g(0x68)
        self.fixup_recs = h + g(0x6c)
        self.data_pages = g(0x80)            # absolute file offset
        self.objects = []
        for i in range(self.num_objs):
            o = self.obj_tab + 24 * i
            vsize, base, flags, pidx, pcnt = struct.unpack_from('<5I', data, o)
            self.objects.append(dict(num=i + 1, vsize=vsize, base=base,
                                     flags=flags, page_index=pidx,
                                     page_count=pcnt))

    def page_file_offset(self, page):           # page is 1-based, logical
        e = self.page_map + 4 * (page - 1)
        b = self.data[e:e + 4]
        num = (b[0] << 16) | (b[1] << 8) | b[2]
        return self.data_pages + (num - 1) * self.page_size, b[3]

    def page_len(self, page):
        return self.last_page if page == self.num_pages else self.page_size

    def object_image(self, obj):
        img = bytearray(obj['vsize'])
        for k in range(obj['page_count']):
            page = obj['page_index'] + k
            off, flags = self.page_file_offset(page)
            n = min(self.page_len(page), len(img) - k * self.page_size)
            if n > 0:
                img[k * self.page_size:k * self.page_size + n] = \
                    self.data[off:off + n]
        return img

    def fixups(self, page):
        """Yield (src_type, src_off_in_page, target_obj, target_off) for one
        logical page.  Only internal references occur in these binaries."""
        d = self.data
        start = self.fixup_recs + u32(d, self.fixup_pages + 4 * (page - 1))
        end = self.fixup_recs + u32(d, self.fixup_pages + 4 * page)
        p = start
        while p < end:
            src, flg = d[p], d[p + 1]
            p += 2
            if src & 0x20:
                cnt = d[p]; p += 1
                offs = None
            else:
                offs = [struct.unpack_from('<h', d, p)[0]]; p += 2
            if flg & 3 != 0:
                raise ValueError('non-internal fixup (flags 0x%02x) on page %d'
                                 % (flg, page))
            if flg & 0x40:
                tobj = u16(d, p); p += 2
            else:
                tobj = d[p]; p += 1
            stype = src & 0x0f
            toff = 0
            if stype != 0x02:
                if flg & 0x10:
                    toff = u32(d, p); p += 4
                else:
                    toff = u16(d, p); p += 2
            if offs is None:
                offs = list(struct.unpack_from('<%dh' % cnt, d, p)); p += 2 * cnt
            for so in offs:
                yield stype, so, tobj, toff

    def load(self):
        """Return {objnum: bytearray} with fixups applied, plus stats."""
        imgs = {o['num']: self.object_image(o) for o in self.objects}
        stats = {}
        for o in self.objects:
            img = imgs[o['num']]
            for k in range(o['page_count']):
                page = o['page_index'] + k
                for stype, so, tobj, toff in self.fixups(page):
                    stats[stype] = stats.get(stype, 0) + 1
                    at = k * self.page_size + so
                    tgt = self.objects[tobj - 1]['base'] + toff
                    if stype == 0x07:                       # 32-bit offset
                        if 0 <= at and at + 4 <= len(img):
                            struct.pack_into('<I', img, at, tgt)
                    elif stype == 0x08:                     # 32-bit self-rel
                        if 0 <= at and at + 4 <= len(img):
                            rel = tgt - (o['base'] + at + 4)
                            struct.pack_into('<i', img, at, rel)
                    elif stype == 0x02:                     # 16-bit selector
                        if 0 <= at and at + 2 <= len(img):
                            struct.pack_into('<H', img, at, tobj)
                    elif stype == 0x05:                     # 16-bit offset
                        if 0 <= at and at + 2 <= len(img):
                            struct.pack_into('<H', img, at, tgt & 0xffff)
                    elif stype == 0x06:                     # 16:32 pointer
                        if 0 <= at and at + 6 <= len(img):
                            struct.pack_into('<IH', img, at, tgt, tobj)
                    else:
                        raise ValueError('fixup type 0x%x' % stype)
        return imgs, stats

    def summary(self):
        return dict(eip_object=self.eip_obj, eip=self.eip,
                    entry=self.objects[self.eip_obj - 1]['base'] + self.eip,
                    esp_object=self.esp_obj, esp=self.esp,
                    page_size=self.page_size, num_pages=self.num_pages,
                    objects=self.objects)


def write_elf(path, objects, imgs, entry):
    """Minimal ELF32 i386 ET_EXEC: header, program headers, object bytes."""
    phnum = len(objects)
    off = 52 + 32 * phnum
    off = (off + 0xfff) & ~0xfff
    ph, body = b'', b''
    for o in objects:
        img = bytes(imgs[o['num']])
        fl = 0
        if o['flags'] & 1: fl |= 4                       # readable
        if o['flags'] & 2: fl |= 2                       # writable
        if o['flags'] & 4: fl |= 1                       # executable
        ph += struct.pack('<8I', 1, off + len(body), o['base'], o['base'],
                          len(img), len(img), fl, 0x1000)
        body += img
        body += b'\0' * (((len(body) + 0xfff) & ~0xfff) - len(body))
    eh = b'\x7fELF' + bytes([1, 1, 1, 0]) + b'\0' * 8
    eh += struct.pack('<HHIIIIIHHHHHH', 2, 3, 1, entry, 52, 0, 0, 52, 32,
                      phnum, 40, 0, 0)
    out = eh + ph
    out += b'\0' * (off - len(out)) + body
    open(path, 'wb').write(out)


def main():
    exe = sys.argv[1]
    le = LE(open(exe, 'rb').read())
    s = le.summary()
    imgs, stats = le.load()
    s['fixups'] = {'0x%02x' % k: v for k, v in sorted(stats.items())}
    if len(sys.argv) > 2:
        pre = sys.argv[2]
        write_elf(pre + '.elf', le.objects, imgs, s['entry'])
        for o in le.objects:
            open('%s.obj%d.bin' % (pre, o['num']), 'wb').write(imgs[o['num']])
        json.dump(s, open(pre + '.json', 'w'), indent=1)
    print('entry 0x%x  esp obj%d:0x%x  pages %d x %d' %
          (s['entry'], s['esp_object'], s['esp'], s['num_pages'], s['page_size']))
    for o in le.objects:
        print('  obj%d base 0x%08x vsize 0x%06x flags 0x%04x pages %d@%d' %
              (o['num'], o['base'], o['vsize'], o['flags'], o['page_count'],
               o['page_index']))
    print('  fixups', s['fixups'])


if __name__ == '__main__':
    main()
