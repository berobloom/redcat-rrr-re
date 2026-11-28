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
- **Two .OBJ files of the same C never compare equal** (`md5`, `cmp`): the OMF records and the
  fixup bytes differ even when the code is identical. Compare segments masked with
  `tools/wc/segcmp.py A.OBJ B.OBJ` (used to prove 10.5 and 10.6 generate the same code).
- **WLINK wants a definition for every EXTDEF a linked module lists, even dead ones**: Fastgraph
  4.03's ASM modules list DATA.ASM globals their code never touches, and the link fails on them with
  no fixup site anywhere to read an address from.
- **XIB/XIE/YIB/YIE are WLINK's own symbols, not a module's** (2026-11-25): they are the bounds of
  the merged XI and YI runs, so a clib module that references them (inirt386) links with those four
  fixups aimed silently at the end of DGROUP, and no error, unless the link contains XI/YI runs of
  the right length. Fix: give the data blob segments those names (`blob.py --dseg XI@6c668
  --dseg YI@6c6a4-12`). WLINK's run order and the 4-byte run alignment are in discoveries "the data
  object's segment order".
- **FILES order also picks the *symbols* WLINK invents** (2026-11-26): XIB/XIE/YIB/YIE wrap the XI/YI
  runs only if a module named them before the run was laid out, so a data blob carrying those runs has
  to be fed after inirt386; fed earlier the four symbols sit at the end of DGROUP and inirt386's
  fixups aim there with no error. Same class of trap as the entry above.
- **one merged run per segment name means one object per gap** (2026-11-26): two SEGDEFs of one name
  in one object always merge, so placing a real object's `_DATA` inside a blob range forces the blob
  to be split into several objects, and their FILES position fixes both objects' contents. Feed each
  piece as late as its run allows: the fixup records follow, and a data blob fed early costs thousands
  of bytes of them.
- **an empty segment still aligns its run** (2026-11-26): almost every clib module declares empty
  CONST/CONST2/_DATA with dword alignment, and WLINK pads the run for them. A blob piece that starts
  off a 4-byte boundary, fed late past such modules, lands 2 bytes late (dosret + SETMODE pilot).
  blob.py feeds the piece's head up to the boundary right after the placed segment.
- **WLINK's data run order is first-heard order** (2026-11-26): whichever object first names a
  segment decides where its run goes. Grouping a CONST piece with a _DATA piece in the first data
  object put _DATA ahead of CONST2, and the whole data object moved (322770). The first code blob
  names the leading runs.
- **input_order's block split is ambiguous** (2026-11-26): several partitions of a page's fixup list
  into 512-byte blocks can be valid, and with a single blob any of them rebuilds the page. A real
  object in the middle only lines up with the true one. blob.py takes the split with the fewest
  address descents.
- **a real `_BSS` needs an address too** (2026-11-26): only giving it a length (appending it to the
  _BSS run) moves every game BSS variable after it, and the data pages still look identical. Only
  the code's fixups show it (6426 of 9909). `dataplace.py` gives `_BSS=ADDR`, and blob.py needs
  `--dseg _BSS@6c96c:BSS`.
- **archive.org ISOs need a UA and ranged GETs:** a plain `curl` through the download redirect dies at
  0 bytes; `-A "Mozilla/5.0"` plus `-r start-end` chunks work (watcom.md, 10.6). Background downloads
  started inside a tool call are killed when it returns - keep them foreground and resumable.
- **`pkill -f NAME` kills the shell that runs it** when NAME is in its own command line (exit 144).
  Use `pkill -x` or a pid. [generic]
- **The same goes for `pgrep -f` waits:** background sweeps that waited for "the previous sweep" by
  pattern matched their own command line and waited on themselves - three queued sweeps (player_tick,
  bull_tick, player_crash_tick) sat over half an hour without one compile. Chain them in one job or
  wait on a pid. [generic] (chat 2026-11-26)
- **A variant generator that finds no anchor text must fail loudly:** a want-switch run expected an
  old declaration order, wrote no variants and "ran" without error. Assert that each substitution
  happened. [generic] (chat 2026-11-26)
- **Ghidra refuses a project path with a dot-directory (`.work`):** decomp.sh makes its project with
  `mktemp` under `$TMPDIR`, so a `TMPDIR` inside a dot-directory breaks it again. (chat 2026-10-29)
- **Watcom's assembler does not run here:** WASM from the main install fails under Wine and the 10.5
  CD's copy hung, so a MASM target or an `.asm` source (blit_sprite 0x11bb4, blit_sprite_trans
  0x17d94) cannot be assembled locally; scratch.py checks a GNU twin instead. (chat 2026-11-26)
- **The port's build can go stale against a header:** reken_sim printed empty sums after reken.h
  changed until a clean rebuild. Rebuild clean before trusting a check that suddenly prints
  nothing. (chat 2026-10-29)
- **A Unicorn check must restart its emulator after the original crashes on a seed:** check_reken.py
  went on in a half-finished machine and reported 4 bogus differences in 25,000 (open question 22's
  "fault in the check"). Same for any new emu.py check. [generic] (chat 2026-10-29)
- **The race ignores input until the start lights end (~5 s; main's first pass runs start_lights
  0x1103e):** a key held during the lights proves nothing. RETRACTED 2026-10-29: "keys do not reach
  the browser build" - they did; the arrow was held during the lights. LIKELY, not retested:
  xdotool under a WM-less Xvfb does not deliver keys to an SDL window.
- **GitHub's image proxy caches README media:** a replaced GIF kept showing the old clip; a new file
  name (new URL) fixed it, and the old GIF stays in the history. [generic] LIKELY (chat 2026-10-30)
- **Setup outside the repo:** Emscripten is a symlink `.work/emsdk -> ../../a2/.work/emsdk`; Ghidra is
  expected at `~/Downloads/ghidra_12.1.4_PUBLIC` (`GHIDRA_INSTALL_DIR` overrides). If Ghidra is
  missing, ask, and meanwhile use disasm.py/xref.py.
- **`reverse-engineering/tools/vault.py` writes the Obsidian vault** (`~/Documents/Knowledge/redcat-rrr-re`)
  from these files: rerun it after changing them, never edit its notes (they are overwritten).

### Matching

- **A `return` whose jump lands in another block's `xor eax,eax` can mean the function's LAST
  return is not a constant.** WCC sends an early `return 0` to the final `xor eax,eax` when it can
  share it. bull_tick's period return lands in the crash block (0x128af) because its end is
  `mov eax,ebx; ...; xor eax,ebx` (y ^ y, still 0). Read the epilogue before probing the head.
  WCC 10.5 compiles `t ^ t` on one variable to `xor eax,eax` and propagates int copies, so
  `xor eax,R` after `mov eax,R` needs two temporaries the optimiser cannot merge, e.g. an int store
  to memory (`(gi = t) ^ t`) or a merge of two paths that both yield t (`r = t ? t : 0;`, which
  leaves no branch). An assignment to a short field always adds a movsx (open question 34). [generic]
- **A near miss tuned under a wrong statement is tuned to the wrong optimum.** Fixing bull_tick's
  tail and head (both CONFIRMED against the game) took it from 50 to 100+ rows: WCC allocates over
  the whole function, so the declaration order and spellings chosen under `return 0;` no longer fit.
  Keep the confirmed statement in every later search, even while src/ holds the lower-count text.
- **A control-flow merge ends WCC's reuse of computed addresses.** After `r = y ? y : 0;` (or any
  if/else) WCC recomputed `movsx ebx,si; shl ebx,4` for the next `objects[n]` instead of reusing the
  offset it already had, and then folded the earlier offset into a pointer. Putting the store before
  the merge (`objects[n].y = y; if (y) r = y; else r = 0; return r ^ y;`) kept the offset in one
  register to the end, as the game does (bull_tick, 100 -> 63 rows). If the game reuses a value
  across a spot where ours recomputes it, look for a merge between the two in our C. [generic]
- **A wrong *field* of a right global is invisible to every check:** the fixup masks the whole
  `[reg + disp]`, so `bodies[r].f1c = 0` and `bodies[r].speed = 0` score alike (check.sh, fndiff and
  nmrefs all print it as `@`). Only the game's raw displacement says, and a second site pins the
  base - 0x1b2f0's `mov [r*40+0x6cc08],0` is bodies+0x18 (speed) because the boost body's narrowed
  `add word [r*40+0x6cc0a],8` in the same function is bodies+0x1a. [generic]
- **`_TEXT DISAGREES` in check.sh is expected** while functions between the matched ones are
  missing; it means something only once `src/rrr.c` is complete in address order.
- **RRR was several modules.** `src/rrr.c` is one file; where a function's code depends on what
  else is in its module (static functions, inlining, shared string pools), note it as LAYOUT, not as
  a lever problem.
- **nmrefs filtered library calls off the game's side only**, so every MATCHed function that prints a
  string printed `DIFFER` (an unnamed 0x40000-0x60000 address is a clib call on the game's side and one
  of our own CONST literals through the segment fixup on ours). Both sides are filtered now (2026-11-28,
  `main` 0x111e8: DIFFER -> same, 75/75 refs). Read `DIFFER` on a MATCH as a tool bug first.
- **libmatch's short hits are not evidence.** A module under ~40 bytes can match anywhere
  (`cstrt386` "at 0x10000", `mblen` at 0x1ae36 inside the game's C); trust long modules only.
- **The function list is a recursive-descent scan (~60% of the code).** Functions reached only
  through pointers are missing until named: 0x10010 and 0x10029 are not in `rrr_funcs.tsv`.
- **An `e8` byte inside an immediate is not a call.** `fn_1963e` 0x1963e's only "caller" (0x18f3e)
  is `mov eax, 0x6fbe8`; it is a basic block of `player_crash_tick` 0x19497, and `entries.py` still
  calls it `loose ... own boundary`. Check the bytes around a "call" before splitting a function.

### Levers that produced MATCHes on RRR (try these before SKIP)

- **Block splits: Watcom ends a basic block every 75 instruction ids.** At each statement start the
  code generator splits the current block once `InsId - first_id >= 75` (BlkTooBig, 10.6's loaded
  image 0xa11ee5; Open Watcom `makeblk.c`, `INS_PER_BLOCK`). Ids are counted when an instruction is
  *generated*, so copies, folded address arithmetic and dead stores count although the optimiser
  deletes them. The scheduler and the register allocator never cross that boundary, so a store
  "stuck early" or a pair swapped at the end of a long straight-line run can be a split in the wrong
  place: no listing shows it. Probe: `reverse-engineering/tools/wc/splitprobe.py NAME 0xADDR` pads
  each statement with n dead stores; a row count below control means a split decides the function.
  The fix is the natural source with more (or fewer) ids, never the dead stores: init_objects 0x148db
  reaches 0 rows when BULL's and MUIS's stats stores are written `stats[bodies[2].stats].fuel` like
  every other line (constant propagation from `bodies[2].stats = 1;` folds them to `stats[1].fuel`,
  same bytes, more ids). CONFIRMED (2026-11-28, Watcom 10.6 under winedbg, research/open-questions.md
  35). Check `src/check.sh | grep WRONG` after such a change: moving the split re-sorts other
  same-shaped stores, and fndiff's masked fixups hide a swapped field.

- **`imul ebx; shrd eax,edx,16` is Davilex's inline fixed-point pragma, not C arithmetic:**
  `fixmul = "imul ebx" "shrd eax,edx,16" parm [eax] [ebx] value [eax] modify [edx]` and `fixdiv`
  (`cdq; shld edx,eax,16; shl eax,16; idiv ebx`) in src/rrr.h. The argument order picks which value
  goes into EAX and which into EBX (`fixmul(0x4000, abs(vz))` vs `fixmul(vx, 0x2666)`). The pattern
  `f7 eb 0f ac d0 10` occurs 79 times in RRR's code (first at 0x123e6); hud_update 0x1918d and
  fn_1733e 0x1733e MATCH with it. CONFIRMED (chat 2026-11-22).
- **Same index variable, not the same value:** two sites that write `stats[bodies[2].stats]` and
  `stats[bodies[4].stats]` get two temporaries with different register pairs, so the `*3 << 4` chains
  never merge and the function grows. Reading one of them through a local the function uses again
  (`h = bodies[2].stats; stats[h].damage -= 10;`) merges the chains: dropped2_tick 0x178f6 16 -> 2
  rows, dropped0/dropped4_tick and five callbacks to MATCH (117 -> 124). A dedicated `int s` fails,
  a `short s` merges but truncates the load.
- **Loop shape:** `-ox` rotates loops. The game's top-tested `while` (`cmp; jg exit` at the top)
  needs `while (i <= 4 && t[g][i] == 0) i++;`; the same `for` with `i < 5` tests at the bottom
  (`stage_is_last` 0x1ae67).
- **Two calls in an if/else, not `cond ? 1 : 0`:** Watcom cross-jumps them into the game's
  `test ax,ax; je; mov eax,1; jmp; xor eax,eax` (sound_init's `/STEREO`, 13 -> 10 rows); the ternary
  gives `setne al; movzx eax,al`.
- **Chained assignment:** `a = b = 4000;` stores `b` and reloads it into `a`; two separate
  statements do not (`clear_objects` 0x14861).
- **Rows that are only stack displacements: change the frame, not the spelling.** WCC puts the 4-byte
  scalars in a band at the top of the frame, then a gap (its FP temporaries / alignment), then what it
  will not band - local arrays and 8-byte objects - at the next 8-byte-aligned offset; unused locals,
  dead stores, propagated constants and `register` cost no frame space, and declaration order only
  picks which scalar gets which band slot. Read the frame with `tools/wc/stackmap.py
  .work/src/<MODULE>.OBJ rrr NAME` (slots touched + holes) and land the value where the game's sits:
  cos_sin_init 0x13c34 stopped at 2 rows through 156 spellings of a local `x` (always [esp+10]) and
  MATCHed as `int v[3]` with the value in `v[2]` (game: 12-byte object at [esp+0..0xB]) with the angle
  expression written twice so WCC CSEs it (no `double t`, frame stays 0x20); 127 -> 128. Found by
  sweeping 216 array-element x declaration-order spellings - the sweep is in git,
  `tools/wc/sweeps/cos_sin_init_frames.py`, its lesson is the frame rule above. [generic]
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
- **Clamp the parameter, not a copy of it:** `if (dist < 1) dist = 1;` (play_fx 0x103d1, 2 rows ->
  MATCH). Watcom then moves the parameter into EBX *before* the call because it must survive it, as
  in the game; `vol = dist; if (dist < 1) vol = 1;` after the test copies it after the call instead,
  and every spelling that keeps a separate local before the test is worse.

- **Watcom re-associates sums; parentheses keep the game's common subexpression:**
  `(short) (((bodies[b].x + dx1) + dx2) >> 16)` and `(short) (((bodies[b].x + dx1) - dx2) >> 16)` give
  the game's one `add edx,[esp+dx1]` shared by both corners; without the inner parentheses Watcom
  adds `dx1 + dx2` first and recomputes per corner (racer_contact 0x1404a, 58 -> 35 rows).

- **The order of the declaration list picks the stack slots and who keeps a register** (EBP here);
  `int hb, ha, s;` rather than `ha, hb, s;` gives racer_bounce 0x14534 the game's EBP/spill choice and
  with it the `shl/add/mov` index shape instead of a scaled index (15 rows -> 0). On a function with
  ten spilled locals the right order is worth searching in bulk: 54 orders through `inplace.py` took
  racer_contact 0x1404a from 110 to 25 rows (its exact slot map is still missing).

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

- **Plain C division reproduces both of Watcom's forms.** `x / 32` → the `sar 0x1f` / `shl` /
  `sbb` / `sar` shift trick; `x / 40` → `mov reg, 40` + `idiv reg` (a non-power-of-two constant
  goes into a register). camera_follow 0x138ed needs both, and neither needs help.

- **Do not retype a global to force a load width.** The game loads `view_tiles_w`/`view_tiles_h`
  (0x60044, 0x60048) as *words* inside camera_follow but as dwords in new2io.c: Watcom narrows
  where it knows the value fits. Declaring them `short` in rrr.h costs two byte-identical
  functions there for two rows here (110 -> 108).

### Probes

- **Appended copies of a big function compile differently** (sound_init: 565 bytes as a probe copy,
  804 in place; shared tails between the copies). For anything over ~100 bytes score with
  `reverse-engineering/tools/wc/inplace.py NAME 0xADDR variant.c ...`, which swaps the definition in
  a copy of the src/*.c that defines NAME. `probe.sh` is fine for small leaf functions.
- **A probe object holding several functions misreads under `variants.py`:** it compares each name
  against the same [ADDR,END) window, so a five-function probe of 0x1401d-0x1467c reported
  racer_contact at 294 rows where `reverse-engineering/tools/wc/fndiff.py rrr PROBE.OBJ
  racer_contact 0x1404a` says 0. Score such an object per name with fndiff.py at the game's address.
- **Variants of one function merge with each other in a shared probe object,** so both their length
  and their row count lie (ground_check: 648 spellings in chunks of 54 ranked 25-51 rows, and the
  best of them, 575 bytes, was really 575/15 only when compiled alone; most of the "525 bytes"
  entries were cut off mid-function). Spellings of one near miss need one object each: WCC386 takes
  only one source file per run (`E1139`), so a sweep is one Wine start per variant - ~10 s each
  (.work/probe/gc3/one.py, sweep1.py). Static barrier functions between the variants do not stop it.

- **Do not run several WCC386 passes in parallel in a sweep:** with six Wine starts sharing a driver,
  objects come out missing or truncated and the scorer reports phantom scores (a 2400-variant sweep
  and a 1000-pragma sweep both read rows `-2`/wrong in parallel and read 3/5/8 when the same variants
  were compiled one at a time). Rank with one compile at a time, and re-check any hit serially.
  Private directories per compile do not make it safe: the muis_crash_tick sweeps ran eight in
  parallel that way and only the two winners were re-checked, so their "nothing below 6" floors are
  LIKELY until rerun serially (chat 2026-11-26).
- **`variants.py`'s END is the function's own end, not the next public.** The cos_sin_init frame sweep
  scored its MATCHing spelling as 2 rows because it passed END=0x13cbc, two bytes past the function's
  real end 0x13cba (0x13c34 + 134): the neighbour's head counted as diffs. Compute END from the known
  byte length before ranking variants; `fndiff.py`, which cuts at the next public, said 0 all along. [generic]
- **A slot regex that demands the hex prefix silently reports no hits.** capstone prints
  `[esp + 8]`, not `[esp + 0x8]`: my sweep's slot parser matched only the `0x` form, so a sweep that
  had already found the answer reported "no hit" and cost a round. Parse both forms and sanity-check
  the parser on a function whose slots you already know (`tools/wc/stackmap.py` does both). [generic]

- **An epilogue that is not shared does not prove a module boundary.** WCC shares an epilogue only
  between functions whose epilogues are identical, so three neighbours that each end in their own
  pop/ret say nothing if those differ anyway (the three normal ticks: 0x1281a `xor eax,eax;
  add esp,0x10`, 0x130ec `xor eax,ebx; add esp,0x10`, 0x138e1 `xor eax,eax; add esp,0xc`). Our
  bull_tick could jmp into muis_tick's epilogue only because our frame was 0xc where the game's is
  0x10 - the frame, not the object. Check the frame map before reading a boundary. [generic]
- **A near miss 4 bytes short of the game's frame is missing a spilled local, and that is why its
  register homes never move.** `tools/wc/stackmap.py .work/src/<MODULE>.OBJ rrr NAME` prints our frame
  and slots; count the game's `[esp+…]` uses in the same way. bull_tick 0x12826: the game uses all
  four slots of its 0x10 frame (14 accesses), ours touches two of 0xc, and 22 of its 50 rows are
  homes (2026-11-27). Same check says when a frame already matches, so the rows are not a slot
  question (muis_tick 0x130f8: frame 0xc, slots [esp], [esp+4], [esp+8], equal to the game's). [generic]

### Dead ends on RRR: SKIP at once unless you bring a new idea

- **You cannot force Watcom to spill by taking a local's address.** `int *pd = &d;` and reading
  `*pd` later does not reproduce a game function's extra stack slot - it restructures everything
  (bull_tick 0x12826, 50 -> 215 rows, .work/scratch/bull_tick/v_pd.c). A stack slot in the game that
  we keep in a register means its allocator had more live values there; find what is live, do not
  fake the spill. [generic]
- **A stats-index holder is wrong where the game re-derives the index.** `s = bodies[n].stats;` +
  `stats[s]` is what took dropped2_tick 0x178f6 16 -> 2, but in bull_tick 0x12826 it costs 245 rows
  (295) because the game reloads `bodies[n].stats` in every block there - the holder also merges the
  damage/fuel tests' index chain, which the game does not merge. Same text, opposite verdict: check
  the per-block loads in the game's listing before introducing a holder. [generic]

- **`objects[n]` store encoding (spawn_dropped_object 0x18759, 8 rows):** the game is an if/else-if
  chain (kind == 2, then kind == 0) keeping `n << 4` in EAX and the objects pointer in EDX per arm.
  Our `switch` has the game's registers but two other things: Watcom lays out its own compare tree
  (`test edx,edx; jbe` first), and the default arm recomputes `n << 4` (306 bytes, the game 301).
  Every if shape gets the game's tree but mirrors the registers (11 rows, 299 bytes); `!kind`,
  `(*(objects + n))` also 11, a `struct object *` local 19. (Corrected 2026-11-26 from chat; the old
  text described an if/else build.)
- **The droppedN_tick damage block's load register (open question 25):** the game loads
  `bodies[2].stats` into EAX, ours into EDX, and no spelling or compiler option moves it (30+ tried).
  It is worth 8 functions, so it is worth a fresh idea, not another 30 spellings - see the note in
  src/shadeobj2.c before starting.
- **follower_tick 0x167aa's last 9 rows:** two Watcom choices, the short parameter's sign extension
  (`cwde` + `mov esi,eax` + `dec eax` in the game, `movsx` + `lea` in ours) and an EAX/EDX swap in the
  `frame = 32` store; ~60 variants stop there (.work/probe/ft2). What did work, and is worth trying
  in the other object callbacks: the two-part test written as `!(damage > 0 && fuel > 0)` (30 rows
  better than `damage <= 0 || fuel <= 0`), and follower_tick alone in its own file so its epilogue
  stops merging into fn_16e46's (4 rows).
- **hit_test 0x1430e's last 5 rows (71 -> 5):** only two swapped stack slots - the game has h at
  [esp+0x10] and x1 at [esp+0x14] (0x14371 `mov [esp+0x10],eax`, 0x1440b `mov [esp+0x14],ebx`), ours
  the other way; every other byte of the 550 matches. What
  got it to 5 only works inside racer.o (a standalone probe of hit_test alone never beats ~24):
  `sin_tbl[h]` as the second product and `(h + 0x28) & 0x1f` for the third/fourth, test 5's
  y-check as `bodies[b].y > yl` (the game loads `bodies[b].y` into a register once and compares
  twice), natural corner statement order, declaration tail `y1, x2, y2, x3, y3, x4, x1, y4`.
  ~5000 in-module declaration orders (3600 with the winning shape, 1400 with `h` moved into the
  tail, .work/probe/ht) each move another slot or flip x1 to a register - the slot *numbers* are
  not steerable from C.
- **ground_check 0x1889d's last 13 rows (41 -> 15 -> 13):** setting `cy` again to the current row
  before the second lookup fixed the d == -1 arm (2026-11-26); what is left is the second lookup's
  kept `cy * map_w` product (ours 1 byte longer, everything after it shifted). The analysis of the
  15-row state: which temporary survives the inner branch. The game keeps
  `(bodies[n].y >> 16) / 40` in EAX, multiplies into a fresh register (`mov edx,[map_w]; imul
  edx,eax; add edx,ebp; imul edi,edx,6`) and recomputes `imul eax,[map_w]` at 0x18aab; 10.5 keeps the
  product instead and so is 9 bytes shorter. The same expression with one register less busy does
  come out the game's way (.work/probe/gc3/Q.C q0/q1), so it is register pressure, not the text:
  index operand orders, 720 declaration orders, ox/oy local or inline, the product as its own local,
  an int map_w and the if/ternary/key spellings all stayed at 15.

## Working with agents

Lessons from the chat history (2026-10-29..11-26) about how the user wants the work done, and what
went wrong between sessions.

- **Never delete in a command: write each attempt to a new directory.** The user stopped `rm -r` and
  an `rm -f $L/*.OBJ` on a variable-built path and said to go on "without rm"; probes and link tries
  go to fresh folders since. CONFIRMED (chat 2026-11-22, 2026-11-25)
- **Loops in a short Python script, not shell loops with `$(( ))` and command substitution** (the
  permission check flags those; the user: "Sure with python"). CONFIRMED (chat 2026-11-22)
- **When the permission check blocks a step, hand the user one `!` command and stop;** do not route
  around it (so far: `gh repo create`, running a downloaded PATCH.EXE in DOSBox, posting to outside
  sites). The `!` command's output comes back into the session. CONFIRMED
- **Uploading game code is the user's call** (a decomp.me scratch, a Discord post, a public repo):
  prepare the files (`.work/scratch/<fn>/`), the user creates and shares, and pastes scores back
  (decomp.me gives scripts 403). CONFIRMED (chat 2026-11-26)
- **Push this repo only when asked.** CONFIRMED (chat 2026-11-26)
- **Say so when you add a tracked file the user did not ask for.** LIKELY (chat 2026-11-26)
- **Keep sweep generators as tracked tools with their inputs,** not inline Python in the chat: after
  the muis_crash_tick work the saved generators read relative paths never copied, and the variants
  the score tables name were in a temporary scratchpad. CONFIRMED (chat 2026-11-26)
- **A brief for another model goes stale with the text:** BRIEF.md for muis_crash_tick called four
  spellings ruled out and "all 288 declaration orders exhausted"; under the later text a `go, m` swap
  gave 32 -> 24 and the four spellings together 23 -> 6. Date a brief, regenerate it after each
  change, and check any outside result with check.sh and nmrefs (a model can improve a diff by
  changing a condition or a constant: LIKELY, not seen yet). CONFIRMED (chat 2026-11-26)
- **An unexpected diff can be a live session, not an interrupted run:** on 2026-11-25 ~230 lines in
  blob.py and libmatch.py came from another session working at the same time. Check for a running
  agent, or ask, before building on uncommitted work (trap 5). CONFIRMED
- **A READY FOR REVIEW note can sit under the wrong QUEUE item** (Q5 carried Q2's library-map note,
  so "first open item" pointed wrong): check the note answers that item's Accept line; move a
  misfiled note, never delete it. CONFIRMED (chat 2026-11-25)
- **Do not explain a tool output you only remember; rerun the command.** A local model built turns
  of theory on a "114/114 checks passed" no command printed (`2>/dev/null` had hidden "No such
  file"). [generic] CONFIRMED (chat 2026-10-31)
- **Keep "located" apart from "understood":** finding where data lives is not knowing the logic that
  uses it. [generic] CONFIRMED (user correction, chat 2026-10-31)
- **Correct the user when there is a better option** ("Always correct me if needed"). CONFIRMED
- **Search online yourself before asking the user for a file;** if the user stops a fetch, do not
  retry it unless asked. CONFIRMED (chat 2026-11-22, 2026-11-25)
- **Offer `[generic]` traps to DREA** (`~/projects/active/DREA`, the user's DOS RE template) for its
  GOTCHAS/TRAPS. CONFIRMED (user instruction)
