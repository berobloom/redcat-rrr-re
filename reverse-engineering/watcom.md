# Watcom C/C++ 10.6 (and 10.5)

The game was built with Watcom C/C++ **10.6** - the library fingerprint, evidence in
`research/discoveries.md`, "Executables". For byte-matching work either one runs under Wine:
10.5 and 10.6 emit identical segments for every module of `src/` (`tools/wc/segcmp.py`), so
`src/check.sh` counts the same with both installs.

## Install

From the Watcom C/C++ 10.5 CD (`Watcom_C++_10.5.iso`, 515,213,312 bytes):

- The loose `BINW\WCC386.EXE` on that ISO is cut off at 65,536 bytes
  (dated 1995-07-13, the rest 07-11), and `BINNT\WCC386.EXE` is a 9 KB stub
  that loads it, so the loose tree has no working C compiler. `WPP386`,
  `WLINK` and the libraries are intact.
- The full compiler (567,558 bytes) is in the install packs
  (`DISKIMGS\DISK02\PCK00017.1` + `DISK03\PCK00017.2`). 10.0a's `WPACK.EXE`
  cannot read 10.5 packs (CRC errors, garbage output); 10.5's own
  `DOSSETUP.EXE` can.

Steps used (2026-11-22):

1. `7z x -o.work/watcom/iso105 Watcom_C++_10.5.iso`
2. A copy of `.work/dos/redcat.conf` without its `[autoexec]`, memsize 64,
   mounting an empty `.work/watcom/dos/C` as C: and the extracted ISO as D:
   (CD-ROM), driven with `dosrun.py` (CONF/DOS pointed at it).
3. `DOSSETUP` -> install on hard disk -> `C:\WATCOM` -> Selective, 16-bit
   compilers off, host DOS -> copy (about 7 minutes at the config's cycles)
   -> "make the modifications later".
4. Copy the stubs `WCC386.EXE WPP386.EXE WLINK.EXE WLIB.EXE` from the ISO's
   `BINNT` into `.work/watcom/dos/C/WATCOM/BINNT` (the stub loads the BINW
   program from its sibling directory).

Result: `.work/watcom/dos/C/WATCOM` (28 MB): BINW, H, LIB386 (CLIB3R/3S,
MATH387R/S, ...). Its `LIB386\DOS\CLIB3R.LIB` is byte-identical to the ISO's.

## 10.6 (the game's version) - no installer needed

`archive.org` item `Watcom_C_10.6`, file `Watcom_C++_10.6.iso` (664,373,248 bytes,
md5 `5a252e04e3a9fd5c802d1d3fd33f458e`). Unlike the 10.5 CD, its loose tree is complete:
`BINW\WCC386.EXE` is the full 567,698-byte compiler, so there is no DOSSETUP/PCK step.

```sh
# one request through the archive.org redirect stalls at 0 bytes; a UA and ranged GETs do not
for i in 0 1 2 3 4 5; do off=$((i*120000000)); end=$((off+119999999));
  curl -sS -A "Mozilla/5.0" -r $off-$end --max-time 200 -o part$i \
    "https://ia803105.us.archive.org/6/items/Watcom_C_10.6/Watcom_C%2B%2B_10.6.iso"; done
cat part? > .work/watcom/Watcom_C++_10.6.iso && rm -f part? && md5sum $_
7z x -y -o.work/watcom/iso106 .work/watcom/Watcom_C++_10.6.iso "BINW/*" "BINNT/*" "H/*" "LIB386/*"
mkdir -p .work/watcom/dos/C106 && cp -al .work/watcom/iso106 .work/watcom/dos/C106/WATCOM
WC=$PWD/.work/watcom/dos/C106/WATCOM src/check.sh      # 10.6; default is the 10.5 install
```

10.6's `LIB386\DOS\CLIB3R.LIB` (1996-02-29) is the one RRR, REKEN and INTRO link - compare libraries,
not objects, with `tools/wc/libmatch.py` (118/113/98 modules vs 10.5's 108/104/89).

## Use

```sh
reverse-engineering/tools/watcom.sh wcc386 -3r -s -ox foo.c     # any 10.5 tool, run in the current dir
reverse-engineering/tools/wc/wcc.sh DIR FOO.C -3r -s -ox        # compile DIR/FOO.C, messages in DIR/WCC.OUT
```

Both print `WATCOM C32 Optimizing Compiler Version 10.5` (or `10.6` with `WC=` pointing at
`.work/watcom/dos/C106/WATCOM`). Give `-fo=` a relative name (Wine wants DOS paths). `WDISASM` hangs under Wine; `reverse-engineering/tools/wc/fndiff.py` and `objdiff.py`
disassemble objects with capstone instead.

The compare tools are in `reverse-engineering/tools/wc/` (from wacky-re): `libmatch.py` finds an
OMF library's modules in a program's code, `objmatch.py` compares an object's functions with the
game at their named addresses. The workflow is `MATCHING.md`.

## decomp.me

decomp.me runs the same compiler: its `wcc10.5` entry ("Watcom Optimizing C i386 Compiler 10.5",
`binnt/wcc386.exe` under wibo, adding `-zq` and its own include paths; decomp.me's
`cromper/cromper/compilers.py`). CONFIRMED for one function, 2026-11-26: scratch DKj58,
start_lights 0x1103e with `-bt=dos -3r -s -ox`, compiles to the same 111 instructions as ours
and the game. Two traps:

- The `wpp10.5` entry ("... C++ ...") compiles `src.cpp` with `wpp386`. Its code is not ours:
  compiled locally as C++ inside `extern "C"`, muis_tick goes 24 -> 151 rows, player_tick
  143 -> 216, muis_crash_tick 32 -> 79. Pick the C entry.
- The target assembly is assembled with JWasm (MASM syntax: `.386p`/`.model flat`, `extrn`,
  a `_TEXT segment ... use32 'CODE'`, `proc`/`endp`, hex as `2b4h`), not GNU as. Watcom's
  names: data `_name`, functions `name_`. The diff label is the function's `name_`.

The site sits behind a Cloudflare check: scripts (curl, fetch tools) get 403, so scratches are
read in a browser.

More from the DKj58 session (chat 2026-11-26):

- The option checkboxes cannot express `-ox` (no -ob, no -oe): type `-bt=dos -3r -s -ox` into the
  arguments line, which reads "No arguments" until edited. CONFIRMED (the option list). That `-ox`
  is `-obmiler -s` is from memory: LIKELY.
- The source pane appears only after Create; use the Custom preset. objdiff's "Unknown file magic"
  is expected (it does not read OMF: LIKELY): read the plain diff.
- To see which compiler a scratch really uses, start it with `#ifdef __cplusplus` / `#error compiled
  as C++` / `#endif`; the C++ entry prints `src.cpp(1): Error! E080: (col 2) compiled as C++`.
  CONFIRMED
- RETRACTED: "decomp.me has no Watcom 10.5 preset" - a search missed the `wcc10.5`/`wpp10.5` entries.
- Permuters: decomp-permuter (simonlindholm) supports MIPS/PPC/ARM, not Watcom, but is driven by a
  compile.sh and a .toml, so wcc.sh plus inplace.py as the scorer could drive it (not done;
  SPECULATIVE that it works). The Daggerfall decomp (brotchie) is a byte-identical Watcom 10.0a
  rebuild of unoptimised code (`-od -s -of+ -4r -d2`); its `w10_frame.py` predicts the frame layout
  from the declarations and could replace declaration-order sweeps (not tried). Its
  `docs/natural_c.md` was not read.

`reverse-engineering/tools/wc/scratch.py PROG NAME 0xADDR OURS.OBJ OUTDIR` writes a function's
target assembly in that form, with its addresses from the executable's fixup records as
symbol+offset of our object's externals, and checks a GNU twin against the game's bytes.
It also names callback stores by their function, writes the module's own strings (a debug message)
into a `CONST` segment of the target, names a strength-reduced loop's base below an array as
`_bodies-28h`, and spells out the game's long jumps that an assembler would shorten
(`db 0fh, 85h` / `dd L - $ - 4`). The last two are not yet tried on decomp.me (2026-11-27).
`reverse-engineering/tools/wc/mkscratch.sh NAME 0xADDR [SRC.c]` makes a whole scratch folder,
`.work/scratch/NAME/`: context.h, NAME.c, target.asm and target.s. It prints the standalone score
next to the in-module one, because decomp.me compiles the function on its own.
