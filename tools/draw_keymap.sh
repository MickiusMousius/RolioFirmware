#!/bin/bash
# Redraws the keymap diagram (doc/images/keymap.svg) from the Rolio46 keymap using keymap-drawer
#     The first run sets up a Python virtual environment in tools/venv with keymap-drawer installed
#     Any arguments are passed on to draw_keymap.py (e.g. --output some_other.svg)

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
VENV_DIR="$SCRIPT_DIR/venv"
KEYMAP_DRAWER_VERSION="0.23.0"

if [ ! -x "$VENV_DIR/bin/python" ]; then
    echo "⚙️  Creating Python virtual environment in $VENV_DIR..."
    python3 -m venv "$VENV_DIR" || exit 1
fi

if ! "$VENV_DIR/bin/python" -m pip show keymap-drawer 2>/dev/null | grep -q "^Version: $KEYMAP_DRAWER_VERSION$"; then
    echo "📦 Installing keymap-drawer $KEYMAP_DRAWER_VERSION..."
    "$VENV_DIR/bin/python" -m pip install -q "keymap-drawer==$KEYMAP_DRAWER_VERSION" || exit 1
fi

"$VENV_DIR/bin/python" "$SCRIPT_DIR/draw_keymap.py" "$@"
