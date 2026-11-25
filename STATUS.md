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

1. QUEUE.md's first open item. Q1's newest step is `init_objects` 0x148db written in `src/init.c`:
   that puts it in front of `find_waypoints` 0x1517a in the same object, which lands the game's
   three-byte jump-table pad at 0x1556c and takes `find_waypoints` to MATCH - `src/check.sh |
   grep -c ' MATCH at'` = 117, `grep find_waypoints` shows `1053 bytes MATCH at 0x01517a` with no
   WRONG fixup. The mutation that makes it fail: any spelling of init_objects' player-stats block
   that changes its length (e.g. `damage` stored last instead of after `fuel`, 1630 bytes instead
   of the game's 1635) sends `find_waypoints` back to a diff at +1010. `init_objects` itself is a
   7-row near miss at the exact game length (two register ties, open question 31), and its only
   string 'player_direction : %d wp: %d\n' (0x50e04) runs into 'init.c' at 0x50e22, which is what
   says it belongs to this module (open question 30). Earlier in the same file: `find_waypoints`
   0x1517a rewritten with `unsigned short w` and no n local (190 -> 0 instruction rows), and it plus
   `find_bonus_objects` 0x15597 moved out of `src/rrr.c` into a module named after their own
   `__FILE__` string 'init.c'. Those two mutations still make those checks fail: writing `int w`
   and an `n` local again (find_waypoints back to 177 rows, find_bonus_objects' tail jmp wrong
   again at +207), and putting either function back in `src/rrr.c`, where Watcom merges
   find_bonus_objects' tail into an earlier epilogue (its old near miss). The pad byte that used to
   hold find_waypoints back: Watcom 4-aligns a jump table inside its own object, so the game's three
   pad bytes at 0x1556c needed something 3 mod 4 bytes in front of find_waypoints in init.o, and
   init_objects (1635 B) is it (open question 30, resolved).
2. Otherwise: the next unwritten function of RRR's own C in `PROGRESS.md`, smallest and leaf-most
   first (functions whose callees are matched or are library calls). Four are left: the assembly
   blit_sprite 0x11bb4 and the three C ticks bull_tick 0x12826, muis_tick 0x130f8 and player_tick
   0x11eb5. 2026-11-27 wrote the three crash ticks into a new `src/tick.c`, muis_crash_tick
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
3. Near misses: `src/nearmiss.txt`, first line with an empty status. cell_effect 0x18b8d matched on 2026-11-23, and so did racer_contact 0x1404a and
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
