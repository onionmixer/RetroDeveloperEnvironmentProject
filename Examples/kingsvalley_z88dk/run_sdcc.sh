#!/usr/bin/env bash
# run_sdcc.sh — SDCC 로 빌드한 kings_sdcc.rom 을 openMSX GT 에서 실행.
#
# 직접 비교용: 같은 game source 를 SDCC 와 z88dk(+sccz80) 로 빌드해
# GT 에서 도구 사용 시 멈칫 양상 비교. 두 ROM 모두 mplayer/AKM 무력화
# stub (음악 없음) — game logic 만 비교.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

OPENMSX="${OPENMSX:-$PROJECT_ROOT/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx}"
OPENMSX_SHARE="${OPENMSX_SHARE:-$PROJECT_ROOT/Emulator/openMSX/share}"
MACHINE="${MACHINE:-Panasonic_FS-A1GT}"

ROM="${ROM:-$SCRIPT_DIR/game/build/kings_sdcc.rom}"

[[ -x "$OPENMSX" ]] || { echo "ERROR: openmsx not at $OPENMSX"; exit 1; }
[[ -f "$ROM" ]] || { echo "ERROR: ROM not found at $ROM (run build_sdcc.sh first)"; exit 1; }

SIZE="$(wc -c < "$ROM")"
SIG="$(xxd -p -l 2 "$ROM")"
echo "Booting SDCC ROM: $ROM ($SIZE bytes, sig=$SIG)"
echo "  Machine: $MACHINE  (NO -romtype: plain auto-detect)"

export OPENMSX_SYSTEM_DATA="$OPENMSX_SHARE"
export OPENMSX_DISABLE_SDL_JOYSTICK="${OPENMSX_DISABLE_SDL_JOYSTICK:-1}"

exec "$OPENMSX" -machine "$MACHINE" -carta "$ROM"
