"""RedCat's Razende Rekenrace data tools.

Usage (DATA = the CD root or the install dir, or $RCTOOLS_DATA):
  python3 port/rctools [-d DATA] list ARCH               entries of ARCH.IND/IMG
  python3 port/rctools [-d DATA] extract ARCH OUTDIR     every entry as a file
  python3 port/rctools [-d DATA] pcx ARCH:name OUT.png
  python3 port/rctools [-d DATA] sprites ARCH PAL OUTDIR sprites as PNG; PAL is
                                                         ARCH:name.pcx or .img
  python3 port/rctools [-d DATA] tileset ARCH:name OUT.png
  python3 port/rctools [-d DATA] map N OUT.png [SCALE]   race map N (0..22)
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import archives  # noqa: E402
import image  # noqa: E402
import pcx as pcxmod  # noqa: E402
import sprite as spritemod  # noqa: E402
import track  # noqa: E402


def find_file(data_dir, name):
    """Case-insensitive lookup: the CD has RC.IND, the batch copies menu.IMG."""
    want = name.lower()
    for f in os.listdir(data_dir):
        if f.lower() == want:
            return os.path.join(data_dir, f)
    raise FileNotFoundError(f"{name} not in {data_dir}")


class Archive:
    def __init__(self, data_dir, base):
        self.name = base.upper()
        ind = open(find_file(data_dir, base + ".ind"), "rb").read()
        self.img = open(find_file(data_dir, base + ".img"), "rb").read()
        self.kind = archives.detect_ind(ind)
        if self.kind == "named":
            _, ents = archives.parse_named_ind(ind)
            sizes = archives.entry_sizes(ents, len(self.img))
            self.entries = [(n, o, s) for (n, _, o), s in zip(ents, sizes)]
        else:
            _, offs = archives.parse_binary_ind(ind)
            ends = offs[1:] + [len(self.img)]
            self.entries = [("%04d" % i, o, e - o)
                            for i, (o, e) in enumerate(zip(offs, ends))]

    def get(self, name):
        for n, o, s in self.entries:
            if n.lower() == name.lower():
                return self.img[o:o + s]
        raise KeyError(f"{name} not in {self.name}")


def load(data_dir, ref):
    arch, name = ref.split(":", 1)
    return Archive(data_dir, arch).get(name)


def palette_of(data):
    if data[0] == 0x0A:
        return pcxmod.load_pcx(data)["palette"]
    return track.parse_tileset(data)[1]


def main(argv):
    data_dir = os.environ.get("RCTOOLS_DATA", ".")
    if len(argv) > 2 and argv[1] == "-d":
        data_dir = argv[2]
        argv = argv[:1] + argv[3:]
    if len(argv) < 2:
        print(__doc__)
        return 1
    cmd, args = argv[1], argv[2:]
    if cmd == "list":
        a = Archive(data_dir, args[0])
        print(f"{a.name}: {a.kind}, {len(a.entries)} entries")
        for n, o, s in a.entries:
            print(f"  {n:20} {o:9} {s:9}")
    elif cmd == "extract":
        a = Archive(data_dir, args[0])
        os.makedirs(args[1], exist_ok=True)
        for n, o, s in a.entries:
            with open(os.path.join(args[1], n if a.kind == "named"
                                   else n + ".spr"), "wb") as f:
                f.write(a.img[o:o + s])
    elif cmd == "pcx":
        p = pcxmod.load_pcx(load(data_dir, args[0]))
        pcxmod.write_png(args[1], p["width"], p["height"], p["pixels"],
                         p["palette"])
    elif cmd == "sprites":
        a = Archive(data_dir, args[0])
        pal = palette_of(load(data_dir, args[1]))
        os.makedirs(args[2], exist_ok=True)
        for n, o, s in a.entries:
            sp = spritemod.parse(a.img, o, s)
            rows = spritemod.to_rgba_rows(sp, pal)
            image.write_png_rgb(os.path.join(args[2], n + ".png"),
                                sp["width"], sp["height"], rows, alpha=True)
    elif cmd == "tileset":
        tiles, pal = track.parse_tileset(load(data_dir, args[0]))
        cols = 16
        rows = []
        for r in range((len(tiles) + cols - 1) // cols):
            for py in range(track.TILE_H):
                row = bytearray()
                for c in range(cols):
                    i = r * cols + c
                    t = tiles[i] if i < len(tiles) else bytes(track.TILE_W
                                                              * track.TILE_H)
                    row += t[py * track.TILE_W:(py + 1) * track.TILE_W]
                rows.append(bytes(row))
        pcxmod.write_png(args[1], cols * track.TILE_W, len(rows), rows, pal)
    elif cmd == "map":
        mapname, imgname = track.MAPS[int(args[0])]
        tiles, pal = track.parse_tileset(load(data_dir, "rcs:" + imgname))
        w, h, cells = track.parse_map(load(data_dir, "rcs:" + mapname))
        rows = track.render(w, h, cells, tiles)
        scale = int(args[2]) if len(args) > 2 else 1
        if scale > 1:
            rows = [r[::scale] for r in rows[::scale]]
        pcxmod.write_png(args[1], len(rows[0]), len(rows), rows, pal)
        print(f"{mapname} ({imgname}): {w}x{h} tiles")
    else:
        print(__doc__)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
