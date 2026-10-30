#!/bin/sh
# Build the WebAssembly game (port/web/out/redcat.*). The C sources are the
# desktop port's own; the web-only part is the __EMSCRIPTEN__ main loop in
# game.c.
#
#   port/web/build.sh      (needs Emscripten: source emsdk_env.sh first, or
#                           have it in .work/emsdk)
#   port/web/serve.sh      then open http://localhost:8080/redcat.html
#
# No game data goes into out/: the page asks for the player's own game folder
# once and keeps the files it needs in the browser (shell.html, save.js).
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
PORT=$(cd "$HERE/.." && pwd)
ROOT=$(cd "$PORT/.." && pwd)

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

OUT="$HERE/out"
mkdir -p "$OUT"
cd "$PORT"
CORE=$(sources library rccore)
emcc -O2 -std=gnu99 -Isrc $CORE $(sources executable redcat) \
    -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 \
    -sFORCE_FILESYSTEM=1 -lidbfs.js --pre-js "$HERE/save.js" \
    -sINVOKE_RUN=0 -sEXPORTED_RUNTIME_METHODS=callMain,FS \
    --shell-file "$HERE/shell.html" \
    -o "$OUT/redcat.html"
echo "built $OUT/redcat.html"
