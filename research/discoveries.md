# Confirmed discoveries

Evidence-backed findings from the RedCat's Razende Rekenrace CD. Confidence
levels per AGENTS.md.

## Source media

- CONFIRMED: Project input is `Redcat Razende Rekenrace.iso` (45,400,064
  bytes, MD5 5da0cf309f93029de17496b815894de7), ISO 9660, volume label
  `RCREKEN`. 145 files; full list with sizes and MD5s in
  `reverse-engineering/analysis/cd-file-inventory.tsv`.
- CONFIRMED: `LEESMIJ.TXT` says "(c) 1996 Davilex Software BV". Newest file
  dates on the CD are 1997-08-21 (Autorun.exe, Setup.exe, LEESMIJ.doc); the
  game executables are dated 1997-08-18.
- CONFIRMED: requirements per LEESMIJ.TXT: 386, 6 MB free memory, SVGA, Sound
  Blaster compatible. The CD carries a `vesa/` folder of VESA BIOS drivers for
  many 1995-era cards.

## Executables

All 32-bit programs are Watcom C, LE format, bound to DOS/4GW. Details in
`reverse-engineering/executables.md`.

- CONFIRMED: `MOTOR.EXE` (16-bit Watcom C, 19,194 bytes) is the program the
  start batch file runs (`MOTOR /DAVILEX %1..%5`, rcreken.tmp → RCREKEN.BAT).
  It contains the names `intro.exe`, `rrr.exe`, `reken.exe`, `rrrmenu.exe`,
  `motor.exe` and the message "De installatie van Redcat's Razende Rekenrace is
  waarschijnlijk niet compleet."
- CONFIRMED: the four 32-bit programs and their archives (from the string
  tables): `INTRO.EXE` (intro.img/ind, introsnd.img/ind, rrrintro.smk),
  `RRRMENU.EXE` (rrrmenu, rrranim, rrrmsnd, rrrfont, RRRHIGH.DAT,
  EINDANI2.SMK), `RRR.EXE` (rc, rcs, sndrcs: the race), `REKEN.EXE` (reken,
  rekensnd: the arithmetic exercises).
- CONFIRMED: all programs share Davilex library code; source names embedded in
  error strings include `rctools.c`, `rcascii.c`, `rcbutton.c`, `rcpcx.c`,
  `rcobj.c`, `rcimgret.c`, `fileio.c`, `smacker.c`, `sndbuf.c`, `audio.c`,
  `mouse.c`, `irq7.c`. REKEN.EXE keeps full paths:
  `C:\PROTMOD\WINLIB\mgl2\fg2\rctools.c`, `C:\PROTMOD\SOUNDLIB\wc32\mod\audio.c`.
- CONFIRMED: Smacker video is decoded by the game's own `smacker.c`
  (INTRO.EXE); there is no Smacker DLL. INTRO.EXE accepts `/NOSMACK`.
- CONFIRMED: command-line switches in the 32-bit programs: `/DAVILEX`,
  `/DEBUG`, `/NOSOUND`, `/NOMUSIC`, `/SBP:2x0`, `/IRQ:n`, `/DMA:n`
  (and `/NOSMACK` in INTRO.EXE). Errors go to `debug.txt`, which the batch
  file shows with DAVIVIEW when present.

## Archives (IND/IMG)

- CONFIRMED: the same Davilex IND/IMG pair as A2 Racer II (1998) uses, in both
  variants (`reverse-engineering/file-formats.md`):
  - named: `u16 count; {char name[20]; u32 offset}[count]`, IND length exactly
    `2 + 24*count` (INSTALL, INTRO, INTROSND, RCS, REKEN, REKENSND, RRRMENU,
    RRRMSND, SNDINST, SNDRCS);
  - indexed: `u16 count; u32 offset[count]`, IND length `2 + 4*count`, IMG
    repeating the count in its first u16 (INSTMNU, MENU, RC, RRRANIM,
    XVGALIB).
- CONFIRMED: every entry of the indexed archives INSTMNU (110), MENU (479),
  RC (1056) and RRRANIM (72) is a sprite `u16 w, h, x, y; u8 pixels[w*h]`:
  `8 + w*h` equals the entry size for all 1,717 entries. XVGALIB's three
  entries (2,050 bytes each) are something else.
- CONFIRMED: named archives hold ordinary PCX (640×480×8), RIFF WAV and
  Smacker 2 files (`rrrintro.smk`; `wbrutus/wredcat/wsnitch.smk` 320×200, 160
  frames), plus the game's own `.MAP`, tileset `.img` and font `ASCIInn.IMG`.

## Race tracks

- CONFIRMED: the race view is a top-down tile map. A map (`trackNx.map`,
  in RCS.IMG) is `u16 width, u16 height` then `width*height` cells of three
  u16; file size is exactly `4 + 6*w*h` for all 19 maps. Cell field 0 is the
  tile index (rendered TRACK1A.MAP draws a coherent desert circuit).
- CONFIRMED: a tileset (`track1.img`, `track2.img`) is `u16 count` (256),
  `count` tiles of 0x500 bytes, then a 768-byte 6-bit VGA palette:
  2 + 256×1280 + 768 = 328,450, both files. RRR's `load_xvga_map_images`
  (0x1fad6) reads the count, then 0x500 bytes per tile, passing 0x20 and 0x28
  to `0x1fa1b`. Tiles are 32 wide and 40 high (row-major), confirmed visually.
- CONFIRMED: RRR picks the map in `race_init` (0x10817 → 0x10ae4):
  `map = stage_map_index()` (0x1ae44) = `table_0x57b8c[group][stage] - 1`,
  5 stages per group row. If that is outside 0..22 it falls back to
  `rand() % 22`. Map file names are at 0x57a60[23], their tilesets at
  0x57abc[23].
- CONFIRMED: the table at 0x57b8c, six rows of five, decoded to map names
  (— = entry 0). That row r is groep r+3 is LIKELY (six rows, groepen 3–8 on
  the menu screen); see hypotheses.

  | row | maps by column |
  |---|---|
  | 0 | track1a, track1b, —, track1c, — |
  | 1 | track2a, —, track2b, track2c, — |
  | 2 | track3a, track3b, track3c, track3d, — |
  | 3 | track4a, track4b, —, track4c, track4d |
  | 4 | track5a, —, track4a, track6b, track4d |
  | 5 | track5a, track6b, track6c, track6d, track6e |

- CONFIRMED: tilesets per map (table 0x57abc): `track1.img` for track1a,
  track1c, track2a/b/c and track3a; `track2.img` for track1b and every map
  from track3b on.
