#!/bin/sh
# Compile each module of src/ (src/*.c, sharing src/rrr.h) with the game's compiler and options
# (Watcom C/C++ 10.5, -3r -s -ox; MATCHING.md) into .work/src/<MODULE>.OBJ, and compare every function
# with RRR.EXE (addresses from reverse-engineering/analysis/rrr_names.txt by name), then the fixup
# targets, then each object's data segments against the game's data object, one object at a time.
#   src/check.sh [-v]  (-v: also the differing bytes)
set -eu
cd "$(dirname "$0")/.."
w=.work/src; mkdir -p $w; rm -f $w/*.OBJ $w/*.obj $w/*.C $w/*.H
cp src/rrr.h $w/RRR.H
for f in src/*.c; do
    m=$(basename "$f" .c | tr a-z A-Z)
    cp "$f" $w/$m.C
    reverse-engineering/tools/wc/wcc.sh $w $m.C -bt=dos -3r -s -ox -fo=$m.OBJ | grep -iE 'error|warning' | grep -v ' 0 warnings, 0 errors' || true
    [ -f $w/$m.OBJ ] || { echo "src/check.sh: no object for $f (see $w/WCC.OUT)"; exit 1; }
done
for f in src/*.c; do
    m=$(basename "$f" .c | tr a-z A-Z)
    if [ "${1:-}" = -v ]; then .work/venv/bin/python reverse-engineering/tools/wc/objmatch.py rrr $w/$m.OBJ
    else .work/venv/bin/python reverse-engineering/tools/wc/objmatch.py rrr $w/$m.OBJ | grep -v '^      '; fi
done | tee $w/check.out
# the data total: bytes of the objects' data segments (CONST, _DATA, ...) that sit byte for byte at the
# game's address, a segment counting only when all of it does (src/data.baseline holds the number)
awk '/^data: /{s+=$2; t+=$4} END{printf "DATA: %d of %d data-segment bytes identical in place\n", s, t}' $w/check.out
