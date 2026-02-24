#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HITECH="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN"
BIN="$HITECH/bin"
SHARKSYM="$HITECH/lib/CPMEMU_HI-TECH_C"

WORK="${WORK:-/tmp/sharksym_phase6_app1_baseline}"
OUT="$WORK"
INC_LC="$WORK/include_sharksym_lc"
REPORT="${REPORT:-$WORK/app1_baseline_report.json}"
TARGET="${TARGET:-APP1}"
CFG_DIR="${CFG_DIR:-$HITECH/examples/sharksym/1HELLO}"
SRC_LIST="${SRC_LIST:-BLCRT.C BL.C MAIN.C}"
LIB_LIST="${LIB_LIST:-LIBCMSX2.LIB}"
CFLAGS_DERIVED="${CFLAGS_DERIVED:--DANSI -DCPM -DDOS2ONLY -DBL_1BANK}"
PSECT_CFG="${PSECT_CFG:-,data,bss,comm=9400H}"

mkdir -p "$OUT" "$INC_LC"

for h in "$SHARKSYM"/*.H; do
  bn="$(basename "$h")"
  cp "$h" "$INC_LC/$(echo "$bn" | tr 'A-Z' 'a-z')"
done

CPP_FLAGS=(-I"$CFG_DIR" -I"$INC_LC" -I"$SHARKSYM" -I"$HITECH/include/hitechc" -I"$HITECH/include/msx")
read -r -a DEFINES <<<"$CFLAGS_DERIVED"

resolve_src() {
  local s="$1"
  if [[ -f "$s" ]]; then echo "$s"; return 0; fi
  if [[ -f "$CFG_DIR/$s" ]]; then echo "$CFG_DIR/$s"; return 0; fi
  if [[ -f "$SHARKSYM/$s" ]]; then echo "$SHARKSYM/$s"; return 0; fi
  return 1
}

resolve_lib() {
  local s="$1"
  if [[ -f "$s" ]]; then echo "$s"; return 0; fi
  if [[ -f "$CFG_DIR/$s" ]]; then echo "$CFG_DIR/$s"; return 0; fi
  if [[ -f "$SHARKSYM/$s" ]]; then echo "$SHARKSYM/$s"; return 0; fi
  return 1
}

compile_obj() {
  local src="$1"
  local base="$2"
  "$BIN/cpp_new3" -P "${DEFINES[@]}" "${CPP_FLAGS[@]}" "$src" "$OUT/$base.i"
  "$BIN/p1x3" "$OUT/$base.i" "$OUT/$base.p1"
  "$BIN/cgen3" "$OUT/$base.p1" "$OUT/$base.as"
  if [[ "$base" == "BLCRT" || "$base" == "BLGRP" ]]; then
    sed -i 's/-32768/32768/g' "$OUT/$base.as"
  fi
  (cd "$OUT" && "$BIN/zasx3" -j "$base.as" >/dev/null 2>&1)
}

echo "[phase6-app1-baseline] compile objects"
read -r -a SRC_ARR <<<"$SRC_LIST"
for src_name in "${SRC_ARR[@]}"; do
  src_path="$(resolve_src "$src_name")" || { echo "[phase6-app1-baseline] source not found: $src_name" >&2; exit 1; }
  base="$(basename "$src_name")"
  stem="${base%.*}"
  ext="${base##*.}"
  shopt -s nocasematch
  if [[ "$ext" == "c" ]]; then
    compile_obj "$src_path" "$stem"
  elif [[ "$ext" == "as" || "$ext" == "asm" ]]; then
    cp "$src_path" "$OUT/$base"
    (cd "$OUT" && "$BIN/zasx3" -j "$base" >/dev/null 2>&1)
  else
    echo "[phase6-app1-baseline] unsupported source extension: $src_name" >&2
    exit 1
  fi
  shopt -u nocasematch
done

objs=()
for src_name in "${SRC_ARR[@]}"; do
  base="$(basename "$src_name")"
  stem="${base%.*}"
  objs+=("$OUT/$stem.obj")
done

libs=()
if [[ -n "${LIB_LIST// /}" ]]; then
  read -r -a LIB_ARR <<<"$LIB_LIST"
  for lib_name in "${LIB_ARR[@]}"; do
    lib_path="$(resolve_lib "$lib_name")" || { echo "[phase6-app1-baseline] library not found: $lib_name" >&2; exit 1; }
    libs+=("$lib_path")
  done
fi

echo "[phase6-app1-baseline] link $TARGET.COM"
link_cmd=("$BIN/linq3" "-z" "-ptext=100H${PSECT_CFG}" "-m$OUT/$TARGET.map" "-o$OUT/$TARGET.COM")
link_cmd+=("${objs[@]}")
if [[ ${#libs[@]} -gt 0 ]]; then link_cmd+=("${libs[@]}"); fi

set +e
"${link_cmd[@]}" >/tmp/phase6_app1_base_link.log 2>&1
link_rc=$?
set -e
if [[ $link_rc -ne 0 && ! -s "$OUT/$TARGET.COM" ]]; then
  echo "[phase6-app1-baseline] link failed rc=$link_rc" >&2
  sed -n '1,120p' /tmp/phase6_app1_base_link.log >&2 || true
  exit 1
fi

cat >"$REPORT" <<EOF
{
  "tool": "build_sharksym_phase6_app1_baseline",
  "status": "pass",
  "target": "$TARGET",
  "out_dir": "$OUT",
  "cfg_dir": "$CFG_DIR",
  "src_list": "$SRC_LIST",
  "lib_list": "$LIB_LIST",
  "outputs": {
    "com": "$OUT/$TARGET.COM",
    "map": "$OUT/$TARGET.map"
  }
}
EOF
echo "[phase6-app1-baseline] report=$REPORT status=pass"
echo "[phase6-app1-baseline] done: $OUT"
