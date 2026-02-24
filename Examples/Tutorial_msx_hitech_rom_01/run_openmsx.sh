#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ROM="${1:-$ROOT/Examples/Tutorial_msx_hitech_rom_01/build/HELLO48_NONMAPPER.rom}"
OPENMSX="$ROOT/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx"

if [[ ! -f "$ROM" ]]; then
  echo "ROM not found: $ROM"
  echo "Run: (cd \"$ROOT/Examples/Tutorial_msx_hitech_rom_01\" && ./compile.sh all)"
  exit 1
fi

DISPLAY="${DISPLAY:-:1}" \
OPENMSX_SYSTEM_DATA="${OPENMSX_SYSTEM_DATA:-$HOME/.openMSX/share}" \
OPENMSX_DISABLE_SDL_JOYSTICK=1 \
SDL_AUDIODRIVER=dummy \
"$OPENMSX" -machine Panasonic_FS-A1GT -carta "$ROM" -romtype normal
