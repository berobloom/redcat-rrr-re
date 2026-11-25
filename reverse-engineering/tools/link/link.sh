#!/bin/sh
# Rebuild RRR.EXE with WLINK 10.6 and compare it with the original.
#
#   reverse-engineering/tools/link/link.sh [OUTDIR]     (default .work/link/out; created, never emptied)
#
# Today every byte comes from blobs carved out of the original (blob.py): CODEBLOB.OBJ (the code
# object) and DATABLOB.OBJ (the data object), with every fixup written back in the order the original
# objects gave them to the linker. RRR.LNK holds the directives the original link needed (os2 le, the
# 10.6 wstub, modname rrr, preload code and data). The last line of lecmp.py is the score:
# `LINK: N of M file bytes identical`.
set -eu
cd "$(dirname "$0")/../../.."
out=${1:-.work/link/out}
mkdir -p "$out"
cp reverse-engineering/tools/link/RRR.LNK "$out/RRR.LNK"
.work/venv/bin/python reverse-engineering/tools/link/blob.py .work/iso/RRR.EXE "$out"
reverse-engineering/tools/link/wlink.sh "$out" RRR.LNK | grep -v -i -E 'copyright|trademark|linker version' || true
[ -f "$out/RRR.EXE" ] || { echo "link.sh: no RRR.EXE (see $out/WLINK.OUT)"; exit 1; }
.work/venv/bin/python reverse-engineering/tools/link/lecmp.py "$out/RRR.EXE" .work/iso/RRR.EXE
