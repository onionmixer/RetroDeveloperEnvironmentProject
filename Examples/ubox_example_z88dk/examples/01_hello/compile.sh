#!/usr/bin/env bash
# 01_hello — ROM cartridge build (z88dk + ubox-msx-lib-z88dk)
# Pattern: prototype_05_MSX_ROM_MSXDOS/compile.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
TARGET_BASE="01_hello"

ZCC="${ZCC:-/opt/z88dk/bin/zcc}"
UBOX_DIR="$PROJECT_ROOT/Library/MSX/ubox-msx-lib-z88dk"
UBOX_INC="$UBOX_DIR/include"
UBOX_LIB="$UBOX_DIR/lib"

[[ -x "$ZCC" ]] || { echo "Error: zcc not found at $ZCC" >&2; exit 1; }

# Auto-build ubox.lib if missing
if [[ ! -f "$UBOX_LIB/ubox.lib" ]]; then
    echo "ubox.lib missing — building..."
    make -C "$UBOX_DIR"
fi

# Canonical ROM ZCCFLAGS (32 KiB plain cartridge — Konami mapper not needed)
ZCCFLAGS="+msx -subtype=rom -compiler=sccz80 -SO2 -create-app"
ZCCFLAGS="$ZCCFLAGS -I$UBOX_INC -I$SCRIPT_DIR/src"
ZCCFLAGS="$ZCCFLAGS -L$UBOX_LIB -lubox"

SRCS="src/main.c"

case "${1:-build}" in
    build|all)
        mkdir -p "$BUILD_DIR"
        echo "[ROM] zcc $ZCCFLAGS -m -o build/$TARGET_BASE $SRCS"
        (cd "$SCRIPT_DIR" && "$ZCC" $ZCCFLAGS -m -o "$BUILD_DIR/$TARGET_BASE" $SRCS)

        ROM="$BUILD_DIR/${TARGET_BASE}.rom"
        [[ -f "$ROM" ]] || { echo "Error: ROM not produced: $ROM" >&2; exit 1; }
        SIZE="$(wc -c < "$ROM")"
        echo "ROM: $ROM ($SIZE bytes)"
        # MSX plain cartridges are 16 KiB or 32 KiB; multi-bank (Konami/ASCII) >= 64 KiB.
        # z88dk picks the smallest power-of-2*16K that fits.
        case "$SIZE" in
            16384|32768) echo "  -> $((SIZE / 1024)) KiB plain cartridge" ;;
            *)           echo "Warning: unusual ROM size ($SIZE bytes); expected 16384 or 32768" >&2 ;;
        esac
        ;;
    clean)
        rm -rf "$BUILD_DIR"
        echo "Cleaned: $BUILD_DIR"
        ;;
    *)
        echo "usage: $0 {build|clean|all}" >&2
        exit 2
        ;;
esac
