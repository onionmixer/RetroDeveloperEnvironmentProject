#!/usr/bin/env bash
# 10_breakout — ROM cartridge build (z88dk + ubox + spman)
# Pattern: prototype_05_MSX_ROM_MSXDOS/compile.sh, with spman source inclusion.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
TARGET_BASE="10_breakout"

ZCC="${ZCC:-/opt/z88dk/bin/zcc}"
UBOX_DIR="$PROJECT_ROOT/Library/MSX/ubox-msx-lib-z88dk"
UBOX_INC="$UBOX_DIR/include"
UBOX_LIB="$UBOX_DIR/lib"
SPMAN_DIR="$PROJECT_ROOT/Library/MSX/spman-z88dk"
SPMAN_INC="$SPMAN_DIR/include"
SPMAN_SRC="$SPMAN_DIR/src/spman/spman.c"

[[ -x "$ZCC" ]] || { echo "Error: zcc not found at $ZCC" >&2; exit 1; }
[[ -f "$UBOX_LIB/ubox.lib" ]] || { echo "ubox.lib missing — building..."; make -C "$UBOX_DIR"; }
[[ -f "$SPMAN_SRC" ]] || { echo "Error: spman source not found at $SPMAN_SRC" >&2; exit 1; }

ZCCFLAGS="+msx -subtype=rom -compiler=sccz80 -SO2 -create-app"
ZCCFLAGS="$ZCCFLAGS -I$UBOX_INC -I$SPMAN_INC -I$SCRIPT_DIR/src"
ZCCFLAGS="$ZCCFLAGS -L$UBOX_LIB -lubox"

# spman is included as direct source (no .lib build — see Library/MSX/spman-z88dk/README.md)
SRCS="src/main.c src/game.c src/util.c $SPMAN_SRC"

case "${1:-build}" in
    build|all)
        mkdir -p "$BUILD_DIR"
        echo "[ROM] zcc $ZCCFLAGS -m -o build/$TARGET_BASE $SRCS"
        (cd "$SCRIPT_DIR" && "$ZCC" $ZCCFLAGS -m -o "$BUILD_DIR/$TARGET_BASE" $SRCS)
        ROM="$BUILD_DIR/${TARGET_BASE}.rom"
        [[ -f "$ROM" ]] || { echo "Error: ROM not produced: $ROM" >&2; exit 1; }
        SIZE="$(wc -c < "$ROM")"
        echo "ROM: $ROM ($SIZE bytes)"
        case "$SIZE" in
            16384|32768) echo "  -> $((SIZE / 1024)) KiB plain cartridge" ;;
            *) echo "Warning: unusual ROM size ($SIZE bytes); expected 16384 or 32768" >&2 ;;
        esac
        ;;
    clean) rm -rf "$BUILD_DIR"; echo "Cleaned: $BUILD_DIR" ;;
    *) echo "usage: $0 {build|clean|all}" >&2; exit 2 ;;
esac
