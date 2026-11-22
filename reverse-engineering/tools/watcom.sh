#!/bin/sh
# Run a Watcom C/C++ 10.5 tool under Wine: watcom.sh wcc386 -ox -3r foo.c
# Needs the DOSSETUP install in .work/watcom/dos/C/WATCOM (see
# reverse-engineering/watcom.md). The BINNT programs are stubs that load
# the BINW (LX, DOS/4GW) program of the same name.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
W=$(winepath -w "$ROOT/.work/watcom/dos/C/WATCOM" 2>/dev/null)
tool=$(echo "$1" | tr a-z A-Z); shift
WATCOM="$W" INCLUDE="$W\\H" WINEPATH="$W\\BINW;$W\\BINNT" WINEDEBUG=-all \
    exec wine "$W\\BINNT\\$tool.EXE" "$@"
