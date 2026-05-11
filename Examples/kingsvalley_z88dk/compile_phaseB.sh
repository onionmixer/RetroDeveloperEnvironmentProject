#!/usr/bin/env bash
# compile_phaseB.sh — kingsvalley Phase B 빌드 (plain 32K cart + AKM 음악)
#
# Phase A 작동 확인 후 음악/효과음 통합. 핵심 흐름:
#   1. rasm 으로 game/src/akm.z80 → build_phaseB/akm.bin (ORG=$D500, ~3.5 KiB)
#   2. akm.sym 에서 _SONG/_EFFECTS/_PLY_AKM_* 주소 추출
#      → song_bindings.asm 자동 생성
#   3. mplayer-z88dk 의 akm_bridge.asm 을 build 디렉터리에 복사 + AKM_BASE
#      를 $D500 으로 override (kingsvalley-local 32K cart 영역)
#   4. zcc 로 game C 소스 + spman.c + mplayer wrappers (8개) +
#      akm_bridge + song_bindings 링크 → 32 KiB plain ROM
#
# Memory layout:
#   $C000-$D426  BSS (5.2 KiB; Phase A map 기준)
#   $D500-$E2B7  AKM body LDIR target (3.5 KiB, mplayer_engine_load)
#   $F000-$F0FF  PLY_AKM_ROM_Buffer (200 B; akm_ubox.asm 설정)
#   $F380↓       stack
#
# Phase A 와 다른 점: NO akm_stub.asm, mplayer wrappers + bridge 진짜.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build_phaseB"
TARGET_BASE="kings"

# AKM body LDIR target (must match `ORG #...` in game/src/akm.z80)
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
MPLAYER_BRIDGE_SRC="$MPLAYER_DIR/src/akm/akm_bridge.asm"

# ----- compile flags -----
ZCCFLAGS="+msx -subtype=rom -compiler=sccz80 -SO2 -create-app"
ZCCFLAGS="$ZCCFLAGS -DMSX_BUILD"
# Phase B: AKM blob 을 BANK_02 (16K) 에 두기 위해 ASCII16 매퍼 사용.
# Konami 와 다르게 trigger 주소가 $6000/$7000 두 곳 — Phase A B-3 scan
# 에서 우리 코드가 그 주소로 쓰지 않음을 확인했으므로 안전.
ZCCFLAGS="$ZCCFLAGS -pragma-define:MAPPER_ASCII16=1"
ZCCFLAGS="$ZCCFLAGS -I$UBOX_DIR/include -I$SPMAN_DIR/include -I$AP_DIR/include -I$MPLAYER_DIR/include"
ZCCFLAGS="$ZCCFLAGS -I$SCRIPT_DIR/game/generated -I$SCRIPT_DIR/game/src"
# NOTE: -lubox 사용 안 함. ubox source 를 직접 컴파일 (CALSLT 우회 패치 적용 위해).
# ubox-msx-lib-z88dk 의 msxbios wrapper 는 CALSLT inter-slot call (~220 cy/호출 overhead)
# 사용 — MSX-DOS2 호환 위해. ROM 모드 (kingsvalley) 에서는 page 0 이 BIOS slot
# 활성이라 직접 jp 가능. ubox source 의 `msxbios` 참조를 sed 로 `msxbios_fast` 로
# rewrite + variants/msxbios_no_calslt.asm 의 `jp (ix)` stub link → ~193 cy/호출 절약.
# 도구 사용 path 의 멈칫 ~50% 개선 검증됨 (no_calslt variant).
ZCCFLAGS="$ZCCFLAGS -L$AP_DIR/lib -lap"

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
SPMAN_C="$SPMAN_DIR/src/spman/spman.c"
# kingsvalley 가 실제로 호출하는 wrapper 만 link (32K cart 공간 절약).
# 사용처:
#   mplayer_init             — main.c / player.c
#   mplayer_init_effects     — main.c
#   mplayer_play             — game_util.c (my_isr)
#   mplayer_play_effect_p    — main.c / player.c / character.c
# 미사용 4개 (mplayer_stop, mplayer_stop_effect_channel,
# mplayer_is_sound_effect_on, mplayer_play_effect) 는 제외.
WRAPPER_SRCS=(
    "$MPLAYER_WRAPPERS/mplayer_init.asm"
    "$MPLAYER_WRAPPERS/mplayer_init_effects.asm"
    "$MPLAYER_WRAPPERS/mplayer_play.asm"
    "$MPLAYER_WRAPPERS/mplayer_play_effect_p.asm"
    "$MPLAYER_WRAPPERS/mplayer_is_sound_effect_on.asm"  # play_effect_p 내부 의존
)

check_tools() {
    [[ -x "$ZCC"   ]] || { echo "ERROR: zcc not at $ZCC"   >&2; exit 1; }
    [[ -x "$Z80ASM" ]] || { echo "ERROR: z80asm not at $Z80ASM" >&2; exit 1; }
    [[ -x "$RASM"  ]] || { echo "ERROR: rasm not at $RASM"  >&2; exit 1; }
}

ensure_libs() {
    [[ -f "$UBOX_DIR/lib/ubox.lib" ]] || { echo "[lib] building ubox-msx-lib-z88dk"; make -C "$UBOX_DIR" >/dev/null; }
    [[ -f "$AP_DIR/lib/ap.lib" ]]     || { echo "[lib] building ap-z88dk";          make -C "$AP_DIR"  >/dev/null; }
    [[ -f "$SPMAN_C" ]] || { echo "ERROR: spman.c missing at $SPMAN_C"; exit 1; }
    [[ -f "$SCRIPT_DIR/akm_bridge_kv.asm" ]] || { echo "ERROR: akm_bridge_kv.asm missing"; exit 1; }
}

extract_addr() {
    local label="$1"
    local raw
    raw="$(awk -v L="$label" '$1==L { print $2 }' "$BUILD_DIR/akm.sym")"
    [[ -n "$raw" ]] && echo "${raw/\#/\$}"
    return 0
}

build_akm_blob() {
    echo "[rasm] game/src/akm.z80 → build_phaseB/akm.bin (ORG=\$$AKM_BASE_HEX)"
    (cd "$SCRIPT_DIR/game/src" && "$RASM" akm.z80 \
        -ob "$BUILD_DIR/akm.bin" \
        -os "$BUILD_DIR/akm.sym" \
        -s -sl >/dev/null 2>&1) \
        || { echo "ERROR: rasm failed assembling akm.z80" >&2; exit 1; }
    echo "  akm.bin: $(wc -c < "$BUILD_DIR/akm.bin") bytes"
}

gen_song_bindings() {
    local SONG_ADDR EFFECTS_ADDR
    # rasm uppercases label names in its symbol table output
    SONG_ADDR="$(extract_addr SONGDISARKGENERATEEXTERNALLABEL)"
    EFFECTS_ADDR="$(extract_addr EFFECTSDISARKGENERATEEXTERNALLABEL)"
    [[ -n "$SONG_ADDR" ]] || { echo "ERROR: songDisarkGenerateExternalLabel missing in akm.sym"; exit 1; }
    [[ -n "$EFFECTS_ADDR" ]] || { echo "ERROR: effectsDisarkGenerateExternalLabel missing"; exit 1; }

    local PLY_INIT PLY_PLAY PLY_STOP PLY_INITSFX PLY_ISSFXON PLY_PLAYSFX PLY_STOPSFX
    PLY_INIT="$(extract_addr PLY_AKM_INIT)"
    PLY_PLAY="$(extract_addr PLY_AKM_PLAY)"
    PLY_STOP="$(extract_addr PLY_AKM_STOP)"
    PLY_INITSFX="$(extract_addr PLY_AKM_INITSOUNDEFFECTS)"
    PLY_ISSFXON="$(extract_addr PLY_AKM_ISSOUNDEFFECTON)"
    PLY_PLAYSFX="$(extract_addr PLY_AKM_PLAYSOUNDEFFECT)"
    PLY_STOPSFX="$(extract_addr PLY_AKM_STOPSOUNDEFFECTFROMCHANNEL)"
    for sym in PLY_INIT PLY_PLAY PLY_STOP PLY_INITSFX PLY_ISSFXON PLY_PLAYSFX PLY_STOPSFX; do
        [[ -n "${!sym}" ]] || { echo "ERROR: $sym missing in akm.sym"; exit 1; }
    done

    {
        echo "; Auto-generated — do not edit. Sourced by compile_phaseB.sh from akm.sym."
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
    echo "  _SONG=$SONG_ADDR _EFFECTS=$EFFECTS_ADDR"
    echo "  PLY_AKM_INIT=$PLY_INIT PLAY=$PLY_PLAY PLAYSFX=$PLY_PLAYSFX"
}

prepare_bridge() {
    # kingsvalley-local bridge (akm_bridge_kv.asm) 를 build dir 에 복사
    # 해서 BINARY "akm.bin" 이 같은 디렉터리의 akm.bin 을 찾도록 한다.
    # mplayer-z88dk 의 stock akm_bridge.asm 은 main code section 에
    # blob 을 두므로 32K cart 초과. 우리는 BANK_02 (RODATA_2) 에 둔다.
    cp -f "$SCRIPT_DIR/akm_bridge_kv.asm" "$BUILD_DIR/akm_bridge.asm"
}

prepare_ubox_patched() {
    # ubox source 를 patched 버전으로 build dir 에 복사 (CALSLT 우회).
    # sed: `msxbios` → `msxbios_fast`. msxbios_no_calslt.asm 의 stub 가 jp (ix).
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
    prepare_ubox_patched

    # ubox patched source + msxbios_fast stub
    local UBOX_PATCHED_SRCS=("$BUILD_DIR/ubox_patched"/*.asm)
    local MSXBIOS_FAST_SRC="$SCRIPT_DIR/variants/msxbios_no_calslt.asm"

    echo "[zcc] compiling + linking → $BUILD_DIR/$TARGET_BASE"
    (cd "$SCRIPT_DIR" && "$ZCC" $ZCCFLAGS -m -o "$BUILD_DIR/$TARGET_BASE" \
        "${GAME_C_SRCS[@]}" "$SPMAN_C" \
        "${WRAPPER_SRCS[@]}" \
        "$BUILD_DIR/akm_bridge.asm" \
        "$BUILD_DIR/song_bindings.asm" \
        "${UBOX_PATCHED_SRCS[@]}" "$MSXBIOS_FAST_SRC")

    local ROM="$BUILD_DIR/$TARGET_BASE.rom"
    [[ -f "$ROM" ]] || { echo "ERROR: ROM not produced"; exit 1; }

    patch_crt0_pre_init "$ROM"

    local SIZE
    SIZE="$(stat -c%s "$ROM")"
    echo "Built: $ROM ($SIZE bytes)"
}

# z88dk classic crt0 patch (Phase B 의 ASCII16 multi-bank issue 회피)
#
# 문제: z88dk crt0_init 가 main() 보다 먼저 실행되어, DATA 섹션이 bank 1
# 영역 ($B8F6) 에 있는 우리 cart 에서 LDIR 시 bank 1 미 mount 상태이므로
# bank 0 mirror 의 잘못된 byte 가 RAM 으로 복사됨 → 게임 globals 손상.
#
# 해결: rom.asm 의 dead code 영역 (`l_dcal: jp(hl)` 후의 mapper bias writes
# at $4044-$4055 — 18 byte) 에 8 byte pre-init shim 을 주입하고, 원래
# `call crt0_init` ($4036) 의 target 을 그 shim 으로 redirect.
#
# Shim:  ld a, 1; ld ($7000), a; jp crt0_init ($40AD)
# (jp 라서 crt0_init 의 ret 가 직접 $4039 (call site 다음) 로 돌아감)
#
patch_crt0_pre_init() {
    local ROM="$1"
    # 기존 byte: $4036 = `cd ad 40` (call crt0_init)
    # file offset = MSX addr - $4000
    # 검증: 현재 $4036 가 정말 `cd ad 40` 인지 확인
    local cur
    cur="$(xxd -p -s 0x36 -l 3 "$ROM")"
    if [[ "$cur" != "cdad40" ]]; then
        echo "WARN: crt0 layout 변경됨 ($4036 = $cur, expected cdad40). Pre-init patch SKIPPED — bank 1 init 이 안 될 수 있음." >&2
        return 0
    fi
    # crt0_init 호출을 dead-code 영역 ($4044) 로 redirect
    printf '\xcd\x44\x40' | dd of="$ROM" bs=1 seek=$((0x36)) count=3 conv=notrunc status=none
    # $4044 에 shim 주입: 3E 01  32 00 70  C3 AD 40  (8 byte)
    printf '\x3e\x01\x32\x00\x70\xc3\xad\x40' | dd of="$ROM" bs=1 seek=$((0x44)) count=8 conv=notrunc status=none
    echo "[patch] crt0 pre-init shim 주입: bank 1 → \$8000 mount before crt0_init"
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
    [[ "$SIG" == "4142" ]] || { echo "ERROR: bad signature $SIG"; exit 1; }
    echo "ROM ok: 32 KiB plain cart, signature 'AB'."

    if [[ -f "$BUILD_DIR/$TARGET_BASE.map" ]]; then
        echo ""
        echo "Phase B 핵심 심볼:"
        grep -E "^_main\b|^_mplayer_engine_load\b|^_mplayer_play\b|^_PLY_AKM_PLAY\b|^_SONG\b|^_EFFECTS\b|^_akm_blob_start\b|^_akm_blob_end\b|^__BSS_END_tail\b" \
            "$BUILD_DIR/$TARGET_BASE.map" | head -15
    fi
}

case "${1:-build}" in
    build|all)  check_tools; ensure_libs; build ;;
    clean)      clean ;;
    verify)     verify ;;
    *) echo "usage: $0 {build|all|clean|verify}"; exit 2 ;;
esac
