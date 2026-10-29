# RedCat's Razende Rekenrace: port

A reimplementation in C. It reads the original game data unchanged; nothing
here modifies or redistributes original files. All format knowledge comes from
`../research/` and `../reverse-engineering/`.

## Status

Early. What exists:

- `rccore`: readers for the IND/IMG archives, PCX screens, sprites, race
  tilesets and maps (`src/archive.c`, `src/gfx8.c`).
- `rc_dump <data dir>`: opens every archive and parses every entry it has a
  reader for; exits non-zero on any failure.
- `rcview <data dir> [map]`: an SDL2 viewer at the original's 640×480 in 8-bit
  colour. It scrolls a race map with the arrow keys (PgUp/PgDn: other map),
  and Tab shows the menu, race and exercise screens (left/right to step).
- `redcat <data dir> [map] [--race] [--groep N] [--module 1..3] [--fps N]`:
  first an exercise screen of the arithmetic part (`src/reken_screen.c`):
  module 1 Handig rekenen, 8 sums answered with the digit keys or by
  clicking the keypad, Backspace/TERUG, Enter/OK; module 2 Wat is waar?,
  8 rounds of three sums, 1/2/3 or a click picks the true one; module 3
  Memory, a grid of SOM and ANTWOORD cards turned by mouse, a found pair
  leaves the grid. Each right answer or pair adds the shown prize. Then the stage's race
  (track1b) starts with the fuel, repair, boosts and rockets earned.
  `--race` goes straight to the race. The race (`src/race.c`, `src/game.c`):
  start lights, RedCat's driving, Brutus and Snitch following their
  waypoints, walls, jumps, mines, boosts, cacti, ramps, racer contact,
  crashes, the finish line, the camera. Arrows drive, right Shift boosts,
  F1/F2/F3 pick the racer to follow, PgUp/PgDn change map, Esc quits.
- `race_sim <data dir> [map|all] [frames] [trace N]`: races without a
  screen; reports whether the computer racers finish each map.
- `reken_sim play [groep] [seed]`: the arithmetic part's first module,
  Handig rekenen, in the terminal (`src/reken.c`): 8 sums for the groep, a
  prize per sum (fuel, repair, boosts, rockets), the totals at the end.
  `reken_sim shot <data dir> out.ppm [ms] [keys] [sum answer prize]` renders
  the exercise screen headless (`shot2 <data dir> out.ppm [ms] [keys]
  [groep] [seed]` module 2's, `shot3` with comma-separated cells to click
  Memory's). `reken_sim gen` / `gen2` / `gen3` / `prize` print
  generator output for `tools/check_reken.py`, which compares it seed by
  seed with REKEN.EXE's own code run under Unicorn.

Not ported yet: rockets, dropped objects, exhaust and explosions, sound, the
race's end (podium, RRRIO.INF), the menu, the exercises' Help example,
mouse pointer and spoken feedback, and modules 2–4.

### Frame rate

The original race has no frame timer: it ran as fast as the PC allowed (29
frames/s under DOSBox at 40000 cycles, 105 at 120000), and every speed is per
frame. The port runs a fixed 30 frames/s; `--fps` changes it.

## Building

```sh
port/play.sh <game data dir> [map 0..22]     # CMake + SDL2; builds, runs a race
```

The data dir is the CD root (or an install dir) with the RCS and RC archives
and `TRANS.DAT`.
File names are matched without regard to case, as on DOS.

### In the browser

```sh
port/web/build.sh <game data dir>    # needs Emscripten (emcc, or .work/emsdk)
port/web/serve.sh                    # then open http://localhost:8080/redcat.html
.work/venv/bin/python port/web/test_web.py <shots dir> wait:10 hold:ArrowDown:3 shot:race
```

The build packs the archives each page needs into `port/web/out/*.data`.
Those files are game data: keep them to yourself.

`src/anims_tab.c` is generated from RRR.EXE's animation table (frame and
image numbers only) by `port/tools/gen_anims.py`.

## Data tools

`port/rctools` is a stdlib-only Python package for the same formats:

```sh
python3 port/rctools -d <data dir> list RCS
python3 port/rctools -d <data dir> extract RCS out/
python3 port/rctools -d <data dir> sprites RC RCS:track2.img out/rc/
python3 port/rctools -d <data dir> map 0 track1a.png 4
```

## Source conventions

Where the port follows the original, the comment names the address in the
original executable it was taken from (for example RRR.EXE `0x57abc` for the
map → tileset table). Deliberate differences:

- a fixed frame rate (above);
- `racer_contact` tests a turned 100 px square directly instead of the
  original's four cross products (`0x1401d`), whose argument order is not
  fully read yet.
