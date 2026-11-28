#!/usr/bin/env bash
# Health gate for overnight passes. Checks a committed revision in a throwaway worktree (with the
# untracked ISO and .work/ symlinked in), so the result reflects what is in git.
# Usage: overnight/check.sh [rev]. Exit 0 = healthy; output on failure is fed back to the agent.
set -uo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
REV=${1:-HEAD}
WT=$(mktemp -d "${TMPDIR:-/tmp}/redcatcheck.XXXXXX")
trap 'git -C "$ROOT" worktree remove --force "$WT" >/dev/null 2>&1; rm -rf "$WT"' EXIT
git -C "$ROOT" worktree add --detach -q "$WT" "$REV" || { echo "FAIL: cannot check out $REV"; exit 1; }
ln -s "$ROOT/.work" "$WT/.work"
ln -s "$ROOT/Redcat Razende Rekenrace.iso" "$WT/Redcat Razende Rekenrace.iso"
cd "$WT"
fail=0
bad() { echo "FAIL: $*"; fail=1; }

# 1. the original game files are untouched (the ISO and its extracted programs)
(cd "$ROOT" && sha256sum --quiet -c overnight/original.sha256) \
  || bad "an original game file was modified (re-extract it from the ISO: AGENTS.md Setup; never edit them)"

# 2. every tool compiles
for f in $(find reverse-engineering/tools -name '*.py' -not -path '*/__pycache__/*'); do
  python3 -m py_compile "$f" 2>&1 || bad "$f does not compile"
done

# 3. the matching rebuild: src/ compiles with the game's options, keeps its byte-identical
#    functions and data, and every fixup target agrees with the game's.
#    Raise src/matched.baseline / src/data.baseline in the same commit when they go up.
out=$(src/check.sh 2>&1); rc=$?
[ $rc -eq 0 ] || { bad "src/check.sh fails:"; echo "$out" | grep -iE 'error' | head -15; }
m=$(echo "$out" | grep -c ' MATCH at')
b=$(cat src/matched.baseline 2>/dev/null || echo 0)
[ "$m" -ge "$b" ] || bad "src/ now matches only $m functions, src/matched.baseline is $b (a regression): re-introduce what broke"
d=$(echo "$out" | sed -n 's/^DATA: \([0-9]*\) of.*/\1/p' | tail -1)
db=$(cat src/data.baseline 2>/dev/null || echo 0)
[ "${d:-0}" -ge "$db" ] || bad "DATA is ${d:-?} bytes, src/data.baseline is $db (a regression)"
w=$(echo "$out" | grep -c 'WRONG ')
[ "$w" -eq 0 ] || { bad "$w fixup targets disagree with the game (a MATCH whose fields are swapped is not a match):"; echo "$out" | grep 'WRONG ' | head -10; }

# 4. the human's files and STATUS stay in shape
[ -f QUEUE.md ] || bad "QUEUE.md missing"
[ -f STATUS.md ] || bad "STATUS.md missing"
n=$(wc -l < STATUS.md 2>/dev/null || echo 999)
[ "$n" -le 260 ] || bad "STATUS.md has $n lines (limit 260: history belongs in git and research/, not STATUS)"

# 5. no big binaries committed (the ISO and .work/ stay out of git)
big=$(git ls-files -z | xargs -0 -r du -k 2>/dev/null | awk '$1>4096{print $2}' | grep -v '^docs/media/')
[ -z "$big" ] || bad "files over 4 MB committed: $big"

# 6. the port builds
if [ -f port/CMakeLists.txt ]; then
  out=$( { cmake -S port -B "$WT/_portbuild" -DCMAKE_BUILD_TYPE=Release && cmake --build "$WT/_portbuild" -j; } 2>&1) \
    || { bad "port cmake build fails:"; echo "$out" | tail -20; }
fi

[ $fail -eq 0 ] && echo "OK: $REV healthy ($m matched functions, DATA ${d:-?})"
exit $fail
