#!/bin/sh
# Decompile a whole LE program with Ghidra headless into .work/ghidra/<prog>.c
# (names from analysis/<prog>_names.txt applied; one "// ==== addr name"
# header per function, so grep and sed can find them).
#   reverse-engineering/tools/ghidra/decomp.sh rrr
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../.." && pwd)
PROG=${1:?usage: decomp.sh <prog>}
GHIDRA=${GHIDRA_INSTALL_DIR:-$HOME/Downloads/ghidra_12.1.4_PUBLIC}
OUT="$ROOT/.work/ghidra"
mkdir -p "$OUT"
# Ghidra refuses project paths with a dot-directory in them (.work)
PROJ=$(mktemp -d "${TMPDIR:-/tmp}/redcat-ghidra.XXXXXX")
trap 'rm -rf "$PROJ"' EXIT
A="$ROOT/reverse-engineering/analysis"
"$GHIDRA/support/analyzeHeadless" "$PROJ" "$PROG" -overwrite \
    -import "$ROOT/.work/le/$PROG.elf" \
    -scriptPath "$HERE" \
    -postScript DumpDecomp.java "$A/${PROG}_names.txt" "$A/${PROG}_funcs.tsv" "$OUT/$PROG.c" \
    -deleteProject > "$OUT/$PROG.log" 2>&1
grep -c '^// ====' "$OUT/$PROG.c" | sed "s|^|$OUT/$PROG.c: |; s|$| functions|"
