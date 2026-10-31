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
- `redcat <data dir> [map] [--race] [--groep N] [--module 1..4] [--fps N]
  [--save DIR] [--no-intro]`: the game's round as MOTOR.EXE runs it. It
  opens with the intro film (INTRO.EXE's rrrintro.smk with rrrintro.wav,
  `src/smacker.c`; Enter, Space or Esc ends it; `--no-intro` as MOTOR's
  `/NI`), then the menu,
  which first asks the pupil's name (the last four from RRRIO.INF, ↑/↓ and
  Enter, or typed; Esc gives RedCat) and plays MENU.WAV (Esc skips it)
  (`src/menu.c`: Start! S/Enter, R or a click on the Race panel the next
  world, 3..8 the groep, G the sound, B the race keys, P the Rapport (T
  back), E Einde, which shows the high scores; Esc in a race returns to it). A world
  opens with its race; then for each of its modules the exercise screen of
  the arithmetic part (`src/reken_screen.c`) and the race of that stage,
  with the fuel, repair, boosts and rockets earned and the last finishing
  order as the start order, each race followed by the podium with the
  world's points (`src/podium.c`), the world's last race by the points
  leader's film (w*.smk with tropical.wav; Enter, Space or Esc ends it); after the last race the next world. The
  modules: 1 Handig rekenen, 8 sums answered with the digit keys or by
  clicking the keypad, Backspace/TERUG, Enter/OK; 2 Wat is waar?, 8 rounds
  of three sums, 1/2/3 or a click picks the true one; 3 Memory, a grid of
  SOM and ANTWOORD cards turned by mouse, a found pair leaves the grid; 4
  Grote getallenspel, 8 rounds of reaching a target from three or four
  numbers: 1..4 or a click takes a number, + − × : (keypad, x, colon) an
  operator, = or Enter makes the sum, Backspace TERUG, o OPNIEUW. Each right
  answer, pair or target adds the shown prize. `--module N` starts with
  module N in the first world that has it; a map number replaces the next
  race's map.
  `--race` goes straight to the race. The record (RRRIO.INF) and the
  pupil's results (`%8.8d.SMD`, one record per module played, `src/smd.c`;
  the files are the original's format) are kept in the save directory:
  `--save`, else SDL's preferences path (on Linux
  `~/.local/share/Davilex/RedCat`); in the browser in IndexedDB. After
  each race RedCat's world points go into the high scores (RRRHIGH.DAT,
  `src/highscore.c`), as RRR does. The game
  data directory is only read. The race (`src/race.c`, `src/game.c`):
  start lights, RedCat's driving, Brutus and Snitch following their
  waypoints, walls, jumps, mines, time bombs, boosts, cacti, ramps, racer contact,
  crashes, the finish line, rockets and their explosions, the missiles
  Brutus and Snitch drop, exhaust smoke (fire when damaged), the
  after-images of a boost, the camera. The keys are the record's, set in
  the menu's Bediening (B); by default arrows drive, Enter boosts, right
  Shift fires a rocket; T is the horn. F1/F2/F3 pick the racer to follow,
  PgUp/PgDn change map, Esc quits (in the round: back to the menu).
  Sound (`src/audio.c`, the original's Sound Blaster mixer, from the
  SNDRCS, RRRMSND and REKENSND archives): the race's effects, start
  jingle, music and finish tune, the menu's effects and music, the
  exercises' spoken introductions, prizes and "goed" / "fout" and their
  button sounds, as the menu's Geluid (G) sets them: effects and music,
  effects, nothing. With sound on the exercises wait for what is said,
  as the original does; Enter or S cuts a module's introduction short.
- `race_sim <data dir> [map|all] [frames] [trace N]`: races without a
  screen; reports whether the computer racers finish each map.
- `reken_sim play [groep] [seed]`: the arithmetic part's first module,
  Handig rekenen, in the terminal (`src/reken.c`): 8 sums for the groep, a
  prize per sum (fuel, repair, boosts, rockets), the totals at the end.
  `reken_sim shot <data dir> out.ppm [ms] [keys] [sum answer prize]` renders
  the exercise screen headless (`shot2 <data dir> out.ppm [ms] [keys]
  [groep] [seed]` module 2's, `shot3` with comma-separated cells to click
  Memory's, `shot4` with keys, `=` for OK and `b` for TERUG). `reken_sim
  gen` / `gen2` / `gen3` / `gen4` / `prize` print
  generator output for `tools/check_reken.py`, which compares it seed by
  seed with REKEN.EXE's own code run under Unicorn.

Not ported yet: the menu's joystick set-up and the exercises' mouse
pointer.

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
port/web/build.sh                    # needs Emscripten (emcc, or .work/emsdk)
port/web/serve.sh                    # then open http://localhost:8080/redcat.html
.work/venv/bin/python port/web/test_web.py <shots dir> data:<game data dir> play wait:10 shot:intro
```

The build holds no game data. The page asks once for the player's game
folder (the CD or the installed RCREKEN) and copies the archives the port
reads into the browser's IndexedDB (`shell.html`, `save.js`), next to the
saves. The browser starts the sound after the first click or key.

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
- the exercises' prize and face animations run a frame every two ticks
  (110 ms) and count a unit every 330 ms, where the code waits one tick
  and four ticks: the time a 1996 PC would also have taken to present the
  whole screen each frame (`src/reken_screen.c` `DRAW_MS`);
- `racer_contact` tests a turned 100 px square directly instead of the
  original's four cross products (`0x1401d`), whose argument order is not
  fully read yet.
