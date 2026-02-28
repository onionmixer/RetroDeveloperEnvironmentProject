#!/usr/bin/env bash
# prototype_01_MSX_MSXDOS build helper

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
OUTPUT_NAME="HELLO"
DISK_IMAGE="$BUILD_DIR/prototype_01_MSX_MSXDOS.dsk"
RDEDISKTOOL="$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool"

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
    if [[ -f "$BUILD_DIR/${OUTPUT_NAME}.COM" ]]; then
        printf '%s\n' "$BUILD_DIR/${OUTPUT_NAME}.COM"
        return 0
    fi
    if [[ -f "$BUILD_DIR/${OUTPUT_NAME}" ]]; then
        printf '%s\n' "$BUILD_DIR/${OUTPUT_NAME}"
        return 0
    fi
    return 1
}

clean() {
    print_header "Cleaning"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
}

generate_data() {
    print_header "Generating room data"
    python3 "$SCRIPT_DIR/tools/json_to_room_data_msx.py" --output-dir "$SCRIPT_DIR/src"
}

build() {
    print_header "Building with z88dk"

    if [[ -z "$ZCC" || ! -x "$ZCC" ]]; then
        echo "Error: zcc not found" >&2
        exit 1
    fi

    mkdir -p "$BUILD_DIR"
    echo "Using zcc: $ZCC"

    generate_data

    "$ZCC" +msx -subtype=msxdos2 -compiler=sccz80 -SO2 -create-app \
        -o "$BUILD_DIR/${OUTPUT_NAME}.COM" \
        "$SCRIPT_DIR/src/main.c" \
        "$SCRIPT_DIR/src/help.c" \
        "$SCRIPT_DIR/src/logic.c" \
        "$SCRIPT_DIR/src/render.c" \
        "$SCRIPT_DIR/src/room_data.c" \
        "$SCRIPT_DIR/src/monster.c"

    local com_path
    com_path="$(resolve_com_path)"
    cp -f "$com_path" "$SCRIPT_DIR/${OUTPUT_NAME}.COM"
    echo "Built: $com_path ($(stat -c%s "$com_path") bytes)"
}

create_disk() {
    print_header "Creating disk image"
    require_file "$RDEDISKTOOL" "rdedisktool not found"

    local com_path
    com_path="$(resolve_com_path)" || {
        echo "Error: output not found. Run build first." >&2
        exit 1
    }

    "$RDEDISKTOOL" create "$DISK_IMAGE" -f msxdsk --fs msxdos --force
    if ! "$RDEDISKTOOL" info "$DISK_IMAGE" | rg -q "File System: MSX-DOS"; then
        echo "Error: created image is not recognized as MSX-DOS." >&2
        "$RDEDISKTOOL" info "$DISK_IMAGE" || true
        exit 1
    fi
    "$RDEDISKTOOL" add "$DISK_IMAGE" "$com_path" "${OUTPUT_NAME}.COM"
    echo "Disk image: $DISK_IMAGE"
}

run_tests() {
    print_header "Validating generated data"
    python3 "$SCRIPT_DIR/tools/json_to_room_data_msx.py" --validate

    if [[ -x "$RDEDISKTOOL" && -f "$DISK_IMAGE" ]]; then
        print_header "Validating disk image"
        "$RDEDISKTOOL" info "$DISK_IMAGE"
        "$RDEDISKTOOL" list "$DISK_IMAGE"
        "$RDEDISKTOOL" validate "$DISK_IMAGE"
    fi
}

run_emulator() {
    print_header "Launching via diskaddtest script"
    local com_path
    com_path="$(resolve_com_path)" || {
        echo "Error: output not found. Run './compile.sh build' first." >&2
        exit 1
    }

    exec env AUTO_BUILD=0 PROGRAM_FILE="$com_path" \
        "$SCRIPT_DIR/run_openmsx_prototype_01_msx_msxdos_diskaddtest.sh"
}

usage() {
    cat <<USAGE
Usage: $0 [command]

Commands:
  clean   - Remove build outputs
  build   - Generate data and build HELLO.COM
  disk    - Create MSX-DOS disk image and add HELLO.COM
  test    - Run validation checks
  run     - Run emulator with prepared prototype disk image
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
