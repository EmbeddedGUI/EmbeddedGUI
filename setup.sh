#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SETUP_SCRIPT="$SCRIPT_DIR/scripts/setup_env.py"

if [ ! -f "$SETUP_SCRIPT" ]; then
    echo "[!!] Setup script not found: $SETUP_SCRIPT" >&2
    exit 1
fi

if command -v python3 >/dev/null 2>&1; then
    PYTHON_BIN=python3
elif command -v python >/dev/null 2>&1; then
    PYTHON_BIN=python
else
    echo "[!!] Python 3.8+ was not found." >&2
    echo "     Install Python, then run setup.sh again." >&2
    exit 1
fi

exec "$PYTHON_BIN" "$SETUP_SCRIPT" "$@"
