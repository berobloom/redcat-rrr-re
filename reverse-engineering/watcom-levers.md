# Watcom matching levers, from wacky-re

wacky-re (Wacky Wheels, Watcom C/C++32 **9.5b**, `-s -3r -oneatx`) collected these while matching
213 functions. Only its entries tagged `[generic]` are copied, unchanged; the function names and
addresses in them are Wacky Wheels'. This game uses Watcom **10.5** with `-3r -s -ox`, so a lever
here is a thing to try, not a fact about RRR: when one works or fails on RRR, write that in
`GOTCHAS.md` ("Levers" / "Dead ends") with the RRR function. Grep this file; do not read it whole.

### Reading a near-miss diff (do this before spending variants)

- **difflib's autojunk inflates row counts on long functions.** `SequenceMatcher`'s default ignores
  lines that make up over 1% of a 200+ line list (`call @`, `mov eax,@`), so whole blocks misalign:
  fn_17928 counted 171 rows with it and 35 without, race_run 697 vs 213, and a harmless edit to main
  read 80 -> 192. Turning it off is not always better (netkeys_in 29 -> 48), so `fndiff.align()` takes
  the smaller of the two; fndiff, variants.py and progress.py use it (iter 39). Use it in any new
  scorer. [generic]
- **`src/check.sh` is the verdict, every other count is a hint.** `fndiff.py` (ours LEFT, game
  right) and `fncompare.py` print displacements as `@`, so swapped stack slots show 0 rows while
  check.sh says "differs" (net_send 0x2dc38). A MATCH masks fixup bytes, so read the `WRONG sym@addr`
  lines after it too (fn_1c88c loaded save_c for save_a). [generic]
- **A function out of address order can own another's jump table.** fndiff and check.sh measure a
  function up to the next public, and Watcom puts a switch's table in front of its function, so a
  function standing before a switch function in `src/ww.c` but not in the game counts that table as
  its own rows: ent_sprites_load (0x21310) stood before main_menu and read 75 rows, 43 of them
  main_menu's table, called "padding" for 40 iterations; in address order it is 32 (iter 56c). Check
  that a near miss's neighbours in the source are its neighbours in the game. [generic]
- **Fake rows to subtract first** (iter 39: fndiff, variants, permute and nmrefs now fold the `- @`/`+ @`
  pair themselves, so their counts no longer include it): a `[reg - 0x6f6f6f70]`/`[reg +/- @]` pair is a masked fixup
  (0x90 filler) - 7 of self_project's 10 rows; rows past the end of a shorter function are its own
  epilogue or alignment (race_records_menu, ctl_label's nops); a padding row (`lea eax,[eax]` vs
  `nop`) is the size gap of a register choice above it (com_wait_byte 0x2df70). [generic]
- **Rows that are only stack displacements = the slot-permutation dead end** (below): count them
  first (showlines 0x2a268: 10 of 18). Count them, and every other class, with one pass over the two
  full listings: normalise each instruction (addresses as `@`) and subtract the COUNTERS per side -
  what remains is the register/width/epilogue gap, not the slot permutation. [generic]
- **Equal instruction counts but a byte compare that starts on a branch displacement = a register
  home, not a missing instruction.** AL/EAX own the short absolute forms (`a0`-`a3`, 5 bytes; any
  other register 6, `add dl,imm8` 3 vs 2), so one home shifts every later jump (shots_move 460 vs
  462 bytes at 0x2613b). Sum instruction sizes per side to find the store. [generic]
- **Line up the two listings' loads before probing:** load ORDER differs = 9.5b's scheduler, only a
  statement reordering can move it (net_input, fn_16ca0 fixed that way; fn_19db8's switch
  controlling expression is not a statement: SKIP). Order equal, homes permuted = the colourer, try
  the levers below, then SKIP. A whole function whose temporaries mirror the game's EAX<->EDX is
  that class: four C spellings of race_run's `x * 8` pointer walk (0x3066d, four sites) all scored
  equal to the control. [generic]
- **A `cmp` row whose only difference is the operand order is a dead end, not a lever:** 9.5b gives a
  comparison's source-LEFT operand both the cmp destination and the first register home, so flipping
  the C operands swaps the two temps and leaves the emitted `cmp` alone (car_laps 0x12ee0, 21
  spellings; intro 0x351ae, `j == k + 1` 12 -> 14). [generic]
- **Read the width of a compare AND of a load:** `cmp r16,r16` = both shorts in C, `cmp r32,r32` =
  at least one int; `movzx ax,al` vs the game's `movzx eax,al`, or `cmp dword [slot],N` vs its
  `cmp word`, is the same signal. Pair the game-only and ours-only rows that agree on opcode and
  immediate but differ in width: menu 0x155c0 `isnet` int 267 -> 233, cars_menu 0x1a380 `cd15c`/
  `slide_up`/`slide_dn` short 499 -> 476. Fix the C types before logging a register pairing
  (fn_16eac: `int cnt` 17 -> 11 rows). A
  `short` that the game keeps in a register may take a stack slot in ours and the reverse
  (fn_1c0f4 vs scores_screen): try the type before restructuring. `xor ah,ah` before `cmp ax,N` says
  `short`, not `unsigned char` (com_open's `iir` 0x10401: 33 -> 19 rows). [generic]
- **A lower row count can be a worse function**: a variant that drops one of the game's
  instructions realigns the rest (shots_clear lost `push ebx`, scored 11 vs 13). And a variant can
  print the game's text while reading the wrong value (line_draw: dx/dy rotation moved above its
  read). Check register by register that each def reads what the game's def reads. [generic]
- **Function boundaries:** a `ret` is not an end (kart_hit continues at 0x2175f after one at
  0x2175e: run `xref.py` on an "after-ret" address); no caller is not "not a function" either
  (0x1c88c and 0x1c968 are dead code with their own prologue/ret/padding). Decide from
  prologue/ret/padding. [generic]
- **globrefs prints every string of the game as `game only`** (iter 61, chat): it resolves only
  *external* fixups through symmap, and our own literals hang off *segment* fixups, which it skips.
  nmrefs does resolve them (it votes a common base), so strings from nmrefs, data addresses from
  globrefs. [generic]
- **A census tool that finds a function by `startswith` compares the wrong body, and a our-side scan
  that ignores absolute `disp32`s invents one-sided addresses** (iter 65, `tools/wc/globrefs.py`, a
  census of all 102 near misses): `nm.startswith(name)` resolved `fade_out` to the 20-byte
  `fade_out_screen_` and `main` to `main_menu_names_` (Watcom OMF publics carry a trailing underscore)
  - match `name` or `name + "_"` and print the public it picked; the last public in `_TEXT` has no
  following public, so the range end must default to the segment length or the tool dies in
  `StopIteration`, and a sweep that pipes each run through `sed -n '2,20p'` drops the traceback line,
  so that function silently contributes nothing. It also collected only ext fixups and immediates on
  our side while taking every memory displacement on the game's, so the game's
  `mov [edx+0x6e6e0],eax` 0x17a3a, `mov [edx+0x6e6e4],eax` 0x17a45 and `mov edx,[eax+0x6508e]`
  0x1828c printed as `game only` against our byte-identical operands (0x79ae, 0x79b9, 0x81f0: an
  absolute `disp32` with no fixup on it). Fix: take our MEM displacements too unless a fixup covers
  the four bytes at `ins.address + ins.disp_offset` (capstone 5 has `disp_offset`). After the three
  fixes, one near miss of 102 still had a one-sided non-string address - showlines 0x2a268, the
  relocated `&netstats[1]` addend (SAME). `tools/wc/test_globrefs.py` pins all three fixes. Fed to
  DREA the same session (harness/TRAPS.md "Static analysis tools"). [generic]
- **A `check.sh` MATCH means the bytes agree, not the operands - 4556 of them are masked inside the
  202 MATCHed functions** (iter 66, `tools/wc/fixsites.py`). objmatch checks only the *named*
  externals site by site (`WRONG sym@addr`, 3116 agree) plus one base per segment, so an internal
  `call`, a switch-table dword, an unnamed global and a string literal inside a MATCHed function are
  masked and unchecked - the very class the hybrid link's two bugs came from. Inside a MATCHed
  function the mirror address is exact (the unmasked bytes are equal), so compare site by site:
  `ext` = its symmap address + the fixup disp + the addend in our bytes; `seg _TEXT` = the target
  offset held in our bytes (the fixup disp is 0), mapped through the public containing it to that
  function's game address + intra-function offset; a `_TEXT` target below the first public is a string
  literal WCC put in the code segment (our `_TEXT+0/0x144/0x1ec/0x228` are the game's
  0x10010/0x10154/0x101fc/0x10238, "UNO", "JAN FEB MAR...", "MAINMENU", "SHAUN"); every other segment
  = one base by vote. Result 0 disagreements (ext 4133, `seg _TEXT` 94, CONST 327, `_BSS` 2): no
  MATCHed function reads another global, calls another function or passes another literal. Two traps
  on the way: a `.OBJ` data segment is a *list of offset/data records* (51 for CONST), so "its bytes
  at offset X" from the concatenated records is not what the link puts there - that version called 325
  of 327 literals different while the same 327 agree on base 0x60004; and a switch table whose targets
  lie in a near miss is not decidable at all (54 entries in 6 tables - kart_frame, fn_19db8,
  cars_menu, kart_fire, main_menu, trial_book), because a near miss's blocks are laid out differently
  and its intra-function offsets mean nothing (QUESTIONS 22). `tools/wc/test_fixsites.py` pins both.
  Its `--arms` pins are frame-slot sensitive: the C fix of iter 75 (`p7d838`'s bump, once per pass) moved
  which locals `cars_menu` keeps in registers, so the game's `[esp+0x16c]` pair is now ours at
  `[esp+0x144]` and our arm stores the 6 as an immediate where iter 74's went through DI - re-read the
  pins from the tool's output after a C change, never hand-copy them.
  Fed to DREA the same session (harness/TRAPS.md "Old compilers", commit e28a7eb). [generic]
- **The switch-table entries that census leaves over ARE decidable - compare the two arm blocks, not the
  entry addresses** (iter 67, `tools/wc/fixsites.py ww .work/src/WW.OBJ --arms NAME [-v]`). A pair is linked
  by its slot: both sides' table sits at the same mirrored offsets of a MATCHed function, so slot *i* of ours
  is slot *i* of the game's and the question is what the two blocks DO. Per block (up to its first branch or
  `ret`): mnemonic + `op_str` with the numbers masked (the game folds a base into a displacement -
  `[eax+0x6aaa2]` is our `[eax+0x12]` + fixup) followed by the resolved fields: 4-byte disps/immediates
  through the same fixup resolution the census uses, **every 1- and 2-byte immediate too** (`mov word
  [ents frame], 6` hides its 6 in a 2-byte field, and a wrong one there is exactly this bug class), jump
  targets masked and any field pointing into a near-miss body masked. `sorted(a) == sorted(b)` says "same
  instructions, another order" - then hand-check that the read-before-write order of each address the block
  touches held. Of the 36 entries left from iter 66, that found ONE bug (`cars_menu`'s carstat==0 slide arm
  never stored `ents[i].frame = 2`, game 0x1b7de: the arm reads `ents` either way so no address census saw
  it, and fndiff rows went 433 -> 431 when the store came back) and 35 same-shape (another register; the
  value carried in ECX to its store, 0x2459c -> 0x245bb; `mov ecx,[curpos]` 0x24613 hoisted above a branch;
  independent stores swapped). Capstone needs the game's virtual address, not the file offset, or every
  branch/call target it prints is off by the 0x10000 code base - that faked 3 "different call targets".
  Fed to DREA the same session (harness/TRAPS.md "Old compilers"). [generic]
- **Our C can hold the game's expression and never use it, and then both counters move the WRONG way**
  (iter 60, cars_menu: `capx = (0x140 - 8*strlen(buf1))/2` was in the source but the print call passed
  `x2e`, so 9.5b deleted the division and the caption drew at x=46 instead of centred; the same
  function was also missing one of the game's three `spr_draw` calls for the selection box, 0x1ae5d).
  Both made our function shorter and its fndiff count SMALLER (476 -> 433 while getting two things
  right), so a shrinking row count is no evidence of a fix. nmrefs named them: `game more 1x
  0x36fd0(spr_draw)` and `game only #0x140 #0x1f` (`mov eax,0x140` + `sar edx,0x1f` 0x1a456/0x1a466
  = the game's divide by the 320 px screen width). Read a length gap as missing calls, not padding.
  Fed to DREA the same session (harness/TRAPS.md, iter 60 - the wacky commit for it is the DREA commit
  "Traps: the port's own dead code deletes a computation the original performs"). [generic]

### Probes (`tools/wc/variants.py`)

- **A probe file must hold what the function sees in src/ww.c:** `#include <string.h>` (else real
  strcat/strlen calls), the same externs, and the DEFINITIONS of game functions it inlines (a
  prototype only gives a smaller function: com_wait_byte 80 vs 152 bytes). Confirm the control
  variant's size and score equal the whole-file ones before trusting a ranking (com_expect: equal
  -> rankings transferred). [generic]
- **Copies in one probe share their identical tail** (9.5b merges epilogues across functions):
  cut the score range before the shared tail (demo_mode 0x1e69f). Both sides are cut at END since
  iter 16; re-run older probes. A `variants.py` score can be 1 above `fndiff.py` from the variant's
  own filler: compare variants with each other at equal size. [generic]
- **Rank in a probe, decide in `src/ww.c`:** reordering unchanged statements re-schedules the whole
  probe (msgbox control 3 -> 9), line layout of the same tokens changed a probe's size (com_isr 338
  vs 328 bytes), and a probe's best spelling can lose in the full file (line_draw 4 -> 13). Copy the
  committed body verbatim and vary one block. [generic]
- WCC386 runs in DOSBox: probe file names must be 8.3 or it fails silently (no OBJ). [generic]

### Levers that produced MATCHes (try these before SKIP)

- **`#pragma aux f modify exact [regs]` stops WCC re-materialising register-passed arguments after a
  call - plain `#pragma aux f modify [regs]` does not**, because plain `modify` only *adds* to the
  default clobber set (this is why the 2026-11-27 sound_init probes looked inert and produced the wrong
  rule; open question 26). Declare it when the callee's own body proves the clobber set is small -
  fn_1d2a6 0x1d2a6 is `mov [0x5fe99],eax; ret`, so `modify exact [eax]` lets the game's next call reuse
  the ECX/EBX/EDX arguments set 16 bytes earlier (sound_init 0x1043a 10 -> 2 rows). [generic]
- **WCC materialises a call's arguments in reverse `parm` order**, so an explicit
  `#pragma aux f parm [edx][ebx][ecx][eax]` plus the matching argument order at the call site lands a
  game that sets its arguments in ascending register order without touching which register holds what
  (sound_init's last 2 rows -> MATCH; `parm [eax][ecx][ebx][edx]` gives the reverse, `parm
  [eax][edx][ebx][ecx]` = the default gives nothing). [generic]

- **Frame size first: our `sub esp, N` differs from the game's = a local array has the wrong size.**
  Read the array sizes off the game's slots: overlay_draw's frame 0x34 with buf at [esp+0x14] and
  dword slots from 0x20 meant `char buf[0xc]`, not 0x14 (62 -> 46, every scalar slot moved back);
  race_run's word array at [esp+0x28] with two slots from 0x3c meant `short placed[10]` although its
  largest index is 7 (0x2fd8c, 128 -> 120). Also scores_screen `long sc[10]`, time_str `buf[4]`. [generic]
- **The game CALLS a function our -oe inlines:** call an alias at those sites,
  `void f_x(int); #pragma aux f_x "f_"` (com_getc_x, waitkey_x, makepath_x, setvmode_x, ...); for
  a function called everywhere `#define readdat readdat_x` after its definition. Where the game
  DOES inline the real seekdat/readdat (effects_load 0x2754c) `#undef` them around that function.
  Defining a small game function lets -oe inline it into callers that already match (fn_1c068 broke
  net_end): xref the callers first. Nothing turns -oe off per function. [generic]
- **The register of a local follows its first DEFINITION, and among pointer temps the declaration
  order:** move the first assignment (classes_menu nrows got EDX 0x19f22); `d = q + q; c = base;
  a = c; b = c + d;` with c declared first gave track_pics_load's five instructions. [generic]
- **Route a temp through a local that already lives in the wanted register:** `n = i * 12;
  f(path, (char *)names + n);` (n later takes datsize()'s EAX) fixed songs_load's EAX/EDX swap.
  [generic]
- **Split the arithmetic with a compound assignment so the value stays live in its home:**
  `v = *p; v <<= 2; *p++ = v;` (pal_shift 0x114b0, whole-register shift + the game's registers);
  `m = v88b88 - 1; m *= 0xf3c;` with m declared first (track_load 0x27a49 `movsx ecx` + `dec ecx`).
  [generic]
- **A slot swap with a pointer the compiler made from `a[i]`: make the pointer an explicit local,**
  declared BEFORE the scalar it swaps with and assigned where the game first stores it
  (`char *nm;` ... after the preceding call `nm = names[0];` ... `nm += 12;`): sounds_load 0x12124,
  0x168/0x16c, MATCH. Declared after, or assigned earlier, keeps a swap. [generic]
- **One local shared by two switch arms has one home, two locals get two:** one `short b` for the
  receive and transmit byte fixed com_isr 0x106c8 and com_isr_fifo. [generic]
- **A 0/1 value in its own home: braced `if (c) { x = 1; } else { x = 0; }`**, not `x = (c)` (which
  computes in a scratch + `mov`): ctl_label 0x19d0e `sete al; movzx eax,al`. The `? 1 : 0` ternary
  form also changed ents_show's whole loop shape: braced, 9.5b emits the game's `jmp` to a bottom
  test with one epilogue (0x14393) instead of a top test + duplicated epilogue (49 -> 36). [generic]
- **An int stored as a dword slot and read back `movsx r, word [slot]` = an `int` local passed to a
  `short` parameter** (or `(short)n`): duck_screen's three centred x values are an `int w` (game slot
  0xc8), its loop keeps `short x` (cwde); screenshot's file counter is `int n` with `itoa((short)n)`
  (51 -> 6). Scan: `mov dword [esp+N],r` followed within 4 instructions by `movsx r2, word [esp+N]`.
  A local the game only tests and divides with is `short` as well: `cmp ax,word [esp+4]` 0x267db and
  `movsx ebx,di` + `idiv ebx` 0x26803, where an int gives `cmp r32` + `idiv dword [esp]`
  (lights_project `short sy`, `short a`: 112 -> 85). [generic]
- **`(int)` casts on both arms of a short ternary index** give per-arm `movsx` with no `cwde` after
  the join (draw_status `pnames[v7eb80 ? (int)myplayer : (int)v88c6a]`, 48 -> 30). [generic]
- **A `je` straight to the epilogue is an early `return`, not the end of an `if`:** duck_screen
  0x2bad6 `if (!after) return;` - the rebuild had run the PRESS FIRE/fade tail for both callers. Check
  every conditional jump whose target is the epilogue against the C before probing codegen.
  [generic]
- **Raw-address macros (`WS(0x6ab14)`) cost rows:** 9.5b loads the constant into a register and
  stores through it, the game addresses the symbol (`mov word [players+0x84],bx`). Rewrite with the
  struct fields of the right global (fn_16d68). [generic]
- **A constant stored to a global through a register = an `int` local assigned inside that arm**
  (`int row; ... if (...) { row = 0x5c; spr_y = row; }`; a short local or a ternary folds to an
  immediate store): msgbox 0x2a45c. [generic]
- **A conditional flip of a global = `if (g == 0) g = K; else g = 0;`, never a ternary** (the
  ternary is a value select through EAX; the game never materialises 0x3e80 in a register:
  55 `mov word [m],0x3e80` sites, 0 `mov eax,0x3e80`): cars_menu, enter_name. [generic]
- **Load order:** write a load the game emits early as an earlier statement (fn_16ca0 0x16cf9:
  `i = v7ea58s;` before `v88c7a = v88c7c >> 1;`), and sink an assignment the game loads late below
  the call that does not read it (net_key 0x2dd8a `send = 'Z'` after net_send). Independent bit
  tests in the game's load order (net_input). [generic]
- **`volatile`:** a `volatile int t` local gives the game's `sub esp,4` slot and `[esp]`
  spill/reload (shot_step 0x25f43); do arithmetic on it in its own statement (`k = last - 1;`,
  drawsprite_clip 0x11416). A `volatile` alias of a global pins a deferred store
  (`extern volatile unsigned char v88ccav; #pragma aux v88ccav "_v88cca"`, demo_play 0x2e35f) and
  restores a re-test the game repeats after stores (race_results); alias per function, a volatile
  global changes other functions. It pins stores, not loads: an `extern char *volatile` alias on the
  other side of a swapped pair of loads left the order alone (shot_step 0x25f46). A byte stack slot
  stored then reloaded with `xor edx,edx; mov dl,[slot]` is a `volatile unsigned char` local, an int slot
  after it a `volatile int` the byte is copied into (ASS FMCHIP FM_SetChannelTimbre 0x504a5, 40 -> 22
  rows, the game's frame size exactly). [generic]
- **A second typed name for one global** (`extern short w7ed8c; #pragma aux w7ed8c "_v7ed8c"`): a
  word load + `xor ah,ah` truncation (net_key 0x2dd42); int aliases of char* globals stop the
  scheduler hoisting a pointer chain (data_load 0x28180). No alias needed for the same effect: write
  `(int *)charbase + i * 2` where the address is the same as `(int *)(charbase + i * 8)` - the cast
  around the whole sum keeps the sum's load late and re-pairs the index registers (track_around
  0x283a4, 31 rows -> MATCH). [generic]
- **Chained assignment computes through a scratch:** `k = g = x` loads into AL; `k = x; g = k;`
  loads into k's home (demo_play). `t = dx >> 1; err = t;` computes in EAX and moves, `err = dx >> 1`
  computes in place (kart_line 0x21de0) - which one the game used varies per function. [generic]
- **Loop shapes:** a tail-tested loop with its end from the variable, `end = p + N; do {...} while
  (p != end);` (track_load .SIN `lea edi,[esi+0x360]` 0x27d80); a guard clause `if (skip) { p++;
  continue; }` with the step at the body end keeps the game's tail order where every `for (...; p++,
  i++)` is re-sorted (anim_tick 0x261fc); a pointer re-loaded twice in a loop needs a second local
  re-armed at the body end (fn_1c994 0x1ca83); a retry loop is `while (1) { if (x != 0) {...return;}
  if (y != 0) break; }` (answer_call 0x1e0f4); 9.5b unrolls constant-count loops itself, write the
  plain `for` (crack_draw); a `while (i <= g)` keeps 9.5b's TOP test where the game has the test at
  the bottom re-reading the global - write `for (i = 1, x = 4; i <= g; i++)` (fn_173c4 0x175d9). [generic]
- **Control-flow shapes:** a cold `then` block after the chain = `if (!c) { big } else small;`
  (classes_menu 0x19f61); a 3-case `switch` on a short is a compare tree with cold blocks
  (laps_menu 0x1a148), not an if-chain; `if (c != 4) { ... } else x = 6;` shares a store block that
  an early `return` duplicates; repeated `store; ret` tails are one shared `store; return;`
  (shot_hit 0x218a4); a compare `movsx r,[m]; cmp r,N` is `k = m; if (k >= N)`, a direct `cmp word
  [m],N` is the field compared in place (ents_flags 0x213f0). [generic]
- **An asm routine's prototype comes from its own prologue, not from what callers load:** pcx_blit
  0x375cd is `pushal ... popal; ret` (0x376d5) and reads p7d714, so it is `void pcx_blit(void)` (all
  registers survive). The "EAX/EDX arguments, result in DX" were the caller's `p7d714 = p` value and a
  `short` save of page_draw held in DX across the call: `p7d714 = p; fs = page_draw; pcx_blit(); page_draw = fs;`
  (0x1ce57/0x1ce6c). Retired the old "16-bit EDX argument" dead end; credits_screen 44 -> 32. [generic]
- **A ternary as a call argument blocks 9.5b's strength reduction of the loop index**; the same
  call in an `if`/`else` lets it replace `k` by stepped pointers/offsets like the game
  (kartsets_load 0x28038, 48 -> 36). [generic]
- **Strings:** a byte-loop strcpy is `strcpy(buf, &"LIT"[0])` (keeps the literal in CONST; songs_pick
  0x32e60); `strcpy(p, "")` is a 1-byte copy of the pooled "" at 0x600ac (the NUL of "WACK").
  Watch check.sh's `segment CONST` line after every new string: one base = literals in place.
  [generic]
- **Check a permute.py winner's diff before believing its score:** until iter 56 two mutations broke
  the code silently - `comm` swapped `1 + X` in `- 1 + X` (a sign flip) and `bound` rewrote the
  `> 1)` of `>> 1)` into `>>= 2)` (a store into the field). kart_bump's "49 -> 42" came only from
  those two; the four safe mutations of that winner score 49. Both regexes are fixed; results logged
  before iter 56 that list `comm` or `bound` were scored on wrong code. [generic]
- **Score the control copy before trusting a variants.py probe:** copies of a function appended to
  the end of a probe file do not always compile like the original in place. race_records_menu
  0x1d13c: the unchanged copy scored 38 rows / 476 bytes appended, 12 / 512 in place (fn_1c0f4's
  control did reproduce). When the control differs, compile each variant in place in its own
  directory (parallel wcc.sh runs are fine). [generic]
- **A near miss can hide a behaviour bug the row count never shows:** a wrong field (cboxes[1].x2
  for .x), a swapped string, a store into the wrong table, or code made unreachable (no label after
  a `goto`, which 9.5b drops without a warning: fn_1832c lost its joystick path and trial_book
  call). `tools/wc/nmrefs.py` compares what both sides reference; run it after every change. One
  shape to watch: the game computes an expression over globals once and reuses that register on both
  sides of a store to one of them (`sub [spr_w],ax` then `mov [clipy],eax`, 0x34346/0x3435f) - the
  source had a local; writing the expression twice re-reads the stored value and changes behaviour
  (players_ahead's clipy was always 0, findings/video.md). [generic]
- **Read a nmrefs count line as a store census first:** count that global's reads and writes on both
  sides (decoded operands with an absolute address; the writes are the `mov`/`add`-style ones in the
  destination). Equal write counts and a game-side `add [g],imm` (0x264f8 `add [p7d838],4`), a frame
  slot (0x25694 `mov [esp],edx` for `sy`) or a folded base (0x2134f `movsx edx,[ebp+0x7299c]` =
  `attrs[i].w`) mean shape, not behaviour; iter 58's three passes found only that. What is real: a
  store that is missing, and a global left with a different value at the return - xref the global and
  check who reads it without writing (shots_project's v7d874: the game stores the past-the-table
  pointer 0x26697, and it is dead because every reader sits under a writer, findings/game-loop.md).
  Two shapes that end the hunt: C's `g += x` is ONE address-bearing instruction (`add [p7d838],eax`
  0x2692e) where a rebuild that keeps the value in a register has load+store (lights_project 0x266a8,
  ours x5 vs game x4), and the game skips a flag test Watcom can prove from an earlier `je`
  (race_results 0x2bff0: three source ifs, two reads, the scores_screen arm tests only its other operand
  0x2d218, with a dead `je` 0x2d1f7 left behind) - deleting your own test to match the count changes
  behaviour. Pair the sites in instruction order and compare where each failing test jumps. [generic]
- **A one-sided nmrefs jump-target block is three SAME classes before it is a bug:** (1) the game
  tail-merges a store pair, so the block an arm jumps into uses only the second half (shot_step
  0x25ff1 stores x then `jmp 0x26021` 0x26008 into the shared y store, where our `return` jumps to
  the epilogue and our tail block re-reads the struct); (2) the game skips a block it can prove dead
  from the condition that got it there (ents_show 0x145c3 `jmp 0x146bd` = the loop tail, leaving both
  `sel == -1 && ...` blocks 0x14623/0x1468d, reached only from inside `if (sel != -1)`); (3) the game
  keeps ONE head for nested retry loops, so its retry jumps re-test an outer condition our build
  tests once (fn_1be3c 0x1beae: the rand retries 0x1bec2/0x1bec8 and the duplicate-found `je 0x1beae`
  all land on the `splitscreen` test, ours land on the call 0x1beb8 or on the bottom test 0x1bef5).
  In all three compare reachability and the values at the stores, not the jump targets. [generic]
- **An unsigned `cmp eax,1; jbe A` + `cmp eax,2; je B` + `jmp` chain is a `switch` with `case 0: case
  1:`**, not `if (v <= 1) ... else if (v == 2)` (signed `jg`, other block order). A block the game
  jumps BACK into from a later test is the same statements written twice in the source and
  cross-jumped: `if (netgame == 0) { A } else switch (v) { case 0: case 1: A; break; case 2: B; }`
  (fn_1c0f4 0x1c538-0x1c58b, 67 -> 14). [generic]
- **A row loop over `char rows[N][K]` from 1: an explicit `char *r` declared last and stepped in the
  for header** (`for (i = 1, r = rows[1]; i <= n; i++, r += K)`) took volume_menu 0x1e9cc 64 -> 48
  and fixed the scalar slots after it; declared first or set in the body, no gain. [generic]
- **Two local arrays in each other's slots: a pointer to the one the game puts at [esp], initialised
  at entry.** fn_17928's `char *nb = numbuf;` (declared last, used only in the itoa call) moved
  numbuf to [esp] and msg to [esp+0x50] like the game: 12 `lea edi,[esp+0x50]` rows gone for one
  `mov ecx,esp`. Declaration order, `(void)numbuf`, a block-local array or a late `nb = numbuf` did
  nothing; using nb everywhere cost an EBP frame. [generic]
- **A permuted group of scalar slots: one `volatile` array of those values** (wtext 0x146d4, 6 ->
  MATCH; a plain `int sl[4]` or struct with constant indices is scalarized and its slots scramble,
  6 -> 21). Only works when they are the function's ONLY spilled locals - with other scalar slots
  around, the elements are still coloured individually (screenshot 0x114c8, release_check 0x034458:
  no gain). A slot holding a *parameter* spill counts as one of those other slots: ents_show
  0x14380 has four slots, three permuted locals and one incoming argument, and `volatile int sl[3]`
  for the three is allocated above all four, not at [esp] - frame 848 -> 860 bytes, 7 -> 20 rows
  (iter 103, both declaration orders, both index orders). [generic]

### Dead ends: SKIP at once (0 variants) unless you bring a new idea

- **Stack-slot permutation of scalar locals** (QUESTIONS 9, 10), unless one of the swapped slots
  is a compiler-made pointer (lever above) or the group is written as one `volatile` array (Levers,
  wtext 0x146d4): neither declaration nor first-use
  order (arrays DO follow declaration order, ascending from esp+0). Exceptions, iter 56: release_check 0x34458
  went 7 -> MATCH with the loop step written `i = 1 + i` instead of `i++`; in kart_bump (11 shorts,
  8 spilled) a random declaration order from permute.py's `declorder` mutation took 49 -> 36 rows -
  it moved register homes as much as slots. Hand-picked orders do not find this; let the permuter
  shuffle the declarations. 43 orders in enter_name, also
  release_check, wtext, scores_screen, fn_1c0f4, showlines, cars_menu. In a function that already
  has 20+ scalar slots the pointer lever backfires - it renumbers every slot (trial_book 0x2e4b8,
  four spellings: probe 155 -> 256). Size a local array from the
  frame, not its largest index (scores_screen `long sc[10]`); never-used locals are dropped, so a
  frame gap means the game used those slots - but a gap BETWEEN two arrays is the earlier array being
  bigger than the code writes (fn_193a8: `char name[0x28]`, 12 bytes of a 40-byte buffer used, frame
  0x310 -> the game's 0x32c). The reverse too: an over-sized buffer inflates the frame
  and every slot displacement (`char buf[8]` gave time_str 0x2a61d a 0x14 frame where the game has
  0x10; `buf[4]` fixed it, 33 -> 31 rows). [generic]
- **A global kept in a register across a loop** (load at entry, store before each call and at exit)
  is NOT a dead end: the game's source used the global itself as the working variable. Drop the
  local copy and compute in the global (`spr_x = p->x - 0x400; if (spr_x < 0) spr_x = 0; ...`):
  minimap 0x2a4ac 44 -> MATCH, fn_16eac (`n = nplayers` copy removed) 11 -> MATCH. Signature to
  scan for: `mov [G],r ... call; mov r,[G]` in the game - the store can sit up to a dozen
  instructions before the call (shot_fire 0x2d5ad: v7d874 walks the shot table, 57 -> MATCH;
  fn_16d68: `v7d7f8 = &trklines[k]`, stored once at the exit, 58 -> 26). Mirror image: if the game ALSO
  reloads the global after its calls (`mov r,[G]` 0x26403/0x26491/0x2667e), the source used a local
  pointer plus `G = p;` in the body - shots_project 0x2632c 155 -> 117 that way (open in
  players_project/ents_project). A global that is already
  the working variable and still is not cached (players_ahead's v7d800) is unexplained. The lever
  also fails when the cached global is a struct pointer used hundreds of times: kart_frame 0x290a0
  with `k = curkart` at entry, `k->` at all 139 sites and `curkart = k; k = curkart;` around every
  call and return scored 535 -> 533, and the game still had 19 more 0x7d5b0 references. It works for
  a global only READ, and 9.5b does the promoting itself: fn_173c4's `short mp` local (myplayer) had
  to go to get the load-at-entry/reload-after-call/store-at-exit trio (0x173ca/0x17405/0x17855) - a
  local also lets 9.5b CSE every derived `index * stride` address out of the loops, which is how it
  hid 3 array references and 6 jump blocks from nmrefs there (0x173c4, 189 -> 100). [generic]
- **Which of two locals gets EAX, or which of two pushed registers** (QUESTIONS 1/2, the "coin
  flip"): fn_19db8, pcx_load ESI/EDI, fade_in, netkeys_out/pack/in (c in DL), net_recv crc,
  com_wait_byte, com_expect, self_project, drawsprite_clip, engines_menu (the int that reaches a shared
  tail: game EDX, ours [esp+8]; `short` wins EDX but every op turns 16-bit). Check the width rule and
  the levers above first. 9.5b has NO explicit register variables (`@ edx` is E1009 in a function,
  E1059 at file scope), and declaration order of globals or comma-`for` initializers is not a lever.
  A loop-invariant literal argument is the one exception that moved a home (fn_16f24, 39 -> 26).
  Before calling it a flip, ask whether the original had the local at all: netkeys_out/pack/in
  (DL) were the compiler's own temp for a global the source read or built in place (`v88cf0 = 0;
  if (..) v88cf0 += 2;`, `x = v88cef & 1;` each line), iter 93s: 3 matches. Same for a global that is
  a POINTER walked in place: shots_clear 0x2d4fc 11 -> 0 with `v7d874 = shots; ... (v7d874++)->f04 = 0;`
  and no `struct shot *p` - the local is what handed EBX to the loop counter (iter 94). The *form of
  the use* moves it too: `vec_x = v7d690[0]; vec_y = v7d690[1]; v7d690++;` gives shots_move 0x2602c
  the game's EBX home for the walked pointer where `*v7d690++` had it in EDX (12 -> 7 rows, iter 94),
  and two CONSTANT pseudos are a genuine tie - voices_reset 0x11b1e keeps -1 in EBX whichever store is
  written first (iter 94, 7 spellings). The allocator traces
  in findings/compiler.md show a real tie: equal savings, and the order is the unstable Shell
  sort's over creation order (`tools/wcc95/shellsort.py`). Two more levers are now closed (iter 95,
  `tools/wcc95/vbatch.py`): the operand order and parentheses of a flat sum never reach the codegen
  (self_project 0x26a3c: `(spr_w - 1) + spr_y`, `spr_w - 1 + spr_y` and `spr_y + (spr_w - 1)` compile
  byte-identical, 3 rows each), and a home the game got from an incremented index register is not
  spelled back into it (shots_move 0x2602c: the game stores the walked global twice 0x26130/0x2613b and
  reads the second element through the incremented pointer 0x26141; all eight such spellings cost 6
  rows more than the indexed pair). Caching a global in a local also has to be free: drawsprite_clip
  0x11404 `short sy = spr_y` puts back the load the game keeps (`add ax,[spr_y]` 0x11437), 3 -> 11.
  Deleting the local is the one compile that says which case you are in: the row survives the delete
  only when the register is 9.5b's own temp (com_wait_byte 0x2df70 - `if (ticks - t0 >= 0x110)` with
  no `d` anywhere still `mov eax,[0x7e720]`/`sub eax,edx`/`cmp eax,0x110`, iter 96), while a real home
  shows up loudly (fn_1bf00 0x1bf00 - `players[i].face = v7ec6c;` with no `val` adds a fifth `push`,
  128 -> 136 bytes, 9 -> 15 rows, iter 96). And `t = g & m` is one fused load+mask: splitting it
  (`b = g; b &= m`) makes every copy live, +20 bytes and 8 -> 31 rows (net_input 0x2f4a8, iter 96).
  Two more whole-file levers (iter 97): the *form of the loop* picks which counter gets the first
  pushed register - fade_in 0x32cf8 `for (i = 0, k = 0; i <= 0x2ff; i++)` keeps i in EBX, while
  `k = 0; i = 0; do { ... i++; } while (i <= 0x2ff);` gives the game's i EDX / k EBX, 13 -> 0 rows,
  and writing `k = 0;` before `i = 0;` is what fixes the order of the two zeroing `xor`s (0x32d1c);
  and merging a loop's value local with the function's return code flips the homes of the OTHER
  locals (com_expect 0x2e008: one `c` for the com_getc result and the return code, no `r`, moves
  want ESI -> EBX (0x2e00b `mov ebx,eax`) and t0 EBX -> ESI, 8 -> 4 rows). And a local that only
  *pays for itself through the scheduler* is still the game's: line_draw 0x21f04 needs a second
  rotation temp (`u` for the sx/sy swap next to `t` for dx/dy) before 9.5b lifts the `movsx edx,cx`
  above the rotation moves (0x21fad) and reads the register the game reads - with one shared temp the
  same source order sinks it below them (0x21fb3, `movsx edx,bx`), 2 -> 0 rows (iter 100). [generic]
- **Scheduling the game does differently in one straight-line run:** a zero argument constant -oe
  sinks below inlined stores (music_jingle, duck_screen, race_results, trial_book: `xor edx,edx`;
  iter 100: `song && 0` and `2 - 2` fold to the same sunk xor, so a non-constant spelling is not a
  trigger either),
  one swapped pair of loads (track_setup 0x124e8; iter 101 vbatch, 9 spellings: declaration order flips
  which of EBX/EDX is the home, nothing moves the order of the three loads), a conversion the game
  hoists above a store (shot_hit 0x21be1 `movsx edx,cx`; iter 101: a shared `struct kart *k = &karts[i]`
  costs 50+ rows because the game recomputes the index at 0x21c16, and an `int` local for the index
  changes nothing - no local owns that `movsx`), a store 9.5b hoists or sinks out of a group
  (classes_menu 0x1a05d, QUESTIONS 3), two independent load chains interleaved (in_sector 0x24a08; a
  statement written below a branch sinks past the whole branch), a dead copy of an incoming register
  (fn_182d0 `mov esi,ebx`; iter 101, 9 more whole-file spellings - `lim = a3; lim *= 3;`,
  `lim = lim * 3;`, `(short)(a3 * 3)`, `long t = a3; short lim = t * 3;`, `register`, three declaration
  orders - all fold it: 9.5b re-materialises `imul r,r,imm` from the argument register, so a dead copy
  in the game's bytes is a def for an unmodelled local, not a spelling, QUESTIONS 28), an allocator spill `mov [esp],eax; mov edx,[esp]` with no room for a
  local (view_at_curpos), a param a copy into a local will not spill the way the game does (its
  mirror image: the incoming value lands in the new slot at the prologue and the register is
  reloaded after the call - fn_1d704 0x1d7b5, 11 spellings). `tools/wc/permute.py` does not move
  these pairs either (shot_step 0x25f46, track_setup 0x124f6: 270 copies over 9 rounds, best =
  control every round). `-3` vs `-3r` is not the cause
  (identical object). [generic]
- **Not a dead end: `mov r,N*stride; add r,sym` is an INDEX loop.** 9.5b folds a pointer's
  `&netstats[1]` (or `netstats + 1`, `(char *)netstats + 0x3c`) into one immediate; only
  `for (i = 1; ...; i++) f(netstats[i])` makes the strength-reduced pointer start as
  `mov r,0x3c; add r,netstats` (iter 73 probe, 8 spellings; showlines 18 -> 13 rows and the game's
  size). Also: a base one row earlier names an array base - declare it (`mrows` 0x7bc00,
  trial_records_menu). [generic]
- **Store->load value forwarding:** `v->f0a = d; x = v->f0a;` compiles to a register copy before
  the store, no reload; a `(volatile short *)` cast on the read does not force one (9.5b forwards
  anyway). The game's `mov word [ebx+0xa],dx; mov cx,[ebx+0xa]` (kart_catchup 0x28bf8) is not
  reachable; a partial store breaks the form. SKIP. [generic]
- **Extra bytes from a larger frame for one more local** cost spills and re-colour unrelated temps
  far away (kart_draw 17 -> 28; track_load's v88b88 load): score the whole function. [generic]
- **The game's single `ret` is not a target to reach:** merging our returns into one tail (`goto`, one
  `return`) so the epilogue count matches costs rows - menu 0x155c0 has one epilogue at 0x163df and our
  five duplicates align with its per-site `jmp` tail better merged (233 -> 261). The game's C may well
  have had one exit; the row count does not care. [generic]
- **There is no per-function optimisation control in WCC386:** `#pragma loopopt` is a WPP pragma
  and the pipeline (wcc386 on .C, no preprocessor) silently ignores it - `#pragma loopopt OFF`
  on fn_16f90 changed nothing (0x16f90, the loop-strength-reduction dead-end class). [generic]

### General rebuild facts

- **Equal instruction count, frame and spill-slot count with different homes = register tie, not
  spelling** (draw_view 0x11526, 51 rows, 2026-11-25: 190 in-place spellings - loop/clip/increment
  mutations, 72 declaration shuffles, pointer locals, pragma forms - never move ESI from `px` to
  `col`; only the C *shape* the compiler saw would). Do not spend a second session on such a line.
  [generic]
- **Open Watcom is no model of 9.5b's register choices** (iter 93q): of the 205 functions 9.5b
  builds byte-identical, WC 10.0 prerelease gives 99, 10.0a 74, Open Watcom 2.0 19 (median 23 rows
  off; native `.work/toolchains/ow2`, 9.5b headers lower-cased in `.work/toolchains/h95b`). Use
  its source for the rules below, not for tie-breaks. [generic]
- **9.5b frame and evaluation-order rules, measured (iter 73 probes, Open Watcom cg source as the
  map: `temps.c` AssignOtherLocals, `x86temps.c` TempAllocBefore, `dataflo.c` RoughSortTemps):**
  (a) address-taken locals and arrays are sorted by SIZE, smallest highest in the frame (char, short,
  int, arrays), ties in declaration order with the first declared highest; first use does not
  matter. So `short` vs `int` moves a local across the whole frame. (b) Spilled scalars in a clean
  probe (10 ints live across calls, straight-line or one loop): the most-used get registers in the
  order EDX, EBX, ECX, ESI, EDI/EBP, the spilled ones sit by use count, more uses = higher slot, and
  declaration order, assignment order and the names change nothing. A use inside a loop outweighs
  16 uses outside it. BUT real functions break (b): credits_screen's `dir` (6 references, inner-loop
  ones) sits below `first`/`y` (3 each) on both sides. The mechanism (OW `generate.c` Generate:
  MakeConflicts -> SplitVars -> MakeConflicts -> RegAlloc -> AssignTemps): slots go out top-down in
  the temp list's order after a size sort, and that order is the savings sort (`RoughSortTemps`,
  weighted IR reference counts, unstable ShellSort) of the LAST MakeConflicts plus temps created
  after it (SplitVars gives every later disconnected live range of a reused variable a new temp).
  IR counts are not C counts, and ties are reordered by the ShellSort's gaps, so the result is
  chaotic: in a credits_screen probe one extra read of `first` moved `page` to the top slot, extra
  reads of `y` moved `first` to the bottom (iter 73, 6 probes). Declaration order acts only through
  ties - which is why a shuffle (`declorder`) sometimes wins and hand-picked orders do not. Not
  solvable by rules; it needs a simulation of the compiler, or search. (c) Of two operands the LARGER subtree is
  evaluated first whatever the written order (`g(a) + g(b) * c` == `g(b) * c + g(a)`); written
  order decides only between equal subtrees, and no-op wrappers (`(int)`, `+x`, `| 0`, `-(-x)`) are
  folded, so they cannot steer it: a game that evaluates the smaller side first had a different
  tree (grouping, or a temp in its own statement). permute.py's `comm` can only help on equal
  subtrees. [generic]
- **Compare the data, not only the code, and expect dead bytes**: `tools/wc/datamatch.py ww
  .work/src/WW.OBJ` (iter 68) puts our build's data groups on the game's image. Two traps, both found
  getting a clean run: (a) the group's base must be voted over **every** occurrence of each anchor -
  counting only the first hit of each anchor put the CONST group 4 bytes early and made 8225 of its 9046
  bytes look wrong; (b) a differing byte is behaviour only if a dword somewhere in the image holds
  its address, because the linker shares pooled literals and the game's pool keeps dead text where our
  build has NUL padding (findings/executables.md "Data layout (obj2)"). `tools/wc/test_datamatch.py`
  pins both, with a flipped literal byte as the mutation. [generic]
- **`src/check.sh` after a compile error scores the PREVIOUS object** unless it deletes it first
  (fixed iter 36: `rm -f .work/src/WW.OBJ`); grep its output for `Error!` before reading a count.
  [generic]
- **`tools/le/xref.py` already prints the callers** of an address: capstone shows a `call rel32`
  with its resolved absolute target, so the operand scan catches it (`xref.py ww 0x357ac` ->
  `call 0x357ac` at 0x1691d). Writing an `0xE8` sweep script is redundant; `.work` holds four.
  [generic]
- **A block's fields need a range census, not an xref of its base**: Watcom reads a struct/array
  element through its own absolute address (`cfg7ee1c + 0x3e` is `word [0x7ee5a]` = `v7ee5a`), so
  `xref.py ww 0x7ee1c` returns only the whole-block transfers; `tools/le/rangexref.py ww 0x7ee1c
  0x7f2ae` -> 17 addresses, which is what proves the `WACKY.ING` record area is unread. The same trap
  in the listing: when the base is held in a register capstone prints the field reads with the base
  folded into the displacement (`movsx edx, word ptr [ebp + 0x7299c]` = `sprattr[0].w`), so grepping
  the disassembly for the name finds only the 9 base loads - census the fields by offset
  (`grep -oE '\[[a-z0-9]+ \+ 0x729..\]'` gave the whole SPRITE.ATR read set, 0x72998/9c/9e/a4/a6/bc).
  [generic]
- Structs are pack(1) (`imul 0x6a` where the game has `0x68` = natural packing). [generic]
- **A function's entry block can land far from its body.** When function A ends in a tail call to
  B and B is not compiled yet, 9.5b emits B's entry block right after A (the `jmp` to the next
  instruction is dropped) and B's remaining blocks where B is defined. fade_out_screen 0x1bf80 falls
  into fade_out's head at 0x1bf94; fade_out is defined after pic_show, so its loop tail sits at
  0x32cc1 straight after pic_show's `ret` (iter 56c: a "gap" plus a "padding diff" were one
  function). Signs: a function with no prologue that falls into the next, and a block right after
  another function's `ret` with no alignment and no entry of its own. [generic]
- A switch's jump table sits in the gap BEFORE the function (0x1a360 in cars_menu), table order is
  source-label order, and case bodies are emitted in source order: sort the cases by jump-table
  target before writing the switch (main_menu case 7 before case 6; 0x19da8 for fn_19db8).
  [generic]
- A Watcom switch table may be indexed by the switch value **minus one**: `mov bx,[obj+0x18]` +
  `dec ebx` + `cmp bx,0xd; ja default` (kart_frame 0x298cf, table 0x29068 - entry 0 is `case 1`,
  entry 13 is `case 14`). Read the index computation before mapping entries to cases: otherwise a
  nmrefs gap like `game more 4x <table>` / `ours more 1x <other table>` reads as the wrong table in
  an arm, when it is plain block merging (kart_frame's 9 ANIM_RESET base loads vs our 5). [generic]
- Read esp offsets per instruction: a 5-argument call pushes one dword, so displacements after a
  `push` are 4 lower (cars_menu 0x1adf7). [generic]
- A call target can be an interior label of a library module (0x430f7 inside `__stos` 0x430be):
  declare it as its own extern with the call site's registers. [generic]
- A short parameter whose callers do not sign-extend: the callers had no prototype - declare
  `void f();` and define with `short` (fn_1d704). Call-site argument count must match the
  definition (E1151): pass a callee-ignored extra argument through an aux-pragma'd parameter list
  (voice_play 0x18da1). A stack slot incremented but never read is a parameter used as the counter
  (`first++`, credits_page 0x1eee4). [generic]
- `#pragma aux f value [dx]` is accepted where `value [edx]` is E1121; the prologue's `push ebx`
  means a value lived in EBX, not an ABI rule (ctl_label). [generic]
- Pinned -o letters from single-function probes can be wrong for the whole program: re-check every
  flag against the full MATCH count (com_open needs `e`). [generic]
- The gdb stub refuses every breakpoint above linear 0xFFFFF (`Z0` -> `E01`, dos-re-suite's
  `gdb_addr_to_seg_off`): a DOS/4GW program's code (here 0x1e2000+) cannot be broken on at all.
  Halt + read registers works: sample EIP/SS in a loop instead (SS tells the program from an ISR
  stack). [generic]
- After `load` of a saved state, QMP key taps did not reach the game (it stayed in `menu_`): for a
  key-driven run boot fresh and press the keys. A sampler that halts every 30 ms also swallows taps:
  press first, then sample. [generic]
- WLINK 9.5b page-faults (`CS:EIP = 000Ch:0001FFD5h`, stock DOSBox then aborts) on a hand-written OMF
  object that has fixups but lacks WCC386's two COMENT records (class 0xAA `OS220`, 0x9B `3fOp`);
  OMF segment indices are 1-based (index 0 crashes it too). With `option quiet` nothing is printed:
  read the map. [generic]
- A game global written as a number in C (`strcpy((char *)0x80ace, ...)`) matches byte for byte,
  because the game's unrelocated bytes hold the same constant, but it has no fixup: the linked program
  writes to linear 0x80ace, not to the relocated variable (DOS/4GW loads the data object elsewhere).
  It hung the hybrid's race load (fn_17928, music ISR starved). Use an extern symbol (`litNNNNN`,
  src/ww.c line 18). [generic]
- Wrong screen block but the reader, the filler and the index writers are all byte-identical? Bisect
  the *state*: `ORIG=<fn> tools/link/racetest.sh` builds that C function from the game's bytes; census
  the block's pixels before and after. Proving which bytes were read (decode the data file) beats
  reading more code. `[generic]` (iter 72)
- `tools/le/disasm.py` stops at the first undecodable byte, so a sweep of the whole code object is
  silently truncated and a grep for "this address is never stored" may only have seen the first part.
  Use `tools/le/gd.py <prog> <from> <to>` (same annotation, skips a bad byte and resumes). `[generic]`
- Two structurally-identical compound assignments to the same struct field (`stats[...].damage -= 10`)
  in one function: Watcom's allocator flips the FIRST one's load register (EAX -> EDX plus a copy);
  a single such statement compiles in the natural form. A structurally different second statement
  (assign form) keeps the first natural but changes its own codegen (load-sub-store, not RMW
  `sub mem,imm`); pointer spellings fold back to the same tree (dropped2_tick 0x178f6, probes in
  .work/probe/S.C). [generic]
- A switch case whose test is the literal De Morgan complement of another case, with the bodies
  swapped, is merged by Watcom into ONE shared test block (both dispatch jumps target it): case 15
  `d <= 0 || f <= 0` + case 16 `d > 0 && f > 0` in cell_effect 0x18b8d collapsed 43 bytes. If the
  game keeps two test blocks, no `>` spelling reproduces it; a non-normalised comparison (`>= 1`,
  cmp imm 1) keeps them apart. [generic]
