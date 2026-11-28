#!/usr/bin/env bash
# Kill only THIS project's leftover processes. wacky-re, re_cold and others run their own
# DOSBox-X/gdb/Xvfb/Wine on this machine: never pkill by bare name. Every pattern names this repo's
# own paths; patterns use [x] so pgrep never matches this script's own command line.
# Not killed: Xvfb :57 (dosrun.py's display is shared with wacky-re's harness).
pats=('redcat-rrr-re/[.]work/dos/redcat[.]conf'           # dosrun.py's DOSBox-X
      '[r]everse-engineering/tools/dosrun[.]py'           # this repo's dosrun.py (wacky's is tools/dosrun.py)
      'redcat-rrr-re.[.]work.watcom.*WCC386'              # a Wine compile left hanging (winedbg or the compiler)
      'redcat-rrr-re/[.]work/watcom/dos/C106.*winedbg')
for pat in "${pats[@]}"; do
  for pid in $(pgrep -f "$pat"); do
    [ "$pid" = "$$" ] && continue
    kill "$pid" 2>/dev/null && echo "cleanup: killed $pid ($pat)"
  done
done
sleep 1
for pat in "${pats[@]}"; do
  for pid in $(pgrep -f "$pat"); do [ "$pid" = "$$" ] || kill -9 "$pid" 2>/dev/null; done
done
exit 0
