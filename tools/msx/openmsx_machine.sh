# openmsx_machine.sh — openMSX 시험 기종 준비 (루트 run_openmsx_rom.sh · run_openmsx_msxdos2.sh 공용)
#
# ★DKFS_retro/prototype_20 의 `tools/openmsx_machine.sh`(`p20_machine_prep`)와 **같은 절차**다 — 그쪽이 실기 시험에 쓴
#   기종(Panasonic FS-A1F · 매퍼 256 KB · VRAM 64 KB · A1 Cockpit 제거 사본 `…_256K_V64_NOCKPT`)을 루트 스크립트도 쓰게 한다.
#   (서브모듈 파일을 루트에서 source 하지 않으려고 사본을 둔다 — 절차를 바꾸면 두 곳을 함께 고칠 것.)
#
# 사용(호출자가 source 한 뒤):
#   MACHINE=… OPENMSX_SHARE=… MAPPER_KB=… VRAM_KB=…   (호출자가 미리 정한다 · set -u 안전)
#   openmsx_machine_prep        → MACHINE 을 사본 이름으로 바꾸고 HAS_S1990(0/1) 을 세운다 · 실패면 exit 1
#
# 무엇을 하나
#   · turbo R 판정 = 원본 XML 에 `<S1990` 장치가 있는가 — openMSX 자신의 판정과 같다(`MSXMotherBoard::isTurboR`).
#     turbo R 은 R800 을 갖고 DOS2 가 내장이다 · 아니면(FS-A1F 등) R800 이 없다(Z80) · DOS2 는 `-ext msxdos2` 카트리지가 필요하다.
#   · 사본(`~/.openMSX/share/machines/<이름>.xml`): `<size>`(매퍼 KB)·`<vram>` 치환 + A1 Cockpit ROM 제거 —
#     각각 **정확히 1 곳**이 아니면 실패(fail-closed).
#   · ★A1 Cockpit: FS-A1F 는 전원 투입 시 내장 소프트웨어 메뉴가 디스크보다 먼저 뜬다 — openMSX 에 끄는 스위치가 없어 그 ROM(3-3)만 뺀다.
openmsx_machine_prep() {
    local src="$OPENMSX_SHARE/machines/${MACHINE}.xml"
    if [[ ! -f "$src" ]]; then
        echo "Error: machine XML not found: $src" >&2; exit 1
    fi
    local info
    info=$(python3 - "$src" <<'PY'
import sys
s = open(sys.argv[1]).read()
print('%d %d' % ('<S1990' in s, s.count('<ROM id="A1 Cockpit">')))
PY
) || { echo "Error: machine XML 판독 실패: $src" >&2; exit 1; }
    HAS_S1990="${info% *}"
    local cockpit="${info#* }"
    if [[ -z "$MAPPER_KB" && -z "$VRAM_KB" && "$cockpit" == 0 ]]; then
        return 0                                   # 원본 그대로
    fi
    local user_mach="$HOME/.openMSX/share/machines"
    mkdir -p "$user_mach"
    local custom="${MACHINE}_${MAPPER_KB:-orig}K_V${VRAM_KB:-orig}"
    [[ "$cockpit" != 0 ]] && custom="${custom}_NOCKPT"
    python3 - "$src" "$user_mach/${custom}.xml" "$MAPPER_KB" "$VRAM_KB" <<'PY' || { echo "Error: 시험 기종 사본 생성 실패(fail-closed)" >&2; exit 1; }
import re, sys
src, dst, mapper, vram = sys.argv[1:5]
s = open(src).read()
def one(pat, rep, what, flags=0):
    global s
    s, n = re.subn(pat, rep, s, flags=flags)
    if n != 1:
        sys.exit('%s 가 %d 곳 — 기종 정의가 바뀌었다(정확히 1 곳이어야)' % (what, n))
if mapper:
    one(r'<size>[0-9]+</size>', '<size>%s</size>' % mapper, '<size>(매퍼)')
if vram:
    one(r'<vram>[0-9]+</vram>', '<vram>%s</vram>' % vram, '<vram>')
if '<ROM id="A1 Cockpit">' in s:
    one(r'\s*<secondary slot="\d">\s*<ROM id="A1 Cockpit">.*?</ROM>\s*</secondary>', '', 'A1 Cockpit 블록', re.S)
open(dst, 'w').write(s)
PY
    MACHINE="$custom"
}
