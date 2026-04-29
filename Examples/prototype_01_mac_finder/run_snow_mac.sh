#!/usr/bin/env bash
# prototype_01_mac_finder — single-disk snow launcher
#
# Boots snow (Mac SE FDHD) from the per-example combined image
# `system_608_with_hello.img` (System 6.0.8 trimmed boot disk + Hello app
# injected via rdedisktool add --macbinary). After boot, double-click
# `Hello` on the desktop, then click the button to quit.
#
# Optional environment overrides:
#   SNOW=...         snow binary path
#   MAC_MODEL=...    plus | se_fdhd | classic | se30 | macii   (default: se_fdhd)
#   MAC_ROM=...      explicit ROM path (overrides MAC_MODEL)
#   BOOT_DISK=...    explicit boot disk path (default: this script's local
#                    system_608_with_hello.img)
#   EXTRA_DISKS=...  colon-separated extra floppy images (rarely needed —
#                    the local disk already contains both System and Hello)
#
# Regenerating the local disk: see ./compile.sh disk (rebuilds from project
# root's diskwork/bootdisk/macintosh/system_608.img + build/Hello.bin via
# rdedisktool add --macbinary).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

LOCAL_DISK="$SCRIPT_DIR/system_608_with_hello.img"

if [[ ! -f "$LOCAL_DISK" && -z "${BOOT_DISK:-}" ]]; then
    echo "error: $LOCAL_DISK not found" >&2
    echo "       regenerate it: $SCRIPT_DIR/compile.sh disk" >&2
    echo "       (requires Hello.bin built + diskwork/bootdisk/macintosh/system_608.img + rdedisktool)" >&2
    exit 1
fi

export BOOT_DISK="${BOOT_DISK:-$LOCAL_DISK}"

exec "$PROJECT_ROOT/run_snow_mac.sh"
