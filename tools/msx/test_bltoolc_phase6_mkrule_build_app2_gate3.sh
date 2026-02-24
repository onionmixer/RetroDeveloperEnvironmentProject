#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BLTOOLC="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c/bltoolc"
TMP="/tmp/bltoolc_phase6_app2_gate3"
CFG="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/examples/sharksym/2HELLO/2HELLO.CFG"

rm -rf "$TMP"
mkdir -p "$TMP"

"$BLTOOLC" mkrule "$CFG" "$TMP/2HELLO.MK" >/tmp/bltoolc_phase6_app2_gate3_mkrule.log 2>&1
"$BLTOOLC" mkrule-build --manifest "$TMP/2HELLO.MK.json" >/tmp/bltoolc_phase6_app2_gate3_build.log 2>&1

python3 - <<'PY'
import json, sys
from pathlib import Path
base = Path('/tmp/bltoolc_phase6_app2_gate3/2HELLO.OUT')
summary = json.loads((base / 'build_summary.json').read_text())
if summary.get('mode') != 'gate3':
    print(f"unexpected mode={summary.get('mode')!r}", file=sys.stderr)
    sys.exit(1)
for key in ('gate3_report', 'comm_json', 'merged_bin'):
    p = Path(str(summary.get(key, '')))
    if not p.exists():
        print(f'missing {key}: {p}', file=sys.stderr)
        sys.exit(1)
print('[test-bltoolc-phase6-app2-gate3] summary+artifacts OK')
PY

echo "[test-bltoolc-phase6-app2-gate3] PASS"
