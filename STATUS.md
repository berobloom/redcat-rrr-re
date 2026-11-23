# STATUS

Short: where the project stands, and what is next. History is in git; no diary.

## State

- Executables: RRR, REKEN, RRRMENU, INTRO: bound DOS/4GW LE, Watcom C/C++ **10.5**, `-3r -s -ox`
  (research/discoveries.md "Executables"; MATCHING.md). REKEN carries Watcom debug info naming its
  modules (not decoded beyond the module list).
- **Matching decompilation of RRR.EXE:** `src/` (`rrr.h`, `new2io.c`, `order.c`, `rrr.c` for the modules not split yet), checked by `src/check.sh`; the count is in
  `src/matched.baseline`, the numbers in `PROGRESS.md` / `docs/RRRPROGRESS.HTML`. The game's own C is
  0x10010-0x1B915; the libraries after it are not identified beyond Watcom's clib and math library.
- Toolchain: `.work/watcom/dos/C/WATCOM` (reverse-engineering/watcom.md), Wine.
- The port (`port/`) plays the whole round; it is a new engine, not built from `src/`.

## Next

1. QUEUE.md's first open item.
2. Otherwise: the next unwritten function of RRR's own C in `PROGRESS.md`, smallest and leaf-most
   first (functions whose callees are matched or are library calls).
3. Near misses: `src/nearmiss.txt`, first line with an empty status (hit_test 0x1430e was
   written this session and sits at the bottom of that list with 71 rows).
   The unwritten list is now honest: `tools/entries.py` showed 10 of the 157 starts in
   the C range are byte-scan artifacts, so PROGRESS.md reads 147 functions, 110
   identical (74.8%), 22 near misses, and these 15 to write: blit_sprite 0x11bb4,
   player_tick 0x11eb5, bull_tick 0x12826, muis_tick 0x130f8, fn_13a78 0x13a78,
   init_objects 0x148db, find_waypoints 0x1517a, find_bonus_objects 0x15597,
   race_end 0x1585e, podium 0x15a9f, player_crash_tick 0x19497, fn_1963e 0x1963e,
   bull_crash_tick 0x19c8c, muis_crash_tick 0x1a42a, ai_waypoint_action 0x1af83.
4. Later: module boundaries in 0x10010-0x1B915 (one source file per original module); Davilex's
   libraries (REKEN's debug info names their modules); REKEN, RRRMENU, INTRO.
