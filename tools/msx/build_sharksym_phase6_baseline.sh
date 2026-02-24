#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HITECH="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN"
BIN="$HITECH/bin"
SHARKSYM="$HITECH/lib/CPMEMU_HI-TECH_C"
EX0="$HITECH/examples/sharksym/0HELLO"

WORK="${WORK:-/tmp/sharksym_phase6_baseline}"
OUT="$WORK/out"
INC_LC="$WORK/include_sharksym_lc"
REPORT="${REPORT:-$WORK/build_report.json}"
USE_OPTIM="${USE_OPTIM:-0}"

mkdir -p "$OUT" "$INC_LC"

for h in "$SHARKSYM"/*.H; do
  bn="$(basename "$h")"
  cp "$h" "$INC_LC/$(echo "$bn" | tr 'A-Z' 'a-z')"
done

CPP_FLAGS=(-I"$INC_LC" -I"$SHARKSYM" -I"$HITECH/include/hitechc" -I"$HITECH/include/msx")
DEFINES=(-DCPM -DANSI -DBL_DISABLE -DBL_DOS1)

compile_obj() {
  local src="$1"
  local base="$2"
  shift 2
  local extra_defs=("$@")

  "$BIN/cpp_new3" -P "${DEFINES[@]}" "${extra_defs[@]}" "${CPP_FLAGS[@]}" "$src" "$OUT/$base.i"
  "$BIN/p1x3" "$OUT/$base.i" "$OUT/$base.p1"
  "$BIN/cgen3" "$OUT/$base.p1" "$OUT/$base.as"

  if [[ "$base" == "BLCRT" || "$base" == "BLGRP" ]]; then
    sed -i 's/-32768/32768/g' "$OUT/$base.as"
  fi

  if [[ "$USE_OPTIM" == "1" ]]; then
    set +e
    timeout 8s "$BIN/optim3" "$OUT/$base.as" "$OUT/$base.asm" >/tmp/phase6_base_optim_"$base".log 2>&1
    local optim_rc=$?
    set -e
    if [[ $optim_rc -eq 0 ]]; then
      (cd "$OUT" && "$BIN/zasx3" -j "$base.asm" >/dev/null 2>&1)
      return
    fi
  fi

  (cd "$OUT" && "$BIN/zasx3" -j "$base.as" >/dev/null 2>&1)
}

echo "[phase6-baseline] compile objects"
compile_obj "$SHARKSYM/BLCRT.C" BLCRT
compile_obj "$SHARKSYM/BL.C" BL
compile_obj "$EX0/MAIN.C" MAIN
compile_obj "$SHARKSYM/BLSND.C" BLSND

echo "[phase6-baseline] link TEST_MINP4.COM"
set +e
"$BIN/linq3" -z -ptext=100H,data,bss -m"$OUT/TEST_MINP4.map" -o"$OUT/TEST_MINP4.COM" \
  "$OUT/BLCRT.obj" "$OUT/BL.obj" "$OUT/MAIN.obj" "$SHARKSYM/LIBCMSX.LIB" \
  >/tmp/phase6_base_link.log 2>&1
link_rc=$?
set -e
if [[ $link_rc -ne 0 && ! -s "$OUT/TEST_MINP4.COM" ]]; then
  echo "[phase6-baseline] link failed rc=$link_rc" >&2
  sed -n '1,120p' /tmp/phase6_base_link.log >&2 || true
  exit 1
fi

python3 - "$REPORT" "$OUT" "$link_rc" "$USE_OPTIM" <<'PY'
import json, os, sys
report, out_dir, link_rc, use_optim = sys.argv[1:]
files = ["BLCRT.obj", "BL.obj", "MAIN.obj", "BLSND.obj", "TEST_MINP4.COM", "TEST_MINP4.map"]
obj = {
  "tool": "build_sharksym_phase6_baseline",
  "status": "pass",
  "out_dir": out_dir,
  "use_optim": bool(int(use_optim)),
  "link_rc": int(link_rc),
  "outputs": {f: os.path.join(out_dir, f) for f in files},
}
for f in files:
    p = os.path.join(out_dir, f)
    if not os.path.exists(p):
        obj["status"] = "fail"
        obj["missing"] = f
        break
with open(report, "w", encoding="utf-8") as fp:
    json.dump(obj, fp, indent=2, ensure_ascii=True)
    fp.write("\n")
print(f"[phase6-baseline] report={report} status={obj['status']}")
PY

if [[ ! -s "$OUT/BLCRT.obj" || ! -s "$OUT/BL.obj" || ! -s "$OUT/MAIN.obj" || ! -s "$OUT/BLSND.obj" ]]; then
  echo "[phase6-baseline] missing required object output" >&2
  exit 1
fi

echo "[phase6-baseline] done: $OUT"
