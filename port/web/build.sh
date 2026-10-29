#!/bin/sh
# Build the WebAssembly race and viewer (port/web/out/redcat.* and
# rcview.*). The C sources are the desktop port's own; the web-only part is
# the __EMSCRIPTEN__ main loop in game.c and main.c.
#
#   port/web/build.sh <game data dir>   (needs Emscripten: source emsdk_env.sh
#                                         first, or have it in .work/emsdk)
#   port/web/serve.sh                   then open http://localhost:8080/redcat.html
#
# The game data is packed into out/*.data. It is the original game's:
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

# the source list of a CMake target in CMakeLists.txt
sources() {
    sed -n "/add_$1($2/,/)/p" "$PORT/CMakeLists.txt" | tr -d ' ' | grep '\.c$'
}

# preload flags for the named files (archives as BASE.IND + BASE.IMG)
preload() {
    for f in "$@"; do
        case $f in
        *.*) names=$f ;;
        *) names="$f.IND $f.IMG" ;;
        esac
        for n in $names; do
            src=$(find "$DATA" -maxdepth 1 -iname "$n" | head -n 1)
            [ -n "$src" ] || { echo "$n not in $DATA" >&2; exit 1; }
            printf ' --preload-file %s@/data/%s' "$src" "$n"
        done
    done
}

OUT="$HERE/out"
mkdir -p "$OUT"
cd "$PORT"
CORE=$(sources library rccore)
build() {
    name=$1; shift
    files=$1; shift
    # shellcheck disable=SC2046,SC2086
    emcc -O2 -std=gnu99 -Isrc $CORE $(sources executable "$name") \
        -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 \
        --shell-file "$HERE/shell.html" $(preload $files) \
        -o "$OUT/$name.html"
    echo "built $OUT/$name.html"
}
build redcat "RCS RC TRANS.DAT"
build rcview "RCS RRRMENU REKEN"
