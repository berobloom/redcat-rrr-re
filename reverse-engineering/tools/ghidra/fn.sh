#!/bin/sh
# Print one function from .work/ghidra/<prog>.c (decomp.sh output).
#   reverse-engineering/tools/ghidra/fn.sh rrr 0x11526 [more addrs...]
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
PROG=$1; shift
for a in "$@"; do
    a=$(printf '%08x' "$a")
    awk -v a="$a" '/^\/\/ ==== /{p = ($3 == a)} p' "$ROOT/.work/ghidra/$PROG.c"
done
