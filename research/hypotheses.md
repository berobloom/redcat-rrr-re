# Hypotheses

Unproven but plausible claims, with the evidence that suggests them.
Confidence levels per AGENTS.md.

## Launch flow

- STRONGLY SUPPORTED: `MOTOR.EXE` is the shell, like A2 Racer II's
  `a2_II.exe`: it runs `intro.exe` once, then alternates `rrrmenu.exe` with
  `rrr.exe` / `reken.exe`, passing state through a file. Evidence: it is what
  RCREKEN.BAT starts, it holds exactly those program names plus `dos4gw.exe`,
  and the 32-bit programs never name each other. Not yet traced: how it picks
  the next program and which file carries the state (open question 1).
- LIKELY: `rrrio.inf` is the state file between the menu and the race. RRR
  names it in "Illegal ammount of maps in rrrio.inf: %d" (main.c line 270).

## Game structure

- CONFIRMED (session 4, was the user's description): the arithmetic part
  earns the race's fuel, repair, boosts and rockets; see discoveries "The
  arithmetic part".
- SUPERSEDED (session 4): "row of 0x57b8c = groep". The row is the world
  (+0x2c) and the groep is +0x28; discoveries "The session".

- LIKELY: the three racers are RedCat, Brutus and Snitch. RRR logs "RC", "BULL"
  and "MUIS" drive anims; the podium screens are named for the six finishing
  orders (`rbs.pcx`, `rsb.pcx`, `brs.pcx`, `bsr.pcx`, `sbr.pcx`, `srb.pcx`).

## Rendering

- DISPROVEN (session 2): "the race uses an unchained 320x200 mode". A
  screenshot shows the race at 640x480; "xvga" is the library's name. See
  discoveries "Running the original".
- LIKELY: `tile_to_xvga` (0x1fa1b) converts tiles for the banked VESA
  640x480 screen; not read yet.

## Map cells

- SETTLED (session 2): bytes 2-3 are waypoints, byte 4 the object/surface
  type, byte 5 the ground level. See discoveries "The race: map cells".
  Open: byte 1 (high byte of the tile word) holds the object number after
  loading; is it ever non-zero in the files?
- LIKELY: type 11 is the road surface (it holds the top speed near 12
  px/frame while off-road speeds above 12 drop); the exact effect over five
  checks per frame is not worked out.
- SUPERSEDED: "type 12 is a mine (putbom.wav)". It is a mine (discoveries
  "map cells", "Sound": it explodes with explo_m.wav); putbom.wav is never
  played.

## Matching

- LIKELY: bull_crash_tick 0x19c8c can go below its recorded 128 rows. Switching on the `f24` field
  itself reached 98 rows on 700 sampled one-per-line declaration orders (the existing text alone 107
  on 300). Not applied, not in nearmiss.txt; scored in parallel (GOTCHAS), so recheck serially.
  (chat 2026-11-26)
- SPECULATIVE: muis_crash_tick's set (field switch, `switch (want)`, body - waypoint, `h - heading`,
  plus a one-per-line declaration sweep) carries over to muis_tick, bull_tick, player_tick and
  player_crash_tick. The drive.c ticks share tails, so score them together. The bull_crash_tick
  probe above is the only test. (chat 2026-11-26)
- LIKELY: 6 rows is only the floor of what was tried on muis_crash_tick: the 5,040-order and
  ~2,500-respelling sweeps ran eight compiles in parallel and only the two winners were checked
  serially. (chat 2026-11-26)
