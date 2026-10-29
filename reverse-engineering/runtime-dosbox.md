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
- The intro does not skip on Esc, Space or Enter (mouse not tried).

## Observed

- `RRR /DAVILEX /DEBUG` without the menu: no RRRIO.INF on disk, and RRR
  writes one (552 bytes) and races track1a (row 0, column 0 of the
  0x57b8c table), so a missing state file means groep 3, first race.
- DEBUG.TXT lists every load step (file positions in the archives), the
  waypoints found per racer, a 33x33 table of 32 directions, and the sound
  set-up.
