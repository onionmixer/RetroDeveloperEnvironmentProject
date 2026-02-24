#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OPENMSX="$ROOT/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx"
OPENMSX_SHARE="$ROOT/Emulator/openMSX/share"
BOOT_DISK="$ROOT/diskwork/bootdisk/msx/msxdos23.dsk"
MACHINE="${OPENMSX_MACHINE:-Panasonic_FS-A1GT}"
TIMEOUT_SEC="${TIMEOUT_SEC:-5}"
WORK_DIR="${WORK_DIR:-/tmp/openmsx_gui_precheck}"
LOG="$WORK_DIR/openmsx_gui_precheck.log"
REPORT="${REPORT:-$WORK_DIR/openmsx_gui_precheck_report.json}"
TRY_GUI_RUNTIME="${TRY_GUI_RUNTIME:-0}"

mkdir -p "$WORK_DIR"

status="pass"
reason="ready"
binary_ok=0
share_ok=0
boot_disk_ok=0
rom_ok=0
display_ok=0
runtime_rc=-1
runtime_checked=0

if [[ -x "$OPENMSX" ]]; then binary_ok=1; else status="fail"; reason="missing_openmsx_binary"; fi
if [[ -d "$OPENMSX_SHARE" ]]; then share_ok=1; else status="fail"; reason="missing_openmsx_share"; fi
if [[ -f "$BOOT_DISK" ]]; then boot_disk_ok=1; else status="fail"; reason="missing_boot_disk"; fi

if [[ -f "$OPENMSX_SHARE/systemroms/fs-a1gt_firmware.rom" || -f "$HOME/.openMSX/share/systemroms/fs-a1gt_firmware.rom" ]]; then
  rom_ok=1
else
  if [[ "$status" == "pass" ]]; then
    status="skip"
    reason="missing_machine_rom"
  fi
fi

if [[ -n "${DISPLAY:-}" || -n "${WAYLAND_DISPLAY:-}" ]]; then
  display_ok=1
else
  if [[ "$status" == "pass" ]]; then
    status="skip"
    reason="missing_display_env"
  fi
fi

if [[ "$TRY_GUI_RUNTIME" == "1" && "$status" == "pass" ]]; then
  runtime_checked=1
  set +e
  timeout "${TIMEOUT_SEC}s" env \
    OPENMSX_SYSTEM_DATA="$OPENMSX_SHARE" \
    OPENMSX_DISABLE_SDL_JOYSTICK="${OPENMSX_DISABLE_SDL_JOYSTICK:-1}" \
    SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-dummy}" \
    "$OPENMSX" -machine "$MACHINE" -diska "$BOOT_DISK" >"$LOG" 2>&1
  runtime_rc=$?
  set -e
  if [[ $runtime_rc -ne 0 && $runtime_rc -ne 124 ]]; then
    status="fail"
    reason="gui_runtime_error"
  fi
fi

python3 - "$REPORT" "$status" "$reason" "$OPENMSX" "$OPENMSX_SHARE" "$BOOT_DISK" "$MACHINE" "$binary_ok" "$share_ok" "$boot_disk_ok" "$rom_ok" "$display_ok" "$runtime_checked" "$runtime_rc" "$TRY_GUI_RUNTIME" "$LOG" <<'PY'
import json, os, sys
(
  report, status, reason, openmsx, share, boot_disk, machine,
  binary_ok, share_ok, boot_disk_ok, rom_ok, display_ok,
  runtime_checked, runtime_rc, try_gui_runtime, log_path
) = sys.argv[1:]
obj = {
  "tool": "openmsx_gui_precheck",
  "status": status,
  "reason": reason,
  "openmsx": openmsx,
  "openmsx_share": share,
  "boot_disk": boot_disk,
  "machine": machine,
  "binary_ok": bool(int(binary_ok)),
  "share_ok": bool(int(share_ok)),
  "boot_disk_ok": bool(int(boot_disk_ok)),
  "rom_ok": bool(int(rom_ok)),
  "display_ok": bool(int(display_ok)),
  "try_gui_runtime": bool(int(try_gui_runtime)),
  "runtime_checked": bool(int(runtime_checked)),
  "runtime_rc": int(runtime_rc),
  "runtime_log": log_path if os.path.exists(log_path) else "",
}
with open(report, "w", encoding="utf-8") as f:
    json.dump(obj, f, indent=2, ensure_ascii=True)
    f.write("\n")
print(f"[openmsx-gui-precheck] report={report} status={status} reason={reason}")
PY

