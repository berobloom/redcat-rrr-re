#!/bin/sh
# Install the game for DOSBox-X the way the CD's INSTMAX.BAT does: replay its
# COPY lines from the extracted CD into .work/dos/C/DAVILEX/RCREKEN, then
# write .work/dos/redcat.conf (C: = .work/dos/C, D: = the CD as a CD-ROM).
#
#   reverse-engineering/dosbox/install.sh [extracted CD dir]   (default .work/iso)
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
CD=$(cd "${1:-$ROOT/.work/iso}" && pwd)
DOS="$ROOT/.work/dos"
GAME="$DOS/C/DAVILEX/RCREKEN"
mkdir -p "$GAME"

# COPY <src> C:\DAVILEX\RCREKEN[\<dst>]   (DOS names: match case-insensitively)
tr -d '\r' < "$CD/INSTMAX.BAT" | grep -i '^COPY ' | while read -r _ src dst _; do
    f=$(find "$CD" -maxdepth 1 -iname "$src" | head -n 1)
    [ -n "$f" ] || { echo "missing on CD: $src" >&2; exit 1; }
    name=$(echo "$dst" | sed 's/.*\\//' | tr 'a-z' 'A-Z')
    case "$name" in RCREKEN) name=$(basename "$f" | tr 'a-z' 'A-Z') ;; esac
    cp "$f" "$GAME/$name"
done
# the batch files start with "PAUSE" for the CD; RCREKEN.BAT is kept as is

sed -e "s|@C@|$DOS/C|" -e "s|@CD@|$CD|" "$HERE/redcat.conf.in" > "$DOS/redcat.conf"
echo "installed to $GAME ($(ls "$GAME" | wc -l) files); config $DOS/redcat.conf"
