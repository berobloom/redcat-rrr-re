# Architecture

What is known about how the game is put together. Unsettled parts are marked
and tracked in `research/`.

## Installation

The installer (`INSTALL.EXE`, driven by `INSTALL.SCR`, or the fallback batch
files `INSTMIN/INSTNORM/INSTMAX.BAT`) copies the programs and the IND files to
`C:\DAVILEX\RCREKEN`. The minimal and normal installs leave the large IMG
archives on the CD; the game reads them from the CD drive. MOTOR.EXE contains
`D:\INTRO.IMG`, presumably a default path it rewrites (not yet checked).

Batch files written at install time:

| batch | runs |
|---|---|
| RCREKEN | `MOTOR /DAVILEX` |
| RCSTIL | `MOTOR /NOSOUND /DAVILEX` |
| RCSND | `INSTALL /SETSOUND` (sound setup) |
| rcvesa | `/V` |
| rcmin | `/NOSOUND /NOMUSIC /V /NOINTRO` |

When `DEBUG.TXT` exists after the game exits, the batch file wraps it in
`MELDING.TXT` with the readme and opens it in DAVIVIEW.

## Programs

```
RCREKEN.BAT -> MOTOR.EXE (shell, 16-bit)
                 |-> INTRO.EXE     rrrintro.smk + rrrIntro.wav
                 |-> RRRMENU.EXE   menu: groep 3-8, Start, Rapport,
                 |                  Bediening (controls), Geluid (sound),
                 |                  high scores (RRRHIGH.DAT)
                 |-> RRR.EXE       the race (RC, RCS, SNDRCS archives)
                 '-> REKEN.EXE     arithmetic exercises (REKEN, REKENSND)
```

The order MOTOR runs them in, and how they hand over state, is open
question 1.

## The race (RRR.EXE)

`race_init` (0x10817) runs, in order (from its debug messages): allocate
objects; pick the map (`stage_map_index`, 0x1ae44, or random); load the map
(`load_map`, 0x1fc4c) and its tileset (`load_xvga_map_images`, 0x1fad6);
search the waypoints (0x1517a); init anims; load the drive anims of the three
racers RC, BULL and MUIS.

The race view scrolls a 32×40-tile map seen from above. The HUD background is
`RACESCR3.PCX` (speedometer, fuel, "SAM BOOST", "MINI RAKETTEN").
