# Open questions

## Launch / process flow

1. **RESOLVED (session 5)**: by exit codes (discoveries "The round").
   The flag at `0x1212c` is Start! itself: the menu never exits 43.
   The podium does show in DOSBox-X; the earlier miss did not reproduce
   (discoveries "podium").
2. **RESOLVED.** INTRO.EXE plays rrrintro.smk with rrrintro.wav and
   ends; MOTOR runs it unless `/NI` (`/NOINTRO` does not match).
   Discoveries "The round".

23. **RESOLVED (session 7): the harness.** QMP `send-key` released the key
    too soon for INTRO's own INT 9 table; with real holds an 80 ms tap ends
    the film, under MOTOR too (discoveries "The round").

## Race

3. **RESOLVED (session 2).** Map cell bytes: 2-3 waypoint, 4 object or
   surface type, 5 ground level. Discoveries "The race: map cells".
4. **RESOLVED (session 3).** The computer racers steer toward their
   waypoints; discoveries "computer racers". The 0x18759 objects are
   missiles (discoveries "dropped missiles"); stats +0x20 is their count,
   never used up.
5. **RESOLVED.** RC.IMG's layout follows the anim table: RedCat 32
   headings per pose, Brutus and Snitch 16 (discoveries "map objects").
6. **RESOLVED.** One pass, no laps: the first touch of a type-20 cell
   finishes a racer, and the sizes follow course length (discoveries "Race
   tracks").
7. **RESOLVED (session 4).** Not in the race: REKEN awards fuel, repair,
   boosts and rockets per correct sum and passes them in RRRIO.INF;
   discoveries "The arithmetic part".

## Arithmetic (REKEN.EXE)

16. Module 1's generators are **RESOLVED (session 4)** (discoveries, checked
    under Unicorn); module 2's `0x12068`.. family too (session 5).
17. **RESOLVED.** The exercises' Help examples (`0x11a6a`, `0x12ca2`,
    `0x147a9`, `0x171ea`) are decoded and ported (discoveries). Modules 2–4
    themselves are **RESOLVED (session 5)**.
20. **RESOLVED.** REKEN aborts on module 2's divide by zero: DOS/4GW
    error 2001, exit code 209, and MOTOR ends the game (discoveries
    "module 2").
18. **RESOLVED.** `NNNNNNNN.SMD`: file-formats.md "Pupil files"; the
    number is one past the last file's when no name matches. Not checked
    at run time: REKEN's own second append (the full-stride rewrite is
    read from the code).
19. **RESOLVED.** +0x30 / [0x46c87] is the sound mode: bit 0 effects (and
    sound at all), bit 1 music (discoveries "Sound").
22. **RESOLVED: a fault in the check.** The 4 differing module 2 cases were
    seeds whose run in the original faults (as #20); after a Unicorn fault
    the check went on in the half-run machine. With a fresh emulator per
    fault: 0 of 25000 differ, 6 faults.

## Data

8. **RESOLVED.** XVGALIB.IMG holds the button library's OK / JA / NEE
   message-box buttons, loaded but (LIKELY) never drawn; discoveries
   "archives".
9. **RESOLVED (session 4).** `ASCIInn.IMG` and `RRRFONT.IMG` fonts:
   file-formats.md "ASCII fonts".
10. **RESOLVED (session 3).** `TRANS.DAT` is a 255×255 50% blend table; discoveries "drawing".
21. **RESOLVED.** The name is asked by RRRMENU under MOTOR's `/1` (first
    menu start), discoveries "The menu: the pupil's name"; ported. The
    cursor's blink is measured; the mouse does nothing in the dialog (the
    pointer has no sprite until after it).
11. **RESOLVED.** `RRRHIGH.DAT`: file-formats.md, discoveries "High
    scores".

## Tooling

12. **RESOLVED (session 2).** REA decompiles `.work/le/*.elf` with Ghidra
    12.1.4, and `tools/ghidra/decomp.sh` dumps a whole program to
    `.work/ghidra/<prog>.c`. Ghidra has no Watcom register convention, so
    register arguments show as `in_EAX`/`extraout_ECX`; check details in
    `disasm.py`.
13. **RESOLVED: none.** The race runs as fast as the CPU allows and
    nothing in RRR caps it (discoveries "The race: speed"). What rate the
    designers tuned for is not recorded in the binary.
14. **RESOLVED.** 10 is a time bomb that goes off by itself, 15/16 cacti;
    17, 21 and 22 occur in no map (22's case is dead code); the five points
    are the racer's cell and its four neighbours (discoveries "map cells").
15. **RESOLVED.** Rockets, explosions, exhaust and over-speed objects:
    discoveries "rockets and explosions", "exhaust, smoke and
    after-images".

24. **PARTLY RESOLVED.** RRR's options are `-3r -s -ox` (discoveries,
    "Executables"). The FP option is settled: the compiler's default (no `-fp`
    option) rebuilds dir_tbl_init 0x13cba (fild, atan2 as a call to IF@DATAN2, __CHP before
    fistp) byte for byte, and cos_sin_init 0x13c34 (inline fsin/fcos) to the same code apart from
    one stack slot (all 120 declaration orders of i, a, y, x, t and 36 statement/declaration
    combinations stop at 2 rows - our `x` lands in [esp+0x10], the game's in [esp+8]). RESOLVED
    2026-11-27 (that slot as well, and it was C): the game's cos value is not one of the three 4-byte
    band locals at all; `int v[3]` with the cos value in `v[2]` and the angle expression written twice
    (no `double t`) rebuilds it byte for byte. See discoveries "Toolchain: how WCC lays out a
    function's stack frame". RESOLVED 2026-11-27 (the version half): it is **10.6** - RRR links
    10.6's CLIB3R modules (118 byte-identical, including `sprintf` 0x1b916, which is the one byte off
    in 10.5's), 10.5's in only 108; see discoveries "Executables". The version does *not* explain the
    near misses: 10.5 and 10.6 agree byte for byte on every module of `src/` (tools/wc/segcmp.py), so
    the one-register-choice questions (25, 26, 28, 29, 31) are still C questions. Still open: what
    else REKEN's debug block holds (locals, types, line numbers).

25. **Open: the droppedN_tick damage-block register choice.** In
    dropped2_tick 0x178f6 (and dropped0_tick 0x17b61, dropped4_tick 0x17c74)
    the game's first damage block loads `bodies[2].stats` with
    `mov eax, [0x6cc60]` and builds the stats index in EDX; every C spelling
    found so far loads EDX first, pays a `mov edx,eax`, and merges its tail
    with the second (racer 4) damage block 8 bytes later. That leaves our
    shared epilogue at +0x26d instead of +0x263, which is where
    mine_tick 0x16b53, sign_tick 0x16c5d, bomb_tick 0x16d30, fn_16e46
    0x16e46 and cactus_tick 0x17236 - all otherwise byte-identical - jump to.
    Prize: 8 functions. 30+ spellings and the option sets -ox, -ox -oe,
    -ox -oa, -ox -oi and -oe alone change nothing (session 13; the full list
    is in the note in src/shadeobj2.c). The function is byte-identical to the
    game up to 0x179bd, so the choice is made from the damage statement itself.
    2026-11-27, the two new source-shape levers from the crash ticks (MATCHING.md: unsigned fields,
    and `if (f <= 0) A else if (f == 3) B` block order) do not apply here: dropped2_tick has no such
    multi-test chain on one field, and `f24 <= 0` at its head already matches byte for byte.

26. **RESOLVED: `#pragma aux f modify exact [eax]` is the lever, and plain
    `modify` is not.** The 2026-11-27 conclusion ("WCC rematerialises register
    arguments whatever it knows about the callee") came from probing
    `modify [eax]` and `parm [...] modify [eax]`: plain `modify` only *adds* to
    the default clobber set, so those probes could not change anything. With
    `modify exact [eax]` - correct, because fn_1d2a6 0x1d2a6 is literally
    `mov [0x5fe99], eax; ret` - WCC keeps ECX/EBX/EDX live from the fn_1d2a6
    call into easy_init 0x20b80's arguments: 10 -> 2 rows. The last 2 rows were
    the order of the four argument loads (game EAX,ECX,EBX,EDX at 0x10624, WCC
    default ECX,EBX,EDX,EAX): WCC materialises the parm list back to front, so
    `parm [edx][ebx][ecx][eax]` plus `fn_1d2a6(34, wav_handles, wav_names,
    11025)` lands the order with the same registers and fixups, and sound_init
    is byte-identical (MATCH count 125). CONFIRMED: the pragma behaviour and
    the reverse emission order (two parm orders measured). SPECULATIVE: that
    RRR's own header wrote such a parm list.
    2026-11-27, the clobber-assumption idea is dead: `#pragma aux` `modify []`, `modify [eax]` on
    fn_1d2a6, on easy_init, on both, `parm [eax][edx][ebx][ecx] value [eax]`, and shared arguments
    as locals plus `modify [eax]` all stay at 10 rows / 809 bytes (.work/probe/si). The pragmas are
    not inert (the same form on free_object/explode_objects moves dropped2_tick between 16, 32, 56
    and 87 rows), so WCC386 materialises register-passed call arguments at each call site whatever
    it knows about the callee - which also removes that candidate explanation from open questions
    25, 28, 29 and 31.
27. **Open: where the original modules start and end inside
    0x10010-0x1B915.** New lever (2026-11-23): Watcom shares an epilogue
    between two functions only inside one object, and the game shares one
    only 6 times in the whole C range (discoveries.md "The game's own C is
    many objects"). So a function whose epilogue the game gives it own must
    live in a different file than the one ours merged into: that moved
    racer_contact and racer_bounce to MATCH (src/racer.c, 113 -> 115). The
    six shares also say which functions are certainly together: order_add
    -> 0x11add, trans_generate -> 0x11e23, camera_follow -> 0x13f8b,
    racer_bounce -> racer_contact 0x142d6, find_waypoints + find_bonus_objects
    -> 0x15565, dropped0/2/4_tick + mine/sign/bomb/fn_16e46 -> 0x17b5b.
    Where each module starts is still unknown; string-block boundaries and
    the error strings' file names are the other two tests MATCHING.md 4 gives.
    Caveat added 2026-11-27: the test works one way only. Sharing proves
    together; *not* sharing proves nothing unless the two epilogues are
    identical (WCC shares only identical ones), so check the `add esp` count
    and the return-value instruction first. The three normal ticks -
    player_tick 0x11eb5, bull_tick 0x12826, muis_tick 0x130f8 - end at
    0x1281a `xor eax,eax; add esp,0x10`, 0x130ec `xor eax,ebx; add esp,0x10`
    and 0x138e1 `xor eax,eax; add esp,0xc`: three different epilogues, so the
    game may have had them in one object or three. src/ now keeps them in
    drive.c / bull.c / muis.c because that is where our compiler is closest to
    the game (player_tick 143 -> 136 rows, muis_tick 2030 -> 2036 of its 2037
    bytes), not because the boundary is proven.
    2026-11-26, the two tests agree at shadeobj.c: a padding byte 0x51783 between
    'illegal shade object : %d following :%d' (used only by follower_tick 0x167aa)
    and 'illegal object : %d ' (used only from obj_callback_1696c 0x1696c to
    dropped4_tick 0x17c74) puts a module boundary at 0x16945, which is exactly
    where the epilogue evidence puts one - follower_tick keeps its own pop/ret
    while fn_16e46 jumps into dropped2_tick's at 0x17b5b. Splitting follower_tick
    into src/shadeobj.c and the callbacks into src/shadeobj2.c won 4 rows
    (13 -> 9) and changed nothing else.
    2026-11-24, the same evidence names init.c: find_waypoints 0x1517a and
    find_bonus_objects 0x15597 share the epilogue at 0x15565 and both pass the
    literal 'init.c'; in src/init.c find_bonus_objects went to MATCH with no C
    change at all (its tail jmp now resolves to find_waypoints' pop/ret).
    2026-11-25, the negative case works too: the three crash ticks 0x19497 /
    0x19c8c / 0x1a42a sit back to back and share no epilogue in the game (each
    has its own xor eax,eax + add esp,0xc + pops + ret), so they cannot be one
    object; splitting src/tick.c into tick.c (player) + crashbull.c + crashmuis.c
    took player_crash_tick 282 -> 276 rows. Three consecutive functions can be
    three objects, so "back to back" does not say "one module" - only a shared
    epilogue does. What the three modules' names are stays unknown (no string
    block in any of the three).
    2026-11-28, a negative for the string-block test, at main.c: putting
    init_game 0x10817, finish_check 0x10eff, start_lights 0x1103e and main
    0x111e8 in their own file (rrr_funcs.tsv carries 'main.c' strings at
    0x10817, 0x10eff and 0x111e8, and the first new2io.c strings at draw_view
    0x11526, so main is the last function of that candidate module) changes
    nothing: check.sh reads the same 126 MATCH and the same first-diff offset
    on every near miss, and start_lights stays at 3 rows. So that candidate
    boundary is inert for matching (the four functions share no epilogue in
    the game, and ours share none either once main is byte-identical), and the
    string-block test alone does not earn a file split - keep a split only when
    a row count or a jmp target changes.
28. **Open: follower_tick 0x167aa's last 9 rows are two register choices.**
    (a) the game turns the short parameter into an int where it sits -
    `cwde`, then `mov esi,eax` for the objects index and `dec eax` for the
    check_object_nr argument, all before the call - we sign-extend into the
    index register with `movsx` and derive the argument with `lea`, and our
    `shl esi,4` lands after the call; (b) in the `frame = 32` store the game
    keeps objects in EAX and puts n<<4 in EDX, we mirror. ~60 variants
    (.work/probe/ft2, each compiled alone) do not move either: parameter and
    local types, every declaration order, `0x20`/`(short)32`/braces, `--n`,
    `(int)`/`(short)`/`L` casts, `t = n - 1` split in two, an extra local, and
    the mode/period/anim/bodies statements reordered. The rest of the function
    is byte-identical, so - as in open questions 25 and 26 - a new mechanism is
    worth more than more spellings.
29. **RESOLVED 2026-11-28: which temporary WCC386 keeps live across a branch - the source says it.**
    ground_check 0x1889d went to MATCH (0 rows, 584 bytes) by writing
    `cy = (bodies[n].y >> 16) / 40;` a SECOND time before the last lookup: the re-assignment kills the
    CSE of `cy * map_w`, so the quotient is what stays live in EAX and each lookup multiplies it into
    its own register - the game's shape below. Written once, 10.5 keeps the product and reuses it; that
    is the 13/15-row state, and no holder, cast, operand order or declaration order reaches the game
    (discoveries "Toolchain: what kills a common subexpression", .work/pi/gc/v.py, v2.py). What
    follows is the reading that stood until then.
    ground_check 0x1889d's last 15 rows are that choice. In the game the
    second cell lookup preserves the division result: `mov edx,[map_w]`,
    `imul edx,eax` (EAX stays (bodies[n].y >> 16) / 40), `add edx,ebp`,
    `imul edi,edx,6`, and its third lookup therefore recomputes
    `imul eax,[map_w]` at 0x18aab - 9 bytes the game has and we do not,
    which is also why our epilogue sits 9 bytes early. 10.5 propagates the
    product instead (`imul eax,[map_w]`, `add ebp,eax`, `imul edi,ebp,6`)
    and reuses it in the third lookup. The same expression written in a
    probe with one register less busy does come out the game's way
    (.work/probe/gc3/Q.C q0/q1: `mov esi,[map_w]; imul esi,eax`), so
    pressure, not the source text, picks it: index operand orders, all 720
    declaration orders of the six locals, ox/oy local or inline, the
    product as its own local, an `int` map_w and the if/ternary/key-test
    spellings all stay at 15. Same family as open questions 25, 26 and 28
    (dropped2_tick's load register, sound_init's reloaded arguments,
    follower_tick's sign extension): one register choice, the rest of the
    function byte-identical. 2026-11-27: raising the pressure is not the lever - eight hoisted locals
    in place (row, column, `x0 = bodies[n].x >> 16`, `z = bodies[n].z`, every combination,
    .work/gc/v1-v8 with inplace.py) go to 46-158 rows, all worse; nmclass.py counts ground_check's 15 as
    2 register homes + 3 shape rows + 8 only-ours instructions against 2 only-game ones.
30. **RESOLVED 2026-11-27: init_objects 0x148db is in the init.o object, in front of
    find_waypoints.** Writing it in src/init.c put find_waypoints' three-byte jump-table pad at
    0x1556c exactly like the game and it went to MATCH (src/check.sh, 116 -> 117); with the same
    file but init_objects 14 bytes shorter (a different statement order in its player-stats block)
    find_waypoints fails again at +1010, which is the mutation that makes the check fail. Two more
    supports: init_objects' only string 'player_direction : %d wp: %d\n' sits at 0x50e04 and runs
    into 'init.c' at 0x50e22 (29 + 1 bytes, and the four bytes before it are zero), so its string
    is the head of the init.c string block; and start_crash 0x14f3e, switch_anims 0x15084 and
    fn_15166 0x15166 sit between init_objects and find_waypoints, still candidates for the same
    module (their strings are still in rrr.c's pool, which is why find_waypoints' __FILE__ strings
    now land 1-2 bytes off the game's - layout, no match lost).
    Original question, kept for the record:
    find_waypoints is now 0 instruction rows: +0..+1009 (through its ret at
    0x1556b) are byte-identical, and the only difference left is the padding in
    front of find_bonus_objects' ten-entry jump table - three bytes
    (`lea eax,[eax]`, the game at 0x1556c) against our two (`mov eax,eax`,
    +1010). Watcom 4-aligns a jump table inside its own object, so the pad is
    set by the size mod 4 of everything earlier in the object: ours is empty in
    front (find_waypoints is INIT.OBJ's first function). Putting fn_15166
    (0x15166, 20 B = 0 mod 4) in front keeps the two-byte pad, so it is not
    enough; init_objects 0x148db is 1635 B = 3 mod 4 and would land the table
    at +1013 like the game. Which functions are in init.o is unknown: its
    string block runs from 'init.c' 0x50e22 (find_waypoints' first use, 0x1523d)
    at least to '(%s):%d Cannot initialise boost object.' 0x5103a
    (find_bonus_objects, 0x15627); clear_objects 0x14861, init_objects 0x148db,
    start_crash 0x14f3e and switch_anims 0x15084 sit right before it and are
    candidates. Writing init_objects and adding it to src/init.c is the test:
    find_waypoints goes to MATCH if that is right, and a pad byte of the wrong
    candidate shows up immediately in src/check.sh.

31. **Open: init_objects 0x148db's last 2 rows.** Update 2026-11-26: computing the player's fuel
    into the dead `i` before the drops store puts the fuel<<16 product in EBX as in the game
    (7 -> 2 rows, commit 877ee94). Left: the MUIS heading store's slot - a result holder floats
    it but flips BULL's block (8 rows); result and index holders fix both but swap two
    neighbouring instructions (2 rows); ~7,000 variants stop at 2 (src/nearmiss.txt). The
    7-row analysis it started from: two register ties. The player's
    `rrrio.fuel << 16` product: the game keeps it in EBX and only then loads rrrio.boosts
    into EDX, we keep it in EDX (5 rows at +607); and the MUIS heading store, which floats
    past the racer_top_speeds[] load in the game (its result sits in EBX) and is forced
    early in ours. Length is exact (1635 = 1635 bytes) and every address aligns, so the C
    is right; BULL's identical text matches. Tried: * 0x10000, * 65536, < 16, (int) casts,
    an extra store, all four spellings of the crash0 test, and 192 statement permutations
    of the player, BULL and MUIS stats blocks (.work/probe/io/y*.c, inplace.py) - best 7.
    Same family as open questions 25, 26, 28 and 29; here the tie sits at the END of a
    long straight-line block, so a new mechanism for it may be the one that cracks those.

32. **Open: dropped2_tick 0x178f6's last 2 rows are one register tie.** The last x store:
    the game does `sub edi,0x10` and stores DI, ours `lea esi,[edi-0x10]` and stores SI, for
    the same `(bodies[n].x >> 16) - frames[k].f8 - 16`. Both are 3 bytes and EDI is dead after
    it in both builds; 619 = 619 bytes with every address aligned. Tried: parens, operand
    order, `- 16 - frames[k].f8`, `- (frames[k].f8 + 16)`, `- 8 - 8`, `(short)` cast, `h =
    bodies[n].x >> 16` and `h = ... - frames[k].f8` holders, the same for the y line, hex and
    decimal constants, `objects[n].x = ...; objects[n].x -= 16;`, moving the vy statement,
    the f24 decrement form, and the `k` index casts (~70 spellings, .work/probe/d8-d11,
    inplace.py) - all 2 rows or worse. Same family as open questions 25, 26, 28, 29 and 31;
    the lever that cracked the other 14 rows of this function was a shared index holder
    (discoveries, "switch bodies and `lea` operands"), which suggests the tie is a
    value-identity question, not a spelling one.
    2026-11-28 (overnight): 16 more vbatch spellings (.work/pi/d2/v.py, v2.py) - the k load as
    `animset[16]` / `steps[bodies[n].stats]` (38, 29), a holder for the whole x value and the
    three-statement chain (40, 51), `bodies[n].speed` and `bodies[n].stats` cached in the dead `h`
    (19, 54), `int k` (12), the damage statement as an assignment (53), the y line's operand order
    (40) - and dropping the `(int)` cast in `rc_error("illegal object : %d ", n)`, which scores the
    same 2 rows with the same bytes. So the pair reacts to any id change in the function (nothing in
    its tail or its earlier blocks leaves 2) but never lands on the game's `sub` + `mov ,di`:
    STRONGLY SUPPORTED that the value's identity, not this function's spelling, decides it, and no
    block split is in reach (splitprobe 2026-11-28, .work/pi/split/probe/dropped2_tick.txt).

33. **Resolved 2026-11-26: start_lights 0x1103e's last 3 rows were how many pointer locals the lamps
    share.** With one `struct frame *f` for all five lamps the fourth lamp's product lands in EAX
    (`imul eax,eax,0xc; mov edx,[frames]`), the game's in EDX (`imul edx,eax,0xc; mov eax,[frames]`);
    ~2400 one-local spellings (.work/probe/sl2) did not move it. Two locals, lamps 1/2/5 through `f`
    and 3/4 through `g`, MATCH. 26 of the 52 set partitions of the five lamps over locals match
    (.work/probe/sl4), so CONFIRMED that the original used more than one local, SPECULATIVE which
    sharing. The earlier LIKELY mechanism (a register-resident index) is wrong. Lesson for the family
    (25, 26, 28, 29, 31, 32): a register tie can be decided by how many C variables carry the values,
    even when each one is dead between uses.

34. **Open: where WCC puts the crash-return block in the three normal ticks.**
    In bull_tick 0x12826 (and with its own constants in player_tick 0x11eb5 and
    muis_tick 0x130f8) the game tests `objects[n].period == -1` at 0x12887 and
    jumps to 0x128af - which is the `xor eax,eax` in the *tail of the crash-return
    block* (`crash2 = 1` at 0x128a5, then 0x128af/0x128b1) - and its first damage
    test falls *through* into that store (`jge 0x128b6` skips it, and the fuel
    test at 0x128b6 jumps back to it with `jl 0x128a5`). Ours puts both tests
    together and the store out of line at 0x128ae, and the period test jumps
    straight to the epilogue. Four spellings of the test leave the placement
    unchanged, two of them byte-identical to the current text: merged `damage < 0
    || fuel < 0` (50 rows), `!(damage >= 0 && fuel >= 0)` (50), `!(damage >= 0)
    || !(fuel >= 0)` (191), `!(damage > -1 && fuel > -1)` (52), `a < 0 ? 1 : b < 0`
    (119) - .work/scratch/bull_tick, 2026-11-27. A shared tail written with a label
    (`goto out;` for the period test, `out:` right after the `crash2 = 1` store -
    the literal shape the game's jump suggests) gives 57, still places the store
    out of line, and moves our go/m slots to [esp]/[esp+4]. So the fall-through
    choice is not the C's polarity here (unlike the follower_tick 0x167aa lever,
    where negating the pair did move the store) and neither is a labelled shared
    return; whether the placement is WCC's own or follows a source-order
    difference is open. It is worth three functions and 210 near-miss rows, so it
    is the first thing to bring a new idea to.
    2026-11-27 (later), **the jump target is explained by the tail, and only bull_tick has it**
    (CONFIRMED from the game's bytes): muis_tick's period return is 0x13158 `je 0x138e1` and
    player_tick's is 0x11f45 `je 0x1281a`, both straight to their final `xor eax,eax`, so the
    "same head in three ticks" premise above was wrong for the target. bull_tick's final return is
    NOT a constant: 0x130e5 `mov eax,ebx` / 0x130e7 `mov [edx+edi+6],bx` (objects[n].y) / 0x130ec
    `xor eax,ebx` returns y XOR y. With no shareable `xor eax,eax` at the end, the period return lands
    on the crash block's own one at 0x128af. bull_crash_tick 0x19c8c ends the same way (0x1a417
    `mov eax,ecx` / 0x1a419 `mov [edx+ebx+6],cx` / 0x1a41e `xor eax,ecx`). A byte scan of RRR's
    code found no other `mov eax,R ... xor eax,R`. STRONGLY SUPPORTED: any non-constant return moves
    the head. A stand-in `return k;` gives the game's `sub esp,0x10` frame and rewrites the period
    test. Still open: the C behind it. In WCC 10.5 (about 300 one-function probes in
    .work/scratch/bull2): `t ^ t` on one variable compiles to `xor eax,eax` (and `t - t` to `sub
    eax,eax`); a copy through an int local is propagated away (`r = t; return r ^ t;` is also `xor
    eax,eax`). Two distinct temporaries survive only through an int store to memory (`return (gi = t) ^ t;`
    gives the game's exact `mov eax,edx; mov [gi],edx; xor eax,edx`) or through two reaching definitions
    (`if (c) r = t; else r = t;`, which leaves a dead `cmp`). An assignment to the short field always adds
    a `movsx`/`cwde`, whatever the return type (int, short, char, unsigned, long) or operand types. In
    the full function the closest is `return (objects[n].y = y = E) ^ y;` with a new `int y;` declared
    first: 97 rows, worse than the 50 we have, so src/ keeps `return 0;`.
    2026-11-27 (latest), **both the tail and the head layout reproduced, the total not yet.** A value
    merged from two paths that both equal y survives as a second temporary. `y = E; r = y ? y : 0;
    objects[n].y = y; return r ^ y;` compiles to the game's exact `mov eax,ebx; mov [..],bx; xor eax,ebx`
    with no branch left (CONFIRMED, one-function probe, .work/scratch/bull2/gen7). So does `if (y) r = y;
    else r = y;`. In bull_tick, that tail plus the damage and fuel tests written as two separate `if`
    blocks gives the game's crash-block layout (0x128a5 store falling into the shared 0x128af `xor eax,eax;
    jmp`, the period test's `je` into it): no diff rows left there (z_2_y_f.c). Neither change does it
    alone: the head change with `return 0;` is 63 rows, the tail with the merged test leaves ours
    jumping back to the period block's copy. CONFIRMED for the layout. The other rows go up: 114 at
    first, and the declaration-order floor is 100 (reverse-engineering/tools/wc/psweep.py: every
    order of 7 declarations with h as y and the copy in f, want or go, 5,040 each, floor 114; 2,000
    sampled orders of 8 with a new y and the copy in want, m, go or a new r, floor 100). What is left
    is allocation: the game keeps objects[n]'s offset in EDI across the whole tail (0x13061 `shl
    edi,4`, reused by the three stores), where ours recomputes it. The current C's other spellings
    were tuned against `return 0;` and have to be searched again with this tail and head. Whether
    `?:` is the game's construct is SPECULATIVE: any two-path merge of equal values gives these bytes.
    bull_crash_tick 0x19c8c has the same ending (0x1a41e). Its head already matched (its period
    return is inline, 0x19cea), and the same tail takes it from 42 to 119+ rows untuned.
    Inline, without the extra variable, `objects[n].y = y; return (y ? y : 0) ^ y;` scores 71 at the best
    of 3,000 sampled orders of 8 declarations (78 over all 5,040 with h as y). But at that order WCC keeps
    a real branch for the `?:` (`je` / `mov edx,ebx` / `jmp` / `xor edx,edx` before the xor), which the game
    does not have. So 71 is not on the path to the game's bytes, even though its head layout matches.
    The branch-free form is the one with the copy in its own variable before the store (floor 100 so far).
    2026-11-27 (later), **the store goes before the merge: 58 rows.** `objects[n].y = y; if (y) f = y;
    else f = 0; return f ^ y;` (f is the existing `unsigned f`) gives the game's tail: `sub ebx,eax;
    mov eax,ebx; mov [edx+edi+6],bx; xor eax,ebx`, with EDI holding objects[n]'s offset from 0x13061
    to the end, as in the game (CONFIRMED, in place; only the `mov edx,[objects]` is scheduled one
    instruction earlier than the game's). With the merge before the store, WCC recomputes the offset
    after the merge. With this tail, the best of 3,000 sampled declaration orders is 63. Mirroring the
    second `abs()` of both distance sums (`abs((waypoint2[i].y >> 16) - (bodies[2].y >> 16))` and
    `abs((bodies[0].y >> 16) - (bodies[n].y >> 16))`) gives 58, and 58 is the floor over all 40,320
    orders of the 8 declarations (CONFIRMED for this text; .work/scratch/bull2/tpl_M.c, sw_M_all.txt).
    These changes leave it at 58 or worse: any of the 24 orders of the four head statements, six
    spelling toggles (operand order of the waypoint-count and vz compares, the negated racer_contact
    test, `f24 = 3`, `d * 16`, `f == 0`) and six declaration types. What is left is allocation:
    - the game's frame is 0x10 with a spilled temp at `[esp]` in the head (0x1285b, the sprite
      index), and `d` at `[esp+0xc]` where ours has `d` at `[esp]` in a 0xc frame;
    - the n extension and `want` are in each other's registers (game EDI/EBP, ours EBP/EDI);
    - a few subtractions and the anim/k lookup in the tail pick other registers.
    The spill suggests that one more value is live across the game's head than across ours, and that
    is SPECULATIVE. A probe that makes `h` possibly-unset does give a 0x10 frame with head spills, but
    not the game's.
    bull_crash_tick: with the store first, `if (y) f = y; else f = y;` gives the game's tail except for
    one extra `mov edx,ecx` before the xor, 120 rows untuned (.work/scratch/bcrash).
    src/ keeps the 50-row `return 0;` text. On that text, mirroring only the waypoint abs() gives 48,
    but that text is tuned to the wrong tail, so it was not taken.

35. **Open: why WCC 10.x keeps init_objects' MUIS heading store early (the function's last 2 rows).**
    2026-11-27, reverse-engineering/tools/wc/simsched.py and schedfit.py, probes in
    .work/scratch/sched.
    - CONFIRMED: `-oeilmr -s` compiles init_objects to exactly `-ox`'s bytes, and `-oeilm -s` (no
      `r`, no scheduling) gives the order before the scheduler: MUIS copies the result
      (`mov ebx,eax`), loads the index, then stores heading, f24, damage, top speed, vx, vy, vz.
    - CONFIRMED: Watcom 10.6 (the game's compiler) gives the same 2 rows, and -4r/-5r/-3s/-4s/-5s
      are 109+ rows, so it is neither the version nor the CPU target.
    - CONFIRMED, small probes: the store's slot depends on the code AFTER the block. With the
      follower statements removed, MUIS's store sinks like the game's. With all but the last two
      removed from the start it still sinks; with three or more it does not. Padding with
      unrelated stores changes nothing. A label, braces, `if (1) {}` and `;` are no block
      boundary.
    - The scheduler (Open Watcom inssched.c) builds each block bottom-up and breaks ties by
      stall cost, then height, then 'stallable', then source order. A model of it reproduces
      450 of the 530 blocks in src/. On init_objects' 118-instruction block (BULL, MUIS and the
      followers) it matches the real compile everywhere except MUIS's heading store - and there
      it gives the game's order exactly (f24, index load, damage, table load, heading, vx, top).
      So the published logic turns our unscheduled code into the game's bytes, and real 10.5/10.6
      do not. CONFIRMED for the model's output; whether the model is right is LIKELY.
    - The deciding tie is the 'stallable' rule: heading (`mov [eax+4],ebx`, 5) against f24/damage
      stores and the table load (3), equal in cost and height. Without that rule MUIS comes out as
      ours, but then BULL's heading store stops sinking below its f24 store, which the game and
      our compile both do, and small blocks such as high_load 0x18f keep source order. So 10.x
      breaks this tie by something the model does not have. SPECULATIVE what: a fit over
      ancestor height, dependant height, both summed, stallable with and without registers,
      successor and predecessor counts, flags, adjacency to the instruction below, and source
      order found nothing better than the published order (80 of 530 blocks wrong).
    - 2026-11-27, later (pi agent, .work/pi/base/, notes in src/nearmiss.txt): the "different
      rule" reading above is superseded. Spelling B (`ri = <call>; si = bodies[4].stats;
      stats[si].heading = ri;`) compiles byte-identical to src/ at -oeilm -s but schedules
      differently, so the scheduler reads state the listing does not show (CONFIRMED).
      Under the published rules, marking MUIS's heading store (or its f24 store) as an indexed
      operand with unknown base (aliases every memory operand, redefby.c ZapsIndexed)
      reproduces our real -ox block 149-266 exactly; it is the only one of 708 single
      markings that does (CONFIRMED for the model). So LIKELY our compile loses that store's
      base symbol where the game keeps it, and the scheduler is the same. B keeps the base
      (store in the game's slot) but swaps the result copy and the index load at 0x14d7a; the
      model reproduces that only with an extra copy<->load edge that no redefby.c rule makes
      (cause SPECULATIVE). Statement sequence is out (Open Watcom sets it only for x87 code).
    - 2026-11-27e (pi agent, .work/pi/fold: simsched3.py = simsched2.py plus forced-stallable and
      index-name markings, cmp3.py, search_stall.py, gen_muis.py, gen_decl.py, gen_order.py).
      Step 1, where an N_INDEXED can lose its base in OW bld/cg (all paths, file:line): only
      ScaleIndex (namelist.c:577-613,606-607) ever writes i.base, so base == NULL can only enter at
      addrfold.c:204 (CL_POINTER - the address sits in a register/temp with no symbol: a real C
      pointer), :211 (CL_TEMP_OFFSET), :220/:224 (CL_CONS and default - an integer value used as an
      address), x86split.c:358 (SplitIndex, whose flags get X_FAKE_BASE at :353) and 386tls.c:224
      (thread-local storage, explicit NULL). Every other call site passes op->i.base through and can
      only propagate a base already lost: cse.c:1383, loopopts.c:1580/2314, unroll.c:351/357,
      fixindex.c:109 (FixIndexAdjust's index temp), inssched.c:812/822 (the scheduler's own
      INDEX_ADJUST rewrite), regalloc.c:131, scins.c:184, x86index.c:259/413, 386sib.c:223,
      386opseg.c:71. `stats[bodies[4].stats]` is a CL_GLOBAL_INDEX, which addrfold.c:208 builds with
      the stats symbol and X_FAKE_BASE cleared, so LIKELY neither our nor the game's compile loses
      that base, and the w214u marking is one equivalent change rather than the mechanism
      (SPECULATIVE). cse.c:1377-1386 is the one path that changes an indexed operand's *index* name
      (it points it at the CSE'd address temp); marking the index as a different name with the base
      kept gives 3 rows, not 0, so that path is not it either.
      Step 2, BULL (-oeilm 166-185) vs MUIS (203-222): the two groups have the same shape and the
      same single *48 product shared by the heading/f24/damage/top_speed stores with nothing between
      the address and them; only the roles swap (BULL copies the result to EDX at 171 then loads
      bodies[2].stats into EBX at 172, MUIS copies to EBX at 208 and loads bodies[4].stats into EDX
      at 209) and in src/init.c only the vx store's source slot differs (before top_speed in BULL,
      after it in MUIS). No base-loss path distinguishes them; the store's value register is the only
      visible difference.
      Step 3: the remaining 2 rows are one InsStallable tie. inssched.c:101-130 scores each operand
      by *name class* (N_INDEXED +3, N_REGISTER +2, N_MEMORY +1, N_TEMP +0, plus +3 for an indexed
      result) and inssched.c:908-930 uses it after cost and height; at the pick that places the MUIS
      heading store, it and the f24/damage stores and the table load are all cost 0 and height 74, so
      the store wins by counting 5 (value a named register/memory name - the game) or loses on the id
      tie-break counting 3 (value a bare N_TEMP - ours, 0x14d8c). Forcing instruction 214's stallable
      below 3 reproduces real A's block exactly (rows=0) and is the only one of 826 single forced
      stallables over the whole block that does; the same forced value on BULL's identical store
      (177) costs 4 rows, so our compile counts BULL's call-result value and not MUIS's (LIKELY).
      No forced stallable, base marking or index-name marking reproduces real B - its pair still
      needs the extra copy<->load edge. 10.5 accepts -oa, the CGSW_GEN_RELAX_ALIAS switch of
      redefby.c:85-90/203-208 (base-unknown indexed operands stop aliasing): 1515 bytes and 87 rows,
      so the game was not built with it and no option carries this. C candidates on the A seed: every
      spelling that names the value does move the store to the game's slot and pays elsewhere -
      result holder alone 8 rows, ri + si (seed B) 2 with the 0x14d7a pair swapped in all six
      declaration orders, `stats[(si = bodies[4].stats)].heading = ri;` 2, one `si` holder over the
      whole MUIS store group 13-34, two holders / `register` / `unsigned` 8 - and moving the heading
      statement to each other slot of its block on the A seed gives 5, 11, 23, 23, 25, 27. Best is
      still 2 (.work/pi/fold/V/m_ri_si.c).
    Next: the store's slot is a name-class question (its stored value must count +1 or +2 in
    InsStallable), and the pair at 0x14d7a is the open half: find the C that names the value without
    turning its copy into the holder assignment - or the dependency that holds the index load under
    the copy in seed B (no redefby.c rule and no name-class marking gives it).
    - 2026-11-28, content (not spelling) probes on the A seed, all in the src/init.c module
      (.work/pi/content: defs.py runner, v1-v4.py, opts.sh); every one leaves the 2 rows or is worse,
      so none of these facts about the program changes the tie (CONFIRMED for each):
      * storage class: stats/bodies/racer_top_speeds/map_no/waypoint4/waypoint_index4/objects
        defined in init.c (non-extern, and static for the first four with rrr.h's extern removed) -
        2 rows each;
      * direction_to_waypoint's declaration: unprototyped `int f();`, no declaration, and `#pragma aux`
        `modify [eax]`, `modify exact [eax]`, `modify exact []`, `parm [eax][edx][ebx]`, `value [eax]`,
        `parm caller` - 2 rows; `modify nomemory` (which the function's body would allow: it saves
        ECX/EDX/EBX and writes no global, 0x13d60-0x13e3c) 48, `parm nomemory modify nomemory` 58;
      * generic per-racer code (`n = 4;` then objects/bodies/direction_to_waypoint/stats indexed by n,
        in the M, B, P blocks and combinations, `i` or a new local): 17-115 rows, the constant is
        materialised, so the original was not written that way;
      * options added to -3r -s -ox: -on -oc -ou -ol+ -oe=0/40/100 -op -oz -oil -zc -ze -za -j -ei -ri
        -d1 -fpi87 -fp3 -fpc -zp1/4/8 -mf -ms -zdp -zff -zgf -zu all 2 rows (-oh -ok -oi+ are not 10.5
        options; -oa 87);
      * the module compiled as C++ (wpp386, inside extern "C"): 30 rows;
      * 240 MUIS-only conversions (index as int/unsigned/long/unsigned long/+x/(int)(unsigned), result
        through 8 casts, store as `stats[..].heading`, `*&`, `->`, `*(int *)&`, `*(unsigned *)&`):
        all 2.
    - **2026-11-28, ANSWERED (the 2 rows' cause): a block split.** Read in the real compiler (10.6
      under winedbg, research/discoveries.md "debugged under Wine"): the scheduler, InsStallable and
      the tie-breaks are Open Watcom's, but init.c's long straight-line run is TWO basic blocks.
      BlkTooBig (0xa11ee5) ends the block at the next statement start once 75 instruction ids were
      generated since it began; ours began at id 133 (after the crash0 `if`) and passes 75 during
      MUIS's heading statement (ids 198-211), so the split falls between MUIS's heading store and
      its f24 store, and the store cannot sink. Every earlier observation follows: holders and
      statement orders move the split (ids, not bytes), and "the slot depends on the code after the
      block" was the split. CONFIRMED by experiment: 14, 16 or 20 dead stores before
      `objects[4].anim = 1;` give 0 rows (10 give 4, 12 give 2), and the natural spelling
      `stats[bodies[2].stats].fuel` / `stats[bodies[4].stats].fuel` (etc.) for the six constant-index
      stores of BULL and MUIS gives 0 rows (`.work/pi/split/init_BM.c`): with it the split moves to
      id 210, before MUIS's fuel store. STRONGLY SUPPORTED that this is the original's spelling: the
      game uses the `stats[bodies[n].stats]` form everywhere else, and constant propagation from
      `bodies[2].stats = 1;` folds it to `stats[1]` before the call. NOT A MATCH YET: with that
      spelling src/check.sh prints MATCH but 10 WRONG fixups in the player's stats block (0x14b58-
      0x14bb2: the same-shaped `stats.f = rrrio.g` stores come out with their fields rotated), so the
      player block's statement order (tuned in 2026-11-26 under the old split) must be re-derived.
      The game's store order there is drops, top_speed, f24, fuel, boosts, f10, f14, vx, rockets,
      vy, damage, vz (0x14b2f-0x14bb0). 100 of 240 orders tried (.work/pi/split/pl2.py, pl2.out):
      best 4 WRONG (damage after f14 or vy); damage after rockets loses the reload (1630 bytes).

36. **Open: which register WCC 10.x picks for a shared `[base + index]` address is decided by the
   statement that FOLLOWS it (spawn_dropped_object 0x18759, 7 of its 8 rows).** 2026-11-29,
   vbatch probes in .work/pi/spawn/ (v.py, v2.py, v3.py, v4.py; side-by-side dumps with
   .work/pi/spawn/one.py).
   - CONFIRMED: with `switch (kind) {case 2: / case 0: / default:}` the period store compiles to the
     game's `mov ecx,[objects]` (0x1876f), `shl eax,4` (0x18775), `mov word [ecx+eax],1` (0x18778),
     and all three arm stores keep the game's `mov edx,[objects]` + `mov [edx+eax+0xc]`; what is
     wrong is only Watcom's dispatch tree, `test edx,edx` + `jbe` (0x1877e) and `cmp edx,2` + `jne`
     (0x18782), against the game's `cmp edx,2` + `jne` (0x1877e) and `test edx,edx` + `jne` (0x18793),
     plus one arm's 5-byte `mov eax,edi` + `shl eax,4` recompute (the whole 306 vs 301 byte gap).
   - CONFIRMED: with an `if (kind == 2) ... else if (kind == 0) ... else ...` chain the two tests land
     at the game's addresses, but the same period-store statement compiles to `mov ecx,eax` +
     `mov eax,[objects]` + `shl ecx,4`, and the three arms become `mov eax,[objects]` +
     `mov [ecx+eax+0xc]`: 11 rows. No chain or switch spelling tried (31) gets both halves.
   - SPECULATIVE: WCC's global register assignment treats EAX as the cheaper sacrifice in a straight
     if-chain and keeps the live shifted value in EAX through a switch DAG; that is the same allocator
     tie as left_of 0x1401d, muis_crash_tick 0x1a42a, follower_tick 0x167aa and draw_view 0x11526.
     What would settle it is a model of WCC's register allocator (not its scheduler), which we do not
     have; simsched.py models only the scheduler and cannot see this.
   - Same tie class, 2026-11-28: camera_follow 0x138ed (35 spellings, floor 19) is ONE decision - the
     game homes the `tx` temporary at [esp+8] (frame 0xc, store 0x13966, reload 0x13996) which frees
     ESI for `ty` (its `lea eax,[esi+edx]` 0x13991), and every row including its epilogue pair follows
     from that; and muis_tick 0x130f8 (24 spellings, floor 24) keeps the game's `movsx edx,dx`
     0x138cd only if the tail's difference lands in EDX, which no spelling of that line reaches. In
     both, a holder, a cast or an arm swap moves one site and costs 30-170 rows elsewhere.
