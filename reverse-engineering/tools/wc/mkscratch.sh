#!/bin/sh
# Prepare a decomp.me scratch for one function: .work/scratch/<NAME>/ gets
#   context.h   src/rrr.h as it is (the Context box)
#   NAME.c      the function's text from its src/ module, or SRC.c if given (the Source box)
#   target.asm  the game's code in MASM syntax (scratch.py), target.s its GNU twin
#   build/      the standalone compile of context.h + NAME.c, scored against the game
#
#   reverse-engineering/tools/wc/mkscratch.sh NAME 0xADDR [SRC.c]
#
# Prints the standalone score next to the in-module one: decomp.me compiles the function alone,
# so a function whose bytes depend on its module scores differently there.
set -eu
[ $# -ge 2 ] || { sed -n 2,11p "$0"; exit 2; }
root=$(cd "$(dirname "$0")/../../.." && pwd)
name=$1; addr=$2; py=$root/.work/venv/bin/python; T=$root/reverse-engineering/tools/wc
out=$root/.work/scratch/$name; mkdir -p "$out/build"
cp "$root/src/rrr.h" "$out/context.h"
if [ $# -ge 3 ]; then cp "$3" "$out/$name.c"; else
    "$py" - "$root" "$name" "$out/$name.c" <<'EOF'
import glob, os, re, sys
root, name, dst = sys.argv[1:4]
pat = re.compile(r'^[^\n;]*\b%s\([^;{]*\)\s*\{' % re.escape(name), re.M)
for mod in sorted(glob.glob(os.path.join(root, 'src', '*.c'))):
    src = open(mod).read(); m = pat.search(src)
    if m:
        break
start = src.rfind('\n', 0, m.start()) + 1
end = src.find('/* ---- 0x', m.end()); end = len(src) if end < 0 else end
open(dst, 'w').write(src[start:end].rstrip() + '\n')
EOF
fi
cp "$out/context.h" "$out/build/CONTEXT.H"
{ echo '#include "context.h"'; cat "$out/$name.c"; } > "$out/build/S.C"
rm -f "$out/build/S.OBJ"
"$T/wcc.sh" "$out/build" S.C -bt=dos -3r -s -ox -fo=S.OBJ >/dev/null || { cat "$out/build/WCC.OUT"; exit 1; }
echo "standalone: $("$py" "$T/fndiff.py" rrr "$out/build/S.OBJ" "$name" "$addr" | tail -1)"
echo "in module:  $("$py" "$T/inplace.py" "$name" "$addr" "$out/$name.c" | sed 's/^[^ ]* *//')"
"$py" "$T/scratch.py" rrr "$name" "$addr" "$out/build/S.OBJ" "$out"
