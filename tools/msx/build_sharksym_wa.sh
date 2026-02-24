#!/usr/bin/env bash
set -euo pipefail

# Verification-only workaround builder for sharksym on Linux native toolchain.
# It mirrors the currently known working path in specs/PLAN_sharksym_VERIFICATION.md.

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HITECH="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN"
BIN="$HITECH/bin"
SHARKSYM="$HITECH/lib/CPMEMU_HI-TECH_C"
INCHC="$HITECH/include/hitechc"
INCMSX="$HITECH/include/msx"

WORK="${WORK:-/tmp/sharksym_wa_build}"
OUT="$WORK/out"
INC_LC="$WORK/include_sharksym_lc"

mkdir -p "$WORK" "$OUT" "$INC_LC"

for h in "$SHARKSYM"/*.H; do
  bn="$(basename "$h")"
  cp "$h" "$INC_LC/$(echo "$bn" | tr 'A-Z' 'a-z')"
done

CPP_FLAGS=(-I"$INC_LC" -I"$SHARKSYM" -I"$INCHC" -I"$INCMSX")
DEFINES_APP0=(-DCPM -DANSI -DBL_DISABLE -DBL_DOS1)

compile_c() {
  local src="$1"
  local base="$2"
  shift 2
  local extra_defs=("$@")

  "$BIN/cpp_new3" -P "${DEFINES_APP0[@]}" "${extra_defs[@]}" "${CPP_FLAGS[@]}" "$src" "$OUT/$base.i"
  "$BIN/p1x3" "$OUT/$base.i" "$OUT/$base.p1"
  "$BIN/cgen3" "$OUT/$base.p1" "$OUT/$base.as"

  if timeout 5s "$BIN/optim3" "$OUT/$base.as" "$OUT/$base.asm" >/dev/null 2>&1; then
    if (cd "$OUT" && "$BIN/zasx3" -j "$base.asm" >/dev/null 2>&1); then
      return
    fi
  fi

  # Fallback: assemble raw .as output when optim3 is unstable/hangs.
  if (cd "$OUT" && "$BIN/zasx3" -j "$base.as" >/dev/null 2>&1); then
    return
  fi

  # Known BLGRP workaround: -32768 literal can overflow in zasx3.
  if [[ "$base" == "BLGRP" ]]; then
    cp "$OUT/$base.as" "$OUT/${base}_fix.as"
    sed -i 's/defw\t-32768/defw\t32768/' "$OUT/${base}_fix.as"
    (cd "$OUT" && "$BIN/zasx3" -j "${base}_fix.as")
    mv "$OUT/${base}_fix.obj" "$OUT/$base.obj"
    return
  fi

  echo "ERROR: assemble failed for $base" >&2
  return 1
}

assemble_direct() {
  local src_as="$1"
  local base="$2"
  if [[ "$src_as" != "$OUT/$base.as" ]]; then
    cp "$src_as" "$OUT/$base.as"
  fi
  (cd "$OUT" && "$BIN/zasx3" -j "$base.as")
}

make_bl_bank_stub() {
  cat > "$OUT/bl_bank_stub.as" <<'EOF'
	global	_bl_bank
_bl_bank	equ	_bl_bank_idx
EOF
  set +e
  (cd "$OUT" && "$BIN/zasx3" -j "bl_bank_stub.as")
  local rc=$?
  set -e
  if [[ $rc -ne 0 && ! -f "$OUT/bl_bank_stub.obj" ]]; then
    echo "ERROR: failed to build bl_bank_stub.obj" >&2
    return 1
  fi
}

echo "[1/5] Compile BL core"
compile_c "$SHARKSYM/BL.C" BL
compile_c "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/examples/sharksym/0HELLO/MAIN.C" MAIN

echo "[2/5] Build BLCRT via direct asm fallback"
"$BIN/cpp_new3" -P "${DEFINES_APP0[@]}" "${CPP_FLAGS[@]}" "$SHARKSYM/BLCRT.C" "$OUT/BLCRT.i"
"$BIN/p1x3" "$OUT/BLCRT.i" "$OUT/BLCRT.p1"
"$BIN/cgen3" "$OUT/BLCRT.p1" "$OUT/BLCRT.as"
assemble_direct "$OUT/BLCRT.as" BLCRT

echo "[3/5] Compile BL graphics/sound group (workaround flags)"
compile_c "$SHARKSYM/BLGRP.C" BLGRP -DNO_BLGRP_IL
compile_c "$SHARKSYM/BLGCM.C" BLGCM -DNO_BLGRP_IL
compile_c "$SHARKSYM/BLGRC.C" BLGRC -DNO_BLGRP_IL
compile_c "$SHARKSYM/BLGFN.C" BLGFN -DNO_BLGRP_IL -DNO_BLGRPFNT
compile_c "$SHARKSYM/BLSND.C" BLSND -DNO_BLGRP_IL

echo "[4/5] Link variants"
"$BIN/linq3" -z -ptext=100H,data,bss -o"$OUT/TEST_MINP4.COM" \
  "$OUT/BLCRT.obj" "$OUT/BL.obj" "$OUT/MAIN.obj" "$SHARKSYM/LIBCMSX.LIB" || true

"$BIN/linq3" -z -ptext=100H,data,bss -o"$OUT/TEST_FULL_WA_NOSND.COM" \
  "$OUT/BLCRT.obj" "$OUT/BL.obj" "$OUT/BLGRP.obj" "$OUT/BLGFN.obj" "$OUT/BLGCM.obj" "$OUT/BLGRC.obj" \
  "$OUT/MAIN.obj" "$SHARKSYM/LIBCMSX.LIB" || true

make_bl_bank_stub
"$BIN/linq3" -z -ptext=100H,data,bss -o"$OUT/TEST_FULL_WA_STUB.COM" \
  "$OUT/BLCRT.obj" "$OUT/BL.obj" "$OUT/BLGRP.obj" "$OUT/BLGFN.obj" "$OUT/BLGCM.obj" "$OUT/BLGRC.obj" "$OUT/BLSND.obj" \
  "$OUT/bl_bank_stub.obj" "$OUT/MAIN.obj" "$SHARKSYM/LIBCMSX.LIB" || true

echo "[5/5] Done"
ls -lh "$OUT"/*.COM 2>/dev/null || true
echo "Output directory: $OUT"
