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

## Running the original

- CONFIRMED: the game runs in the dos-re-suite DOSBox-X (`runtime-dosbox.md`).
  `RRR /DAVILEX /DEBUG` starts a race on its own; with no RRRIO.INF it uses
  the defaults of `rrrio_load` (0x1abb6) and races track1a.
- CONFIRMED (screenshot): the race is 640×480, map view on top, HUD
  (RACESCR3.PCX) in the bottom 80 rows. The earlier guess of an unchained
  320×200 "xvga" mode was wrong; "xvga" is the name of Davilex's graphics
  library (`einde init xvga_lib`), not a mode.
- CONFIRMED: DOS/4GW loads RRR's code object at linear 0x1d4000 and its data
  object at 0x206000 under `redcat.conf` (deltas +0x1c4000 / +0x1b6000), found
  by searching memory for fixup-free byte windows (`dosrun.py locate`).

## RRRIO.INF

- CONFIRMED: RRRIO.INF is a raw 0x228-byte image of RRR's globals
  0x75178..0x753a0, read by `rrrio_load` (0x1abb6) and written by
  `rrrio_save` (0x1ab7f). Layout and defaults in `file-formats.md`. A file
  whose first int is not 3 is replaced by the defaults.

## The race: frame loop

- CONFIRMED: `race_main` (0x111e8) calls `race_init` (0x10817), then loops
  until Esc (scancode 1) or S (0x1f): `switch_anims`, `objects_tick`
  (0x25c55), `camera_follow`, `sort_draw_order`, `joystick_to_keys`,
  `hud_update`, `draw_view(scroll_y % 40, scroll_x % 32)`; the first pass
  runs `start_lights` (0x1103e: lamp steps after 1500, 1000, 1000, 1000 and
  500 ms). `finish_check` (0x10eff) ends the race loop once the finishing
  order is complete. F1/F2/F3 point the camera at racer 0/2/4 (a debug key).
- CONFIRMED: `objects_tick` (rcobj.c) runs 306 (0x132) object slots of 16
  bytes at `[0x804b8]`: `+0` tick period (-1 = free), `+2` sprite slot, `+4`
  x, `+6` y, `+8` anim, `+10` frame, `+12` callback. A slot runs when the
  tick counter is a multiple of its period; the callback moves it and the
  anim frame selects the sprite.
- CONFIRMED (`init_objects` 0x148db): slot 0 is the player (callback
  0x11eb5), 2 Brutus/BULL (0x12826), 4 Snitch/MUIS (0x130f8); 1, 3, 5 run
  0x167aa and copy the position of the racer before them; 6..305 are the free
  pool (0x16945).
- CONFIRMED: per object there is a 40-byte record at 0x6cbf0 + 0x28·n:
  `+0` x, `+4` y, `+8` height z, `+0xc` vx, `+0x10` vy, `+0x14` vz, `+0x18`
  speed (all 16.16 fixed point, pixels per frame), `+0x20` stats index.
  Per racer a 48-byte stats record at 0x6fbc0 + 0x30·k: `+0` mass (player
  60.0), `+4` heading 0..31, `+8` fuel, `+0xc` boosts, `+0x1c` rockets,
  `+0x28` damage points (100), `+0x2c` top speed.

## The race: driving (player_tick 0x11eb5)

- CONFIRMED: headings are 32 steps of 11.25°: 0 up, 8 left, 16 down, 24
  right. vx = speed·cos[h], vy = speed·sin[h] with the tables at 0x73de0 /
  0x73e60 (built at run time; cos[0] = 0, sin[0] = -1).
- CONFIRMED: steering is absolute. The four direction keys select one of
  eight screen directions from the 3×3 table at 0x56030
  (`[4,0,28, 8,-1,24, 12,16,20]`, rows up/none/down, columns
  left/none/right); the heading steps toward it through the 32×32 turn
  table at 0x71de0 twice per frame (22.5° per frame).
- CONFIRMED: holding a direction accelerates: speed += 30/mass per frame
  (player: +0.5 px/frame), fuel −0x666 per frame, an exhaust puff every 4th
  frame (0x17e87). Above the top speed (+4 px/frame) 0x180d3 runs. Otherwise
  speed ×= 0xf851/0x10000 (≈0.970) per frame, and snaps to 0 below 0x3333.
- CONFIRMED: the boost key (binding at 0x71450) adds 16× the acceleration to
  speed and 64× to vz, using one boost, when height < 40; the rocket key
  (0x7144c) fires 0x185d4 and uses one rocket.
- CONFIRMED: height: z += vz; gravity vz −= 0x4000 + |vz|/4 while
  vz > −20. Landing on the ground level of the cell under the racer: vx, vy
  ×(1−0.15), vz = −0.3·vz (sound above 2).
- CONFIRMED: the anim frame is heading + 0/32/64 (vz > 3, > 4 px) or
  96/128 (vz < −5, < −7): five poses of 32 headings.
- CONFIRMED: the player's waypoint index advances when within 120 px
  (|dx|+|dy|) of the next REDCAT waypoint.
- CONFIRMED: fuel or damage below 0 ends the race for the player
  (`[0x56078] = 1`).

## The race: map cells

- CONFIRMED: of a cell's 6 bytes, byte 5 is the ground level. A racer whose
  z is below level·1000.0 cannot enter the cell (`0x1889d`); walls are
  steps in level. The racer then slides: its heading is turned through
  0x721e0 / 0x725e0 / 0x729e0, depending on the side hit. On TRACK1A 74% of
  cells are level 2 (the desert above the canyon), the track is level 0.
- CONFIRMED: bytes 2-3 are a waypoint: byte 3 the racer (1 REDCAT, 2 BULL,
  4 MUIS), byte 2 its number; at most 150 per racer. Position: cell centre
  (x·32+16, y·40+20); MUIS waypoints use +16 vertically (log:
  "MUIS wayp.nr 0 found at (1040,256)").
- CONFIRMED: byte 4 is an object or surface type, checked under five points
  of each racer every frame (`0x18ae5` → `0x18b8d`, map.c; > 21 is an
  error). 11: the top speed moves toward 12 px/frame by 3 per check (off it,
  speeds above 12 lose 3); 12: a hazard below height 20: −10 damage, a bump
  upward, the cell cleared; 13: +1 boost, cell cleared; 14: a ramp, above
  4 px/frame vz += speed/10; 20: finish line, sets the racer's finished flag.
  At load, `find_bonus_objects` (0x15597) creates objects for 10, 12, 13
  (boosts), 15, 16 (cacti), 18 (finish sign), 19 (start sign) and stores the
  object number in byte 1.
- CONFIRMED: racer–racer contact (`0x1404a`): within 60 height units and
  300 px, four corner tests 50 px along the heading; on contact (`0x14534`)
  headings merge through the table at 0x72de0 and both speeds become their
  average.
- CONFIRMED: the computer racers' top speed comes from 0x57b18 indexed by the
  map number (13, 13, 14, 14, 14, 15, 15, 15 px/frame for maps 0–7).

## The race: speed

- CONFIRMED (experiment): the race loop is not tied to vsync or a timer.
  Counting `objects_tick`'s counter [0x600c4] against the BIOS tick at 0x46c
  over 5 s: 29.4 frames/s at DOSBox `cycles=fixed 40000`, 105 frames/s at
  120000. All movement is per frame (`player_tick`), so the original game ran
  faster on a faster PC. (The `out 0x43` code at 0x26b65 drives the PC
  speaker, channel 2, not the frame clock.)
