# Open questions

## Launch / process flow

1. How does MOTOR.EXE choose the next program? (The state file is
   RRRIO.INF, shared by RRRMENU, REKEN and RRR; discoveries "The session".)
   Disassemble MOTOR.EXE (16-bit, small) and RRRMENU's writer.
2. What does INTRO.EXE do besides playing `rrrintro.smk` with
   `rrrIntro.wav`? Is the intro skipped by `/NOINTRO` (INSTALL.SCR's `rcmin`
   batch) in MOTOR.EXE?

## Race

3. **RESOLVED (session 2).** Map cell bytes: 2-3 waypoint, 4 object or
   surface type, 5 ground level. Discoveries "The race: map cells".
4. **RESOLVED (session 3).** The computer racers steer toward their
   waypoints; discoveries "computer racers". Open: what the 0x18759
   objects are (mines? oil?) and what stats +0x20 counts.
5. How are the racers' sprites laid out in RC.IMG (1056 entries; rotations
   per racer)?
6. What do the 19 `.MAP` files' different sizes mean for play: is the whole
   map one lap, or a point-to-point course?
7. **RESOLVED (session 4).** Not in the race: REKEN awards fuel, repair,
   boosts and rockets per correct sum and passes them in RRRIO.INF;
   discoveries "The arithmetic part".

## Arithmetic (REKEN.EXE)

16. The exact sum generators (`0x11496`, `0x11619`, `0x1122e`, `0x1179e`
    and the `0x12068`.. family for the other modules): Ghidra drops their
    Watcom register arguments; read them from the disassembly.
17. Modules 2–4 (Wat is waar?, Memory, Grote getallenspel): screens, rules,
    prizes.
18. `NNNNNNNN.SMD` layout and how a pupil's number is chosen (Naam is
    empty under `REKEN /DAVILEX`).
19. What +0x30 / [0x46c87] (default 3) means; it gates the prize sounds.

## Data

8. What is XVGALIB.IMG (three 2,050-byte entries; first u16s read 62×33)?
9. Format of the `ASCIInn.IMG` fonts and `RRRFONT.IMG`.
10. **RESOLVED (session 3).** `TRANS.DAT` is a 255×255 50% blend table; discoveries "drawing".
11. What is in `RRRHIGH.DAT` (written at run time, not on the CD)?

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
15. The rocket (0x185d4), the exhaust (0x17e87), over-speed (0x180d3) and
    the explosion (0x17faa) objects.
