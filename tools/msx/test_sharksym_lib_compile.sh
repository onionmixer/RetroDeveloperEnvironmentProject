#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HITECH="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN"
BIN="$HITECH/bin"
SH="$HITECH/lib/CPMEMU_HI-TECH_C"
WORK="${WORK:-/tmp/sharksym_lib_compile_test}"
OUT="$WORK/out"
INC_LC="$WORK/include_sharksym_lc"
SUMMARY="$WORK/summary.tsv"

mkdir -p "$OUT" "$INC_LC"
rm -f "$OUT"/* "$SUMMARY"

for h in "$SH"/*.H; do
  bn="$(basename "$h")"
  cp "$h" "$INC_LC/$(echo "$bn" | tr 'A-Z' 'a-z')"
done

CPP_FLAGS=(-I"$INC_LC" -I"$SH" -I"$HITECH/include/hitechc" -I"$HITECH/include/msx")
DEFINES=(-DCPM -DANSI -DDOS2ONLY)
SRCS=(BL.C BLCRT.C BLGCM.C BLGFN.C BLGRC.C BLGRP.C BLSND.C)

printf "file\tcpp\tp1\tcgen\toptim\tzas\tobj\n" > "$SUMMARY"

compile_one() {
  local src="$1"
  local base="${src%.C}"
  local cpp=0 p1=0 cgen=0 optim=0 zas=0 obj=0

  set +e
  "$BIN/cpp_new3" -P "${DEFINES[@]}" "${CPP_FLAGS[@]}" "$SH/$src" "$OUT/$base.i" >/tmp/libct_"$base"_cpp.log 2>&1
  cpp=$?
  if [[ $cpp -eq 0 ]]; then
    "$BIN/p1x3" "$OUT/$base.i" "$OUT/$base.p1" >/tmp/libct_"$base"_p1.log 2>&1
    p1=$?
  fi
  if [[ $cpp -eq 0 && $p1 -eq 0 ]]; then
    "$BIN/cgen3" "$OUT/$base.p1" "$OUT/$base.as" >/tmp/libct_"$base"_cgen.log 2>&1
    cgen=$?
  fi
  if [[ $cpp -eq 0 && $p1 -eq 0 && $cgen -eq 0 ]]; then
    if [[ "$base" == "BLGRP" || "$base" == "BLCRT" ]]; then
      sed -i 's/-32768/32768/g' "$OUT/$base.as"
    fi
    if [[ "$base" == "BLSND" ]]; then
      # BLSND: optim3 .asm 경로가 불안정해서 .as 직접 어셈블을 우선 사용.
      optim=0
      (cd "$OUT" && "$BIN/zasx3" -j "$base.as" >/tmp/libct_"$base"_zas.log 2>&1)
      zas=$?
    else
      timeout 5s "$BIN/optim3" "$OUT/$base.as" "$OUT/$base.asm" >/tmp/libct_"$base"_optim.log 2>&1
      optim=$?
      if [[ $optim -eq 0 ]]; then
        (cd "$OUT" && "$BIN/zasx3" -j "$base.asm" >/tmp/libct_"$base"_zas.log 2>&1)
        zas=$?
      else
        (cd "$OUT" && "$BIN/zasx3" -j "$base.as" >/tmp/libct_"$base"_zas.log 2>&1)
        zas=$?
      fi
    fi
  fi
  set -e

  if [[ -s "$OUT/$base.obj" ]]; then
    obj=1
  fi

  printf "%s\t%d\t%d\t%d\t%d\t%d\t%d\n" "$src" "$cpp" "$p1" "$cgen" "$optim" "$zas" "$obj" >> "$SUMMARY"

  if [[ $obj -ne 1 ]]; then
    echo "[lib-compile] FAIL: missing obj for $src" >&2
    return 1
  fi
}

echo "[lib-compile] compile sharksym C sources"
for s in "${SRCS[@]}"; do
  compile_one "$s"
done

echo "[lib-compile] verify BLSND symbols"
"$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLEXTREF" --quiet -o "$WORK/ext_blsnd.json" "$OUT/BLSND.obj"
python3 - "$WORK/ext_blsnd.json" <<'PY'
import json, sys
obj = json.load(open(sys.argv[1], "r", encoding="utf-8"))["records"][0]
defs = set(obj.get("defined", []))
need = {
    "_bl_snd_bgm_get_pos",
    "_bl_snd_bgm_get_avail",
    "_bl_snd_bgm_play",
    "_bl_snd_bgm_stop",
    "_bl_snd_bgm_pause",
    "_bl_snd_bgm_resume",
}
missing = sorted(need - defs)
if missing:
    raise SystemExit(f"missing BLSND defs: {', '.join(missing)}")
print("[lib-compile] BLSND symbols OK")
PY

echo "[lib-compile] summary"
cat "$SUMMARY"
echo "[lib-compile] objects"
ls -lh "$OUT"/*.obj | sed -n '1,120p'
echo "[lib-compile] PASS (WORK=$WORK)"

