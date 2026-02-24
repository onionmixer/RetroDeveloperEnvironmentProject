#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BLTOOLC="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c/bltoolc"
TMP="/tmp/bltoolc_phase6_app0_generic"

rm -rf "$TMP"
mkdir -p "$TMP"

"$BLTOOLC" mkrule \
  "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/lib/CPMEMU_HI-TECH_C/EXAMPLE/0HELLO/0HELLO.CFG" \
  "$TMP/0HELLO_LIBEX.MK" >/tmp/bltoolc_phase6_app0_generic_mkrule.log 2>&1

"$BLTOOLC" mkrule-build \
  --manifest "$TMP/0HELLO_LIBEX.MK.json" >/tmp/bltoolc_phase6_app0_generic_build.log 2>&1

python3 - <<'PY'
import json, sys
from pathlib import Path
base = Path("/tmp/bltoolc_phase6_app0_generic/0HELLO.OUT")
summary = json.loads((base / "build_summary.json").read_text())
if summary.get("mode") != "app0-baseline":
    print(f"unexpected mode={summary.get('mode')!r}", file=sys.stderr)
    sys.exit(1)
com = Path(str(summary.get("com", "")))
if not com.exists():
    print(f"missing com: {com}", file=sys.stderr)
    sys.exit(1)
if com.stat().st_size <= 0:
    print("empty com", file=sys.stderr)
    sys.exit(1)
print("[test-bltoolc-phase6-app0-generic] summary+com OK")
PY

echo "[test-bltoolc-phase6-app0-generic] PASS"
