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
struct { u16 tile, f1, f2; } cell[height][width];
```

`tile` indexes the tileset (low byte; max 0xFF seen). `f1` and `f2` are not
yet understood (hypotheses "Map cells").

Which tileset goes with which map, and which map a race uses, is in RRR's
tables (discoveries "Race tracks").
