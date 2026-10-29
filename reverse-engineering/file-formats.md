# File formats

All integers little-endian. Confidence and evidence for each claim are in
`research/discoveries.md`; this is the reference layout.

## IND/IMG archives

An `.IND` indexes the blob in the `.IMG` of the same name. Entry sizes are
implicit: the next entry's offset, or the IMG length for the last.

Named (IND length = 2 + 24·count):

```c
u16 count;
struct { char name[20]; u32 offset; } entry[count];  /* name NUL-terminated,
                                                         junk after the NUL */
```

Indexed (IND length = 2 + 4·count; the IMG starts with the same u16 count,
so entry 0 is at offset 2):

```c
u16 count;
u32 offset[count];
```

| archive | kind | entries | contents |
|---|---|---|---|
| INSTALL | named | 7 | installer screens, ASCII12.IMG, pif.pif |
| INSTMNU | indexed | 110 | installer sprites |
| INTRO / INTROSND | named | 1 / 1 | rrrintro.smk / rrrIntro.wav |
| MENU | indexed | 479 | sprites |
| RC | indexed | 1056 | race sprites |
| RCS | named | 34 | maps, tilesets, podium and wait screens, racer videos, fonts |
| REKEN | named | 6 | fonts, exercise screens |
| REKENSND | named | 100 | spoken numbers and phrases |
| RRRANIM | indexed | 72 | menu animation sprites |
| RRRMENU | named | 6 | menu screens, eindani2.smk |
| RRRMSND | named | 12 | menu sounds and music |
| SNDINST | named | 1 | installer sound test |
| SNDRCS | named | 39 | race sounds |
| XVGALIB | indexed | 3 | unknown, 2,050 bytes each |

`port/rctools` lists and extracts them: `python3 port/rctools list RCS`.

## Sprites (entries of MENU, RC, RRRANIM, INSTMNU)

```c
u16 width, height;
u16 x, y;               /* hotspot (meaning not yet checked in this game) */
u8  pixels[width * height];   /* row-major palette indices; 0xFF pads the
                                 corners of RC.IMG's sprites and is A2 Racer
                                 II's transparent key: LIKELY the same here */
```

No palette; the screen's palette applies.

## PCX

Standard ZSoft PCX version 5, 8 bits, one plane, RLE, 640×480, with the
256-colour palette (0x0C marker) at the end.

## Tileset (`track1.img`, `track2.img`)

```c
u16 count;                     /* 256 */
u8  tile[count][40][32];       /* 32 wide, 40 high, row-major */
u8  palette[256][3];           /* 6-bit VGA DAC values (0..63) */
```

## Map (`trackNx.map`)

```c
u16 width, height;             /* in tiles; TRACK1A is 99 x 155 */
struct {
    u8 tile;                   /* tileset index */
    u8 object;                 /* 0 in the files; RRR stores the object
                                  number of a bonus/sign here at load */
    u8 waypoint;               /* waypoint number, when racer != 0 */
    u8 racer;                  /* 1 REDCAT, 2 BULL, 4 MUIS: whose waypoint */
    u8 type;                   /* object/surface type, 0..21 */
    u8 level;                  /* ground level; blocks z < level*1000 */
} cell[height][width];
```

Types seen in code: 10, 12, 13 bonus objects; 11 a speed surface; 12 hazard
(damage); 13 boost pickup; 14 ramp; 15, 16 cacti; 18 finish sign; 19 start
sign; 20 finish line. Details: discoveries "The race: map cells".

Which tileset goes with which map, and which map a race uses, is in RRR's
tables (discoveries "Race tracks").

## RRRIO.INF

The state RRRMENU, REKEN and RRR share (REKEN keeps it at 0x534f4): a raw dump of RRR's globals
0x75178..0x753a0 (0x228 bytes). Defaults from `rrrio_load` (0x1abb6):

| offset | RRR global | default | meaning |
|---|---|---|---|
| 0x00 | 0x75178 | 3 | format version (anything else: defaults) |
| 0x04 | 0x7517c | 100 | damage points at the start |
| 0x0c | 0x75184 | 5 | boosts |
| 0x14 | 0x7518c | 10 | rockets |
| 0x18 | 0x75190 | 0 | ? (stats +0x10) |
| 0x1c | 0x75194 | 0 | ? (stats +0x14) |
| 0x20 | 0x75198 | 75 | fuel |
| 0x24 | 0x7519c | 0 | stage: map column, = the module REKEN last played |
| 0x28 | 0x751a0 | 6 | groep 3–8 |
| 0x2c | 0x751a4 | 0 | world 0–5 (row of the map and module tables) |
| 0x30 | 0x751a8 | 3 | ? (REKEN [0x46c87], gates prize sounds) |
| 0x34 | 0x751ac | 0 | joystick on |
| 0x50 | 0x751c8 | 0x1c, 0x36, 0x4b, 0x4d, 0x48, 0x50 | key scancodes: Enter, right Shift, ←, →, ↑, ↓ |
| 0x68 | 0x751e0 | | the pupil's name, a C string (RRRMENU 0x10ed1 puts "RedCat" when empty); the 400 bytes up to 0x1f8 are cleared by default |
| 0x1f8 | 0x75370 | 0 | race done (set by RRR 0x1585e; lets REKEN advance) |
| 0x1fc | 0x75374 | 0, 2, 4 | start order of the racers |
| 0x214 | 0x7538c | 0, 0, 0 | points per racer |

## Pupil files (`%8.8d.SMD`)

One file per pupil in the game directory, written by REKEN at the end of
each module (`smd_append` 0x19f07) and read by the menu's Rapport (RRRMENU
0x12afd). Both executables carry the same code ("somdata.c"). CONFIRMED
from the code and, for the reading side, by DOSBox runs with crafted files.

| offset | size | meaning |
|---|---|---|
| 0 | 255 | the pupil's name (RRRIO +0x68), zero-padded |
| 0xff | 4 | record count n (little-endian) |
| 0x103 | | n records, 0x5b (91) bytes apart; the last one may be only 27 bytes |

A record: +0 module 0..3 (the one played: groep 3's Memory instead of
Grote getallenspel records 2), +4 sums asked (Memory: pairs tried), +8
right (Grote getallenspel: always 0, see discoveries), +12 the date as
"dd/mm/yyyy\0" (11 bytes; the month is `tm_mon`, counted from 0), +23 the
groep. The 64 bytes after +27 are not used.

Finding the file (`smd_find` 0x19cfb): `*.SMD` in directory order; the
first whose name matches (case-insensitive, 0x1a807) is the pupil's. If none
matches, the new file's number is one past the number of the last file seen
(1 if none). Appending reads the n records (n × 0x5b bytes; a short last
record reads short), then rewrites the file: the name, n + 1, the n records
at the full stride (bytes past each record's 27 are whatever the buffer
held), and the new record's 27 bytes.

## ASCII fonts (`ASCII12/20/30.IMG` in REKEN.IMG and INSTALL.IMG, `RRRFONT.IMG`)

Read by `0x21bc7` (rcascii.c) in REKEN:

```c
u8  present[256];         /* non-zero: the program keeps this glyph */
u16 width, height;        /* 8x12, 10x20, 18x29; RRRFONT 10x21 */
u16 fg;                   /* glyph body colour: 200 (RRRFONT 253) */
u16 bk;                   /* 255 = transparent */
u8  glyph[256][height][width];   /* only the first 135 are read */
```

Pixels other than fg and bk are shading (201 = shadow, 202..223 edge
tones). The program recolours the loaded glyphs in place: `0x219a0` fg → c,
`0x21a6b` bk → c, `0x21b36` (c, old) old → c; REKEN sets fg 254, bk 255 and
201 → 0 after loading.
