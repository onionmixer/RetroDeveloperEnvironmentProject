#!/usr/bin/env bash
#
# compile.sh - MSX-DOS2 program build/test using z88dk
#
# Usage:
#   ./compile.sh all
#   ./compile.sh clean|build|disk|test|run
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
RDEDISKTOOL="$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool"
BOOT_DISK="$PROJECT_ROOT/diskwork/bootdisk/msx/msxdos23.dsk"
OUTPUT_NAME="HELLO"
DISK_IMAGE="$BUILD_DIR/Tutorial_msx_z88dk_01.dsk"

# Prefer fixed z88dk path used by this repository.
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

require_file() {
    local p="$1"
    local msg="$2"
    if [[ ! -e "$p" ]]; then
        echo "Error: $msg ($p)" >&2
        exit 1
    fi
}

resolve_com_path() {
    if [[ -f "$BUILD_DIR/${OUTPUT_NAME}" ]]; then
        printf '%s\n' "$BUILD_DIR/${OUTPUT_NAME}"
        return 0
    fi
    if [[ -f "$BUILD_DIR/${OUTPUT_NAME}.COM" ]]; then
        printf '%s\n' "$BUILD_DIR/${OUTPUT_NAME}.COM"
        return 0
    fi
    if [[ -f "$BUILD_DIR/${OUTPUT_NAME}.com" ]]; then
        printf '%s\n' "$BUILD_DIR/${OUTPUT_NAME}.com"
        return 0
    fi
    return 1
}

clean() {
    print_header "Cleaning build directory"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    echo "Done."
}

build() {
    print_header "Building with z88dk"

    if [[ -z "${ZCC}" || ! -x "${ZCC}" ]]; then
        echo "Error: zcc not found. Install z88dk or set /opt/z88dk/bin/zcc." >&2
        exit 1
    fi

    mkdir -p "$BUILD_DIR"

    "${ZCC}" +msx -subtype=msxdos2 -compiler=sccz80 -SO2 -create-app \
        -o "$BUILD_DIR/${OUTPUT_NAME}.COM" "$SCRIPT_DIR/main.c"

    local com_path
    com_path="$(resolve_com_path)" || {
        echo "Error: build succeeded but ${OUTPUT_NAME}.COM not found in $BUILD_DIR" >&2
        exit 1
    }

    echo "Built: $com_path ($(stat -c%s "$com_path") bytes)"
}

create_disk() {
    print_header "Creating tutorial disk image"

    require_file "$RDEDISKTOOL" "rdedisktool not found"

    local com_path
    com_path="$(resolve_com_path)" || {
        echo "Error: ${OUTPUT_NAME}.COM not found. Run './compile.sh build' first." >&2
        exit 1
    }

    "$RDEDISKTOOL" create "$DISK_IMAGE" -f msxdsk --fs msxdos --force
    "$RDEDISKTOOL" add "$DISK_IMAGE" "$com_path" "${OUTPUT_NAME}.COM"

    # Keep compatibility with previous tutorial layout.
    cp -f "$com_path" "$SCRIPT_DIR/${OUTPUT_NAME}.COM"
    cp -f "$DISK_IMAGE" "$SCRIPT_DIR/Tutorial_msx_z88dk_01.dsk"

    echo "Disk image: $DISK_IMAGE"
}

run_tests() {
    print_header "Verifying output image"

    require_file "$RDEDISKTOOL" "rdedisktool not found"
    require_file "$DISK_IMAGE" "disk image not found"

    "$RDEDISKTOOL" info "$DISK_IMAGE"
    "$RDEDISKTOOL" list "$DISK_IMAGE"
    "$RDEDISKTOOL" validate "$DISK_IMAGE"

    echo "Image verification passed."
}

run_emulator() {
    print_header "Launching openMSX (MSX-DOS2 boot)"

    local run_script
    run_script="$PROJECT_ROOT/run_openmsx_msxdos2.sh"

    require_file "$run_script" "run_openmsx_msxdos2.sh not found"
    require_file "$BOOT_DISK" "MSX-DOS2 boot disk not found"
    require_file "$DISK_IMAGE" "tutorial disk image not found"

    echo "After boot, open OSD menu (F9) and change Disk A to:"
    echo "  $DISK_IMAGE"
    echo "Then run:"
    echo "  DIR"
    echo "  HELLO"

    exec "$run_script"
}

usage() {
    cat <<USAGE
Usage: $0 [command]

Commands:
  clean   - Remove build outputs
  build   - Build HELLO.COM with z88dk
  disk    - Create MSX-DOS disk image and add HELLO.COM
  test    - Verify disk image (info/list/validate)
  run     - Launch openMSX with boot disk + tutorial disk on B:
  all     - clean + build + disk + test
USAGE
}

case "${1:-all}" in
    clean)
        clean
        ;;
    build)
        build
        ;;
    disk)
        create_disk
        ;;
    test)
        run_tests
        ;;
    run)
        run_emulator
        ;;
    all)
        clean
        build
        create_disk
        run_tests
        ;;
    *)
        usage
        exit 1
        ;;
esac
