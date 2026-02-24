#!/usr/bin/env bash
# compile.sh - Build ProDOS-ready HELLO for Tutorial_apple_prodos_01

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

require_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Error: required command not found: $1" >&2
        exit 1
    fi
}

build() {
    require_cmd cl65
    require_cmd tail

    echo "[1/2] Building HELLO with cc65 (apple2 target)"
    cl65 -t apple2 -O -o HELLO hello.c

    echo "[2/2] Stripping AppleSingle header to keep raw payload"
    tail -c +59 HELLO > HELLO.tmp
    mv HELLO.tmp HELLO

    echo "Done: $SCRIPT_DIR/HELLO ($(wc -c < HELLO) bytes)"
}

clean() {
    rm -f HELLO HELLO.tmp hello.o
    echo "Cleaned build outputs."
}

usage() {
    cat <<USAGE
Usage: $0 [build|clean|all]
  build : compile hello.c and produce raw HELLO
  clean : remove build outputs
  all   : clean + build (default)
USAGE
}

case "${1:-all}" in
    build)
        build
        ;;
    clean)
        clean
        ;;
    all)
        clean
        build
        ;;
    *)
        usage
        exit 1
        ;;
esac
