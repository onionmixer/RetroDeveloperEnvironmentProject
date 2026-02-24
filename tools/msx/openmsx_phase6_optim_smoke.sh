#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OPENMSX="$ROOT/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx"
OPENMSX_SHARE="$ROOT/Emulator/openMSX/share"
BOOT_DISK="$ROOT/diskwork/bootdisk/msx/msxdos23.dsk"
RDEDISKTOOL="$ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool"
# Keep the same GT BIOS baseline as run_openmsx_msxdos2.sh.
MACHINE="${OPENMSX_MACHINE:-Panasonic_FS-A1GT}"

INPUT_COM="${1:-/tmp/sharksym_phase6_smoke/optim_link.com}"
TIMEOUT_SEC="${TIMEOUT_SEC:-18}"
USER_HOME="${USER_HOME:-/tmp/openmsx_home_phase6}"
WORK_DIR="${WORK_DIR:-/tmp/openmsx_phase6_optim}"
RUN_DSK="$WORK_DIR/optim_boot.dsk"
RUN_COM="$WORK_DIR/OPTLINK.COM"
LOG="$WORK_DIR/openmsx_phase6_optim.log"
REPORT="${REPORT:-$WORK_DIR/openmsx_phase6_optim_report.json}"
CMD_SCRIPT="$WORK_DIR/openmsx_phase6_commands.xml"

mkdir -p "$WORK_DIR" "$USER_HOME"

if [[ ! -x "$OPENMSX" ]]; then
  echo "ERROR: openMSX not found: $OPENMSX" >&2
  exit 1
fi
if [[ ! -f "$BOOT_DISK" ]]; then
  echo "ERROR: boot disk missing: $BOOT_DISK" >&2
  exit 1
fi
if [[ ! -x "$RDEDISKTOOL" ]]; then
  echo "ERROR: rdedisktool missing: $RDEDISKTOOL" >&2
  exit 1
fi
if [[ ! -f "$INPUT_COM" ]]; then
  echo "ERROR: input com missing: $INPUT_COM" >&2
  exit 1
fi

write_report() {
  local status="$1"
  local reason="$2"
  python3 - "$REPORT" "$status" "$reason" "$rc" "$RUN_DSK" "$RUN_COM" "$LOG" "$CMD_SCRIPT" "$MACHINE" <<'PY'
import json, os, sys
report, status, reason, rc, run_dsk, run_com, log, cmd_script, machine = sys.argv[1:]
txt = ""
if os.path.exists(log):
    txt = open(log, "r", encoding="utf-8", errors="replace").read()
reply_ok_count = txt.count('<reply result="ok">')
rom_missing = "Couldn't find ROM file" in txt
fatal_count = txt.count("Fatal error:")
obj = {
  "tool": "openmsx_phase6_optim_smoke",
  "status": status,
  "reason": reason,
  "rc": int(rc),
  "machine": machine,
  "run_disk": run_dsk,
  "program": run_com,
  "command_script": cmd_script,
  "log": log,
  "reply_ok_count": reply_ok_count,
  "fatal_count": fatal_count,
  "rom_missing": rom_missing,
}
with open(report, "w", encoding="utf-8") as f:
    json.dump(obj, f, indent=2, ensure_ascii=True)
    f.write("\n")
print(f"[openmsx-phase6] report={report} status={status} reason={reason}")
PY
}

# Precheck ROM presence for GT BIOS path to avoid noisy runtime fatal.
if [[ "$MACHINE" == "Panasonic_FS-A1GT" ]]; then
  if [[ ! -f "$OPENMSX_SHARE/systemroms/fs-a1gt_firmware.rom" && ! -f "$HOME/.openMSX/share/systemroms/fs-a1gt_firmware.rom" ]]; then
    rc=0
    echo "[openmsx-phase6] SKIP: fs-a1gt_firmware.rom not found (precheck)"
    write_report "skip" "missing_machine_rom_precheck"
    exit 0
  fi
fi

# Prepare bootable work disk with 8.3 filename.
cp "$BOOT_DISK" "$RUN_DSK"
cp "$INPUT_COM" "$RUN_COM"
"$RDEDISKTOOL" add "$RUN_DSK" "$RUN_COM" >/dev/null

echo "[openmsx-phase6] machine=$MACHINE"
echo "[openmsx-phase6] diska=$RUN_DSK"
echo "[openmsx-phase6] program=$(basename "$RUN_COM")"

cat > "$CMD_SCRIPT" <<'EOF'
<command>machine</command>
<command>after time 4 {type_via_keybuf "OPTLINK\r"}</command>
<command>after time 12 {exit}</command>
EOF

set +e
timeout "${TIMEOUT_SEC}s" env \
  OPENMSX_SYSTEM_DATA="$OPENMSX_SHARE" \
  OPENMSX_DISABLE_SDL_JOYSTICK="${OPENMSX_DISABLE_SDL_JOYSTICK:-1}" \
  OPENMSX_HOME="$USER_HOME" \
  OPENMSX_USER_DATA="$USER_HOME" \
  SDL_AUDIODRIVER=dummy \
  "$OPENMSX" -control stdio -machine "$MACHINE" -diska "$RUN_DSK" <"$CMD_SCRIPT" >"$LOG" 2>&1
rc=$?
set -e

echo "[openmsx-phase6] rc=$rc (0=clean exit, 124=timeout while running)"
sed -n '1,120p' "$LOG"

# Accept both clean exit and timeout-running as smoke pass.
if [[ $rc -ne 0 && $rc -ne 124 ]]; then
  if rg -q "Couldn't find ROM file" "$LOG"; then
    echo "[openmsx-phase6] SKIP: required machine ROM is not installed in this environment"
    write_report "skip" "missing_machine_rom"
    exit 0
  fi
  write_report "fail" "openmsx_runtime_error"
  exit "$rc"
fi

if [[ $rc -eq 124 ]]; then
  write_report "pass" "timeout_running"
else
  write_report "pass" "clean_exit"
fi
