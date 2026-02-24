#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
OPENMSX="$ROOT_DIR/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx"
ROM="$SCRIPT_DIR/build/HELLO48_NONMAPPER.rom"
STEP="${1:-default}"

build_fallback() {
  (cd "$SCRIPT_DIR" && ROM_ENTRY_MODE=loop ./compile.sh all)
}

build_main_pure() {
  (cd "$SCRIPT_DIR" && ROM_ENTRY_MODE=main-pure ./compile.sh all)
}

verify_main_pure_static() {
  (cd "$SCRIPT_DIR" && ./compile.sh verify-main-pure)
}

verify_fallback_static() {
  (cd "$SCRIPT_DIR" && ./compile.sh verify-fallback)
}

run_openmsx() {
  if [[ ! -f "$ROM" ]]; then
    echo "ROM not found: $ROM" >&2
    exit 1
  fi

  DISPLAY="${DISPLAY:-:1}" \
  OPENMSX_SYSTEM_DATA="${OPENMSX_SYSTEM_DATA:-$HOME/.openMSX/share}" \
  OPENMSX_DISABLE_SDL_JOYSTICK=1 \
  SDL_AUDIODRIVER=dummy \
  "$OPENMSX" \
    -machine Panasonic_FS-A1GT \
    -carta "$ROM" \
    -romtype normal
}

case "$STEP" in
  default)
    build_main_pure
    run_openmsx
    ;;
  default-verified)
    verify_main_pure_static
    run_openmsx
    ;;
  fallback)
    build_fallback
    run_openmsx
    ;;
  fallback-verified)
    verify_fallback_static
    run_openmsx
    ;;
  run-only)
    run_openmsx
    ;;
  *)
    cat <<USAGE
Usage: $0 [default|default-verified|fallback|fallback-verified|run-only]

  default         Build with default mode (main-pure) and run openMSX
  default-verified Run compile.sh verify-main-pure, then run openMSX
  fallback        Build with ROM_ENTRY_MODE=loop and run openMSX
  fallback-verified Run compile.sh verify-fallback, then run openMSX
  run-only        Run current build/HELLO48_NONMAPPER.rom only
USAGE
    exit 1
    ;;
esac
