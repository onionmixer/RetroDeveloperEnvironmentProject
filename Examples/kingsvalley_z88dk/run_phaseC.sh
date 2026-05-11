#!/usr/bin/env bash
# run_phaseC.sh — Phase C (Konami 64K cart) 를 openMSX GT 에서 부팅.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

OPENMSX="${OPENMSX:-$PROJECT_ROOT/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx}"
OPENMSX_SHARE="${OPENMSX_SHARE:-$PROJECT_ROOT/Emulator/openMSX/share}"
MACHINE="${MACHINE:-Panasonic_FS-A1GT}"
ROMTYPE="${ROMTYPE:-Konami}"

ROM="${ROM:-$SCRIPT_DIR/build_phaseC/kings.rom}"

[[ -x "$OPENMSX" ]] || { echo "ERROR: openmsx not at $OPENMSX"; exit 1; }
[[ -f "$ROM" ]] || { echo "ERROR: ROM not found at $ROM (run compile_phaseC.sh first)"; exit 1; }

SIZE="$(wc -c < "$ROM")"
SIG="$(xxd -p -l 2 "$ROM")"
echo "Booting: $ROM ($SIZE bytes, sig=$SIG)"
echo "  Machine: $MACHINE  romtype: $ROMTYPE"

export OPENMSX_SYSTEM_DATA="$OPENMSX_SHARE"
export OPENMSX_DISABLE_SDL_JOYSTICK="${OPENMSX_DISABLE_SDL_JOYSTICK:-1}"

exec "$OPENMSX" -machine "$MACHINE" -carta "$ROM" -romtype "$ROMTYPE"
