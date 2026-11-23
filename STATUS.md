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

1. QUEUE.md's first open item. Q1's newest step is race_end 0x1585e: `src/check.sh | grep -c
   ' MATCH at'` = 112 and `grep race_end` shows `MATCH at 0x01585e`; writing the third rc_error line
   number 609 instead of 613, or dropping the `(short)` on the `>> 16` fuel store, makes it fail.
2. Otherwise: the next unwritten function of RRR's own C in `PROGRESS.md`, smallest and leaf-most
   first (functions whose callees are matched or are library calls).
3. Near misses: `src/nearmiss.txt`, first line with an empty status (sound_init 0x1043a went to
   SKIP at 10 rows and fn_13a78 0x13a78 arrived at 15, both register allocation - open questions
   25 and 26). The smallest empty status is now `1 0x15597 find_bonus_objects`, which waits on
   `find_waypoints`; the first one to actually work on is `15 0x17c74 dropped4_tick`, which waits on
   dropped2_tick. PROGRESS.md reads 147 functions, 112 identical (76.2%), 25 near misses, and 10 not
   written: blit_sprite 0x11bb4 and blit_sprite_trans 0x17d94 are assembly (the only two `enter`
   frames in the C range) so they are not C matches to chase, and 8 C functions remain, smallest
   first: ai_waypoint_action 0x1af83 (1254 B), init_objects 0x148db (1635 B),
   muis_crash_tick 0x1a42a (1877 B), bull_crash_tick 0x19c8c (1950 B), muis_tick 0x130f8 (2037 B),
   player_crash_tick 0x19497 (2037 B - `fn_1963e` 0x1963e is a basic block of it, its only "call"
   at 0x18f3e is `mov eax, 0x6fbe8`, so the 423 + 1614 B in PROGRESS.md are one function),
   bull_tick 0x12826 (2258 B), player_tick 0x11eb5 (2417 B). The next MATCH may still come from a
   near miss: `find_waypoints` 0x1517a (190 rows, register/slot mirror) is one row from making
   `find_bonus_objects` match too.
4. Later: module boundaries in 0x10010-0x1B915 (one source file per original module); Davilex's
   libraries (REKEN's debug info names their modules); REKEN, RRRMENU, INTRO.
