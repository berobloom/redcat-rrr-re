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
  - 2026-11-28: still READY FOR REVIEW. `main` 0x111e8 byte-identical: `src/check.sh 2>&1 | grep -c '
    MATCH at'` = 126 of 147 (85.7%), 19 near misses (plus the 2 ASM). One shape carries it: the
    quit-after-finish fill is `if (!finished2)` with `if (!finished4)` inside each arm. Mutations that
    fail: delete any one of the three `!` (1 row each, 806 bytes); spell the same behaviour with a
    positive test (the spelling that was in the tree, and 71 more in .work/probe/mn: 794-833 bytes,
    6-30 rows - every positive-test form merges the game's two `finish_order[1] = 2` stores). Checked
    and rejected: making init_game/finish_check/start_lights/main their own object (the rrr_funcs.tsv
    main.c string blocks) changes no row count, so that boundary stays unproven and unsplit (open
    question 27). `nmrefs main` reads refs 75/75 same with no one-sided jump-target block - after
    fixing nmrefs, which filtered unnamed 0x40000-0x60000 addresses on the game's side only and so
    called every string-printing MATCH a DIFFER.
  - 2026-08-09: READY FOR REVIEW - `reverse-engineering/tools/entries.py` proved 10 of
    the old 157 starts in the range are byte-scan artifacts (the code before them runs
    straight through the address), so `PROGRESS.md` now reads 147 functions, 110
    byte-identical (74.8%), 22 near misses, 15 to write (the list is in STATUS.md).
  - 2026-11-23: still READY FOR REVIEW, counts moved: sound_init 0x1043a 13 -> 10 rows, the new
    fn_13a78 0x13a78 is a 15-row near miss, and `enter`-frame scan of the C range shows blit_sprite
    0x11bb4 and blit_sprite_trans 0x17d94 are assembly, so 110 of 147, 23 near misses, 14 unwritten
    (13 C). `src/check.sh` still shows 110 MATCH and no WRONG fixups.
  - 2026-11-28 (later): `sound_init` 0x1043a byte-identical: `src/check.sh 2>&1 | grep -c ' MATCH at'`
    = 125 of 147 (85.0%), 20 near misses. What it needed was a declaration, not a spelling:
    `#pragma aux fn_1d2a6 parm [edx][ebx][ecx][eax] modify exact [eax];` in src/rrr.h. Two facts behind
    it: fn_1d2a6 0x1d2a6 is `mov [0x5fe99],eax; ret` (clobbers only EAX), and WCC materialises a call's
    arguments in reverse `parm` order. Mutations that fail: deleting the pragma (2 rows), `modify [eax]`
    instead of `modify exact [eax]` (10 rows - plain `modify` only adds to the default clobber set, so
    the 2026-11-27 probes of this were inert and open question 26's rule is withdrawn), and
    `parm [eax][ecx][ebx][edx]` (6 rows, the reverse order). 2026-11-27's `#pragma aux` probes are
    retracted: they used plain `modify`, which could not have worked.
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
  - 2026-11-25: no count moved (126 of 147). `ai_waypoint_action` 0x1af83 stays at 64 rows, 1151 of
    1254 B, because the C kept here has the game's two-cmp/jne dispatch. A probe that writes the racer
    test as `switch (racer)` reproduces the two behaviours still missing (the script index homed in a
    register, 0x1af8d `mov ecx,eax` in the game and `mov esi,eax` in the probe, so each arm re-derives the `i*24` offset; the two wait bodies are
    two copies) at 1197 B / 39 rows (.work/probe/awa3 w4.c) - mutation that rules it out: WCC lowers
    that switch as a cmp/jb + cmp/jbe + cmp/je tree with a second epilogue copy, and no spelling of the
    if/else-if chain (nested, else-if, separate ifs at 385) ever homes the index. Open: how the game
    gets both.
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

  - 2026-11-25: bookkeeping - `src/matched.baseline` 124 -> 125. The sound_init commit raised the count
    in its message and in `PROGRESS.md` but left the baseline at 124; no code changed here. Check:
    `src/check.sh 2>&1 | grep -c ' MATCH at'` equals `src/matched.baseline` (125 MATCH, 20 near misses,
    2 ASM = 147). Mutation that fails it: delete `#pragma aux fn_1d2a6 parm [edx][ebx][ecx][eax]
    modify exact [eax];` from `src/rrr.h` and the count drops to 124.
    start_lights 0x1103e stays a 3-row near miss after ~2400 spellings (.work/probe/sl2); the tie now
    has a measured mechanism and a lead - open question 33, discoveries "Toolchain: switch bodies and
    `lea` operands". Same sweep also found parallel WCC386 runs score wrong (GOTCHAS "Probes"), so the
    numbers quoted there are all from one-compile-at-a-time runs.

- [ ] **Q2 Map RRR.EXE's unidentified library code**
  Report: the human asked what a full-game byte match still needs (2026-11-25); 128,477 bytes of
  0x1B916-0x4142F are neither the game's C nor Watcom's clib/math.
  Accept: `reverse-engineering/analysis/rrr_libmap.tsv` gives every byte of 0x1B916-0x4142F one
  owner - clib, math, Davilex WINLIB/SOUNDLIB (by source module where evidence allows), Fastgraph,
  Smacker, DOS/4GW/Watcom start-up, padding, or `unknown` - with evidence per range; `PROGRESS.md`
  reports the split (generated by `tools/progress.py`); the module boundaries REKEN's debug info
  gives for the shared Davilex modules are decoded and used.

  - 2026-11-27: READY FOR REVIEW. `rrr_libmap.tsv` (generated by `tools/libmap.py`, rerun by
    `tools/progress.py`) gives every byte of 0x1B916-0x4142F exactly one owner: 504 rows,
    154,394 bytes total (= 0x4142f - 0x1b916 + 1), contiguous with no gaps and no overlaps
    (verified by script: first row 0x1b916, last row ends exclusive at 0x41430). Owners: clib,
    math, emu, startup, fastgraph, smacker, soundlib, winlib, davilex-asm, davilex-diff,
    unknown (8,035 bytes, 5.2%); every row carries an evidence column (libmatch / reken /
    smacker / strings / -) and a name where known. REKEN's Watcom debug block is decoded by
    `tools/wdbg.py` and its module boundaries applied by `tools/modmatch.py REKEN.EXE
    RRR.EXE --funcs` (the winlib/soundlib rows carry REKEN's own paths, e.g. C:\PROTMOD\WINLIB\
    mgl2\fg2\stdlib.c). Check: `python3` contiguity pass over the TSV reports 0 gaps, 0
    overlaps, total == 154394; `src/check.sh` regenerates PROGRESS.md's split via progress.py.
    Mutation that fails it: deleting one TSV data row drops the total below 154394 and opens a
    gap; libmap.py rerun restores it.

- [ ] **Q3 Compare the data object**
  Report: same question; `objmatch.py` compares code only, so initialised data, strings and BSS
  size are never checked.
  Accept: `src/check.sh` also compares each object's `_DATA`/`CONST` contributions against the
  game's data object (by address from the names file) and prints a count; the count is recorded
  in a baseline like `matched.baseline`.

  - 2026-11-25: READY FOR REVIEW. `objmatch.py` compares every data segment of an object with the game's
    data object at the base the object's references give (fixup bytes masked; a segment nothing
    references is searched for), and `src/check.sh` ends with `DATA: N of M data-segment bytes identical
    in place`; `src/data.baseline` = 1946 (of 5,861: heap, map, new2io, order, raceanim, shade, shadeobj,
    shadeobj2 CONST identical; hiscore, init and rrr.c differ). Check: `src/check.sh | tail -1` equals
    the baseline. Mutation that fails it: `illegal free` -> `illegal frEe` in src/order.c line 63 gives
    1834 (order.c's CONST: 111 of 112). `grep -c ' MATCH at'` still counts functions only (126); data
    lines say IDENTICAL. Only CONST exists today: the game's initialised globals and BSS are not
    defined in src/ yet, so the measure covers 5.9 KB of a 0x3b478-byte data object.

- [ ] **Q4 First WLINK of RRR.EXE**
  Report: same question; there is no link step, so a whole-file match cannot be measured.
  Accept: a script links `src/` objects + the Watcom 10.6 start-up and libraries (plus the game's
  own bytes for the parts not rebuilt yet) into an LE executable, and a tool reports how many bytes
  of the code object, data object and fixup table equal RRR.EXE's.

  - 2026-11-25: first link done, not yet the Accept line. `reverse-engineering/tools/link/link.sh`
    carves RRR.EXE into CODEBLOB/DATABLOB.OBJ (blob.py), links them with WLINK 10.6 under Wine
    (RRR.LNK: os2 le, wstub, modname rrr, preload code and data) and compares (lecmp.py): `LINK: 419179
    of 419179 file bytes identical`, `cmp` clean. Mutation: drop the two `preload` lines -> 419177.
    Open: link src/ objects and the Watcom/Fastgraph LIBs in place of blob ranges (blob.py's
    `--keep-out`, which needs a blob fixup into a replaced range to go through that object's public).

  - 2026-11-25 (later): real objects link in place of blobs. `link.sh OUTDIR --obj FILE.OBJ@ADDR ...`
    (blob.py cuts the code blob around each object, defines the externals it needs at the addresses
    read back from the game, routes blob fixups into it through its publics). Ten 10.6 clib modules
    (sprintf..fwrite, 0x1b916-0x1c5af, 2,476 B) linked as themselves: still `cmp`-identical.
    Mutation: 10.5's sprintf instead -> 419178 (the one-byte fingerprint at 0x1b918).
    `tools/link/libplace.py` picks 117 code-only modules (clib, math, emu, Fastgraph 4.03); linking
    all of them stops on two Fastgraph externals, PMhugeincr (VBPASTE/ERASE.ASM) and mouse_y
    (MOUSEMOV.ASM), which blob.py does not define: libmatch.fixups_of does not list how these asm
    modules reference them (look at their FIXUPP records/threads, or a non-code segment). Not done
    yet either: objects with data (37 library modules, every src/ object) - needs the original
    segment order inside the data object.

  - 2026-11-25 (later still): the two Fastgraph externals are solved and **103 of the 117 modules now
    link as themselves with the result still byte-identical**: `link.sh OUTDIR $(cat
    .work/link/libobjs.args4)` -> `LINK: 419179 of 419179 file bytes identical`. Report:
    - PMhugeincr/mouse_y are **dead EXTDEF entries**: the 4.03 ASM modules list them, their code never
      references them (each placed module's fixup sites equal RRR's LE sites exactly, nothing hidden
      in FIXUPP threads - the 0x88 records every FG ASM module carries are boilerplate `00a201`).
      WLINK still requires a definition for every listed EXTDEF. Both come from FG32.LIB **DATA.ASM**,
      one 0x44e0 `_DATA` at game **0x60a56** holding Fastgraph's globals plus the PM* helpers as code
      in that segment (PMhugeincr +0x71a, mouse_y +0x8c). `blob.py --lib LIB` now defines such names
      at defining module's segment + PUBDEF offset, the segment anchored by `needs()` addresses of
      sibling names (mouse_drv/vis/x and PMzseg all give 0x60a56).
    - 14 modules stay in blobs. inirt386: its 4 fixups target XIB/XIE/YIB/YIE, zero-size in itself;
      their bytes merge from 13 unplaced data-carrying clib modules (clockxib, stinit, argcv386,
      _8087386, fltused, stk386, delay386, environ, inirmsel, p5prof, ...) at game 0x6c668/0x6c6a4/
      0x6c6b6 - the data-segment-order problem, and WLINK resolves it silently wrong (no error).
      The 13 at 0x29c82-0x29fd6 and 0x3d266-0x3d472: on code pages 0x29000 and 0x3d000 the original
      feed order is not placement order (Smacker-region sites fed before the fpe*/fputs sites below
      them, mouse.c's sites after the clib modules above it), and FILES order controls placement too
      (swapping two objects in FILES.LNK moves their code), so no real-object placement reproduces
      those two pages; they link exactly as blobs. Mutation that fails the check: put
      clib3r_fpeintel.OBJ@3d3f4 back -> fixup records drop to 81862 of 84546 (pages stay identical,
      all 9909 fixups semantically right).

  - 2026-11-25: **the data-object segment order is known**, and `inirt386` now links as itself -
    **104 real objects still byte-identical**: `link.sh OUT $(cat .work/link/libobjs.args4)
    --obj .work/exp/.../clib3r_inirt386.OBJ@32d22 --dseg XI@6c668 --dseg YI@6c6a4-12
    --dseg DATA32@6c6b8` -> `LINK: 419179 of 419179`, `fixups: 9909 in ours, 9909 in the game, 9909
    identical`. New tool `tools/link/dataobj.py` content-matches every candidate module's data
    segments against the game's data object (linker-filled bytes masked on both sides): it pins 23
    segments - CONST 10, CONST2 2, _DATA 9, DATA32 2 - and its last section reports **0 out of
    code-address order** per segment name, so WLINK's data object is one run per segment name and
    inside a run the modules come in the order they were fed, the same order as the code object
    (dosret._DATA 0x60370/code 0x28a0c then SETMODE.ASM._DATA 0x60384/0x28ac1; DATA.ASM._DATA
    0x60a56 between SVGAINIT 0x6050c and MAKEGIF 0x64f9c). WLINK's own map names the runs and their
    fixed order - CONST, CONST2, _DATA, XIB, XI, XIE, YIB, YI, YIE, DATA32, _BSS, STACK - each run
    starting on a 4-byte boundary (the game's YI ends 0x6c6b6, DATA32 starts 0x6c6b8), which is
    exactly the order the content matches give. `blob.py --dseg NAME@ADDR[-LEN][:CLASS]` carves the
    data blob into those runs. Why that fixes inirt386: **XIB/XIE/YIB/YIE are WLINK's own symbols**,
    the bounds of the merged XI and YI runs (no clib module has them as a public), so without XI/YI
    runs of the game's length its four fixups land on the end of DGROUP, silently. The game's XI run
    = 10 far pointers 0x6c668-0x6c6a4, YI = 3 more 0x6c6a4-0x6c6b6, and their bytes are all fixup
    sites, so the blob supplies them. Mutations that fail the check: drop the XI block -> 419177,
    drop the YI block -> 419173, move the XI block to 0x6c600 -> 419177; and emitting the data-blob
    fixups run by run instead of in page input order -> fixup records 83227 of 84546.
    Still open for the 43 data-carrying modules and every src/ object: placing a real data segment
    byte-exactly. What is done (2026-11-26): `blob.py --obj F.OBJ@code,NAME=data` places a real
    object's data segment, cuts the data blob into one object per gap (FILES order = merge order),
    routes blob fixups that aim inside a placed segment through that object's data publics, shrinks
    STACK by the placed `_BSS` lengths, reads a module's data fixups too (`libmatch.fixups_of(
    only_code=False)`), and feeds every piece as late as its run allows - with the XI/YI blocks after
    inirt386, which is what lets WLINK create XIB/XIE/YIB/YIE around the run. The 104-object link is
    still `LINK: 419179 of 419179` with this machinery (`$(cat .work/link/libobjs.args5)`).
    What is left: the piece after a placed segment starts 2 bytes late (dosret 0x28a0c/_DATA@60370 +
    SETMODE 0x28ac1/_DATA@60384 give `LINK: 405807 of 419179`, first data difference 0x10418, _DATA
    run 0x1c66a vs 0x1c668) - WLINK re-aligns after a real segment, so try the blob pieces' SEGDEF
    alignment (real objects use 0x99 align 5 / 0x98 align 1, blobs use 1). Then repeat over the 43
    modules in `.work/exp/dmods.skip`, data addresses from `tools/link/dataobj.py`.

  - 2026-11-26: **143 real objects link as themselves, 39 of them carrying data, BSS or XI/YI
    entries** - `link.sh OUT $(cat .work/link/libobjs.args6)` -> `LINK: 419179 of 419179`, `fixups:
    9909 identical`, every region IDENTICAL. args6 = args5 (the 104) + `dataplace.py`'s output for the
    data modules, minus three, + `--dseg CONST@50000 --dseg CONST2@552e4 --dseg _DATA@55338 --dseg
    _BSS@6c96c:BSS`. What it took:
    (1) the 2 bytes were not the blob's alignment: the clib modules fed between SETMODE and the next
    blob piece declare *empty* CONST/_DATA/... with dword alignment, and WLINK pads the run for them
    (0x6040a -> 0x6040c). blob.py now feeds a gap's head up to the next 4-byte boundary straight after
    the placed segment it continues, the body still goes late.
    (2) pieces of different segment names whose feed windows overlap share one object (fed at the
    narrowest window's end), so emit_ordered interleaves their fixups as the game's page lists do
    (page 0x6c000: the _DATA tail with XI/YI/DATA32; three blob modules each bringing XI and YI
    entries ahead of isdbcs).
    (3) new `tools/link/dataplace.py` places every data segment from the module's own fixups: the
    game's LE fixup target at each site of the module that reaches the segment (or one of its own
    publics, reached as an EXTDEF), minus the addend; and backwards for XI/YI entries and pointer
    tables (the LE sites aiming at the module's placed targets, intersected over the segment). 56
    segments of 41 modules, 51 by self-reference, 5 by pointers, none by content alone; every run
    comes out in code order, and no segments overlap.
    (4) `_BSS` placed too (`--dseg _BSS@6c96c:BSS`, blob BSS pieces are lengths without LEDATA);
    appending the real _BSS at the start of the run moved all of the game's own BSS (6426 of 9909
    fixups right). STACK then starts at the 4-byte boundary after sigsy's _BSS (0x852d0) and comes
    out 0x61a8 = 25,000 bytes, a round number.
    Left out: clib nomem (its 18-byte _TEXT is noefgfmt's twin at 0x33abf, and 0x54e0c holds
    noefgfmt's 36-byte CONST, so nomem is LIKELY not in the game); emu387 386inite (no code hit in
    RRR); math387 _matherr and clib fpeinth (code on page 0x3d000, where Smacker's fixups come first
    in the game's input order: with fpeinth in, fixup records 83938 of 84546).
    The CONST2/_DATA boundary (0x55338; since main.c links as itself, 0x55400 - see the 2026-11-26 main.c note) and the CONST start (0x50000) are SPECULATIVE: blob bytes
    only, any 4-aligned choice between the placed segments links the same bytes; 0x55338 is the
    one that also gives the fixup order (0x5ff7c put the game's _DATA fixups at 0x5fdf4 in the
    CONST2 piece).
    Mutations that fail the check: iomode `_DATA=6a00c` instead of 6a008 -> 405865 (data object 4
    bytes long); `BLOB_NOHEAD=1` on the dosret+SETMODE pilot -> 406055; fpeinth back in -> 418571;
    without the `_BSS` block blob.py refuses; rand's `_DATA` moved -> refused (a blob fixup at
    0x6007a lands inside it with no public to reach it). Regression: args5 alone still 419179.

  - 2026-11-26 (later): **the first src/ objects link as themselves: 147 real objects**, `link.sh
    OUT $(cat .work/link/libobjs.args7)` -> `LINK: 419179 of 419179`. args7 = args6 + `dataplace.py
    .work/link/srcobj/{ORDER@1195f,SHADE@11c11,RACEANIM@15c02,HUD@1918d}.OBJ` (copies of check.sh's
    .work/src objects: the four whose whole _TEXT is the game's, CONST placed by self-reference at
    0x50cac/0x50d1c/0x51258; HISCORE's code also matches, at 0x1b469, but it reaches its CONST at
    two bases, and check.sh has 72 of 130 CONST bytes agreeing). Two blob.py fixes it needed:
    (1) **WLINK lays out the data runs in the order it first hears their names**, not in a fixed
    order: the first data object fed (a CONST piece grouped with a _DATA piece) put _DATA ahead of
    CONST2 (`322770 of 419179`). The first code blob now names CONST, CONST2 and _DATA, empty and
    byte-aligned, in the game's order.
    (2) input_order's block split was one valid partition of several. With one blob feeding a page,
    any of them rebuilds it; a real object in between needs the true one (`418388`, page 0x11000:
    0x11f0f and 0x11ffe moved). It now takes the partition with the fewest address descents. On
    0x11000 that is strictly ascending.
    With the new split, fpeinth/_matherr still break page 0x3d000 (417547, with `--dseg
    _DATA@553ac` for _matherr's CONST2): no valid split puts Smacker's sites (0x3d50b-0x3dff2) after
    theirs, so that exception is real.
    Next: the other src/ modules as they reach a whole-module match, then the 0x3d000 page.

  - 2026-11-26 (later): **149 real objects**, `$(cat .work/link/libobjs.args8)` -> `LINK: 419179 of
    419179`. hiscore.c was two modules: new `src/guard.c` (set_keys, joystick_to_keys,
    check_object_nr, check_waypoint_nr, 0x1b469-0x1b6bf, CONST 60 B at 0x51a5c) and hiscore.c
    (high_score, high_load, and `high_names`, now defined there: CONST 91 B at 0x51a98, _DATA 12 B at
    0x5fdf4). Both are now whole-module and data-identical (src/data.baseline 1946 -> 2109). The
    evidence: Watcom emits an initialiser's literals before any function's, so in one object
    high_names' strings came first, and the game has the guards' strings first; the guards' 60
    bytes end on a 4-byte boundary, exactly where "RedCat" begins. Mutation that fails: one object
    again (hiscore.c as before) -> its CONST 72 of 151 bytes in place, data 1958. The four joystick
    calls are Fastgraph's (fg_initjoy/fg_button/fg_getxjoy/fg_getyjoy, CONFIRMED by the linked
    FG32 modules; as fn_2779e etc. WLINK left them undefined).

  - 2026-11-26 (later): **150 real objects**, `$(cat .work/link/libobjs.args9)` -> `LINK: 419179 of
    419179`. New `src/rrrio.c` = rrr.c's 0x1ab7f-0x1af52 (rrrio_save .. ai_script_find, 9 functions,
    all MATCH) plus `game_names` (0x57c04) and `world_names` (0x57c18), now defined there: CONST 213 B
    at 0x51984 and _DATA 44 B at 0x57c04 both identical in place (data.baseline 2109 -> 2366).
    Evidence: the module's CONST starts right after map.c's padding with those tables' strings, and
    its "wb"/"RRRIO.INF"/"rb" sit unaligned at 0x51a49 after them. Without the tables the object
    starts its CONST on a fresh dword (0x51a4c) and the link breaks (381533, data object 4 bytes
    long). module_table moves to 0x57c30 and is read `[world][i - 1]` (the code's base 0x57c2c was
    that -4 folded in, and it fell inside world_names).
    0x10010-0x11526 (sndman.c #included in main.c, discoveries) is one module and waits on
    start_lights (3 rows, nearmiss). The other whole-module candidates are one function short each,
    and each of those is a long-standing register-tie SKIP: HEAP (spawn_dropped_object), MAP
    (ground_check), NEW2IO (draw_view), SHADEOBJ (hit_test). rrr.c's race_end/winner_film/podium
    (0x1585e-0x15c02) are init.c's: they share a merged literal with it (0x50e22) and their strings
    continue its CONST (0x5113b -> 0x5116a), so init.c = 0x148db-0x15c02, waiting on init_objects.

  - 2026-11-26 (main.c): **151 real objects**, `$(cat .work/link/libobjs.args11)` -> `LINK: 419179 of
    419179`. start_lights matched (two pointer locals, MATCHING levers), so 0x10010-0x11526 is whole:
    new `src/main.c` (22 functions, all MATCH) with its _DATA defined there in address order - the two
    1309-byte font blocks, sndman's tables (blaster_ports has 8 entries, not 6), main's ints. CONST
    3115 B at 0x50004 and _DATA 2995 B at 0x55400 identical in place (data.baseline 2366 -> 8476).
    The CONST2/_DATA seam moves from 0x55338 to 0x55400: with 0x55338 the 0x5538c-0x553a8 table
    (a library's, read from 0x3d208) is a _DATA piece that has to be fed before main.c, and page
    0x55000's fixups come out in the wrong order (418994); 0x553a8 -> 419028; anything from 0x553ac
    to 0x55400 links byte-identically. So that table is CONST2 (CONFIRMED by the fixup order) and the
    seam is 0x553ac-0x55400 (0x55400 chosen: main.c's first variable). Two calls had placeholder
    names WLINK could not resolve once their modules are real: fn_1b469 is guard.c's set_keys and
    fn_20d1f Fastgraph's _fg_makepcx. blob.py writes a fixup's addend modulo 2^32 (a target below the
    nearest public of a real object).
  - 2026-11-26 (init.c): src/init.c now holds the whole module 0x148db-0x15c02 (start_crash,
    switch_anims, fn_15166, race_end, winner_film, podium out of rrr.c). Its CONST (1106 B at
    0x50e04) is identical in place after find_waypoints' two "...SNITCH:%d" messages lost a "\n"
    the game never had: `DATA: 9677 of 9677` (data.baseline 8476 -> 9677). init_objects 7 -> 2 rows
    (fuel into `i` before the drops store); its last 2 rows (MUIS heading store slot) keep init.c
    from linking as itself.

  - 2026-11-27: READY FOR REVIEW. The Accept line holds: `reverse-engineering/tools/link/link.sh
    .work/link/q4check $(cat .work/link/libobjs.args11)` links 151 real objects (src/ modules
    MAIN/ORDER/SHADE/RACEANIM/HUD/GUARD/HISCORE/RRRIO/INIT plus the Watcom 10.6 clib/math/emu and
    Fastgraph library modules, inirt386, and the data-carrying dmods) plus the game's own bytes for
    the not-yet-rebuilt ranges (blobs) into an LE executable, and `tools/link/lecmp.py` reports the
    byte equality: `LINK: 419179 of 419179 file bytes identical`, with every section IDENTICAL (MZ
    stub 11408, LE header 172, object table 48, page map 316, resident names 8, fixup page table
    320, fixup records 84546, object 1 pages 204800, object 2 pages 117099), `LE header fields
    differing: 0`, and `fixups: 9909 in ours, 9909 in the game, 9909 identical (source, type,
    target)`. Check: run the link.sh command above; its last line is the score. Mutation that fails
    it: drop the two `preload` directives from RRR.LNK -> 419177; or put a library module back to a
    blob (e.g. remove one `--obj` from args11) and the carved range no longer matches a real module
    public, moving fixup sites.

- [ ] **Q5 The six tick functions and ai_waypoint_action**
  Report: same question; these are the only near misses with one-sided rows (wrong C), per
  `tools/wc/nmclass.py`.
  Accept: each one is MATCH, or SKIP with nmclass showing home rows only.
  - 2026-11-25: READY FOR REVIEW. `tools/wdbg.py` decodes REKEN's debug block (200 modules, 1,550
    globals, 350 address ranges); `tools/modmatch.py` finds REKEN's modules/functions in RRR with
    fixups masked; `tools/libmap.py` writes `reverse-engineering/analysis/rrr_libmap.tsv`, one owner
    and one evidence source per range, and `tools/progress.py` reruns it for PROGRESS.md's table.
    Split of the 154,394 bytes: Smacker 28,761, clib 26,254, Fastgraph 24,057, SOUNDLIB 16,918,
    Davilex functions that differ from REKEN's copy 14,975, emu387 12,850, WINLIB 12,309, math
    3,711, Davilex asm 3,154, start-up 2,992, unknown 8,413. Check: the TSV's ranges are contiguous
    and sum to 154,394 (`awk -F'\t' 'NR>1{t+=$3} END{print t}'`). Mutation that fails it:
    `SMACKER_C = (0x21c00, ...)` stops libmap with "Smacker range called from 0x21b58, outside
    smacker.c". PROGRESS.md's headline now counts only libmatch-verified library bytes (58,990,
    29.2%); emu387 and cstrt386 _TEXT match REKEN only, not 10.6's LIBs.
  - 2026-11-26 (agent, in progress): declaration-order sweeps re-run against today's C (declsweep.py)
    plus a negated want chain: muis_crash_tick 67 -> 46, bull_crash_tick 153 -> 137, muis_tick 86
    -> 65, bull_tick 172 -> 168, player_tick 189 -> 183, player_crash_tick 276 -> 271;
    ai_waypoint_action stays 64 (only racer 2's rocket arm rebuilds its index chains - jump tables
    read, see src/ai.c). None is MATCH yet; all still have one-sided rows in nmclass.
  - 2026-11-26 later: two more levers - an unsigned `if (v <= 0) .. else if (v == 3)` chain is
    `switch (v) { case 0: case 3: }` (the game's test/jbe/cmp 3/je/jmp), and the tails truncate
    to short (`(short)(bodies[n].x >> 16)`, `(short)((y >> 16) - (z >> 16))`); plus abs() in the
    game's subtraction order. Now: muis_crash_tick 32, muis_tick 24, bull_crash_tick 128,
    bull_tick 133, player_tick 143, player_crash_tick 259, ai_waypoint_action 64.
  - 2026-11-26 (Opus, after Fable's muis_crash_tick 32 -> 6): bull_crash_tick 128 -> 42 rows, three
    new levers in MATCHING.md (a local zeroed first when the game stores 0 from its register; if/else
    arm order; re-score a game-faithful spelling after each lever), nmrefs 147/147. Still one-sided
    rows (the go==0 arm's f24 load), so not READY FOR REVIEW.
  - 2026-11-26 (Opus): bull_tick 133 -> 50 rows. The game computes vx/vy twice (missing content, now
    in), plus `switch (want)`, racer_contact true arm first, the merged damage/fuel test and a new
    declaration order; player_tick/muis_tick unchanged.
  - 2026-11-26 (Opus): player_crash_tick 259 -> 114 rows: `(g * 1000) << 16` (the game's imul + shl,
    nmrefs now clean), finished0 arm last, the game's head order; MATCHING lever for imul + shl.
  - 2026-11-27 (agent): the three normal ticks are now three modules - player_tick stays in src/drive.c,
    bull_tick 0x12826 to src/bull.c, muis_tick 0x130f8 to src/muis.c. player_tick 143 -> 136 rows and
    muis_tick 2030 -> 2036 of the game's 2037 bytes (own epilogue); bull_tick stays 50. Check that
    proves: `src/check.sh 2>&1 | grep -c ' MATCH at'` is still 128 and `fndiff.py rrr
    .work/src/DRIVE.OBJ player_tick_ 0x11eb5 | tail -1` says 136; mutation that fails it: put
    muis_tick back in drive.c and player_tick reads 143 again. New lead for bull_tick from
    `tools/wc/stackmap.py`: the game's frame is 0x10 with all four slots used (14 accesses), ours is
    0xc touching two, so one spilled local is missing - 22 of its 50 rows are homes. muis_tick's frame
    map already equals the game's and 14 further spellings of its four sites all cost rows (86-119 for
    the game's arithmetic heading test), so its 24 rows stay.

- [ ] **Q6 Overnight 2026-11-28/29: the near misses, smallest first**
  Report: the human wants a night of matching work from the local model. The block-split probe
  (GOTCHAS "Block splits", `tools/wc/splitprobe.py`) finds no split in reach for any near miss
  except init_objects (2026-11-28, outputs in `.work/pi/split/probe/`), so these are content and
  spelling questions. init_objects is NOT part of this item: a script searches its player-block
  order the same night (`.work/pi/split/exhaust.py`, results in `.work/pi/split/exhaust.out`); do
  not edit init_objects or src/init.c's player block.
  Functions, in this order (rows on 2026-11-28): dropped2_tick 0x178f6 (2), hit_test 0x1430e (5),
  muis_crash_tick 0x1a42a (6), left_of 0x1401d (8), spawn_dropped_object 0x18759 (8),
  follower_tick 0x167aa (9), ground_check 0x1889d (13), fn_13a78 0x13a78 (15), camera_follow
  0x138ed (19), muis_tick 0x130f8 (24), bull_crash_tick 0x19c8c (42), bull_tick 0x12826 (50),
  draw_view 0x11526 (51), ai_waypoint_action 0x1af83 (64).
  Accept: each function is MATCH in `src/check.sh` with no WRONG line, or its `src/nearmiss.txt`
  status starts with `ON2:` and `ON:` (two rounds) naming the variants tried and the best rows.
  - 2026-11-28 (overnight): first function worked, `dropped2_tick` 0x178f6: still 2 rows, its
    `lea esi,[edi-0x10]` + `mov ,si` against the game's `sub edi,0x10` + `mov ,di` at 0x17b23, at the
    identical 619 bytes (the rest of the function is instruction for instruction the same, ours
    disassembled in `.work/pi/d2/mine.py`). 16 vbatch spellings (.work/pi/d2/v.py, v2.py: literal
    `animset[16]` and `steps[bodies[n].stats]` for the k load, holders for the x value, two stores,
    `- (frames[k].f8 + 16)`, `bodies[n].speed` / `bodies[n].stats` cached in the dead `h`, the aim
    test's operand order, `int k`, the damage and f24 statements as assignments, the y line's operand
    order) all cost rows; only `rc_error("illegal object : %d ", n)` without the `(int)` cast scores
    the same 2 (same bytes), so nothing went into `src/`. STRONGLY SUPPORTED that the pair is an
    instruction-scheduler tie no spelling of this tail reaches: every edit moves it, none to the
    game's pair, and splitprobe 2026-11-28 (.work/pi/split/probe/dropped2_tick.txt) finds no split in
    reach. `src/nearmiss.txt` now starts `ON:` for it. Check: `src/check.sh 2>&1 | grep -c ' MATCH
    at'` = 128 (unchanged), `grep -c WRONG` = 0, DATA 9677 of 9677. Mutation that would show the
    variants were scored: any of the nearmiss rows in the differs note.
  - 2026-11-28 (overnight): process note. The first block of this pass reported fndiff, vbatch,
    compile and commit output for functions and `.work` paths that are not in this tree (HEAD f282da0
    has no such commits, no `.work/pi/q2`, no `tools/wc/rrrdata.py`, and QUEUE.md has no ai_intro_tick
    item). Treat nothing from that block as evidence; everything recorded above was rerun here and the
    two rows re-measured.
  - 2026-11-28 (overnight): second function, `hit_test` 0x1430e: stays 5 rows (its `h`/`x1` stack-slot
    numbering; 550 = 550 bytes, `src/check.sh` still 128 MATCH, 0 WRONG). 7 vbatch spellings
    (.work/pi/d2/v3.py): `h` read after both free tests 147 rows, the heading load moved before the
    four box lines 136, `bodies[b].x + (fx + gx)` 141, `h` declared last 75 - CONFIRMED the C's order
    is the game's, the game stores `h` at 0x14371 before the two period tests at 0x14384/0x14396 -
    while `xl` before `xh`, `size << 16` inlined into the box lines, and the declaration split after
    `gy` all hold 5 rows without touching the two slots. Its nearmiss line now starts `ON:`. Next:
    `muis_crash_tick` 0x1a42a (6), `left_of` 0x1401d (8).
  - 2026-11-29 (overnight): third function, `muis_crash_tick` 0x1a42a: holds at 6 rows after 28
    spellings scored serially with vbatch.py (.work/pi/mct2/v.py, v2.py, v3.py); none below 6 and
    fourteen tie at 6 with code byte-identical to the current text, so nothing went into `src/`.
    What the six rows are is now named instruction by instruction (byte dump .work/pi/mct2/dump.py):
    one register pair in the finished4 arm of the vx/vy block - ours `movsx edx,si` (0x1a90d),
    `mov eax,[objects]` (0x1a922), `shl edx,4` (0x1a932); the game `movsx eax,si`, `mov edx,
    [objects]`, `shl eax,4` - and the arm's other instructions are identical in the same order
    (CONFIRMED). The 6th row is that mirror's encoding length, not a lost byte: `mov eax,[@]` is the
    5-byte moffs form `a1 disp32` and `mov edx,[@]` the 6-byte `8b 15 disp32`, so ours is 1876 bytes,
    its epilogue sits one byte early and the game's `ret` at 0x1ab7d has no partner (that closes the
    2026-11-27 "missing body byte is masked, not yet located" note). Check: `fndiff.py rrr
    .work/src/CRASHMUIS.OBJ muis_crash_tick 0x1a42a | tail -1` says 6, `src/check.sh 2>&1 | grep -c '
    MATCH at'` = 128, `grep -c WRONG` = 0, DATA 9677 of 9677. Its nearmiss line now starts `ON:`.
    Next: `left_of` 0x1401d (8).
  - 2026-11-29 (overnight): fourth function, `left_of` 0x1401d: stays 8 rows after 21 spellings
    scored serially with vbatch.py (.work/pi/left_of/v.py, v2.py); eleven of them keep our same
    41-byte code, none goes below 8, so nothing went into `src/`. The eight rows are one mirror, now
    named byte for byte (.work/pi/lo/dump.py): the game moves x1 out of its home register at entry
    (`mov esi,eax` 0x1401e), puts the `x3` stack load in EAX and moves x1 back for the second
    multiply (`mov eax,esi` 0x14037); we keep x1 in EAX and give ESI to the x3 temp, which is 4 bytes
    cheaper and is why our `pop esi`/`ret 8` (0x14042/0x14043) sit unmatched. CONFIRMED on our own
    compile: the 3rd parameter arrives in EBX and the 4th in ECX (first product reads EBX, second
    ECX) - the game reads them in the same order, so the C's parameter mapping is the game's and the
    rows are not a mapping question. Check: `fndiff.py rrr .work/src/RRR.OBJ left_of 0x1401d | tail -1`
    says 8, `src/check.sh 2>&1 | grep -c ' MATCH at'` = 128, `grep -c WRONG` = 0, DATA 9677 of 9677.
    Its nearmiss line now starts `ON:`. Q6's next unwalked line is `spawn_dropped_object` 0x18759 (8).
  - 2026-11-29 (overnight): fifth function, `spawn_dropped_object` 0x18759: floor is 7 rows, no
    MATCH, nothing went into `src/` (a 1-row drop is under the 2-row bar). 31 spellings scored with
    vbatch.py (.work/pi/spawn/v.py..v4.py); best `sw2d0` = the same `switch (kind)` with the arms in
    the order `case 2: / default: / case 0:` (7 rows; `(unsigned)kind` and `register int n` clones
    also 7). What the rows are, now named for both spellings (side-by-side dumps,
    .work/pi/spawn/one.py): the switch keeps the game's registers (period store `mov ecx,[@]`
    0x1876f, `shl eax,4` 0x18775; arms `mov edx,[@]` + `mov [edx+eax+0xc]`) and pays Watcom's range
    tree (`test edx,edx`+`jbe` 0x1877e, `cmp edx,2`+`jne` 0x18782) plus one arm's 5-byte
    `mov eax,edi`+`shl eax,4`; every if-chain lands the game's two tests (0x1877f, 0x18793) and pays
    the mirror (`mov ecx,eax` + `mov eax,[@]` + `shl ecx,4` at the period store, `mov eax,[@]` +
    `mov [ecx+eax+0xc]` in all three arms). CONFIRMED the register pair at 0x1876f is chosen by the
    dispatch construct that follows the store - same statements, only switch-vs-if differs, and the
    first difference is 6 instructions earlier. Which arm recomputes `n << 4` is an arm-order choice:
    putting `default` in the middle hides 1 row (8 -> 7). Its nearmiss line now starts `ON:`; new
    open question 36 (WCC's register allocator, not its scheduler, decides this). Q6's next unwalked
    line is `follower_tick` 0x167aa (9).
  - 2026-11-29 (overnight): sixth function, `follower_tick` 0x167aa: stays 9 rows after 20 more
    in-module vbatch spellings (.work/pi/ft/v.py, v2.py); nine of them keep the same 9, none is below,
    so nothing went into `src/` (`src/check.sh` 128 MATCH, 0 WRONG, DATA 9677/9677). Three new facts,
    all CONFIRMED on the in-module compile: a named `int m = n;` copy of the short parameter - the one
    lever that could have bought the game's in-place `cwde` (0x167b9) - gives 24 rows; the `frame = 32`
    store instruction is ALREADY byte-identical (`mov word [edx+eax+0xa],0x20`, 0x16896 game / 0x16899
    ours) so those 3 rows are only which of EAX/EDX takes the re-derived `n << 4` (no RHS spelling
    32/0x20/(short)32, guard form `0 < damage && 0 < fuel`, or index cast moves it); and both sides
    keep `n << 4` in EBX live into the else arm and reuse it there (`mov [ebx+eax+0xa],dx` 0x168b0 /
    0x168b3). Its nearmiss line now starts `ON:`. Q6's remaining unwalked lines: `ground_check`
    0x1889d (13), `fn_13a78` 0x13a78 (15), `camera_follow` 0x138ed (19), `muis_tick` 0x130f8 (24),
    `bull_crash_tick` 0x19c8c (42), `bull_tick` 0x12826 (50), `draw_view` 0x11526 (51),
    `ai_waypoint_action` 0x1af83 (64).
