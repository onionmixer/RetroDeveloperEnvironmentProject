#!/usr/bin/env bash
# 09_tetris — MSX-DOS2 .COM launcher in openMSX
# Stages a fresh /tmp working disk on every run.
# Pattern: prototype_05_MSX_ROM_MSXDOS/run_openmsx_dos.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"

OPENMSX="${OPENMSX:-$PROJECT_ROOT/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx}"
OPENMSX_SHARE="${OPENMSX_SHARE:-$PROJECT_ROOT/Emulator/openMSX/share}"
MACHINE="${MACHINE:-Panasonic_FS-A1GT}"
RDEDISKTOOL="${RDEDISKTOOL:-$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool}"
BOOT_DISK="${BOOT_DISK:-$PROJECT_ROOT/diskwork/bootdisk/msx/msxdos23.dsk}"
BUILD_DIR="$SCRIPT_DIR/build_dos"
TARGET_BASE="09_tetris"
DOS_NAME="TETRIS"

[[ -x "$OPENMSX"     ]] || { echo "Error: openMSX not executable: $OPENMSX"     >&2; exit 1; }
[[ -x "$RDEDISKTOOL" ]] || { echo "Error: rdedisktool not executable: $RDEDISKTOOL" >&2; exit 1; }
[[ -f "$BOOT_DISK"   ]] || { echo "Error: boot disk not found: $BOOT_DISK"      >&2; exit 1; }

COM=""
[[ -f "$BUILD_DIR/${TARGET_BASE}.COM" ]] && COM="$BUILD_DIR/${TARGET_BASE}.COM"
[[ -z "$COM" && -f "$BUILD_DIR/$TARGET_BASE" ]] && COM="$BUILD_DIR/$TARGET_BASE"
[[ -n "$COM" ]] || { echo "Error: COM not found in $BUILD_DIR (run ./compile_dos.sh)" >&2; exit 1; }

WORK_DIR="${WORK_DIR:-/tmp/09_tetris_dos}"
mkdir -p "$WORK_DIR"
WORK_DISK="$WORK_DIR/${TARGET_BASE}_$(date +%Y%m%d_%H%M%S).dsk"
cp -f "$BOOT_DISK" "$WORK_DISK"
"$RDEDISKTOOL" delete "$WORK_DISK" "${DOS_NAME}.COM" 2>/dev/null || true
"$RDEDISKTOOL" add    "$WORK_DISK" "$COM" "${DOS_NAME}.COM"

export OPENMSX_SYSTEM_DATA="$OPENMSX_SHARE"
export OPENMSX_DISABLE_SDL_JOYSTICK="${OPENMSX_DISABLE_SDL_JOYSTICK:-1}"

echo "Starting 09_tetris in MSX-DOS2..."
echo "  COM:     $COM"
echo "  Disk:    $WORK_DISK"
echo "  Machine: $MACHINE"
echo ""
echo "Type ${DOS_NAME} at the MSX-DOS prompt to run."
exec "$OPENMSX" -machine "$MACHINE" -diska "$WORK_DISK"
