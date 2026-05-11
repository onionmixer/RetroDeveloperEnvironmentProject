#!/usr/bin/env bash
# compile_phaseA.sh — kingsvalley Phase A 빌드 (plain 32K cart, NO Konami)
#
# 목표: 음악 없는 minimal viable boot. 타이틀 화면이 GT 에서 표시되면 성공.
# Konami pragma 미사용 → openMSX 자동 plain mapper 매핑 (PLAN §7 Iter 33).
# AKM/mplayer 심볼은 akm_stub.asm 가 모두 no-op 으로 흡수.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build_phaseA"
TARGET_BASE="kings"

# ----- toolchain -----
if [[ -x "/opt/z88dk/bin/zcc" ]]; then
    ZCC="/opt/z88dk/bin/zcc"
else
    ZCC="$(command -v zcc 2>/dev/null || true)"
fi
Z80ASM="${Z80ASM:-/opt/z88dk/bin/z80asm}"

# ----- libraries (z88dk 포트) -----
UBOX_DIR="$PROJECT_ROOT/Library/MSX/ubox-msx-lib-z88dk"
SPMAN_DIR="$PROJECT_ROOT/Library/MSX/spman-z88dk"
AP_DIR="$PROJECT_ROOT/Library/MSX/ap-z88dk"
# mplayer 라이브러리는 link 하지 않지만 header 는 필요 (game source 가 #include <mplayer.h>)
MPLAYER_DIR="$PROJECT_ROOT/Library/MSX/mplayer-z88dk"

# ----- compile flags -----
# NOTE: NO `-pragma-define:MAPPER_KONAMI=1` → plain 32K cart (PLAN §0 Iter 4, §6 Iter 26)
ZCCFLAGS="+msx -subtype=rom -compiler=sccz80 -SO2 -create-app"
ZCCFLAGS="$ZCCFLAGS -DMSX_BUILD"
# IMPORTANT: z88dk 포트 헤더가 kingsvalley/include/ 의 SDCC 헤더보다 먼저 와야 함.
# kingsvalley/include/ubox.h 는 `__asm halt __endasm` macro 가 있어 sccz80 가 못
# 처리한다. -I$SCRIPT_DIR/include 는 의도적으로 제외.
ZCCFLAGS="$ZCCFLAGS -I$UBOX_DIR/include -I$SPMAN_DIR/include -I$AP_DIR/include -I$MPLAYER_DIR/include"
ZCCFLAGS="$ZCCFLAGS -I$SCRIPT_DIR/game/generated -I$SCRIPT_DIR/game/src"
# NOTE: -lubox 사용 안 함. ubox source 를 직접 컴파일 (CALSLT 우회 패치 적용 위해).
# ubox-msx-lib-z88dk 의 msxbios wrapper 는 CALSLT inter-slot call (~220 cy/호출 overhead)
# 사용 — MSX-DOS2 호환 위해. ROM 모드 (kingsvalley 등) 에서는 page 0 이 BIOS slot
# 활성이라 직접 jp 가능. ubox source 의 `msxbios` 참조를 sed 로 `msxbios_fast` 로
# rewrite + variants/msxbios_no_calslt.asm 의 `jp (ix)` stub link → ~193 cy/호출 절약.
# 도구 사용 path 의 멈칫 ~50% 개선 검증됨 (no_calslt variant).
ZCCFLAGS="$ZCCFLAGS -L$AP_DIR/lib -lap"
# spman 은 source distribution — spman.c 를 직접 컴파일 (Library/MSX/spman-z88dk/Makefile 주석 참고)

# ----- sources -----
GAME_C_SRCS=(
    game/src/main.c
    game/src/game.c
    game/src/game_util.c
    game/src/data.c
    game/src/character.c
    game/src/character_move.c
    game/src/enemy.c
    game/src/gate.c
    game/src/item.c
    game/src/jewel.c
    game/src/knife.c
    game/src/player.c
    game/src/pushdoor.c
    game/src/trap.c
)

# spman 은 source 직접 컴파일 (라이브러리 빌드 안 함)
SPMAN_C="$SPMAN_DIR/src/spman/spman.c"

# Phase A: AKM/mplayer 모두 stub
ASM_SRCS=(akm_stub.asm)

# ----- helpers -----
check_tools() {
    [[ -n "$ZCC" && -x "$ZCC" ]] || { echo "ERROR: zcc not found"; exit 1; }
    [[ -x "$Z80ASM" ]] || { echo "ERROR: z80asm not found"; exit 1; }
}

ensure_libs() {
    if [[ ! -f "$UBOX_DIR/lib/ubox.lib" ]]; then
        echo "[lib] building ubox-msx-lib-z88dk"
        make -C "$UBOX_DIR" >/dev/null
    fi
    [[ -f "$SPMAN_C" ]] || { echo "ERROR: spman.c not at $SPMAN_C"; exit 1; }
    if [[ ! -f "$AP_DIR/lib/ap.lib" ]]; then
        echo "[lib] building ap-z88dk"
        make -C "$AP_DIR" >/dev/null
    fi
}

prepare_ubox_patched() {
    # ubox source 를 patched 버전으로 build dir 에 복사 (CALSLT 우회).
    # sed: `msxbios` → `msxbios_fast` (EXTERN / jp / call 의 3가지 형태).
    mkdir -p "$BUILD_DIR/ubox_patched"
    local f
    for f in "$UBOX_DIR/src/ubox"/*.asm; do
        sed -e 's/\bjp msxbios\b/jp msxbios_fast/g' \
            -e 's/\bcall msxbios\b/call msxbios_fast/g' \
            -e 's/\bEXTERN msxbios\b/EXTERN msxbios_fast/g' \
            "$f" > "$BUILD_DIR/ubox_patched/$(basename "$f")"
    done
}

build() {
    mkdir -p "$BUILD_DIR"

    prepare_ubox_patched

    # Assemble Phase A asm sources
    for asm in "${ASM_SRCS[@]}"; do
        echo "[asm] $asm"
        "$Z80ASM" -O"$BUILD_DIR" -o"${asm%.asm}.o" "$SCRIPT_DIR/$asm"
    done

    local OBJS=()
    for asm in "${ASM_SRCS[@]}"; do
        OBJS+=("$BUILD_DIR/${asm%.asm}.o")
    done

    # ubox patched source + msxbios_fast stub
    local UBOX_PATCHED_SRCS=("$BUILD_DIR/ubox_patched"/*.asm)
    local MSXBIOS_FAST_SRC="$SCRIPT_DIR/variants/msxbios_no_calslt.asm"

    echo "[zcc] compiling + linking → $BUILD_DIR/$TARGET_BASE"
    (cd "$SCRIPT_DIR" && "$ZCC" $ZCCFLAGS -m -o "$BUILD_DIR/$TARGET_BASE" \
        "${GAME_C_SRCS[@]}" "$SPMAN_C" "${OBJS[@]}" \
        "${UBOX_PATCHED_SRCS[@]}" "$MSXBIOS_FAST_SRC")

    local ROM="$BUILD_DIR/$TARGET_BASE.rom"
    if [[ ! -f "$ROM" ]]; then
        echo "ERROR: ROM not produced"
        exit 1
    fi
    local SIZE
    SIZE="$(stat -c%s "$ROM")"
    echo "Built: $ROM ($SIZE bytes)"
    [[ "$SIZE" == "32768" ]] || echo "WARN: expected 32768 bytes, got $SIZE"
}

clean() {
    rm -rf "$BUILD_DIR"
    echo "Cleaned $BUILD_DIR"
}

verify() {
    local ROM="$BUILD_DIR/$TARGET_BASE.rom"
    [[ -f "$ROM" ]] || { echo "ERROR: build first"; exit 1; }

    local SIG
    SIG="$(xxd -p -l 2 "$ROM")"
    [[ "$SIG" == "4142" ]] || { echo "ERROR: bad signature $SIG (expected 4142 'AB')"; exit 1; }

    echo "ROM ok: 32 KiB, signature 'AB'."
    echo "Entry vector:"
    xxd -s 2 -l 2 "$ROM"
}

case "${1:-build}" in
    build|all)  check_tools; ensure_libs; build ;;
    clean)      clean ;;
    verify)     verify ;;
    *) echo "usage: $0 {build|all|clean|verify}"; exit 2 ;;
esac
