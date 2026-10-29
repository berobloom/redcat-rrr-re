"""Shared helpers for rctools (pure stdlib)."""

import struct

u16 = struct.Struct("<H")
u32 = struct.Struct("<I")


def rd_u16(b, o):
    return u16.unpack_from(b, o)[0]


def rd_u32(b, o):
    return u32.unpack_from(b, o)[0]


def vga_palette(raw):
    """768 bytes of 6-bit VGA DAC values -> [(r, g, b)] scaled to 0..255."""
    return [tuple((c << 2) | (c >> 4) for c in raw[i:i + 3])
            for i in range(0, 768, 3)]
