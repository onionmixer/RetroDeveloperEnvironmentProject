#!/usr/bin/env bash
# 05_music — ROM cartridge launcher in openMSX
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
OPENMSX="${OPENMSX:-$PROJECT_ROOT/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx}"
OPENMSX_SHARE="${OPENMSX_SHARE:-$PROJECT_ROOT/Emulator/openMSX/share}"
MACHINE="${MACHINE:-Panasonic_FS-A1GT}"
ROM_PATH="${ROM_PATH:-$SCRIPT_DIR/build/05_music.rom}"
[[ -x "$OPENMSX" ]] || { echo "Error: openMSX not executable: $OPENMSX" >&2; exit 1; }
[[ -f "$ROM_PATH" ]] || { echo "Error: ROM not found: $ROM_PATH (run ./compile.sh)" >&2; exit 1; }
export OPENMSX_SYSTEM_DATA="$OPENMSX_SHARE"
export OPENMSX_DISABLE_SDL_JOYSTICK="${OPENMSX_DISABLE_SDL_JOYSTICK:-1}"
echo "Starting 05_music ROM in openMSX..."
exec "$OPENMSX" -machine "$MACHINE" -carta "$ROM_PATH"
