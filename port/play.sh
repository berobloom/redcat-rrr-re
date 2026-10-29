#!/bin/sh
# Build (if needed) and run the viewer on a game directory: the CD root or
# the installed C:\DAVILEX\RCREKEN (the one with RCS.IND and RCS.IMG).
#   port/play.sh <game data dir> [map 0..22]
set -e
PORT=$(cd "$(dirname "$0")" && pwd)
DATA=${1:?usage: play.sh <game data dir> [map 0..22]}
shift
if [ ! -x "$PORT/build/rcview" ]; then
    cmake -S "$PORT" -B "$PORT/build" >/dev/null
fi
cmake --build "$PORT/build" -j >/dev/null
exec "$PORT/build/rcview" "$DATA" "$@"
