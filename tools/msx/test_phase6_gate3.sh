#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
EXAMPLE="${EXAMPLE:-2HELLO}"
EXAMPLE_VARIANT="${EXAMPLE_VARIANT:-}"
TARGET_TAG="$EXAMPLE"
if [[ "$EXAMPLE" == "2ASSERT" && "$EXAMPLE_VARIANT" == "NDEBUG" ]]; then
  TARGET_TAG="2ASSERTN"
fi
if [[ -z "${WORK:-}" ]]; then
  if [[ "$TARGET_TAG" == "2HELLO" ]]; then
    WORK="/tmp/sharksym_phase6_gate3"
  else
    WORK="/tmp/sharksym_phase6_gate3_${TARGET_TAG}"
  fi
fi

echo "[test-gate3] run app-mode2 gate3 pipeline"
EXAMPLE="$EXAMPLE" EXAMPLE_VARIANT="$EXAMPLE_VARIANT" "$ROOT/tools/msx/run_phase6_gate3_app2.sh" >/tmp/test_phase6_gate3.log 2>&1
tail -n 30 /tmp/test_phase6_gate3.log

echo "[test-gate3] verify warning-free gate3 log"
if rg -n "^\[gate3\] warning:" /tmp/test_phase6_gate3.log >/tmp/test_phase6_gate3_warn.log 2>&1; then
  echo "[test-gate3] unexpected gate3 warning detected" >&2
  cat /tmp/test_phase6_gate3_warn.log >&2
  exit 1
fi

echo "[test-gate3] verify required outputs"
for f in \
  "$WORK/bank00.com" \
  "$WORK/bank01.com" \
  "$WORK/bank00.map" \
  "$WORK/bank01_final.map" \
  "$WORK/comm.json" \
  "$WORK/gate3_report.json" \
  "$WORK/out/blcaller.as" \
  "$WORK/out/blcaller.obj" \
  "$WORK/${TARGET_TAG}_app2_merged.bin" \
  "$WORK/${TARGET_TAG}_app2_merged.bin.tbl"
do
  if [[ ! -f "$f" ]]; then
    echo "[test-gate3] missing output: $f" >&2
    exit 1
  fi
done
if [[ -f "$WORK/bank02.com" ]]; then
  for f in "$WORK/bank02.com" "$WORK/bank02_final.map"; do
    if [[ ! -f "$f" ]]; then
      echo "[test-gate3] missing optional bank02 output: $f" >&2
      exit 1
    fi
  done
fi

echo "[test-gate3] verify comm path (_main -> _sub)"
python3 - "$WORK/comm.json" <<'PY'
import json, sys
obj = json.load(open(sys.argv[1], "r", encoding="utf-8"))
xs = obj.get("cross_bank_symbols", [])
sym_map = {x.get("symbol"): x for x in xs if isinstance(x, dict)}
assert "_main" in sym_map, "missing _main cross-bank symbol"
assert "bank00" in sym_map["_main"].get("referenced_in", []), "_main not referenced by bank00"
if "_sub" in sym_map:
    assert "bank01" in sym_map["_sub"].get("referenced_in", []), "_sub not referenced by bank01"
print("[test-gate3] comm path OK")
PY

echo "[test-gate3] verify map symbols"
python3 - "$WORK/bank00.map" "$WORK/bank01_final.map" "$WORK/bank02_final.map" "$WORK/comm.json" <<'PY'
import re, sys
import json, os
checks = [(sys.argv[1], ["_main", "start"]), (sys.argv[2], ["_main"])]
comm = json.load(open(sys.argv[4], "r", encoding="utf-8"))
need_sub = False
for x in comm.get("cross_bank_symbols", []):
    if isinstance(x, dict) and x.get("symbol") == "_sub":
        need_sub = True
        break
if need_sub and os.path.exists(sys.argv[3]):
    checks.append((sys.argv[3], ["_sub"]))
for path, syms in checks:
    txt = open(path, "r", encoding="utf-8", errors="replace").read()
    assert "Symbol Table" in txt, f"missing Symbol Table: {path}"
    for sym in syms:
        if not re.search(rf"\b{re.escape(sym)}\b\s+\(?\s*(?:text|data|bss|comm|abs)\s*\)?\s+[0-9A-Fa-f]{{3,6}}", txt):
            raise AssertionError(f"missing symbol {sym} in {path}")
print("[test-gate3] map symbols OK")
PY

echo "[test-gate3] verify gate3 report"
python3 - "$WORK/gate3_report.json" <<'PY'
import json, sys
obj = json.load(open(sys.argv[1], "r", encoding="utf-8"))
assert obj.get("tool") == "phase6_gate3_app2", "invalid gate3 report tool"
assert obj.get("status") == "pass", "gate3 report status is not pass"
assert obj.get("mode") == "native", "unexpected gate3 mode"
assert obj.get("example") is not None, "missing example field"
assert obj.get("target_tag") is not None, "missing target_tag field"
print("[test-gate3] gate3 report OK")
PY

if [[ "${ENABLE_OPENMSX_SMOKE:-0}" == "1" ]]; then
  echo "[test-gate3] verify openmsx gate3 report"
  if [[ ! -f "$WORK/openmsx_gate3_report.json" ]]; then
    echo "[test-gate3] missing openmsx report: $WORK/openmsx_gate3_report.json" >&2
    exit 1
  fi
  python3 - "$WORK/openmsx_gate3_report.json" <<'PY'
import json, sys
obj = json.load(open(sys.argv[1], "r", encoding="utf-8"))
assert obj.get("tool") == "openmsx_phase6_optim_smoke", "invalid report tool"
status = obj.get("status")
assert status in ("pass", "skip"), f"unexpected openmsx status: {status}"
print(f"[test-gate3] openmsx report OK (status={status})")
PY
fi

echo "[test-gate3] PASS"
