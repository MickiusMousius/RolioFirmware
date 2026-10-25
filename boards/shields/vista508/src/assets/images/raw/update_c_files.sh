#!/bin/bash

# Get the directory of the script
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Paths
RAW_DIR="$DIR"
C_DIR="$DIR/.."
TOOLS_DIR="$DIR/../../../../../../../tools"
SCRIPT="$TOOLS_DIR/gif_to_lvgl.py"

echo "Updating modifier C files..."

# Ensure target directory exists
mkdir -p "$C_DIR"

for gif_file in "$RAW_DIR"/*.gif; do
    if [ -f "$gif_file" ]; then
        filename=$(basename -- "$gif_file")
        prefix="${filename%.*}"
        c_file="$C_DIR/$prefix.c"
        
        echo "Processing $filename -> $prefix.c"
        # We pass --dual to generate both inverted & non-inverted arrays in the C file
        python3 "$SCRIPT" "$gif_file" "$c_file" --prefix "$prefix" --dual
    fi
done

echo "Done!"
