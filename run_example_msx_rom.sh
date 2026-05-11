#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

OPENMSX="${OPENMSX:-$PROJECT_ROOT/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx}"
OPENMSX_SHARE="${OPENMSX_SHARE:-$PROJECT_ROOT/Emulator/openMSX/share}"
MACHINE="${MACHINE:-Panasonic_FS-A1GT}"
ROM_PATH="${ROM_PATH:-$PROJECT_ROOT/resource/MSX/sample/example_msx.rom}"

if [[ ! -x "$OPENMSX" ]]; then
    echo "Error: openMSX not executable: $OPENMSX" >&2
    exit 1
fi
if [[ ! -d "$OPENMSX_SHARE" ]]; then
    echo "Error: openMSX share directory not found: $OPENMSX_SHARE" >&2
    exit 1
fi
if [[ ! -f "$ROM_PATH" ]]; then
    echo "Error: ROM not found: $ROM_PATH" >&2
    exit 1
fi

export OPENMSX_SYSTEM_DATA="$OPENMSX_SHARE"
export OPENMSX_DISABLE_SDL_JOYSTICK="${OPENMSX_DISABLE_SDL_JOYSTICK:-1}"

# Override examples:
#   MACHINE=Sony_HB-F1XDJ ./run_example_msx_rom.sh
#   ROM_PATH=/path/to/other.rom ./run_example_msx_rom.sh
#   OPENMSX=/path/to/openmsx OPENMSX_SHARE=/path/to/share ./run_example_msx_rom.sh
echo "Starting example MSX ROM in openMSX..."
echo "  ROM:     $ROM_PATH"
echo "  Machine: $MACHINE"

exec "$OPENMSX" \
  -machine "$MACHINE" \
  -carta "$ROM_PATH" \
  -romtype normal
