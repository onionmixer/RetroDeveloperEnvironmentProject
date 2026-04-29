#!/usr/bin/env bash
# Macintosh emulator launch script (snow)
#
# Usage: ./run_snow_mac.sh
#
# Optional environment overrides:
#   SNOW=...       snow binary path (default: Emulator/macintosh/snow/target/release/snowemu)
#   MAC_MODEL=...  plus | se_fdhd | classic | se30 | macii  (default: se_fdhd)
#                  Resolves to a ROM filename in resource/Macintosh/rom/.
#   MAC_ROM=...    Explicit ROM path; overrides MAC_MODEL resolution when set.
#   BOOT_DISK=...  Mac boot disk image to mount on startup (optional)
#                  Accepts DiskCopy 4.2 (.image/.dc42) or raw sector (.img/.dsk).
#                  When omitted, snow boots to the chooser and you load images via the GUI.
#   EXTRA_DISKS=.. Colon-separated list of additional floppy images to mount alongside
#                  BOOT_DISK. Useful for two-floppy workflows (System disk + app disk).
#
# Default model is SE FDHD (1.44 MB SuperDrive) so System 6.0.x / 7.x 1.44 MB disks
# boot out of the box. Override with MAC_MODEL=plus for 800K-only Mac Plus emulation.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

first_existing_file() {
    local p
    for p in "$@"; do
        if [[ -f "$p" ]]; then
            printf '%s\n' "$p"
            return 0
        fi
    done
    return 1
}

first_existing_exec() {
    local p
    for p in "$@"; do
        if [[ -x "$p" ]]; then
            printf '%s\n' "$p"
            return 0
        fi
    done
    return 1
}

SNOW="${SNOW:-}"
MAC_MODEL="${MAC_MODEL:-se_fdhd}"   # plus | se_fdhd | classic | se30 | macii (resolves to ROM filename)
MAC_ROM="${MAC_ROM:-}"
BOOT_DISK="${BOOT_DISK:-}"

if [[ -z "$SNOW" ]]; then
    SNOW="$(first_existing_exec \
        "$SCRIPT_DIR/Emulator/macintosh/snow/target/release/snowemu" \
        "$(command -v snowemu 2>/dev/null || true)" \
    )" || SNOW="$SCRIPT_DIR/Emulator/macintosh/snow/target/release/snowemu"
fi

if [[ ! -x "$SNOW" ]]; then
    echo "error: snow binary not found at $SNOW" >&2
    echo "       build it first: (cd Emulator/macintosh/snow && cargo build --release)" >&2
    exit 1
fi

# Resolve MAC_ROM from MAC_MODEL when MAC_ROM is not explicitly set.
# Supports both lowercase and original casing of ROM filenames since macmade/Macintosh-ROMs
# distributes them in mixed case (SE_FDHD.rom, Classic.rom, etc.).
if [[ -z "$MAC_ROM" ]]; then
    case "$MAC_MODEL" in
        plus)     CANDIDATES=("plus.rom" "Plus.rom") ;;
        se_fdhd)  CANDIDATES=("SE_FDHD.rom" "se_fdhd.rom" "se.rom") ;;
        classic)  CANDIDATES=("Classic.rom" "classic.rom") ;;
        se30)     CANDIDATES=("SE30.rom" "se30.rom" "MacSE30.rom") ;;
        macii)    CANDIDATES=("MacII.rom" "macii.rom") ;;
        *)
            echo "error: unknown MAC_MODEL '$MAC_MODEL' (supported: plus, se_fdhd, classic, se30, macii)" >&2
            echo "       or set MAC_ROM explicitly to bypass model resolution" >&2
            exit 1 ;;
    esac
    PATHS=()
    for n in "${CANDIDATES[@]}"; do PATHS+=("$SCRIPT_DIR/resource/Macintosh/rom/$n"); done
    MAC_ROM="$(first_existing_file "${PATHS[@]}")" || MAC_ROM="${PATHS[0]}"
fi

if [[ ! -f "$MAC_ROM" ]]; then
    echo "error: Mac ROM not found at $MAC_ROM" >&2
    echo "       drop a Mac Plus ROM (128 KiB) at resource/Macintosh/rom/plus.rom" >&2
    echo "       (ROMs are copyrighted and not committed; see resource/Macintosh/rom/README.md)" >&2
    exit 1
fi

if [[ -z "$BOOT_DISK" ]]; then
    # Prefer raw (.img) over DC42 (.image) — snow's bundled fluxfox auto-detects raw cleanly,
    # while .image triggers a RawSectorImage misdetection that includes the 84-byte DC42
    # header in sector 0 and corrupts the boot block.
    BOOT_DISK="$(first_existing_file \
        "$SCRIPT_DIR/diskwork/bootdisk/macintosh/system_608.img" \
        "$SCRIPT_DIR/diskwork/bootdisk/macintosh/system_608.image" \
        "$SCRIPT_DIR/diskwork/bootdisk/macintosh/system_608_master.image" \
    )" || BOOT_DISK=""
fi

ARGS=("$MAC_ROM")
if [[ -n "$BOOT_DISK" ]]; then
    if [[ ! -f "$BOOT_DISK" ]]; then
        echo "error: BOOT_DISK not found: $BOOT_DISK" >&2
        exit 1
    fi
    ARGS+=(--floppy "$BOOT_DISK")
fi

EXTRA_DISKS_LIST=()
if [[ -n "${EXTRA_DISKS:-}" ]]; then
    IFS=':' read -r -a EXTRA_DISKS_LIST <<< "$EXTRA_DISKS"
    for d in "${EXTRA_DISKS_LIST[@]}"; do
        [[ -z "$d" ]] && continue
        if [[ ! -f "$d" ]]; then
            echo "error: EXTRA_DISKS entry not found: $d" >&2
            exit 1
        fi
        ARGS+=(--floppy "$d")
    done
fi

ROM_SIG=""
if command -v xxd >/dev/null 2>&1; then
    ROM_SIG="$(xxd -l 4 -p "$MAC_ROM" 2>/dev/null)"
fi
ROM_SIZE="$(stat -c%s "$MAC_ROM" 2>/dev/null || echo '?')"

echo "snow:     $SNOW"
echo "model:    $MAC_MODEL"
echo "ROM:      $MAC_ROM (${ROM_SIZE} bytes, sig=${ROM_SIG:-?})"
[[ -n "$BOOT_DISK" ]] && echo "Disk:     $BOOT_DISK"
for d in "${EXTRA_DISKS_LIST[@]}"; do
    [[ -n "$d" ]] && echo "+Disk:    $d"
done

exec "$SNOW" "${ARGS[@]}"
