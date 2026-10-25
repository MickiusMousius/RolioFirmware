#!/bin/bash
# A simple script to run clang-format over the RolioFirmware source code

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
FIRMWARE_DIR="$(dirname "$SCRIPT_DIR")"

echo "Formatting source files in $FIRMWARE_DIR/boards..."

# Find all .c and .h files in boards/ and format them in-place
#     Generated image, animation & font data is skipped as it's overwritten whenever it is regenerated
find "$FIRMWARE_DIR/boards" -type f \( -name "*.c" -o -name "*.h" \) \
    -not -path "*/assets/*" -not -path "*/widgets/art.c" \
    -exec clang-format --style="{IndentWidth: 4, ColumnLimit: 120, MaxEmptyLinesToKeep: 2}" -i {} +

echo "Formatting complete!"
