#!/usr/bin/env bash
#
# compile.sh - X68000 tutorial build/test script
#
# Usage:
#   ./compile.sh all
#   ./compile.sh clean|build|test|run
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
TOOLCHAIN_DIR="$PROJECT_ROOT/Toolchain/x68000/toolchain/m68k-xelf"
CC="$TOOLCHAIN_DIR/bin/m68k-xelf-gcc"
RUN68="$TOOLCHAIN_DIR/bin/run68"
export PATH="$TOOLCHAIN_DIR/bin:$PATH"

OUTPUT_X="$BUILD_DIR/hello.x"
OUTPUT_ELF="$BUILD_DIR/hello.x.elf"
RUN_LOG="$BUILD_DIR/run68_output.log"

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

clean() {
    print_header "Cleaning build directory"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    rm -f "$SCRIPT_DIR/hello.x" "$SCRIPT_DIR/hello.x.elf"
    echo "Done."
}

build() {
    print_header "Building X68000 tutorial binary"

    require_exec "$CC" "m68k-xelf-gcc"
    mkdir -p "$BUILD_DIR"

    "$CC" -O2 -Wall -m68000 -o "$OUTPUT_X" "$SCRIPT_DIR/hello.c"

    if [[ ! -f "$OUTPUT_X" ]]; then
        echo "Error: build output not found: $OUTPUT_X" >&2
        exit 1
    fi

    cp -f "$OUTPUT_X" "$SCRIPT_DIR/hello.x"
    if [[ -f "$OUTPUT_ELF" ]]; then
        cp -f "$OUTPUT_ELF" "$SCRIPT_DIR/hello.x.elf"
    fi

    echo "Built: $OUTPUT_X ($(stat -c%s "$OUTPUT_X") bytes)"
}

test_run68() {
    print_header "Testing with run68"

    require_exec "$RUN68" "run68"
    if [[ ! -f "$OUTPUT_X" ]]; then
        echo "Error: binary not found. Run './compile.sh build' first." >&2
        exit 1
    fi

    (
        cd "$BUILD_DIR"
        "$RUN68" ./hello.x
    ) > "$RUN_LOG" 2>&1

    if ! rg -q "Hello, X68000 Tutorial!" "$RUN_LOG"; then
        echo "Error: expected output not found in run68 log" >&2
        cat "$RUN_LOG" >&2
        exit 1
    fi

    echo "run68 output check passed."
    sed -n '1,20p' "$RUN_LOG"
}

run_gui_hint() {
    print_header "Run in px68k (manual)"

    if [[ ! -f "$OUTPUT_X" ]]; then
        echo "Error: binary not found. Run './compile.sh build' first." >&2
        exit 1
    fi

    cat <<MSG
1) Start Human68k emulator:
   ./run_px68k_humanos.sh

2) Copy '$OUTPUT_X' to your working disk image, then run in Human68k prompt:
   hello

Note:
- For quick functional validation, './compile.sh test' (run68) is recommended first.
MSG
}

usage() {
    cat <<USAGE
Usage: $0 [command]

Commands:
  clean   - Remove build outputs
  build   - Build hello.x
  test    - Run hello.x with run68 and verify output
  run     - Show px68k manual run guide
  all     - clean + build + test
USAGE
}

case "${1:-all}" in
    clean)
        clean
        ;;
    build)
        build
        ;;
    test)
        test_run68
        ;;
    run)
        run_gui_hint
        ;;
    all)
        clean
        build
        test_run68
        ;;
    *)
        usage
        exit 1
        ;;
esac
