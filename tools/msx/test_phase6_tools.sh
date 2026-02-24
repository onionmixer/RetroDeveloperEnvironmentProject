#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="${WORK:-/tmp/sharksym_phase6_smoke}"

echo "[test-phase6] run smoke pipeline"
"$ROOT/tools/msx/run_phase6_smoke.sh" >/tmp/test_phase6_smoke.log 2>&1
tail -n 20 /tmp/test_phase6_smoke.log

echo "[test-phase6] verify outputs"
for f in \
  "$WORK/2HELLO.MK" \
  "$WORK/2HELLO.MK.json" \
  "$WORK/ext_bank00.json" \
  "$WORK/ext_bank01.json" \
  "$WORK/ext_bank02.json" \
  "$WORK/ext_libcmsx.json" \
  "$WORK/comm.json" \
  "$WORK/blcaller.as" \
  "$WORK/merged.bin" \
  "$WORK/merged.bin.tbl" \
  "$WORK/optim_passthrough.com" \
  "$WORK/optim_graph.json" \
  "$WORK/optim_keep.rsp" \
  "$WORK/subset_lib/subset_lib_manifest.json" \
  "$WORK/subset_lib/LIBCMSX.subset.lib" \
  "$WORK/optim_link.com" \
  "$WORK/optim_link.map"
do
  if [[ ! -f "$f" ]]; then
    echo "[test-phase6] missing output: $f" >&2
    exit 1
  fi
done

echo "[test-phase6] verify comm.json semantics"
python3 - "$WORK/comm.json" <<'PY'
import json, sys
p = sys.argv[1]
obj = json.load(open(p, "r", encoding="utf-8"))
assert "cross_bank_symbols" in obj, "missing cross_bank_symbols"
syms = [x.get("symbol") for x in obj["cross_bank_symbols"] if isinstance(x, dict)]
assert "_main" in syms, "expected _main cross-bank symbol"
print("[test-phase6] comm.json OK")
PY

echo "[test-phase6] verify caller offset resolution"
python3 - "$WORK/blcaller.as" <<'PY'
import re, sys
txt = open(sys.argv[1], "r", encoding="utf-8").read()
m = re.search(r"_blcalloff_main:\s*\n\s*defw\s+([0-9A-Fa-f]+)h", txt, re.MULTILINE)
assert m, "missing _blcalloff_main"
assert int(m.group(1), 16) != 0, "_blcalloff_main unresolved (0)"
print("[test-phase6] caller offset OK")
PY

echo "[test-phase6] verify optim graph report"
python3 - "$WORK/optim_graph.json" <<'PY'
import json, sys
obj = json.load(open(sys.argv[1], "r", encoding="utf-8"))
assert obj.get("tool") == "BLOPTIM", "invalid tool"
kept = obj.get("kept_modules", [])
removed = obj.get("removed_modules", [])
resolved_by_map = obj.get("resolved_by_map_symbols", [])
assert isinstance(kept, list) and len(kept) >= 1, "expected kept modules"
assert isinstance(removed, list), "expected removed modules list"
assert isinstance(resolved_by_map, list), "expected resolved_by_map_symbols list"
print("[test-phase6] optim graph OK")
PY

echo "[test-phase6] verify subset lib manifest"
python3 - "$WORK/subset_lib/subset_lib_manifest.json" <<'PY'
import json, sys, pathlib
obj = json.load(open(sys.argv[1], "r", encoding="utf-8"))
libs = obj.get("subset_libs", [])
assert isinstance(libs, list) and len(libs) >= 1, "no subset libs"
item = libs[0]
assert item["built_modules"] >= 1, "subset lib has no modules"
assert item["subset_size"] < item["source_size"], "subset lib is not smaller than source"
assert pathlib.Path(item["subset_lib"]).exists(), "subset lib file missing"
print("[test-phase6] subset lib OK")
PY

echo "[test-phase6] verify optim strict negative case (without resolve-map)"
set +e
python3 "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool.py" optim \
  --extref "$WORK/ext_bank00.json" \
  --extref "$WORK/ext_bank01.json" \
  --extref "$WORK/ext_bank02.json" \
  --strict-unresolved \
  --root start --root _main \
  -o "$WORK/optim_graph_strict_fail.json" \
  >/tmp/out_test_phase6_optim_strict_neg.log 2>&1
RC=$?
set -e
if [[ $RC -ne 2 ]]; then
  echo "[test-phase6] expected RC=2, got RC=$RC" >&2
  sed -n '1,120p' /tmp/out_test_phase6_optim_strict_neg.log >&2
  exit 1
fi
echo "[test-phase6] optim strict negative OK (RC=2)"

echo "[test-phase6] verify optim link map symbols"
python3 - "$WORK/optim_link.map" <<'PY'
import re, sys
txt = open(sys.argv[1], "r", encoding="utf-8", errors="replace").read()
assert "Symbol Table" in txt, "missing Symbol Table section"
for sym in ("_main", "start"):
    if not re.search(rf"\b{re.escape(sym)}\b\s+(?:text|data|bss|comm|abs)\s+[0-9A-Fa-f]{{3,6}}", txt):
        raise AssertionError(f"missing symbol in map: {sym}")
print("[test-phase6] optim link map symbols OK")
PY

if [[ "${ENABLE_OPENMSX_SMOKE:-0}" == "1" ]]; then
  echo "[test-phase6] verify openmsx phase6 report"
  if [[ ! -f "$WORK/openmsx_phase6_report.json" ]]; then
    echo "[test-phase6] missing openmsx report: $WORK/openmsx_phase6_report.json" >&2
    exit 1
  fi
  python3 - "$WORK/openmsx_phase6_report.json" <<'PY'
import json, sys
obj = json.load(open(sys.argv[1], "r", encoding="utf-8"))
assert obj.get("tool") == "openmsx_phase6_optim_smoke", "invalid report tool"
status = obj.get("status")
assert status in ("pass", "skip"), f"unexpected openmsx status: {status}"
print(f"[test-phase6] openmsx report OK (status={status})")
PY
fi

if [[ "${ENABLE_OPENMSX_GUI_PRECHECK:-0}" == "1" ]]; then
  echo "[test-phase6] run openmsx GUI precheck"
  GUI_REPORT="$WORK/openmsx_gui_precheck_report.json"
  REPORT="$GUI_REPORT" "$ROOT/tools/msx/openmsx_gui_precheck.sh"
  python3 - "$GUI_REPORT" <<'PY'
import json, sys
obj = json.load(open(sys.argv[1], "r", encoding="utf-8"))
assert obj.get("tool") == "openmsx_gui_precheck", "invalid GUI precheck tool"
status = obj.get("status")
assert status in ("pass", "skip", "fail"), f"unexpected GUI precheck status: {status}"
if status == "fail":
    raise AssertionError(f"GUI precheck failed: reason={obj.get('reason')}")
print(f"[test-phase6] GUI precheck OK (status={status}, reason={obj.get('reason')})")
PY
fi

if [[ "${ENABLE_SHARKSYM_LIB_COMPILE_TEST:-0}" == "1" ]]; then
  echo "[test-phase6] run sharksym library compile test"
  WORK="/tmp/sharksym_lib_compile_test" "$ROOT/tools/msx/test_sharksym_lib_compile.sh"
  if [[ ! -f /tmp/sharksym_lib_compile_test/summary.tsv ]]; then
    echo "[test-phase6] missing compile summary: /tmp/sharksym_lib_compile_test/summary.tsv" >&2
    exit 1
  fi
  python3 - <<'PY'
import csv
path = "/tmp/sharksym_lib_compile_test/summary.tsv"
rows = list(csv.DictReader(open(path, "r", encoding="utf-8"), delimiter="\t"))
assert rows, "empty compile summary"
for r in rows:
    if r["obj"] != "1":
        raise AssertionError(f"compile obj missing: {r['file']}")
print(f"[test-phase6] sharksym lib compile summary OK ({len(rows)} files)")
PY
fi

echo "[test-phase6] PASS"
