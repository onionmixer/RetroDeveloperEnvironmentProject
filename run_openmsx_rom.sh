#!/usr/bin/env bash
# MSX ROM Emulator Launch Script (openMSX with GT BIOS)
#
# Usage: ./run_openmsx_rom.sh
#
# Optional environment overrides:
#   OPENMSX=...        openMSX binary path
#   OPENMSX_SHARE=...  openMSX share path
#   ROM_PATH=...       Cartridge ROM path
#   ROM_TYPE=...       openMSX romtype (default: normal)
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
ROM_PATH="${ROM_PATH:-}"
ROM_TYPE="${ROM_TYPE:-normal}"
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
        "$HOME/.openMSX/share" \
    )" || OPENMSX_SHARE="$SCRIPT_DIR/Emulator/openMSX/share"
fi

if [[ -z "$ROM_PATH" ]]; then
    ROM_PATH="$(first_existing_file \
        "$SCRIPT_DIR/Examples/Tutorial_msx_z88dk_rom_01/build/HELLO_ROM_Z88DK.rom" \
        "$SCRIPT_DIR/Examples/Tutorial_msx_z88dk_rom_01/HELLO_ROM_Z88DK.rom" \
    )" || ROM_PATH="$SCRIPT_DIR/Examples/Tutorial_msx_z88dk_rom_01/build/HELLO_ROM_Z88DK.rom"
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

if [[ ! -f "$ROM_PATH" ]]; then
    echo "Error: ROM not found at $ROM_PATH"
    echo "Build it first from: $SCRIPT_DIR/Examples/Tutorial_msx_z88dk_rom_01"
    echo "  ./compile.sh all"
    exit 1
fi

# Set environment variable for openMSX system data
export OPENMSX_SYSTEM_DATA="$OPENMSX_SHARE"
# Workaround: some SDL2/udev combinations crash during joystick subsystem init.
# Allow override from caller; default to enabled workaround.
export OPENMSX_DISABLE_SDL_JOYSTICK="${OPENMSX_DISABLE_SDL_JOYSTICK:-1}"

echo "Starting openMSX with cartridge ROM..."
echo "  openMSX:      $OPENMSX"
echo "  Share:        $OPENMSX_SHARE"
echo "  Machine:      $MACHINE"
echo "  Cartridge A:  $ROM_PATH"
echo "  ROM Type:     $ROM_TYPE"
echo ""
echo "Environment:"
echo "  OPENMSX_DISABLE_SDL_JOYSTICK=$OPENMSX_DISABLE_SDL_JOYSTICK"
echo ""
echo "Note: GT BIOS ROM files must be installed in:"
echo "  ~/.openMSX/share/systemroms/"
echo ""

exec "$OPENMSX" -machine "$MACHINE" -carta "$ROM_PATH" -romtype "$ROM_TYPE"
