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
- **Under Wine the compiler targets NT unless told otherwise:** `__NT__` is defined and `struct find_t`
  gets a 256-byte name (frame 0x120, not the game's 0x2c, `movie_dir_exists` 0x1002e). Always pass
  `-bt=dos` (`src/check.sh` does).
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

- **A `movzx r, r8` before a call = a cast in the source, not a char parameter:** `snd_play` 0x10350
  takes an int and calls `sndbuf_play(track, sample, (unsigned char)vol)`; with an `unsigned char`
  parameter its callers (play_fx) get a `movzx` the game does not have.
- **`a || b` in a test with the same consequence is two `if`s in the game** (`/NOSOUND`, `/NS` in
  sound_init 0x1043a: each its own `return -1`).
- **A fill loop, not memset:** `for (i = 0; i < 4; i++) tracks[i] = -1;` gives the game's `_STOSD`
  call with ECX = 4, EDX = -1; `memset` gives other registers (sound_init).
- **Locals' stack slots follow declaration order, not first use:** sound_init's irq/port/dma slots
  matched only with `int dma, port, irq` (score all orders in place).

- **A register set up before a call that never reads it = the NEXT call's argument, hoisted:** the
  compiler loads it early because the callee preserves it. init_game 0x10817's `mov ebx, esi` before
  cd_check(0, "intro.img") is has_arg("/V")'s argv; declaring cd_check with three parameters cost 5
  rows. Read the callee before giving it the register as a parameter.
- **A caller that passes a short without `movsx` where the callee's own parameter is short:** the
  calling module declared it with an int. Model that with an alias:
  `int main_sound_init(int argc, char **argv); #pragma aux main_sound_init "sound_init_";`.
- **Stack arguments with `ret N` and only ESI/EDI saved:** `#pragma aux f parm routine [] modify [eax
  ebx ecx edx];` (init_game). Library functions called with pushes and no `add esp` get `parm routine
  [] modify [eax ecx edx]`; without the modify list the caller keeps values in ECX across them.
- **A short argc passed to a function taking int is `movsx`ed by the caller** (has_arg's callers), so
  the callee's parameter is int, not short.

- **Check a stack-convention callee's real modify list in its own code:** delay_ms 0x26beb changes
  EAX, EBX, EDX but keeps ECX; with ECX in its pragma's modify list the caller reloads ECX after each
  call and the ECX/EBX loads come out swapped (start_lights 9 -> 3 rows).
- **Locals' declaration order picks who gets EBP and who gets the spill slot:** `int hb, ha, s;` (not
  `ha, hb, s;`) makes Watcom keep the first lookup in EBP and spill the second, as in the game, and
  with it the second index's `shl/add/mov` shape instead of a scaled index (racer_bounce 0x14534,
  15 rows -> 0).

- **A sum kept in a local and two plain stores, not a chained assignment,** when the game stores the
  left target first: `s = (x + y) >> 1; a = s; b = s;` (racer_bounce 0x14534); `b = a = (x + y) >> 1;`
  stores a first but costs rows elsewhere in the same expression.

- **`x[n++] = v` vs `x[n] = v; n++;`:** the second gives the game's `inc dword ptr [n]` in memory
  (finish_check 0x10eff, 19 rows -> MATCH).

- **Identical case bodies the game did not merge = fall-through.** draw_view 0x11526's cases 4, 5
  and 6 each have their own copy of the same blit, and case 4 runs straight into 5 and 5 into 6
  with no jump: the source had no `break` (Watcom then keeps the copies apart and uses a jump
  table; with breaks it merged them and compiled a compare chain).
- **A loop that tests only at the bottom with a non-constant limit is a do-while** in the source
  (draw_view's tile loops); a `for` gets a jump to the test at the top.
- **Strings one byte off between our object and the game, at the first string of a new source
  file:** each module's strings start on a fresh boundary in the game (main.c ends at 0x50c2f,
  new2io.c's "new2io.c" is at 0x50c30). nmrefs reports it; it is layout, not behaviour, and goes
  away when src/ is split into the original modules.

- **One object per module, or epilogues and tables move:** in a single object Watcom jumps to an
  identical `pop ... ret` of an earlier function instead of emitting its own (order_add jumped to
  main's), and pads before a function where the game has the next module's jump table (main +25
  rows). When a diff ends in a `jmp` to outside the function or in nops, look for a module
  boundary (its source-file strings, the string padding) and give that module its own src/*.c.

- **Operand order inside a sum picks the registers:** trans_generate 0x11ca2 matched only with
  `palette[a][0] + palette[b][0]`, `palette[a][1] + palette[b][1]`, `palette[a][2] + palette[b][2]`
  (one of 8 orders; the others swap EBX/ECX for a*3 and b*3).
- **`x = abs(..)` into a short, then `x * x` in the sum:** nearest_colour 0x11c11 (`imul esi, esi`);
  an int, or `x *= x`, gives `imul esi, eax`.

- **A store the game makes after a call was written after the call.** Watcom does not move a
  global store across a call: ai_unstick 0x1467c's `stuck2 = 0` sits after rand() in the game,
  so the source reset the counter after the heading statement (10 rows -> MATCH).
- **An unsigned compare (`jae`/`jb`) against a size = an unsigned global:** map_w/map_h are
  unsigned (camera_start 0x1475f's clamp).

- **`__LINE__` constants: `#line N "file.c"`.** A `rc_error(..., __FILE__, __LINE__)` guard pushes
  the original line number as an immediate (not a fixup). `#line N` alone was ignored by WCC386;
  `#line N "raceanim.c"` sets it (raceanim.c, all 28 set functions).
- **A register load hoisted above a library call = the pragma is missing a modify list.** With
  `parm routine []` alone Watcom assumes EDX survives the call and loads the next argument early;
  `modify [eax ecx edx]` puts it after the call, as in the game (fn_1bd71/fn_1bda1). A function
  that saves EBX/ECX/EDX it never uses calls something declared `modify [eax ebx ecx edx]`
  (fn_1bcb7).

- **`if (n == -1) return -1;`, not `return n;`.** With `return n` Watcom jumps to a `mov eax, n`
  before the epilogue; with the constant it knows EAX already holds -1 and jumps past it, as the
  game does (the heap.c spawners 0x180d3-0x18502, 2 bytes per function).

- **A function's entry block can move next to its caller.** In one object Watcom placed
  hud_count's first 54 bytes (the switch compares) straight after fn_15166, which tail-calls it,
  and the rest at its own place: the jumps then reach 2 KB away. Compiled in an object of its own
  it matched. A near miss whose first jump goes far outside the function means a caller earlier
  in the same file belongs to another module. `.work/objdis.py OBJ NAME` (or objmatch's publics)
  shows where our object put each function.

### Probes

- **Appended copies of a big function compile differently** (sound_init: 565 bytes as a probe copy,
  804 in place; shared tails between the copies). For anything over ~100 bytes score with
  `reverse-engineering/tools/wc/inplace.py NAME 0xADDR variant.c ...`, which swaps the definition in
  a copy of src/rrr.c. `probe.sh` is fine for small leaf functions.

### Dead ends on RRR: SKIP at once unless you bring a new idea

- **A `?:` 0/1 argument the game computes with branches** (sound_init's `/STEREO`): 9 spellings tried,
  see its `differs:` note.
