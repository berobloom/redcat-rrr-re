#!/bin/sh
# Build the WebAssembly viewer (port/web/out/rcview.{html,js,wasm,data}).
# The C sources are the desktop port's own; the web-only part is the
# __EMSCRIPTEN__ main loop in main.c.
#
#   port/web/build.sh <game data dir>   (needs Emscripten: source emsdk_env.sh
#                                         first, or have it in .work/emsdk)
#   port/web/serve.sh                   then open http://localhost:8080/
#
# The game data is packed into out/rcview.data. It is the original game's:
# never publish out/.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
PORT=$(cd "$HERE/.." && pwd)
ROOT=$(cd "$PORT/.." && pwd)
DATA=${1:?usage: build.sh <game data dir>}
DATA=$(cd "$DATA" && pwd)

if ! command -v emcc >/dev/null 2>&1; then
    if [ -x "$ROOT/.work/emsdk/upstream/emscripten/emcc" ]; then
        PATH="$ROOT/.work/emsdk/upstream/emscripten:$PATH"
    else
        echo "emcc not found: install Emscripten (emsdk) or source its emsdk_env.sh" >&2
        exit 1
    fi
fi

# the source lists of rccore and rcview in CMakeLists.txt
sources() {
    sed -n "/add_$1($2/,/)/p" "$PORT/CMakeLists.txt" | tr -d ' ' | grep '\.c$'
}
SRC=$( (sources library rccore; sources executable rcview) | sort -u)

# only the archives the viewer opens, under the names it asks for
PRELOAD=""
for base in RCS RRRMENU REKEN; do
    for ext in IND IMG; do
        f=$(find "$DATA" -maxdepth 1 -iname "$base.$ext" | head -n 1)
        [ -n "$f" ] || { echo "$base.$ext not in $DATA" >&2; exit 1; }
        PRELOAD="$PRELOAD --preload-file $f@/data/$base.$ext"
    done
done

OUT="$HERE/out"
mkdir -p "$OUT"
cd "$PORT"
# shellcheck disable=SC2086
emcc -O2 -std=gnu99 -Isrc $SRC \
    -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 \
    --shell-file "$HERE/shell.html" $PRELOAD \
    -o "$OUT/rcview.html"
echo "built $OUT/rcview.html"
