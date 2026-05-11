#!/usr/bin/env bash
# compile_phaseB_variant.sh — Phase B build with one diagnostic patch
# applied. Used to ground-truth the spacebar/tool-use glitch hypothesis.
#
# usage:
#     ./compile_phaseB_variant.sh baseline       # = compile_phaseB.sh
#     ./compile_phaseB_variant.sh no_altreg      # ubox_isr drops alt-set save
#     ./compile_phaseB_variant.sh diei_efx       # mplayer_play_effect_p +di/ei
#
# Each variant lands in build_phaseB_<variant>/kings.rom. run_phaseB.sh
# can boot any of them by setting ROM=path explicitly.

set -euo pipefail

VARIANT="${1:-baseline}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build_phaseB_${VARIANT}"
TARGET_BASE="kings"
AKM_BASE_HEX="D500"

ZCC="${ZCC:-/opt/z88dk/bin/zcc}"
Z80ASM="${Z80ASM:-/opt/z88dk/bin/z80asm}"
RASM="${RASM:-$PROJECT_ROOT/Examples/ubox_example_z88dk/bin/rasm}"

UBOX_DIR="$PROJECT_ROOT/Library/MSX/ubox-msx-lib-z88dk"
SPMAN_DIR="$PROJECT_ROOT/Library/MSX/spman-z88dk"
AP_DIR="$PROJECT_ROOT/Library/MSX/ap-z88dk"
MPLAYER_DIR="$PROJECT_ROOT/Library/MSX/mplayer-z88dk"
MPLAYER_WRAPPERS="$MPLAYER_DIR/src/wrappers"
MPLAYER_BRIDGE_SRC="$MPLAYER_DIR/src/akm/akm_bridge.asm"

ZCCFLAGS="+msx -subtype=rom -compiler=sccz80 -SO2 -create-app"
ZCCFLAGS="$ZCCFLAGS -DMSX_BUILD"
ZCCFLAGS="$ZCCFLAGS -pragma-define:MAPPER_ASCII16=1"
ZCCFLAGS="$ZCCFLAGS -I$UBOX_DIR/include -I$SPMAN_DIR/include -I$AP_DIR/include -I$MPLAYER_DIR/include"
ZCCFLAGS="$ZCCFLAGS -I$SCRIPT_DIR/game/generated -I$SCRIPT_DIR/game/src"
ZCCFLAGS="$ZCCFLAGS -L$AP_DIR/lib -lap"
# NOTE: -lubox는 variant 에 따라 추가 결정 (no_altreg 면 user override 사용)

GAME_C_SRCS=(
    game/src/main.c game/src/game.c game/src/game_util.c game/src/data.c
    game/src/character.c game/src/character_move.c game/src/enemy.c
    game/src/gate.c game/src/item.c game/src/jewel.c game/src/knife.c
    game/src/player.c game/src/pushdoor.c game/src/trap.c
)
SPMAN_C="$SPMAN_DIR/src/spman/spman.c"

# Wrapper variant resolution: 기본은 mplayer-z88dk lib 의 wrapper.
PLAY_EFX_P_WRAPPER="$MPLAYER_WRAPPERS/mplayer_play_effect_p.asm"

# Variant 별 override
case "$VARIANT" in
    baseline)
        UBOX_LINK_ARGS="-L$UBOX_DIR/lib -lubox"
        EXTRA_USER_OBJS=()
        ;;
    no_altreg)
        UBOX_LINK_ARGS="-L$UBOX_DIR/lib -lubox"
        ;;
    diei_efx)
        UBOX_LINK_ARGS="-L$UBOX_DIR/lib -lubox"
        PLAY_EFX_P_WRAPPER="$SCRIPT_DIR/variants/mplayer_play_effect_p_diei.asm"
        EXTRA_USER_OBJS=()
        ;;
    no_vdp_diei)
        # ubox_vdp_direct 의 di/ei 제거 — sprite/VRAM transfer 시 ISR 차단 회피
        UBOX_LINK_ARGS="-L$UBOX_DIR/lib -lubox"
        EXTRA_USER_OBJS=()
        ;;
    no_calslt)
        # msxbios 자체는 z88dk msx_crt0 의 일부라 override 불가 (duplicate
        # PUBLIC 에러). 대신 ubox/spman/mplayer 함수들의 EXTERN msxbios /
        # jp msxbios / call msxbios 를 모두 msxbios_fast 로 sed-rewrite
        # 한 patched source 를 build dir 에 생성하고, ubox.lib 빼고 그
        # patched source 직접 link. 우리 user stub 이 msxbios_fast 제공.
        UBOX_LINK_ARGS=""
        EXTRA_USER_OBJS=("$SCRIPT_DIR/variants/msxbios_no_calslt.asm")
        ;;
    *)
        echo "ERROR: unknown variant '$VARIANT'. valid: baseline | no_altreg | diei_efx | no_vdp_diei | no_calslt" >&2
        exit 2
        ;;
esac

ZCCFLAGS="$ZCCFLAGS $UBOX_LINK_ARGS"

WRAPPER_SRCS=(
    "$MPLAYER_WRAPPERS/mplayer_init.asm"
    "$MPLAYER_WRAPPERS/mplayer_init_effects.asm"
    "$MPLAYER_WRAPPERS/mplayer_play.asm"
    "$PLAY_EFX_P_WRAPPER"
    "$MPLAYER_WRAPPERS/mplayer_is_sound_effect_on.asm"
)

[[ -x "$ZCC"   ]] || { echo "ERROR: zcc not at $ZCC"; exit 1; }
[[ -x "$Z80ASM" ]] || { echo "ERROR: z80asm not at $Z80ASM"; exit 1; }
[[ -x "$RASM"  ]] || { echo "ERROR: rasm not at $RASM"; exit 1; }
[[ -f "$UBOX_DIR/lib/ubox.lib" ]] || make -C "$UBOX_DIR" >/dev/null
[[ -f "$AP_DIR/lib/ap.lib" ]]     || make -C "$AP_DIR"   >/dev/null

mkdir -p "$BUILD_DIR"

echo "[variant] $VARIANT → $BUILD_DIR"

# Variant override 용 ubox source 합본
UBOX_SRC_DIR="$UBOX_DIR/src/ubox"
UBOX_SRCS=()
case "$VARIANT" in
    no_altreg)
        UBOX_SRCS+=("$SCRIPT_DIR/variants/ubox_isr_no_altreg.asm")
        for f in "$UBOX_SRC_DIR"/*.asm; do
            [[ "$(basename "$f")" == "ubox_isr.asm" ]] && continue
            UBOX_SRCS+=("$f")
        done
        ;;
    no_vdp_diei)
        UBOX_SRCS+=("$SCRIPT_DIR/variants/ubox_vdp_direct_no_diei.asm")
        for f in "$UBOX_SRC_DIR"/*.asm; do
            [[ "$(basename "$f")" == "ubox_vdp_direct.asm" ]] && continue
            UBOX_SRCS+=("$f")
        done
        ;;
    no_calslt)
        # 모든 ubox source 를 patched 버전으로 link.
        # sed: msxbios → msxbios_fast
        mkdir -p "$BUILD_DIR/ubox_patched"
        for f in "$UBOX_SRC_DIR"/*.asm; do
            sed -e 's/\bjp msxbios\b/jp msxbios_fast/g' \
                -e 's/\bcall msxbios\b/call msxbios_fast/g' \
                -e 's/\bEXTERN msxbios\b/EXTERN msxbios_fast/g' \
                "$f" > "$BUILD_DIR/ubox_patched/$(basename "$f")"
            UBOX_SRCS+=("$BUILD_DIR/ubox_patched/$(basename "$f")")
        done
        ;;
esac
if [[ ${#UBOX_SRCS[@]} -gt 0 ]]; then
    # ubox.lib 비활성화하고 source 로 직접 link
    ZCCFLAGS="${ZCCFLAGS// -lubox/}"
    ZCCFLAGS="${ZCCFLAGS//-L$UBOX_DIR\/lib/}"
fi

# Build AKM blob
echo "[rasm] game/src/akm.z80 → $BUILD_DIR/akm.bin"
(cd "$SCRIPT_DIR/game/src" && "$RASM" akm.z80 \
    -ob "$BUILD_DIR/akm.bin" -os "$BUILD_DIR/akm.sym" -s -sl >/dev/null 2>&1)

extract_addr() {
    local label="$1"
    local raw
    raw="$(awk -v L="$label" '$1==L { print $2 }' "$BUILD_DIR/akm.sym")"
    [[ -n "$raw" ]] && echo "${raw/\#/\$}"
    return 0
}

# song bindings
SONG_ADDR="$(extract_addr SONGDISARKGENERATEEXTERNALLABEL)"
EFFECTS_ADDR="$(extract_addr EFFECTSDISARKGENERATEEXTERNALLABEL)"
PLY_INIT="$(extract_addr PLY_AKM_INIT)"
PLY_PLAY="$(extract_addr PLY_AKM_PLAY)"
PLY_STOP="$(extract_addr PLY_AKM_STOP)"
PLY_INITSFX="$(extract_addr PLY_AKM_INITSOUNDEFFECTS)"
PLY_ISSFXON="$(extract_addr PLY_AKM_ISSOUNDEFFECTON)"
PLY_PLAYSFX="$(extract_addr PLY_AKM_PLAYSOUNDEFFECT)"
PLY_STOPSFX="$(extract_addr PLY_AKM_STOPSOUNDEFFECTFROMCHANNEL)"

cat > "$BUILD_DIR/song_bindings.asm" <<EOF
    SECTION code_user
    PUBLIC _SONG
    DEFC _SONG = $SONG_ADDR
    PUBLIC _EFFECTS
    DEFC _EFFECTS = $EFFECTS_ADDR
    PUBLIC _PLY_AKM_INIT
    DEFC _PLY_AKM_INIT = $PLY_INIT
    PUBLIC _PLY_AKM_PLAY
    DEFC _PLY_AKM_PLAY = $PLY_PLAY
    PUBLIC _PLY_AKM_STOP
    DEFC _PLY_AKM_STOP = $PLY_STOP
    PUBLIC _PLY_AKM_INITSOUNDEFFECTS
    DEFC _PLY_AKM_INITSOUNDEFFECTS = $PLY_INITSFX
    PUBLIC _PLY_AKM_ISSOUNDEFFECTON
    DEFC _PLY_AKM_ISSOUNDEFFECTON = $PLY_ISSFXON
    PUBLIC _PLY_AKM_PLAYSOUNDEFFECT
    DEFC _PLY_AKM_PLAYSOUNDEFFECT = $PLY_PLAYSFX
    PUBLIC _PLY_AKM_STOPSOUNDEFFECTFROMCHANNEL
    DEFC _PLY_AKM_STOPSOUNDEFFECTFROMCHANNEL = $PLY_STOPSFX
EOF

cp -f "$SCRIPT_DIR/akm_bridge_kv.asm" "$BUILD_DIR/akm_bridge.asm"

echo "[zcc] linking variant=$VARIANT"
EXTRA_SRCS=()
if [[ ${#UBOX_SRCS[@]} -gt 0 ]]; then
    EXTRA_SRCS=("${UBOX_SRCS[@]}")
fi
# user override .asm (e.g., msxbios_no_calslt)
if [[ ${#EXTRA_USER_OBJS[@]} -gt 0 ]]; then
    EXTRA_SRCS+=("${EXTRA_USER_OBJS[@]}")
fi

(cd "$SCRIPT_DIR" && "$ZCC" $ZCCFLAGS -m -o "$BUILD_DIR/$TARGET_BASE" \
    "${GAME_C_SRCS[@]}" "$SPMAN_C" \
    "${WRAPPER_SRCS[@]}" \
    "$BUILD_DIR/akm_bridge.asm" \
    "$BUILD_DIR/song_bindings.asm" \
    "${EXTRA_SRCS[@]}")

ROM="$BUILD_DIR/$TARGET_BASE.rom"
[[ -f "$ROM" ]] || { echo "ERROR: ROM not produced"; exit 1; }

# crt0 pre-init shim 패치 (Phase B 와 동일)
cur="$(xxd -p -s 0x36 -l 3 "$ROM")"
if [[ "$cur" == "cdad40" ]]; then
    printf '\xcd\x44\x40' | dd of="$ROM" bs=1 seek=$((0x36)) count=3 conv=notrunc status=none
    printf '\x3e\x01\x32\x00\x70\xc3\xad\x40' | dd of="$ROM" bs=1 seek=$((0x44)) count=8 conv=notrunc status=none
    echo "[patch] crt0 pre-init shim"
else
    echo "WARN: crt0 layout shifted ($cur), shim skipped — variant build may not boot"
fi

SIZE="$(stat -c%s "$ROM")"
echo "Built: $ROM ($SIZE bytes)"
