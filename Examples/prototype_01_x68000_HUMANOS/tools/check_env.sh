#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
TOOLCHAIN_DIR="$PROJECT_ROOT/Toolchain/x68000/toolchain/m68k-xelf"

CC="$TOOLCHAIN_DIR/bin/m68k-xelf-gcc"
RUN68="$TOOLCHAIN_DIR/bin/run68"
PX_SCRIPT="$PROJECT_ROOT/run_px68k_humanos.sh"

need_exec() {
    local p="$1"
    local msg="$2"
    if [[ ! -x "$p" ]]; then
        echo "[ERR] $msg: $p"
        exit 1
    fi
    echo "[OK ] $msg: $p"
}

need_file() {
    local p="$1"
    local msg="$2"
    if [[ ! -f "$p" ]]; then
        echo "[ERR] $msg: $p"
        exit 1
    fi
    echo "[OK ] $msg: $p"
}

need_exec "$CC" "m68k-xelf-gcc"
need_exec "$RUN68" "run68"
need_exec "$PX_SCRIPT" "run_px68k_humanos.sh"

if command -v rdedisktool >/dev/null 2>&1; then
    echo "[OK ] rdedisktool: $(command -v rdedisktool)"
elif [[ -x "$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool" ]]; then
    echo "[OK ] rdedisktool: $PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool"
elif [[ -x "$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool" ]]; then
    echo "[OK ] rdedisktool: $PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool"
else
    echo "[WARN] rdedisktool not found in PATH/local build"
fi

need_file "$PROJECT_ROOT/diskwork/bootdisk/x68000/HUMAN302.XDF" "Human68k boot disk"
need_file "$PROJECT_ROOT/Emulator/x68000/px68k-onionmixer/iplrom.dat" "IPL ROM"
need_file "$PROJECT_ROOT/Emulator/x68000/px68k-onionmixer/cgrom.dat" "CG ROM"

"$CC" --version | sed -n '1,1p'
echo "Environment check: OK"
