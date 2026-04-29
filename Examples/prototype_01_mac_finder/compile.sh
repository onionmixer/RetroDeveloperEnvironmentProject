#!/usr/bin/env bash
# prototype_01_mac_finder — Retro68 build wrapper
#
# Targets: build | disk | clean | test | all
#
# Optional environment overrides:
#   RETRO68_BUILD=...   path to Library/Retro68-build (default: <repo>/Library/Retro68-build)
#   TOOLCHAIN_FILE=...  cmake toolchain file (default: $RETRO68_BUILD/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake)
#   RDEDISKTOOL=...     rdedisktool binary (default: <repo>/RetroDeveloperEnvironmentDisktool/build/rdedisktool)
#   MASTER_DISK=...     trimmed System 6.0.8 boot disk to clone (default:
#                       <repo>/diskwork/bootdisk/macintosh/system_608.img)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

RETRO68_BUILD="${RETRO68_BUILD:-$PROJECT_ROOT/Library/Retro68-build}"
TOOLCHAIN_FILE="${TOOLCHAIN_FILE:-$RETRO68_BUILD/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake}"
RDEDISKTOOL="${RDEDISKTOOL:-$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool}"
MASTER_DISK="${MASTER_DISK:-$PROJECT_ROOT/diskwork/bootdisk/macintosh/system_608.img}"
LOCAL_DISK="$SCRIPT_DIR/system_608_with_hello.img"

cd "$SCRIPT_DIR"

require_toolchain() {
    if [[ ! -f "$TOOLCHAIN_FILE" ]]; then
        echo "error: Retro68 cmake toolchain file not found at $TOOLCHAIN_FILE" >&2
        echo "       build it first: (mkdir -p $RETRO68_BUILD && cd $RETRO68_BUILD && ../Retro68/build-toolchain.bash --no-ppc)" >&2
        exit 1
    fi
}

clean() {
    rm -rf "$BUILD_DIR"
    rm -f  "$LOCAL_DISK"
}

disk() {
    local app_bin="$BUILD_DIR/Hello.bin"
    if [[ ! -f "$app_bin" ]]; then
        echo "error: $app_bin not found — run '$0 build' first" >&2
        exit 1
    fi
    if [[ ! -f "$MASTER_DISK" ]]; then
        echo "error: master boot disk not found at $MASTER_DISK" >&2
        echo "       expected the trimmed System 6.0.8 image (created earlier in this project)" >&2
        exit 1
    fi
    if [[ ! -x "$RDEDISKTOOL" ]]; then
        echo "error: rdedisktool not found at $RDEDISKTOOL" >&2
        echo "       build it: (cd RetroDeveloperEnvironmentDisktool && mkdir -p build && cd build && cmake .. && make)" >&2
        exit 1
    fi
    echo "Cloning master boot disk → $LOCAL_DISK"
    cp -f "$MASTER_DISK" "$LOCAL_DISK"
    # rdedisktool 'add --macbinary' (since 0ebb152) writes both forks + Finder
    # info into the HFS catalog. --bootdisk-mode warn lets us mutate a System
    # disk while still running the safe-add verification (no boot-block /
    # System / Finder corruption).
    echo "Injecting Hello.bin via rdedisktool add --macbinary ..."
    "$RDEDISKTOOL" --bootdisk-mode warn add --force --macbinary "$LOCAL_DISK" "$app_bin"
    echo
    echo "Result:"
    ls -lh "$LOCAL_DISK"
    "$RDEDISKTOOL" list "$LOCAL_DISK" -v
}

build() {
    require_toolchain
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" -DCMAKE_BUILD_TYPE=Release "$SCRIPT_DIR"
    cmake --build . -- -j"$(nproc)"
}

test_run() {
    local dsk="$BUILD_DIR/Hello.dsk"
    local appl="$BUILD_DIR/Hello.APPL"
    local bin="$BUILD_DIR/Hello.bin"
    [[ -f "$dsk" ]]  || { echo "error: $dsk missing — run '$0 build' first" >&2; exit 1; }
    [[ -f "$appl" ]] || { echo "error: $appl missing"  >&2; exit 1; }
    [[ -f "$bin" ]]  || { echo "error: $bin missing"   >&2; exit 1; }

    echo "=== Build artefacts ==="
    ls -lh "$BUILD_DIR"/Hello.* "$BUILD_DIR"/%Hello.* 2>/dev/null || true

    echo
    echo "=== Hello.dsk inspection (rdedisktool) ==="
    if [[ -x "$RDEDISKTOOL" ]]; then
        "$RDEDISKTOOL" info "$dsk"
        echo
        "$RDEDISKTOOL" list "$dsk"
    else
        echo "warning: rdedisktool not found at $RDEDISKTOOL — build it for inspection" >&2
    fi

    echo
    echo "Single-disk run (System+Hello combined):"
    echo "  $SCRIPT_DIR/run_snow_mac.sh"
}

case "${1:-build}" in
    build)  build ;;
    disk)   disk ;;
    clean)  clean ;;
    test)   test_run ;;
    all)    clean; build; disk; test_run ;;
    *)
        echo "usage: $0 {build|disk|clean|test|all}" >&2
        exit 2
        ;;
esac
