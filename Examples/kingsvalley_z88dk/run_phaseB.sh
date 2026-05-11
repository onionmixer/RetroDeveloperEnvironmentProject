#!/usr/bin/env bash
# run_phaseB.sh — boot kingsvalley Phase B ROM in openMSX
#
# Phase B 산출물 = build_phaseB/kings.rom (64 KiB, 4×16K banks, ASCII16 매퍼).
# bank 0+1 = main code, bank 2 = AKM blob (mplayer_engine_load 가 LDIR).
# bank 3 = unused padding.
#
# 매퍼 force: openMSX 가 hash/heuristics 로 ASCII16 자동 감지하지 않을
# 가능성에 대비해 명시 지정.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

OPENMSX="${OPENMSX:-$PROJECT_ROOT/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx}"
OPENMSX_SHARE="${OPENMSX_SHARE:-$PROJECT_ROOT/Emulator/openMSX/share}"
MACHINE="${MACHINE:-Panasonic_FS-A1GT}"
ROMTYPE="${ROMTYPE:-ASCII16}"

ROM="${ROM:-$SCRIPT_DIR/build_phaseB/kings.rom}"

[[ -x "$OPENMSX" ]] || { echo "ERROR: openmsx not at $OPENMSX" >&2; exit 1; }
[[ -f "$ROM" ]]    || { echo "ERROR: ROM not found at $ROM (run compile_phaseB.sh first)" >&2; exit 1; }

SIZE="$(wc -c < "$ROM")"
[[ "$SIZE" == "65536" ]] || { echo "WARN: expected 65536 bytes (4 banks), got $SIZE" >&2; }

SIG="$(xxd -p -l 2 "$ROM")"
[[ "$SIG" == "4142" ]] || { echo "WARN: signature is '$SIG', expected '4142' (AB)" >&2; }

export OPENMSX_SYSTEM_DATA="$OPENMSX_SHARE"
export OPENMSX_DISABLE_SDL_JOYSTICK="${OPENMSX_DISABLE_SDL_JOYSTICK:-1}"

echo "Booting Phase B ROM: $ROM ($SIZE bytes)"
echo "  Machine: $MACHINE  romtype: $ROMTYPE"

exec "$OPENMSX" -machine "$MACHINE" -carta "$ROM" -romtype "$ROMTYPE"
