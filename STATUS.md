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

0. **The overnight pass of 2026-11-28 21:45-23:40 ran in a different state of this project and its
   work is gone.** It worked `update_speed` 0x12e1b (370 near-miss rows, that tree's QUEUE item 11)
   in a tree with `src/matched.baseline` 145, a `src/physics.c`, a `GOALS.md` and a `.work` whose
   `rrr.obj1.bin` was 505,776 B; this directory, `.git` included, replaced it mid-session, and
   another agent was working here at the same time (`.work/pi/split/exhaust.py`, init_objects).
   Evidence (CONFIRMED): this `.git`'s 276-entry reflog holds neither of its commits
   (`git cat-file -t c514b90` -> no such object), `src/nearmiss.txt` here has no `update_speed`
   line, `.work/pi/update_speed/` is gone, and `.work/le/rrr.obj1.bin` here is 201,776 B
   (`leparse.py .work/iso/RRR.EXE`: obj1 base 0x10000, vsize 0x31430), so the addresses it recorded
   cannot be re-checked against this `.work`. Its finding is in the message of d2f8268; restore that
   state (or re-run the item) before resuming `update_speed`.

0. QUEUE Q4: a WLINK 10.6 link reproduces RRR.EXE byte for byte (`reverse-engineering/tools/
   link/link.sh`, `LINK: 419179 of 419179`), and since 2026-11-25 **104 library modules link as
   themselves** in place of their blob ranges (`$(cat .work/link/libobjs.args4)` plus
   `--obj .../clib3r_inirt386.OBJ@32d22 --dseg XI@6c668 --dseg YI@6c6a4-12 --dseg DATA32@6c6b8`;
   74 clib + 10 math387 + 20 Fastgraph ASM; dead Fastgraph EXTDEFs resolved through DATA.ASM anchored at
   0x60a56). **The data-object segment order is settled** (discoveries "the data object's segment
   order", `tools/link/dataobj.py`): one merged run per segment name in the order WLINK first heard the names, CONST,
   CONST2, _DATA, XIB, XI, XIE, YIB, YI, YIE, DATA32, _BSS, STACK, runs 4-byte aligned, and inside a
   run the modules in feed order = code-object order (23 content-pinned segments, 0 out of order);
   XIB/XIE/YIB/YIE are WLINK's own bounds-of-XI/YI symbols, which is what blocked inirt386. blob.py
   now places a real object's data segment too (`--obj F.OBJ@code,NAME=data`: the data blob is cut
   into one object per gap, FILES order = merge order, pieces fed as late as their run allows, and
   the XI/YI blocks after inirt386 so WLINK invents XIB/XIE/YIB/YIE around the run) and the link is
   still `LINK: 419179 of 419179`. Since 2026-11-26 **143 real objects link as themselves,
   39 of them with data, BSS or XI/YI entries** (`$(cat .work/link/libobjs.args6)`, `LINK: 419179
   of 419179`). `tools/link/dataplace.py` places their data segments from their own fixups,
   `_BSS` included (STACK comes out at 25,000 bytes). blob.py feeds a gap's head right after the
   placed segment (empty dword-aligned segments pad the run), and pieces whose feed windows overlap
   share one object (discoveries "real data segments link byte-identically"). Left out: nomem (not in
   the game, LIKELY), 386inite, and _matherr/fpeinth on code page 0x3d000. **Four src/ modules
   (ORDER, SHADE, RACEANIM, HUD) link as themselves too, and GUARD and HISCORE after the hiscore.c
   split, RRRIO (out of rrr.c) and MAIN (0x10010-0x11526, src/main.c): 151 real objects**, `$(cat .work/link/libobjs.args11)`,
   `LINK: 419179 of 419179`. More src/ modules can join as their
   whole module matches. Code pages 0x29000/0x3d000 stay blobs (the
   original feed order there is not placement order, QUEUE Q4 note).
   QUEUE Q3 (data comparison) is READY FOR REVIEW: `src/check.sh`'s last line `DATA: 1946 of 5861
   data-segment bytes identical in place` = `src/data.baseline`; mutation: one character of order.c's
   rc_error string -> 1834. Next is Q4 (first WLINK). Smacker hunt (2026-11-25): RRR's Smacker is
   2.1e-2.1j, its decompressor identical to the 2.1g SMACKW32.DLL; no DOS SDK found (discoveries).
   QUEUE Q2 (library map) is READY FOR REVIEW too. The library
   range 0x1B916-0x4142F is mapped in `reverse-engineering/analysis/rrr_libmap.tsv` (tools/libmap.py,
   from REKEN's decoded debug block, tools/wdbg.py + tools/modmatch.py): Smacker 28.8 KB, clib
   26.3 KB, Fastgraph 24.1 KB, SOUNDLIB 16.9 KB, Davilex functions that differ from REKEN's copy
   15.0 KB, emu387 12.9 KB, WINLIB 12.3 KB, unknown 8.4 KB (discoveries "Executables"). Check: the
   TSV is contiguous and sums to 154,394; mutation: a narrower `SMACKER_C` in libmap.py makes it stop.
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
   question 32; 2026-11-28 overnight: 16 more vbatch spellings (.work/pi/d2/v.py, v2.py) all cost
   rows, so it stays 2 and `src/check.sh` stays at 128 MATCH, 0 WRONG, DATA 9677/9677; QUEUE Q6's
   next functions are `muis_crash_tick` 0x1a42a (6) and `left_of` 0x1401d (8) - `hit_test` 0x1430e
   holds at 5 (its `h`/`x1` stack slots; 7 more spellings on 2026-11-28, none below 5). 2026-11-29
   overnight: `muis_crash_tick` 0x1a42a stays 6 and `left_of` 0x1401d stays 8 after 28 and 21
   spellings scored serially (.work/pi/mct2/, .work/pi/left_of/); each is now pinned down to ONE
   register pair plus the length the pair decides - muis_crash_tick's `mov eax,[@]` at 0x1a922 is
   the 5-byte moffs form where the game's `mov edx,[@]` needs 6, which is its 6th row (discoveries
   "Toolchain: register parameters and the 5-byte `mov eax,[disp32]`"); `src/check.sh` is still 128
   MATCH of 147 (87.1%), 0 WRONG, DATA 9677/9677. 2026-11-29 overnight, two more:
   `spawn_dropped_object` 0x18759 holds 8 (floor 7, switch arm order `case 2 / default / case 0`; its
   rows are one register mirror that the dispatch construct BELOW the period store decides - open
   question 36) and `follower_tick` 0x167aa holds 9 after 20 more spellings (the `frame = 32` store is
   already byte-identical; a named `int m = n;` costs 15). Both nearmiss lines now start `ON:`; counts
   unchanged at 128 of 147. 2026-11-28 overnight: `ground_check` 0x1889d is a **MATCH**: writing
   `cy = (bodies[n].y >> 16) / 40;` again before the last cell lookup kills WCC's CSE of `cy * map_w`, so the
   quotient survives the branch and each lookup multiplies it into its own register - the game's 584 bytes
   (GOTCHAS "Levers", open question 29 closed). 129 of 147 (87.8%), 0 WRONG, DATA 9677/9677, 16 near misses;
   the whole `src/map.c` 0x18886-0x1918c now MATCHes (the Q4 lead). Q6 round 1 walked `camera_follow` 0x138ed (19, the game's tx spill to [esp+8]) and `muis_tick` 0x130f8 (24, its tail movsx is the byte gap) on 2026-11-28; four lines still have no `ON:` status: `bull_crash_tick` 0x19c8c (42), `bull_tick` 0x12826 (50), `draw_view` 0x11526 (51), `ai_waypoint_action` 0x1af83 (64). Earlier in
   `src/init.c`: `init_objects` 0x148db (1635 B) in front of
   `find_waypoints` 0x1517a lands the game's three jump-table pad bytes at 0x1556c, and
   `find_waypoints` with `unsigned short w` and no n local is 190 -> 0 rows (open question 30).
2. Otherwise: a near miss (item 3). Nothing in 0x10010-0x1B915 is unwritten C any more:
   `blit_sprite` 0x11bb4 and `blit_sprite_trans` 0x17d94 carry `ASM:` lines in `src/nearmiss.txt`
   (the game's two `enter`-frame assembly modules, discoveries "Executables"), so `PROGRESS.md` reads
   129 of 147 (87.8%), 16 near misses, 0 not written, 2 assembly, and `grep -c 'not in src/ yet'
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
   question 27) took `player_crash_tick` 282 -> 276. Same day, in the tick group: the game's
   arithmetic truth test `if (h - stats[bodies[n].stats].heading && h != -1)` (it computes the
   difference and tests it, discoveries "Toolchain: narrow types") is worth 35 rows on `bull_tick`
   0x12826 (208 -> 172) and 5 on `bull_crash_tick`, where the identical two words cost 5 rows on
   `muis_crash_tick` and 72 on `muis_tick` (86) - four near-copies of one routine, four winners, so
   per function only `src/check.sh` decides. Over the six tick and crash-tick functions that is
   1025 -> 947 instruction rows today; `PROGRESS.md`'s counts are unchanged (124 of 147) because
   none of them reached MATCH. **The compiler-version
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
   2026-11-28: `sound_init` 0x1043a went to MATCH (125) on a Watcom declaration, not a spelling:
   fn_1d2a6 0x1d2a6 is `mov [0x5fe99],eax; ret`, so `#pragma aux fn_1d2a6 parm [edx][ebx][ecx][eax]
   modify exact [eax]` lets easy_init reuse its ECX/EBX/EDX (10 -> 2) and the reversed parm list lands
   the game's argument-materialisation order (2 -> 0). The old rule in open question 26, "WCC
   rematerialises register arguments whatever it knows about the callee", came from probing plain
   `modify`, which only adds to the default clobber set; it is withdrawn and the two levers are in
   `reverse-engineering/watcom-levers.md`. They are worth re-checking against every near miss where the
   game reuses argument registers across a call.
   2026-11-28, two near misses rewritten against their own dataflow; `src/check.sh` was at 124
   MATCH and `PROGRESS.md` reads 124 of 147 (84.4%), 21 near misses, 0 not written, 2 assembly.
   Content, not spelling, is what moves the big ones now: `ai_waypoint_action` 0x1af83 145 -> 64 rows
   because the game has a sixth waypoint action, `f4 == 4` "wait" (both jump tables 0x1af53/0x1af6b
   hold six entries, action 4 is a fallthrough), action 3 is written in both switches but WCC emits
   its body once at 0x1b25e for both: the tables are six entries each, 0x1af53 = 0x1b465, 0x1b465,
   0x1b196, 0x1b157, 0x1b25e, 0x1b2c7 for the `jmp [edx*4+0x1af53]` at 0x1b14f and 0x1af6b = 0x1b465,
   0x1b465, 0x1b389, 0x1b34a, 0x1b25e, 0x1b407 for the `jmp [edx*4+0x1af6b]` at 0x1b342), racer 4's rocket arm reads **script2**'s
   argument not script4's (0x1b3bf `mov edx,[eax+0x57c98]`, a game bug, copied), and the fallback's
   drop kind rides in the argument (`spawn_dropped_object(n, 4)`, -11 rows). `bodies[r].speed` is at
   +0x18, not the +0x1c a research note said: a fixup on a named array hides the displacement from
   every check (GOTCHAS.md, Matching). `player_tick` 0x11eb5 193 -> 189 rows and that is the wall -
   its waypoint-distance `abs()` pair wants its operands mirrored (-4) but 71 rows are register homes
   (game frame 0x10: go [esp], vv [esp+4], m [esp+8], h [esp+0xc], want in EBP; ours 0x14 with h in
   EDI), the `want == 3` body is out of line at 0x120cd, and chained zero stores, both `want`-chain
   spellings and four spellings of the friction test (the game's `imul eax,edi,0x3e8` + `shl eax,0x10`
   against WCC's shift chain for the folded 65536000, the only nmrefs constant difference there) all
   score 193 or worse. `dropped2_tick` 0x178f6 stays at 2 rows.
   2026-11-28: `main` 0x111e8 went to MATCH, `src/check.sh 2>&1 | grep -c ' MATCH at'` = 126 of 147
   (85.7%), 19 near misses listed by check.sh (21 with the two ASM). It was block placement, and the
   lever is a negated test pair: the quit-after-finish fill is `if (!finished2)` with `if
   (!finished4)` inside each arm. Every positive-test spelling of that same behaviour merges the
   game's two `finish_order[1] = 2` stores (794 of 806 bytes, 11-14 rows) and removing any one of the
   three `!` costs a row; 72 spellings in .work/probe/mn, each compiled in place by `tools/wc/inplace.py`
   (0.8 s a compile). Matched with it: the expected module prize is not there - giving init_game
   0x10817, finish_check 0x10eff, start_lights 0x1103e and main their own file (the main.c string
   blocks in `rrr_funcs.tsv`) changes no row count, so start_lights' 3 rows are not an object question
   (open question 27). Tool fix: nmrefs filtered unnamed 0x40000-0x60000 addresses off the game's side
   only, so every MATCHed function that prints a string read `DIFFER`; both sides are filtered now and
   `nmrefs main` says refs 75/75 same with no one-sided jump-target block (GOTCHAS, Matching).
   2026-11-25: `ai_waypoint_action` 0x1af83 stays at 64 rows, 1151 of 1254 B, with its if/else-if
   dispatch. A probe whose racer test is `switch (racer)` gets the game's two missing behaviours (the
   script index homed in a register so the arms re-derive the offset, and two wait bodies) at 39 rows,
   but WCC's switch tree is not the game's two cmp/jne - no spelling of the chain gives the index a
   home. See the differs note in `src/ai.c` and nearmiss.txt.
   2026-11-28: `src/check.sh` is at 129 MATCH (129/147, 87.8%); `src/data.baseline` 9677/9677.
   QUEUE: Q2 (library map) and Q4 (first WLINK) are both marked READY FOR REVIEW - Q4's Accept line
   holds, `link.sh .work/link/q4check $(cat .work/link/libobjs.args11)` -> `LINK: 419179 of 419179
   file bytes identical`, every section IDENTICAL, 9909/9909 fixups identical. `muis_crash_tick`
   0x1a42a is a known hard near-miss (SKIP, 6 rows = 5 home + 1 ret): the else-arm register lever is
   the fixmul **if-arm's IR length** (probe .work/probe/mcd/), confirmed empirically - deleting the
   sin fixmul flips the else arm to the game's allocation (n->EAX), a shared speed temp does not,
   a shared heading temp gives a third pattern (n->EBX). Yet the game's if-arm has BOTH fixmuls with
   byte-identical bytes to our plain form, so it behaves like a short if-arm in the allocator with
   no plain C form found (a paradox). MIRROR: bull_crash_tick's finished2 arm (the same 4-statement
   C) emits n->EAX/ptr->EDX in OUR build - the game's muis pattern - while the game's bull arm emits
   n->EDX/5n->EAX; each function's surrounding structure fixes the other's allocation, so the arm C
   cannot be copied to fix it. A 1-byte length gap (ours 1876 B, game 1877) misaligns the ret. The
   tractable Q5 targets now: muis_tick 0x130f8 (24 rows, 14 home), bull_crash_tick 0x19c8c (42),
   bull_tick 0x12826 (50), ai_waypoint_action 0x1af83 (64), player_crash_tick 0x19497 (114),
   player_tick 0x11eb5 (136); `bull_crash_tick` is `ON:` after 2026-11-28's 32 spellings (floor 42).
4. Later: the rest of the module boundaries in 0x10010-0x1B915 (one source file per original
   module; `src/racer.c` 0x1404a-0x1467b found from epilogue sharing and `src/init.c`
   0x148db-0x1585d from its string block head ('player_direction : %d wp: %d' 0x50e04 runs into
   'init.c' 0x50e22), the shared epilogue and now the jump-table pad - open question
   27 lists the six shares the game has, each pair surely in one module); Davilex's
   libraries (REKEN's debug info names their modules); REKEN, RRRMENU, INTRO.
