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

1. QUEUE.md's first open item. Q1's newest step is racer_contact 0x1404a + racer_bounce 0x14534:
   `src/check.sh | grep -c ' MATCH at'` = 115 and they show `MATCH at 0x01404a` and `MATCH at
   0x014534`. Three mutations make it fail: moving those four functions (0x1404a-0x1467b) back into
   `src/rrr.c`, where Watcom merges racer_contact's epilogue into main's (racer_contact 5 rows, and
   racer_bounce's tail jmp is wrong again); going back to the old local order and statement order
   (declare `x1, y1, y2, x2`, write x1, y1, x2, y2: 22 rows, the slots move); and the first corner
   test's old argument
   list `x1, y1, x2, y1` (59 rows).
   2026-11-26, no new MATCH but follower_tick 0x167aa went 41 -> 9 rows: `nearmiss.sh | grep
   follower` says `9 0x0167aa follower_tick` and its differs note names the two Watcom choices left.
   Two mutations make that 9 worse: the damage/fuel test as `damage <= 0 || fuel <= 0` (the game's
   fall-through arm is the 32 store, so it wrote `!(damage > 0 && fuel > 0)`; 13 rows in the split
   file), and putting follower_tick back in the callbacks' object, where Watcom merges its epilogue
   into fn_16e46's (`src/shadeobj2.c`; 13 rows). `src/check.sh | grep -c ' MATCH at'` stays 115 and
   the other 12 callbacks are unchanged.
2. Otherwise: the next unwritten function of RRR's own C in `PROGRESS.md`, smallest and leaf-most
   first (functions whose callees are matched or are library calls).
3. Near misses: `src/nearmiss.txt`, first line with an empty status (sound_init 0x1043a went to
   SKIP at 10 rows and fn_13a78 0x13a78 arrived at 15, both register allocation - open questions
   25 and 26). cell_effect 0x18b8d matched on 2026-11-23, and so did racer_contact 0x1404a and
   racer_bounce 0x14534 once the four functions 0x1404a-0x1467b moved to `src/racer.c` (MATCHING.md:
   epilogue sharing is per object, and the game shares one only 6 times in the C range - open
   question 27 lists the six, which name the function groups that must share a module). The first
   near miss with an empty status is now `41 0x1889d ground_check` (35 -> 41 when cell_effect's tail
   moved), then `51 0x11526 draw_view` and `62 0x138ed camera_follow`; follower_tick 0x167aa went to
   SKIP at 9. dropped2_tick 0x178f6 went to SKIP on 2026-11-23: the
   mechanism is pinned
   (a second same-shape `stats[...].damage -=` statement flips the first one's load register; see
   the note in src/shadeobj2.c and the probes in .work/probe/S.C) but no block-2 spelling keeps
   block 1 natural *and* the game's RMW tail; it gates mine/sign/bomb/cactus/fn_16e46 (0 rows)
   and dropped0/dropped4_tick. PROGRESS.md reads 147 functions, 115 identical (78.2%), 23 near misses,
   and 9 not
   written: blit_sprite 0x11bb4 and blit_sprite_trans 0x17d94 are assembly (the only two `enter`
   frames in the C range) so they are not C matches to chase, and 7 C functions remain, smallest
   first: init_objects 0x148db (1635 B),
   muis_crash_tick 0x1a42a (1877 B), bull_crash_tick 0x19c8c (1950 B), muis_tick 0x130f8 (2037 B),
   player_crash_tick 0x19497 (2037 B - `fn_1963e` 0x1963e is a basic block of it, its only "call"
   at 0x18f3e is `mov eax, 0x6fbe8`, so the 423 + 1614 B in PROGRESS.md are one function),
   bull_tick 0x12826 (2258 B), player_tick 0x11eb5 (2417 B). The next MATCH may still come from a
   near miss: `find_bonus_objects` 0x15597 is 0 instruction rows and waits on one jmp fixup (its
   epilogue is find_waypoints', which sits at 190 rows), and the five 0-row dropped/mine/sign/bomb
   lines wait on dropped2_tick. Another is likely from the module boundaries of item 4: a function
   whose epilogue the game keeps for itself must not share a file with the one ours merged into
   (`src/nearmiss.txt`, MATCHING.md).
4. Later: the rest of the module boundaries in 0x10010-0x1B915 (one source file per original
   module; `src/racer.c` 0x1404a-0x1467b is the first found from epilogue sharing - open question
   27 lists the six shares the game has, each pair surely in one module); Davilex's
   libraries (REKEN's debug info names their modules); REKEN, RRRMENU, INTRO.
