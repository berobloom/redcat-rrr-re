# Work queue (human-owned)

Work the first item that is neither ticked nor marked READY FOR REVIEW, top to
bottom. While any item here is open it overrides STATUS.md `## Next`.

Rules for the agent:

- **Only the human ticks `[x]`.** When you think an item is done, add
  `READY FOR REVIEW: <check or evidence>` under it, and say in STATUS.md which
  check proves it and which mutation makes that check fail.
- An item is done when its **Accept** line holds, not when a theory explains
  the report.
- If new evidence shows an item is wrong or should be split, write that under
  the item. Never delete an item.
- If an item needs the live harness and the harness fails twice in a row,
  write BLOCKED with the reason under it and take the next item that can be
  done offline.

## Items

- [ ] **Q1 Matching: RRR.EXE's own C**
  Report: the human asked for a matching decompilation in wacky-re's structure (2026-11-22).
  Accept: every function in 0x10010-0x1B915 is in `src/` (its module's file or `src/rrr.c`), in address order, and `src/check.sh`
  shows it as MATCH or it has a `src/nearmiss.txt` line with a SKIP/LAYOUT status and a
  `/* differs: ... */` note; `PROGRESS.md` regenerated.
  - 2026-08-09: READY FOR REVIEW - `reverse-engineering/tools/entries.py` proved 10 of
    the old 157 starts in the range are byte-scan artifacts (the code before them runs
    straight through the address), so `PROGRESS.md` now reads 147 functions, 110
    byte-identical (74.8%), 22 near misses, 15 to write (the list is in STATUS.md).
  - 2026-11-23: still READY FOR REVIEW, counts moved: sound_init 0x1043a 13 -> 10 rows, the new
    fn_13a78 0x13a78 is a 15-row near miss, and `enter`-frame scan of the C range shows blit_sprite
    0x11bb4 and blit_sprite_trans 0x17d94 are assembly, so 110 of 147, 23 near misses, 14 unwritten
    (13 C). `src/check.sh` still shows 110 MATCH and no WRONG fixups.
  - 2026-11-23: podium 0x15a9f byte-identical: `src/check.sh 2>&1 | grep -c ' MATCH at'` says 111 and
    `grep podium` shows `podium_ 355 bytes MATCH at 0x015a9f` with no WRONG fixup (the world_names
    0x57c18 and rcascii_printf 0x200b0 targets agree); deleting either switch level or one vararg of
    the "%-15s : %3d" line makes the check fail. 111 of 147, 23 near misses, 12 unwritten.
