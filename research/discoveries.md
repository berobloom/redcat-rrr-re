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
- CONFIRMED (file after the run): `NNNNNNNN.SMD` (`0x19cfb` finds the pupil's
  file, `%8.8d.SMD`) ends with a record holding module 1, 8 sums, 7 correct
  (big-endian-looking u32s), the date as text "29/09/2026" on 29 October
  (LIKELY tm_mon without +1) and the groep. Layout not decoded yet.
