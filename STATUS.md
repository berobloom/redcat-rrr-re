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

1. QUEUE.md's first open item. Q1's newest step is cell_effect 0x18b8d: `src/check.sh | grep -c
   ' MATCH at'` = 113 and `grep cell_effect` shows `1536 bytes MATCH at 0x018b8d`; writing case 16 as
   `damage >= 1 && fuel >= 1` again (87 rows) or the case-22 store as `(y * map_w + x)` (1 row) makes
   it fail.
2. Otherwise: the next unwritten function of RRR's own C in `PROGRESS.md`, smallest and leaf-most
   first (functions whose callees are matched or are library calls).
3. Near misses: `src/nearmiss.txt`, first line with an empty status (sound_init 0x1043a went to
   SKIP at 10 rows and fn_13a78 0x13a78 arrived at 15, both register allocation - open questions
   25 and 26). cell_effect 0x18b8d matched on 2026-11-23, so the first near miss with an empty status
   is `25 0x1404a racer_contact`, then `41 0x1889d ground_check` (35 -> 41 when cell_effect's tail
   moved) and `41 0x167aa follower_tick`. dropped2_tick 0x178f6 went to SKIP on 2026-11-23: the
   mechanism is pinned
   (a second same-shape `stats[...].damage -=` statement flips the first one's load register; see
   the note in src/shadeobj.c and the probes in .work/probe/S.C) but no block-2 spelling keeps
   block 1 natural *and* the game's RMW tail; it gates mine/sign/bomb/cactus/fn_16e46 (0 rows)
   and dropped0/dropped4_tick. PROGRESS.md reads 147 functions, 113 identical (76.9%), 25 near misses,
   and 9 not
   written: blit_sprite 0x11bb4 and blit_sprite_trans 0x17d94 are assembly (the only two `enter`
   frames in the C range) so they are not C matches to chase, and 7 C functions remain, smallest
   first: init_objects 0x148db (1635 B),
   muis_crash_tick 0x1a42a (1877 B), bull_crash_tick 0x19c8c (1950 B), muis_tick 0x130f8 (2037 B),
   player_crash_tick 0x19497 (2037 B - `fn_1963e` 0x1963e is a basic block of it, its only "call"
   at 0x18f3e is `mov eax, 0x6fbe8`, so the 423 + 1614 B in PROGRESS.md are one function),
   bull_tick 0x12826 (2258 B), player_tick 0x11eb5 (2417 B). The next MATCH may still come from a
   near miss: `find_bonus_objects` 0x15597 is 0 instruction rows and waits on one jmp fixup, and
   `racer_bounce` 0x14534 the same (layout, see `src/nearmiss.txt`).
4. Later: module boundaries in 0x10010-0x1B915 (one source file per original module); Davilex's
   libraries (REKEN's debug info names their modules); REKEN, RRRMENU, INTRO.
