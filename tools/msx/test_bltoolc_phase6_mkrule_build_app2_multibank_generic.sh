#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BLTOOLC="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c/bltoolc"
TMP="/tmp/bltoolc_phase6_app2_multibank_generic"
EX="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/examples/sharksym/2HELLO"

rm -rf "$TMP"
mkdir -p "$TMP"

cat > "$TMP/APP2MB.CFG" <<CFG
###
# target
#
target = APP2MB

tool-mode = 1
app-mode = 2
rom-mode = 0
tsr-mode = 0
lib-float = 0
lib-r800 = 0

src-bank00 += \\

lib-bank00 += \\
    \$(LIBC) \\

src-bank01 += \\
    $EX/MAIN.C \\

lib-bank01 += \\
    \$(LIBC) \\

src-bank02 += \\
    $EX/SUB.C \\

lib-bank02 += \\
    \$(LIBC) \\
CFG

"$BLTOOLC" mkrule \
  "$TMP/APP2MB.CFG" \
  "$TMP/APP2MB.MK" >/tmp/bltoolc_phase6_app2_multibank_generic_mkrule.log 2>&1

"$BLTOOLC" mkrule-build \
  --manifest "$TMP/APP2MB.MK.json" >/tmp/bltoolc_phase6_app2_multibank_generic_build.log 2>&1

python3 - <<'PY'
import json, sys
from pathlib import Path
base = Path('/tmp/bltoolc_phase6_app2_multibank_generic/APP2MB.OUT')
summary = json.loads((base / 'build_summary.json').read_text())
if summary.get('mode') != 'app2-multibank-baseline':
    print(f"unexpected mode={summary.get('mode')!r}", file=sys.stderr)
    sys.exit(1)
status = summary.get('status')
if status not in ('pass', 'partial'):
    print(f"unexpected status={status!r}", file=sys.stderr)
    sys.exit(1)
banks = summary.get('banks')
if not isinstance(banks, list) or len(banks) < 2:
    print(f"unexpected banks={banks!r}", file=sys.stderr)
    sys.exit(1)
print(f"[test-bltoolc-phase6-app2-multibank-generic] summary OK (status={status}, banks={len(banks)})")
PY

echo "[test-bltoolc-phase6-app2-multibank-generic] PASS"
