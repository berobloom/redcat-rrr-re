# GOTCHAS: read every session

The traps that cost the most time, in one screen. The long list of Watcom matching levers and dead
ends is `reverse-engineering/watcom-levers.md` (from wacky-re, Watcom 9.5b): grep it when a near
miss will not move. Harness traps of the DOSBox-X driver are in `reverse-engineering/runtime-dosbox.md`.

Rules for this file: one line or two per trap; harness, tool and process traps only (facts about
the game go in `research/discoveries.md`); delete an entry when it stops being true. Mark an entry
`[generic]` when it is not about this game.

## The ones that waste whole sessions

1. **Run-time addresses move.** DOS/4GW loads code and data at different offsets per boot and
   config: derive them with `dosrun.py locate <prog>`, never copy them from notes; notes record
   LE image addresses (code object at 0x10000). [generic]
2. **A breakpoint that never fires** is far more often a wrong address than code that never runs.
   Recompute before concluding anything. [generic]
3. **Notes are not proof, and a check that cannot fail proves nothing.** Re-check a claim against
   the code before building on it (the names file had `fclose` at 0x22e32; clib's is 0x1c2ac).
   [generic]
4. **`src/check.sh` is the verdict, every other count is a hint.** A MATCH masks fixup bytes, so
   read its `WRONG sym@addr` lines too: a wrong global compiles to the same bytes. [generic]
5. **One agent per worktree and per harness.** Commit each finished function with explicit paths;
   never end with a dirty tree. [generic]
6. **The same call failed twice with the same message: stop repeating it.** Change the approach,
   or mark the QUEUE item BLOCKED and work offline. [generic]
7. **Ghidra drops Watcom register arguments** (`in_EAX`, `extraout_ECX`): read call sites in
   `reverse-engineering/tools/disasm.py`. [generic]

## This project's traps

### Toolchain

- **The Watcom 10.5 CD's loose `BINW\WCC386.EXE` is cut off at 64 KB**, and `BINNT\WCC386.EXE` only
  loads it ("Loader read error"). Install with the CD's own `DOSSETUP.EXE` (watcom.md); 10.0a's
  `WPACK.EXE` cannot unpack 10.5's packs (CRC errors, garbage).
- **Wine wants DOS paths.** Give `wcc386 -fo=` a relative name and run in the source directory
  (`wcc.sh` does); an absolute Unix path fails without a clear message. [generic]
- **The BINNT tools are stubs for the BINW programs:** a stub copied away from `BINW` fails.
  `WDISASM` hangs under Wine: use `fndiff.py` / `objdiff.py`.
- **`pkill -f NAME` kills the shell that runs it** when NAME is in its own command line (exit 144).
  Use `pkill -x` or a pid. [generic]

### Matching

- **`_TEXT DISAGREES` in check.sh is expected** while functions between the matched ones are
  missing; it means something only once `src/rrr.c` is complete in address order.
- **RRR was several modules.** `src/rrr.c` is one file; where a function's code depends on what
  else is in its module (static functions, inlining, shared string pools), note it as LAYOUT, not as
  a lever problem.
- **libmatch's short hits are not evidence.** A module under ~40 bytes can match anywhere
  (`cstrt386` "at 0x10000", `mblen` at 0x1ae36 inside the game's C); trust long modules only.
- **The function list is a recursive-descent scan (~60% of the code).** Functions reached only
  through pointers are missing until named: 0x10010 and 0x10029 are not in `rrr_funcs.tsv`.

### Levers that produced MATCHes on RRR (try these before SKIP)

- **Loop shape:** `-ox` rotates loops. The game's top-tested `while` (`cmp; jg exit` at the top)
  needs `while (i <= 4 && t[g][i] == 0) i++;`; the same `for` with `i < 5` tests at the bottom
  (`stage_is_last` 0x1ae67).
- **Chained assignment:** `a = b = 4000;` stores `b` and reloads it into `a`; two separate
  statements do not (`clear_objects` 0x14861).
- **Store order is source order:** `rrrio_load` sets `joy[0], joy[1], joy[4], joy[5], joy[2],
  joy[3]` in that order; write the stores as the game does them.

### Dead ends on RRR: SKIP at once unless you bring a new idea

(none yet)
