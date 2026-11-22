#!/bin/sh
# Compile src/rrr.c with the game's compiler and options (Watcom C/C++ 10.5, -3r -s -ox; MATCHING.md)
# and compare every function with RRR.EXE (addresses from reverse-engineering/analysis/rrr_names.txt
# by name), then the fixup targets.   src/check.sh [-v]  (-v: also the differing bytes)
set -eu
cd "$(dirname "$0")/.."
w=.work/src; mkdir -p $w; rm -f $w/RRR.OBJ $w/RRR.obj; cp src/rrr.c $w/RRR.C
reverse-engineering/tools/wc/wcc.sh $w RRR.C -bt=dos -3r -s -ox -fo=RRR.OBJ | grep -iE 'error|warning' | grep -v ' 0 warnings, 0 errors' || true
[ -f $w/RRR.OBJ ] || { echo "src/check.sh: no object (see $w/WCC.OUT)"; exit 1; }
if [ "${1:-}" = -v ]; then .work/venv/bin/python reverse-engineering/tools/wc/objmatch.py rrr $w/RRR.OBJ
else .work/venv/bin/python reverse-engineering/tools/wc/objmatch.py rrr $w/RRR.OBJ | grep -v '^      '; fi
