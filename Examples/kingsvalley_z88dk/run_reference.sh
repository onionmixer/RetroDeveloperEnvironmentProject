#!/usr/bin/env bash
# run_reference.sh — boot the original (upstream-published) kingsvalley ROM
# in openMSX. Used as a diagnostic baseline: confirms what kingsvalley
# is *supposed* to look/behave like on the same machine we target with
# our z88dk port.
#
# ROMs are auto-downloaded from the upstream release (pdpdds/ubox_example
# v1.0) into ./ref_roms/ on first run. Three variants exist:
#
#   kings_original.rom — closest to upstream original kingsvalley
#   kings.rom          — pdpdds rebuild (v1)
#   kings_ver2.rom     — v2 with map tweaks
#
# Each is 32 KiB plain MSX cart (no Konami mapper writes) — verified.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
REF_DIR="$SCRIPT_DIR/ref_roms"

OPENMSX="${OPENMSX:-$PROJECT_ROOT/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx}"
OPENMSX_SHARE="${OPENMSX_SHARE:-$PROJECT_ROOT/Emulator/openMSX/share}"
MACHINE="${MACHINE:-Panasonic_FS-A1GT}"

# Pick which reference ROM to run (default: original)
VARIANT="${1:-original}"
case "$VARIANT" in
    original|orig) ROM_NAME="kings_original.rom" ;;
    v1|kings|"")   ROM_NAME="kings.rom" ;;
    v2|ver2)       ROM_NAME="kings_ver2.rom" ;;
    *)
        echo "usage: $0 {original|v1|v2}" >&2
        echo "  original — kings_original.rom (default)" >&2
        echo "  v1       — kings.rom" >&2
        echo "  v2       — kings_ver2.rom" >&2
        exit 2
        ;;
esac

ROM_PATH="$REF_DIR/$ROM_NAME"
RELEASE_BASE="https://github.com/pdpdds/ubox_example/releases/download/v1.0"

# Ensure prerequisites
[[ -x "$OPENMSX" ]] || { echo "Error: openmsx not found at $OPENMSX" >&2; exit 1; }
command -v curl >/dev/null || { echo "Error: curl required for first-run download" >&2; exit 1; }

# Auto-download if missing
mkdir -p "$REF_DIR"
if [[ ! -f "$ROM_PATH" ]]; then
    echo "[fetch] Downloading reference ROMs from upstream release..."
    for f in kings_original.rom kings.rom kings_ver2.rom; do
        if [[ ! -f "$REF_DIR/$f" ]]; then
            echo "  $f"
            curl -fsSLo "$REF_DIR/$f" "$RELEASE_BASE/$f" \
                || { echo "  Error: failed to fetch $f" >&2; exit 1; }
        fi
    done
fi

# Sanity checks
SIZE="$(wc -c < "$ROM_PATH")"
[[ "$SIZE" == "32768" ]] || { echo "Warning: $ROM_NAME is $SIZE bytes, expected 32768" >&2; }

SIG="$(xxd -p -l 2 "$ROM_PATH")"
[[ "$SIG" == "4142" ]] || { echo "Warning: $ROM_NAME signature is $SIG, expected 4142 (AB)" >&2; }

export OPENMSX_SYSTEM_DATA="$OPENMSX_SHARE"
export OPENMSX_DISABLE_SDL_JOYSTICK="${OPENMSX_DISABLE_SDL_JOYSTICK:-1}"

echo "Booting reference ROM: $ROM_NAME ($SIZE bytes)"
echo "  Machine: $MACHINE"
echo "  ROM path: $ROM_PATH"

# Plain 32 KiB cart — let openMSX auto-detect mapper (kingsvalley ROMs are
# in openMSX's hash database; should resolve to Plain/MSXRom mapper).
exec "$OPENMSX" -machine "$MACHINE" -carta "$ROM_PATH"
