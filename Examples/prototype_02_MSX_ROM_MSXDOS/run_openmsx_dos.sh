#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

OPENMSX="${OPENMSX:-$PROJECT_ROOT/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx}"
OPENMSX_SHARE="${OPENMSX_SHARE:-$PROJECT_ROOT/Emulator/openMSX/share}"
MACHINE="${MACHINE:-Panasonic_FS-A1GT}"

RDEDISKTOOL="$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool"
BOOT_DISK="${BOOT_DISK:-$PROJECT_ROOT/diskwork/bootdisk/msx/msxdos23.dsk}"
BUILD_DIR="$SCRIPT_DIR/build_dos"
TARGET_BASE="PROTO02"

if [[ ! -x "$OPENMSX" ]]; then
    echo "Error: openMSX not executable: $OPENMSX" >&2
    exit 1
fi

# Find COM file
COM_PATH=""
if [[ -f "$BUILD_DIR/$TARGET_BASE.COM" ]]; then
    COM_PATH="$BUILD_DIR/$TARGET_BASE.COM"
elif [[ -f "$BUILD_DIR/$TARGET_BASE" ]]; then
    COM_PATH="$BUILD_DIR/$TARGET_BASE"
else
    echo "Error: COM not found in $BUILD_DIR" >&2
    echo "Run: ./compile_dos.sh all" >&2
    exit 1
fi

if [[ ! -f "$BOOT_DISK" ]]; then
    echo "Error: Boot disk not found: $BOOT_DISK" >&2
    exit 1
fi

if [[ ! -x "$RDEDISKTOOL" ]]; then
    echo "Error: rdedisktool not found: $RDEDISKTOOL" >&2
    exit 1
fi

# Create working disk from boot disk
WORK_DIR="${WORK_DIR:-/tmp/prototype_02_dos}"
mkdir -p "$WORK_DIR"
WORK_DISK="$WORK_DIR/${TARGET_BASE}_$(date +%Y%m%d_%H%M%S).dsk"
cp -f "$BOOT_DISK" "$WORK_DISK"

# Remove old COM if present, then add new one
"$RDEDISKTOOL" delete "$WORK_DISK" "${TARGET_BASE}.COM" 2>/dev/null || true
"$RDEDISKTOOL" add "$WORK_DISK" "$COM_PATH" "${TARGET_BASE}.COM"

# Add data files (ROOM grids and tileset binaries)
for f in "$BUILD_DIR"/ROOM?? "$BUILD_DIR"/TILES?; do
    [ -f "$f" ] || continue
    fname="$(basename "$f")"
    "$RDEDISKTOOL" delete "$WORK_DISK" "$fname" 2>/dev/null || true
    "$RDEDISKTOOL" add "$WORK_DISK" "$f" "$fname"
    echo "  Added data: $fname"
done

export OPENMSX_SYSTEM_DATA="$OPENMSX_SHARE"
export OPENMSX_DISABLE_SDL_JOYSTICK="${OPENMSX_DISABLE_SDL_JOYSTICK:-1}"

echo "Starting prototype_02 MSX-DOS in openMSX..."
echo "  COM:     $COM_PATH"
echo "  Disk:    $WORK_DISK"
echo "  Machine: $MACHINE"
echo ""
echo "Type PROTO02 at MSX-DOS prompt to run."

exec "$OPENMSX" \
    -machine "$MACHINE" \
    -diska "$WORK_DISK"
