#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build_dos"
TARGET_BASE="PROTO02"
RDEDISKTOOL="$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool"

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

ZCCFLAGS="+msx -subtype=msxdos2 -compiler=sccz80 -SO2 -create-app"
ZCCFLAGS="$ZCCFLAGS -I$UBOX_INC -I$SCRIPT_DIR/src"
ZCCFLAGS="$ZCCFLAGS -L$UBOX_LIB -lubox"

# Uses main_dos.c and render_dos.c instead of main.c and render.c
SRCS="src/main_dos.c src/logic.c src/render_dos.c src/input.c src/help.c src/room_data.c src/monster.c"

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
    local COM
    if [[ -f "$BUILD_DIR/$TARGET_BASE.COM" ]]; then
        COM="$BUILD_DIR/$TARGET_BASE.COM"
    elif [[ -f "$BUILD_DIR/$TARGET_BASE" ]]; then
        COM="$BUILD_DIR/$TARGET_BASE"
    else
        echo "ERROR: build failed"
        exit 1
    fi
    echo "Built: $COM ($(stat -c%s "$COM") bytes)"
}

resolve_com_path() {
    if [[ -f "$BUILD_DIR/$TARGET_BASE.COM" ]]; then
        printf '%s\n' "$BUILD_DIR/$TARGET_BASE.COM"
        return 0
    fi
    if [[ -f "$BUILD_DIR/$TARGET_BASE" ]]; then
        printf '%s\n' "$BUILD_DIR/$TARGET_BASE"
        return 0
    fi
    return 1
}

disk() {
    if [[ ! -x "$RDEDISKTOOL" ]]; then
        echo "ERROR: rdedisktool not found: $RDEDISKTOOL"
        exit 1
    fi
    local COM
    COM="$(resolve_com_path)" || {
        echo "ERROR: COM not found. Run build first."
        exit 1
    }
    local DSK="$BUILD_DIR/$TARGET_BASE.dsk"
    "$RDEDISKTOOL" create "$DSK" -f msxdsk --fs msxdos --force
    "$RDEDISKTOOL" add "$DSK" "$COM" "${TARGET_BASE}.COM"
    echo "Disk image: $DSK"
}

verify() {
    local COM
    COM="$(resolve_com_path)" || {
        echo "ERROR: COM not found. Run build first."
        exit 1
    }
    local size
    size="$(stat -c%s "$COM")"
    echo "COM size: $size bytes ($(( size / 1024 ))KB)"

    if [[ -x "$RDEDISKTOOL" && -f "$BUILD_DIR/$TARGET_BASE.dsk" ]]; then
        "$RDEDISKTOOL" list "$BUILD_DIR/$TARGET_BASE.dsk"
    fi
}

clean() {
    rm -rf "$BUILD_DIR"
}

usage() {
    echo "Usage: $0 {clean|build|disk|verify|all}"
}

case "${1:-all}" in
    clean)  clean ;;
    build)  check_z88dk; check_ubox; build ;;
    disk)   disk ;;
    verify) verify ;;
    all)    clean; check_z88dk; check_ubox; build; disk; verify ;;
    *)      usage; exit 1 ;;
esac
