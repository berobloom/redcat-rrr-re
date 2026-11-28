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
| 0x1B916-0x4142F | library modules in the order WLINK pulled them in: Watcom's clib and math library mixed with Davilex's own libraries (`C:\PROTMOD\WINLIB\mgl2\fg2`, `C:\PROTMOD\SOUNDLIB\wc32\mod`), Fastgraph V4.03 and the Smacker player; owner of every range in `reverse-engineering/analysis/rrr_libmap.tsv` (`tools/libmap.py`) | Watcom's libraries as they are; Davilex's C to rebuild; Fastgraph and Smacker from their LIBs (not in hand) |

The game's own C ends where the first library module starts (clib `sprintf` at 0x1B916, then
`fopen` at 0x1B959): the functions before it carry RRR's own source files' names, the ones after it
the libraries' (`rrr_funcs.tsv`). STRONGLY SUPPORTED. Its `sprintf` module is byte-identical to
**10.6**'s CLIB3R and one byte off 10.5's (`89 d3` mov ebx,edx where 10.5 has `88 d3` mov bl,dl) -
the fingerprint that names the compiler version (open question 24, discoveries "Executables").

REKEN, RRRMENU and INTRO have the same structure and come later (REKEN's appended debug info names
its modules, `research/discoveries.md`).

## 1. The toolchain

- **Compiler: Watcom C/C++ 10.6** (was recorded as 10.5). CONFIRMED by the library fingerprint: 10.6's
  CLIB3R.LIB matches RRR in 118 modules byte for byte, 10.5's in 108, 10.0a's in 42 (`libmatch.py`,
  discoveries "Executables"). It makes no difference to `src/`: 10.5 and 10.6 emit identical segments
  for every module, so `src/check.sh` counts the same either way. `reverse-engineering/watcom.md`
  installs both; `WC=.work/watcom/dos/C106/WATCOM src/check.sh` picks 10.6.
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
  fixable in C>`, `ASM: <why the game's function is hand-written assembly, so check.sh never compares
  it and nearmiss.sh never lists it>` (that line is written by hand).
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

**Data.** After the functions, each object's data segments (today only `CONST`: every global is
`extern` in `src/rrr.h`, so no `_DATA` is defined yet) are compared with the game's data object at
the base their references give, fixup bytes masked: `data CONST ... IDENTICAL at 0x...` or `differs
at ...: K of N agree, first at +D` with both sides shown. A segment counts only when all of it is
identical; `check.sh`'s last line is `DATA: N of M data-segment bytes identical in place`, and
`src/data.baseline` holds N. Like the MATCH count it only goes up, raised in the commit that raises
it. A string block that differs at +0 usually means a missing or extra literal (hiscore.c's block
starts with `" error\n"` in the game; init.c's has two bytes `"@"` ahead of ours).

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
   function in one compile; compile the winner in place before believing it. If the only rows are
   stack displacements (two `[esp+...]` numbers differ), stop permuting spellings and compare frames:
   `reverse-engineering/tools/wc/stackmap.py .work/src/<MODULE>.OBJ rrr NAME` prints the slots a
   compiled function touches and the holes it leaves (see the frame-layout lever below).
5. **Write down what you learned.** A near miss keeps a `/* differs: ... */` note above it in
   `src/rrr.c` with the causes and the spellings tried; the `nearmiss.txt` line gets the count and
   the status. Commit after each function, MATCH or SKIP.

Levers seen so far (add to this list when one produces a MATCH):

- **Frame layout: put the value in an object, not a local.** WCC lays the frame out as a band of
  4-byte scalars at the top, then a gap (its FP temporaries / alignment), then what it will not band -
  local arrays and 8-byte objects - at the next 8-byte-aligned offset; unused locals, dead stores,
  propagated constants and `register` cost no frame space. Declaration order only picks which scalar
  gets which band slot. cos_sin_init 0x13c34 stayed at 2 rows through 156 spellings of a local `x`
  (always [esp+10]) and matched as `int v[3]` with the value in `v[2]` (the game's 12-byte object at
  [esp+0..0xB]), with the angle expression written twice so WCC CSEs it and no `double t` is needed.
  Check the shape with `tools/wc/stackmap.py`; found by sweeping 216 array-element x declaration-order
  spellings (`tools/wc/sweeps/cos_sin_init_frames.py`, table in `reverse-engineering/analysis/
cos_sin_init_frames.txt`, 2026-11-27).

- **Loop shape.** `-ox` includes loop rotation: a `for` loop with the test at the bottom of the
  generated code becomes a test at the top only when written as `while (cond && ...)`
  (`stage_is_last`).
- **Chained assignments.** `a = b = 4000;` and `b = 4000; a = b;` compile differently
  (`clear_objects`).
- **Two-value switches nest.** `switch (finish_order[0])` with a `switch (finish_order[1])` in each
  case gives the game's `<2 / ==2 / ==4` compare tree exactly (`podium`); an if chain gives 40 rows
  and `else if` 27 (variants.py, 2026-11-23).
- **Write the `default:` the game jumps to.** A ten-arm jump table whose empty arms (kinds 11, 14,
  17) all go to the switch exit means an explicit `default: break;`: without it Watcom places the
  merged `check_object_nr`+store+loop-increment block after the last case body, with it after the
  first, which is where the game has it (`find_bonus_objects` 32 rows -> 1, variants.py 2026-11-25).
  Case bodies are emitted in source order, so a jump table also fixes the order to write them in.
- **A function's neighbours decide its tail.** Watcom merges an epilogue into the previous function
  when they agree; if the function before us is wrong our shared tail lands elsewhere, so the last
  jmp of an otherwise perfect function can wait for its predecessor (`find_bonus_objects` waits for
  `find_waypoints`, like `mine_tick` waits for `dropped2_tick`).
- **Two switch cases with the same code: write the text twice, identically.** Duplicating case 15's
  test and bodies in case 16 made Watcom re-emit the two compares and jump into case 15's shared
  blocks, which is exactly what the game does at 0x19123 (`cell_effect` 17 rows -> MATCH). Any other
  spelling of the same behaviour (`damage >= 1 && fuel >= 1`, `> 0 &&`, `!(<= 0 ||)`, nested ifs,
  early break/return) folds or duplicates the blocks instead - `>= 1 &&` costs 87 rows.
- **Negate the tests, not the arms, to keep two identical store blocks apart.** A nested if/else over
   four cases merges them: `if (finished2) {...} else if (finished4) {...} else {...}` and the
   positive-test nested forms all fold the two `finish_order[1] = 2;` stores into one block (794 of
   the game's 806 bytes, 11-14 rows). Writing both tests negated - `if (!finished2)` and `if
   (!finished4)` inside each arm - keeps them separate the way the game has them, because one of them
   jumps to the shared `finish_order[2] = 4` and the other falls into it (`main` 0x111e8 6 rows ->
   MATCH; removing any one of the three `!` costs a row, 72 spellings in .work/probe/mn).
- **Watcom shares an epilogue only inside one object.** When the game gives a function its own
  `pop`/`ret` and ours jmps into an earlier function's, the two are in different objects: move the
  function and everything the jmp crosses into a new `src/` file and re-check. `racer_contact` (5 rows
  left) and `racer_bounce` (0 rows, tail jmp only) both went to MATCH with no C change at all that way
  (`src/racer.c`, 0x1404a-0x1467b: racer_bounce jmps into racer_contact's epilogue, so those four are
  one module). The game shares an epilogue only 6 times in 0x10010-0x1B915, our one-object `rrr.c`
  dozens of times - which is how the mismatch is spotted (discoveries.md "The game's own C is many
  objects"). `follower_tick` 0x167aa is the other direction: the game gives it its own epilogue, so it
  now sits alone in `src/shadeobj.c` and the callbacks moved to `src/shadeobj2.c` (13 -> 9 rows);
  there the string blocks say the same thing (one padding byte, 0x51783, between the two modules'
  strings) - take the boundary when both tests agree. Read the two epilogues before using this test in
  that direction: WCC shares only *identical* ones, so a function that ends in its own `pop`/ret next
  to a neighbour whose epilogue differs anyway (`add esp` count, the return-value instruction) proves
  nothing. The three normal ticks are the case: 0x1281a `xor eax,eax; add esp,0x10`, 0x130ec `xor
  eax,ebx; add esp,0x10`, 0x138e1 `xor eax,eax; add esp,0xc` - their split into src/drive.c, src/bull.c
  and src/muis.c is worth 7 rows and 6 bytes, but the module question stays open (open question 27).
- **Negate the whole test to choose the fall-through arm.** Two conditions joined by `||` are not the
  same text as the negated pair: the game's `jle` into the `frame = 32` store with the `heading` store
  forward means it wrote `if (!(damage > 0 && fuel > 0)) frame = 32; else frame = heading & 31;`.
  `damage <= 0 || fuel <= 0` costs 30 rows and swapping the arms 20 (`follower_tick` 41 -> 13). Same
  family as the duplicated switch case above: it is the source text, not the meaning, that picks the
  block layout.
- **Operand order picks the SIB pair in `lea`.** `map[(x + y * map_w) * 6 + 4]` gives
  `lea eax, [edi + ebx]` (x as base) and `(y * map_w + x)` gives `[ebx + edi]`; same semantics,
  one row (last row of `cell_effect`). Where Watcom uses a plain `add` the order is invisible.
- **Narrow type, no temp for a subexpression.** `find_waypoints` 0x1517a sat at 190 rows through
  14 declaration orders and five probe files; `int w` + `int n` costs four 32-bit live values, a
  spill of the shifted x/y to `[esp]`, `n*8` kept instead of a scale-8 SIB, and `-1` compared as an
  immediate in the hole-check loop instead of hoisted into EBX. Writing `unsigned short w` and no
  `n` at all - `(w & 0xff)` wherever the number is used - gives the game's allocation exactly: the
  value stays in BX (load `mov bx, word [...]`, mask `xor bh, bh`), its int copy in ESI indexes
  with the SIB, the shifted value in EBP, no stack slot: **0 instruction rows** (variants.py,
  2026-11-24; `.work/probe/FW2VAR.C` V04, `unsigned short` beats `short` 7 to 10 rows and inline
  `(w & 0xff)` beats an `n` local by ~140). The 16-bit holder is what frees the register.
- **Where a store sits in a long store sequence decides whether Watcom re-reads it.** `init_objects`
  0x148db stored the twelve player/racer fields in the order its disassembly reads; writing
  `stats[bodies[0].stats].damage = rrrio.damage;` after `fuel` instead of last (its own store is
  then not the last one) makes WCC386 emit the game's `cmp dword [stats+0x28],0` reload in the
  crash0 test instead of `test edx,edx`, and with it the game's exact 1635-byte length: 16 rows ->
  7, and the two following racers' heading stores came right with it (one of the two).
- **The comparison's sign is C-level, and it shows in the opcode.** An `unsigned` local makes WCC386
  emit `test edx,edx` + `jbe`/`ja` where an `int` of the same value gives `jle`/`jg` (0x76/0x7F vs
  0x7E/0x7F): `muis_crash_tick` 0x1a42a's f24 and `want` tests go from four rows of wrong jump
  opcodes to identical when the two locals are declared `unsigned`. So a `jbe`/`ja` after a `test`
  in the game is evidence that the original variable was unsigned, and `stats[].f24` is LIKELY too.
- **Which arm you write first picks which body Watcom puts after the test chain.** For three tests
  of one field (`f == 0`, `f == 3`, anything else) `muis_crash_tick` 0x1a42a emits the `f == 0`
  body inline right after `test edx,edx; jbe +7; cmp edx,3; je B; jmp END`. Written as
  `if (f > 0) { if (f == 3) B } else A` WCC386 puts `B` inline and `A` at the end; written
  `if (f <= 0) A; else if (f == 3) B` it puts `A` inline and `B` at the end, which is the game's
  order: 78 rows -> 70. `f <= 0` and `!(f > 0)` score the same.
- **A jump table's pad belongs to the object, not the function.** Watcom 4-aligns a switch's jump
  table inside its own object, so the pad before it (a 2-byte `mov eax,eax` or a 3-byte
  `lea eax,[eax]`) depends on the size mod 4 of everything the object holds before it. That is the
  whole remaining diff of `find_waypoints`: +0..+1009 identical, then the game pads
  `find_bonus_objects`' ten-entry table with three bytes and we with two (LAYOUT in
  `src/nearmiss.txt`). Adding a function to the module can fix or break it - `draw_hud_counts` (20 B,
  0 mod 4) changes nothing, `init_objects` 0x148db (1635 B, 3 mod 4) would land the table right.
- **A value used twice is not automatically a local.** `ground_check` 0x1889d computed its x and y
  cell numbers into `ox`/`oy` locals; writing them inline in the two `map[]` reads moved the whole
  register picture to the game's (n in ESI, the cell height and the `/40` divisor in EDI): 41 rows
  -> 15. Declaration order changes nothing (all 720 permutations of the six locals: best 33 rows).
- **... but two identical address chains only merge if the index comes from one variable.**
  `dropped2_tick` 0x178f6's two damage blocks both compute `stats[bodies[r].stats] * 48`; written with
  the literal racers (`stats[bodies[2].stats].damage -= 10;`, `stats[bodies[4].stats]...`) WCC colours
  the two index temporaries with different register pairs, so the chains are different text, the
  second block cannot `jmp` into the first's, and the function lands 10 bytes long - which also puts
  the epilogue the five 0-row callbacks jmp to out of place. Reading the first site's index through
  the `int h` the function reuses later (`h = bodies[2].stats; stats[h].damage -= 10;`) matches:
  16 -> 2 rows and seven functions to MATCH (117 -> 124). A dedicated `int s` does not merge (44
  rows), a `short s` merges but truncates the load to `mov ax,[..]` + `cwtl` (21), and the holder in
  both blocks does not (43) - the second site keeps its literal index (variants.py/inplace.py,
  .work/probe/d4-d7).

- **Count the variables, not just the spellings.** `start_lights` 0x1103e sat at 3 rows (one lamp's
  `imul` destination mirrored) through ~2400 spellings that all used one `struct frame *f` for the
  five lamps. Two locals, `f` for lamps 1, 2, 5 and `g` for lamps 3, 4, MATCH; 26 of the 52 ways to
  share locals among the lamps do (.work/probe/sl4, 2026-11-26). Each local is dead between its
  uses, but each C variable is its own allocation candidate in WCC386, so reusing one for unrelated
  values changes the register ties. Sweep the set partitions of a function's reused local.

- **... and where a holder is assigned.** `init_objects` 0x148db's `rrrio.fuel << 16` landed in EDX
  whether written inline or through a local assigned just before the fuel store. Assigning it to
  the dead loop counter `i` two statements EARLIER, before the `drops` store, puts the product in
  EBX and the next load after it, as in the game (7 rows -> 2, .work/probe/io2, 2026-11-26).

- **A near miss that is 50+ bytes short is missing code, not spelling: read the game's listing
  for it before sweeping.** `bull_tick` 0x12826 was 2170 of 2258 bytes at 133 rows; the gap was
  a second vx/vy fixmul pair the game computes unconditionally before the `if (!finished2)` one
  (a redundancy in the original source). Counting one table's references per function on the
  game's side (`cos_tbl` 0x73de0: 2 in bull_tick, 1 in the other five ticks) shows such a
  duplicate quickly. Adding it: 133 -> 111 rows before any spelling work (2026-11-26).

- **`imul r, x, K; shl r, S` is `(x * K) << S`, not `x * K * 2^S`.** WCC folds a product of
  constants into one multiplier and lowers it as a shift/add chain; the game's `imul eax, edi,
  0x3e8; shl eax, 0x10` keeps the two apart, so the C shifts. `player_crash_tick` 0x19497's
  ground test `(g * 1000) << 16`: the only one-side constant gone and 259 -> 147 rows after a
  declaration sample (2026-11-26). Same bits, so behaviour-neutral. player_tick 0x11eb5 has the
  same test.

- **Re-sweep declaration order after every text change.** An order swept once stops being the best
  when the C around it changes: `muis_crash_tick` 0x1a42a's negated want chain scored 72 under the
  old declarations and 46 under `short k; int go, d, m, h; unsigned want, f;`, 16 of 576 order x
  spelling combinations (2026-11-26). `.work/probe/declsweep.py FILE.c FUNC OUTDIR [MAX]` writes
  the orders for inplace.py; `reverse-engineering/tools/wc/psweep.py` scores them itself, 8 at a
  time (about 0.11 s an order, so all 5,040 orders of 7 declarations in ~9 min). In a file with several near misses, score the whole object: drive.c's
  three ticks share tails, so muis_tick's best order costs player_tick 7 rows.

- **`test; jbe A; cmp 3; je B; jmp join` is a switch.** On an unsigned value, WCC lowers
  `switch (v) { case 0: ... case 3: ... }` to that, with both arms out of line; `if (v <= 0) ...
  else if (v == 3) ...` puts them in the fall-through slot instead. `muis_crash_tick` 0x1a42a's
  f24 chain: 46 -> 40 rows (2026-11-26). Spotting it needs the game's listing, not fndiff: fndiff
  only shows the missing `jmp`.

- **Switch on the field, not on a copy of it.** `f = stats[...].f24; switch (f)` and
  `switch (stats[...].f24)` are the same compares, but the local is one more allocation candidate
  and it moves registers all over the function: `muis_crash_tick` 0x1a42a went 23 -> 6 rows on
  that one line (the field unsigned, 2026-11-26). It only showed once the game's other spellings
  were in together (`switch (want)`, body - waypoint, `h - heading`), each of which costs rows
  alone: when several "true" spellings each score worse, try them all at once and re-sweep the
  declarations, one variable per line (.work/probe/mcb).

- **A `movsx` after a `sar` is a `(short)` cast in the C.** The tick tails' `movsx ebx, bx` straight
  after `sar ebx, 0x10` comes from `(short)(bodies[n].x >> 16)`: without the cast WCC emits no
  movsx there, with it it does (observed, not a documented rule). Worth 3-30 rows per tick function (2026-11-26), but
  usually only together with a fresh declaration-order sweep.

- **Inside `abs(a - b) + abs(c - d)` the operand order is per SUM and per TERM.** `bull_tick`
  0x12826: its waypoint sum is `abs((bodies[2].x >> 16) - (waypoint2[i].x >> 16)) + abs((waypoint2[i].y
  >> 16) - (bodies[2].y >> 16))` - the first term body-first, the second waypoint-first - while its
  `play_fx(6, distance to RedCat)` sum is `bodies[n] - bodies[0]` in both terms. Mirroring the second
  term of the first sum wins 2 rows (50 -> 48), mirroring the play_fx pair costs 1-7 and mirroring
  three of the four is worse than two (2026-11-28, .work/pi/bt/v.py, v2.py). Score each term on its
  own: a mirror written up as "costs rows" had only ever been scored bundled with the tail rewrite.

- **A store of 0 from a register means a local was zeroed first.** When the game opens with
  `xor ebp, ebp` and then stores EBP where the C stores 0, the local that lives in EBP (`go` in
  `bull_crash_tick` 0x19c8c) is assigned 0 before those stores: WCC reuses the zeroed register
  for the constant. Moving `go = 0;` from third to first statement, with the next item: 128 -> 48
  rows (2026-11-26, .work/probe/bcx/t2). The sibling crash ticks keep it third, so read each
  function's prologue rather than copying the order.

- **Which arm of an if/else comes first follows the C's arm order, not the test's sense.**
  `if (!racer_contact(n, 4)) v55ff4 = 0; else if (...)` puts the short arm in the fall-through slot;
  the game's `je` over the long arm to an out-of-line `v55ff4 = 0` is `if (racer_contact(n, 4)) {
  ... } else v55ff4 = 0;` (bull_crash_tick, -5 rows with or without `go = 0` first, which alone is -29: 82 -> 53 -> 48). Same rule as
  `main`'s negated tests: write the arm the game places first as the if-arm.

- **A spelling that costs rows alone can be the game's: score it again after the next lever.**
  bull_crash_tick's `(short)` x in the tail cost 60+ rows on the 48-row text (best 110 over 5,040
  orders); with four more neutral respellings found by a random search (.work/probe/bcx/genR.py) it
  reached 51, and a fresh declaration sweep then 42, with the tail matching the game's
  `sar edx, 0x10; movsx ebp, dx`. Keep the game-faithful spelling in the search set even when
  it looks worse.

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
- A WLINK link that reproduces `RRR.EXE`'s layout: the all-blob link is byte-identical
  (`reverse-engineering/tools/link/link.sh`); what is left is linking real objects in place of the blobs.
