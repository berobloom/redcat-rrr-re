#!/usr/bin/env bash
# Start the overnight RE loop for RedCat's Razende Rekenrace. Keeps the machine awake; logs to
# overnight/runs/<stamp>/.
#   overnight/run.sh                 # full night (config.json: max_hours, max_passes)
#   overnight/run.sh --hours 2       # shorter
#   touch overnight/STOP             # finish the current pass, then stop
cd "$(dirname "$0")/.." || exit 1
# Own memory-capped scope: a runaway agent command gets OOM-killed alone (OOMPolicy=continue)
# instead of filling RAM+swap and taking the whole terminal scope down with it.
exec systemd-run --user --scope -q -p MemoryMax=9G -p MemorySwapMax=2G -p OOMPolicy=continue \
  systemd-inhibit --what=sleep:idle --who=redcat-overnight --why="RedCat RRR overnight RE loop" \
  python3 overnight/overnight.py "$@"
