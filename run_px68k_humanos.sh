#!/usr/bin/env bash
# X68000 Human68k Emulator Launch Script (px68k-onionmixer)
#
# Usage:
#   ./run_px68k_humanos.sh
#
# Optional environment overrides:
#   PX68K=...      emulator binary path
#   IPL_ROM=...    IPL ROM path
#   CG_ROM=...     CG ROM path
#   BOOT_DISK=...  FDD0 boot disk path
#   FDD1_DISK=...  optional FDD1 disk path

set -euo pipefail

# Get script directory (project root)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Resolve first existing path from candidates.
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

# Paths (override allowed; otherwise auto-discover from known roots)
PX68K="${PX68K:-}"
IPL_ROM="${IPL_ROM:-}"
CG_ROM="${CG_ROM:-}"
BOOT_DISK="${BOOT_DISK:-}"
FDD1_DISK="${FDD1_DISK:-}"

if [[ -z "$PX68K" ]]; then
    PX68K="$(first_existing_exec \
        "$SCRIPT_DIR/Emulator/x68000/px68k-onionmixer/px68k-onionmixer" \
        "$SCRIPT_DIR/Toolchain/x68000/px68k-onionmixer/px68k-onionmixer" \
    )" || PX68K="$SCRIPT_DIR/Emulator/x68000/px68k-onionmixer/px68k-onionmixer"
fi

if [[ -z "$IPL_ROM" ]]; then
    IPL_ROM="$(first_existing_file \
        "$SCRIPT_DIR/Emulator/x68000/px68k-onionmixer/iplrom.dat" \
        "$SCRIPT_DIR/Toolchain/x68000/px68k-onionmixer/iplrom.dat" \
    )" || IPL_ROM="$SCRIPT_DIR/Emulator/x68000/px68k-onionmixer/iplrom.dat"
fi

if [[ -z "$CG_ROM" ]]; then
    CG_ROM="$(first_existing_file \
        "$SCRIPT_DIR/Emulator/x68000/px68k-onionmixer/cgrom.dat" \
        "$SCRIPT_DIR/Toolchain/x68000/px68k-onionmixer/cgrom.dat" \
    )" || CG_ROM="$SCRIPT_DIR/Emulator/x68000/px68k-onionmixer/cgrom.dat"
fi

if [[ -z "$BOOT_DISK" ]]; then
    BOOT_DISK="$(first_existing_file \
        "$SCRIPT_DIR/Emulator/x68000/px68k-onionmixer/HUMAN302.XDF" \
        "$SCRIPT_DIR/Emulator/x68000/work.xdf" \
        "$SCRIPT_DIR/Toolchain/x68000/HUMAN302.XDF" \
        "$SCRIPT_DIR/Toolchain/x68000/work.xdf" \
    )" || BOOT_DISK="$SCRIPT_DIR/Emulator/x68000/px68k-onionmixer/HUMAN302.XDF"
fi

# Check if px68k exists
if [[ ! -x "$PX68K" ]]; then
    echo "Error: px68k-onionmixer not executable at $PX68K"
    echo "Please build px68k-onionmixer first."
    exit 1
fi

# Check if IPL ROM exists
if [[ ! -f "$IPL_ROM" ]]; then
    echo "Error: IPL ROM not found at $IPL_ROM"
    echo "Please place iplrom.dat in the px68k-onionmixer directory (or override IPL_ROM)."
    exit 1
fi

# Check if CG ROM exists
if [[ ! -f "$CG_ROM" ]]; then
    echo "Error: CG ROM (font) not found at $CG_ROM"
    echo "Please place cgrom.dat in the px68k-onionmixer directory (or override CG_ROM)."
    exit 1
fi

# Check if boot disk exists
if [[ ! -f "$BOOT_DISK" ]]; then
    echo "Error: Boot disk not found at $BOOT_DISK"
    exit 1
fi

if [[ -n "$FDD1_DISK" && ! -f "$FDD1_DISK" ]]; then
    echo "Error: FDD1 disk not found at $FDD1_DISK"
    exit 1
fi

echo "Starting px68k-onionmixer with Human68k..."
echo "  PX68K:        $PX68K"
echo "  IPL ROM:      $IPL_ROM"
echo "  CG ROM:       $CG_ROM"
echo "  FDD0 (Boot):  $BOOT_DISK"
if [[ -n "$FDD1_DISK" ]]; then
    echo "  FDD1:         $FDD1_DISK"
fi
echo ""
echo "Keyboard shortcuts:"
echo "  F12           - Menu (disk change, settings, etc.)"
echo "  F11           - Fullscreen toggle"
echo ""

# Build command args
args=(--iplrom "$IPL_ROM" --cgrom "$CG_ROM" --fdd0 "$BOOT_DISK")
if [[ -n "$FDD1_DISK" ]]; then
    args+=(--fdd1 "$FDD1_DISK")
fi

exec "$PX68K" "${args[@]}"
