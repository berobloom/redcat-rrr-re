#!/bin/sh
# Run Watcom 10.6's WLINK under Wine on a directive file.
#
#   reverse-engineering/tools/link/wlink.sh DIR FILE.LNK
#
# DIR holds FILE.LNK and the objects it names (relative DOS names) and receives the output and
# WLINK.OUT (the linker's messages). WC=<dir> picks another install (default 10.6, the game's linker).
set -eu
[ $# -ge 2 ] || { sed -n 2,7p "$0"; exit 2; }
root=$(cd "$(dirname "$0")/../../.." && pwd)
wc=$(cd "${WC:-$root/.work/watcom/dos/C106/WATCOM}" && pwd); dir=$(cd "$1" && pwd); lnk=$2
W=$(winepath -w "$wc" 2>/dev/null)
(cd "$dir" && WATCOM="$W" LIB="$W\\LIB386;$W\\LIB386\\DOS" WINEPATH="$W\\BINW;$W\\BINNT" WINEDEBUG=-all \
    timeout 300 wine "$W\\BINNT\\WLINK.EXE" @"$lnk" > WLINK.OUT 2>&1) || true
tr -d '\r' < "$dir/WLINK.OUT" | grep -v '^$' | tail -15
