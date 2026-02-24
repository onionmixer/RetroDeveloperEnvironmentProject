#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HITECH="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN"
BIN="$HITECH/bin"
SHARKSYM="$HITECH/lib/CPMEMU_HI-TECH_C"

WORK="${WORK:-/tmp/sharksym_phase6_rom_baseline}"
OUT="$WORK"
INC_LC="$WORK/include_sharksym_lc"
REPORT="${REPORT:-$WORK/rom_baseline_report.json}"
TARGET="${TARGET:-0HELLO}"
ROM_KB="${ROM_KB:-32}"
CFG_DIR="${CFG_DIR:-$HITECH/examples/sharksym/0HELLO.ROM}"
SRC_LIST="${SRC_LIST:-BLCRT.C BL.C MAIN.C}"
LIB_LIST="${LIB_LIST:-LIBCMSXR.LIB}"
CFLAGS_DERIVED="${CFLAGS_DERIVED:--DANSI -DCPM -DBL_DISABLE -DBL_DOS1 -DBL_ROM=${ROM_KB}}"
PSECT_CFG="${PSECT_CFG:-,data=8800H,bss}"

mkdir -p "$OUT" "$INC_LC"

for h in "$SHARKSYM"/*.H; do
  bn="$(basename "$h")"
  cp "$h" "$INC_LC/$(echo "$bn" | tr 'A-Z' 'a-z')"
done

CPP_FLAGS=(-I"$CFG_DIR" -I"$INC_LC" -I"$SHARKSYM" -I"$HITECH/include/hitechc" -I"$HITECH/include/msx")
read -r -a DEFINES <<<"$CFLAGS_DERIVED"

resolve_src() {
  local s="$1"
  if [[ -f "$s" ]]; then
    echo "$s"
    return 0
  fi
  if [[ -f "$CFG_DIR/$s" ]]; then
    echo "$CFG_DIR/$s"
    return 0
  fi
  if [[ -f "$SHARKSYM/$s" ]]; then
    echo "$SHARKSYM/$s"
    return 0
  fi
  return 1
}

resolve_lib() {
  local s="$1"
  if [[ -f "$s" ]]; then
    echo "$s"
    return 0
  fi
  if [[ -f "$CFG_DIR/$s" ]]; then
    echo "$CFG_DIR/$s"
    return 0
  fi
  if [[ -f "$SHARKSYM/$s" ]]; then
    echo "$SHARKSYM/$s"
    return 0
  fi
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

echo "[phase6-rom-baseline] compile objects"
read -r -a SRC_ARR <<<"$SRC_LIST"
for src_name in "${SRC_ARR[@]}"; do
  src_path="$(resolve_src "$src_name")" || { echo "[phase6-rom-baseline] source not found: $src_name" >&2; exit 1; }
  base="$(basename "$src_name")"
  stem="${base%.*}"
  ext="${base##*.}"
  shopt -s nocasematch
  if [[ "$ext" == "c" ]]; then
    compile_obj "$src_path" "$stem"
  elif [[ "$ext" == "as" || "$ext" == "asm" ]]; then
    # Use normalized lowercase extension to keep predictable object name (<stem>.obj).
    cp "$src_path" "$OUT/$stem.as"
    (cd "$OUT" && "$BIN/zasx3" -j "$stem.as" >/dev/null 2>&1)
  else
    echo "[phase6-rom-baseline] unsupported source extension: $src_name" >&2
    exit 1
  fi
  shopt -u nocasematch
done

echo "[phase6-rom-baseline] link $TARGET.out"
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
    lib_path="$(resolve_lib "$lib_name")" || { echo "[phase6-rom-baseline] library not found: $lib_name" >&2; exit 1; }
    libs+=("$lib_path")
  done
fi

link_cmd=("$BIN/linq3" "-Ptext=4000H${PSECT_CFG}" "-m$OUT/$TARGET.map" "-o$OUT/$TARGET.out")
link_cmd+=("${objs[@]}")
if [[ ${#libs[@]} -gt 0 ]]; then
  link_cmd+=("${libs[@]}")
fi

set +e
"${link_cmd[@]}" >/tmp/phase6_rom_base_link.log 2>&1
link_rc=$?
set -e
if [[ $link_rc -ne 0 && ! -s "$OUT/$TARGET.out" ]]; then
  echo "[phase6-rom-baseline] link failed rc=$link_rc" >&2
  sed -n '1,120p' /tmp/phase6_rom_base_link.log >&2 || true
  exit 1
fi

echo "[phase6-rom-baseline] objtohex $TARGET.hex"
set +e
"$BIN/objtohex" "$OUT/$TARGET.out" "$OUT/$TARGET.hex" >/tmp/phase6_rom_base_objtohex.log 2>&1
hex_rc=$?
set -e
if [[ $hex_rc -ne 0 && ! -s "$OUT/$TARGET.hex" ]]; then
  echo "[phase6-rom-baseline] objtohex failed rc=$hex_rc" >&2
  sed -n '1,120p' /tmp/phase6_rom_base_objtohex.log >&2 || true
  exit 1
fi

cat >"$REPORT" <<EOF
{
  "tool": "build_sharksym_phase6_rom_baseline",
  "status": "pass",
  "target": "$TARGET",
  "rom_kb": $ROM_KB,
  "out_dir": "$OUT",
  "cfg_dir": "$CFG_DIR",
  "src_list": "$SRC_LIST",
  "lib_list": "$LIB_LIST",
  "outputs": {
    "out": "$OUT/$TARGET.out",
    "map": "$OUT/$TARGET.map",
    "hex": "$OUT/$TARGET.hex"
  }
}
EOF
echo "[phase6-rom-baseline] report=$REPORT status=pass"
echo "[phase6-rom-baseline] done: $OUT"
