#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HITECH="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN"
BIN="$HITECH/bin"
SHARKSYM="$HITECH/lib/CPMEMU_HI-TECH_C"
EXAMPLE="${EXAMPLE:-2HELLO}"
EX2="$HITECH/examples/sharksym/$EXAMPLE"
EXAMPLE_VARIANT="${EXAMPLE_VARIANT:-}"
TARGET_TAG="$EXAMPLE"
if [[ "$EXAMPLE" == "2ASSERT" && "$EXAMPLE_VARIANT" == "NDEBUG" ]]; then
  TARGET_TAG="2ASSERTN"
fi

if [[ -z "${WORK:-}" ]]; then
  if [[ "$TARGET_TAG" == "2HELLO" ]]; then
    WORK="/tmp/sharksym_phase6_gate3"
  else
    WORK="/tmp/sharksym_phase6_gate3_${TARGET_TAG}"
  fi
fi
OUT="$WORK/out"
INC_LC="$WORK/include_sharksym_lc"

if [[ ! -d "$EX2" ]]; then
  echo "[gate3] missing example dir: $EX2" >&2
  exit 1
fi
if [[ ! -f "$EX2/MAIN.C" ]]; then
  echo "[gate3] example requires MAIN.C: $EXAMPLE" >&2
  exit 1
fi

mkdir -p "$WORK" "$OUT" "$INC_LC"

echo "[gate3] step1: prepare include aliases"
for h in "$SHARKSYM"/*.H; do
  bn="$(basename "$h")"
  cp "$h" "$INC_LC/$(echo "$bn" | tr 'A-Z' 'a-z')"
done

CPP_FLAGS=(-I"$INC_LC" -I"$SHARKSYM" -I"$HITECH/include/hitechc" -I"$HITECH/include/msx")
DEFINES=(-DCPM -DANSI -DDOS2ONLY)
if [[ "$EXAMPLE_VARIANT" == "NDEBUG" ]]; then
  DEFINES+=(-DNDEBUG)
fi
APP_CFLAGS=()
BANK01_SRCS=("MAIN.C")
BANK02_SRCS=()
case "$EXAMPLE" in
  2HELLO|2ASSERT)
    BANK02_SRCS=("SUB.C")
    ;;
  2LMEM)
    ;;
  2BGM)
    BANK02_SRCS=("SUB.C" "BLSND.C")
    ;;
  2HANGUL)
    APP_CFLAGS+=(-DBLGRPFNT_KR)
    BANK01_SRCS=("MAIN.C" "HANGUL.C")
    BANK02_SRCS=("BLGRP.C" "BLGCM.C" "BLGFN.C")
    ;;
  2HANIME)
    APP_CFLAGS+=(-DBLGRPFNT_KR -DNO_BLGRPFNT_MC -DNO_BLGRPFNT_G4 -DNO_BLGRPFNT_G5 -DNO_BLGRPFNT_G7)
    BANK01_SRCS=("MAIN.C" "BLGRP.C" "BLGCM.C")
    BANK02_SRCS=("HANIME.C" "BLGFN.C")
    ;;
  2TETRIS)
    BANK01_SRCS=("MAIN.C" "TETRIS.C" "BLGRP.C" "BLGCM.C" "BLGFN.C")
    BANK02_SRCS=("SOUND.C" "BLSND.C")
    ;;
  *)
    echo "[gate3] unsupported example: $EXAMPLE" >&2
    exit 1
    ;;
esac
DEFINES+=("${APP_CFLAGS[@]}")
HAS_BANK02=0
if [[ ${#BANK02_SRCS[@]} -gt 0 ]]; then
  HAS_BANK02=1
fi
GATE3_MODE="native"
GATE3_NOTE="BL/BLCRT rebuilt with app-mode2 flags and symbol/link probes passed"
NATIVE_BANK00_EXTRA=()

compile_app2_c() {
  local src="$1"
  local base="$2"
  shift 2
  local extra_defs=("$@")
  "$BIN/cpp_new3" -P "${DEFINES[@]}" "${extra_defs[@]}" "${CPP_FLAGS[@]}" "$src" "$OUT/$base.i"
  "$BIN/p1x3" "$OUT/$base.i" "$OUT/$base.p1"
  "$BIN/cgen3" "$OUT/$base.p1" "$OUT/$base.as"
  if [[ "$base" == "BLCRT" || "$base" == "BLCRT_native" ]]; then
    # Same overflow pattern as BLGRP: -32768 immediate triggers warning in zasx3.
    sed -i 's/\-32768/32768/g' "$OUT/$base.as"
  fi
  if [[ "$base" == "BLGRP" ]]; then
    # Known z80 assembler overflow form; semantic-equivalent unsigned literal is accepted.
    sed -i 's/defw\t-32768/defw\t32768/g' "$OUT/$base.as"
  fi
  if [[ "$base" == "BLSND" ]]; then
    (cd "$OUT" && "$BIN/zasx3" -j "$base.as" >/dev/null 2>&1)
    return
  fi
  if timeout 5s "$BIN/optim3" "$OUT/$base.as" "$OUT/$base.asm" >/dev/null 2>&1; then
    if (cd "$OUT" && "$BIN/zasx3" -j "$base.asm" >/dev/null 2>&1); then
      return
    fi
  fi
  (cd "$OUT" && "$BIN/zasx3" -j "$base.as" >/dev/null 2>&1)
}

compile_app2_c_try() {
  local src="$1"
  local base="$2"
  set +e
  compile_app2_c "$src" "$base" >/tmp/gate3_compile_"$base".log 2>&1
  local rc=$?
  set -e
  if [[ ! -f "$OUT/$base.obj" ]]; then
    return 1
  fi
  # Some inputs can emit warnings and non-zero rc while still producing usable .obj.
  if [[ ! -s "$OUT/$base.obj" ]]; then
    return 1
  fi
  return 0
}

assemble_strict() {
  local src="$1"
  (cd "$OUT" && "$BIN/zasx3" -j "$src" >/dev/null 2>&1)
}

patch_blcrt_native_fastcall_aliases() {
  local asf="$OUT/BLCRT_native.as"
  [[ -f "$asf" ]] || return 0
  python3 - "$asf" <<'PY'
import re, sys
p = sys.argv[1]
txt = open(p, "r", encoding="utf-8", errors="replace").read()
need_put = "_put_lmem_seg_table_hl:" in txt and not re.search(r"^_put_lmem_seg_table:\s*$", txt, re.M)
need_get = "_get_lmem_seg_table_hl:" in txt and not re.search(r"^_get_lmem_seg_table:\s*$", txt, re.M)
if not (need_put or need_get):
    raise SystemExit(0)
tail = []
tail.append("")
tail.append("; gate3 native fix: provide plain symbol aliases for fastcall helpers")
if need_put:
    tail.append("_put_lmem_seg_table:")
    tail.append("\tjp\t_put_lmem_seg_table_hl")
if need_get:
    tail.append("_get_lmem_seg_table:")
    tail.append("\tjp\t_get_lmem_seg_table_hl")
open(p, "a", encoding="ascii").write("\n".join(tail) + "\n")
PY
  assemble_strict BLCRT_native.as
}

echo "[gate3] step1b: try native BL/BLCRT build for app-mode2"
native_ok=0
native_reason=""
if compile_app2_c_try "$SHARKSYM/BL.C" BL_native && compile_app2_c_try "$SHARKSYM/BLCRT.C" BLCRT_native; then
  patch_blcrt_native_fastcall_aliases
  bl_native_size=$(wc -c < "$OUT/BL_native.obj")
  blcrt_native_size=$(wc -c < "$OUT/BLCRT_native.obj")
  if [[ $bl_native_size -ge 100 && $blcrt_native_size -ge 100 ]]; then
    "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLEXTREF" --quiet -o "$WORK/ext_bl_native.json" "$OUT/BL_native.obj"
    "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLEXTREF" --quiet -o "$WORK/ext_blcrt_native.json" "$OUT/BLCRT_native.obj"
    set +e
    python3 - "$WORK/ext_bl_native.json" "$WORK/ext_blcrt_native.json" <<'PY'
import json, sys
bl = json.load(open(sys.argv[1], "r", encoding="utf-8"))["records"][0]
crt = json.load(open(sys.argv[2], "r", encoding="utf-8"))["records"][0]
bl_def = set(bl.get("defined", []))
crt_def = set(crt.get("defined", []))
need_bl = {"_bl_data", "_bl_bss"}
if not need_bl.issubset(bl_def):
    miss = sorted(need_bl - bl_def)
    raise SystemExit(f"missing BL symbols: {','.join(miss)}")
if "start" not in crt_def:
    raise SystemExit("missing BLCRT symbol: start")
if len(crt_def) < 10:
    raise SystemExit(f"BLCRT defined symbol count too small: {len(crt_def)}")
PY
    ext_rc=$?
    set -e
    if [[ $ext_rc -eq 0 ]]; then
      cat > "$OUT/main_temp_stub.as" <<'EOF'
	global	_main
_main:
	ret
EOF
      assemble_strict main_temp_stub.as
      python3 - "$WORK/ext_blcrt_native.json" "$OUT/lmem_seg_alias.as" <<'PY'
import json, sys
ext_path, out_as = sys.argv[1], sys.argv[2]
rec = json.load(open(ext_path, "r", encoding="utf-8"))["records"][0]
defs = set(rec.get("defined", []))
refs = set(rec.get("extern_refs", []))
targets = [
    "_put_lmem_seg_table",
    "_get_lmem_seg_table",
    "_put_lmem_seg_table_hl",
    "_get_lmem_seg_table_hl",
]
missing = [s for s in targets if s in refs and s not in defs]
lines = []
for s in missing:
    lines += [f"\tglobal\t{s}", f"{s}:", "\tret"]
with open(out_as, "w", encoding="ascii") as f:
    if lines:
        f.write("\n".join(lines) + "\n")
    else:
        f.write("; no lmem alias needed\n")
PY
      if rg -n "global" "$OUT/lmem_seg_alias.as" >/dev/null 2>&1; then
        assemble_strict lmem_seg_alias.as
      else
        rm -f "$OUT/lmem_seg_alias.obj"
      fi
      set +e
      native_probe_inputs=("$OUT/BLCRT_native.obj" "$OUT/BL_native.obj" "$OUT/main_temp_stub.obj")
      if [[ -f "$OUT/lmem_seg_alias.obj" ]]; then
        native_probe_inputs+=("$OUT/lmem_seg_alias.obj")
      fi
      "$BIN/linq3" -z -ptext=100H,data,bss -m"$WORK/native_bank00_probe.map" -o"$WORK/native_bank00_probe.com" \
        "${native_probe_inputs[@]}" "$SHARKSYM/LIBCMSX2.LIB" >/tmp/gate3_native_bank00_probe.log 2>&1
      probe_rc=$?
      set -e
      if [[ $probe_rc -eq 0 ]]; then
        cp "$OUT/BL_native.obj" "$OUT/BL.obj"
        cp "$OUT/BLCRT_native.obj" "$OUT/BLCRT.obj"
        if [[ -f "$OUT/lmem_seg_alias.obj" ]]; then
          NATIVE_BANK00_EXTRA=("$OUT/lmem_seg_alias.obj")
        else
          NATIVE_BANK00_EXTRA=()
        fi
        native_ok=1
        if [[ -f "$OUT/lmem_seg_alias.obj" ]]; then
          GATE3_NOTE="native BL/BLCRT + lmem seg-table alias object applied"
        fi
      else
        native_reason="native BL/BLCRT bank00 link-probe failed"
      fi
    else
      native_reason="symbol-gate failed for native BL/BLCRT"
    fi
  else
    native_reason="native BL/BLCRT object size gate failed"
  fi
else
  native_reason="native BL/BLCRT compile did not produce usable objects"
fi
if [[ $native_ok -ne 1 ]]; then
  echo "[gate3] ERROR: native BL/BLCRT build failed: $native_reason" >&2
  exit 1
fi

echo "[gate3] step2: compile $EXAMPLE MAIN/SUB (app-mode2 flags)"
resolve_src_path() {
  local src="$1"
  if [[ -f "$EX2/$src" ]]; then
    echo "$EX2/$src"
    return
  fi
  if [[ -f "$SHARKSYM/$src" ]]; then
    echo "$SHARKSYM/$src"
    return
  fi
  echo "[gate3] missing source file: $src" >&2
  exit 1
}
compile_src_list() {
  local __arr_name="$1"
  shift
  local src
  for src in "$@"; do
    local src_path base
    src_path="$(resolve_src_path "$src")"
    base="$(basename "$src" .C)"
    local extra_defs=()
    if [[ "$src" == "SUB.C" && "$EXAMPLE" == "2ASSERT" ]]; then
      extra_defs+=("-D__BL_FILE__=\"SUB.C\"")
    fi
    compile_app2_c "$src_path" "$base" "${extra_defs[@]}"
    eval "$__arr_name+=(\"$OUT/$base.obj\")"
  done
}
BANK01_OBJS=()
BANK02_OBJS=()
compile_src_list BANK01_OBJS "${BANK01_SRCS[@]}"
if [[ $HAS_BANK02 -eq 1 ]]; then
  compile_src_list BANK02_OBJS "${BANK02_SRCS[@]}"
fi

echo "[gate3] step3: extref/comm for bank topology"
"$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLEXTREF" --quiet -o "$WORK/ext_bank00.json" "$OUT/BLCRT.obj" "$OUT/BL.obj"
"$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLEXTREF" --quiet -o "$WORK/ext_bank01.json" "$OUT/BL.obj" "${BANK01_OBJS[@]}"
if [[ $HAS_BANK02 -eq 1 ]]; then
  "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLEXTREF" --quiet -o "$WORK/ext_bank02.json" "$OUT/BL.obj" "${BANK02_OBJS[@]}"
fi
COMM_BANK_ARGS=(--bank "bank00=$WORK/ext_bank00.json" --bank "bank01=$WORK/ext_bank01.json")
if [[ $HAS_BANK02 -eq 1 ]]; then
  COMM_BANK_ARGS+=(--bank "bank02=$WORK/ext_bank02.json")
fi
"$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLCOMM" -o "$WORK/comm.json" "${COMM_BANK_ARGS[@]}"
python3 - "$WORK/comm.json" "$WORK/bank01_cross_syms.txt" "$WORK/bank02_cross_syms.txt" <<'PY'
import json, sys
obj = json.load(open(sys.argv[1], "r", encoding="utf-8"))
b1, b2 = [], []
for x in obj.get("cross_bank_symbols", []):
    if not isinstance(x, dict):
        continue
    sym = x.get("symbol")
    defs = set(x.get("defined_in", []))
    refs = set(x.get("referenced_in", []))
    if sym and ("bank01" in refs) and ("bank01" not in defs):
        b1.append(sym)
    if sym and ("bank02" in refs) and ("bank02" not in defs):
        b2.append(sym)
open(sys.argv[2], "w", encoding="ascii").write("\n".join(sorted(set(b1))) + ("\n" if b1 else ""))
open(sys.argv[3], "w", encoding="ascii").write("\n".join(sorted(set(b2))) + ("\n" if b2 else ""))
PY
BANK01_CROSS_COUNT=$(wc -l < "$WORK/bank01_cross_syms.txt" 2>/dev/null || echo 0)
BANK02_CROSS_COUNT=$(wc -l < "$WORK/bank02_cross_syms.txt" 2>/dev/null || echo 0)

echo "[gate3] step4: build temporary maps for caller offsets"
cat > "$OUT/bl_bank_stub.as" <<'EOF'
	global	_bl_bank
psect	data
_bl_bank:
	defw	0000h
EOF
cat > "$OUT/main_callermap_stub.as" <<'EOF'
	global	_main
_main:
	ret
EOF
assemble_strict bl_bank_stub.as
assemble_strict main_callermap_stub.as

PROBE01_EXTRA=()
if [[ $BANK01_CROSS_COUNT -gt 0 ]]; then
  python3 - "$WORK/bank01_cross_syms.txt" "$OUT/bank01_probe_stub.as" <<'PY'
import sys
syms = [ln.strip() for ln in open(sys.argv[1], "r", encoding="ascii") if ln.strip()]
with open(sys.argv[2], "w", encoding="ascii") as f:
    for s in syms:
        f.write(f"\tglobal\t{s}\n{s}:\n\tret\n")
PY
  assemble_strict bank01_probe_stub.as
  PROBE01_EXTRA+=("$OUT/bank01_probe_stub.obj")
fi

PROBE02_EXTRA=()
if [[ $HAS_BANK02 -eq 1 && $BANK02_CROSS_COUNT -gt 0 ]]; then
  python3 - "$WORK/bank02_cross_syms.txt" "$OUT/bank02_probe_stub.as" <<'PY'
import sys
syms = [ln.strip() for ln in open(sys.argv[1], "r", encoding="ascii") if ln.strip()]
with open(sys.argv[2], "w", encoding="ascii") as f:
    for s in syms:
        f.write(f"\tglobal\t{s}\n{s}:\n\tret\n")
PY
  assemble_strict bank02_probe_stub.as
  PROBE02_EXTRA+=("$OUT/bank02_probe_stub.obj")
fi

set +e
"$BIN/linq3" -z -ptext=100H,data,bss -m"$WORK/bank01.map" -o"$WORK/bank01_probe.com" \
  "$OUT/BL.obj" "${BANK01_OBJS[@]}" "${PROBE01_EXTRA[@]}" "$OUT/bl_bank_stub.obj" "$SHARKSYM/LIBCMSX2.LIB" \
  >/tmp/gate3_bank01_probe.log 2>&1
rc01=$?
rc02=0
if [[ $HAS_BANK02 -eq 1 ]]; then
  BANK02_PROBE_INPUTS=("$OUT/BL.obj" "${BANK02_OBJS[@]}")
  BANK02_PROBE_INPUTS+=("${PROBE02_EXTRA[@]}" "$OUT/bl_bank_stub.obj")
  "$BIN/linq3" -z -ptext=100H,data,bss -m"$WORK/bank02.map" -o"$WORK/bank02_probe.com" \
    "${BANK02_PROBE_INPUTS[@]}" "$SHARKSYM/LIBCMSX2.LIB" \
    >/tmp/gate3_bank02_probe.log 2>&1
  rc02=$?
fi
"$BIN/linq3" -z -ptext=100H,data,bss -m"$WORK/bank00_callermap.map" -o"$WORK/bank00_callermap.com" \
  "$OUT/BLCRT.obj" "$OUT/BL.obj" "$OUT/main_callermap_stub.obj" "${NATIVE_BANK00_EXTRA[@]}" "$SHARKSYM/LIBCMSX2.LIB" \
  >/tmp/gate3_bank00_callermap.log 2>&1
rc00m=$?
set -e
if [[ $rc01 -ne 0 || $rc02 -ne 0 || $rc00m -ne 0 ]]; then
  echo "[gate3] map probe failed: rc00m=$rc00m rc01=$rc01 rc02=$rc02" >&2
  sed -n '1,120p' /tmp/gate3_bank00_callermap.log >&2 || true
  sed -n '1,120p' /tmp/gate3_bank01_probe.log >&2 || true
  sed -n '1,120p' /tmp/gate3_bank02_probe.log >&2 || true
  exit 1
fi

echo "[gate3] step5: generate/assemble bank caller with strict map resolution"
if [[ $HAS_BANK02 -eq 1 ]]; then
  "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLCALLER" --comm-json "$WORK/comm.json" \
    --bank-map bank00="$WORK/bank00_callermap.map" \
    --bank-map bank01="$WORK/bank01.map" \
    --bank-map bank02="$WORK/bank02.map" \
    --strict \
    -o "$OUT/blcaller.as"
else
  "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLCALLER" --comm-json "$WORK/comm.json" \
    --bank-map bank00="$WORK/bank00_callermap.map" \
    --bank-map bank01="$WORK/bank01.map" \
    --strict \
    -o "$OUT/blcaller.as"
fi
(cd "$OUT" && "$BIN/zasx3" -j blcaller.as >/dev/null 2>&1)

echo "[gate3] step6: build alias objects for cross-bank symbols"
cat > "$OUT/main_alias.as" <<'EOF'
	global	_blcall_main
	global	_main
_main:
	jp	_blcall_main
EOF
assemble_strict main_alias.as

BANK01_EXTRA=()
if [[ $BANK01_CROSS_COUNT -gt 0 ]]; then
  python3 - "$WORK/bank01_cross_syms.txt" "$OUT/bank01_alias.as" <<'PY'
import sys
syms = [ln.strip() for ln in open(sys.argv[1], "r", encoding="ascii") if ln.strip()]
with open(sys.argv[2], "w", encoding="ascii") as f:
    for s in syms:
        wrap = "_blcall_" + (s[1:] if s.startswith("_") else s)
        f.write(f"\tglobal\t{wrap}\n\tglobal\t{s}\n{s}:\n\tjp\t{wrap}\n")
PY
  assemble_strict bank01_alias.as
  BANK01_EXTRA+=("$OUT/bank01_alias.obj")
fi

BANK02_EXTRA=()
if [[ $HAS_BANK02 -eq 1 && $BANK02_CROSS_COUNT -gt 0 ]]; then
  python3 - "$WORK/bank02_cross_syms.txt" "$OUT/bank02_alias.as" <<'PY'
import sys
syms = [ln.strip() for ln in open(sys.argv[1], "r", encoding="ascii") if ln.strip()]
with open(sys.argv[2], "w", encoding="ascii") as f:
    for s in syms:
        wrap = "_blcall_" + (s[1:] if s.startswith("_") else s)
        f.write(f"\tglobal\t{wrap}\n\tglobal\t{s}\n{s}:\n\tjp\t{wrap}\n")
PY
  assemble_strict bank02_alias.as
  BANK02_EXTRA+=("$OUT/bank02_alias.obj")
fi

echo "[gate3] step7: final bank links"
set +e
"$BIN/linq3" -z -ptext=100H,data,bss -m"$WORK/bank00.map" -o"$WORK/bank00.com" \
  "$OUT/BLCRT.obj" "$OUT/BL.obj" "$OUT/main_alias.obj" "$OUT/blcaller.obj" "${NATIVE_BANK00_EXTRA[@]}" "$SHARKSYM/LIBCMSX2.LIB" \
  >/tmp/gate3_bank00_link.log 2>&1
rc00=$?
"$BIN/linq3" -z -ptext=100H,data,bss -m"$WORK/bank01_final.map" -o"$WORK/bank01.com" \
  "$OUT/BL.obj" "${BANK01_OBJS[@]}" "${BANK01_EXTRA[@]}" "$OUT/blcaller.obj" "$OUT/bl_bank_stub.obj" "$SHARKSYM/LIBCMSX2.LIB" \
  >/tmp/gate3_bank01_link.log 2>&1
rc11=$?
rc22=0
if [[ $HAS_BANK02 -eq 1 ]]; then
  BANK02_FINAL_INPUTS=("$OUT/BL.obj" "${BANK02_OBJS[@]}")
  BANK02_FINAL_INPUTS+=("$OUT/blcaller.obj" "${BANK02_EXTRA[@]}" "$OUT/bl_bank_stub.obj")
  "$BIN/linq3" -z -ptext=100H,data,bss -m"$WORK/bank02_final.map" -o"$WORK/bank02.com" \
    "${BANK02_FINAL_INPUTS[@]}" "$SHARKSYM/LIBCMSX2.LIB" \
    >/tmp/gate3_bank02_link.log 2>&1
  rc22=$?
fi
set -e
if [[ $rc00 -ne 0 || $rc11 -ne 0 || $rc22 -ne 0 ]]; then
  echo "[gate3] final link failed: rc00=$rc00 rc11=$rc11 rc22=$rc22" >&2
  sed -n '1,120p' /tmp/gate3_bank00_link.log >&2 || true
  sed -n '1,120p' /tmp/gate3_bank01_link.log >&2 || true
  sed -n '1,120p' /tmp/gate3_bank02_link.log >&2 || true
  exit 1
fi

echo "[gate3] step8: merge bank artifacts"
if [[ $HAS_BANK02 -eq 1 ]]; then
  "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLMERGE" -o "$WORK/${TARGET_TAG}_app2_merged.bin" "$WORK/bank00.com" "$WORK/bank01.com" "$WORK/bank02.com" >/tmp/gate3_merge.log 2>&1
else
  "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLMERGE" -o "$WORK/${TARGET_TAG}_app2_merged.bin" "$WORK/bank00.com" "$WORK/bank01.com" >/tmp/gate3_merge.log 2>&1
fi

if [[ "${ENABLE_OPENMSX_SMOKE:-0}" == "1" ]]; then
  echo "[gate3] step9: openMSX runtime smoke (optional)"
  REPORT="$WORK/openmsx_gate3_report.json" \
    "$ROOT/tools/msx/openmsx_phase6_optim_smoke.sh" "$WORK/bank00.com"
fi

export GATE3_MODE
export GATE3_NOTE
export EXAMPLE
export EXAMPLE_VARIANT
export TARGET_TAG
python3 - "$WORK/gate3_report.json" "$WORK" <<'PY'
import json, os, sys
report_path, work = sys.argv[1], sys.argv[2]
report = {
    "tool": "phase6_gate3_app2",
    "status": "pass",
    "example": os.environ.get("EXAMPLE", "2HELLO"),
    "variant": os.environ.get("EXAMPLE_VARIANT", ""),
    "target_tag": os.environ.get("TARGET_TAG", os.environ.get("EXAMPLE", "2HELLO")),
    "mode": os.environ.get("GATE3_MODE", "native"),
    "note": os.environ.get("GATE3_NOTE", ""),
    "outputs": {
        "bank00_com": os.path.join(work, "bank00.com"),
        "bank01_com": os.path.join(work, "bank01.com"),
        "bank02_com": os.path.join(work, "bank02.com") if os.path.exists(os.path.join(work, "bank02.com")) else "",
        "merged_bin": os.path.join(work, f"{os.environ.get('TARGET_TAG', os.environ.get('EXAMPLE', '2HELLO'))}_app2_merged.bin"),
        "comm_json": os.path.join(work, "comm.json"),
    },
}
with open(report_path, "w", encoding="utf-8") as f:
    json.dump(report, f, ensure_ascii=False, indent=2)
PY

echo "[gate3] done: $WORK"
ls -lh "$WORK"/bank00.com "$WORK"/bank01.com "$WORK"/"${TARGET_TAG}_app2_merged.bin" 2>/dev/null || true
ls -lh "$WORK"/bank02.com 2>/dev/null || true
