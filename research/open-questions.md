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
    FP local layout, not the C). Open: 10.5 or 10.6: RRR's `sprintf` (0x1b916) differs from 10.5's CLIB3R
    module in one byte (`89 d3` for `88 d3`), so a 10.5 patch or 10.6 is likely (compare their CLIB3R with
    `tools/wc/libmatch.py`); what else REKEN's debug block holds (locals,
    types, line numbers).

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
    is in the note in src/shadeobj.c). The function is byte-identical to the
    game up to 0x179bd, so the choice is made from the damage statement itself.
