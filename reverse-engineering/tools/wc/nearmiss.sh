#!/bin/sh
# nearmiss.sh: rank every near miss of src/check.sh by instruction diffs (fndiff.py), smallest first.
# Prints "DIFFS 0xADDR name" lines; src/nearmiss.txt is built from this.
# usage: reverse-engineering/tools/wc/nearmiss.sh [> .work/nm_ranked.txt]
cd "$(dirname "$0")/../../.." || exit 1
PY=.work/venv/bin/python
src/check.sh 2>&1 | grep "differs from" | awk '{print $1, $6}' |
while read n a; do
    r=$($PY reverse-engineering/tools/wc/fndiff.py rrr .work/src/RRR.OBJ "${n%_}" "$a" 2>&1 | tail -1 |
        grep -oE "[0-9]+ instruction diffs" | cut -d' ' -f1)
    echo "${r:-?} $a ${n%_}"
done | sort -n
