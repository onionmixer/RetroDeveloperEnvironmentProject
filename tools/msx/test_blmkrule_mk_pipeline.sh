#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="${WORK:-/tmp/blmkrule_mk_pipeline}"

CFG0="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/examples/sharksym/0HELLO/0HELLO.CFG"
CFG2="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/examples/sharksym/2HELLO/2HELLO.CFG"

rm -rf "$WORK"
mkdir -p "$WORK"

echo "[test-blmkrule] case1 app-mode0 build"
"$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLMKRULE" "$CFG0" "$WORK/0HELLO.MK"
make -f "$WORK/0HELLO.MK" build
python3 - "$WORK/0HELLO.OUT/build_summary.json" "$WORK/0HELLO.OUT/0HELLO.COM" <<'PY'
import json, pathlib, sys
summary = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
assert summary.exists(), f"missing summary: {summary}"
assert out.exists(), f"missing output: {out}"
obj = json.loads(summary.read_text(encoding="utf-8"))
assert obj.get("app_mode") == "0", "unexpected app_mode"
assert isinstance(obj.get("banks"), list) and len(obj["banks"]) >= 1, "missing bank info"
print("[test-blmkrule] case1 summary OK")
PY
make -f "$WORK/0HELLO.MK" cleanup

echo "[test-blmkrule] case2 app-mode2 gate3 build"
"$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLMKRULE" "$CFG2" "$WORK/2HELLO.MK"
make -f "$WORK/2HELLO.MK" build
python3 - "$WORK/2HELLO.OUT/build_summary.json" "$WORK/2HELLO.OUT/gate3_report.json" "$WORK/2HELLO.OUT/2HELLO_app2_merged.bin" <<'PY'
import json, pathlib, sys
summary = pathlib.Path(sys.argv[1])
gate3 = pathlib.Path(sys.argv[2])
merged = pathlib.Path(sys.argv[3])
assert summary.exists(), f"missing summary: {summary}"
obj = json.loads(summary.read_text(encoding="utf-8"))
assert obj.get("app_mode") == "2", "unexpected app_mode"
assert obj.get("mode") == "gate3", "expected gate3 mode for app-mode2 example cfg"
assert gate3.exists(), f"missing gate3 report: {gate3}"
assert merged.exists(), f"missing merged bin: {merged}"
print("[test-blmkrule] case2 summary OK")
PY
make -f "$WORK/2HELLO.MK" cleanup

echo "[test-blmkrule] PASS"
