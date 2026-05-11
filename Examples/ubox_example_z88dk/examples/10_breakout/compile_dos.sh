#!/usr/bin/env bash
# 10_breakout — MSX-DOS2 .COM build (z88dk + ubox + spman)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build_dos"
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

ZCCFLAGS="+msx -subtype=msxdos2 -compiler=sccz80 -SO2 -create-app"
ZCCFLAGS="$ZCCFLAGS -I$UBOX_INC -I$SPMAN_INC -I$SCRIPT_DIR/src"
ZCCFLAGS="$ZCCFLAGS -L$UBOX_LIB -lubox"
ZCCFLAGS="$ZCCFLAGS -DMSXDOS"

SRCS="src/main_dos.c src/game.c src/util.c $SPMAN_SRC"

case "${1:-build}" in
    build|all)
        mkdir -p "$BUILD_DIR"
        echo "[DOS] zcc $ZCCFLAGS -m -o build_dos/$TARGET_BASE $SRCS"
        (cd "$SCRIPT_DIR" && "$ZCC" $ZCCFLAGS -m -o "$BUILD_DIR/$TARGET_BASE" $SRCS)
        COM=""
        [[ -f "$BUILD_DIR/${TARGET_BASE}.COM" ]] && COM="$BUILD_DIR/${TARGET_BASE}.COM"
        [[ -z "$COM" && -f "$BUILD_DIR/$TARGET_BASE" ]] && COM="$BUILD_DIR/$TARGET_BASE"
        [[ -z "$COM" ]] && { echo "Error: COM not produced under $BUILD_DIR" >&2; exit 1; }
        echo "COM: $COM ($(wc -c < "$COM") bytes)"
        ;;
    clean) rm -rf "$BUILD_DIR"; echo "Cleaned: $BUILD_DIR" ;;
    *) echo "usage: $0 {build|clean|all}" >&2; exit 2 ;;
esac
