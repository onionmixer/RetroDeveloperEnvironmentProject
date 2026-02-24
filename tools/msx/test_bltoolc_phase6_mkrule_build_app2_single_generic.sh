#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BLTOOLC="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c/bltoolc"
TMP="/tmp/bltoolc_phase6_app2_single_generic"
SRC_MAIN="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/examples/sharksym/2HELLO/MAIN.C"

rm -rf "$TMP"
mkdir -p "$TMP"

cat > "$TMP/APP2ONE.CFG" <<CFG
###
# target name
#
target = APP2ONE

###
# tool mode
#
tool-mode = 1

###
# app mode
#
app-mode = 2

###
# rom mode (app2 baseline is non-ROM)
#
rom-mode = 0

###
# tsr mode
#
tsr-mode = 0

###
# float / r800
#
lib-float = 0
lib-r800 = 0

###
# bank00 only
#
src-bank00 += \\
    $SRC_MAIN \\

lib-bank00 += \\
    \$(LIBC) \\
CFG

"$BLTOOLC" mkrule \
  "$TMP/APP2ONE.CFG" \
  "$TMP/APP2ONE.MK" >/tmp/bltoolc_phase6_app2_single_generic_mkrule.log 2>&1

"$BLTOOLC" mkrule-build \
  --manifest "$TMP/APP2ONE.MK.json" >/tmp/bltoolc_phase6_app2_single_generic_build.log 2>&1

python3 - <<'PY'
import json, sys
from pathlib import Path
base = Path('/tmp/bltoolc_phase6_app2_single_generic/APP2ONE.OUT')
summary = json.loads((base / 'build_summary.json').read_text())
if summary.get('mode') != 'app2-single-baseline':
    print(f"unexpected mode={summary.get('mode')!r}", file=sys.stderr)
    sys.exit(1)
com = Path(str(summary.get('com', '')))
if not com.exists():
    print(f'missing com: {com}', file=sys.stderr)
    sys.exit(1)
if com.stat().st_size <= 0:
    print('empty com', file=sys.stderr)
    sys.exit(1)
print('[test-bltoolc-phase6-app2-single-generic] summary+com OK')
PY

echo "[test-bltoolc-phase6-app2-single-generic] PASS"
