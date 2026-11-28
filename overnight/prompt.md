You are running ONE iteration of the RedCat's Razende Rekenrace (RRR.EXE, Watcom C 10.6, DOS/4GW)
reverse-engineering and matching-rebuild project, unattended, overnight. No human will answer: never
ask, never wait for confirmation. When you must choose, pick the conservative option and write the
decision down where step 4 says.

Do exactly this, in order:

1. `git status --short`. An interrupted run may have left work behind: if a `src/*.c` file is
   modified and you did not write it in this pass, run `git checkout -- <that file>`. Then read
   `QUEUE.md` and take its FIRST item that is not ticked `[x]`, not READY FOR REVIEW and not BLOCKED.
   Read `AGENTS.md`, then grep (do not read whole) `MATCHING.md`, `GOTCHAS.md` and
   `research/open-questions.md` for the function or subject you are about to touch. Never read whole:
   `src/rrr.h`, `src/*.c` (grep `^[a-z].* NAME(` and read from the comment above it to its closing
   brace), `research/discoveries.md`, `QUEUE.md` past the item you work.

2. THE TASK comes from `QUEUE.md` only. If no item is open, write nothing, commit nothing, print
   `NO TASK: QUEUE.md has no open item` and stop.
   For a near-miss item, take the item's functions in its order, up to 2 per iteration. Round 1:
   skip any whose `src/nearmiss.txt` status already starts with `ON:` or `MATCH`. When every listed
   function has `ON:` or `MATCH`, round 2: skip those with `ON2:` or `MATCH`, and use levers the
   `ON:` note did not try. When all have `ON2:` or `MATCH`, add `READY FOR REVIEW` under the item.
   For one function:
   a. Rows: `src/check.sh >/dev/null 2>&1` once if `.work/src/<MODULE>.OBJ` is missing, then
      `.work/venv/bin/python reverse-engineering/tools/wc/fndiff.py rrr .work/src/<MODULE>.OBJ NAME
      0xADDR | tail -40` (the module is the src/*.c file that defines NAME, upper-cased). Read the
      function's `differs:` note (grep `^[a-z].* NAME(` in src/*.c and read from the comment above
      it to the closing brace) and its nearmiss.txt line. Name what each differing row holds: which
      local, global or field, and which register or slot the game uses for it. Look at the game's
      own instructions around the rows too:
      `objdump -D -b binary -m i386 -M intel --adjust-vma=0x10000 --start-address=0xA --stop-address=0xB
      .work/le/rrr.obj1.bin` (A, B: the function's address range).
   b. CONTENT FIRST (f3decomp's finding, and ours: init_objects, ai_waypoint_action,
      find_waypoints): a near miss is usually a misreading of what the source says, not an order.
      Ask before spelling: does the game do something ours does not (an extra load, a re-read of a
      global after a call, a value computed twice, a sixth switch arm, a field written through a
      different index)? Is a constant the result of an expression the compiler folded (`stats[bodies
      [2].stats].x` after `bodies[2].stats = 1;` compiles to `stats[1].x`)? Does a sibling function
      (bull_* vs muis_* vs player_*, dropped0/2/4_tick) already MATCH with a shape you can copy?
   c. Write up to 12 variants into `.work/pi/NAME/v.py`, none already listed in the differs note
      or the nearmiss.txt line: `VARIANTS = [('label', [('old text', 'new text'), ...]), ...]`. Each
      old text must occur exactly once in the module: include the function's own lines (a statement
      plus its neighbour), never a bare `c = 0;`. Use triple-quoted strings for multi-line text and
      keep the file's 4-space indentation exactly. Levers, best first (MATCHING.md, GOTCHAS "Levers
      that produced MATCHes on RRR"):
      1. content from b;
      2. a global read in place instead of through a local, or the reverse (cache it in a local);
      3. a pointer local <-> the indexed expression (`p->f` <-> `arr[i].f`); one local shared by
         several sites <-> one per site (start_lights needed two locals for five lamps);
      4. merge two locals never live together, or split one local used for two jobs;
      5. if/else arm order, `!(a && b)` <-> `!a || !b`, `switch` <-> if-chain, `for` <-> `while`;
      6. declaration order of the locals (only when the rows are register homes or stack slots).
   d. Run `.work/venv/bin/python reverse-engineering/tools/wc/vbatch.py NAME 0xADDR
      .work/pi/NAME/v.py` (about half a second a variant: it compiles a copy of the module in
      .work/vbatch/ and never touches src/). It prints rows, MATCH and `wrong` (fixups naming
      another field than the game's) per variant. `control` must show the row count from step a.
      You may run up to 6 batches per function: each new batch builds on the best of the last
      (put the winning edits first in every variant of the next batch). A variant that prints
      `pattern found N times`: fix its old text.
   e. A variant with MATCH and wrong=0: apply exactly that change to `src/`, run `src/check.sh 2>&1 |
      grep -c ' MATCH at'` (must be `src/matched.baseline` + 1) and `src/check.sh 2>&1 | grep -c
      WRONG` (must be 0). Then write the new count into `src/matched.baseline`, replace the `differs:`
      part of the comment with one line naming the source shape, set the nearmiss.txt line to `0
      0xADDR NAME  MATCH <date>: <shape>. Was: <old status>`, run `.work/venv/bin/python
      reverse-engineering/tools/progress.py`, and commit (the src file, src/matched.baseline,
      src/nearmiss.txt, PROGRESS.md, docs/RRRPROGRESS.*). If a lever worked, add one line to the
      matching GOTCHAS.md "Levers" entry naming the function.
      A variant with fewer rows but not 0 (and wrong=0): apply it to src/ only if the whole check's
      MATCH count stays the same, WRONG stays 0 and the rows drop by 2 or more; update the number at
      the start of its nearmiss.txt line and say so in the differs note.
      A MATCH with wrong > 0 is not a match: it means statement order or field choice; keep working
      it (reorder the statements that build the WRONG stores) or record it, never commit it to src/.
   f. Whatever the outcome, prepend `ON: <date> <K> variants (<labels>), best <R> rows (<label>)` (or
      `ON2:` in round 2) to the function's nearmiss.txt status, and add the labels to the differs
      note's tried list. Commit that. ALWAYS end a function with a commit; a pass without a commit
      counts as stuck. Budget: about 35 minutes per function.
   g. Never commit a `src/` that lowers the MATCH count or the DATA count (last line of src/check.sh)
      or prints a WRONG line.
   Not tonight: the DOSBox-X harness (`dosrun.py`, `reverse-engineering/dosbox/`), `winedbg`/gdb on
   the compiler, `.work/wcc/` (an interactive session's debugger scripts), new functions, the port.
   `reverse-engineering/tools/wc/wcc.sh` (used by src/check.sh, vbatch.py, inplace.py and
   splitprobe.py) runs Watcom under Wine and is allowed. Never name `dosrun.py` in a shell command.
   `splitprobe.py NAME 0xADDR` (GOTCHAS "Block splits") is allowed but found nothing for any listed
   function on 2026-11-28 (.work/pi/split/probe/): rerun it only after a change that adds or removes
   whole statements.

3. Mark every claim CONFIRMED / STRONGLY SUPPORTED / LIKELY / SPECULATIVE with its evidence (the
   probe output, addresses, the compile). Never present an inference as fact.

4. Where things go (AGENTS.md): what a function needed -> its `differs:` note in `src/*.c` and its
   `src/nearmiss.txt` line; a compiler lever that worked -> one entry under GOTCHAS.md "Block splits"
   naming the function; a confirmed finding -> `research/discoveries.md`; an open question ->
   `research/open-questions.md`. Do not duplicate what is already there (grep first). Progress on
   the item -> a dated `- <date> (overnight): ...` note under the item in `QUEUE.md`; when its Accept
   line holds, add `READY FOR REVIEW: <check>` there instead. Only the human ticks `[x]`.

5. Update `STATUS.md` once, inside your last commit (not a commit of its own): only the match count
   and the `## Next` items. Keep it at 260 lines or fewer; if it is longer, delete your own
   additions.

6. Commit after each function: `git add` only the files you changed (never `git add -A` or `.`),
   then `git commit -m "overnight: <one-line summary>"`. End the message with
   `Co-Authored-By: overnight agent <noreply@localhost>`.

7. Run `overnight/check.sh` after committing. If it fails, fix the cause and commit again until it
   prints OK. Never edit `overnight/`, the ISO or `.work/iso/` to make it pass.

8. Stop. Do not start a second iteration.

How to not make the classic mistakes (GOTCHAS.md):
- A near miss whose rows are only register homes or a two-instruction swap is still a source
  question: the bytes moved because of something the C says. Look for content (missing statements,
  a different expression the compiler folds) before spelling.
- `inplace.py` and `splitprobe.py` compile the function inside its own module; scratch copies
  compiled alone score differently (GOTCHAS "Probes"). Trust only in-module numbers.
- fndiff masks fixup bytes: 0 rows is not a MATCH until `src/check.sh` shows MATCH and no WRONG.
- Never hand-type a constant from the binary without citing its address.

Hard rules: never push, never switch or create branches, never rewrite history (no reset, rebase,
amend, checkout of other commits), never modify `overnight/`, the ISO, `.work/iso/` or
`.work/watcom/`, never write outside this repository (except /tmp), never kill processes by name (no
pkill/killall): other projects' DOSBox-X harnesses and Wine compiles run on this machine.
