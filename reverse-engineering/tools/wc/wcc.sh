#!/bin/sh
# Compile C with Watcom C/C++ 10.5 under Wine (the game's compiler, reverse-engineering/watcom.md).
#
#   reverse-engineering/tools/wc/wcc.sh SRCDIR FILE.C [wcc386 options]
#
# SRCDIR holds FILE.C and receives FILE.OBJ and WCC.OUT (the compiler's messages). Exit 0 when the
# object is newer than the start of this run. WC=<dir> picks another install (default
# .work/watcom/dos/C/WATCOM); WCC=WPP386 picks another compiler.
set -eu
[ $# -ge 2 ] || { sed -n 2,9p "$0"; exit 2; }
root=$(cd "$(dirname "$0")/../../.." && pwd)
wc=$(cd "${WC:-$root/.work/watcom/dos/C/WATCOM}" && pwd); src=$(cd "$1" && pwd); file=$2; shift 2
base=${file%.*}
W=$(winepath -w "$wc" 2>/dev/null)
stamp=$(mktemp); trap 'rm -f "$stamp"' EXIT
rm -f "$src/WCC.OUT"
(cd "$src" && WATCOM="$W" INCLUDE="$W\\H" WINEPATH="$W\\BINW;$W\\BINNT" WINEDEBUG=-all \
    timeout 300 wine "$W\\BINNT\\${WCC:-WCC386}.EXE" "$@" "$file" > WCC.OUT 2>&1) || true
tr -d '\r' < "$src/WCC.OUT" | grep -v '^$' | tail -8
[ "$src/$base.obj" -nt "$stamp" ] || [ "$src/$base.OBJ" -nt "$stamp" ] || [ "$src/$(echo "$base" | tr a-z A-Z).OBJ" -nt "$stamp" ]
