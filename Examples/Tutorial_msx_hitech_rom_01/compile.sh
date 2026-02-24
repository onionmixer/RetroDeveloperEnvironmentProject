#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

HITECH="$ROOT_DIR/Toolchain/MSX/HITECH_TOOLCHAIN"
BIN="$HITECH/bin"
LIBDIR="$HITECH/lib/CPMEMU_HI-TECH_C"
PACKER="$ROOT_DIR/Toolchain/MSX/HITECH_TOOLCHAIN/source/PACKING/msx_hitech_nonmapper_pack.py"

TARGET_BASE="HELLO48"
OUT_OBJ="$BUILD_DIR/${TARGET_BASE}.out"
OUT_MAP="$BUILD_DIR/${TARGET_BASE}.map"
OUT_HEX="$BUILD_DIR/${TARGET_BASE}.hex"
OUT_ROM="$BUILD_DIR/${TARGET_BASE}.rom"
OUT_CART_ROM="$BUILD_DIR/${TARGET_BASE}_NONMAPPER.rom"
ROM_ENTRY_MODE="${ROM_ENTRY_MODE:-main-pure}"
DEBUG_BORDER="${DEBUG_BORDER:-0}"

# Operational intent:
# - main-pure (default): proof path for "C program -> ROM -> visible Hello World"
# - loop            : fallback regression path
# - DEBUG_BORDER=1  : visual diagnostics path (color transition), not the proof baseline

print_header() {
  echo "========================================"
  echo "$1"
  echo "========================================"
}

require_exec() {
  local p="$1"
  local name="$2"
  if [[ ! -x "$p" ]]; then
    echo "Error: $name not found: $p" >&2
    exit 1
  fi
}

clean() {
  print_header "Cleaning"
  rm -rf "$BUILD_DIR"
  rm -f "$SCRIPT_DIR/${TARGET_BASE}" \
        "$SCRIPT_DIR/${TARGET_BASE}.out" \
        "$SCRIPT_DIR/${TARGET_BASE}.map" \
        "$SCRIPT_DIR/${TARGET_BASE}.hex" \
        "$SCRIPT_DIR/${TARGET_BASE}.rom" \
        "$SCRIPT_DIR/${TARGET_BASE}_NONMAPPER.rom"
}

build() {
  print_header "Building Hi-Tech C ROM (app-mode=0, rom-mode=2 concept)"

  require_exec "$BIN/cpp_new3" "cpp_new3"
  require_exec "$BIN/p1x3" "p1x3"
  require_exec "$BIN/cgen3" "cgen3"
  require_exec "$BIN/optim3" "optim3"
  require_exec "$BIN/zasx3" "zasx3"
  require_exec "$BIN/linq3" "linq3"
  require_exec "$BIN/objtohex" "objtohex"
  require_exec "$(command -v python3)" "python3"

  mkdir -p "$BUILD_DIR"

  local tmp_root
  local work
  tmp_root="/tmp/htr"
  mkdir -p "$tmp_root"
  work="$(mktemp -d "$tmp_root/w.XXXXXX")"
  trap 'if [[ -n "${work:-}" ]]; then rm -rf "$work"; fi' RETURN

  cp -f "$SCRIPT_DIR/MAIN.C" "$work/APP.C"
  cp -f "$SCRIPT_DIR/MAIN_HELPER.AS" "$work/MAIN_HELPER.AS"

  local asm_entry
  local asm_hello
  local asm_hello_chput
  asm_hello_chput="$(cat <<'ASM'
	ld	a,'H'
	call	0A2h
	ld	a,'e'
	call	0A2h
	ld	a,'l'
	call	0A2h
	ld	a,'l'
	call	0A2h
	ld	a,'o'
	call	0A2h
	ld	a,' '
	call	0A2h
	ld	a,'W'
	call	0A2h
	ld	a,'o'
	call	0A2h
	ld	a,'r'
	call	0A2h
	ld	a,'l'
	call	0A2h
	ld	a,'d'
	call	0A2h
	ld	a,0Dh
	call	0A2h
	ld	a,0Ah
	call	0A2h
ASM
)"
  if [[ "$ROM_ENTRY_MODE" == "main-pure" ]]; then
    if [[ "$DEBUG_BORDER" == "1" ]]; then
      # Debug mode: use ROM-entry direct path for stable visual diagnostics.
      asm_entry='
	jp	rom_loop
'
      asm_hello="$asm_hello_chput"
    else
      # Proof baseline: ROM entry initializes text mode then dispatches to C main().
      asm_entry='
	global	_main
	call	_main
	; main-pure mode should not return, but keep safe fallback.
	jp	$
'
      asm_hello=''
    fi
  elif [[ "$ROM_ENTRY_MODE" == "loop" ]]; then
    # Fallback baseline: ROM entry prints directly then enters fallback wait path.
    asm_entry='
	jp	fallback_wait
'
    asm_hello="$asm_hello_chput"
  else
    echo "Error: unsupported ROM_ENTRY_MODE: $ROM_ENTRY_MODE (use loop|main-pure)" >&2
    exit 1
  fi

  local border_block
  if [[ "$DEBUG_BORDER" == "1" ]]; then
    border_block='
rom_loop:
	ld	a,b
	and	0fh
	or	0f0h
	defb	0d3h,099h
	ld	a,087h
	defb	0d3h,099h
	ld	de,0FFFFh
rom_wait:
	dec	de
	ld	a,d
	or	e
	jp	nz,rom_wait
	inc	b
	jp	rom_loop
'
  else
    border_block='
rom_loop:
	halt
	jr	rom_loop
'
  fi

  cat > "$work/ROM.AS" <<ASMEOF
	psect	text
	global	_start

_start:
	defs	100h
	defm	'ROM '
	defw	start
start:
	di
	ld	sp,(0FC4Ah)
	call	06Ch
	call	0C3h
${asm_hello}
${asm_entry}
${border_block}

fallback_wait:
	halt
	jr	fallback_wait
ASMEOF

  local cpp_defs
  cpp_defs=(-DANSI -DCPM)
  if [[ "$ROM_ENTRY_MODE" == "main-pure" ]]; then
    cpp_defs+=(-DMAIN_PURE)
  fi
  cpp_defs+=(-DDEBUG_BORDER="$DEBUG_BORDER")

  "$BIN/cpp_new3" "${cpp_defs[@]}" "$work/APP.C" "$work/APP.I"
  "$BIN/p1x3" "$work/APP.I" "$work/APP.P1"
  "$BIN/cgen3" "$work/APP.P1" "$work/APP.AS"
  if [[ "$ROM_ENTRY_MODE" == "main-pure" ]]; then
    # Avoid optimizer instability on complex inline-asm loops in main-pure mode.
    "$BIN/zasx3" -j "$work/APP.AS"
  else
    "$BIN/optim3" "$work/APP.AS" "$work/APP.OPT"
    "$BIN/zasx3" -j "$work/APP.OPT"
  fi
  "$BIN/zasx3" -j "$work/ROM.AS"
  "$BIN/zasx3" -j "$work/MAIN_HELPER.AS"
  local helper_obj
  helper_obj="$work/MAIN_HELPER.obj"
  # Some zasx3 invocations may emit object in current directory.
  if [[ ! -f "$helper_obj" ]]; then
    if [[ -f MAIN_HELPER.obj ]]; then
      mv -f MAIN_HELPER.obj "$helper_obj"
    elif [[ -f MAIN_HELPER.OBJ ]]; then
      mv -f MAIN_HELPER.OBJ "$helper_obj"
    fi
  fi
  if [[ ! -f "$helper_obj" ]]; then
    local helper_found
    helper_found="$(find "$work" -maxdepth 1 -type f \( -iname 'main_helper.obj' -o -iname 'main_helper.OBJ' -o -iname 'main_he.obj' -o -iname 'main_he.OBJ' \) | head -n 1)"
    if [[ -n "$helper_found" ]]; then
      helper_obj="$helper_found"
    else
      echo "Error: MAIN_HELPER object not produced by assembler" >&2
      ls -la "$work" >&2 || true
      exit 1
    fi
  fi

  "$BIN/linq3" -Ptext=4000H -m"$OUT_MAP" -o"$OUT_OBJ" \
    "$work/ROM.obj" "$helper_obj" "$work/APP.obj" "$LIBDIR/LIBCMSXR.LIB"

  "$BIN/objtohex" "$OUT_OBJ" "$OUT_HEX"

  python3 - "$OUT_HEX" "$OUT_ROM" <<'PY'
from pathlib import Path
import sys

hex_path = Path(sys.argv[1])
out_path = Path(sys.argv[2])
rom = bytearray([0xFF] * 0xC000)

for line in hex_path.read_text(encoding="utf-8", errors="ignore").splitlines():
    line = line.strip()
    if not line.startswith(":"):
        continue
    n = int(line[1:3], 16)
    addr = int(line[3:7], 16)
    rtype = int(line[7:9], 16)
    if rtype == 0x01:
        break
    if rtype != 0x00:
        continue
    data = bytes.fromhex(line[9:9 + n * 2])
    # Map CPU address space 0x4000-0xFFFF into file offset 0x0000-0xBFFF.
    if addr < 0x4000:
        continue
    off = addr - 0x4000
    if off >= len(rom):
        continue
    end = min(off + n, len(rom))
    rom[off:end] = data[: end - off]

out_path.write_bytes(rom)
print(f"wrote: {out_path} ({len(rom)} bytes)")
PY

if [[ "$ROM_ENTRY_MODE" == "main-pure" && "$DEBUG_BORDER" != "1" ]]; then
    python3 - "$OUT_ROM" "$OUT_MAP" <<'PY'
from pathlib import Path
import sys

rom_path = Path(sys.argv[1])
map_path = Path(sys.argv[2])
rom = bytearray(rom_path.read_bytes())

# start entry at 0x4106 => file offset 0x0106
entry = 0x0106
if len(rom) < entry + 96:
    raise SystemExit("ROM too small for main-mode call fixup")

if not (rom[entry] == 0xF3 and rom[entry + 1] == 0xED and rom[entry + 2] == 0x7B):
    raise SystemExit("unexpected entry prologue; aborting main-mode call fixup")

call_sites = []
# Entry trampoline window only; avoid touching compiled C body.
for i in range(entry, min(entry + 32, len(rom) - 2)):
    if rom[i] == 0xCD:
        addr = rom[i + 1] | (rom[i + 2] << 8)
        call_sites.append((i, addr))

if not call_sites:
    raise SystemExit("main-mode fixup: no CALL found near entry")

sym_addr = None
if map_path.exists():
    txt = map_path.read_text(encoding="utf-8", errors="ignore")
    tokens = txt.replace("\n", " ").split()
    for i, tok in enumerate(tokens):
        if tok == "_main" and i + 2 < len(tokens) and tokens[i + 1] == "text":
            try:
                sym_addr = int(tokens[i + 2], 16)
                break
            except ValueError:
                pass

if sym_addr is None:
    raise SystemExit("failed to resolve _main symbol from map for fixup")

patched = 0
# Entry CALL _main slot is fixed in this ROM prologue:
# DI, LD SP,(FC4A), CALL INITXT, CALL CLS, CALL _main
main_call_pos = entry + 11
if main_call_pos + 2 >= len(rom) or rom[main_call_pos] != 0xCD:
    raise SystemExit("main-mode fixup: expected CALL opcode at entry main-call slot")
main_call_addr = rom[main_call_pos + 1] | (rom[main_call_pos + 2] << 8)
if main_call_addr != sym_addr:
    rom[main_call_pos + 1] = sym_addr & 0xFF
    rom[main_call_pos + 2] = (sym_addr >> 8) & 0xFF
    patched += 1

main_calls = 0
for pos, _ in call_sites:
    addr = rom[pos + 1] | (rom[pos + 2] << 8)
    if addr == sym_addr:
        main_calls += 1

rom_path.write_bytes(rom)
print(
    f"main-call-fixup: calls={len(call_sites)} patched={patched} "
    f"main_calls={main_calls} map_main=0x{sym_addr:04X}"
)
PY
  fi

  python3 "$PACKER" \
    --input "$OUT_ROM" \
    --output "$OUT_CART_ROM" \
    --init 0x4106 \
    --no-loader \
    --strict \
    --report "$BUILD_DIR/${TARGET_BASE}_pack_report.json"

  echo "ROM_ENTRY_MODE=$ROM_ENTRY_MODE"
  echo "DEBUG_BORDER=$DEBUG_BORDER"

  cp -f "$OUT_OBJ" "$SCRIPT_DIR/${TARGET_BASE}.out"
  cp -f "$OUT_MAP" "$SCRIPT_DIR/${TARGET_BASE}.map"
  cp -f "$OUT_HEX" "$SCRIPT_DIR/${TARGET_BASE}.hex"
  cp -f "$OUT_ROM" "$SCRIPT_DIR/${TARGET_BASE}.rom"
  cp -f "$OUT_CART_ROM" "$SCRIPT_DIR/${TARGET_BASE}_NONMAPPER.rom"
}

verify() {
  print_header "Verifying outputs"

  if [[ ! -f "$OUT_ROM" ]]; then
    echo "Error: missing $OUT_ROM" >&2
    exit 1
  fi

  if [[ ! -f "$OUT_CART_ROM" ]]; then
    echo "Error: missing $OUT_CART_ROM" >&2
    exit 1
  fi

  local size_raw
  local sig_raw
  size_raw="$(stat -c%s "$OUT_ROM")"
  sig_raw="$(xxd -p -s 0x100 -l 4 "$OUT_ROM")"

  if [[ "$size_raw" -ne 49152 ]]; then
    echo "Error: invalid raw ROM size: $size_raw" >&2
    exit 1
  fi
  if [[ "$sig_raw" != "524f4d20" ]]; then
    echo "Error: invalid raw ROM signature at 0x100: $sig_raw" >&2
    exit 1
  fi

  local size_cart
  local sig_cart
  local init_cart
  size_cart="$(stat -c%s "$OUT_CART_ROM")"
  sig_cart="$(xxd -p -l 2 "$OUT_CART_ROM")"
  init_cart="$(xxd -p -s 2 -l 2 "$OUT_CART_ROM")"

  if [[ "$size_cart" -ne 49152 ]]; then
    echo "Error: invalid cartridge ROM size: $size_cart" >&2
    exit 1
  fi
  if [[ "$sig_cart" != "4142" ]]; then
    echo "Error: invalid cartridge signature: $sig_cart" >&2
    exit 1
  fi
  if [[ "$init_cart" != "0641" ]]; then
    echo "Error: AB init is not 0x4106: $init_cart" >&2
    exit 1
  fi

  python3 - "$OUT_ROM" <<'PY'
from pathlib import Path
import sys
b = Path(sys.argv[1]).read_bytes()
# Inline CHPUT sequence for "Hello World\r\n":
# ld a,'H'; call 00A2h; ... ; ld a,0Dh; call 00A2h; ld a,0Ah; call 00A2h
sig = bytes([
    0x3E, 0x48, 0xCD, 0xA2, 0x00,
    0x3E, 0x65, 0xCD, 0xA2, 0x00,
    0x3E, 0x6C, 0xCD, 0xA2, 0x00,
    0x3E, 0x6C, 0xCD, 0xA2, 0x00,
    0x3E, 0x6F, 0xCD, 0xA2, 0x00,
    0x3E, 0x20, 0xCD, 0xA2, 0x00,
    0x3E, 0x57, 0xCD, 0xA2, 0x00,
    0x3E, 0x6F, 0xCD, 0xA2, 0x00,
    0x3E, 0x72, 0xCD, 0xA2, 0x00,
    0x3E, 0x6C, 0xCD, 0xA2, 0x00,
    0x3E, 0x64, 0xCD, 0xA2, 0x00,
    0x3E, 0x0D, 0xCD, 0xA2, 0x00,
    0x3E, 0x0A, 0xCD, 0xA2, 0x00,
])
if sig not in b:
    raise SystemExit("Error: Hello World CHPUT opcode pattern not found in ROM image")
print("OK: Hello World CHPUT opcode pattern found in ROM image")
PY

  echo "OK: HELLO48.rom size=49152 sig@0x100='ROM '"
  echo "OK: HELLO48_NONMAPPER.rom size=49152 sig='AB'"
  xxd -g1 -l 16 "$OUT_CART_ROM"
  xxd -g1 -s 0x100 -l 16 "$OUT_ROM"
}

verify_main_pure_mode() {
  print_header "Verifying main-pure entry path"

  local saved_mode
  saved_mode="$ROM_ENTRY_MODE"
  ROM_ENTRY_MODE=main-pure build
  verify

  python3 - "$OUT_ROM" "$OUT_MAP" <<'PY'
from pathlib import Path
import sys

rom = Path(sys.argv[1]).read_bytes()
map_txt = Path(sys.argv[2]).read_text(encoding="utf-8", errors="ignore")

tokens = map_txt.replace("\n", " ").split()
main_addr = None
for i, tok in enumerate(tokens):
    if tok == "_main" and i + 2 < len(tokens) and tokens[i + 1] == "text":
        try:
            main_addr = int(tokens[i + 2], 16)
            break
        except ValueError:
            pass

if main_addr is None:
    raise SystemExit("verify-main-pure: _main symbol not found in map")

entry = 0x0106
if len(rom) < entry + 16:
    raise SystemExit("verify-main-pure: ROM too small")

# Require bios setup calls near entry.
if not (rom[entry + 5] == 0xCD and rom[entry + 6] == 0x6C and rom[entry + 7] == 0x00):
    raise SystemExit("verify-main-pure: missing CALL INITXT (0x006C) at entry+5")
if not (rom[entry + 8] == 0xCD and rom[entry + 9] == 0xC3 and rom[entry + 10] == 0x00):
    raise SystemExit("verify-main-pure: missing CALL CLS (0x00C3) at entry+8")

# Require at least one CALL _main in entry window.
found_main_call = False
for i in range(entry, min(entry + 64, len(rom) - 2)):
    if rom[i] == 0xCD:
        addr = rom[i + 1] | (rom[i + 2] << 8)
        if addr == main_addr:
            found_main_call = True
            break
if not found_main_call:
    raise SystemExit("verify-main-pure: CALL _main not found in entry window")

# main-pure fallback after CALL _main remains JP $.
if b"\x18\xfe" not in rom[entry:entry + 112]:
    raise SystemExit("verify-main-pure: missing JP $ fallback near entry")

print(
    f"verify-main-pure: PASS main=0x{main_addr:04X}"
)
PY

  ROM_ENTRY_MODE="$saved_mode"
}

verify_fallback_mode() {
  print_header "Verifying fallback(loop) entry path"

  local saved_mode
  saved_mode="$ROM_ENTRY_MODE"
  ROM_ENTRY_MODE=loop build
  verify

  python3 - "$OUT_ROM" <<'PY'
from pathlib import Path
import sys

rom = Path(sys.argv[1]).read_bytes()
entry = 0x0106

if len(rom) < entry + 24:
    raise SystemExit("verify-fallback: ROM too small")

# Entry prologue and text init calls should still exist.
if not (rom[entry] == 0xF3 and rom[entry + 1] == 0xED and rom[entry + 2] == 0x7B):
    raise SystemExit("verify-fallback: missing expected entry prologue")
if not (rom[entry + 5] == 0xCD and rom[entry + 6] == 0x6C and rom[entry + 7] == 0x00):
    raise SystemExit("verify-fallback: missing CALL INITXT (0x006C)")
if not (rom[entry + 8] == 0xCD and rom[entry + 9] == 0xC3 and rom[entry + 10] == 0x00):
    raise SystemExit("verify-fallback: missing CALL CLS (0x00C3)")

# Locate inline Hello sequence and verify fallback branch right after it.
hello_sig = bytes([
    0x3E, 0x48, 0xCD, 0xA2, 0x00,
    0x3E, 0x65, 0xCD, 0xA2, 0x00,
    0x3E, 0x6C, 0xCD, 0xA2, 0x00,
    0x3E, 0x6C, 0xCD, 0xA2, 0x00,
    0x3E, 0x6F, 0xCD, 0xA2, 0x00,
    0x3E, 0x20, 0xCD, 0xA2, 0x00,
    0x3E, 0x57, 0xCD, 0xA2, 0x00,
    0x3E, 0x6F, 0xCD, 0xA2, 0x00,
    0x3E, 0x72, 0xCD, 0xA2, 0x00,
    0x3E, 0x6C, 0xCD, 0xA2, 0x00,
    0x3E, 0x64, 0xCD, 0xA2, 0x00,
    0x3E, 0x0D, 0xCD, 0xA2, 0x00,
    0x3E, 0x0A, 0xCD, 0xA2, 0x00,
])
pos = rom.find(hello_sig)
if pos < 0:
    raise SystemExit("verify-fallback: Hello sequence not found")
tail = pos + len(hello_sig)
if tail >= len(rom):
    raise SystemExit("verify-fallback: invalid hello tail")

op = rom[tail]
if op not in (0xC3, 0x18):
    raise SystemExit(f"verify-fallback: expected JP/JR after Hello, got 0x{op:02X}")

# Require wait-loop pattern near entry window.
win = rom[entry:entry + 256]
if b"\x76\x18\xFD" not in win:
    raise SystemExit("verify-fallback: missing HALT/JR fallback wait loop pattern")

branch_kind = "JP" if op == 0xC3 else "JR"
print(f"verify-fallback: PASS branch_after_hello={branch_kind}")
PY

  ROM_ENTRY_MODE="$saved_mode"
}

usage() {
  cat <<USAGE
Usage: $0 [clean|build|verify|verify-main-pure|verify-fallback|all]
USAGE
}

case "${1:-all}" in
  clean)
    clean
    ;;
  build)
    build
    ;;
  verify)
    verify
    ;;
  verify-main-pure)
    verify_main_pure_mode
    ;;
  verify-fallback)
    verify_fallback_mode
    ;;
  all)
    clean
    build
    verify
    ;;
  *)
    usage
    exit 1
    ;;
esac
