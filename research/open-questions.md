# Open questions

## Launch / process flow

1. **RESOLVED (session 5)**: by exit codes (discoveries "The round").
   The flag at `0x1212c` is Start! itself: the menu never exits 43.
   Open: why DOSBox-X did not show the podium (discoveries "podium").
2. **RESOLVED.** INTRO.EXE plays rrrintro.smk with rrrintro.wav and
   ends; MOTOR runs it unless `/NI` (`/NOINTRO` does not match).
   Discoveries "The round".

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
    cursor's blink is measured. Open: whether the pointer shows during the
    name list and a click there takes a name, as the code says (untested:
    DOSBox-X's mouse cannot be moved in the game's VESA mode).
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
14. What do the rest of the cell types do: 10, 15, 16, 21, 22 (0x16), and
    the five check points of `0x18ae5`?
15. **RESOLVED.** Rockets, explosions, exhaust and over-speed objects:
    discoveries "rockets and explosions", "exhaust, smoke and
    after-images".
