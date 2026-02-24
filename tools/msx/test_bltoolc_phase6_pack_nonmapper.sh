#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BLTOOLC="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c/bltoolc"
PY_PACKER="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/PACKING/msx_hitech_nonmapper_pack.py"
TMP="/tmp/bltoolc_phase6_pack"

rm -rf "$TMP"
mkdir -p "$TMP"

python3 - <<'PY'
from pathlib import Path
p = Path("/tmp/bltoolc_phase6_pack/in.rom")
raw = bytearray([0xFF] * (48 * 1024))
raw[0x0100:0x0104] = b"ROM "
p.write_bytes(raw)
PY

run_case() {
  local name="$1"; shift
  python3 "$PY_PACKER" \
    --input "$TMP/in.rom" \
    --output "$TMP/py_out_${name}.rom" \
    --report "$TMP/py_report_${name}.json" \
    "$@" >/tmp/bltoolc_phase6_pack_py_${name}.log 2>&1

  "$BLTOOLC" pack-nonmapper \
    --input "$TMP/in.rom" \
    --output "$TMP/c_out_${name}.rom" \
    --report "$TMP/c_report_${name}.json" \
    "$@" >/tmp/bltoolc_phase6_pack_c_${name}.log 2>&1

  cmp -s "$TMP/py_out_${name}.rom" "$TMP/c_out_${name}.rom"
}

run_case safe --profile safe-no-loader --init startup --no-loader --strict
run_case page2 --profile loader-page2 --strict
run_case legacy --profile legacy-loader --strict

python3 - <<'PY'
import json, sys
from pathlib import Path
cases = ["safe", "page2", "legacy"]
for name in cases:
    py = json.loads(Path(f"/tmp/bltoolc_phase6_pack/py_report_{name}.json").read_text())
    c = json.loads(Path(f"/tmp/bltoolc_phase6_pack/c_report_{name}.json").read_text())
    keys = [
        "tool", "size", "rom_signature_at_0x0100_ascii", "rom_signature_at_0x0100_hex",
        "startup_addr", "has_rom_signature", "profile", "map_page0", "loader_injected",
        "loader_variant", "loader_size", "init_shim", "force_overwrite", "loader_addr"
    ]
    for k in keys:
        if py.get(k) != c.get(k):
            print(f"{name}:mismatch:{k}: py={py.get(k)!r} c={c.get(k)!r}", file=sys.stderr)
            sys.exit(1)
    if py["ab_header"]["sig"] != c["ab_header"]["sig"] or py["ab_header"]["init"] != c["ab_header"]["init"]:
        print(f"{name}:mismatch:ab_header", file=sys.stderr)
        sys.exit(1)
print("[test-bltoolc-phase6-pack] parity OK (safe/page2/legacy)")
PY

echo "[test-bltoolc-phase6-pack] PASS"
