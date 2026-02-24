#!/usr/bin/env bash
# Apple II ProDOS disk-add boot test launcher

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
ADD_TARGET_NAME="${ADD_TARGET_NAME:-HELLO}"

if [[ -z "$RDEDISKTOOL" ]]; then
    RDEDISKTOOL="$(first_existing_exec \
        "$SCRIPT_DIR/RetroDeveloperEnvironmentDisktool/build_local/rdedisktool" \
        "$SCRIPT_DIR/RetroDeveloperEnvironmentDisktool/build/rdedisktool" \
        "$(command -v rdedisktool 2>/dev/null || true)" \
    )" || RDEDISKTOOL="$SCRIPT_DIR/RetroDeveloperEnvironmentDisktool/build_local/rdedisktool"
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
        "$SCRIPT_DIR/diskwork/bootdisk/AppleII/ProDOS_2_4_3.po" \
        "$SCRIPT_DIR/diskwork/bootdisk/AppleII/prodos242.dsk" \
    )" || BOOT_DISK_SRC="$SCRIPT_DIR/diskwork/bootdisk/AppleII/ProDOS_2_4_3.po"
fi

if [[ -z "$ADD_SOURCE_FILE" ]]; then
    ADD_SOURCE_FILE="$(first_existing_file \
        "$SCRIPT_DIR/Examples/Tutorial_apple_prodos_01/HELLO_RAW" \
        "$SCRIPT_DIR/Examples/Tutorial_apple_prodos_01/HELLO" \
    )" || ADD_SOURCE_FILE="$SCRIPT_DIR/Examples/Tutorial_apple_prodos_01/HELLO_RAW"
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
ext="po"
[[ "$BOOT_DISK_SRC" == *.dsk ]] && ext="dsk"
TEST_BOOT_DISK="$WORK_DIR/prodos_diskaddtest_$(date +%Y%m%d_%H%M%S).$ext"
cp "$BOOT_DISK_SRC" "$TEST_BOOT_DISK"

echo "[diskaddtest] copied boot disk: $BOOT_DISK_SRC -> $TEST_BOOT_DISK"

echo "[diskaddtest] add file via rdedisktool"
"$RDEDISKTOOL" --bootdisk-mode strict add --type B --addr 0x0803 \
  "$TEST_BOOT_DISK" "$ADD_SOURCE_FILE" "$ADD_TARGET_NAME"

echo "[diskaddtest] boot with AppleWin (single drive: D1 only)"
exec "$APPLEWIN" --d1 "$TEST_BOOT_DISK"
