# MATCHING: rebuilding RRR.EXE's C byte for byte

The method is wacky-re's (itself after fnv_decomp and sam-re), adapted to a 1996 Watcom C game in
a DOS/4GW LE executable:

1. pin one exact binary;
2. recover the exact toolchain;
3. match the libraries first (they are free matches);
4. keep one list of every function: the plan and the progress meter;
5. one check nobody can argue with: a function is byte-identical or it is not;
6. rewrite functions one at a time in C until each compiles to the same bytes;
7. name things with their evidence, and say what is known about every near miss.

This file is the workflow. The numbers live in `src/matched.baseline`, `src/nearmiss.txt` and
`PROGRESS.md`; do not copy them here.

## 0. The target (CONFIRMED)

`RRR.EXE` from the CD (419,179 bytes, MD5 69da5879..., `reverse-engineering/executables.md`): an MZ
stub with DOS/4GW bound in front of an LE program. Addresses are the LE image's own (code object at
0x10000), as `reverse-engineering/tools/leparse.py` loads them into `.work/le/rrr.*`.

| Range | What | Rebuilt from |
|---|---|---|
| 0x10000-0x1000F | `BEGTEXT` (`int3`, `jmp $-1`, padding) | the Watcom start-up |
| 0x10010-0x1B915 | **the game's own C**: sndman.c, main.c, new2io.c, order.c, shade.c, init.c, raceanim.c, heap.c, map.c, ... | `src/*.c` with `src/rrr.h` (this file's subject) |
| 0x1B916-0x4142F | library modules in the order WLINK pulled them in: Watcom's clib and math library mixed with Davilex's own libraries (`C:\PROTMOD\WINLIB\mgl2\fg2`, `C:\PROTMOD\SOUNDLIB\wc32\mod`), Fastgraph V4.03 and the Smacker player | Watcom's libraries as they are; the rest not identified yet |

The game's own C ends where the first library module starts (clib `sprintf` at 0x1B916, then
`fopen` at 0x1B959): the functions before it carry RRR's own source files' names, the ones after it
the libraries' (`rrr_funcs.tsv`). STRONGLY SUPPORTED. The `sprintf` module differs from 10.5's
CLIB3R.LIB in one byte (`89 d3` mov ebx,edx where the LIB has `88 d3` mov bl,dl), so the game's C
library may be a patched 10.5 or 10.6 (open question 24).

REKEN, RRRMENU and INTRO have the same structure and come later (REKEN's appended debug info names
its modules, `research/discoveries.md`).

## 1. The toolchain

- **Compiler: Watcom C/C++ 10.5.** STRONGLY SUPPORTED: 10.5's CLIB3R.LIB matches RRR in 87 modules
  byte for byte, 10.0a's in 27 (`research/discoveries.md`, "Executables").
- **Options: `-3r -s -ox`**, plus `-bt=dos` under Wine (GOTCHAS). CONFIRMED on four functions; `-4r`, `-5r`, `-oneatx`, `-os`, `-ot`
  each change at least two of them.
- **Install and Wine:** `reverse-engineering/watcom.md`.
- **Driver:** `reverse-engineering/tools/wc/wcc.sh SRCDIR FILE.C [options]` runs WCC386 under Wine.
  It writes the object and the compiler's messages (`WCC.OUT`) into SRCDIR and exits 0 only when
  the object is newer than the start of the run. Give `-fo=` a relative name: Wine wants DOS paths.
- There is no per-function optimisation control in WCC386: every function is compiled with the
  same options.

## 2. Libraries first

`reverse-engineering/tools/wc/libmatch.py [-l] rrr LIB...` rebuilds each module of an OMF library
with the fixup bytes masked and searches for it in the code object. The clib and math modules match
as they are, so they need no source. Fastgraph V4.03 and Smacker are commercial libraries: match
them against their own LIB files when we have them, do not rewrite them. Davilex's own libraries
(rctools.c, fileio.c, audio.c, ...) are C of the same compiler and are the next thing to rebuild
after the game's own C (REKEN keeps their full paths).

## 3. The list of work

- `reverse-engineering/analysis/rrr_names.txt`: every function and global by address,
  `0xADDR name  # LEVEL: why`. `src/check.sh` takes each function's address from here by name, so a
  function without an entry is not compared; a global used by `src/rrr.c` needs an entry too, or its
  fixups are listed as `unnamed`.
- `src/nearmiss.txt`: every function that compiles but differs, ranked by instruction rows
  (`reverse-engineering/tools/wc/nearmiss.sh` regenerates the numbers; the status column is kept by
  hand). Status: empty = open, `MATCH`, `SKIP: <causes, what was tried>`, `LAYOUT: <why it is not
  fixable in C>`.
- `reverse-engineering/analysis/rrr_subsystems.txt`: our grouping of the functions, for the report.
- `PROGRESS.md`, `docs/RRRPROGRESS.HTML` and `docs/RRRPROGRESS.SVG`: written by
  `reverse-engineering/tools/progress.py` from a fresh build, never by hand.

## 4. The check: `src/check.sh`

**One file per original module.** `src/rrr.h` holds the shared types, globals and prototypes;
each module whose boundary is known has its own file (`src/new2io.c`, `src/order.c`), and
`src/rrr.c` holds the rest in address order until their boundaries are found. A module boundary
shows in the game as (1) the module's error strings naming its source file, (2) its strings
starting on a fresh boundary (one padding byte after the previous module's last string), (3) a
function that ends without the shared epilogue/jump-table layout one object would give. Splitting
matters for matching: within one object Watcom merges identical epilogues across functions and
places jump tables and padding differently (order_add 8 rows -> MATCH, main 31 -> 6).

```sh
src/check.sh            # compile each src/*.c into .work/src/<MODULE>.OBJ, compare every function
src/check.sh -v         # also the differing bytes
src/check.sh 2>&1 | grep -c ' MATCH at'    # the number that counts
```

It deletes the old object first (a compile error must not score the previous one), compiles with
the options above and runs `reverse-engineering/tools/wc/objmatch.py rrr`: each public's bytes,
fixup bytes masked, against the game at its address from the names file. Then it checks that the
fixup targets agree with the names ("fixups: N named targets agree"; a `WRONG` line is a wrong name
or a wrong global in the C). Its `_TEXT DISAGREES` line is expected while functions between the
matched ones are missing: the bases agree only once the module is complete in address order.

There is no link yet: each object is compared function by function; `_TEXT DISAGREES` within one
object means its functions are not yet one contiguous run of the game's code.

**The rule:** the MATCH count only goes up. `src/matched.baseline` holds it; raise it in the same
commit as the code that adds a MATCH, and only after `src/check.sh` shows the count one higher.

## 5. Matching one function

1. **Write the C** in its module's file (or `src/rrr.c`) at its address order (address order, types and externs first).
   Disassemble the game's function with `reverse-engineering/tools/disasm.py rrr 0xADDR --func`;
   Ghidra's output (`.work/ghidra/rrr.c`) helps with the shape, but it has no Watcom register
   convention.
2. **Run `src/check.sh`.** MATCH: raise the baseline, commit.
3. **Otherwise read the diff before changing anything:**
   `reverse-engineering/tools/wc/fndiff.py rrr .work/src/<MODULE>.OBJ NAME 0xADDR`. OURS is the LEFT
   column, the game the RIGHT. `fncompare.py` (instruction counts per side, same text but different
   encoding) and `objdiff.py` (both full listings) answer what fndiff cannot.
4. **Probe spellings in bulk.** `reverse-engineering/tools/wc/variants.py` scores many copies of the
   function in one compile; compile the winner in place before believing it.
5. **Write down what you learned.** A near miss keeps a `/* differs: ... */` note above it in
   `src/rrr.c` with the causes and the spellings tried; the `nearmiss.txt` line gets the count and
   the status. Commit after each function, MATCH or SKIP.

Levers seen so far (add to this list when one produces a MATCH):

- **Loop shape.** `-ox` includes loop rotation: a `for` loop with the test at the bottom of the
  generated code becomes a test at the top only when written as `while (cond && ...)`
  (`stage_is_last`).
- **Chained assignments.** `a = b = 4000;` and `b = 4000; a = b;` compile differently
  (`clear_objects`).
- **Two-value switches nest.** `switch (finish_order[0])` with a `switch (finish_order[1])` in each
  case gives the game's `<2 / ==2 / ==4` compare tree exactly (`podium`); an if chain gives 40 rows
  and `else if` 27 (variants.py, 2026-11-23).

## 6. A near miss is not a proof: `nmrefs.py`

A matching function proves the C means what the game's code does. A near miss does not: fndiff
prints every absolute address as `@`, so a wrong global or a wrong call can hide in a low row
count. `reverse-engineering/tools/wc/nmrefs.py rrr .work/src/RRR.OBJ [-v] [NAME=0xADDR ...]` reduces
both sides to the addresses they use (globals, strings, calls, jump targets). After a change, a new
address on one side is a behaviour difference: fix it or do not keep the change.

## 7. Names, after the match, with their evidence

Names go in `reverse-engineering/analysis/rrr_names.txt` with an evidence level (CONFIRMED,
STRONGLY SUPPORTED, LIKELY, SPECULATIVE) and the reason. A function's name in `src/rrr.c` is the one
in the names file; `fn_XXXXX` stays until there is evidence for a better one. A match proves the
code, not the name.

## 8. What is left

- The game's own C, function by function (`PROGRESS.md`).
- The module boundaries inside 0x10010-0x1B915, then one source file per original module.
- Davilex's libraries, then REKEN, RRRMENU and INTRO.
- A WLINK link that reproduces `RRR.EXE`'s layout.
