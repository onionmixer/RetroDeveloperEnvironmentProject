#!/usr/bin/env bash
set -euo pipefail

# Verification-only app-mode1 workaround:
# BLCRT.C path is currently blocked, so use CRT2.O + BL.obj + MAIN.obj.

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HITECH="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN"
BIN="$HITECH/bin"
SHARKSYM="$HITECH/lib/CPMEMU_HI-TECH_C"
INCHC="$HITECH/include/hitechc"
INCMSX="$HITECH/include/msx"
RDEDISKTOOL="$ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool"

WORK="${WORK:-/tmp/sharksym_phase5_wa}"
OUT="$WORK/out"
INC_LC="$WORK/include_sharksym_lc"
mkdir -p "$OUT" "$INC_LC"

for h in "$SHARKSYM"/*.H; do
  bn="$(basename "$h")"
  cp "$h" "$INC_LC/$(echo "$bn" | tr 'A-Z' 'a-z')"
done

CPP_FLAGS=(-I"$INC_LC" -I"$SHARKSYM" -I"$INCHC" -I"$INCMSX")
DEFINES_APP1=(-DCPM -DANSI -DDOS2ONLY -DBL_1BANK)

compile_c() {
  local src="$1"
  local base="$2"
  "$BIN/cpp_new3" -P "${DEFINES_APP1[@]}" "${CPP_FLAGS[@]}" "$src" "$OUT/$base.i"
  "$BIN/p1x3" "$OUT/$base.i" "$OUT/$base.p1"
  "$BIN/cgen3" "$OUT/$base.p1" "$OUT/$base.as"

  if timeout 5s "$BIN/optim3" "$OUT/$base.as" "$OUT/$base.asm" >/dev/null 2>&1; then
    if (cd "$OUT" && "$BIN/zasx3" -j "$base.asm" >/dev/null 2>&1); then
      return
    fi
  fi
  (cd "$OUT" && "$BIN/zasx3" -j "$base.as")
}

echo "[1/3] Build BL.obj + MAIN.obj"
compile_c "$SHARKSYM/BL.C" BL
compile_c "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/examples/sharksym/1HELLO/MAIN.C" MAIN

echo "[2/3] Link with CRT2.O"
"$BIN/linq3" -z -ptext=100H,data,bss,comm=9400H -o"$OUT/1HELLO_CRT2_BL.COM" \
  "$SHARKSYM/CRT2.O" "$OUT/BL.obj" "$OUT/MAIN.obj" "$SHARKSYM/LIBCMSX2.LIB"

echo "[3/3] Optional disk image injection"
if [[ -x "$RDEDISKTOOL" ]]; then
  "$RDEDISKTOOL" create "$OUT/verify_p5_crt2_bl.dsk" -f msxdsk --fs msxdos --force >/dev/null
  "$RDEDISKTOOL" add "$OUT/verify_p5_crt2_bl.dsk" "$OUT/1HELLO_CRT2_BL.COM" >/dev/null
fi

ls -lh "$OUT/1HELLO_CRT2_BL.COM" "$OUT/verify_p5_crt2_bl.dsk" 2>/dev/null || true
echo "Output directory: $OUT"
