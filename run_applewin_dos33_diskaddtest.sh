#!/usr/bin/env bash
# Apple II DOS 3.3 disk-add boot test launcher
#
# Process:
#  1) copy boot disk to temp path
#  2) add test file via rdedisktool
#  3) run AppleWin using copied disk as BOOT_DISK

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
APPLEWIN="${APPLEWIN:-}"
BOOT_DISK_SRC="${BOOT_DISK_SRC:-}"
ADD_SOURCE_FILE="${ADD_SOURCE_FILE:-}"
ADD_TARGET_NAME="${ADD_TARGET_NAME:-HELLONEW}"

if [[ -z "$RDEDISKTOOL" ]]; then
    RDEDISKTOOL="$(first_existing_exec \
        "$SCRIPT_DIR/RetroDeveloperEnvironmentDisktool/build/rdedisktool" \
        "$(command -v rdedisktool 2>/dev/null || true)" \
    )" || RDEDISKTOOL="$SCRIPT_DIR/RetroDeveloperEnvironmentDisktool/build/rdedisktool"
fi

if [[ -n "$APPLEWIN" && -f "$APPLEWIN" ]]; then
    sig="$(head -c 2 "$APPLEWIN" 2>/dev/null || true)"
    if [[ "$APPLEWIN" == *.sh || "$sig" == "#!" || "$APPLEWIN" == *run_applewin* ]]; then
        echo "Warning: APPLEWIN points to a wrapper script ($APPLEWIN). Re-resolving to sa2 binary."
        APPLEWIN=""
    fi
fi

if [[ -z "$APPLEWIN" ]]; then
    APPLEWIN="$(first_existing_exec \
        "$SCRIPT_DIR/Emulator/AppleWin/build/sa2" \
        "$(command -v sa2 2>/dev/null || true)" \
    )" || APPLEWIN="$SCRIPT_DIR/Emulator/AppleWin/build/sa2"
fi

if [[ -z "$BOOT_DISK_SRC" ]]; then
    BOOT_DISK_SRC="$(first_existing_file \
        "$SCRIPT_DIR/diskwork/bootdisk/AppleII/dos33.dsk" \
    )" || BOOT_DISK_SRC="$SCRIPT_DIR/diskwork/bootdisk/AppleII/dos33.dsk"
fi

if [[ -z "$ADD_SOURCE_FILE" ]]; then
    ADD_SOURCE_FILE="$(first_existing_file \
        "$SCRIPT_DIR/Examples/Tutorial_apple_01/HELLO.BIN" \
    )" || ADD_SOURCE_FILE="$SCRIPT_DIR/Examples/Tutorial_apple_01/HELLO.BIN"
fi

if [[ ! -x "$RDEDISKTOOL" ]]; then
    echo "Error: rdedisktool not executable at $RDEDISKTOOL"
    exit 1
fi
if [[ ! -x "$APPLEWIN" ]]; then
    echo "Error: AppleWin not executable at $APPLEWIN"
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
TEST_BOOT_DISK="$WORK_DIR/dos33_diskaddtest_$(date +%Y%m%d_%H%M%S).dsk"
cp "$BOOT_DISK_SRC" "$TEST_BOOT_DISK"

echo "[diskaddtest] copied boot disk: $BOOT_DISK_SRC -> $TEST_BOOT_DISK"

echo "[diskaddtest] remove non-boot-essential files from copied DOS 3.3 disk"
# Keep core boot-related files: INTBASIC, FPBASIC, MASTER, BOOT13
for f in \
  HELLO APPLESOFT LOADER.OBJ0 "MASTER CREATE" COPY COPY.OBJ0 COPYA \
  CHAIN RENUMBER FILEM FID CONVERT13 MUFFIN START13 SLOT#; do
  "$RDEDISKTOOL" --bootdisk-mode off delete "$TEST_BOOT_DISK" "$f" >/dev/null 2>&1 || true
done

echo "[diskaddtest] add file via rdedisktool"
"$RDEDISKTOOL" --bootdisk-mode strict add --type B --addr 0x0803 \
  "$TEST_BOOT_DISK" "$ADD_SOURCE_FILE" "$ADD_TARGET_NAME"

echo "[diskaddtest] boot with AppleWin (single drive: D1 only)"
exec "$APPLEWIN" --d1 "$TEST_BOOT_DISK"
