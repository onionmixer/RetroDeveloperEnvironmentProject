#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="${WORK:-/tmp/bltoolc_phase3_comm}"

B0="/tmp/sharksym_phase6_smoke/ext_bank00.json"
B1="/tmp/sharksym_phase6_smoke/ext_bank01.json"
B2="/tmp/sharksym_phase6_smoke/ext_bank02.json"

for f in "$B0" "$B1" "$B2"; do
  if [[ ! -f "$f" ]]; then
    echo "[test-bltoolc-phase3-comm] missing extref json: $f" >&2
    echo "[hint] run tools/msx/run_phase6_smoke.sh first" >&2
    exit 1
  fi
done

rm -rf "$WORK"
mkdir -p "$WORK"

cd "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c"
make >/dev/null

python3 "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool.py" comm \
  --bank bank00="$B0" \
  --bank bank01="$B1" \
  --bank bank02="$B2" \
  -o "$WORK/py_comm.json" >/dev/null

./bltoolc comm \
  --bank bank00="$B0" \
  --bank bank01="$B1" \
  --bank bank02="$B2" \
  -o "$WORK/c_comm.json" >/dev/null

python3 - "$WORK/py_comm.json" "$WORK/c_comm.json" <<'PY'
import json,sys
py=json.load(open(sys.argv[1], 'r', encoding='utf-8'))
c=json.load(open(sys.argv[2], 'r', encoding='utf-8'))
assert py.get('tool') == c.get('tool') == 'BLCOMM'
assert py.get('banks') == c.get('banks')
assert py.get('shared_refs') == c.get('shared_refs')
def norm(arr):
    return sorted(arr, key=lambda r:(r['symbol'], tuple(r['defined_in']), tuple(r['referenced_in'])))
assert norm(py.get('cross_bank_symbols', [])) == norm(c.get('cross_bank_symbols', []))
print('[test-bltoolc-phase3-comm] parity OK')
PY

echo "[test-bltoolc-phase3-comm] PASS"
