"""Sprite entries of the indexed archives (RC, MENU, RRRANIM, INSTMNU).

    u16 width;
    u16 height;
    u16 x, y;           // hotspot
    u8  pixels[width * height];

CONFIRMED: `8 + width*height` equals the entry size for all 1,717 entries of
those four archives.  0xFF pads the corners of RC.IMG sprites (the transparent
key in A2 Racer II; LIKELY the same here).  There is no palette in the entry.
"""

import struct

HEADER_LEN = 8
TRANSPARENT = 0xFF


def parse(data, offset=0, size=None):
    """Decode one sprite entry starting at `offset`."""
    w, h, ax, ay = struct.unpack_from("<4H", data, offset)
    need = HEADER_LEN + w * h
    if size is not None and size != need:
        raise ValueError(f"sprite: entry size {size} != {need} for {w}x{h}")
    px = data[offset + HEADER_LEN:offset + need]
    if len(px) != w * h:
        raise ValueError(f"sprite: truncated ({len(px)} of {w * h})")
    rows = [px[y * w:(y + 1) * w] for y in range(h)]
    return {"width": w, "height": h, "anchor": (ax, ay), "rows": rows}


def parse_archive(img, offsets):
    """Decode every entry given the offsets from the binary IND."""
    out = []
    for i, o in enumerate(offsets):
        end = offsets[i + 1] if i + 1 < len(offsets) else len(img)
        out.append(parse(img, o, end - o))
    return out


def to_rgba_rows(sprite, palette, transparent=TRANSPARENT):
    """Flatten to RGBA rows using a 256-entry [(r,g,b), ...] palette."""
    rows = []
    for row in sprite["rows"]:
        out = bytearray()
        for idx in row:
            if idx == transparent:
                out += b"\0\0\0\0"
            else:
                r, g, b = palette[idx]
                out += bytes((r, g, b, 255))
        rows.append(bytes(out))
    return rows
