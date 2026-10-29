# Executables

All programs are Watcom C. The 32-bit ones are LE images bound to DOS/4GW
(the MZ stub runs `dos4gw.exe`, which the install copies alongside).

| file | size | MD5 | format | role |
|---|---|---|---|---|
| `MOTOR.EXE` | 19,194 | 8a9d7e46… | MZ, Watcom C/C++16 | shell started by RCREKEN.BAT |
| `INTRO.EXE` | 273,829 | ac7d37e8… | LE / DOS4GW | intro video (rrrintro.smk) |
| `RRRMENU.EXE` | 332,723 | 8087ab13… | LE / DOS4GW | main menu, high scores, report |
| `RRR.EXE` | 419,179 | 69da5879… | LE / DOS4GW | the race |
| `REKEN.EXE` | 348,706 | 5670a17d… | LE / DOS4GW | arithmetic exercises |
| `install.exe` | 391,150 | 4cc8054f… | LE / DOS4GW | installer (INSTALL.SCR, INSTALL.IMG) |
| `OVERZICH.EXE` | 12,564 | 0ca07273… | MZ, Watcom C/C++16 | overview ("Handig rekenen", "Wat is waar?") |
| `DAVIVIEW.EXE` | 118,046 | 0e97732c… | MZ | text viewer for LEESMIJ/DEBUG reports |
| `DOS4GW.EXE` | 265,396 | 9c689dbc… | MZ+LE | Rational Systems DOS/4GW extender |
| `Autorun.exe`, `Setup.exe` | ~251 KB | | NE (Win 3.1) | CD autorun front-end |

## LE layout

`reverse-engineering/tools/leparse.py` loads each LE image. All five have two
objects: code (flags 0x2045) at 0x10000 and data (flags 0x2043) after it, and
only 32-bit offset fixups (type 0x07), all internal.

| program | entry | code | data | fixups |
|---|---|---|---|---|
| RRR | 0x26c1c | 0x10000 +0x31430 | 0x50000 +0x3b478 | 9,914 |
| REKEN | 0x22c58 | 0x10000 +0x25f15 | 0x40000 +0x385e0 | 7,592 |
| RRRMENU | 0x1ca74 | 0x10000 +0x2cf30 | 0x40000 +0x2a240 | 7,702 |
| INTRO | 0x175b8 | 0x10000 +0x23830 | 0x40000 +0x27a20 | 6,362 |
| install | 0x1e500 | 0x10000 +0x2dd30 | 0x40000 +0x35100 | 8,016 |

The data object's virtual size includes the BSS; its stack top is the
object's end (ESP = data vsize).

## Calling convention

Watcom register convention (`-3r`/`-5r`): the first four arguments in EAX,
EDX, EBX, ECX, the rest on the stack, the callee preserves the registers it
does not return in. Variadic functions (`rc_error`, `debug_printf`) take
everything on the stack. Example: `fread(buf=eax, size=edx, n=ebx, fp=ecx)`
at 0x1c0d1 in RRR.

## Function lists

`reverse-engineering/analysis/<prog>_funcs.tsv` (from `tools/funcs.py`):
address, number of direct calls in, source file (when the function references
a `xxx.c` string) and the first strings it references. Recursive descent from
the entry plus call-target seeding decodes about 60% of each code object;
the rest is reached through function pointers or is data.

Names go in `analysis/<prog>_names.txt`.

## Source files

From `%s(%d)`-style error strings. Library modules shared by every program:
`rctools.c`, `rcascii.c`, `rcbutton.c`, `rcpcx.c`, `rcobj.c`, `rcimgret.c`,
`fileio.c`, `read.c`, `easy.c`, `commands.c`, `mouse.c`, `irq7.c`, `audio.c`,
`sndbuf.c`, `smacker.c`. REKEN.EXE keeps their full paths:
`C:\PROTMOD\WINLIB\mgl2\fg2\` (graphics: rcascii, rctools) and
`C:\PROTMOD\SOUNDLIB\wc32\mod\` (audio, notes).

RRR adds `main.c`, `init.c`, `heap.c`, `map.c`, `order.c`, `raceanim.c`,
`shade.c`, `fxs.c`, `sndman.c`, `new2io.c`, `stdlib.c`. RRRMENU adds
`menuanim.c`.
