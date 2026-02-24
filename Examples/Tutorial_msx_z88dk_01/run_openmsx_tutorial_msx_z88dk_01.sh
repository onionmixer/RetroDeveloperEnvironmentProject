#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

OPENMSX="${OPENMSX:-$PROJECT_ROOT/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx}"
OPENMSX_SHARE="${OPENMSX_SHARE:-$HOME/.openMSX/share}"
MACHINE="${MACHINE:-Panasonic_FS-A1GT}"
BOOT_DISK="${BOOT_DISK:-$PROJECT_ROOT/diskwork/bootdisk/msx/msxdos23.dsk}"
PROGRAM_DISK="${PROGRAM_DISK:-$SCRIPT_DIR/build/Tutorial_msx_z88dk_01.dsk}"

if [[ ! -x "$OPENMSX" ]]; then
    echo "Error: openMSX not executable: $OPENMSX" >&2
    exit 1
fi
if [[ ! -f "$BOOT_DISK" ]]; then
    echo "Error: boot disk not found: $BOOT_DISK" >&2
    exit 1
fi
if [[ ! -f "$PROGRAM_DISK" ]]; then
    echo "Error: program disk not found: $PROGRAM_DISK" >&2
    echo "Run: ./compile.sh all" >&2
    exit 1
fi

export OPENMSX_SYSTEM_DATA="$OPENMSX_SHARE"
export OPENMSX_DISABLE_SDL_JOYSTICK="${OPENMSX_DISABLE_SDL_JOYSTICK:-1}"

exec "$OPENMSX" \
  -machine "$MACHINE" \
  -diska "$BOOT_DISK" \
  -diskb "$PROGRAM_DISK"
