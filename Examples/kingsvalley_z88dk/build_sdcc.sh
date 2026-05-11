#!/usr/bin/env bash
# build_sdcc.sh — SDCC 로 kingsvalley 빌드 (mplayer/AKM 은 stub).
#
# 목적: z88dk Phase A 빌드와 byte-level 비교용 ground-truth SDCC ROM 생성.
# SDCC + mplayer/AKM full link 는 Disark output 의 라벨 underscore prefix
# 미지원으로 막혀 stub 으로 우회. 게임 logic 동일하므로 도구 사용 시 멈칫
# 비교에는 충분.
#
# 산출물: game/build/kings_sdcc.rom

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DISARK_BIN="${DISARK_BIN:-/tmp/disarkbin/Disark}"
PYALIAS_DIR="${PYALIAS_DIR:-/tmp/pyalias}"

cd "$SCRIPT_DIR"

# 이전 실행에서 trap 실패로 남은 .bak 가 있으면 먼저 복원
if [[ -f game/src/akm.z80.bak ]]; then
    echo "[warn] 이전 실행의 .bak 발견 → 우선 복원"
    mv game/src/akm.z80.bak game/src/akm.z80
fi

# Step 1: ORG #D500 임시 제거 (SDCC 빌드는 ORG 안 필요).
# 강건한 trap: EXIT 시 + 시그널(INT/TERM) 시 모두 복원, .bak 존재 검사.
if grep -q "^ORG #D500" game/src/akm.z80; then
    echo "[step 1] akm.z80 의 ORG #D500 임시 제거"
    cp game/src/akm.z80 game/src/akm.z80.bak
    sed -i '/^ORG #D500/d' game/src/akm.z80
    restore_akm() {
        if [[ -f game/src/akm.z80.bak ]]; then
            mv -f game/src/akm.z80.bak game/src/akm.z80
            echo "[restore] akm.z80 의 ORG 라인 복원"
        fi
    }
    trap restore_akm EXIT INT TERM
fi

# Step 2: libs (이미 빌드되어 있으면 skip)
echo "[step 2] libs"
PATH="$PYALIAS_DIR:$PATH"
[[ -f lib/ubox.lib ]] || make libs
( cd game/data && PATH="$PYALIAS_DIR:$PATH" make ) >/dev/null

# Step 3: game/src .c → .rel
echo "[step 3] sdcc 로 .c → .rel"
mkdir -p game/build
( cd game/src
    CFLAGS="-mz80 --Werror -I../../include -I../generated --fsigned-char --std-sdcc99 --opt-code-speed -I."
    for cfile in *.c; do
        rel="../build/${cfile%.c}.rel"
        if [[ ! -f "$rel" || "$cfile" -nt "$rel" ]]; then
            sdcc $CFLAGS -c "$cfile" -o "$rel"
        fi
    done
)

# Step 4: crt0
echo "[step 4] crt0.rel"
( cd game/src && sdasz80 -g -o ../build/crt0.rel crt0.z80 )

# Step 5: AKM/mplayer stub — _SONG, _EFFECTS, _PLY_AKM_* 모두 no-op
echo "[step 5] akm_stub.rel (mplayer 무력화)"
cat > game/build/akm_stub.asm <<'EOF'
;; SDCC mplayer/AKM stub — no-op all symbols so game links without Disark
        .module akm_stub

        .area _DATA
        .globl _SONG
        .globl _EFFECTS
_SONG::
        .ds 5
_EFFECTS::
        .ds 5

        .area _CODE
        .globl _PLY_AKM_INIT
        .globl _PLY_AKM_PLAY
        .globl _PLY_AKM_STOP
        .globl _PLY_AKM_INITSOUNDEFFECTS
        .globl _PLY_AKM_ISSOUNDEFFECTON
        .globl _PLY_AKM_PLAYSOUNDEFFECT
        .globl _PLY_AKM_STOPSOUNDEFFECTFROMCHANNEL
_PLY_AKM_INIT::
_PLY_AKM_PLAY::
_PLY_AKM_STOP::
_PLY_AKM_INITSOUNDEFFECTS::
_PLY_AKM_ISSOUNDEFFECTON::
_PLY_AKM_PLAYSOUNDEFFECT::
_PLY_AKM_STOPSOUNDEFFECTFROMCHANNEL::
        ld l, #0
        ret
EOF
sdasz80 -g -o game/build/akm_stub.rel game/build/akm_stub.asm

# Step 6: link
echo "[step 6] sdcc link → kings.ihx"
( cd game/src && sdcc -mz80 --Werror -I../../include -I../generated --fsigned-char --std-sdcc99 \
    --opt-code-speed -I. -L../../lib -L. --no-std-crt0 --fomit-frame-pointer \
    -lubox -lspman -lmplayer -lap \
    --code-loc 0x4000 --data-loc 0xc0de \
    ../build/crt0.rel ../build/character.rel ../build/character_move.rel ../build/data.rel \
    ../build/enemy.rel ../build/game.rel ../build/game_util.rel ../build/gate.rel \
    ../build/item.rel ../build/jewel.rel ../build/knife.rel ../build/main.rel \
    ../build/player.rel ../build/pushdoor.rel ../build/trap.rel ../build/akm_stub.rel \
    -o ../build/kings.ihx ) 2>&1 | grep -E "Warning|Error|error" | head -10 || true

# Step 7: hex2bin
echo "[step 7] hex2bin → kings_sdcc.rom"
"$SCRIPT_DIR/bin/hex2bin" -e bin -p 00 -l 8000 game/build/kings.ihx >/dev/null 2>&1
cp game/build/kings.bin game/build/kings_sdcc.rom

SIZE="$(stat -c%s game/build/kings_sdcc.rom)"
echo ""
echo "SDCC build: game/build/kings_sdcc.rom ($SIZE bytes)"
[[ "$SIZE" == "32768" ]] && echo "✓ 32 KiB plain cart"
