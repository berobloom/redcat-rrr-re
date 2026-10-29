# Open questions

## Launch / process flow

1. How does MOTOR.EXE choose the next program, and which file carries the
   state between the menu, the race and the arithmetic part (rrrio.inf?)?
   Disassemble MOTOR.EXE (16-bit, small) and RRRMENU's writer.
2. What does INTRO.EXE do besides playing `rrrintro.smk` with
   `rrrIntro.wav`? Is the intro skipped by `/NOINTRO` (INSTALL.SCR's `rcmin`
   batch) in MOTOR.EXE?

## Race

3. Meaning of map cell fields 1 and 2 (see hypotheses "Map cells").
4. What do the waypoints look like (`find_waypoints` 0x1517a), and how do
   the computer racers follow them?
5. How are the racers' sprites laid out in RC.IMG (1056 entries; rotations
   per racer)?
6. What do the 19 `.MAP` files' different sizes mean for play: is the whole
   map one lap, or a point-to-point course?
7. Where do the arithmetic questions enter the race, if at all (the HUD,
   RACESCR3.PCX, has a speedometer, fuel gauge and boost/rocket buttons:
   "SAM BOOST", "MINI RAKETTEN")?

## Data

8. What is XVGALIB.IMG (three 2,050-byte entries; first u16s read 62×33)?
9. Format of the `ASCIInn.IMG` fonts and `RRRFONT.IMG`.
10. Format of `TRANS.DAT` (65,025 bytes = 255×255: a colour blend table?).
11. What is in `RRRHIGH.DAT` (written at run time, not on the CD)?

## Tooling

12. REA cannot open LE executables, and has no working decompiler here
    (it expects Ghidra at ~/Downloads/ghidra_12.1.4_PUBLIC, which is missing).
    The workaround is `leparse.py`'s ELF wrapper; a Ghidra install would add
    decompilation.
