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

- LIKELY: a session is a series of races (the "stages" of table 0x57b8c,
  indexed by [0x7519c]) for one school group ([0x751a4], groepen 3–8 on the
  menu screen), with arithmetic exercises (REKEN.EXE) between them. A 0 in the
  table means that slot has no race; `0x1ae67` steps over zero entries.
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
- SPECULATIVE: type 12 is a mine ("putbom.wav" in SNDRCS) and 15/16 hurt as
  cacti do (damage −2 or −1 in the 15/16 branch).
