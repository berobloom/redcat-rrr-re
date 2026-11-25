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
    combinations stop at 2 rows - our `x` lands in [esp+0x10], the game's in [esp+8]; that is Watcom's
    FP local layout, not the C). RESOLVED 2026-11-27 (the version half): it is **10.6** - RRR links
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
29. **Open: which temporary WCC386 keeps live across a branch.**
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

31. **Open: init_objects 0x148db's last 7 rows are two register ties.** The player's
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

33. **Open: start_lights 0x1103e's last 3 rows are one register tie** (two instructions; the third
    row is fndiff's alignment row). The fourth lamp's frame pointer: the game does `movsx
    eax,[eax+0xe]; imul edx,eax,0xc; mov eax,[frames]; add eax,edx`, ours `imul eax,eax,0xc; mov
    edx,[frames]; add eax,edx` - 427 = 427 bytes, and the other four lamps come from the same text.
    2026-11-25: ~2400 spellings (.work/probe/sl2) stop at 3 - 20 forms per lamp in every one- and
    two-lamp combination, dead statements in 11 slots, braces and `if (1)` nesting, `register` and
    unused locals, `L`-suffixed constants, start_lights compiled alone vs in all of src/rrr.c,
    Watcom 10.5 vs 10.6, and 40 `#pragma aux modify exact` clobber sets for delay_ms/draw_frame/
    snd_play. What moves it: `f = frames; f += animset[0x56].steps[3].frame;` flips the product to
    EDX and lands the game's `mov eax,[frames]; add eax,edx`, but the whole steps chain moves to EDX
    with it (5 rows; 24 spellings of that shape stay at 5). The target is therefore the MIRROR of
    what a base-first address addition gives: chain in EAX, product in a second register, base
    reloaded into EAX. LIKELY mechanism: at 0x11144 the index register was still holding a
    register-resident variable, so the product could not fold - the game's only other non-folding
    product, hud_update 0x19276, has exactly that (its index is the live local `v`, discoveries
    "Toolchain: switch bodies and `lea` operands"). The prologue (`push ebx/ecx/edx`, no stack slot)
    rules out a memory local, and every local-index spelling found adds or moves instructions.
    Same family as open questions 25, 26, 28, 29, 31 and 32.
