#!/bin/sh
# probe.sh: score C variants of one function in one compile (wraps variants.py).
#
#   reverse-engineering/tools/wc/probe.sh VARIANTS.C 0xADDR 0xEND NAME [NAME...]
#
# VARIANTS.C holds the variants, each under its own NAME. It is compiled after src/rrr.c (so every
# type, extern and prototype there and in src/rrr.h is in scope) with the game's options, into .work/probe/, and
# variants.py ranks the NAMEs by instruction rows that differ from the game in [ADDR, END).
set -eu
cd "$(dirname "$0")/../../.."
v=$1; shift
w=.work/probe; mkdir -p $w; rm -f $w/PROBE.OBJ
cp src/rrr.h $w/RRR.H
cat src/rrr.c "$v" > $w/PROBE.C
reverse-engineering/tools/wc/wcc.sh $w PROBE.C -bt=dos -3r -s -ox -fo=PROBE.OBJ | grep -iE 'error|warning' | grep -v ' 0 warnings, 0 errors' || true
.work/venv/bin/python reverse-engineering/tools/wc/variants.py $w/PROBE.OBJ rrr "$@"
