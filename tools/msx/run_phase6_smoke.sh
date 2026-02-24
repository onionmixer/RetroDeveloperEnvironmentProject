#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CFG="${1:-$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/examples/sharksym/2HELLO/2HELLO.CFG}"
WORK="${WORK:-/tmp/sharksym_phase6_smoke}"
OBJDIR="${OBJDIR:-/tmp/sharksym_phase6_baseline/out}"
ZAS="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/zasx3"
LINQ="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/linq3"
SHARKSYM="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/lib/CPMEMU_HI-TECH_C"

mkdir -p "$WORK"

if [[ ! -s "$OBJDIR/BLCRT.obj" || ! -s "$OBJDIR/BL.obj" || ! -s "$OBJDIR/MAIN.obj" || ! -s "$OBJDIR/BLSND.obj" || ! -s "$OBJDIR/TEST_MINP4.COM" ]]; then
  echo "[phase6-smoke] prebuild: native baseline objects"
  WORK="$(dirname "$OBJDIR")" REPORT="$WORK/phase6_baseline_build_report.json" \
    "$ROOT/tools/msx/build_sharksym_phase6_baseline.sh"
fi

echo "[phase6-smoke] step1: mkrule"
"$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLMKRULE" "$CFG" "$WORK/2HELLO.MK"

echo "[phase6-smoke] step2: extref (bank00/01/02)"
"$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLEXTREF" -o "$WORK/ext_bank00.json" "$OBJDIR/BLCRT.obj" "$OBJDIR/BL.obj"
"$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLEXTREF" -o "$WORK/ext_bank01.json" "$OBJDIR/BL.obj" "$OBJDIR/MAIN.obj"
"$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLEXTREF" -o "$WORK/ext_bank02.json" "$OBJDIR/BL.obj" "$OBJDIR/BLSND.obj"
echo "[phase6-smoke] step2b: extref (library)"
"$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLEXTREF" --quiet -o "$WORK/ext_libcmsx.json" "$SHARKSYM/LIBCMSX.LIB"

echo "[phase6-smoke] step3: comm"
"$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLCOMM" -o "$WORK/comm.json" \
  --bank bank00="$WORK/ext_bank00.json" \
  --bank bank01="$WORK/ext_bank01.json" \
  --bank bank02="$WORK/ext_bank02.json"

echo "[phase6-smoke] step3b: map for bank01 symbol offsets"
"$LINQ" -z -ptext=100H,data,bss -m"$WORK/bank01.map" -o"$WORK/bank01.com" \
  "$OBJDIR/BLCRT.obj" "$OBJDIR/BL.obj" "$OBJDIR/MAIN.obj" "$SHARKSYM/LIBCMSX.LIB" >/dev/null 2>&1 || true

echo "[phase6-smoke] step4: caller"
"$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLCALLER" --comm-json "$WORK/comm.json" \
  --bank-map bank01="$WORK/bank01.map" \
  --strict \
  -o "$WORK/blcaller.as"
echo "[phase6-smoke] step4b: caller assemble"
(cd "$WORK" && "$ZAS" -j blcaller.as >/dev/null 2>&1)

echo "[phase6-smoke] step5: list + merge + optim"
"$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLLIST" "$OBJDIR/TEST_MINP4.COM"
"$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLMERGE" -o "$WORK/merged.bin" "$OBJDIR/TEST_MINP4.COM"
"$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLOPTIM" -o "$WORK/optim_passthrough.com" "$OBJDIR/TEST_MINP4.COM"

echo "[phase6-smoke] step5b: graph optim (extref-based)"
"$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLOPTIM" \
  --extref "$WORK/ext_bank00.json" \
  --extref "$WORK/ext_bank01.json" \
  --extref "$WORK/ext_bank02.json" \
  --extref "$WORK/ext_libcmsx.json" \
  --strict-unresolved \
  --root start --root _main \
  --write-response "$WORK/optim_keep.rsp" \
  --copy-dir "$WORK/optim_kept" \
  -o "$WORK/optim_graph.json"

echo "[phase6-smoke] step5c: build subset lib from optim report"
"$ROOT/tools/msx/build_subset_lib_from_optim.sh" "$WORK/optim_graph.json" "$WORK/subset_lib"

echo "[phase6-smoke] step5d: link from optim_keep.rsp + subset lib"
"$ROOT/tools/msx/link_phase6_from_rsp.sh" \
  "$WORK/optim_keep.rsp" \
  "$WORK/optim_link.com" \
  "$WORK/optim_link.map" \
  "$WORK/subset_lib/LIBCMSX.subset.lib"

if [[ "${ENABLE_OPENMSX_SMOKE:-0}" == "1" ]]; then
  echo "[phase6-smoke] step6: openMSX runtime smoke (optional)"
  REPORT="$WORK/openmsx_phase6_report.json" \
    "$ROOT/tools/msx/openmsx_phase6_optim_smoke.sh" "$WORK/optim_link.com"
fi

echo "[phase6-smoke] done: $WORK"
