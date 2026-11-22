# STATUS

Short: where the project stands, and what is next. History is in git; no diary.

## State

- Executables: RRR, REKEN, RRRMENU, INTRO: bound DOS/4GW LE, Watcom C/C++ **10.5**, `-3r -s -ox`
  (research/discoveries.md "Executables"; MATCHING.md). REKEN carries Watcom debug info naming its
  modules (not decoded beyond the module list).
- **Matching decompilation of RRR.EXE:** `src/rrr.c`, checked by `src/check.sh`; the count is in
  `src/matched.baseline`, the numbers in `PROGRESS.md` / `docs/RRRPROGRESS.HTML`. The game's own C is
  0x10010-0x1B915; the libraries after it are not identified beyond Watcom's clib and math library.
- Toolchain: `.work/watcom/dos/C/WATCOM` (reverse-engineering/watcom.md), Wine.
- The port (`port/`) plays the whole round; it is a new engine, not built from `src/rrr.c`.

## Next

1. QUEUE.md's first open item.
2. Otherwise: the next unwritten function of RRR's own C in `PROGRESS.md`, smallest and leaf-most
   first (functions whose callees are matched or are library calls).
3. Near misses: `src/nearmiss.txt`, first line with an empty status.
4. Later: module boundaries in 0x10010-0x1B915 (one source file per original module); Davilex's
   libraries (REKEN's debug info names their modules); REKEN, RRRMENU, INTRO.
