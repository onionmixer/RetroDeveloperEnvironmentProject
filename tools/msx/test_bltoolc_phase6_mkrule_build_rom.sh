#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BLTOOLC="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c/bltoolc"
TMP="/tmp/bltoolc_phase6_mkrule_rom"

rm -rf "$TMP"
mkdir -p "$TMP"

run_case() {
  local cfg="$1"
  local stem="$2"
  "$BLTOOLC" mkrule "$cfg" "$TMP/$stem.MK" >/tmp/bltoolc_phase6_mkrule_rom_${stem}_mkrule.log 2>&1
  "$BLTOOLC" mkrule-build --manifest "$TMP/$stem.MK.json" >/tmp/bltoolc_phase6_mkrule_rom_${stem}_build.log 2>&1
}

run_case "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/examples/sharksym/0HELLO.ROM/0HELLO.CFG" 0HELLO_ROM
run_case "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/examples/sharksym/0HANGUL.ROM/0HANGUL.CFG" 0HANGUL_ROM

python3 - <<'PY'
import json, sys
from pathlib import Path
cases = [
    ("0HELLO", 32768),
    ("0HANGUL", 49152),
]
for target, expected_size in cases:
    base = Path("/tmp/bltoolc_phase6_mkrule_rom") / f"{target}.OUT"
    summary = json.loads((base / "build_summary.json").read_text())
    if summary.get("mode") != "app0-rom-baseline":
        print(f"{target}: unexpected mode={summary.get('mode')!r}", file=sys.stderr)
        sys.exit(1)
    rom = summary.get("rom_outputs", {})
    if rom.get("mode") != "app0-rom":
        print(f"{target}: bad rom mode", file=sys.stderr)
        sys.exit(1)
    if rom.get("pack_backend") != "bltoolc":
        print(f"{target}: unexpected pack_backend={rom.get('pack_backend')!r}", file=sys.stderr)
        sys.exit(1)
    if int(rom.get("rom_size", -1)) != expected_size:
        print(f"{target}: unexpected rom_size={rom.get('rom_size')}", file=sys.stderr)
        sys.exit(1)
    if not isinstance(rom.get("entry_main_fixup_patched"), bool):
        print(f"{target}: invalid entry_main_fixup_patched type", file=sys.stderr)
        sys.exit(1)
    for key in ("entry_symbol_fixups", "main_helper_call_fixups"):
        val = rom.get(key)
        if not isinstance(val, int) or val < 0:
            print(f"{target}: invalid {key}={val!r}", file=sys.stderr)
            sys.exit(1)
    cart = Path(str(rom.get("cart_rom", "")))
    if not cart.exists():
        print(f"{target}: missing cart rom: {cart}", file=sys.stderr)
        sys.exit(1)
    raw = cart.read_bytes()
    if len(raw) != expected_size:
        print(f"{target}: unexpected output size={len(raw)}", file=sys.stderr)
        sys.exit(1)
    if raw[:2] != b"AB":
        print(f"{target}: missing AB header", file=sys.stderr)
        sys.exit(1)
print("[test-bltoolc-phase6-mkrule-rom] summary+cart OK (0HELLO,0HANGUL)")
PY

echo "[test-bltoolc-phase6-mkrule-rom] PASS"
