# Watcom C/C++ 10.5

The game was built with Watcom C/C++ 10.5 (evidence in
`research/discoveries.md`, "Executables"). For byte-matching work the same
compiler runs under Wine.

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

## Use

```sh
reverse-engineering/tools/watcom.sh wcc386 -3r -s -ox foo.c     # any 10.5 tool, run in the current dir
reverse-engineering/tools/wc/wcc.sh DIR FOO.C -3r -s -ox        # compile DIR/FOO.C, messages in DIR/WCC.OUT
```

Both print `WATCOM C32 Optimizing Compiler Version 10.5`. Give `-fo=` a relative name (Wine wants
DOS paths). `WDISASM` hangs under Wine; `reverse-engineering/tools/wc/fndiff.py` and `objdiff.py`
disassemble objects with capstone instead.

The compare tools are in `reverse-engineering/tools/wc/` (from wacky-re): `libmatch.py` finds an
OMF library's modules in a program's code, `objmatch.py` compares an object's functions with the
game at their named addresses. The workflow is `MATCHING.md`.
