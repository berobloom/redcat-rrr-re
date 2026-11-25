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
- **clib startup modules fixup to zero-size bookkeeping segments** (XIB/XI/XIE/YIB/YI/YIE): their
  bytes merge from other modules' data segments at link time; place such a module without those and
  WLINK resolves them silently to the end of DGROUP (wrong, no error).
- **archive.org ISOs need a UA and ranged GETs:** a plain `curl` through the download redirect dies at
  0 bytes; `-A "Mozilla/5.0"` plus `-r start-end` chunks work (watcom.md, 10.6). Background downloads
  started inside a tool call are killed when it returns - keep them foreground and resumable.
- **`pkill -f NAME` kills the shell that runs it** when NAME is in its own command line (exit 144).
  Use `pkill -x` or a pid. [generic]

### Matching

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
### Dead ends on RRR: SKIP at once unless you bring a new idea

- **`objects[n]` store encoding (spawn_dropped_object 0x18759, 8 rows):** the game keeps `n << 4`
  in EAX (the alloc_object result, shifted in place) and reloads the objects pointer into
  ECX/EDX per store, so the pointer is the base and the index the displacement's index; ours
  mirrors that. `switch` is the best shape found (8 rows); if/else-if, `!kind`, `(*(objects + n))`
  give 11 rows, a `struct object *` local 19.
- **cos_sin_init 0x13c34's last 2 rows:** only `x`'s stack slot (ours [esp+0x10], the game [esp+8]);
  156 spellings (every declaration order of i, a, y, x, t; six statement orders with six declaration
  lists) stop at 2. It is Watcom's FP local layout - open question 24, not the C.
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
  [esp+0x14] and x1 at [esp+0x10], ours the other way; every other byte of the 550 matches. What
  got it to 5 only works inside racer.o (a standalone probe of hit_test alone never beats ~24):
  `sin_tbl[h]` as the second product and `(h + 0x28) & 0x1f` for the third/fourth, test 5's
  y-check as `bodies[b].y > yl` (the game loads `bodies[b].y` into a register once and compares
  twice), natural corner statement order, declaration tail `y1, x2, y2, x3, y3, x4, x1, y4`.
  ~5000 in-module declaration orders (3600 with the winning shape, 1400 with `h` moved into the
  tail, .work/probe/ht) each move another slot or flip x1 to a register - the slot *numbers* are
  not steerable from C.
- **sound_init 0x1043a's last 10 rows:** the game reuses fn_1d2a6's four register arguments as
  easy_init's; WCC386 rematerialises them in every spelling and in a 3-statement probe (aux pragmas,
  int returns, locals - nothing helps). Open question 26, not the C.
- **ground_check 0x1889d's last 15 rows:** which temporary survives the inner branch. The game keeps
  `(bodies[n].y >> 16) / 40` in EAX, multiplies into a fresh register (`mov edx,[map_w]; imul
  edx,eax; add edx,ebp; imul edi,edx,6`) and recomputes `imul eax,[map_w]` at 0x18aab; 10.5 keeps the
  product instead and so is 9 bytes shorter. The same expression with one register less busy does
  come out the game's way (.work/probe/gc3/Q.C q0/q1), so it is register pressure, not the text:
  index operand orders, 720 declaration orders, ox/oy local or inline, the product as its own local,
  an int map_w and the if/ternary/key spellings all stay at 15.
