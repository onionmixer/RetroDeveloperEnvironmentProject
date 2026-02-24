#!/usr/bin/env bash
# MSX-DOS 2 Emulator Launch Script (openMSX with GT BIOS)
#
# Usage: ./run_openmsx_msxdos2.sh
#
# Optional environment overrides:
#   OPENMSX=...        openMSX binary path
#   OPENMSX_SHARE=...  openMSX share path
#   BOOT_DISK=...      MSX-DOS2 boot disk path
#   MACHINE=...        openMSX machine profile (default: Panasonic_FS-A1GT)

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

first_existing_dir() {
    local p
    for p in "$@"; do
        if [[ -d "$p" ]]; then
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

OPENMSX="${OPENMSX:-}"
OPENMSX_SHARE="${OPENMSX_SHARE:-}"
BOOT_DISK="${BOOT_DISK:-}"
MACHINE="${MACHINE:-Panasonic_FS-A1GT}"

if [[ -z "$OPENMSX" ]]; then
    OPENMSX="$(first_existing_exec \
        "$SCRIPT_DIR/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx" \
        "$(command -v openmsx 2>/dev/null || true)" \
    )" || OPENMSX="$SCRIPT_DIR/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx"
fi

if [[ -z "$OPENMSX_SHARE" ]]; then
    OPENMSX_SHARE="$(first_existing_dir \
        "$SCRIPT_DIR/Emulator/openMSX/share" \
    )" || OPENMSX_SHARE="$SCRIPT_DIR/Emulator/openMSX/share"
fi

if [[ -z "$BOOT_DISK" ]]; then
    BOOT_DISK="$(first_existing_file \
        "$SCRIPT_DIR/diskwork/bootdisk/msx/msxdos23.dsk" \
    )" || BOOT_DISK="$SCRIPT_DIR/diskwork/bootdisk/msx/msxdos23.dsk"
fi

if [[ ! -x "$OPENMSX" ]]; then
    echo "Error: openMSX not executable at $OPENMSX"
    echo "Please build openMSX first."
    exit 1
fi

if [[ ! -d "$OPENMSX_SHARE" ]]; then
    echo "Error: openMSX share directory not found at $OPENMSX_SHARE"
    exit 1
fi

if [[ ! -f "$BOOT_DISK" ]]; then
    echo "Error: Boot disk not found at $BOOT_DISK"
    exit 1
fi

# Set environment variable for openMSX system data
export OPENMSX_SYSTEM_DATA="$OPENMSX_SHARE"
# Workaround: some SDL2/udev combinations crash during joystick subsystem init.
# Allow override from caller; default to enabled workaround.
export OPENMSX_DISABLE_SDL_JOYSTICK="${OPENMSX_DISABLE_SDL_JOYSTICK:-1}"

echo "Starting openMSX with MSX-DOS 2..."
echo "  openMSX:      $OPENMSX"
echo "  Share:        $OPENMSX_SHARE"
echo "  Machine:      $MACHINE"
echo "  Disk A:       $BOOT_DISK"
echo ""
echo "Environment:"
echo "  OPENMSX_DISABLE_SDL_JOYSTICK=$OPENMSX_DISABLE_SDL_JOYSTICK"
echo ""
echo "Note: GT BIOS ROM files must be installed in:"
echo "  ~/.openMSX/share/systemroms/"
echo ""
echo "After boot, type:"
echo "  DIR           - List files"
echo "  <program>     - Run program"
echo ""
echo "Keyboard shortcuts:"
echo "  F9            - OSD Menu (disk change, etc.)"
echo "  F12           - Pause/Resume"
echo ""

exec "$OPENMSX" -machine "$MACHINE" -diska "$BOOT_DISK"
