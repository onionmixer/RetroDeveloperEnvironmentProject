#!/usr/bin/env bash
# Prototype 01 MSX/MSX-DOS launcher
#
# Flow:
#  1) (optional) build program
#  2) copy MSX-DOS boot disk to a writable work path
#  3) add compiled COM file using rdedisktool
#  4) run openMSX with the copied boot disk

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

first_existing_file() {
    local p
    for p in "$@"; do
        if [[ -f "$p" ]]; then
            printf '%s\n' "$p"
            return 0
        fi
    done
    return 1
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

RDEDISKTOOL="${RDEDISKTOOL:-}"
BOOT_DISK_SRC="${BOOT_DISK_SRC:-}"
PROGRAM_FILE="${PROGRAM_FILE:-}"
PROGRAM_NAME="${PROGRAM_NAME:-HELLO.COM}"
WORK_DIR="${WORK_DIR:-/tmp/prototype_01_msx_msxdos}"
AUTO_BUILD="${AUTO_BUILD:-0}"

if [[ -z "$RDEDISKTOOL" ]]; then
    RDEDISKTOOL="$(first_existing_exec \
        "$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool" \
        "$(command -v rdedisktool 2>/dev/null || true)" \
    )" || RDEDISKTOOL="$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool"
fi

if [[ -z "$BOOT_DISK_SRC" ]]; then
    BOOT_DISK_SRC="$(first_existing_file \
        "$PROJECT_ROOT/diskwork/bootdisk/msx/msxdos23.dsk" \
    )" || BOOT_DISK_SRC="$PROJECT_ROOT/diskwork/bootdisk/msx/msxdos23.dsk"
fi

if [[ -z "$PROGRAM_FILE" ]]; then
    PROGRAM_FILE="$(first_existing_file \
        "$SCRIPT_DIR/build/HELLO.COM" \
        "$SCRIPT_DIR/HELLO.COM" \
    )" || PROGRAM_FILE="$SCRIPT_DIR/build/HELLO.COM"
fi

if [[ "$AUTO_BUILD" == "1" ]]; then
    if [[ -x "$SCRIPT_DIR/compile.sh" ]]; then
        echo "[run] building program first (compile.sh build)"
        "$SCRIPT_DIR/compile.sh" build
    else
        echo "Error: AUTO_BUILD=1 but compile.sh is not executable at $SCRIPT_DIR/compile.sh"
        exit 1
    fi
fi

if [[ ! -x "$RDEDISKTOOL" ]]; then
    echo "Error: rdedisktool not executable at $RDEDISKTOOL"
    exit 1
fi
if [[ ! -f "$BOOT_DISK_SRC" ]]; then
    echo "Error: boot disk source not found at $BOOT_DISK_SRC"
    exit 1
fi
if [[ ! -f "$PROGRAM_FILE" ]]; then
    echo "Error: program file not found at $PROGRAM_FILE"
    echo "Hint: run ./compile.sh build"
    exit 1
fi

mkdir -p "$WORK_DIR"
TEST_BOOT_DISK="$WORK_DIR/prototype_01_msxdos_$(date +%Y%m%d_%H%M%S).dsk"
cp "$BOOT_DISK_SRC" "$TEST_BOOT_DISK"

echo "[run] copied boot disk: $BOOT_DISK_SRC -> $TEST_BOOT_DISK"
echo "[run] remove previous $PROGRAM_NAME if exists"
"$RDEDISKTOOL" --bootdisk-mode off delete "$TEST_BOOT_DISK" "$PROGRAM_NAME" >/dev/null 2>&1 || true

echo "[run] add program: $PROGRAM_FILE -> $PROGRAM_NAME"
"$RDEDISKTOOL" --bootdisk-mode strict add \
    "$TEST_BOOT_DISK" "$PROGRAM_FILE" "$PROGRAM_NAME"

echo "[run] launching openMSX with Disk A: $TEST_BOOT_DISK"
exec env BOOT_DISK="$TEST_BOOT_DISK" "$PROJECT_ROOT/run_openmsx_msxdos2.sh"
