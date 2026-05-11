#!/usr/bin/env bash
# compile_phaseC.sh — kingsvalley Phase C 빌드 (Konami 64K cart)
#
# Phase A/B 와 다른 점:
#   - Konami 매퍼 (vs Phase B 의 ASCII16)
#   - main code ≤16K 강제 (data.c 를 #pragma bank 1 으로 BANK_01 분리)
#   - akm_bridge_konami.asm 사용 (Phase B 의 akm_bridge_kv.asm 의 ASCII16
#     specific 한 $7000 단일 register write 를 Konami 의 $8000+$A000 dual
#     register write 로 변형)
#   - Phase B 의 crt0 pre-init shim ($4044 의 bank-1 mount) 불필요 —
#     Konami 매퍼는 openMSX RomKonami::reset 가 deterministic 하게
#     bank 2,3 을 $8000-$BFFF 에 mount 함 (boot 시 BANK_01 자동 visible).
#
# Layout:
#   $4000-$5FFF  bank 0 (main code, 항상 mounted; Konami 매퍼 fixed)
#   $6000-$7FFF  bank 1 (main code 의 나머지; Konami initial bank 1)
#   $8000-$BFFF  bank 2+3 (BANK_01 = data.c 의 rodata; Konami initial)
#   $D500-$E2B6  AKM body (mplayer_engine_load 가 BANK_02 에서 LDIR)
#   $F000-$F0FF  PLY_AKM_ROM_Buffer (akm_ubox.asm 설정)
#   $F380↓     stack
#
# 사용자 검증 목표: Phase B 대비 멈칫 변화 (개선/동일/악화) 비교.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build_phaseC"
TARGET_BASE="kings"
AKM_BASE_HEX="D500"

# ----- toolchain -----
ZCC="${ZCC:-/opt/z88dk/bin/zcc}"
Z80ASM="${Z80ASM:-/opt/z88dk/bin/z80asm}"
RASM="${RASM:-$PROJECT_ROOT/Examples/ubox_example_z88dk/bin/rasm}"

# ----- libraries -----
UBOX_DIR="$PROJECT_ROOT/Library/MSX/ubox-msx-lib-z88dk"
SPMAN_DIR="$PROJECT_ROOT/Library/MSX/spman-z88dk"
AP_DIR="$PROJECT_ROOT/Library/MSX/ap-z88dk"
MPLAYER_DIR="$PROJECT_ROOT/Library/MSX/mplayer-z88dk"
MPLAYER_WRAPPERS="$MPLAYER_DIR/src/wrappers"

# ----- compile flags -----
ZCCFLAGS="+msx -subtype=rom -compiler=sccz80 -SO2 -create-app"
ZCCFLAGS="$ZCCFLAGS -DMSX_BUILD"
# Konami 매퍼 — main ≤16K 가 page 1 ($4000-$7FFF) 만 차지하도록 강제.
ZCCFLAGS="$ZCCFLAGS -pragma-define:MAPPER_KONAMI=1"
ZCCFLAGS="$ZCCFLAGS -I$UBOX_DIR/include -I$SPMAN_DIR/include -I$AP_DIR/include -I$MPLAYER_DIR/include"
ZCCFLAGS="$ZCCFLAGS -I$SCRIPT_DIR/game/generated -I$SCRIPT_DIR/game/src"
# CALSLT 우회 (kingsvalley a70b070 commit 의 fix 패턴 그대로 — ubox source
# 의 msxbios → msxbios_fast sed-rewrite + stub link).
# aplib: 라이브러리 link 대신 ap.asm 을 BANK_01 으로 sed-rewrite 후 직접 link
# (main 16K fit 위해; ap_* code 가 main 끝 ~140 byte 침범 회피).
ZCCFLAGS="$ZCCFLAGS -I$AP_DIR/include"

# ----- sources -----
GAME_C_SRCS=(
    game/src/main.c
    game/src/game.c
    game/src/game_util.c
    game/src/data.c
    game/src/frames_bank.c
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
SPMAN_C="$SPMAN_DIR/src/spman/spman.c"

# Phase B 와 동일하게 사용 wrapper 5개만 link.
WRAPPER_SRCS=(
    "$MPLAYER_WRAPPERS/mplayer_init.asm"
    "$MPLAYER_WRAPPERS/mplayer_init_effects.asm"
    "$MPLAYER_WRAPPERS/mplayer_play.asm"
    "$MPLAYER_WRAPPERS/mplayer_play_effect_p.asm"
    "$MPLAYER_WRAPPERS/mplayer_is_sound_effect_on.asm"
)

check_tools() {
    [[ -x "$ZCC"    ]] || { echo "ERROR: zcc not at $ZCC"; exit 1; }
    [[ -x "$Z80ASM" ]] || { echo "ERROR: z80asm not at $Z80ASM"; exit 1; }
    [[ -x "$RASM"   ]] || { echo "ERROR: rasm not at $RASM"; exit 1; }
}

ensure_libs() {
    [[ -f "$AP_DIR/lib/ap.lib" ]] || { echo "[lib] building ap-z88dk"; make -C "$AP_DIR" >/dev/null; }
    [[ -f "$SPMAN_C" ]] || { echo "ERROR: spman.c missing"; exit 1; }
}

build_akm_blob() {
    echo "[rasm] game/src/akm.z80 → build_phaseC/akm.bin (ORG=\$$AKM_BASE_HEX)"
    (cd "$SCRIPT_DIR/game/src" && "$RASM" akm.z80 \
        -ob "$BUILD_DIR/akm.bin" \
        -os "$BUILD_DIR/akm.sym" \
        -s -sl >/dev/null 2>&1) \
        || { echo "ERROR: rasm failed"; exit 1; }
    echo "  akm.bin: $(wc -c < "$BUILD_DIR/akm.bin") bytes"
}

extract_addr() {
    local label="$1"
    local raw
    raw="$(awk -v L="$label" '$1==L { print $2 }' "$BUILD_DIR/akm.sym")"
    [[ -n "$raw" ]] && echo "${raw/\#/\$}"
    return 0
}

gen_song_bindings() {
    local SONG_ADDR EFFECTS_ADDR
    SONG_ADDR="$(extract_addr SONGDISARKGENERATEEXTERNALLABEL)"
    EFFECTS_ADDR="$(extract_addr EFFECTSDISARKGENERATEEXTERNALLABEL)"
    [[ -n "$SONG_ADDR" ]] || { echo "ERROR: SONG label missing"; exit 1; }
    [[ -n "$EFFECTS_ADDR" ]] || { echo "ERROR: EFFECTS label missing"; exit 1; }

    local PLY_INIT PLY_PLAY PLY_STOP PLY_INITSFX PLY_ISSFXON PLY_PLAYSFX PLY_STOPSFX
    PLY_INIT="$(extract_addr PLY_AKM_INIT)"
    PLY_PLAY="$(extract_addr PLY_AKM_PLAY)"
    PLY_STOP="$(extract_addr PLY_AKM_STOP)"
    PLY_INITSFX="$(extract_addr PLY_AKM_INITSOUNDEFFECTS)"
    PLY_ISSFXON="$(extract_addr PLY_AKM_ISSOUNDEFFECTON)"
    PLY_PLAYSFX="$(extract_addr PLY_AKM_PLAYSOUNDEFFECT)"
    PLY_STOPSFX="$(extract_addr PLY_AKM_STOPSOUNDEFFECTFROMCHANNEL)"

    {
        echo "    SECTION code_user"
        echo "    PUBLIC _SONG"
        echo "    DEFC _SONG = $SONG_ADDR"
        echo "    PUBLIC _EFFECTS"
        echo "    DEFC _EFFECTS = $EFFECTS_ADDR"
        echo "    PUBLIC _PLY_AKM_INIT"
        echo "    DEFC _PLY_AKM_INIT = $PLY_INIT"
        echo "    PUBLIC _PLY_AKM_PLAY"
        echo "    DEFC _PLY_AKM_PLAY = $PLY_PLAY"
        echo "    PUBLIC _PLY_AKM_STOP"
        echo "    DEFC _PLY_AKM_STOP = $PLY_STOP"
        echo "    PUBLIC _PLY_AKM_INITSOUNDEFFECTS"
        echo "    DEFC _PLY_AKM_INITSOUNDEFFECTS = $PLY_INITSFX"
        echo "    PUBLIC _PLY_AKM_ISSOUNDEFFECTON"
        echo "    DEFC _PLY_AKM_ISSOUNDEFFECTON = $PLY_ISSFXON"
        echo "    PUBLIC _PLY_AKM_PLAYSOUNDEFFECT"
        echo "    DEFC _PLY_AKM_PLAYSOUNDEFFECT = $PLY_PLAYSFX"
        echo "    PUBLIC _PLY_AKM_STOPSOUNDEFFECTFROMCHANNEL"
        echo "    DEFC _PLY_AKM_STOPSOUNDEFFECTFROMCHANNEL = $PLY_STOPSFX"
    } > "$BUILD_DIR/song_bindings.asm"
    echo "  _SONG=$SONG_ADDR  PLY_AKM_INIT=$PLY_INIT  PLAY=$PLY_PLAY"
}

prepare_bridge() {
    cp -f "$SCRIPT_DIR/akm_bridge_konami.asm" "$BUILD_DIR/akm_bridge.asm"
}

prepare_ap_banked() {
    # ap.asm 의 SECTION code_user → SECTION CODE_1 으로 변경해 BANK_01 으로
    # 라우팅. 사용자 코드의 ap_uncompress 호출은 main bank 에서 cross-bank
    # call 형태가 되지만, Konami 매퍼는 boot default 로 bank 2,3 ($8000-$BFFF
    # = BANK_01) 이 mount 된 상태로 부팅하므로 swap 불필요.
    sed -e 's/^\(\s*\)SECTION code_user/\1SECTION CODE_1/' \
        "$AP_DIR/src/ap.asm" > "$BUILD_DIR/ap_banked.asm"
}

prepare_ubox_patched() {
    # CALSLT 우회 (kingsvalley a70b070 패턴). ubox source 의 msxbios → msxbios_fast.
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
    build_akm_blob
    gen_song_bindings
    prepare_bridge
    prepare_ap_banked
    prepare_ubox_patched

    local UBOX_PATCHED_SRCS=("$BUILD_DIR/ubox_patched"/*.asm)
    local MSXBIOS_FAST_SRC="$SCRIPT_DIR/variants/msxbios_no_calslt.asm"

    echo "[zcc] compiling + linking → $BUILD_DIR/$TARGET_BASE"
    (cd "$SCRIPT_DIR" && "$ZCC" $ZCCFLAGS -m -o "$BUILD_DIR/$TARGET_BASE" \
        "${GAME_C_SRCS[@]}" "$SPMAN_C" \
        "${WRAPPER_SRCS[@]}" \
        "$BUILD_DIR/akm_bridge.asm" \
        "$BUILD_DIR/song_bindings.asm" \
        "$BUILD_DIR/ap_banked.asm" \
        "${UBOX_PATCHED_SRCS[@]}" "$MSXBIOS_FAST_SRC")

    local ROM="$BUILD_DIR/$TARGET_BASE.rom"
    [[ -f "$ROM" ]] || { echo "ERROR: ROM not produced"; exit 1; }

    local SIZE
    SIZE="$(stat -c%s "$ROM")"
    echo "Built: $ROM ($SIZE bytes)"
}

clean() { rm -rf "$BUILD_DIR"; echo "Cleaned $BUILD_DIR"; }

verify() {
    local ROM="$BUILD_DIR/$TARGET_BASE.rom"
    [[ -f "$ROM" ]] || { echo "ERROR: build first"; exit 1; }
    local SIG
    SIG="$(xxd -p -l 2 "$ROM")"
    [[ "$SIG" == "4142" ]] || { echo "ERROR: bad signature $SIG"; exit 1; }
    echo "ROM ok: $(stat -c%s "$ROM") byte, signature 'AB'."
    echo ""
    echo "주요 심볼:"
    grep -E "^_main\b|^_mplayer_engine_load\b|^_PLY_AKM_PLAY\b|^_SONG\b|^_akm_blob_start\b|^__BSS_END_tail\b|^_enemy_sprite\b|^_map0\b" \
        "$BUILD_DIR/$TARGET_BASE.map" 2>/dev/null | head -10
}

case "${1:-build}" in
    build|all)  check_tools; ensure_libs; build ;;
    clean)      clean ;;
    verify)     verify ;;
    *) echo "usage: $0 {build|clean|verify}"; exit 2 ;;
esac
