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

Nothing of the game logic is ported yet.

## Building

```sh
port/play.sh <game data dir> [map 0..22]     # CMake + SDL2; builds then runs
```

The data dir is the CD root (or an install dir) with `RCS.IND`/`RCS.IMG`.
File names are matched without regard to case, as on DOS.

### In the browser

```sh
port/web/build.sh <game data dir>    # needs Emscripten (emcc, or .work/emsdk)
port/web/serve.sh                    # then open http://localhost:8080/rcview.html
.work/venv/bin/python port/web/test_web.py <shots dir> wait:3 shot:map
```

The build packs the RCS, RRRMENU and REKEN archives into
`port/web/out/rcview.data`. That file is game data: keep it to yourself.

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
map → tileset table). Deliberate differences will be listed here.
