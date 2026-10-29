#!/bin/sh
# Serve port/web/out at http://localhost:8080/rcview.html
cd "$(dirname "$0")/out" && exec python3 -m http.server "${1:-8080}" --bind 127.0.0.1
