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

- LIKELY: the race uses an unchained 256-colour mode ("xvga": `xvga_src_fopen`,
  `load_xvga_map_images`, XVGALIB.IMG), while menus and screens are 640×480
  (every PCX is 640×480; VESA drivers ship on the CD; batch option `/V`).
  `tile_to_xvga` (0x1fa1b) converting each tile on load fits a planar layout.

## Map cells

- SPECULATIVE: cell field 2 is the collision/surface class (0x200 on 74% of
  TRACK1A's cells, ten distinct values), and field 1 places objects or
  waypoints (0 on 99%, small packed values elsewhere; "Searching waypoints in
  map" runs right after loading).
