# STATUS

Short: where the project stands, and what is next. History is in git; no diary.

## State

- Executables: RRR, REKEN, RRRMENU, INTRO: bound DOS/4GW LE, Watcom C/C++ **10.5**, `-3r -s -ox`
  (research/discoveries.md "Executables"; MATCHING.md). REKEN carries Watcom debug info naming its
  modules (not decoded beyond the module list).
- **Matching decompilation of RRR.EXE:** `src/` (`rrr.h`, `new2io.c`, `order.c`, `rrr.c` for the modules not split yet), checked by `src/check.sh`; the count is in
  `src/matched.baseline`, the numbers in `PROGRESS.md` / `docs/RRRPROGRESS.HTML`. The game's own C is
  0x10010-0x1B915; the libraries after it are not identified beyond Watcom's clib and math library.
- Toolchain: the game's compiler is Watcom C/C++ **10.6** (clib fingerprint, discoveries
  "Executables"): `.work/watcom/dos/C106/WATCOM` (`WC=... src/check.sh`); `.work/watcom/dos/C/WATCOM`
  holds 10.5 and generates identical segments for `src/` (`tools/wc/segcmp.py`). Wine
  (reverse-engineering/watcom.md).
- The port (`port/`) plays the whole round; it is a new engine, not built from `src/`.

## Next

1. QUEUE.md's first open item. Q1's newest step is a shared index holder in `src/shadeobj2.c`:
   `dropped2_tick` 0x178f6's first damage block reads its stats index through the `int h` the aim
   block uses (`h = bodies[2].stats; stats[h].damage -= 10;`). That makes Watcom emit the `*3 << 4`
   chain once, so the racer-4 block ends in `jmp <chain>` like the game's, the function is exactly
   619 bytes, and the epilogue at 0x17b59 that five callbacks jmp to lands right: `src/check.sh |
   grep -c ' MATCH at'` = 124 of 147 (84.4%), with `dropped0_tick` 0x17b61, `dropped4_tick` 0x17c74,
   `mine_tick`, `sign_tick`, `bomb_tick`, `fn_16e46` and `cactus_tick` MATCHing. The mutations that
   make it fail: `stats[bodies[2].stats].damage -= 10;` back in that block (dropped2_tick to 16 rows,
   the other seven differ again), or a separate `int s` instead of `h` (44 rows). `dropped2_tick`
   itself is a 2-row near miss at the exact length now - one register tie in its last x store, open
   question 32. Earlier in `src/init.c`: `init_objects` 0x148db (1635 B) in front of
   `find_waypoints` 0x1517a lands the game's three jump-table pad bytes at 0x1556c, and
   `find_waypoints` with `unsigned short w` and no n local is 190 -> 0 rows (open question 30).
2. Otherwise: a near miss (item 3). Nothing in 0x10010-0x1B915 is unwritten C any more:
   `blit_sprite` 0x11bb4 and `blit_sprite_trans` 0x17d94 carry `ASM:` lines in `src/nearmiss.txt`
   (the game's two `enter`-frame assembly modules, discoveries "Executables"), so `PROGRESS.md` reads
   124 of 147 (84.4%), 21 near misses, 0 not written, 2 assembly, and `grep -c 'not in src/ yet'
   PROGRESS.md` is 0. What follows is how that C got written: 2026-11-27 wrote the three normal
   ticks, player_tick 0x11eb5 (193 rows),
   bull_tick 0x12826 (208) and muis_tick 0x130f8 (87), into a second new module `src/drive.c`: the
   game lays those three back to back and the three crash ticks back to back with other modules in
   between, and the split is worth 7 rows on player_tick and 4 on muis_tick, because it removes the
   tail merge between the two groups. muis_tick is
   muis_crash_tick's AI plus the damage/fuel test that sets crash4, play_fx(7, distance to RedCat)
   and an anim offset chosen by vz; the two levers that took 112 -> 91 are the separate `if`s for
   damage and fuel (the second `jl` then jumps back into the first crash store) and writing the vz
   thresholds as negative ints (`0xfffb0000` as an int is a huge positive, so WCC compares unsigned;
   -0x50000 gives the game's `jge`); the vz table writes `objects[n].frame` (+0xa), not `anim` (+8).
   bull_tick's 208 is register allocation, not a missing statement: the byte shift between the two
   grows 13 -> 29 -> 114 across the function, the game re-deriving the bodies[] index where we keep
   it in a register. Earlier the same day: the three crash ticks, muis_crash_tick
   0x1a42a (70 rows), bull_crash_tick 0x19c8c (185) and player_crash_tick 0x19497 (282); the three
   crash ticks are per-racer C, not copies - under half their
   instruction streams agree, and only bull's has the racer_contact/racer_bounce block.
   player_crash_tick is the one that reads the keyboard: it names the player's input flags
   key_forward/key_back 0x55fd0/0x55fd4 and the 3x3 steering matrix steer_tbl 0x56030, and writing
   it collapsed the phantom fn_1963e (0x1963e is a basic block inside it), so PROGRESS.md now reads
   146 functions, 80.1%. `stats[].f24`
   is unsigned (the game's tests on it are `test` + `jbe`/`ja`), which is worth four rows on its own,
   and `if (f <= 0) {...} else if (f == 3) {...}` lands the game's block order where
   `if (f > 0) { if (f == 3) ... } else {...}` puts the same bodies the other way round (78 -> 70).
3. Near misses: `src/nearmiss.txt`, first line with an empty status. 2026-11-25: one word takes
   25 rows off `bull_crash_tick` 0x19c8c - the `go == 0` arm's f24 guard is `if (f <= 0)`, not
   `if (f == 0)`, because the game's test is unsigned (`test` + `ja`, discoveries "Toolchain:
   narrow types"), and it is worth 3 more on `muis_crash_tick` 0x1a42a (70 -> 67, now SKIPped:
   36 spellings of the f24/want chains leave one block-placement decision, 22 rows, untouched).
   Splitting `src/tick.c` into one file per crash tick (they share no epilogue in the game, open
   question 27) took `player_crash_tick` 282 -> 276. That is 538 -> 501 rows over the three, the
   biggest single-day drop outside the shared-index-holder commit; the counts in `PROGRESS.md`
   are unchanged (124 of 147) because no function reached MATCH. **The compiler-version
   explanation is dead** (2026-11-27): the game does link 10.6, but 10.5 and 10.6 produce identical
   segments for all 16 modules, so `src/check.sh` reads 117 MATCH and the same 28 near misses with
   both - the one-register-choice family (open questions 25, 26, 28, 29, 31) is still a C question.
   Two places to spend the next cycles, by prize rather than by row count: `dropped2_tick` 0x178f6
   (mine/sign/bomb/fn_16e46/cactus_tick are already at 0 rows and only wait for its epilogue to land
   at 0x17b59, so score the epilogue address, not the rows: the `s = bodies[X].stats;` holder spelling
   reaches the game's load-EAX shape at 9 rows but 580 of 619 bytes, and the missing ~49 bytes of a
   619-byte function are small enough to read off the disassembly), and the six tick functions
   (70 + 87 + 185 + 193 + 208 + 282 rows, more than every other near miss combined) are first-pass C
   with zero probe cycles spent: the levers that worked on 2026-11-27 (unsigned fields, `if (f <= 0)
   A else if (f == 3) B` block order, one module per function group) have not been swept there. The
   A new instrument sorts that work: `tools/wc/nmclass.py` splits every near miss's rows into register
   `home` rows (the C is right, Watcom picked other registers/slots) and `shape` / one-sided rows (an
   instruction the game has and we lack, or the reverse). It says the small SKIPped misses really are
   homes (ground_check 15 = 2 homes + 3 shape; dropped2_tick 16 = 1 home + 0 shape + 6/9 one-sided),
   while the six ticks are full of one-sided code: muis_crash_tick 22 only-ours (58 B) against 22
   only-game (27 B), bull_crash_tick 53 homes but 49/47 one-sided, player_crash_tick 85 homes with
   74/83 one-sided, player_tick 71 homes with 34/44, bull_tick 58 homes with 66/65, muis_tick 26 homes
   with 22/24. So the ticks are where statements and expression shapes are still wrong, and they are
   the only near misses with real C left in them. cell_effect 0x18b8d matched on 2026-11-23, and so did racer_contact 0x1404a and
   racer_bounce 0x14534 once the four functions 0x1404a-0x1467b moved to `src/racer.c` (MATCHING.md:
   epilogue sharing is per object, and the game shares one only 6 times in the C range - open
   question 27 lists the six, which name the function groups that must share a module). The first
   near miss with an empty status is now `145 0x1af83 ai_waypoint_action` (`71 0x1430e hit_test`
   went to SKIP on 2026-11-24: 71 -> 5, 550 = 550 bytes - only the h/x1 stack slot numbers remain;
   the sin_tbl[h] second product, (h + 0x28) & 0x1f, test 5's y-check as `bodies[b].y > yl` and the
   racer.o declaration tail y1, x2, y2, x3, y3, x4, x1, y4 are the C - see the differs note in
   src/racer.c; `62 0x138ed camera_follow` went to
   SKIP on 2026-11-26: 62 -> 19 with the C established - short vw/vh/tx/ty, int x/y with (short)
   casts, int vx/vy locals, ternaries inline - and what is left is two Watcom allocation ties, one
   declaration family spills tx like the game but mirrors vx/vy, the other does the reverse;
   `15 0x1889d ground_check`, 41 -> 15 on 2026-11-24, inline
   cell indices instead of `ox`/`oy` locals, is open question 29, and `51 0x11526 draw_view` went to
   SKIP on 2026-11-25: register homes only, 190 in-place spellings moved nothing); follower_tick 0x167aa went to
   SKIP at 9. dropped2_tick 0x178f6 went to SKIP on 2026-11-23: the
   mechanism is pinned
   (a second same-shape `stats[...].damage -=` statement flips the first one's load register; see
   the note in src/shadeobj2.c and the probes in .work/probe/S.C) but no block-2 spelling keeps
   block 1 natural *and* the game's RMW tail; it gates mine/sign/bomb/cactus/fn_16e46 (0 rows)
   and dropped0/dropped4_tick. 2026-11-27, muis_crash_tick 0x1a42a is written in `src/tick.c`
   (1878 of 1877 bytes, 78 rows); it is not a copy of the other two crash ticks, under half their
   instruction streams agree, and it names racer 4's tick state 0x56018-0x5602c and the puff counter
   0x71ddc. PROGRESS.md reads 147 functions, 117 identical (79.6%), 23 near misses, and 7 not
   written: blit_sprite 0x11bb4 and blit_sprite_trans 0x17d94 are assembly (the only two `enter`
   frames in the C range) so they are not C matches to chase, and 5 C functions remain, smallest
   first:
   bull_crash_tick 0x19c8c (1950 B), muis_tick 0x130f8 (2037 B),
   player_crash_tick 0x19497 (2037 B - `fn_1963e` 0x1963e is a basic block of it, its only "call"
   at 0x18f3e is `mov eax, 0x6fbe8`, so the 423 + 1614 B in PROGRESS.md are one function),
   bull_tick 0x12826 (2258 B), player_tick 0x11eb5 (2417 B). The next MATCH may still come from a
   near miss: the five 0-row dropped/mine/sign/bomb
   lines wait on dropped2_tick. Another is likely from the module boundaries of item 4: a function
   whose epilogue the game keeps for itself must not share a file with the one ours merged into
   (`src/nearmiss.txt`, MATCHING.md).
4. Later: the rest of the module boundaries in 0x10010-0x1B915 (one source file per original
   module; `src/racer.c` 0x1404a-0x1467b found from epilogue sharing and `src/init.c`
   0x148db-0x1585d from its string block head ('player_direction : %d wp: %d' 0x50e04 runs into
   'init.c' 0x50e22), the shared epilogue and now the jump-table pad - open question
   27 lists the six shares the game has, each pair surely in one module); Davilex's
   libraries (REKEN's debug info names their modules); REKEN, RRRMENU, INTRO.
