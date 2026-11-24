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

1. QUEUE.md's first open item. Q1's newest step is `src/init.c`: `find_waypoints` 0x1517a rewritten
   with `unsigned short w` and no n local (190 -> 0 instruction rows, its 1010 code bytes identical),
   and it plus `find_bonus_objects` 0x15597 moved out of `src/rrr.c` into a module named after their
   own `__FILE__` string 'init.c'. `src/check.sh | grep -c ' MATCH at'` = 116, `grep find_bonus` in
   the output shows `MATCH at 0x015597 (fixup bytes not compared; 1 rel32 branch compared by
   target)` and no WRONG fixup. Two mutations make it fail: writing `int w` and an `n` local again
   (find_waypoints back to 177 rows, find_bonus_objects' tail jmp wrong again at +207), and putting either
   function back in `src/rrr.c`, where Watcom merges find_bonus_objects' tail into an earlier
   epilogue (its old near miss). `find_waypoints` itself stays a near miss on one pad byte: the game
   pads find_bonus_objects' jump table with 3 bytes at 0x1556c and we with 2, because Watcom
   4-aligns a jump table inside its own object and the game's init.o holds 3 mod 4 bytes before it
   (open question 30; adding `fn_15166`, 20 B, keeps the 2-byte pad, `init_objects` 0x148db, 1635 B,
   would land it).
2. Otherwise: the next unwritten function of RRR's own C in `PROGRESS.md`, smallest and leaf-most
   first (functions whose callees are matched or are library calls).
3. Near misses: `src/nearmiss.txt`, first line with an empty status. cell_effect 0x18b8d matched on 2026-11-23, and so did racer_contact 0x1404a and
   racer_bounce 0x14534 once the four functions 0x1404a-0x1467b moved to `src/racer.c` (MATCHING.md:
   epilogue sharing is per object, and the game shares one only 6 times in the C range - open
   question 27 lists the six, which name the function groups that must share a module). The first
   near miss with an empty status is now `62 0x138ed camera_follow` (`15 0x1889d ground_check`, 41 -> 15 on 2026-11-24, inline
   cell indices instead of `ox`/`oy` locals, is open question 29, and `51 0x11526 draw_view` went to
   SKIP on 2026-11-25: register homes only, 190 in-place spellings moved nothing); follower_tick 0x167aa went to
   SKIP at 9. dropped2_tick 0x178f6 went to SKIP on 2026-11-23: the
   mechanism is pinned
   (a second same-shape `stats[...].damage -=` statement flips the first one's load register; see
   the note in src/shadeobj2.c and the probes in .work/probe/S.C) but no block-2 spelling keeps
   block 1 natural *and* the game's RMW tail; it gates mine/sign/bomb/cactus/fn_16e46 (0 rows)
   and dropped0/dropped4_tick. PROGRESS.md reads 147 functions, 116 identical (78.9%), 22 near misses,
   and 9 not
   written: blit_sprite 0x11bb4 and blit_sprite_trans 0x17d94 are assembly (the only two `enter`
   frames in the C range) so they are not C matches to chase, and 7 C functions remain, smallest
   first: init_objects 0x148db (1635 B),
   muis_crash_tick 0x1a42a (1877 B), bull_crash_tick 0x19c8c (1950 B), muis_tick 0x130f8 (2037 B),
   player_crash_tick 0x19497 (2037 B - `fn_1963e` 0x1963e is a basic block of it, its only "call"
   at 0x18f3e is `mov eax, 0x6fbe8`, so the 423 + 1614 B in PROGRESS.md are one function),
   bull_tick 0x12826 (2258 B), player_tick 0x11eb5 (2417 B). The next MATCH may still come from a
   near miss: `find_waypoints` 0x1517a is 0 instruction rows and waits on one pad byte, which
   init_objects 0x148db would settle (open question 30), and the five 0-row dropped/mine/sign/bomb
   lines wait on dropped2_tick. Another is likely from the module boundaries of item 4: a function
   whose epilogue the game keeps for itself must not share a file with the one ours merged into
   (`src/nearmiss.txt`, MATCHING.md).
4. Later: the rest of the module boundaries in 0x10010-0x1B915 (one source file per original
   module; `src/racer.c` 0x1404a-0x1467b found from epilogue sharing and `src/init.c`
   0x1517a-0x1585d from the 'init.c' strings plus their shared epilogue - open question
   27 lists the six shares the game has, each pair surely in one module); Davilex's
   libraries (REKEN's debug info names their modules); REKEN, RRRMENU, INTRO.
