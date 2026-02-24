#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HITECH="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN"
BIN="$HITECH/bin"
SHARKSYM="$HITECH/lib/CPMEMU_HI-TECH_C"

WORK="${WORK:?WORK is required}"
TARGET="${TARGET:?TARGET is required}"
BANK_ID="${BANK_ID:?BANK_ID is required}"
CFG_DIR="${CFG_DIR:?CFG_DIR is required}"
SRC_LIST="${SRC_LIST:?SRC_LIST is required}"
LIB_LIST="${LIB_LIST:-}"
CFLAGS_DERIVED="${CFLAGS_DERIVED:--DANSI -DCPM -DDOS2ONLY}"
PSECT_CFG="${PSECT_CFG:-,data,bss,comm=9400H}"

OUT_DIR="$WORK"
TMP_BASE="${TMP_BASE:-/tmp/bltoolc_app2_bank}"
BANK_DIR="$TMP_BASE/${TARGET}_${BANK_ID}"
INC_LC="$BANK_DIR/inc"

mkdir -p "$OUT_DIR" "$TMP_BASE"
rm -rf "$BANK_DIR"
mkdir -p "$BANK_DIR" "$INC_LC"

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
  "$BIN/cpp_new3" -P "${DEFINES[@]}" "${CPP_FLAGS[@]}" "$src" "$BANK_DIR/$base.i"
  "$BIN/p1x3" "$BANK_DIR/$base.i" "$BANK_DIR/$base.p1"
  "$BIN/cgen3" "$BANK_DIR/$base.p1" "$BANK_DIR/$base.as"
  if [[ "$base" == "BLCRT" || "$base" == "BLGRP" ]]; then
    sed -i 's/-32768/32768/g' "$BANK_DIR/$base.as"
  fi
  (cd "$BANK_DIR" && "$BIN/zasx3" -j "$base.as" >/dev/null 2>&1)
}

read -r -a SRC_ARR <<<"$SRC_LIST"
for src_name in "${SRC_ARR[@]}"; do
  src_path="$(resolve_src "$src_name")" || { echo "[phase6-app2-bank] source not found: $src_name" >&2; exit 1; }
  base="$(basename "$src_name")"
  stem="${base%.*}"
  ext="${base##*.}"
  shopt -s nocasematch
  if [[ "$ext" == "c" ]]; then
    compile_obj "$src_path" "$stem"
  elif [[ "$ext" == "as" || "$ext" == "asm" ]]; then
    cp "$src_path" "$BANK_DIR/$base"
    (cd "$BANK_DIR" && "$BIN/zasx3" -j "$base" >/dev/null 2>&1)
  else
    echo "[phase6-app2-bank] unsupported source extension: $src_name" >&2
    exit 1
  fi
  shopt -u nocasematch
done

objs=()
for src_name in "${SRC_ARR[@]}"; do
  base="$(basename "$src_name")"
  stem="${base%.*}"
  objs+=("$BANK_DIR/$stem.obj")
done

libs=()
if [[ -n "${LIB_LIST// /}" ]]; then
  read -r -a LIB_ARR <<<"$LIB_LIST"
  for lib_name in "${LIB_ARR[@]}"; do
    lib_path="$(resolve_lib "$lib_name")" || { echo "[phase6-app2-bank] library not found: $lib_name" >&2; exit 1; }
    libs+=("$lib_path")
  done
fi

if [[ "$BANK_ID" == "00" ]]; then
  OUT_BIN="$OUT_DIR/${TARGET}_BANK${BANK_ID}.COM"
else
  OUT_BIN="$OUT_DIR/${TARGET}_BANK${BANK_ID}.OVL"
fi
OUT_MAP="${OUT_BIN%.*}.map"

set +e
"$BIN/linq3" -z -ptext=100H"$PSECT_CFG" -m"$OUT_MAP" -o"$OUT_BIN" "${objs[@]}" "${libs[@]}" >/tmp/phase6_app2_bank_link_${BANK_ID}.log 2>&1
link_rc=$?
set -e

if [[ $link_rc -ne 0 && ! -s "$OUT_BIN" ]]; then
  echo "[phase6-app2-bank] link failed rc=$link_rc (no output)" >&2
  sed -n '1,120p' "/tmp/phase6_app2_bank_link_${BANK_ID}.log" >&2 || true
  exit $link_rc
fi

echo "[phase6-app2-bank] bank=$BANK_ID link_rc=$link_rc out=$(basename "$OUT_BIN")"
exit 0
