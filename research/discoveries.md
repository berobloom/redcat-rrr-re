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
  (— = entry 0). The row is the world (RRRIO +0x2c), not the groep; see
  "The session".

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

- CONFIRMED (formulas checked against run-time dumps, 0 mismatches): the
  tables built at start-up:
  - cos/sin 0x73de0/0x73e60 (`0x13c34`): a = −(h+8)·π/16,
    cos[h] = (int)(cos a·65536), sin[h] = (int)(sin a·65536), truncated.
  - turn table 0x71de0 (`0x13f00`), 32×32, [target][current]: current if
    equal, else one step toward target the short way (+1 when exactly 16
    apart). Its rows 0/8/16/24 are the "wall slide" tables 0x71de0, 0x721e0,
    0x725e0, 0x729e0: one step toward up/left/down/right.
  - merge table 0x72de0 (`0x13f91`): the same, one step away from target.
  - direction table 0x73ee0 (`0x13cba`), 33×33, [r][c] =
    (56 − trunc(atan2(c−16, r−16)·16/π + 16.5)) & 31.
    `direction_to_waypoint` (0x13d60): with dx, dy = racer − waypoint
    (pixels, float), d = √(dx²+dy²) (0 → heading 0), r = trunc((dx/d+1)·16)
    % 33, c = trunc((dy/d+1)·16) % 33.

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

## The race: computer racers (bull_tick 0x12826, muis_tick 0x130f8)

- CONFIRMED: Brutus and Snitch run the player's physics (same acceleration
  30/mass, friction, gravity, landing, walls, cells, anim poses) with
  steering replaced. The two functions are the same code with separate
  globals: waypoint index/count [0x71920]/[0x7191c] and list 0x71468 (8-byte
  x,y pairs) for Brutus, [0x71dd8]/[0x71dd4] and 0x71924 for Snitch; flags
  0x55fec..0x56008 vs 0x56010..0x5602c; boost below 0x1c0000 vs 0x1a0000.
- CONFIRMED: they always accelerate. Each frame the next waypoint counts as
  reached within 40 px (|dx|+|dy|; the player uses 120), and the heading
  steps once through the turn table 0x71de0 toward `direction_to_waypoint`
  (the player steps twice).
- CONFIRMED: stuck check `0x1467c`: with vx = vy = 0 for 21 frames the
  racer turns to a random heading (heading + 8 + rand()%40, mod 32) and skips
  steering that frame.
- CONFIRMED: on reaching a waypoint `0x1af83` looks the waypoint up in a
  per-racer script (0x57c90 Brutus, 0x5bd40 Snitch; 24-byte entries
  {waypoint, action, …}, 30 per block, block [0x751a4]; actions 1 rocket
  aimed by `0x13e3d`, 2 drop, 3 boost, 4 wait). Both tables are empty in
  the EXE (each block starts with −1) and stay empty at run time (read at
  0x20dc90 during the TRACK1A race), so only the fallback runs:
  rand()%100 < 10 → an object from `0x18759` (kind 0 or 4, half each; needs
  stats +0x20 ≠ 0 and damage ≥ 0); otherwise rand()%100 < 10 → a boost
  (speed and vz +8 px/frame, one boost used). Computer racers never fire
  rockets.
- CONFIRMED: `0x18759` spawns an object with anim 16 in the racer's heading,
  25 px ahead, height 40, speed 25 px/frame, lifetime 150 frames (+0x24);
  callback 0x17b61 (kind 0), 0x178f6 (kind 2) or 0x17c74 (others).
- CONFIRMED: damage or fuel below 0 sets [0x56080] (Brutus) / [0x5607c]
  (Snitch).

## The race: start, crashes, objects

- CONFIRMED (`init_objects` 0x148db): slot i of the start order (RRRIO.INF
  0x1fc) stands on waypoint 0 of list i (REDCAT, BULL, MUIS lists); every
  racer then heads for its own waypoint 1. RedCat drives anim 2, Brutus
  anim 0, Snitch anim 1; shadows anim 4. RedCat: mass 60, fuel 75 (from
  RRRIO), damage 100, top 16. Brutus: mass 90, Snitch: mass 60; both fuel
  100, damage 100, 3 boosts, top speed from 0x57b18.
- CONFIRMED: a racer whose damage or fuel drops below 0 is out
  ([0x56078]/[0x56080]/[0x5607c]). Next frame `switch_anims` (0x15084,
  one racer per frame) calls `start_crash` (0x14f3e): top speed −2 px/frame,
  frame 0, and a crash callback for the rest of the race. A computer
  racer's damage is set back to 100. RedCat (`player_crash_tick` 0x19497)
  drives as a wreck with anim heading + 22 (8 frames), acceleration
  65/mass, friction 0.9 (stop below 2 px/frame), boosts free of charge.
  Brutus/Snitch (`bull_crash_tick` 0x19c8c, `muis_crash_tick` 0x1a42a) keep
  their AI without the out check, anim heading/2 + 54 or + 70. The other
  `switch_anims` helpers only page crash images in and out.
- CONFIRMED (experiment): on TRACK1B (RRRIO.INF stage 1) Snitch drives into
  a corridor of type-12 cells, reaches damage −10 at frame 141, crashes and
  goes on with damage 60 and top speed 8; Brutus ends at damage −221. The
  port's trace matches the original's samples within about one frame up to
  frame 890 and both finish in the same 80-frame sample windows.
- CONFIRMED: map objects (`find_bonus_objects`) sit at the cell centre:
  type 10 anim 5 (life rand&255 + 100), 12 mine anim 6 (life 10, counted
  down by `mine_tick` 0x16b53, drawn under everything), 13 boost anim 8,
  15/16 cactus anim 12 at +16, −10 px (life −1; a hit sets 5, counted down,
  then removed), 19 start sign anim 10, 18 finish sign anim 11. The anim
  table is static data at 0x57798 (87 entries; identical to the run-time
  capture).
- CONFIRMED: sprite hotspots are the RC.IMG headers' x, y as signed 16-bit
  values (544 of 544 loaded images match the run-time records).
- CONFIRMED: the race palette is the tileset's (identical to RACESCR3.PCX's;
  all 169 colours of a race screenshot are in it). `race_init` loads
  wacht.pcx's palette first, for the loading screen.
- CONFIRMED: start lights: anim 86's frames (images 32, 43–46) drawn at
  screen (226, 50) for 1500, 1000, 1000, 1000, 500 ms (the port's start
  frame matches an original screenshot pixel for pixel).
- CONFIRMED: draw order (`sort_draw_order` 0x11b0e): descending key
  (1000 − x)·10 − 1000·y − z − prio, with prio −100000 for shadows and
  mines.

## The race: drawing (draw_view 0x11526, new2io.c)

- CONFIRMED: the view is drawn into an off-screen buffer and copied to the
  screen rectangle x 20..620, y 0..400 (inclusive bounds, from `0x26ef4`'s
  arguments; the screenshot's map pixels end at those columns and rows).
  `draw_view` also has 540×360 at (50,20) and 480×320 at (80,40) for
  [0x55fb8] = 1, 2, but `race_init` sets it to 0 and nothing else writes it.
- CONFIRMED: sprites are drawn in the order of the sort list (0x6fc70,
  0x6fc74 = next) with their top-left at the object's (x, y) minus the
  scroll. Kinds (+8 of a sprite slot): 1, 2 opaque with 0xFF transparent
  (`0x11bb4`); 4, 5, 6 translucent (`0x17d94`): dst = trans[dst·256 + src].
  Other kinds are an error ("unknown type").
- CONFIRMED: the trans table: 255 rows of 255 bytes from TRANS.DAT
  (`0x11e2a`) at row·256 + column, column 255 = row. When the file is
  missing `0x11ca2` builds it: entry (a, b) is the palette colour nearest to
  the average of colours a and b, (a, a) = a; then `0x11db8` saves it. So
  type 4–6 sprites are 50% blends (shadows, smoke).
- CONFIRMED: a racer's sprite position is x − hotspot_x − 16, y − z −
  hotspot_y (image record +8, +10), from `bull_tick` / `player_tick`.

## The race: HUD and camera

- CONFIRMED (`hud_update` 0x1918d; port start frame and boosted frame match
  the original's layout): the gauges are anim frames drawn by `0x2425d` →
  `0x32c78` (screen coordinates, 0xFF transparent) over RACESCR3.PCX:
  speedometer anim 19, frame |((speed·11) >> 16) / 20| (max 10) at
  (20, 414); fuel anim 20, frame (fuel >> 16) / 16 (max 8) at (110, 422);
  damage anim 21, frame damage / 10 (max 9) at (217, 438); boosts and rockets
  as two anim-18 digits (tens at x, ones at x + 16, y 415), x 389 for boosts
  and 494 for rockets.
- CONFIRMED: the camera starts on the player (0x1475f, after
  `init_objects`): scroll = (x − 304, y − 200), clamped to 0 ..
  (map size − view − 1 tile). `race_init` sets the view to 19 × 10 tiles
  ([0x60044], [0x60048]); `camera_follow` (0x138ed) eases 1/16 per frame
  toward x − 320 + 128·sx, y − z − 220 + 160·sy.

## The race: speed

- CONFIRMED (experiment): the race loop is not tied to vsync or a timer.
  Counting `objects_tick`'s counter [0x600c4] against the BIOS tick at 0x46c
  over 5 s: 29.4 frames/s at DOSBox `cycles=fixed 40000`, 105 frames/s at
  120000. All movement is per frame (`player_tick`), so the original game ran
  faster on a faster PC. (The `out 0x43` code at 0x26b65 drives the PC
  speaker, channel 2, not the frame clock.)

## The session: worlds, stages, modules

- CONFIRMED (REKEN 0x10ac9 / 0x198c7, RRR 0x1abb6 / 0x1585e; RRRIO.INF
  before and after a REKEN run): REKEN and RRR share RRRIO.INF. REKEN reads
  damage (+0x04), boosts (+0x0c), rockets (+0x14), fuel (+0x20), groep
  (+0x28), stage (+0x24), world (+0x2c) and +0x30 (copied to [0x46c87]; the
  prize/sound code checks it, 3 by default), and writes the first four back
  in `0x199cf`. RRR's race end `0x1585e` writes fuel, boosts, rockets, damage
  (and stats +0x10, +0x14) back, sets +0x1f8 = 1, stores the finishing order
  at +0x1fc and adds 3, 2, 1 points to +0x214[racer] for places 1, 2, 3.
- CONFIRMED: +0x28 is the groep (3–8). REKEN maps it to a level byte
  [0x46c84]: groep 3, 4, 5 → 3, 4, 5 and groep 6, 7, 8 → 0, 1, 2 (and back
  in `0x199cf`); the screen shows "Groep: 6" for +0x28 = 6. Command-line
  `G3`..`G8` override it, `M1`..`M4` force a module.
- STRONGLY SUPPORTED: +0x2c is the world, not "groep − 3": six rows in the
  stage tables match six world names (REKEN 0x44fe8, RRR 0x57c18: De
  Welkomstrace, De Rotsvlakte, De Snelweg, De Puzzelkloof, De Cactusvallei,
  De Laatste Kans), and row 0 holds the track1 maps.
- CONFIRMED: per world (REKEN 0x45000 = RRR 0x57c30, rows of 4) the enabled
  exercise modules 1–4 (Handig rekenen, Wat is waar?, Memory, Grote
  getallenspel; names at 0x44fd4):

  | world | modules | maps by stage 0..4 (0x44f5c = RRR 0x57b8c) |
  |---|---|---|
  | 0 | 1, 3 | 1, 2, —, 3, — |
  | 1 | 2, 3 | 4, —, 5, 6, — |
  | 2 | 1, 2, 3 | 7, 8, 9, 10, — |
  | 3 | 1, 3, 4 | 11, 12, —, 13, 14 |
  | 4 | 2, 3, 4 | 15, —, 16, 17, 18 |
  | 5 | 1, 2, 3, 4 | 19, 20, 21, 22, 23 |

- CONFIRMED (`0x10cad` in REKEN, the same code at RRR 0x1ad9a): the next
  module is the first enabled one after the stage, taken only when the stage
  is 0 or +0x1f8 (race done) is set; REKEN stores it as the new stage
  (run: stage 0 → 1 after Handig rekenen). Module 4 with groep 3 becomes
  module 3. So stage s's race follows module s: a world is the welcome race
  (stage 0), then per enabled module the exercises and the race in that
  column. Map column and module column line up (zero map ⇔ module off).

## The arithmetic part (REKEN.EXE)

- CONFIRMED (screenshots): REKEN shows RedCat, a fuel counter, the repair
  bar, the prize in a white box, boosts and rockets, Naam / Groep / "Aantal
  sommen" (sums left, from 8), a keypad 0–9, TERUG and OK, Stop and Help.
  Keys 0–9, Backspace and Enter work too (a key must be held ~0.2 s under
  the harness).
- CONFIRMED: module 1 "Handig rekenen" (`0x11ea2`) asks 8 sums. Each sum
  picks a generator from `0x4539c[level]` (4 slots, −1 = unused; levels 0–2
  and 5 use all four, level 3 {2, 3}, level 4 {0, 2, 3}): 0 `0x11496` ×,
  1 `0x11619` :, 2 `0x1122e` + (two or four terms), 3 `0x1179e` −. The
  text goes to 0x5371c, the answer to [0x537e4].
- CONFIRMED (port `src/reken.c` = the original code under Unicorn for 500
  seeds × 4 generators × 6 levels and 8 prize states, `check_reken.py`): the
  generators, with `0x111e0` = lo + rand() % (hi − lo):
  - + (`0x1122e`, rows of 9 bytes {lo, hi, n} at 0x452b0, row 2L and
    2L + 1): levels 3–5 a + b. Levels 0–2, rand() % 2 = 0: two round numbers
    v = multiplier (0x451ae {5, 10, 15}, first n) × between(lo, hi), each
    split as (v − d, d) with d = rand() % (v − 5) + 2, the four parts in one
    of four orders ("handig": 3 + 26 + 14 + 87 = 90 + 40); otherwise one
    round number split in two.
  - × (`0x11496`, 18-byte rows at 0x4531c): level 0 a small factor (0x45388,
    20 entries) × a round number; levels 1–2 (a1 + a2) × round number;
    levels 4–5 a × b; level 3 none.
  - : (`0x11619`, 17-byte rows at 0x4524a): the quotient and divisor are
    drawn and the dividend is their product (level 0 quotient a × {5, 10},
    levels 1–2 divisor × {10, 5, 10}); level 3 none.
  - − (`0x1179e`, 18-byte rows at 0x451de): round numbers at levels 0–2
    (levels 1–2 off by a small ± amount), plain at 3–5; the larger always
    first, equal values nudged apart.
- CONFIRMED (`0x11cfd`, run): per sum, `0x18e7f` picks a prize and shows it;
  answer −2 is Help (`0x11a6a`, an example); a correct answer runs the award
  `0x1910f` and counts [0x537e8], a wrong one `0x19471` (no prize). After 8
  sums `0x19f07` appends to the pupil's `NNNNNNNN.SMD` and REKEN exits.
  Run (groep 6): 8 sums, 7 correct, fuel 75 → 120, rockets 10 → 20, each
  award matching the shown prize.
- CONFIRMED (`0x18e7f`, `0x1910f`): prizes [0x46c86]:

  | prize | award | shown as |
  |---|---|---|
  | 0 | boosts +5 (max 99) | |
  | 1 | rockets +5 (max 99) | Mini-Raket |
  | 2 | repair to 100 | |
  | 3 | repair +30 (max 100) | |
  | 4, 5, 6 | fuel +5, +10, +15 (max 150), one unit per 200 ms | Benzine 5 / 10 / 15 |

  Choice: index rand() % 16 into 0x46c8c {5,6,5,1,4,0,6,1,6,4,5,6,0,1,1,1}
  when repair is 100, or into 0x46cac {3,6,3,5,3,6,6,6,1,3,5,3,3,5,6,3}
  when fuel < 60 or repair < 100 (the second draw wins when both hold);
  repair 0 forces 2; a repair prize at repair 100 becomes rand() % 2;
  rockets at 99 turn 1 into 0, boosts at 99 turn 0 into 1.
- CONFIRMED: answer ranges per level `0x45480`: (15, 50), (15, 100),
  (15, 150) for levels 0–2 (groep 6–8), (2, 9) for levels 3–5.
- CONFIRMED (`0x19cfb`, `0x19f07`, the file after the run): the pupil's
  `%8.8d.SMD` (layout in file-formats.md "Pupil files") gets a record per
  module: module − 1 ([0x46c85]), asked, right, the date, the groep
  (+0x28). The run's file: name empty (REKEN /DAVILEX), 1 record, module 0,
  8 asked, 7 right, "29/09/2026" on 29 October (CONFIRMED by `0x19cb6`:
  `tm_mon` is printed without + 1), groep 6. Asked/right per module: 1:
  8, [0x537e8]; 2: 8, [0x53c78]; 3: [0x5bcf0], [0x5bcec] (pairs tried,
  found); 4: 8, [0x5c094].
- STRONGLY SUPPORTED (code; the read side checked in DOSBox): records are
  read back at a 0x5b stride but appended as 27 bytes. Each append rewrites
  the earlier records at the full stride, so the file stays readable: all
  records but the last take 0x5b bytes (the extra 64 hold stale buffer
  bytes), the last 27. A crafted file with 27-byte records throughout is
  misread by Rapport: only records at multiples of 0x5b count.
- CONFIRMED (`0x21bc7`; all four font files decode with it, glyphs rendered
  and legible): the ASCII font format in file-formats.md, 256 flags, w, h,
  fg, bk and 256 w×h glyphs.

## The arithmetic part: screen

- CONFIRMED (port render vs the original's screenshots: identical except
  the mouse pointer, for the first sum and while typing): module 1 draws
  mod_124.pcx (its palette is the screen's), then MENU.IMG images through
  the anim table 0x4660c (59 anims; image i = MENU entry i − 1).
  `0x10dd8(anim, frame, x, y)` puts an image's top-left at (x, y);
  `0x10f78` puts its hotspot at (x + 1, y) (it increments x) after restoring
  the old frame's area. Positions (modules 1, 2, 4; Memory has its own in
  each record's first two words):
  - face: anim 13 frame 0 at centre (51, 83); after an answer anim 13
    (right, record 0x4680c) or 15 (wrong, 0x467f0) plays 16 frames;
  - gauges (`0x18d8a`): repair anim 4 frame repair / 10 − 1 (0..9) at
    (114, 48); boosts and rockets anim 1 digits at (385, 79) / (385, 125)
    and x + 17; fuel anim 3 digits at (10, 125), +22, +44;
  - prize: 20 frames of anim 45 (rockets), 47 (boosts), 49 (repair), 51
    (full repair), 53 / 55 / 57 (fuel 5 / 10 / 15) centred at (230, 100),
    10 ms apart; the label is part of the image;
  - title bar anim 43 frame module − 1 at (183, 10); keypad anims 33–41
    (keys 1–9) and 42 (0) at the positions of `0x110e2`; TERUG anim 19 at
    (502, 368), OK anim 18 at (502, 424), Help anim 17 at (564, 3); frame 1
    is the pressed look, held 150 ms;
  - sum panel anim 32 at (8, 168), 624 × 168; the sum in ASCII30 centred in
    it with one '?' per answer digit appended, the '?' cells becoming the
    answer field: typed digits and blanks on colour 0x36, the next cell '_'
    on 0x90 (`0x17f7b`, `0x17f81` in module1_init);
  - ASCII12 text: Groep at (520, 105), Naam at (520, 76), sums left
    ("%d " on black) at (576, 134).
- CONFIRMED (`0x1847e`): keys S (Stop, −1), H (Help, −2), Enter or O
  (OK; ignored while the field is empty), digit keys and keypad.

## The arithmetic part: module 2 "Wat is waar?"

- CONFIRMED (screenshots, groep 6 and 3): three rows (anims 28, 29, 30,
  363 × 41, labels 1–3 in the image), each a sum; one of them is true. For
  groep 6–8 each row reads "a op b = R" with the same R; for groep 3–5 the
  rows hold the bare sums and R stands in the Uitkomst box (anim 22 at
  (452, 226), drawn by `0x195e3`). Keys 1, 2, 3 or a click on a row answer;
  S, H and the Stop / Help buttons as in module 1. No keypad.
- CONFIRMED (port = original under Unicorn, `check_reken.py`; see below for
  the faulting seeds): `0x13630` runs 8 rounds of `0x12844` (generate) and
  `0x13229` (ask), then `0x19f07` with module [0x46c85] = 2. `0x12844(L)`:
  three draws of `0x1202a` (= lo + rand() % (hi − lo)) over the level's
  result range `0x45480` into [0x53bd4], [0x53bd8] (overwritten later) and
  R = [0x53be4]; a row of `0x454b0[L][rand() % 5]` (3 type bytes,
  0 × `0x12288`, 1 : `0x124ec`, 2 + `0x12068`, 3 − `0x12614`):

  | level | rows |
  |---|---|
  | 0 | 012 103 310 213 203 |
  | 1, 2, 5 | 012 103 310 213 302 |
  | 3 | 232 323 232 332 322 |
  | 4 | 023 203 302 203 302 |

  Choice i's text goes to 0x537ec + 100 i, its value to the int array at
  0x53be4. Element 0 of that array is R itself, so choice 0 overwrites R
  with its own value and choices 1 and 2 are made around that: choice 0 is
  the true sum, and R as shown is its value.
- CONFIRMED (same check): the generators, R = [0x53be4], levels 3–5 / 0–2:
  - + : a = rand() % R + 1, b = |R − a + rand() % 3 + 1| (a + b is R + 1 to
    R + 3) / a, b = |R/2 − (rand() % 4 + 1)| each.
  - × : `0x121d9`: up to 4000 pairs from the result range until
    0 < |a·b − R| ≤ 1, else last a + 1, last b / a = rand() % 20 + 1,
    b = R / a, then b ± (rand() % 4 + 1) (+ when b < 5).
  - : : `0x123cf`: quotient span [max(R − 2, lo), min(R + 2, hi)] (unsigned
    compares), q = span lo + rand() % (hi − lo) redrawn while q = R,
    divisor from the result range, accepted when q·d is in the range; 4000
    tries, else 8 : 2 (9 : 3 when R = 4) / b = rand() % 6 + 1,
    a = |R − (rand() % 4 + 1)| · b.
  - − : a = rand() % R + 1, b = |rand() % 3 + R − a + 1|, a = b → a + 1,
    and for choices 1, 2 a difference equal to R is nudged by one / a =
    |R − (rand() % 4 + 1)| + b with b = rand() % 100 + 20, equal → a − 5.
    The larger operand goes first.
  So the true row is built on one draw and the others miss its value by a
  little. Each generator keeps choices 1 and 2 off R (by construction, or
  the × / : retry tests); only ×'s fallback (last a + 1) · b is unchecked
  (LIKELY rare: 4000 failed pairs).
- STRONGLY SUPPORTED (Unicorn faults in `0x123cf` at level 5 for some
  seeds): when the span above is empty (R = 11: max(9, 2) = min(13, 9)),
  `rand() % 0` divides by zero. R = 11 happens when choice 0 is a + at
  level 5 (R + 3 with R = 8) and choice 1 is a division (row 213). Whether
  DOS/4GW then aborts REKEN was not tried. The port takes the 8 : 2
  fallback there. A span below the range (R > hi + 2) still draws (idiv
  keeps the remainder positive) and ends in the fallback.
- CONFIRMED (`0x13229`, screenshots): per round: "Aantal sommen" (8 down to
  1), the prize (`0x18e7f`), the rows open (`0x1294b`: frames 5 → 0, 5 ms
  apart; frame 5 is white, 0 black), the true row r = rand() % 3
  ([0x53c80]): row r shows choice 0, row 0 choice r, the third row its own;
  then the texts (ASCII30, centred in 342 × 41 from (x + 22, 208 / 297 /
  385)) and for groep 3–5 R centred in 132 × 144 from (452, 267). The rows
  are centred at (x + 182, 227 / 316 / 404), x = 61 for groep 3–5, else
  131. After a key the two other rows close (`0x12b59`, frames 0 → 5 then
  the background back; frame 0 first hides their text), the award or no
  prize as in module 1, 1 s, and the true row closes too (`0x129c6`).
- CONFIRMED (`0x24741` tables, 22-byte records {1, 0, 5, scan code, 1, x0,
  y0, x1, y1, 0, 0}, ended by 0): mouse buttons per module stand for keys:
  module 1 `0x45060` (keypad, Help (564, 3)–(632, 33), TERUG, OK, Stop
  (10, 6)–(78, 35)), module 2 `0x453fc` (rows (x, 207)–(x + 342, 248),
  +89, +177; Help; Stop (10, 4)–(78, 33)), module 3 `0x4550c`, module 4
  `0x457d4`.

## The arithmetic part: module 3 "Memory"

- CONFIRMED (screenshots, groep 6): module3.pcx with a grid of cards,
  green SOM (Brutus) and yellow ANTWOORD (Snitch); face-up they are black
  with white ASCII20 text, the sum or its value. The face, fuel, prize,
  repair, boosts and rockets stand in a column on the right; no "Aantal
  sommen". Cards are taken by mouse (the whole grid is one button, scan
  code 57, the card read from the pointer; LIKELY Space does the same
  wherever the pointer is, not tried).
- CONFIRMED (port = original under Unicorn for 100 seeds per level,
  `check_reken.py`): the grid per level `0x456e4` {cols, rows, x, y, w, h,
  gap x, gap y}: groep 6 and 5 4 × 5 at (36, 70), groep 7–8 5 × 6 at
  (5, 55) with 2-pixel gaps, groep 3 3 × 4 at (94, 102), groep 4 4 × 4 at
  (36, 102); cards 102 × 67, gaps 12 or 14 / 12. `0x14d5a` makes
  ⌈pairs / n⌉ sums of each of the n types in `0x45684[level]` (the
  `0x4539c` list again), in type order, the extra ones unused:
  + `0x137fa` and − `0x13945` from {10, 30} (groep 6–8) or {1, 5},
  {1, 10}, {1, 20} (groep 3, 4, 5) (− nudges equal operands apart and puts
  the larger first); × `0x13869` a ∈ [1, 10) by b ∈ [10, 20) (groep 6–8),
  [1, 10) (3–4), [1, 20) (5); : `0x138ce` divisor d ∈ [1, 6) or [1, 10)
  (3–4), quotient q ∈ [2, 10), [2, 5), [2, 8), shown as q·d : d. Then
  `0x14378` deals pair p to two cells drawn one at a time (rand() % cells
  left, the list closing up, `0x1434c`): first the SOM, then the ANTWOORD.
  Cards are 58-byte records at 0x58f98: text, value +0x32, kind +0x36
  (0 SOM, 1 ANTWOORD, −1 gone).
- CONFIRMED (`0x14967`): a prize is chosen at the start and after each
  found pair while cards are left. The first card must be a SOM, the
  second an ANTWOORD; a card of the wrong kind shakes (anim 6 / 11, frames
  0–4, 5 ms). A turned card plays anim 5 (SOM) / 10 (ANTWOORD) frames 0–6
  then gets its text, centred, one pixel low. With two up the try counts
  ([0x5bcf0]); equal values (any SOM with any ANTWOORD of that value) count
  as found ([0x5bcec]): the award, then both play anim 7 (8 frames, 5 ms)
  and leave. Otherwise no prize, 1 s, and they turn back (anims 8 / 9, 4
  frames, 40 ms). The module ends when the grid is empty, or on Enter
  (scan 0x1c, keeping what was won); the SMD record gets the tries and
  the pairs found.
- CONFIRMED (records' first two words, `0x18c17`.., port render identical
  to the screenshot outside the prize and the cards): module 3 positions:
  face anims 12 / 14 at (577, 90); prize anims 44–56 (each record's
  module-3 anim, one below the others') at (568, 224); repair anim 4 at
  (612, 188); boosts and rockets anim 1 at (600, 280) / (600, 316), +17;
  fuel anim 2 digits at (539, 149), (560, 148), (582, 147); title anim 43
  frame 2; Groep (ASCII12) at y = 420 centred in 98 pixels from x = 537
  as if one character, Naam at y = 370.
- CONFIRMED (screenshots during Help): Help (`0x147a9`) saves the grid,
  deals a demo (SOM and ANTWOORD alternating, "2 + 3" = 5, "4 + 4" = 8),
  turns cards to the spoken explanation with the title "Help!", and puts
  the grid back. Not ported.

## The arithmetic part: module 4 "Grote getallenspel"

- CONFIRMED (`0x17c57`, port = original under Unicorn for 100 seeds per
  level, `check_reken.py`): 8 rounds. Each round picks row rand() % 4 of
  `0x45908[L]` (L = level, 4 for level 3) and calls its generator, drawn
  again while the target is above 50: × `0x151c6`, : `0x1530f`, +
  `0x154bf`, − `0x155ed`, with number tables `0x45950`, factors
  `0x45a40`, dividends `0x45b30` (values in `port/src/reken.c`). The
  numbers are then rotated left by rand() % count. count is 3 for levels
  4 and 5 (groep 4, 5), else 4 (`0x17e18`, [0x45c30]); with 3 the fourth
  slot holds −1 and stays empty.
- CONFIRMED (`0x165fb`): a number is taken only as the first operand or
  after an operator; an operator only after a first operand (a second one
  replaces it). ':' needs b ≠ 0 and a % b = 0, '−' needs a ≥ b; else
  refused. '=' with all three pushes a history step, writes
  "%d op %d = %d" (ASCII12) as the next line of the panel (y = 178 + 20 k,
  centred in 280 from x = 224) and puts the result into a's slot, b's
  slot emptied. The round ends when one number is left: equal to the
  target gives the prize, otherwise none. TERUG undoes b, the operator, a,
  or with nothing entered the last sum ([0x46c87] ≠ 0). OPNIEUW restores
  the numbers and clears the four line texts.
- CONFIRMED (DOSBox, sum after OPNIEUW drawn at y = 198): OPNIEUW does
  not reset the line and step counters. Past 4 lines nothing is shown and
  the writes run beyond 0x5bfc4 into 0x5c090.. (operands, [0x5c094],
  [0x5c098]); the target survived 6 rounds of this. The port keeps the
  counters but bounds the arrays.
- CONFIRMED (`0x17b95`, `0x18*` SMD writer): [0x5c094], the count module
  4 hands the SMD record, is never incremented: module 4 records 0 right.
  In the draw `0x161d3` a recount of [0x5c098] makes the branch that
  shows a single number in the panel unreachable; `0x15c5f`, an
  alternative screen selected by [0x45c34], is dead ([0x45c34] = 0).
- CONFIRMED (screenshots, port pixel-identical for slots, entry and
  lines): slots anim 21 at (8, 168 + 48 i), numbers ASCII30 centred in
  182 × 41 from (30, 169 + 48 i); target box anim 22 at (502, 168),
  number centred in 132 × 144 from (502, 209); panel anim 23 at
  (224, 168), 280 × 185, the entry centred; + anim 24 (8, 368), × 25
  (78, 368), − 26 (8, 425), : 27 (78, 425), = anim 16 at (78 or 152, 368),
  TERUG 19 (503, 368), OPNIEUW 20 (503, 424). Groep 3–5 have no × and :
  buttons (= moves to x 78). Keys: × scan 0x49 / 0x2d, : 0x48, + 0x4e,
  − 0x4a, = 0x1c / 0x0d, TERUG 0x0e / 0x14, OPNIEUW 0x18.
- Help (`0x171ea`) not read or ported.

## The round: MOTOR, RRRMENU, REKEN, RRR

- CONFIRMED (MOTOR.EXE, 16-bit, DGROUP 0x40d; dispatcher at file image
  0x2ff, loop 0x48f): MOTOR starts code 42 and then runs whatever the
  last program's exit code names: 40 `rrr.exe`, 42 `rrrmenu.exe`, 43
  `reken.exe`; 255 and 101 end the game, 100 is returned as is.
- CONFIRMED (RRRMENU 0x10865): after the menu loop (`0x11bf7`) returns
  "play", the menu exits 43 (REKEN) if RRRIO +0x1f8 (race_done) is set,
  else 40 (RRR); otherwise 255. Start! cleared race_done in both runs
  below (`0x1212c`, under a flag not traced), so it exited 40 both times:
  the menu hands over to the race of the current stage. Choosing another
  world in the menu sets stage 0 and calls `0x1099b` (`0x120de`).
- CONFIRMED (RRRMENU `0x10f95`; DOSBox: RRRIO world 0, stage 3,
  race_done 1, repair 40 → the menu shows "2: De Rotsvlakte" and saved
  world 1, stage 0, repair 100, race_done still 1): at start-up, when
  race_done is set and the world has no module after the stage
  (`0x125ca`), the menu moves to world (world + 1) % 6, stage 0, and
  `0x1099b` sets repair 100, the start order +0x1fc 0, 2, 4 and zeroes
  +0x208..+0x21c (waypoints and points). Fuel, boosts and rockets carry
  over. With world 0, stage 1, race_done 1 it stays (Handig rekenen shown
  dimmed, done) and Start! exits 40.
- CONFIRMED (REKEN main `0x10864`, from 0x10903): load the record, take
  the next module (`0x10d19` → `0x10cad` with take), level 3 (groep 3)
  plays module 4 as 3 (the stage stays 4); M1..M4 override it; no module
  (−1) exits 42 (menu, `0x10a7a`). After the module `0x199cf` saves and
  REKEN exits 40 (RRR). REKEN does not touch race_done on a normal load
  (`0x10c29` is the defaults when the file is missing).
- CONFIRMED (RRR 0x11380..0x11500): Esc (scan 1) or S (0x1f) during the
  race calls `0x1585e(1)`, which writes nothing, and exits 42 (menu).
  A finished race calls `0x1585e(0)`: the player's stats record (0x6fbc0
  + 0x30 k) goes into RRRIO: fuel (+8, 16.16, clamped at 0) to +0x20,
  boosts +0x0c, +0x10 to +0x18, +0x14 to +0x1c, rockets (+0x1c) to
  +0x14, damage (+0x28, clamped at 0) to +0x04; the three racers' next
  waypoint ([0x71444], [0x71920], [0x71dd8]) to +0x208..+0x210; the
  finishing order (object numbers) to +0x1fc; race_done = 1; 3, 2, 1
  points to +0x214 + 4 (object / 2) for places 1–3; then saves. Then if
  `0x1ae67` (in effect stage = 4) `0x15a14(2 × points leader)`, else
  `0x15a9f`, and exit 43 (REKEN).
- So a world is: its stage-0 race, then per enabled module REKEN and the
  race of that stage; after the last REKEN finds no module and goes to
  the menu, which moves on to the next world, whose stage-0 race the
  pupil starts with Start!.

## The race: podium

- CONFIRMED (`0x15a9f`; DOSBox: finished flags of RedCat and Brutus poked
  during `RRR /DAVILEX /DEBUG`, then the 640 × 480 screen buffer
  [0x7f954] dumped during the 4 s wait; port `podium.c` draws it
  pixel-identical): after `race_end` (and `0x1b6bf`, which logs "Save score
  3 for" / "New place = 9", the RRRHIGH.DAT table, see "High scores") the podium
  loads `xyz.pcx` from RCS for the first two places (r RedCat, b Brutus,
  s Snitch: rbs, rsb, brs, bsr, srb, sbr) with its palette (flags 0xf:
  picture, palette, shown), then in Ascii20 recoloured fg → 0xd0, bk
  0xff clear, 201 kept as shadow: "Punten Totaal voor %s" (world name of
  `0x57c18`, padded to 16) at (150, 20), and at x = 200, y = 50 / 70 / 90
  "%-15s    :    %3d" (the name at RRRIO +0x68, empty under /DAVILEX;
  RRRMENU puts "RedCat" there when it is empty, `0x10ed1`), "Brutus de
  Bulldog  :    %3d", "Max de Muis        :    %3d" with the points
  +0x214.. Then `delay_ms(4000)`. The race loop stops at the finish
  itself (no run-out).
- Observed, not explained: in DOSBox-X the screen kept the last race frame
  during those 4 s, though the buffer held the podium and `0x1f604`
  (present the whole screen) runs for flag 8.
- CONFIRMED (strings): after a stage-4 race `0x15a14(2 × points leader)`
  plays `wredcat.smk`, `wbrutus.smk` or `wsnitch.smk` instead (Smacker,
  in RCS). "Max de Muis" is the podium's name for Snitch.

## The menu (RRRMENU.EXE)

- CONFIRMED (DOSBox: the screen buffer [0x5cf94] dumped with RRRIO
  defaults, after R, 4, G, and after R R 3; port `menu.c` renders each
  pixel-identical): menubs01.pcx (RRRMENU.IMG) with its palette, sprites
  from RRRANIM.IMG through the static anim table 0x457bc (9 anims; copied
  to [0x5e2f8] at run time, so `[0x5e2f8] + 8 a + 4` is anim a's frame
  list), blitted with their top-left at the given point, 0xff clear (the
  hotspot is not used). Start!'s RedCat is part of the picture.
- CONFIRMED: the parts. Groep buttons anim 0 (selected) / anim 1 frame
  groep − 3 at (342 + 50 i, 108). Bediening (`0x10d67`) anim 2 at (26,
  270): frame 11 (Brutus with the keyboard) without joystick, frame 0
  with. Geluid (`0x10db8`) at (176, 300): anim 7 frame 0 for sound mode 0
  (or no sound card), anim 6 for 1, anim 5 for 3 (Snitch with the
  guitar). The Race panel (`0x10c18`) in RRRFONT, opaque on black: "%d:"
  at (380, 205) and the world name (0x4589c) at (420, 205); the enabled
  modules (0x4558e, as REKEN's 0x45000) at x 400 from y 265 every 30,
  then blank lines up to y 360; groep 3 shows Memory for Grote
  getallenspel. Colours: `0x10a84` recolours the font from the current
  pair to one of 0x4556e {253, 254}, {249, 250}, {247, 248}, {251, 252};
  the header uses pair 0 (white), the modules pair 3 (dark yellow) until
  the next module, from which on pair 1 (yellow); when the world shown is
  not the record's, all modules are pair 1. The pointer is image 28 (the
  paw, anim 8).
- CONFIRMED (0x45244, set by `0x1f080`; read at run time): buttons turn a
  click into a key (`0x1f1e5`): Start! S (0x1f) (22, 105)–(133, 216), the
  Race panel R (0x13) (360, 162)–(632, 418), Rapport P (0x19)
  (181, 103)–(253, 224), Bediening B (0x30) (26, 268)–(157, 431), Geluid
  G (0x22) (177, 301)–(255, 425), Einde E (0x12) (561, 436)–(628, 464),
  groep 3..8 keys 4..9, 36 × 36 at (342 + 50 i, 108).
- CONFIRMED (`0x11bf7`): S or Enter plays anim 3 (RedCat drives off) at
  (22, 94) and leaves; E leaves without playing (then main shows the
  high scores, `0x119cc`, plays EINDANI2.SMK and exits 255); R shows the next world ((w + 1) % 6) without
  touching the record; 3..8 redraw the groep buttons and the panel; G
  cycles the sound mode 3 → 1 → 0 → 3 playing anim 5, 6 or 7 forward at
  (176, 300). Leaving (`0x120de`): a world other than the record's sets
  stage 0 and calls `0x1099b`; groep → +0x28, sound → +0x30 (so +0x30,
  REKEN's [0x46c87], is the sound mode), joystick → +0x34, the name →
  +0x68 ("RedCat" if empty); Start! clears race_done. DOSBox, after
  R R R 3 G S: world 3, stage 0, groep 3, sound 1, race_done 0, name
  RedCat.
- CONFIRMED (`0x11056`): play_anim(x, y, anim, step, n) blits frames 0..n−1
  (or back), each followed by a present and `0x1e88e(1)`, which waits n
  hundredths of a second (LIKELY, from its × 100 compare); world_flash
  draws the panel 20 times alternating (header yellow / modules white)
  with 10 hundredths between. The idle blink (anim 4 at (22, 94)) comes
  after 20000 + rand() % 10000 loop passes without the mouse moving
  (seen about 25 s in under DOSBox).
- On the first pass of the loop the groep variable is −1, so it takes
  the groep branch and draws the panel with its modules; `menu_init`
  itself draws them only after a finished race.

## The menu: Bediening

- CONFIRMED (`0x1118f`; DOSBox: B, then A D W S Space Ctrl, the buffer
  dumped before and after Enter, port pixel-identical; RRRIO after Start!):
  without a joystick ([0x5246c], [0x52470] zero; `0x11e42`) Bediening shows
  MENUKEY.PCX with its palette, hides the pointer and asks in RRRFONT pair
  0 at (201, 202) "Geef 1 voor 1 de toetsen voor", then per line
  (y = 222 + 20 k) "Linksaf:", "Rechtsaf:", "Vooruit:", "Achteruit:",
  "Turbo Boost:", "Raket:". `0x10add` waits for any key of the table
  0x453e2 (46 {scan, name}: letters, the cursor/keypad keys, Space, Enter,
  Shifts, Ctrl, Alt, Caps Lock, Tab) not chosen yet and prints its name in
  pair 1 at (321, y). Entry 0 is Esc: "Afgebroken!" at (201, 322), nothing
  kept. After the sixth, the keys go into RRRIO: +0x58 left, +0x5c right,
  +0x60 forward, +0x64 back, +0x50 Turbo Boost, +0x54 Raket, and "Druk op
  [Enter]" at (201, 342); Enter redraws the main screen and the panel
  (1, 1). DOSBox: +0x50.. = 0x39 0x1d 0x1e 0x20 0x11 0x1f.
- CONFIRMED (RRR 0x1b469 copies +0x50 to [0x71450], the boost binding, and
  +0x54 to [0x7144c], the rocket's): with the defaults Enter (0x1c) boosts
  and right Shift (0x36) fires a rocket. The port had them the other way.
- With a joystick (not ported) it shows MENUJOY.PCX and calibrates: centre,
  top left, bottom right, a rocket button and a boost button
  ("Joystick is ingesteld!"), into +0x38..+0x4c, +0x220 and +0x224.

## The menu: Rapport

- CONFIRMED (`0x12990`; DOSBox with crafted SMD files: three buffers, port
  pixel-identical but for the pointer row noted below): P (or a click on
  RedCat) shows rapport.pcx with its palette, the pointer stays. The file
  is looked up by the menu's name ([0x528f4], "RedCat" when the record's
  is empty): a file with an empty name is not found. In RRRFONT, pair 2
  (green) "Rapport van" at (38, 438), pair 0 the name at (156, 438). Per
  module i (0x45888 names; top y 111, 192, 274, 354 at 0x4591c; text x 36
  at 0x45914): the name in pair 2 at (36, y); without records "Nog niet
  gedaan" in pair 0 at (36, y + 25); else (`0x12673`) "Cijfer:%2d" in pair
  0 at (36, y + 25) and "Totaal:%2d" in pair 1 at (156, y + 25), with
  the graph (`0x12722`). Cijfer is the last record's right × 100 / asked,
  divided by 10 (`0x12e8d`); Totaal the module's records together: Σ right
  × 100 / Σ asked, / 10 (`0x12e12`, not an average of marks).
- CONFIRMED (`0x12722`): the graph from x 310 ([0x45918]), 300 wide: n =
  the module's records; step = 300 / n, lowered while > 1 and step × n ≥
  300. Ten horizontal lines at y + 48 − 5 k (colour 0x40 for k = 5, else
  0x10); when step > 1 a vertical line (0x10) from y + 48 to y at each
  310 + i × step, i = 0..n. Then from (310, y + 48) a polyline in 0xfd
  (white) through each record's score (right × 100 / asked) at x = 310 +
  i × step, height y + 48 − score / 2 clamped to ≥ y; then from (310,
  y + 48) one in 0xf9 (yellow) through the running Totaal of the first i
  records. Points past x 610 are skipped. The lines are the graphics
  library's `0x1f46e` moveto / `0x1f485` lineto (clipped), matching a
  plain Bresenham in every pixel compared.
- CONFIRMED (0x4592c, DOSBox): one button, T (0x14) on Terug (561, 436)–
  (628, 464); T reloads menubs01.pcx and calls menu_init(0, 1): the main
  screen and the panel as after Bediening (port pixel-identical).
- CONFIRMED (DOSBox): a file written by the port (8 records, then a second
  pupil's file 00000002.SMD) shows in the original's Rapport exactly as in
  the port's.
- Where the pointer stands, the original's save-under puts back the black
  it saved before the graph, so a graph line through the pointer's top
  row is missing there (18 pixels in the test). The port draws the pointer
  over the screen and does not copy this.

## High scores (RRRHIGH.DAT)

- CONFIRMED (RRR `0x1b6bf` called at 0x114aa after race_end, `0x1b82c`):
  after every finished race RRR loads RRRHIGH.DAT (layout in
  file-formats.md) and puts RedCat's points of the world so far
  ([0x7538c], RRRIO +0x214) under the record's name (+0x68) and world
  (+0x2c) before the first entry with a lower score (strictly), moving the
  rest down one and dropping the tenth; then writes the file ("r+b", else
  "wb"). Nothing is written when no entry is lower. So the same pupil
  enters again after each race of a world whose total still beats an
  entry. Names move by strcpy: a shorter name leaves the old one's tail
  in the file (the DOSBox file had "\0edCat", an empty name over RedCat).
- CONFIRMED (RRR `0x1b82c`, RRRMENU `0x136eb`; DOSBox without the file):
  with no file the table is made up: entry i has score 10 − i, the name
  of racer rand() % 3 (RedCat, Brutus, Snitch) and world rand() % 6; then
  entry 0's name is set to RedCat. The menu does not write it.
- CONFIRMED (RRRMENU `0x119cc`; DOSBox: the run's file, a port-written
  one; port pixel-identical): Einde, after the record is written, hides
  the pointer, shows highscor.pcx with its palette and `0x10a04` makes the
  font's background clear (colour 0 → 0xff, without updating [0x4556a]).
  Each entry is a line built in a 46-byte buffer of 40 spaces + "%3d  ":
  the name at column 2, the world name (0x4589c) at 21, the score through
  the "%3d". RRRFONT pair 1 at (51, 101 + 30 i), then pair 0 at (50,
  100 + 30 i). The pointer comes back; one button (0x455ee) Enter over the
  whole screen; Enter or E goes on to EINDANI2.SMK (not ported).
