#!/usr/bin/env bash
# Apple II DOS 3.3 Emulator Launch Script
#
# Usage: ./run_applewin_dos33.sh
#
# Optional environment overrides:
#   APPLEWIN=...     AppleWin binary path (sa2)
#   BOOT_DISK=...    DOS 3.3 boot disk path (D1)
#   PROGRAM_DISK=... program disk path (D2)

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

APPLEWIN="${APPLEWIN:-}"
BOOT_DISK="${BOOT_DISK:-}"
PROGRAM_DISK="${PROGRAM_DISK:-}"

if [[ -z "$APPLEWIN" ]]; then
    APPLEWIN="$(first_existing_exec \
        "$SCRIPT_DIR/Emulator/AppleWin/build/sa2" \
        "$(command -v sa2 2>/dev/null || true)" \
    )" || APPLEWIN="$SCRIPT_DIR/Emulator/AppleWin/build/sa2"
fi

if [[ -z "$BOOT_DISK" ]]; then
    BOOT_DISK="$(first_existing_file \
        "$SCRIPT_DIR/diskwork/bootdisk/AppleII/dos33.dsk" \
    )" || BOOT_DISK="$SCRIPT_DIR/diskwork/bootdisk/AppleII/dos33.dsk"
fi

if [[ -z "$PROGRAM_DISK" ]]; then
    PROGRAM_DISK="$(first_existing_file \
        "$SCRIPT_DIR/Examples/Tutorial_apple_dos33_01/Tutorial_apple_dos33_01.do" \
        "$SCRIPT_DIR/Examples/Tutorial_apple_dos33_01/Tutorial_apple_dos33_01.dsk" \
    )" || PROGRAM_DISK="$SCRIPT_DIR/Examples/Tutorial_apple_dos33_01/Tutorial_apple_dos33_01.do"
fi

if [[ ! -x "$APPLEWIN" ]]; then
    echo "Error: AppleWin not executable at $APPLEWIN"
    echo "Please build AppleWin first."
    exit 1
fi

if [[ ! -f "$BOOT_DISK" ]]; then
    echo "Error: Boot disk not found at $BOOT_DISK"
    exit 1
fi

if [[ ! -f "$PROGRAM_DISK" ]]; then
    echo "Error: Program disk not found at $PROGRAM_DISK"
    echo "Please create the disk image first (or override PROGRAM_DISK)."
    exit 1
fi

echo "Starting AppleWin with DOS 3.3..."
echo "  AppleWin:      $APPLEWIN"
echo "  D1 (Boot):     $BOOT_DISK"
echo "  D2 (Program):  $PROGRAM_DISK"
echo ""
echo "After boot, type:"
echo "  CATALOG,D2      - List files on D2"
echo "  BRUN HELLO,D2   - Run the program"
echo ""

exec "$APPLEWIN" --d1 "$BOOT_DISK" --d2 "$PROGRAM_DISK"
