# Overnight self-healing RE loop

Ralph-style loop: a fresh pi context per pass, all state in files and git. The same supervisor as
wacky-re's and re_cold's loops (`overnight.py`, `watch.py` and `watchdog.ts` are copied unchanged
from `../wacky-re/overnight/`); this copy is wired to RedCat's Razende Rekenrace.

```
overnight/run.sh                        # a night: new branch overnight/YYYYMMDD, sleep inhibited
overnight/run.sh --hours 3              # shorter; --passes N caps passes
python3 overnight/overnight.py --smoke  # ~1 min: model, tool calls, watchdog, policy block
python3 overnight/overnight.py --dry-run
touch overnight/STOP                    # stop after the current pass; kill <pid> = stop now
python3 overnight/watch.py              # live view of the newest run
```
Morning: `overnight/runs/<stamp>/REPORT.md`, then review `git log --stat main..HEAD` and merge.

| File | Role |
|---|---|
| `overnight.py` | supervisor: passes, gates, resumes, rollback, report |
| `watch.py` | read-only live viewer |
| `watchdog.ts` | pi extension: in-session healing (policy blocks, nudges, turn budget) |
| `prompt.md` | the one-iteration procedure (follows AGENTS.md; the task is QUEUE.md's first open item) |
| `check.sh [rev]` | health gate on a clean worktree: original checksums (`original.sha256`: the ISO and `.work/iso/*.EXE`), tools compile, `src/check.sh` MATCH >= `src/matched.baseline`, DATA >= `src/data.baseline`, no `WRONG` fixup, STATUS <= 260 lines, no file > 4 MB, the port builds |
| `cleanup.sh` | kills only this repo's leftovers: DOSBox-X on `.work/dos/redcat.conf`, `reverse-engineering/tools/dosrun.py`, a hung Wine compile under this repo's `.work/watcom` |
| `config.json` | model, budgets, policy |

## This project's differences from wacky-re

- **No harness at night.** RRR's dosrun.py uses QMP 4444 / gdb 2159 and Xvfb `:57`, the same
  display wacky-re's harness uses (and wacky's `cleanup.sh` kills `Xvfb :57`). So the prompt forbids
  the DOSBox-X harness, the policy blocks `dosrun.py`, `winedbg` and every harness port, and this
  cleanup never touches Xvfb. Do not run a RRR night and a wacky-re harness night at the same time
  unless the RRR prompt stays harness-free.
- **The task is QUEUE.md's**, as AGENTS.md says; STATUS.md `## Next` only feeds the supervisor's
  report line. Put tonight's work in QUEUE.md as the first open item before starting.
- **STATUS.md is long** (229 lines on 2026-11-28), so the gate caps it at 260 instead of wacky's 80.
- **WRONG fixups fail the gate**: fndiff masks fixup bytes, so a function can print MATCH with its
  fields swapped (init_objects did, 2026-11-28). check.sh fails on any `WRONG` line.
