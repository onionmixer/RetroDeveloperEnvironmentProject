#!/usr/bin/env bash
# Prototype 01 X68000/Human68k launcher (prepared FDD1 flow)
#
# Flow:
#  1) (optional) compile.sh all
#  2) launch px68k via project-level run_px68k_humanos.sh with prepared FDD1 attached

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

BOOT_DISK="${BOOT_DISK:-}"
PROGRAM_DISK="${PROGRAM_DISK:-$SCRIPT_DIR/build/prototype_01_x68000_HUMANOS_FDD1.xdf}"
AUTO_BUILD="${AUTO_BUILD:-0}"

if [[ -z "$BOOT_DISK" ]]; then
    BOOT_DISK="$(first_existing_file \
        "$PROJECT_ROOT/diskwork/bootdisk/x68000/HUMAN302.XDF" \
    )" || BOOT_DISK="$PROJECT_ROOT/diskwork/bootdisk/x68000/HUMAN302.XDF"
fi

if [[ "$AUTO_BUILD" == "1" ]]; then
    if [[ -x "$SCRIPT_DIR/compile.sh" ]]; then
        echo "[run] preparing program disk first (compile.sh all)"
        "$SCRIPT_DIR/compile.sh" all
    else
        echo "Error: AUTO_BUILD=1 but compile.sh is not executable at $SCRIPT_DIR/compile.sh"
        exit 1
    fi
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
if [[ ! -x "$PROJECT_ROOT/run_px68k_humanos.sh" ]]; then
    echo "Error: launcher not executable at $PROJECT_ROOT/run_px68k_humanos.sh"
    exit 1
fi

echo "[fdd1] launching px68k with prepared FDD1 image"
echo "  BOOT_DISK(FDD0): $BOOT_DISK"
echo "  PROGRAM_DISK(FDD1): $PROGRAM_DISK"
exec env BOOT_DISK="$BOOT_DISK" FDD1_DISK="$PROGRAM_DISK" "$PROJECT_ROOT/run_px68k_humanos.sh"
