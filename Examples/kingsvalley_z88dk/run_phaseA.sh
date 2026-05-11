#!/usr/bin/env bash
# run_phaseA.sh — boot kingsvalley Phase A ROM in openMSX
#
# Phase A 산출물 (build_phaseA/kings.rom) 을 GT (Panasonic_FS-A1GT) 에서 부팅.
# **NO `-romtype` flag** — plain 32K cart 자동 감지에 위임 (PLAN §7 Iter 33).
# Konami 매퍼 강제 시 page-2 영역 write 가 bank-switch 로 가로채여 GT hang
# 유발 (§7 Iter 29). 자동 감지로 RomPlain 매핑이 정답.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

OPENMSX="${OPENMSX:-$PROJECT_ROOT/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx}"
OPENMSX_SHARE="${OPENMSX_SHARE:-$PROJECT_ROOT/Emulator/openMSX/share}"
MACHINE="${MACHINE:-Panasonic_FS-A1GT}"

ROM="${ROM:-$SCRIPT_DIR/build_phaseA/kings.rom}"

[[ -x "$OPENMSX" ]] || { echo "ERROR: openmsx not at $OPENMSX" >&2; exit 1; }
[[ -f "$ROM" ]]    || { echo "ERROR: ROM not found at $ROM (run compile_phaseA.sh first)" >&2; exit 1; }

SIZE="$(wc -c < "$ROM")"
[[ "$SIZE" == "32768" ]] || { echo "WARN: expected 32768 bytes, got $SIZE" >&2; }

SIG="$(xxd -p -l 2 "$ROM")"
[[ "$SIG" == "4142" ]] || { echo "WARN: signature is '$SIG', expected '4142' (AB)" >&2; }

export OPENMSX_SYSTEM_DATA="$OPENMSX_SHARE"
export OPENMSX_DISABLE_SDL_JOYSTICK="${OPENMSX_DISABLE_SDL_JOYSTICK:-1}"

echo "Booting Phase A ROM: $ROM ($SIZE bytes)"
echo "  Machine: $MACHINE  (NO -romtype: plain auto-detect)"

exec "$OPENMSX" -machine "$MACHINE" -carta "$ROM"
