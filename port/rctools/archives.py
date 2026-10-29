"""IND/IMG archive readers (reverse-engineering/file-formats.md).

Two index formats, the same Davilex pair A2 Racer II uses:

1. Indexed IND (RC, MENU, RRRANIM, INSTMNU, XVGALIB):
   u16 count, then count x u32 offsets into the IMG blob.
   The IMG itself also starts with the same u16 count.

2. Named IND (RCS, REKEN, SNDRCS, ...):
       u16 count;
       struct { char name[20]; u32 offset; } entries[count];   // 24 bytes each
   `name` is NUL-terminated inside the 20-byte field; the bytes after the NUL
   are leftovers of the writer's buffer and carry no meaning.

Entry sizes are implicit: next offset - this offset, and IMG length for the
last entry.
"""

import struct

from util import rd_u16, rd_u32

NAMED_NAME_LEN = 20
NAMED_ENTRY_LEN = NAMED_NAME_LEN + 4


def parse_binary_ind(data):
    count = rd_u16(data, 0)
    offsets = [rd_u32(data, 2 + 4 * i) for i in range(count)]
    return count, offsets


def parse_named_ind(data):
    """Return (count, [(name, raw_name_field, offset), ...])."""
    count = rd_u16(data, 0)
    entries = []
    for i in range(count):
        o = 2 + i * NAMED_ENTRY_LEN
        raw = data[o:o + NAMED_NAME_LEN]
        name = raw.split(b"\0", 1)[0].decode("latin1")
        entries.append((name, raw, rd_u32(data, o + NAMED_NAME_LEN)))
    return count, entries






def entry_sizes(entries, img_size):
    """Implicit sizes for named-IND entries: gap to the next offset."""
    offs = [e[2] for e in entries]
    return [(offs[i + 1] if i + 1 < len(offs) else img_size) - offs[i]
            for i in range(len(offs))]


def detect_ind(data):
    if len(data) < 4:
        return None
    count = rd_u16(data, 0)
    # named IND is an exact fit: u16 + count fixed-size records
    if len(data) == 2 + count * NAMED_ENTRY_LEN:
        return "named"
    if len(data) == 2 + count * 4:
        return "binary"
    # fall back to the old shape test
    if 2 + 4 * count <= len(data):
        offs = [rd_u32(data, 2 + 4 * i) for i in range(min(count, 8))]
        if all(offs[i] <= offs[i + 1] for i in range(len(offs) - 1)):
            return "binary"
    return "named"
