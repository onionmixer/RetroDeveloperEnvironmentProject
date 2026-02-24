#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="${WORK:-/tmp/bltoolc_phase3_optim}"

E0="/tmp/sharksym_phase6_smoke/ext_bank00.json"
E1="/tmp/sharksym_phase6_smoke/ext_bank01.json"
E2="/tmp/sharksym_phase6_smoke/ext_bank02.json"
MAP="/tmp/sharksym_phase6_smoke/optim_link.map"

for f in "$E0" "$E1" "$E2" "$MAP"; do
  if [[ ! -f "$f" ]]; then
    echo "[test-bltoolc-phase3-optim] missing input: $f" >&2
    echo "[hint] run tools/msx/run_phase6_smoke.sh first" >&2
    exit 1
  fi
done

rm -rf "$WORK"
mkdir -p "$WORK"

cd "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c"
make >/dev/null
cd "$ROOT"

python3 Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool.py optim \
  --extref "$E0" --extref "$E1" --extref "$E2" \
  --resolve-map "$MAP" \
  --root start --root _main \
  -o "$WORK/py_graph.json" \
  --write-response "$WORK/py.rsp" >/dev/null

Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c/bltoolc optim \
  --extref "$E0" --extref "$E1" --extref "$E2" \
  --resolve-map "$MAP" \
  --root start --root _main \
  -o "$WORK/c_graph.json" \
  --write-response "$WORK/c.rsp" >/dev/null

python3 - "$WORK/py_graph.json" "$WORK/c_graph.json" "$WORK/py.rsp" "$WORK/c.rsp" <<'PY'
import json,sys
py=json.load(open(sys.argv[1], 'r', encoding='utf-8'))
c=json.load(open(sys.argv[2], 'r', encoding='utf-8'))
keys=[
  'tool','mode','roots','resolve_maps','forced_modules','kept_modules','removed_modules',
  'resolved_by_prefix_symbols','resolved_by_map_symbols','unresolved_symbols','kept_real_files','kept_virtual_modules'
]
for k in keys:
    assert py.get(k)==c.get(k), f'mismatch {k}'
assert open(sys.argv[3], 'rb').read()==open(sys.argv[4], 'rb').read(), 'response mismatch'
print('[test-bltoolc-phase3-optim] graph parity OK')
PY

set +e
python3 Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool.py optim \
  --extref "$E0" --extref "$E1" --extref "$E2" \
  --strict-unresolved \
  --root start --root _main \
  -o "$WORK/py_strict.json" >/dev/null 2>&1
py_rc=$?
Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c/bltoolc optim \
  --extref "$E0" --extref "$E1" --extref "$E2" \
  --strict-unresolved \
  --root start --root _main \
  -o "$WORK/c_strict.json" >/dev/null 2>&1
c_rc=$?
set -e

if [[ "$py_rc" -ne 2 || "$c_rc" -ne 2 ]]; then
  echo "[test-bltoolc-phase3-optim] strict rc mismatch (py=$py_rc c=$c_rc)" >&2
  exit 1
fi

echo "[test-bltoolc-phase3-optim] strict rc parity OK (2)"
echo "[test-bltoolc-phase3-optim] PASS"
