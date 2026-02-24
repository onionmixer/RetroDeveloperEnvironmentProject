#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

CFG="$SCRIPT_DIR/TUTORIAL_ROM2.CFG"
BUILD_DIR="$SCRIPT_DIR/build"
WORK_ROOT="${WORK_ROOT:-/tmp/tutorial_rom2_mk}"
WORK_DIR="$WORK_ROOT"
MK="$WORK_DIR/TUTORIAL_ROM2.MK"
OUT_DIR="$WORK_DIR/TUTORIAL_ROM2.OUT"

print_header() {
  echo "========================================"
  echo "$1"
  echo "========================================"
}

clean() {
  print_header "Cleaning"
  rm -rf "$BUILD_DIR"
  rm -rf "$WORK_DIR"
}

build() {
  print_header "Generating .MK from CFG"
  mkdir -p "$BUILD_DIR"
  mkdir -p "$WORK_DIR"
  "$ROOT_DIR/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLMKRULE" "$CFG" "$MK"

  print_header "Building with generated .MK"
  make -f "$MK" build

  print_header "Verifying outputs"
  local out_rom="$OUT_DIR/TUTORIAL_ROM2.rom"
  local out_cart="$OUT_DIR/TUTORIAL_ROM2_NONMAPPER.rom"
  local out_report="$OUT_DIR/TUTORIAL_ROM2_pack_report.json"
  local out_map="$OUT_DIR/TUTORIAL_ROM2.map"

  [[ -f "$out_rom" ]] || { echo "Error: missing $out_rom" >&2; exit 1; }
  [[ -f "$out_cart" ]] || { echo "Error: missing $out_cart" >&2; exit 1; }
  [[ -f "$out_report" ]] || { echo "Error: missing $out_report" >&2; exit 1; }
  [[ -f "$out_map" ]] || { echo "Error: missing $out_map" >&2; exit 1; }

  python3 - "$out_rom" "$out_cart" <<'PY'
from pathlib import Path
import sys

rom = Path(sys.argv[1]).read_bytes()
cart = Path(sys.argv[2]).read_bytes()

if len(rom) != 49152:
    raise SystemExit(f"unexpected rom size: {len(rom)}")
if len(cart) != 49152:
    raise SystemExit(f"unexpected cart size: {len(cart)}")
if rom[0x0100:0x0104] != b"ROM ":
    raise SystemExit("missing BL_ROM signature at 0x0100")
if cart[0:2] != b"AB":
    raise SystemExit("missing AB signature at 0x0000")

print("OK: TUTORIAL_ROM2.rom size=49152 sig@0x100='ROM '")
print("OK: TUTORIAL_ROM2_NONMAPPER.rom size=49152 sig='AB'")
PY

  python3 - "$out_map" <<'PY'
from pathlib import Path
import re
import sys

txt = Path(sys.argv[1]).read_text(encoding="utf-8", errors="ignore")
m = re.search(r"__Lbss\s+bss\s+([0-9A-Fa-f]{4})", txt)
if not m:
    raise SystemExit("missing __Lbss symbol in map")
lbss = int(m.group(1), 16)
if lbss < 0x8000:
    raise SystemExit(f"unsafe __Lbss address: 0x{lbss:04X} (<0x8000)")
print(f"OK: __Lbss=0x{lbss:04X} (safe RAM region)")
PY

  cp -f "$out_rom" "$BUILD_DIR/TUTORIAL_ROM2.rom"
  cp -f "$out_cart" "$BUILD_DIR/TUTORIAL_ROM2_NONMAPPER.rom"
  cp -f "$out_report" "$BUILD_DIR/TUTORIAL_ROM2_pack_report.json"

  print_header "Done"
  echo "Artifacts:"
  echo "- $BUILD_DIR/TUTORIAL_ROM2.rom"
  echo "- $BUILD_DIR/TUTORIAL_ROM2_NONMAPPER.rom"
  echo "- $BUILD_DIR/TUTORIAL_ROM2_pack_report.json"
}

run_openmsx() {
  local rom="${1:-$BUILD_DIR/TUTORIAL_ROM2_NONMAPPER.rom}"
  [[ -f "$rom" ]] || { echo "Error: missing ROM: $rom" >&2; exit 1; }

  local openmsx="$ROOT_DIR/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx"
  [[ -x "$openmsx" ]] || { echo "Error: openMSX not found: $openmsx" >&2; exit 1; }

  DISPLAY="${DISPLAY:-:1}" \
  OPENMSX_SYSTEM_DATA="${OPENMSX_SYSTEM_DATA:-$HOME/.openMSX/share}" \
  OPENMSX_DISABLE_SDL_JOYSTICK=1 \
  SDL_AUDIODRIVER=dummy \
  "$openmsx" \
  -machine Panasonic_FS-A1GT \
  -carta "$rom" \
  -romtype normal
}

usage() {
  cat <<USAGE
Usage: ./compile.sh <cmd>
  clean          Remove build outputs
  build          Build ROM with BLMKRULE-generated .MK
  all            clean + build
  run-openmsx    Run openMSX with built NONMAPPER ROM
USAGE
}

cmd="${1:-all}"
case "$cmd" in
  clean)
    clean
    ;;
  build)
    build
    ;;
  all)
    clean
    build
    ;;
  run-openmsx)
    run_openmsx "${2:-}"
    ;;
  *)
    usage
    exit 1
    ;;
esac
