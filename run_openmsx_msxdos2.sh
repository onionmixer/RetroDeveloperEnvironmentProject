#!/usr/bin/env bash
# MSX-DOS 2 Emulator Launch Script (openMSX · 기본 기종 Panasonic FS-A1F + MSX-DOS2 카트리지 = DKFS_retro/prototype_20 시험 기종)
#
# Usage: ./run_openmsx_msxdos2.sh
#
# Optional environment overrides:
#   OPENMSX=...        openMSX binary path
#   OPENMSX_SHARE=...  openMSX share path
#   BOOT_DISK=...      MSX-DOS2 boot disk path
#   MACHINE=...        openMSX machine profile (default: Panasonic_FS-A1F — DKFS_retro/prototype_20 실기 시험 기종 ·
#                      Z80 MSX2 · R800 없음. 종전 기본 Panasonic_FS-A1GT(turbo R)는 MACHINE=Panasonic_FS-A1GT)
#   MAPPER_KB=...      매퍼 크기 KB (default: 256 · 빈 값 = 원본) — 사본 기종 `<이름>_<KB>K_V<VRAM>[_NOCKPT]`
#   VRAM_KB=...        VRAM KB (default: 64 · 빈 값 = 원본)
#   ★시험 기종 사본은 tools/msx/openmsx_machine.sh 가 ~/.openMSX/share/machines 에 만든다(A1 Cockpit 메뉴 제거 포함).

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
BOOT_DISK="${BOOT_DISK:-}"
MACHINE="${MACHINE:-Panasonic_FS-A1F}"
MAPPER_KB="${MAPPER_KB-256}"
VRAM_KB="${VRAM_KB-64}"

if [[ -z "$OPENMSX" ]]; then
    OPENMSX="$(first_existing_exec \
        "$SCRIPT_DIR/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx" \
        "$(command -v openmsx 2>/dev/null || true)" \
    )" || OPENMSX="$SCRIPT_DIR/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx"
fi

if [[ -z "$OPENMSX_SHARE" ]]; then
    OPENMSX_SHARE="$(first_existing_dir \
        "$SCRIPT_DIR/Emulator/openMSX/share" \
    )" || OPENMSX_SHARE="$SCRIPT_DIR/Emulator/openMSX/share"
fi

if [[ -z "$BOOT_DISK" ]]; then
    BOOT_DISK="$(first_existing_file \
        "$SCRIPT_DIR/diskwork/bootdisk/msx/msxdos23.dsk" \
    )" || BOOT_DISK="$SCRIPT_DIR/diskwork/bootdisk/msx/msxdos23.dsk"
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

if [[ ! -f "$BOOT_DISK" ]]; then
    echo "Error: Boot disk not found at $BOOT_DISK"
    exit 1
fi

# ★시험 기종 준비(사본 · turbo R 판정) — 실패면 여기서 끝난다
source "$SCRIPT_DIR/tools/msx/openmsx_machine.sh"
openmsx_machine_prep
EXTS=()
# ★turbo R 이 아니면 DOS2 커널이 본체에 없다 → MSX-DOS2 카트리지(외부 슬롯)
if [[ "$HAS_S1990" != 1 ]]; then
    EXTS=(-ext msxdos2)
fi

# Set environment variable for openMSX system data
export OPENMSX_SYSTEM_DATA="$OPENMSX_SHARE"
# Workaround: some SDL2/udev combinations crash during joystick subsystem init.
# Allow override from caller; default to enabled workaround.
export OPENMSX_DISABLE_SDL_JOYSTICK="${OPENMSX_DISABLE_SDL_JOYSTICK:-1}"

echo "Starting openMSX with MSX-DOS 2..."
echo "  openMSX:      $OPENMSX"
echo "  Share:        $OPENMSX_SHARE"
echo "  Machine:      $MACHINE  (CPU: $([[ "$HAS_S1990" == 1 ]] && echo "turbo R · R800 부팅" || echo "Z80 · R800 없음 · DOS2 = msxdos2 카트리지"))"
echo "  Disk A:       $BOOT_DISK"
echo ""
echo "Environment:"
echo "  OPENMSX_DISABLE_SDL_JOYSTICK=$OPENMSX_DISABLE_SDL_JOYSTICK"
echo ""
echo "Note: machine system ROM files (FS-A1F: 사용자 제공 · MSX-DOS2 카트리지 msxdos22.rom) must be installed in:"
echo "  ~/.openMSX/share/systemroms/"
echo ""
echo "After boot, type:"
echo "  DIR           - List files"
echo "  <program>     - Run program"
echo ""
echo "Keyboard shortcuts:"
echo "  F9            - OSD Menu (disk change, etc.)"
echo "  F12           - Pause/Resume"
echo ""

exec "$OPENMSX" -machine "$MACHINE" "${EXTS[@]}" -diska "$BOOT_DISK"
