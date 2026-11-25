# Work queue (human-owned)

Work the first item that is neither ticked nor marked READY FOR REVIEW, top to
bottom. While any item here is open it overrides STATUS.md `## Next`.

Rules for the agent:

- **Only the human ticks `[x]`.** When you think an item is done, add
  `READY FOR REVIEW: <check or evidence>` under it, and say in STATUS.md which
  check proves it and which mutation makes that check fail.
- An item is done when its **Accept** line holds, not when a theory explains
  the report.
- If new evidence shows an item is wrong or should be split, write that under
  the item. Never delete an item.
- If an item needs the live harness and the harness fails twice in a row,
  write BLOCKED with the reason under it and take the next item that can be
  done offline.

## Items

- [x] **Q1 Matching: RRR.EXE's own C**
  Report: the human asked for a matching decompilation in wacky-re's structure (2026-11-22).
  Accept: every function in 0x10010-0x1B915 is in `src/` (its module's file or `src/rrr.c`), in address order, and `src/check.sh`
  shows it as MATCH or it has a `src/nearmiss.txt` line with a SKIP/LAYOUT status and a
  `/* differs: ... */` note; `PROGRESS.md` regenerated.
  - 2026-08-09: READY FOR REVIEW - `reverse-engineering/tools/entries.py` proved 10 of
    the old 157 starts in the range are byte-scan artifacts (the code before them runs
    straight through the address), so `PROGRESS.md` now reads 147 functions, 110
    byte-identical (74.8%), 22 near misses, 15 to write (the list is in STATUS.md).
  - 2026-11-23: still READY FOR REVIEW, counts moved: sound_init 0x1043a 13 -> 10 rows, the new
    fn_13a78 0x13a78 is a 15-row near miss, and `enter`-frame scan of the C range shows blit_sprite
    0x11bb4 and blit_sprite_trans 0x17d94 are assembly, so 110 of 147, 23 near misses, 14 unwritten
    (13 C). `src/check.sh` still shows 110 MATCH and no WRONG fixups.
  - 2026-11-28: still READY FOR REVIEW. `src/check.sh 2>&1 | grep -c ' MATCH at'` = 124 of 147
    (84.4%), 21 near misses, 0 C functions unwritten; no function went to MATCH today, but the two
    biggest near misses moved on content rather than spelling. `ai_waypoint_action` 0x1af83 145 -> 64
    rows: the game has a **sixth** waypoint action, `f4 == 4` "wait" (both jump tables 0x1af53/0x1af6b
    hold six entries; action 4 is a fallthrough, action 3 is written in both switches but WCC emits its
    body once at 0x1b25e for both), racer 4's rocket arm reads **script2**'s argument not script4's
    (0x1b3bf `mov edx,[eax+0x57c98]`, a game bug, copied), and the fallback's drop kind rides in the
    argument (`spawn_dropped_object(n, 4)`). The shared body is real: the table at 0x1af53 (the `jmp
    [edx*4+0x1af53]` at 0x1b14f) is 0x1b465, 0x1b465, 0x1b196, 0x1b157, 0x1b25e, 0x1b2c7 and the one at
    0x1af6b (the `jmp [edx*4+0x1af6b]` at 0x1b342) is 0x1b465, 0x1b465, 0x1b389, 0x1b34a, 0x1b25e,
    0x1b407 - six entries for f4 + 1 = 0..5, entry 4 the shared boost body, entry 5 the wait body
    (0x1b2c7, which zeroes bodies[r].speed at 0x1b2f0 and calls aim_at_player at 0x1b2fa). Mutations
    that fail it: dropping the wait arm, or two literal `spawn_dropped_object(racer, 0)` calls instead
    of the kind in the argument (0x1b064 `mov edx,4` + `mov eax,ebx` into the one call at 0x1b06b, which
    the `xor edx,edx` arm at 0x1b012 jmps to: 75 rows). `bodies[r].speed` is at
    +0x18, not the +0x1c a research note claimed - a fixup on a named array hides the displacement from
    objmatch, fndiff and nmrefs alike, so `bodies[r].f1c = 0` and `bodies[r].speed = 0` score the same
    rows (GOTCHAS.md, Matching). `player_tick` 0x11eb5 193 -> 189 on the `abs()` operand mirror alone;
    its rest is 71 register homes, and chained zero stores, both `want`-chain spellings and four
    spellings of its friction test score 193 or worse. `dropped2_tick` 0x178f6 stays at 2 rows.
  - 2026-11-27: still READY FOR REVIEW, counts moved: `src/check.sh 2>&1 | grep -c ' MATCH at'` =
    124 of 147 (84.4%), 21 near misses, 2 unwritten. The lever is a shared index holder:
    `dropped2_tick` 0x178f6's FIRST damage block reads its stats index through the `int h` its aim
    block uses (`h = bodies[2].stats; stats[h].damage -= 10;`), which makes Watcom emit the
    `*3 << 4` chain once and puts its epilogue at the game's 0x17b59. Seven functions went to MATCH
    with it (dropped0_tick 0x17b61, dropped4_tick 0x17c74, mine_tick, sign_tick, bomb_tick,
    fn_16e46, cactus_tick) and dropped2_tick itself is 16 -> 2 rows at the exact 619 bytes. The
    mutation that makes the check fail: `stats[bodies[2].stats].damage -= 10;` back in that block
    (dropped2_tick 16 rows, the other seven differ again); a separate `int s` also fails (44 rows).
  - 2026-11-27: still READY FOR REVIEW, and every C function of the range now has a spelling:
    `src/check.sh 2>&1 | grep -c ' MATCH at'` = 117 of 146 (80.1%), 28 near misses, 1 unwritten
    (blit_sprite 0x11bb4, assembly). The six ticks were written the same day: the crash ticks in
    `src/tick.c` (muis_crash_tick 70 rows, bull_crash_tick 188, player_crash_tick 282) and the normal
    ticks in a second module `src/drive.c` (player_tick 193, bull_tick 208, muis_tick 87). The check
    that proves the module split: with the six in one `src/tick.c`, muis_tick goes 87 -> 91 and
    player_tick 193 -> 200, because WCC then merges the tail of a normal tick into a crash tick's
    epilogue, which the game never does (the three ticks and the three crash ticks are each laid out
    back to back, with other modules between).
  - 2026-11-27: the compiler version is settled and removes one candidate explanation for the near
    misses: RRR/REKEN/INTRO link **10.6**'s clib (118/113/98 byte-identical CLIB3R modules vs 10.5's
    108/104/89, `prtf` 3038 B and `grownear` 1466 B among the ten only 10.6 has), but `src/check.sh`
    reads the same 117 MATCH and 28 near misses with 10.5 and with 10.6
    (`WC=$PWD/.work/watcom/dos/C106/WATCOM src/check.sh`), and `tools/wc/segcmp.py` finds 0 non-fixup
    byte differences between the two objects of every module - so the register-choice near misses are
    C questions after all. Install recipe in `reverse-engineering/watcom.md`.
  - 2026-11-23: podium 0x15a9f byte-identical: `src/check.sh 2>&1 | grep -c ' MATCH at'` says 111 and
    `grep podium` shows `podium_ 355 bytes MATCH at 0x015a9f` with no WRONG fixup (the world_names
    0x57c18 and rcascii_printf 0x200b0 targets agree); deleting either switch level or one vararg of
    the "%-15s : %3d" line makes the check fail. 111 of 147, 23 near misses, 12 unwritten.
  - 2026-11-23: dropped2_tick 0x178f6 SKIPped after the budget: a synthetic probe series
    (.work/probe/S.C) pins the mechanism - one damage statement compiles in the game's load-EAX
    shape, a second same-shape one flips the first to our load-EDX form; the game's rocket_tick
    has the flipped shape and matches, so the game's dropped2 differs in a block-2 statement
    shape that stays RMW-coded; assign-form block 2 fixes block 1 but not itself. check.sh
    still 112 MATCH.

  - 2026-11-23: race_end 0x1585e byte-identical: `src/check.sh | grep -c ' MATCH at'` = 112, `grep
    race_end` shows `438 bytes MATCH at 0x01585e`, no WRONG fixup; rc_error's line numbers are
    603/608/613 and writing 609 makes the check fail (1 row). Also: `fn_1963e` 0x1963e is not a
    function but a basic block of player_crash_tick 0x19497 (its only "call site" 0x18f3e is the
    immediate of `mov eax, 0x6fbe8`), so those two PROGRESS.md rows (423 + 1614 B) are one 2037 B
    function: 10 C functions really remain.
  - 2026-11-25: find_waypoints 0x1517a and find_bonus_objects 0x15597 written (190 and 1 rows - the
    latter is one jmp, its epilogue is find_waypoints' pop/ret at 0x15565, so it MATCHes as soon as
    find_waypoints does), and ai_waypoint_action 0x1af83 written in the new `src/ai.c` (145 rows).
    `src/check.sh | grep -c ' MATCH at'` stays 112; PROGRESS.md: 26 near misses, 9 unwritten (7 C).
    New levers in MATCHING.md: the explicit `default: break;` (find_bonus_objects 32 -> 0 rows) and
    "a function's neighbours decide its tail".
  - 2026-11-24: init.c found and find_bonus_objects 0x15597 byte-identical: find_waypoints 0x1517a
    rewritten with `unsigned short w` and no n local (`(w & 0xff)` where used) goes 190 -> 0
    instruction rows, and the two functions move to `src/init.c` (their own 'init.c' `__FILE__`
    strings, and they share the epilogue at 0x15565). `src/check.sh | grep -c ' MATCH at'` = 116,
    `grep find_bonus` shows `MATCH at 0x015597 (fixup bytes not compared; 1 rel32 branch compared
    by target)`, no WRONG fixup. Two mutations make it fail: `int w` + an `n` local again (177 rows,
    tail jmp wrong again at +207), or moving either function back to `src/rrr.c` (the merged epilogue returns).
    find_waypoints is a near miss on one pad byte only (open question 30).

  - 2026-11-23: cell_effect 0x18b8d byte-identical: `src/check.sh | grep -c ' MATCH at'` = 113, `grep
    cell_effect` shows `1536 bytes MATCH at 0x018b8d` with no WRONG fixup, and map.c's fixups went from
    3 to 90 named targets agree. Two mutations make it fail: case 16 written as `damage >= 1 && fuel
    >= 1` instead of a verbatim copy of case 15 (87 rows), and the case-22 store as `(y * map_w + x)`
    instead of `(x + y * map_w)` (1 row, `lea [ebx+edi]` vs the game's `[edi+ebx]`). New levers in
    MATCHING.md: duplicate an identical case body word for word; operand order picks the `lea` SIB pair.
    113 of 147, 25 near misses, 9 unwritten (7 C); `find_bonus_objects` 0x15597 and `racer_bounce`
    0x14534 now sit at 0 instruction rows but one jmp fixup each, and `ground_check` 0x1889d moved
    35 -> 41 rows when cell_effect's tail grew.
  - 2026-11-23: racer_contact 0x1404a and racer_bounce 0x14534 byte-identical: `src/check.sh | grep -c
    ' MATCH at'` = 115. Three causes fixed: the first corner test passes x1, y2, x2, y1 (the old
    `x1, y1, x2, y1` tested a corner that is not there - left_of's matched body says which register
    holds which parameter), the corners are declared AND written x1, y2, x2, y1 (in `src/racer.c` the
    other 23 declaration orders score 6-12 rows; in rrr.c before the split that order scored 5 and the
    other 23 of its sweep 11+, and the 24 statement orders scored 14-52), and the four functions
    0x1404a-0x1467b moved
    to a new `src/racer.c` - racer_bounce jmps into racer_contact's epilogue, and WCC386 shares an
    epilogue only inside one object, so those four are one module and `racer_contact`'s tail cannot
    live in rrr.c, where it merged into main's. Putting them back into rrr.c breaks both again
    (racer_contact 5 rows, racer_bounce 0 instruction rows but the wrong jmp target), and the old
    ordering scores 22 rows and the old argument list 59 here. New lever in MATCHING.md and new open question 27: the game shares an
    epilogue exactly 6 times in 0x10010-0x1B915 (list there), and each pair must share a module - the
    first hard tool for the module boundaries of STATUS item 4. 115 of 147, 23 near misses, 9
    unwritten (7 C).

  - 2026-11-25: READY FOR REVIEW - the last gap in the Accept line is closed. `blit_sprite` 0x11bb4
    and `blit_sprite_trans` 0x17d94 are the game's own assembly, not C, and now say so: two `ASM:`
    lines in `src/nearmiss.txt` (the only two `enter` stack frames in 0x10010-0x1B915, arguments
    re-read from EBP every row, four-NOP alignment runs, none of which `-3r -s -ox` emits; capstone
    scan in research/discoveries.md "Executables"). `tools/progress.py` reads those statuses, so
    `PROGRESS.md` reads 124 of 147 (84.4%), 21 near misses, 0 not written, 2 assembly. The check:
    `grep -c 'not in src/ yet' PROGRESS.md` = 0 while `src/check.sh 2>&1 | grep -c ' MATCH at'` stays
    124 (no C changed, `src/matched.baseline` untouched). The mutation that makes it fail: delete the
    `ASM:` line for blit_sprite - that row goes back to "not in src/ yet" and PROGRESS.md counts 1 not
    written. Every function of the range is now MATCH, a near miss with a SKIP/LAYOUT status, or ASM.
