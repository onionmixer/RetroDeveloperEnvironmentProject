#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="${WORK:-/tmp/bltoolc_phase3_caller}"
COMM="/tmp/sharksym_phase6_smoke/comm.json"

if [[ ! -f "$COMM" ]]; then
  echo "[test-bltoolc-phase3-caller] missing comm json: $COMM" >&2
  echo "[hint] run tools/msx/run_phase6_smoke.sh first" >&2
  exit 1
fi

rm -rf "$WORK"
mkdir -p "$WORK"

cd "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c"
make >/dev/null

python3 "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool.py" caller --comm-json "$COMM" -o "$WORK/py.as" >/dev/null
./bltoolc caller --comm-json "$COMM" -o "$WORK/c.as" >/dev/null

python3 - "$WORK/py.as" "$WORK/c.as" <<'PY'
import sys
py=open(sys.argv[1], 'r', encoding='utf-8').read().strip().splitlines()
c=open(sys.argv[2], 'r', encoding='utf-8').read().strip().splitlines()
assert py == c, 'caller asm mismatch'
print('[test-bltoolc-phase3-caller] asm parity OK')
PY

set +e
python3 "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool.py" caller --comm-json "$COMM" --strict -o "$WORK/py_strict.as" >/dev/null 2>&1
py_rc=$?
./bltoolc caller --comm-json "$COMM" --strict -o "$WORK/c_strict.as" >/dev/null 2>&1
c_rc=$?
set -e

if [[ "$py_rc" -ne 2 || "$c_rc" -ne 2 ]]; then
  echo "[test-bltoolc-phase3-caller] strict rc mismatch (py=$py_rc c=$c_rc)" >&2
  exit 1
fi

echo "[test-bltoolc-phase3-caller] strict rc parity OK (2)"
echo "[test-bltoolc-phase3-caller] PASS"
