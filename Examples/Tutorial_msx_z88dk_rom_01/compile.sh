#!/usr/bin/env bash
#
# compile.sh - z88dk MSX ROM tutorial
#
# This tutorial is a standalone z88dk ROM flow.
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
TARGET_BASE="HELLO_ROM_Z88DK"

if [[ -x "/opt/z88dk/bin/zcc" ]]; then
    ZCC="/opt/z88dk/bin/zcc"
else
    ZCC="$(command -v zcc 2>/dev/null || true)"
fi

print_header() {
    echo "========================================"
    echo "$1"
    echo "========================================"
}

require_exec() {
    local p="$1"
    local name="$2"
    if [[ -z "$p" || ! -x "$p" ]]; then
        echo "Error: $name not found: $p" >&2
        exit 1
    fi
}

clean() {
    print_header "Cleaning build artifacts"
    rm -rf "$BUILD_DIR"
    rm -f "$SCRIPT_DIR/${TARGET_BASE}" "$SCRIPT_DIR/${TARGET_BASE}.rom" \
          "$SCRIPT_DIR/${TARGET_BASE}_BSS.bin" "$SCRIPT_DIR/${TARGET_BASE}_DATA.bin"
    mkdir -p "$BUILD_DIR"
}

build() {
    print_header "Building MSX ROM with z88dk"
    require_exec "$ZCC" "zcc"
    mkdir -p "$BUILD_DIR"

    "$ZCC" +msx -subtype=rom -compiler=sccz80 -SO2 -create-app \
        -o "$BUILD_DIR/$TARGET_BASE" "$SCRIPT_DIR/main.c"

    cp -f "$BUILD_DIR/$TARGET_BASE" "$SCRIPT_DIR/$TARGET_BASE"
    cp -f "$BUILD_DIR/$TARGET_BASE.rom" "$SCRIPT_DIR/$TARGET_BASE.rom"
    cp -f "$BUILD_DIR/${TARGET_BASE}_BSS.bin" "$SCRIPT_DIR/${TARGET_BASE}_BSS.bin"
    cp -f "$BUILD_DIR/${TARGET_BASE}_DATA.bin" "$SCRIPT_DIR/${TARGET_BASE}_DATA.bin"

    echo "Built: $BUILD_DIR/$TARGET_BASE.rom ($(stat -c%s "$BUILD_DIR/$TARGET_BASE.rom") bytes)"
}

verify() {
    print_header "Verifying ROM image"

    local rom="$BUILD_DIR/$TARGET_BASE.rom"
    if [[ ! -f "$rom" ]]; then
        echo "Error: ROM not found. Run build first." >&2
        exit 1
    fi

    # MSX cartridge signature 'AB' (0x41, 0x42)
    local sig
    sig="$(xxd -p -l 2 "$rom")"
    if [[ "$sig" != "4142" ]]; then
        echo "Error: invalid ROM signature. expected AB, got $sig" >&2
        exit 1
    fi

    local size
    size="$(stat -c%s "$rom")"
    if (( size % 16384 != 0 )); then
        echo "Error: ROM size is not 16KB aligned: $size" >&2
        exit 1
    fi

    echo "ROM signature OK: AB"
    echo "ROM size OK (16KB aligned): $size"
    xxd -l 32 "$rom" | sed -n '1,2p'
}

usage() {
    cat <<USAGE
Usage: $0 [clean|build|verify|all]

  clean   Remove build outputs
  build   Build ROM image using z88dk (+msx -subtype=rom)
  verify  Verify AB signature and bank alignment
  all     clean + build + verify
USAGE
}

case "${1:-all}" in
    clean)
        clean
        ;;
    build)
        build
        ;;
    verify)
        verify
        ;;
    all)
        clean
        build
        verify
        ;;
    *)
        usage
        exit 1
        ;;
esac
