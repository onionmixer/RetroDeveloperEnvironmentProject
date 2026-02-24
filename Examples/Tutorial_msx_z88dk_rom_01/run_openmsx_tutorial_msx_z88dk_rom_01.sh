#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

OPENMSX="${OPENMSX:-$PROJECT_ROOT/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx}"
OPENMSX_SHARE="${OPENMSX_SHARE:-$HOME/.openMSX/share}"
MACHINE="${MACHINE:-Panasonic_FS-A1GT}"
ROM_PATH="${ROM_PATH:-$SCRIPT_DIR/build/HELLO_ROM_Z88DK.rom}"

if [[ ! -x "$OPENMSX" ]]; then
    echo "Error: openMSX not executable: $OPENMSX" >&2
    exit 1
fi
if [[ ! -f "$ROM_PATH" ]]; then
    echo "Error: ROM not found: $ROM_PATH" >&2
    echo "Run: ./compile.sh all" >&2
    exit 1
fi

export OPENMSX_SYSTEM_DATA="$OPENMSX_SHARE"
export OPENMSX_DISABLE_SDL_JOYSTICK="${OPENMSX_DISABLE_SDL_JOYSTICK:-1}"

exec "$OPENMSX" \
  -machine "$MACHINE" \
  -carta "$ROM_PATH" \
  -romtype normal
