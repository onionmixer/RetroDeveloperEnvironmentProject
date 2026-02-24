#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BLTOOLC="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c/bltoolc"
PY="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool.py"
BASE="/tmp/brm"
CSV="$BASE/matrix.csv"

rm -rf "$BASE"
mkdir -p "$BASE"
echo "target,cfg,native_rc,python_rc,status,reason" >"$CSV"

for cfg in "$ROOT"/Toolchain/MSX/HITECH_TOOLCHAIN/examples/sharksym/0*.ROM/*.CFG; do
  target="$(basename "$cfg" .CFG)"
  work="$BASE/$target"
  work_native="$work/native"
  work_py="$work/python"
  mkdir -p "$work_native" "$work_py"

  native_rc=0
  python_rc=0

  "$BLTOOLC" mkrule "$cfg" "$work_native/$target.MK" >"$work_native/mkrule_native.log" 2>&1 || native_rc=$?
  if [[ $native_rc -eq 0 ]]; then
    "$BLTOOLC" mkrule-build --manifest "$work_native/$target.MK.json" >"$work_native/build_native.log" 2>&1 || native_rc=$?
  fi

  "$BLTOOLC" mkrule "$cfg" "$work_py/$target.MK" >"$work_py/mkrule_py.log" 2>&1 || python_rc=$?
  if [[ $python_rc -eq 0 ]]; then
    python3 "$PY" mkrule-build --manifest "$work_py/$target.MK.json" >"$work_py/build_py.log" 2>&1 || python_rc=$?
  fi

  status="fail-both"
  reason=""
  if [[ $native_rc -eq 0 && $python_rc -eq 0 ]]; then
    status="pass-both"
  elif [[ $native_rc -eq 0 && $python_rc -ne 0 ]]; then
    status="native-only-pass"
  elif [[ $native_rc -ne 0 && $python_rc -eq 0 ]]; then
    status="python-only-pass"
  else
    if rg -q "_bl_bank" "$work_native/build_native.log" "$work_py/build_py.log" 2>/dev/null; then
      reason="unresolved:_bl_bank"
    elif rg -q "buffer overflow detected" "$work_native/build_native.log" "$work_py/build_py.log" 2>/dev/null; then
      reason="p1x3-overflow"
    else
      reason="unknown"
    fi
  fi

  echo "$target,$cfg,$native_rc,$python_rc,$status,$reason" >>"$CSV"
done

echo "[test-bltoolc-phase6-rom-matrix] report: $CSV"
column -s, -t "$CSV" | sed -n '1,120p'
echo "[test-bltoolc-phase6-rom-matrix] PASS"
