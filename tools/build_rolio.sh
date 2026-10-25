#!/bin/bash

# ==========================================
# Argument Parsing
# ==========================================
BUILD_LEFT=true
BUILD_RIGHT=true
LOGGING_SNIPPET=""
BOARD="nice_nano_v2"
DISPLAY="vista508"

usage() {
    echo "Usage: $0 [left | right | all] [usb_logging] [--board nice_nano_v2|mikoto@7.2.0] [--display vista508|nice_view]"
}

while [ $# -gt 0 ]; do
    case "$1" in
        left)
            BUILD_RIGHT=false
            echo "⚙️  Target selected: LEFT only"
            ;;
        right)
            BUILD_LEFT=false
            echo "⚙️  Target selected: RIGHT only"
            ;;
        both|all)
            echo "⚙️  Target selected: BOTH sides"
            ;;
        usb_logging|--usb-logging)
            LOGGING_SNIPPET="-S zmk-usb-logging"
            echo "🐛 USB Logging ENABLED"
            ;;
        --board)
            shift
            case "$1" in
                nice_nano_v2|mikoto@7.2.0)
                    BOARD="$1"
                    ;;
                *)
                    echo "❌ Invalid board: $1"
                    usage
                    exit 1
                    ;;
            esac
            ;;
        --display)
            shift
            case "$1" in
                vista508|nice_view)
                    DISPLAY="$1"
                    ;;
                *)
                    echo "❌ Invalid display: $1"
                    usage
                    exit 1
                    ;;
            esac
            ;;
        *)
            echo "❌ Invalid argument: $1"
            usage
            exit 1
            ;;
    esac
    shift
done

echo "⚙️  Board: ${BOARD}, Display: ${DISPLAY}"

# ==========================================
# Define paths
# ==========================================
ZMK_APP_DIR="/workspaces/zmk/app"
BASE_DIR="/workspaces/zmk-modules/RolioFirmware"
CONFIG_DIR="${BASE_DIR}/config"
BUILD_NAME="${BOARD%@*}-${DISPLAY}"
BUILD_DIR="${BASE_DIR}/build/zmk/${BUILD_NAME}"
OUTPUT_DIR="${BASE_DIR}/build/zmk"

# The nice!view uses the built in ZMK status screen, configured by an extra conf file
EXTRA_CMAKE_ARGS=()
if [ "$DISPLAY" = "nice_view" ]; then
    EXTRA_CMAKE_ARGS+=("-DEXTRA_CONF_FILE=${CONFIG_DIR}/rolio-nice_view.conf")
fi

# ==========================================
# Module Definitions
# ==========================================

# Modules used by both halves
SHARED_MODULES=(
    "/workspaces/zmk-modules/RolioFirmware"
    "/workspaces/zmk-modules/zmk-ls0xxvcom-driver"
    "/workspaces/zmk-modules/zmk-defaultlayer"
    "/workspaces/zmk-modules/zmk-patch-batterylevel"
    "/workspaces/zmk-modules/zmk-patch-ec11"
    "/workspaces/zmk-modules/zmk-smart-sleep"
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
    west build -s "${ZMK_APP_DIR}" -d "${BUILD_DIR}/left" -p -b "${BOARD}" -S studio-rpc-usb-uart $LOGGING_SNIPPET -- \
        -DZMK_EXTRA_MODULES="${LEFT_JOINED}" \
        -DZMK_CONFIG="${CONFIG_DIR}" \
        -DSHIELD="rolio_left ${DISPLAY}" \
        "${EXTRA_CMAKE_ARGS[@]}"

    # Copy left firmware if successful
    if [ -f "${BUILD_DIR}/left/zephyr/zmk.uf2" ]; then
        cp "${BUILD_DIR}/left/zephyr/zmk.uf2" "${OUTPUT_DIR}/rolio_${BUILD_NAME}_left.uf2"
        echo -e "✅ Left firmware copied to: ${OUTPUT_DIR}/rolio_${BUILD_NAME}_left.uf2\n"
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
    west build -s "${ZMK_APP_DIR}" -d "${BUILD_DIR}/right" -p -b "${BOARD}" $LOGGING_SNIPPET -- \
        -DZMK_EXTRA_MODULES="${RIGHT_JOINED}" \
        -DZMK_CONFIG="${CONFIG_DIR}" \
        -DSHIELD="rolio_right ${DISPLAY}" \
        "${EXTRA_CMAKE_ARGS[@]}"

    # Copy right firmware if successful
    if [ -f "${BUILD_DIR}/right/zephyr/zmk.uf2" ]; then
        cp "${BUILD_DIR}/right/zephyr/zmk.uf2" "${OUTPUT_DIR}/rolio_${BUILD_NAME}_right.uf2"
        echo -e "✅ Right firmware copied to: ${OUTPUT_DIR}/rolio_${BUILD_NAME}_right.uf2\n"
    else
        echo -e "❌ Right build failed! UF2 file not found.\n"
        exit 1
    fi
fi

echo "🎉 Build process complete! Check ${OUTPUT_DIR} for your firmware."
