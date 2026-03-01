#!/usr/bin/env bash
# Prototype 02 Apple II ProDOS launcher (HGR graphics mode)

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

APPLEWIN="${APPLEWIN:-}"
BOOT_DISK="${BOOT_DISK:-}"
PROGRAM_DISK="${PROGRAM_DISK:-$SCRIPT_DIR/build/prototype_02_appleii_prodos.po}"
PROGRAM_NAME="${PROGRAM_NAME:-PROTO02}"
AUTO_BUILD="${AUTO_BUILD:-0}"

if [[ -n "$APPLEWIN" && -f "$APPLEWIN" ]]; then
    sig="$(head -c 2 "$APPLEWIN" 2>/dev/null || true)"
    if [[ "$APPLEWIN" == *.sh || "$sig" == "#!" || "$APPLEWIN" == *run_applewin* ]]; then
        echo "Warning: APPLEWIN points to wrapper script ($APPLEWIN). Re-resolving to sa2 binary."
        APPLEWIN=""
    fi
fi

if [[ -z "$APPLEWIN" ]]; then
    APPLEWIN="$(first_existing_exec \
        "$PROJECT_ROOT/Emulator/AppleWin/build/sa2" \
        "$(command -v sa2 2>/dev/null || true)" \
    )" || APPLEWIN="$PROJECT_ROOT/Emulator/AppleWin/build/sa2"
fi

if [[ -z "$BOOT_DISK" ]]; then
    BOOT_DISK="$(first_existing_file \
        "$PROJECT_ROOT/diskwork/bootdisk/AppleII/ProDOS_2_4_3.po" \
        "$PROJECT_ROOT/diskwork/bootdisk/AppleII/prodos242.dsk" \
    )" || BOOT_DISK="$PROJECT_ROOT/diskwork/bootdisk/AppleII/ProDOS_2_4_3.po"
fi

if [[ "$AUTO_BUILD" == "1" ]]; then
    echo "[run] preparing program disk first (compile.sh all)"
    "$SCRIPT_DIR/compile.sh" all
fi
if [[ ! -x "$APPLEWIN" ]]; then
    echo "Error: AppleWin not executable at $APPLEWIN"
    exit 1
fi
if [[ ! -f "$BOOT_DISK" ]]; then
    echo "Error: boot disk not found at $BOOT_DISK"
    exit 1
fi
if [[ ! -f "$PROGRAM_DISK" ]]; then
    echo "Error: prepared program disk not found at $PROGRAM_DISK"
    echo "Hint: run ./compile.sh all"
    exit 1
fi

echo "Starting AppleWin (ProDOS) - HGR graphics mode"
echo "  AppleWin: $APPLEWIN"
echo "  D1 boot : $BOOT_DISK"
echo "  D2 prog : $PROGRAM_DISK"
echo
echo "After boot, type:"
echo "  BRUN $PROGRAM_NAME,D2"

echo
exec "$APPLEWIN" --d1 "$BOOT_DISK" --d2 "$PROGRAM_DISK"
