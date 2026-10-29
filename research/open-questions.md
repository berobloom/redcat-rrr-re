# Open questions

## Launch / process flow

1. **RESOLVED (session 5)**: by exit codes (discoveries "The round").
   Open: the flag under which Start! clears race_done (`0x1212c`), so
   when the menu exits 43 straight to REKEN. Why DOSBox-X did not show
   the podium (discoveries "podium").
2. What does INTRO.EXE do besides playing `rrrintro.smk` with
   `rrrIntro.wav`? Is the intro skipped by `/NOINTRO` (INSTALL.SCR's `rcmin`
   batch) in MOTOR.EXE?

## Race

3. **RESOLVED (session 2).** Map cell bytes: 2-3 waypoint, 4 object or
   surface type, 5 ground level. Discoveries "The race: map cells".
4. **RESOLVED (session 3).** The computer racers steer toward their
   waypoints; discoveries "computer racers". The 0x18759 objects are
   missiles (discoveries "dropped missiles"); stats +0x20 is their count,
   never used up.
5. How are the racers' sprites laid out in RC.IMG (1056 entries; rotations
   per racer)?
6. What do the 19 `.MAP` files' different sizes mean for play: is the whole
   map one lap, or a point-to-point course?
7. **RESOLVED (session 4).** Not in the race: REKEN awards fuel, repair,
   boosts and rockets per correct sum and passes them in RRRIO.INF;
   discoveries "The arithmetic part".

## Arithmetic (REKEN.EXE)

16. Module 1's generators are **RESOLVED (session 4)** (discoveries, checked
    under Unicorn); module 2's `0x12068`.. family too (session 5).
17. The exercises' Help examples (`0x11a6a`, `0x12ca2`, `0x147a9`,
    `0x171ea`). Modules 2–4 themselves are **RESOLVED (session 5)**.
20. Does REKEN abort when module 2's `0x123cf` divides by zero (level 5,
    R = 11; discoveries "module 2")?
18. **RESOLVED.** `NNNNNNNN.SMD`: file-formats.md "Pupil files"; the
    number is one past the last file's when no name matches. Not checked
    at run time: REKEN's own second append (the full-stride rewrite is
    read from the code).
19. What +0x30 / [0x46c87] (default 3) means; it gates the prize sounds.

## Data

8. What is XVGALIB.IMG (three 2,050-byte entries; first u16s read 62×33)?
9. **RESOLVED (session 4).** `ASCIInn.IMG` and `RRRFONT.IMG` fonts:
   file-formats.md "ASCII fonts".
10. **RESOLVED (session 3).** `TRANS.DAT` is a 255×255 50% blend table; discoveries "drawing".
21. Where the pupil's name is typed: none of RRRMENU's screens asks for
    it. Rapport, Bediening and the high score screen are **RESOLVED**
    (discoveries).
11. **RESOLVED.** `RRRHIGH.DAT`: file-formats.md, discoveries "High
    scores".

## Tooling

12. **RESOLVED (session 2).** REA decompiles `.work/le/*.elf` with Ghidra
    12.1.4, and `tools/ghidra/decomp.sh` dumps a whole program to
    `.work/ghidra/<prog>.c`. Ghidra has no Watcom register convention, so
    register arguments show as `in_EAX`/`extraout_ECX`; check details in
    `disasm.py`.
13. **RESOLVED (session 2): none.** The race runs as fast as the CPU allows
    (discoveries "The race: speed"). Open: what rate did the designers
    tune for? `delay_ms` (0x26beb) and 0x32cce hold a clock; is there a
    frame cap anywhere above ~105 frames/s?
14. What do the rest of the cell types do: 10, 15, 16, 21, 22 (0x16), and
    the five check points of `0x18ae5`?
15. **RESOLVED.** Rockets, explosions, exhaust and over-speed objects:
    discoveries "rockets and explosions", "exhaust, smoke and
    after-images".
