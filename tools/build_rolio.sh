#!/bin/bash

# ==========================================
# Argument Parsing
# ==========================================
BUILD_LEFT=true
BUILD_RIGHT=true

case "$1" in
    left)
        BUILD_RIGHT=false
        echo "⚙️  Target selected: LEFT only"
        ;;
    right)
        BUILD_LEFT=false
        echo "⚙️  Target selected: RIGHT only"
        ;;
    both|all|"")
        echo "⚙️  Target selected: BOTH sides"
        ;;
    *)
        echo "❌ Invalid argument."
        echo "Usage: $0 [left | right | all]"
        exit 1
        ;;
esac

# ==========================================
# Define paths
# ==========================================
ZMK_APP_DIR="/workspaces/zmk/app"
BASE_DIR="/workspaces/zmk-modules/RolioFirmware"
CONFIG_DIR="${BASE_DIR}/config"
BUILD_DIR="${BASE_DIR}/build/zmk"
OUTPUT_DIR="${BUILD_DIR}"

# ==========================================
# Module Definitions
# ==========================================

# Modules used by both halves
SHARED_MODULES=(
    "/workspaces/zmk-modules/RolioFirmware"
    "/workspaces/zmk-modules/zmk-ls0xxvcom-driver"
    "/workspaces/zmk-modules/zmk-userspace"
    "/workspaces/zmk-modules/zmk-patch-batterylevel"
    "/workspaces/zmk-modules/zmk-patch-ec11"

)

# Modules used ONLY by the left half
LEFT_ONLY_MODULES=(
)

# Modules used ONLY by the right half
RIGHT_ONLY_MODULES=(
)

# Combine and format modules for CMake
LEFT_MODULES=("${SHARED_MODULES[@]}" "${LEFT_ONLY_MODULES[@]}")
RIGHT_MODULES=("${SHARED_MODULES[@]}" "${RIGHT_ONLY_MODULES[@]}")

LEFT_JOINED=$(IFS=\;; echo "${LEFT_MODULES[*]}")
RIGHT_JOINED=$(IFS=\;; echo "${RIGHT_MODULES[*]}")

# Ensure the final output directory exists
mkdir -p "${OUTPUT_DIR}"

# ==========================================
# 1. Build Left Side
# ==========================================
if [ "$BUILD_LEFT" = true ]; then
    echo -e "\n🔨 Building Left Side..."
    west build -s "${ZMK_APP_DIR}" -d "${BUILD_DIR}/left" -p -b "nice_nano_v2" -S studio-rpc-usb-uart -- \
        -DZMK_EXTRA_MODULES="${LEFT_JOINED}" \
        -DZMK_CONFIG="${CONFIG_DIR}" \
        -DSHIELD="rolio_left vista508"

    # Copy left firmware if successful
    if [ -f "${BUILD_DIR}/left/zephyr/zmk.uf2" ]; then
        cp "${BUILD_DIR}/left/zephyr/zmk.uf2" "${OUTPUT_DIR}/rolio_left.uf2"
        echo -e "✅ Left firmware copied to: ${OUTPUT_DIR}/rolio_left.uf2\n"
    else
        echo -e "❌ Left build failed! UF2 file not found.\n"
        exit 1
    fi
fi

# ==========================================
# 2. Build Right Side
# ==========================================
if [ "$BUILD_RIGHT" = true ]; then
    echo -e "\n🔨 Building Right Side..."
    west build -s "${ZMK_APP_DIR}" -d "${BUILD_DIR}/right" -p -b "nice_nano_v2" -- \
        -DZMK_EXTRA_MODULES="${RIGHT_JOINED}" \
        -DZMK_CONFIG="${CONFIG_DIR}" \
        -DSHIELD="rolio_right"

    # Copy right firmware if successful
    if [ -f "${BUILD_DIR}/right/zephyr/zmk.uf2" ]; then
        cp "${BUILD_DIR}/right/zephyr/zmk.uf2" "${OUTPUT_DIR}/rolio_right.uf2"
        echo -e "✅ Right firmware copied to: ${OUTPUT_DIR}/rolio_right.uf2\n"
    else
        echo -e "❌ Right build failed! UF2 file not found.\n"
        exit 1
    fi
fi

echo "🎉 Build process complete! Check ${OUTPUT_DIR} for your firmware."