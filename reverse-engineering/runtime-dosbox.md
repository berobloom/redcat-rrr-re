# Running the original under DOSBox-X

The original runs in the patched DOSBox-X of `../dos-re-suite` (GDB stub on
port 2159, QMP on 4444; one client each).

```sh
reverse-engineering/dosbox/install.sh          # .work/dos/C/DAVILEX/RCREKEN
D=".work/venv/bin/python reverse-engineering/tools/dosrun.py"
$D start                                       # hidden Xvfb display :57
$D type 'RRR /DAVILEX /DEBUG'                  # straight into a race
$D key ret                                     # leave the wait screen
$D shot out.png
$D locate rrr                                  # run-time object addresses
$D mem 0x22b1a4 16                             # read a variable (linear)
$D poke 0x20bfbc 01000000                       # write one (GDB stub)
$D save name / $D load name                    # .work/dos/states/
$D stop
```

`RC_SHOW=1` runs DOSBox-X on the real display instead.

## Things that bite

- The GDB stub holds the machine at boot until a client sends continue;
  `start` does that and detaches.
- A SIGTERM makes DOSBox-X ask "quit anyway?" in a zenity dialog that
  inherits the QMP and GDB ports, so nothing can connect until it is gone.
  The config sets `quit warning=false` and `stop` uses SIGKILL.
- QMP `screendump` looks for `dosbox-x_NNN.png`, but DOSBox-X names captures
  after the running program (`rrr_000.png`); `shot` falls back to the newest
  file in `.work/dos/capture/`.
- DOS/4GW loads the code and data objects at different offsets from the LE
  image (RRR: code 0x10000 -> 0x1d4000, data 0x50000 -> 0x206000 under
  `redcat.conf`, memsize 16). Changing memsize or what runs first moves them;
  `locate` finds them by searching fixup-free byte windows.
- The intro did not skip on Esc, Space or Enter (mouse not tried), though
  its code ends the film on any of them once released (discoveries
  "The round"); not yet explained.

- `cycles = fixed 40000` is not always real time: with another DOSBox-X
  busy on the machine, the BIOS tick (0x46c) came every 76–88 ms of wall
  time instead of 55 (session 6). Time animations in ticks, read from
  0x46c next to the memory being polled, not by the wall clock.
- `dosrun.py` uses QMP 4444 and GDB 2159. When another emulator holds
  them, a copy of `redcat.conf` with other `qmpserver port` /
  `gdbserver port` values runs a second DOSBox-X; connect to it with
  `QmpClient('127.0.0.1', port, 10.0)` and stop it by its own pid
  (`dosrun.py stop` kills by the config path).

## Observed

- `RRR /DAVILEX /DEBUG` without the menu: no RRRIO.INF on disk, and RRR
  writes one (552 bytes) and races track1a (row 0, column 0 of the
  0x57b8c table), so a missing state file means groep 3, first race.
- DEBUG.TXT lists every load step (file positions in the archives), the
  waypoints found per racer, a 33x33 table of 32 directions, and the sound
  set-up.

## Mouse

QMP `input-send-event` `rel` events do not move the game's pointer, with
either driver: DOSBox-X counts mickeys only while the window holds the
mouse (`user_cursor_locked`), and QMP moves never take it. The
built-in INT 33h driver also logs "MOUSE: Unhandled videomode 69 on
reset" for the game's VESA mode 0x101.

What works (session 6, RRRMENU's paw followed it; position read from
[0x5e328] / [0x5e324]):

1. add `[dos]` `int33=false` to `.work/dos/redcat.conf` (generated; the
   template is unchanged), start, and load the CD's driver: `type
   'D:\GMOUSE'` ("Personal System II Mouse driver installed");
2. capture the mouse with real X input on the Xvfb display:
   `DISPLAY=:57 xdotool key --window $(xdotool search --name DOSBox-X)
   ctrl+F10` (the title then ends in "[Ctrl+F10 releases mouse]");
3. move with `xdotool mousemove_relative -- dx dy` and click with
   `xdotool click 1`. In small steps one X pixel moved the pointer about
   2 game pixels across and 1 down; larger steps are accelerated.

Save states made under the default config don't carry the driver, so
take the route from a fresh start.
