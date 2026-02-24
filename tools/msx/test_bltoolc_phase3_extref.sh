#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="${WORK:-/tmp/bltoolc_phase3_extref}"

OBJ0="/tmp/sharksym_phase6_baseline/out/BLCRT.obj"
OBJ1="/tmp/sharksym_phase6_baseline/out/BL.obj"
LIB="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/lib/CPMEMU_HI-TECH_C/LIBCMSX.LIB"

if [[ ! -f "$OBJ0" || ! -f "$OBJ1" ]]; then
  echo "[test-bltoolc-phase3] missing baseline objs. run tools/msx/run_phase6_smoke.sh first." >&2
  exit 1
fi
if [[ ! -f "$LIB" ]]; then
  echo "[test-bltoolc-phase3] missing library: $LIB" >&2
  exit 1
fi

rm -rf "$WORK"
mkdir -p "$WORK"

cd "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c"
make >/dev/null

python3 "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool.py" extref -o "$WORK/py_obj.json" "$OBJ0" "$OBJ1" >/dev/null
./bltoolc extref -o "$WORK/c_obj.json" "$OBJ0" "$OBJ1" >/dev/null

python3 "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool.py" extref -o "$WORK/py_lib.json" "$LIB" >/dev/null
./bltoolc extref -o "$WORK/c_lib.json" "$LIB" >/dev/null

python3 - "$WORK/py_obj.json" "$WORK/c_obj.json" "$WORK/py_lib.json" "$WORK/c_lib.json" <<'PY'
import json,sys
py_obj=json.load(open(sys.argv[1], 'r', encoding='utf-8'))
c_obj=json.load(open(sys.argv[2], 'r', encoding='utf-8'))
py_lib=json.load(open(sys.argv[3], 'r', encoding='utf-8'))
c_lib=json.load(open(sys.argv[4], 'r', encoding='utf-8'))
assert py_obj == c_obj, 'obj extref json mismatch'
key=lambda r:(r['input'], tuple(r['defined']), tuple(r['extern_refs']))
assert sorted(py_lib['records'], key=key) == sorted(c_lib['records'], key=key), 'lib extref json mismatch'
print('[test-bltoolc-phase3] extref parity OK')
PY

echo "[test-bltoolc-phase3] PASS"
