#!/usr/bin/env bash
# X68000 Human68k disk-add boot test launcher

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

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
ADD_SOURCE_FILE="${ADD_SOURCE_FILE:-}"
ADD_TARGET_NAME="${ADD_TARGET_NAME:-HELLO.X}"

if [[ -z "$RDEDISKTOOL" ]]; then
    RDEDISKTOOL="$(first_existing_exec \
        "$SCRIPT_DIR/RetroDeveloperEnvironmentDisktool/build_local/rdedisktool" \
        "$SCRIPT_DIR/RetroDeveloperEnvironmentDisktool/build/rdedisktool" \
        "$(command -v rdedisktool 2>/dev/null || true)" \
    )" || RDEDISKTOOL="$SCRIPT_DIR/RetroDeveloperEnvironmentDisktool/build_local/rdedisktool"
fi

if [[ -z "$BOOT_DISK_SRC" ]]; then
    BOOT_DISK_SRC="$(first_existing_file \
        "$SCRIPT_DIR/diskwork/bootdisk/x68000/HUMAN302.XDF" \
    )" || BOOT_DISK_SRC="$SCRIPT_DIR/diskwork/bootdisk/x68000/HUMAN302.XDF"
fi

if [[ -z "$ADD_SOURCE_FILE" ]]; then
    ADD_SOURCE_FILE="$(first_existing_file \
        "$SCRIPT_DIR/Examples/Tutorial_x68000_01/hello.x" \
    )" || ADD_SOURCE_FILE="$SCRIPT_DIR/Examples/Tutorial_x68000_01/hello.x"
fi

if [[ ! -x "$RDEDISKTOOL" ]]; then
    echo "Error: rdedisktool not executable at $RDEDISKTOOL"
    exit 1
fi
if [[ ! -f "$BOOT_DISK_SRC" ]]; then
    echo "Error: Boot disk source not found at $BOOT_DISK_SRC"
    exit 1
fi
if [[ ! -f "$ADD_SOURCE_FILE" ]]; then
    echo "Error: Add source file not found at $ADD_SOURCE_FILE"
    exit 1
fi

WORK_DIR="${WORK_DIR:-/tmp/rdedisktool_diskaddtest}"
mkdir -p "$WORK_DIR"
TEST_BOOT_DISK="$WORK_DIR/humanos_diskaddtest_$(date +%Y%m%d_%H%M%S).xdf"
cp "$BOOT_DISK_SRC" "$TEST_BOOT_DISK"

echo "[diskaddtest] copied boot disk: $BOOT_DISK_SRC -> $TEST_BOOT_DISK"

echo "[diskaddtest] add file via rdedisktool"
"$RDEDISKTOOL" --bootdisk-mode strict add \
  "$TEST_BOOT_DISK" "$ADD_SOURCE_FILE" "$ADD_TARGET_NAME"

echo "[diskaddtest] boot with px68k (single drive: FDD0 only)"
exec env BOOT_DISK="$TEST_BOOT_DISK" FDD1_DISK= "$SCRIPT_DIR/run_px68k_humanos.sh"
