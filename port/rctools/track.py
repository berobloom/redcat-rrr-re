"""Race tilesets (track1.img, track2.img) and maps (trackNx.map).

Tileset:  u16 count; u8 tile[count][40][32]; u8 palette[768] (6-bit VGA).
Map:      u16 width, height; struct { u16 tile, f1, f2; } cell[h][w].

The map -> tileset pairing is RRR.EXE's table at 0x57abc, copied here.
"""

import struct

from util import rd_u16, vga_palette

TILE_W, TILE_H = 32, 40

# RRR.EXE 0x57a60 (map names) and 0x57abc (their tilesets), 23 entries
MAPS = [
    ("track1a.map", "track1.img"), ("track1b.map", "track2.img"),
    ("track1c.map", "track1.img"), ("track2a.map", "track1.img"),
    ("track2b.map", "track1.img"), ("track2c.map", "track1.img"),
    ("track3a.map", "track1.img"), ("track3b.map", "track2.img"),
    ("track3c.map", "track2.img"), ("track3d.map", "track2.img"),
    ("track4a.map", "track2.img"), ("track4b.map", "track2.img"),
    ("track4c.map", "track2.img"), ("track4d.map", "track2.img"),
    ("track5a.map", "track2.img"), ("track4a.map", "track2.img"),
    ("track6b.map", "track2.img"), ("track4d.map", "track2.img"),
    ("track5a.map", "track2.img"), ("track6b.map", "track2.img"),
    ("track6c.map", "track2.img"), ("track6d.map", "track2.img"),
    ("track6e.map", "track2.img"),
]


def parse_tileset(data):
    count = rd_u16(data, 0)
    size = TILE_W * TILE_H
    if len(data) != 2 + count * size + 768:
        raise ValueError(f"tileset: {len(data)} bytes for {count} tiles")
    tiles = [data[2 + i * size:2 + (i + 1) * size] for i in range(count)]
    return tiles, vga_palette(data[-768:])


def parse_map(data):
    w, h = struct.unpack_from("<2H", data, 0)
    if len(data) != 4 + 6 * w * h:
        raise ValueError(f"map: {len(data)} bytes for {w}x{h}")
    cells = [struct.unpack_from("<3H", data, 4 + 6 * i) for i in range(w * h)]
    return w, h, cells


def render(w, h, cells, tiles):
    """Whole map as rows of palette indices (w*32 x h*40)."""
    rows = []
    for ty in range(h):
        for py in range(TILE_H):
            row = bytearray()
            for tx in range(w):
                t = tiles[cells[ty * w + tx][0] & 0xFF]
                row += t[py * TILE_W:(py + 1) * TILE_W]
            rows.append(bytes(row))
    return rows
