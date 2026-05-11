#!/usr/bin/env bash
# 02_consolehello — ROM cartridge launcher in openMSX
# Pattern: prototype_05_MSX_ROM_MSXDOS/run_openmsx.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"

OPENMSX="${OPENMSX:-$PROJECT_ROOT/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx}"
OPENMSX_SHARE="${OPENMSX_SHARE:-$PROJECT_ROOT/Emulator/openMSX/share}"
MACHINE="${MACHINE:-Panasonic_FS-A1GT}"
ROM_PATH="${ROM_PATH:-$SCRIPT_DIR/build/02_consolehello.rom}"

[[ -x "$OPENMSX" ]] || { echo "Error: openMSX not executable: $OPENMSX" >&2; exit 1; }
[[ -f "$ROM_PATH" ]] || { echo "Error: ROM not found: $ROM_PATH (run ./compile.sh)" >&2; exit 1; }

export OPENMSX_SYSTEM_DATA="$OPENMSX_SHARE"
export OPENMSX_DISABLE_SDL_JOYSTICK="${OPENMSX_DISABLE_SDL_JOYSTICK:-1}"

echo "Starting 02_consolehello ROM in openMSX..."
echo "  ROM:     $ROM_PATH"
echo "  Machine: $MACHINE"
exec "$OPENMSX" -machine "$MACHINE" -carta "$ROM_PATH"
