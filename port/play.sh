#!/bin/sh
# Build (if needed) and run a race on a game directory: the CD root or the
# installed C:\DAVILEX\RCREKEN (the one with RCS.IND, RC.IND and TRANS.DAT).
#   port/play.sh <game data dir> [map 0..22] [--fps N]
set -e
PORT=$(cd "$(dirname "$0")" && pwd)
DATA=${1:?usage: play.sh <game data dir> [map 0..22] [--fps N]}
shift
if [ ! -x "$PORT/build/redcat" ]; then
    cmake -S "$PORT" -B "$PORT/build" >/dev/null
fi
cmake --build "$PORT/build" -j >/dev/null
exec "$PORT/build/redcat" "$DATA" "$@"
