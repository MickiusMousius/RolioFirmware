#!/bin/bash
# Generate LVGL C files from raw GIFs

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
TOOLS_DIR="$DIR/../../../../../../tools"

cd "$DIR"

python3 "$TOOLS_DIR/gif_to_lvgl.py" raw/dance.gif dance_animation.c --prefix dance
python3 "$TOOLS_DIR/gif_to_lvgl.py" raw/sleep.gif sleep_animation.c --prefix sleep
python3 "$TOOLS_DIR/gif_to_lvgl.py" raw/typing.gif typing_animation.c --prefix typing
python3 "$TOOLS_DIR/gif_to_lvgl.py" raw/hello.gif hello_animation.c --prefix hello
