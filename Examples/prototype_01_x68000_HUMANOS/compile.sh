#!/usr/bin/env bash
# prototype_01_x68000_HUMANOS build helper

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
TOOLCHAIN_DIR="$PROJECT_ROOT/Toolchain/x68000/toolchain/m68k-xelf"
CC="$TOOLCHAIN_DIR/bin/m68k-xelf-gcc"
RUN68="$TOOLCHAIN_DIR/bin/run68"
OUTPUT_X="$BUILD_DIR/prototype_01.x"
DISK_IMAGE="$BUILD_DIR/prototype_01_x68000_HUMANOS_FDD1.xdf"
RUN_LOG="$BUILD_DIR/run68_output.log"
CHECK_ENV="$SCRIPT_DIR/tools/check_env.sh"
RDEDISKTOOL="${RDEDISKTOOL:-}"
TARGET_NAME="${TARGET_NAME:-PROTO01.X}"
HELP_FILE="${HELP_FILE:-$SCRIPT_DIR/HELP.TXT}"
HELP_NAME="${HELP_NAME:-HELP.TXT}"
EXTRA_CFLAGS="${CFLAGS:-}"
export PATH="$TOOLCHAIN_DIR/bin:$PATH"

print_header() {
    echo "========================================"
    echo "$1"
    echo "========================================"
}

require_exec() {
    local p="$1"
    local name="$2"
    if [[ ! -x "$p" ]]; then
        echo "Error: $name not found/executable: $p" >&2
        exit 1
    fi
}

first_existing_exec() {
    local p
    for p in "$@"; do
        if [[ -x "$p" ]]; then
            printf '%s\n' "$p"
            return 0
        fi
    done
    return 1
}

check_env() {
    if [[ -x "$CHECK_ENV" ]]; then
        "$CHECK_ENV"
    fi
}

clean() {
    print_header "Cleaning"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    rm -f "$SCRIPT_DIR/prototype_01.x" "$SCRIPT_DIR/PROTO01.X"
}

generate_data() {
    print_header "Generating room data"
    python3 "$SCRIPT_DIR/tools/json_to_room_data_x68000.py" --output-dir "$SCRIPT_DIR/src"
}

build() {
    print_header "Building prototype_01.x"

    check_env
    require_exec "$CC" "m68k-xelf-gcc"
    mkdir -p "$BUILD_DIR"

    "$CC" --version | sed -n '1,1p'

    generate_data

    "$CC" -O2 -Wall -m68000 $EXTRA_CFLAGS -o "$OUTPUT_X" \
        "$SCRIPT_DIR/src/main.c" \
        "$SCRIPT_DIR/src/logic.c" \
        "$SCRIPT_DIR/src/render.c" \
        "$SCRIPT_DIR/src/room_data.c"

    if [[ ! -f "$OUTPUT_X" ]]; then
        echo "Error: output not found: $OUTPUT_X" >&2
        exit 1
    fi

    cp -f "$OUTPUT_X" "$SCRIPT_DIR/prototype_01.x"
    cp -f "$OUTPUT_X" "$SCRIPT_DIR/PROTO01.X"

    echo "Built: $OUTPUT_X ($(stat -c%s "$OUTPUT_X") bytes)"
}

resolve_rdedisktool() {
    if [[ -z "$RDEDISKTOOL" ]]; then
        RDEDISKTOOL="$(first_existing_exec \
            "$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool" \
            "$(command -v rdedisktool 2>/dev/null || true)" \
        )" || RDEDISKTOOL="$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool"
    fi
}

prepare_disk() {
    print_header "Preparing Human68k program disk image"

    check_env
    resolve_rdedisktool

    if [[ ! -x "$RDEDISKTOOL" ]]; then
        echo "Error: rdedisktool not executable at $RDEDISKTOOL" >&2
        exit 1
    fi
    if [[ ! -f "$OUTPUT_X" ]]; then
        echo "Error: binary not found. Run './compile.sh build' first." >&2
        exit 1
    fi

    mkdir -p "$BUILD_DIR"

    "$RDEDISKTOOL" create "$DISK_IMAGE" -f xdf --fs human68k --force
    if ! "$RDEDISKTOOL" info "$DISK_IMAGE" | rg -q "File System: Human68k"; then
        echo "Error: created image is not recognized as Human68k." >&2
        "$RDEDISKTOOL" info "$DISK_IMAGE" || true
        exit 1
    fi
    "$RDEDISKTOOL" validate "$DISK_IMAGE"

    "$RDEDISKTOOL" --bootdisk-mode off delete "$DISK_IMAGE" "$TARGET_NAME" >/dev/null 2>&1 || true
    "$RDEDISKTOOL" --bootdisk-mode strict add "$DISK_IMAGE" "$OUTPUT_X" "$TARGET_NAME"

    if [[ -f "$HELP_FILE" ]]; then
        "$RDEDISKTOOL" --bootdisk-mode off delete "$DISK_IMAGE" "$HELP_NAME" >/dev/null 2>&1 || true
        "$RDEDISKTOOL" --bootdisk-mode off add "$DISK_IMAGE" "$HELP_FILE" "$HELP_NAME"
    fi

    echo "Disk image ready: $DISK_IMAGE"
    "$RDEDISKTOOL" list "$DISK_IMAGE" | rg -i "$TARGET_NAME|$HELP_NAME|Name|Directory listing" || true
}

test_run68() {
    print_header "Testing with run68"

    check_env
    require_exec "$RUN68" "run68"
    if [[ ! -f "$OUTPUT_X" ]]; then
        echo "Error: binary not found. Run './compile.sh build' first." >&2
        exit 1
    fi

    (
        cd "$BUILD_DIR"
        printf 'q\n1\n' | "$RUN68" ./prototype_01.x
    ) > "$RUN_LOG" 2>&1

    if ! rg -q "Prototype 01 X68000" "$RUN_LOG"; then
        echo "Error: expected output not found in run68 log" >&2
        sed -n '1,120p' "$RUN_LOG" >&2
        exit 1
    fi

    echo "run68 output check passed."
    sed -n '1,40p' "$RUN_LOG"
}

validate_data() {
    print_header "Validating room data"
    python3 "$SCRIPT_DIR/tools/json_to_room_data_x68000.py" --validate
    python3 "$SCRIPT_DIR/tests/host/test_room_links.py"

    gcc -Wall -Wextra -std=c99 -o "$BUILD_DIR/test_camera" \
        "$SCRIPT_DIR/tests/host/test_camera.c"
    "$BUILD_DIR/test_camera"

    gcc -Wall -Wextra -std=c99 -I"$SCRIPT_DIR/src" \
        -o "$BUILD_DIR/test_logic_core" \
        "$SCRIPT_DIR/tests/host/test_logic_core.c" \
        "$SCRIPT_DIR/src/logic.c" \
        "$SCRIPT_DIR/src/room_data.c"
    "$BUILD_DIR/test_logic_core"
}

run_gui() {
    print_header "Launching px68k with prepared FDD1 image"

    check_env
    if [[ ! -f "$DISK_IMAGE" ]]; then
        echo "Error: disk image not found. Run './compile.sh disk' first." >&2
        exit 1
    fi

    exec env AUTO_BUILD=0 PROGRAM_DISK="$DISK_IMAGE" \
        "$SCRIPT_DIR/run_px68k_humanos_fdd1_diskadd.sh"
}

test_fdd1_diskadd() {
    print_header "Verifying prepared FDD1 disk image"

    check_env
    resolve_rdedisktool
    if [[ ! -x "$RDEDISKTOOL" ]]; then
        echo "Error: rdedisktool not executable at $RDEDISKTOOL" >&2
        exit 1
    fi
    if [[ ! -f "$DISK_IMAGE" ]]; then
        echo "Error: disk image not found. Run './compile.sh disk' first." >&2
        exit 1
    fi

    if ! "$RDEDISKTOOL" info "$DISK_IMAGE" | rg -q "File System: Human68k"; then
        echo "Error: disk image is not Human68k." >&2
        exit 1
    fi
    "$RDEDISKTOOL" validate "$DISK_IMAGE"

    if ! "$RDEDISKTOOL" list "$DISK_IMAGE" | rg -qi "$TARGET_NAME"; then
        echo "Error: target binary not found on disk image: $TARGET_NAME" >&2
        "$RDEDISKTOOL" list "$DISK_IMAGE" >&2 || true
        exit 1
    fi

    echo "FDD1 prepared disk verification passed."
    "$RDEDISKTOOL" list "$DISK_IMAGE" | rg -i "$TARGET_NAME|$HELP_NAME|Name|Directory listing" || true
}

usage() {
    cat <<USAGE
Usage: $0 [command]

Commands:
  clean   - Remove build outputs
  build   - Generate data and build prototype_01.x
  disk    - Create Human68k disk image and add program
  test    - Run host validation + run68 smoke test
  disktest- Verify prepared FDD1 image via rdedisktool
  run     - Launch px68k with prepared FDD1 image
  all     - clean + build + disk + test + disktest
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
        prepare_disk
        ;;
    test)
        validate_data
        test_run68
        ;;
    disktest)
        test_fdd1_diskadd
        ;;
    run)
        run_gui
        ;;
    all)
        clean
        build
        prepare_disk
        validate_data
        test_run68
        test_fdd1_diskadd
        ;;
    *)
        usage
        exit 1
        ;;
esac
