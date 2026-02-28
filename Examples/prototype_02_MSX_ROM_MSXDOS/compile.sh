#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
TARGET_BASE="PROTO02"

# z88dk
if [[ -x "/opt/z88dk/bin/zcc" ]]; then
    ZCC="/opt/z88dk/bin/zcc"
else
    ZCC="$(command -v zcc 2>/dev/null || true)"
fi

# ubox library
UBOX_DIR="$PROJECT_ROOT/Library/MSX/ubox-msx-lib-z88dk"
UBOX_INC="$UBOX_DIR/include"
UBOX_LIB="$UBOX_DIR/lib"

ZCCFLAGS="+msx -subtype=rom -compiler=sccz80 -SO2 -create-app"
ZCCFLAGS="$ZCCFLAGS -I$UBOX_INC -I$SCRIPT_DIR/src"
ZCCFLAGS="$ZCCFLAGS -L$UBOX_LIB -lubox"

SRCS="src/main.c src/logic.c src/render.c src/input.c src/help.c src/room_data.c src/monster.c"

check_z88dk() {
    if [[ -z "$ZCC" || ! -x "$ZCC" ]]; then
        echo "ERROR: zcc not found. Install z88dk or set PATH."
        exit 1
    fi
}

check_ubox() {
    if [[ ! -f "$UBOX_LIB/ubox.lib" ]]; then
        echo "Building ubox-msx-lib-z88dk..."
        make -C "$UBOX_DIR"
    fi
}

build() {
    mkdir -p "$BUILD_DIR"
    (cd "$SCRIPT_DIR" && "$ZCC" $ZCCFLAGS -o "$BUILD_DIR/$TARGET_BASE" $SRCS)
    local ROM="$BUILD_DIR/$TARGET_BASE.rom"
    if [[ -f "$ROM" ]]; then
        echo "Built: $ROM ($(stat -c%s "$ROM") bytes)"
    else
        echo "ERROR: build failed"
        exit 1
    fi
}

verify() {
    local ROM="$BUILD_DIR/$TARGET_BASE.rom"
    if [[ ! -f "$ROM" ]]; then
        echo "ERROR: ROM not found. Run build first."
        exit 1
    fi
    local sig
    sig="$(xxd -p -l 2 "$ROM")"
    if [[ "$sig" != "4142" ]]; then
        echo "ERROR: invalid ROM signature. expected AB, got $sig"
        exit 1
    fi
    local size
    size="$(stat -c%s "$ROM")"
    echo "ROM signature OK: AB"
    echo "ROM size: $size bytes ($(( size / 1024 ))KB)"
}

clean() {
    rm -rf "$BUILD_DIR"
}

usage() {
    echo "Usage: $0 {clean|build|verify|all}"
}

case "${1:-all}" in
    clean) clean ;;
    build) check_z88dk; check_ubox; build ;;
    verify) verify ;;
    all)   clean; check_z88dk; check_ubox; build; verify ;;
    *)     usage; exit 1 ;;
esac
