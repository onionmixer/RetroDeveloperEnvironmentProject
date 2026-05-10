# PLAN — King's Valley (kingsvalley) z88dk 포팅 (재검토 v2)

**상태**: **Phase A 검증 완료 ✓** (2026-05-10, 사용자 GT 검증 — 타이틀 + 게임 진행 OK, 음악 없음). Phase B (음악) 대기.
**작성**: 2026-05-10 (1차 PLAN 무효화, 새로운 사실 기반 v2)

## 1차 시도 (실패) 의 핵심 오류

이전 PLAN은 **prototype_05 패턴 (Konami 매퍼) 을 그대로 따라야 한다고 가정** 했으나, 실제 원본 kingsvalley ROM 분석 결과:

- **원본 `kings_original.rom` (GitHub release pdpdds/ubox_example v1.0)**: **32 KiB plain cart, 매퍼 사용 안 함** (`xxd -p kings_original.rom | grep -bo "320060\|320080\|3200a0"` → 0건)
- **prototype_05 ROM**: `32 00 60` Konami 매퍼 bias write 존재 (z88dk crt0 가 emit)
- **결론**: 두 reference 가 다른 cart 타입. 우리는 잘못된 reference 채택.

1차 시도 시 적용한 `MAPPER_KONAMI=1` pragma 가 **불필요한 bank-switching 인프라 + 9개 bias writes (27 byte)** 를 cart 에 추가. 그것이 GT 에서 hang 유발 가능성 농후 (uses 가 있는 instruction 패턴이 mapper hardware 에 의해 "register write" 로 해석되어 의도치 않은 bank 전환).

---

## 20-iter 재검토 결과

### Iter 1 — 사실 정립

| | 확인됨 |
|---|---|
| 우리 라이브러리 (z88dk port) | `Library/MSX/{ubox-msx-lib-z88dk, spman-z88dk, mplayer-z88dk, ap-z88dk}/` 빌드 OK |
| z88dk 12 예제 (ubox + spman + mplayer) | 사용자 GT 검증됨 |
| prototype_05 (Konami 매퍼 32K) | GT 작동 ✓ |
| kingsvalley clean state | git restore 로 reset 완료, 원본 코드 그대로 |
| SDCC + sdasz80 + sdar + hex2bin | `/usr/bin/` 에 모두 설치됨 |

### Iter 2 — Reference ROM 확보

GitHub `pdpdds/ubox_example` v1.0 release 에서 다운로드:
- `kings.rom` (32,768 B)
- `kings_ver2.rom` (32,768 B)
- `kings_original.rom` (32,768 B) — **검증의 절대 기준**

모두 **"AB" header + entry $4010 + 32 KiB plain**. `/tmp/kv_refs/` 에 저장.

### Iter 3 — 매퍼 분석: 원본은 plain 32K

| ROM | 32 KiB | Konami bias write (`ld ($6000)/(\$8000)/($A000)`) |
|---|---|---|
| `kings_original.rom` | ✓ | **0건** — plain cart |
| `kings.rom` (v1.0) | ✓ | 0건 |
| `kings_ver2.rom` | ✓ | 0건 |
| `prototype_05/PROTO05.rom` | ✓ | **존재** (z88dk crt0 의 Konami bias) |

**결론**: 원본 kingsvalley 는 plain 32K cart. **z88dk 빌드 시 `MAPPER_KONAMI=1` 사용하면 안 됨**.

### Iter 4 — z88dk plain 32K cart 가능 여부

z88dk MSX rom 모드 (`+msx -subtype=rom`) 는 Konami pragma 없이도 32 KiB main 빌드 가능 (`/opt/z88dk/src/appmake/msxrom.c` 의 `len = 0x8000` 분기). 결과 layout:
- 모든 코드/데이터가 cart $4000-$BFFF 에 linear 배치
- crt0 의 mapper bias writes 모두 skip (조건문이 0)
- 매퍼 인프라 (banked_call, PUT_P2 등) 그래도 link 됨 (사용 안 되면 dead code 로 strip 가능)

**우리 kingsvalley 1차 시도에서 사용한 `MAPPER_KONAMI=1` 은 잘못. 제거해야 함.**

### Iter 5 — 원본 SDCC custom crt0 분석

`game/src/crt0.z80`:
```asm
.module crt0
.area _CODE
ENASLT = 0x0024
RSLREG = 0x0138
CLIKSW = 0xf3db

    ; ROM header (총 16 byte; entry vector $4010)
    .str "AB"
    .dw _main_init
    .ds 12   ; 12 zero bytes (BASIC text / device / stat / extra entry slots)

_main_init::
    di
    ld sp, #0xf380
    ei
    call RSLREG          ; A = primary slot byte
    rrca; rrca; and #3   ; → A = primary slot index
    ld c, a
    add a, #0xc1         ; → SLTTBL+slot offset
    ld l, a
    ld h, #0xfc
    ld a, (hl)            ; SLTTBL entry
    and #0x80
    or c                  ; expanded slot bit
    ld c, a
    inc l x4              ; SLTTBL +4 = sub-slot register copy
    ld a, (hl)            ; current slot config
    and #0x0c
    or c                  ; final slot byte for page 2
    ld h, #0x80
    call ENASLT           ; enable our slot at page 2
    call _gsinit          ; SDCC init globals (call constructors)
    call _main
    ; (no return path)
```

**z88dk standard crt0 도 같은 ENASLT 시퀀스 + 추가 (heap/stdio init, eidi, tms99x8_mode_init)**. 호환 가능.

원본 entry vector at $4010 = 32-byte ROM header + immediately code (di; ld sp; ei; ENASLT; gsinit; main).

### Iter 6 — SDCC ABI vs sccz80 ABI

| 항목 | SDCC z80 | sccz80 |
|------|---------|--------|
| 인자 push 순서 | RIGHT-to-LEFT (last 인자 deepest) | LEFT-to-RIGHT (first 인자 deepest, EMPIRICAL 검증) |
| uint8_t arg | 1 byte (packed) | 2 byte (zero-extended to word) |
| 16-bit arg | 2 byte | 2 byte |
| Return uint8_t | A | L |
| Return uint16_t | HL | HL |
| __z88dk_fastcall | (해석 시) HL = single arg | HL = single arg |

**우리 `mplayer_init/play_effect/play_effect_p` wrapper 는 이 ABI 변환 처리 완료**. 하지만 **kingsvalley 자체 코드는 SDCC 컴파일 가정**, `__SDCC` 가드로 분기.

→ z88dk 빌드 시 `__SDCC` 가드 inside 의 코드는 활성화되면 안 됨 (그건 SDCC 빌드 전용). 우리 1차 시도에서 `__SDCC → MSX_BUILD` rename 한 것은 정확.

### Iter 7 — ap-z88dk 포팅 재검증

원본 `ap.z80` (kingsvalley 의) 와 우리 `ap.asm`:
- Body 알고리즘 byte-identical (sdasz80 vs z88dk z80asm 결과 비교 검증됨)
- C wrapper: SDCC 는 sp+2..3=arg1, sp+4..5=arg2 / sccz80 은 sp+4..5=arg1, sp+2..3=arg2 — wrapper 가 sccz80 layout 으로 변환. Empirical 검증됨 (최근 sccz80 push 순서 테스트).
- IX/IY 보존: wrapper 가 push ix/iy 후 body 호출, pop iy/ix 후 ret — 추가됨

**ap.lib 자체 는 정상. 호출 site 가 정확하면 동작해야 함.**

### Iter 8 — spman-z88dk 재검증

kingsvalley 의 `src/spman/spman.c` 와 우리 `Library/MSX/spman-z88dk/src/spman/spman.c`:
- 함수 7개 (init, alloc_pat, alloc_sprite, alloc_fixed_sprite, sprite_flush, update, hide_all_sprites) 모두 동일
- diff 는 cosmetic (`SPMAN_SPR_ATTRS` 매크로, 주석)
- 우리 spman.c 가 `ubox_wait_vsync()` 함수 호출 (Phase 7 변경 후)

**spman-z88dk 자체 문제 없음.**

### Iter 9 — mplayer-z88dk 재검증

8개 wrapper + AKM bridge:
- 3개 wrapper (init, play_effect, play_effect_p) 의 sccz80 stack 추출 패턴은 검증됨 (05_music/06_sound 작동)
- AKM blob 은 ORG=$C200 (또는 다른 RAM 주소) 에 LDIR-loaded
- kingsvalley는 SDCC build 시 `PLY_AKM_Rom=1` 모드 (cart 직접 실행) 였음. 우리 z88dk 는 LDIR-to-RAM. **두 모드 자체는 호환되나 메모리 배치 다름.**

**mplayer-z88dk 자체는 05_music/06_sound 에서 작동. kingsvalley 통합 시 BSS 와 충돌 회피만 잘 하면 OK.**

### Iter 10 — `<header.h>` vs `"header.h"` 충돌

`game/generated/{enemy,knife}.h` 와 `game/src/{enemy,knife}.h` 가 동명. SDCC: `"x.h"` 는 source-dir-first → src/enemy.h. sccz80: `"x.h"` 가 -I path 우선순위 따름 → 첫 -I 의 enemy.h.

**해결**: generated header rename (`enemy_data.h`, `knife_data.h`) — 우리 1차 시도에서 이미 적용. 유효한 fix.

### Iter 11 — `__SDCC` → `MSX_BUILD` 리팩터링

`__SDCC` 가드 5곳 (main.c × 4 + main.h × 1) — 우리 1차 시도에서 sed 적용. 유효한 fix.

`enum songs` (main.h:14) 는 `_WIN32 || DJGPP || __linux` 가드 (NOT `__SDCC`) — 무관.
`enum effects` (main.h:38) 는 `__SDCC` 가드 — refactor 필요.

### Iter 12 — VRAM 주소 / 화면 설정 가정

원본 main.c (line 38-40):
```c
#if !defined(MSX_BUILD)
uint8_t* VIDEO_MEMORY_ADDRESS = 0;
#else
uint8_t* VIDEO_MEMORY_ADDRESS = 0x1800;
#endif
```

`0x1800` = MSX SCREEN 2 의 NAME table base (per BIOS). `ubox_write_vm(VIDEO_MEMORY_ADDRESS, ...)` 로 사용. 우리 ubox_write_vm 은 BIOS LDIRVM 또는 직접 VDP I/O 호출. 호환.

### Iter 13 — Boot slot / page 설정

원본 SDCC crt0 가 ENASLT 명시 호출 (Iter 5 참조). z88dk standard crt0 도 동일 ENASLT (target/msx/classic/rom.asm `start:` 부근). 양쪽 모두 page 2 ($8000-$BFFF) 를 cart slot 에 매핑.

GT (Panasonic_FS-A1GT) 는 expanded slot 사용 가능. ENASLT 가 sub-slot 비트도 올바르게 처리해야 함. SDCC crt0 의 RSLREG + SLTTBL 조합이 이를 처리. z88dk crt0 의 동등 코드 도 처리.

prototype_05 가 GT 에서 작동하므로 z88dk crt0 의 ENASLT 자체 는 GT 호환.

### Iter 14 — SDCC 빌드 도구 가용성

- `sdcc` ✓
- `sdasz80` ✓
- `sdar` ✓
- `hex2bin` ✓ (`/usr/local/bin/hex2bin`)
- `Disark` ❌ (rasm 출력을 SDCC asasm 으로 변환) — 우리 시스템 미설치
- `rasm` ✓ (in-tree built)

**Disark 부재로 원본 SDCC 의 AKM player 빌드 불가**. 우리 z88dk 포트는 INCBIN + LDIR 우회 — Disark 미필요.

### Iter 15 — 원본 ROM vs 우리 빌드 비교

원본 `kings_original.rom`:
- 32 KiB plain
- entry $4010 (di; ld sp, $F380; ei; ENASLT logic; gsinit; main)
- 매퍼 쓰기 0건

우리 (1차 시도) 빌드:
- 32 KiB Konami 매퍼 (또는 64 KiB w/ BANK_02)
- entry $4010 (z88dk crt0)
- 매퍼 bias writes 12개

**근본 차이 = 매퍼 사용 여부**. 1차 시도 fix 필수.

### Iter 16 — 최소 viable 포팅

minimal 부팅 보장 위한 step:
1. **plain 32K cart** (NO Konami pragma)
2. **MSX_BUILD 가드** 활성 + game/src 리팩터링
3. **enemy.h/knife.h rename** (header 충돌 회피)
4. **mid-block decl 리팩터** (enemy.c, trap.c, game_util.c)
5. **mplayer stub 사용** (음악 미지원, AKM 빌드 우회) — 첫 단계
6. **ap-z88dk** 라이브러리 link (game/src 가 호출)
7. **spman-z88dk** 라이브러리 link
8. **ubox-msx-lib-z88dk** + 누락 함수 (`ubox_update`, `ubox_random`)
9. **z88dk 자체 crt0 사용** (custom crt0 드롭)

### Iter 17 — 점진 포팅 전략

**Phase A**: minimal 부팅 — 음악 없이 게임 화면이 뜨는지 확인
- 원본 SDCC 의 32K layout 정확히 재현 (plain cart)
- 모든 mplayer 호출 stub
- ap_uncompress 호출 → 정상 디컴프 검증 (title 화면 표시)

**Phase B**: 음악 통합 — Phase A 작동 후
- mplayer-z88dk + AKM 추가
- BSS 배치 ($C000+) 와 AKM_BASE 충돌 회피

**Phase C**: 검증 + 정리

### Iter 18 — 신규 PLAN 작성 (이하 § 3)

기존 1차 PLAN 폐기. 아래 § 3 가 v2 PLAN.

### Iter 19 — v2 PLAN 검증 (각 가정의 출처)

| 가정 | 출처 |
|---|---|
| 원본은 plain 32K cart | Iter 2/3 — GitHub release ROM 분석 |
| z88dk plain 32K 가능 | Iter 4 — msxrom.c 분기 분석 |
| ENASLT 호환 | Iter 13 — SDCC vs z88dk crt0 비교 |
| SDCC ABI 호환 wrapper 필요 | Iter 6 — empirical sccz80 push order |
| Disark 미필요 (INCBIN 우회) | Iter 14 — 우리 mplayer-z88dk 패턴 |

### Iter 20 — 최종 정합성 + 실행 가능 산출물

§ 3 의 각 step 이 실행 가능한 single-script 형태 + chmod +x. 원본 대비 minimum 변경으로 작동 보장.

---

## 2. v2 PLAN — Phase A (minimal boot)

### A-1. 라이브러리 (이미 완료)

- `Library/MSX/ubox-msx-lib-z88dk/` — `ubox_update.asm`, `ubox_random.asm` 추가. `ubox_isr.asm` 은 **default `code_user` section 으로 복귀** (page 1 강제 시도 무용).
- `Library/MSX/spman-z88dk/` — 그대로
- `Library/MSX/mplayer-z88dk/` — 그대로
- `Library/MSX/ap-z88dk/` — 그대로

### A-2. kingsvalley game source 변경 (in-place)

1. `__SDCC` → `MSX_BUILD` (5곳 sed)
2. `game/generated/enemy.h` → `enemy_data.h`, `game/generated/knife.h` → `knife_data.h`. game.c, data.c 의 `<enemy.h>`/`<knife.h>` → `<enemy_data.h>`/`<knife_data.h>`.
3. mid-block decl 리팩터 (enemy.c:49, trap.c:17/34, game_util.c:128)
4. `game/src/akm.z80` → 그대로 (AKM 사용 안 함, stub 으로 우회)

### A-3. 신규 파일 (kingsvalley 폴더 안)

- `Examples/kingsvalley/akm_stub.asm` — 모든 mplayer 심볼 + `_SONG`, `_EFFECTS` stub (no-op return)
- `Examples/kingsvalley/compile_phaseA.sh` — **plain 32K cart** (NO `-pragma-define:MAPPER_KONAMI=1`)
- `Examples/kingsvalley/run_phaseA.sh` — `-machine Panasonic_FS-A1GT -carta <rom>` (NO `-romtype` flag, plain cart auto-detect)
- `Examples/kingsvalley/.gitignore` 갱신 (`build/`)
- (Phase B 진행 시 `compile_phaseB.sh`/`run_phaseB.sh` 추가)

### A-4. compile_phaseA.sh 핵심 ZCCFLAGS

```bash
ZCCFLAGS="+msx -subtype=rom -compiler=sccz80 -SO2 -create-app"
# NO -pragma-define:MAPPER_KONAMI=1
ZCCFLAGS="$ZCCFLAGS -DMSX_BUILD"
ZCCFLAGS="$ZCCFLAGS -I$KV/game/generated -I$KV/game/src"
ZCCFLAGS="$ZCCFLAGS -I$UBOX_DIR/include -I$SPMAN_DIR/include -I$MPLAYER_DIR/include -I$AP_DIR/include"
ZCCFLAGS="$ZCCFLAGS -L$UBOX_DIR/lib -lubox -L$AP_DIR/lib -lap"
```

### A-5. 검증

- `compile_phaseA.sh all` → 32 KiB ROM 산출
- map 파일에서 `__BSS_END_tail` 가 BIOS work area ($F380) 미만인지 확인
- `run_phaseA.sh` → GT 에서 타이틀 화면 (kingsvalley 로고/메뉴) 표시 확인
- **이게 작동하면 Phase B 진행** (음악 통합)

---

## 3. v2 PLAN — Phase B (AKM 음악)

Phase A 작동 후:

### B-1. akm_bridge.asm + INCBIN

mplayer-z88dk Phase 5 패턴 그대로 — `game/src/akm.z80` 에 `ORG #C200` 추가, rasm 으로 `akm.bin` 산출, akm_bridge.asm 으로 INCBIN.

### B-2. AKM_BASE 결정

Phase A 작동 후 map 파일 분석으로 BSS 끝 찾기. AKM_BASE 를 BSS 위 + $F380 미만 안전 범위 내 결정 (가능하면 $E000 또는 그 이상).

### B-3. mplayer_engine_load() 호출

main.c 에 `mplayer_engine_load()` 호출 추가 (init 직후, mplayer_init 직전).

### B-4. Verification

음악 + 효과음 동작 확인.

---

## 4. v2 PLAN — Phase C (정리)

- 메모리 노트 작성
- 테스트 ROM 비교
- 문서 업데이트

---

## 5. 위험 / 미결 (v2)

| 항목 | 영향 | 대응 |
|---|---|---|
| z88dk plain 32K cart 의 page 2 매핑 | 알 수 없음 — z88dk crt0 가 ENASLT 호출하는지 plain 모드에서도 | rom.asm 코드 재확인. ENASLT 는 매퍼 무관하게 호출됨 |
| Game code SDCC-isms 잠복 (1차 시도 grep 기반 0 건이었음) | Low | 빌드 에러 시 발견 |
| ROM 사이즈 32 KiB 초과 가능성 | Medium | mplayer wrapper 5개 미사용 link 안 함 + ubox 미사용 함수 dead-strip |
| AKM blob 추가 후 사이즈 초과 | Phase B | Phase B 에서 처리 |
| Konami 매퍼 가 GT 에서 hang 일으킨 root cause | Iter 3 분석으로 확립 | plain cart 빌드 로 회피 |

---

## 6. 보강 분석 (Round-B, Iter 21-26)

> ※ §0 의 Iter 1-20 (재검토 라운드 1) 와 충돌하지 않도록 Iter 21 부터 부여.
> Iter 번호는 분석 라운드의 누적 일련번호임.

### Iter 21 — Konami pragma 영향 정량화

| 매핑 | Konami pragma 32K (1차 시도) | Plain 32K (시도 안 됨) |
|------|----------------------------|----------------------|
| 매퍼 인프라 | bias writes (15B dead code) + banked_call (~50B 미사용) | 0 |
| openMSX `-romtype` | `Konami` (강제) | (auto-detect) |
| 매핑 방식 | bank-based (`$4000`/`$6000`/`$8000`/`$A000` 페이지마다 register-controlled) | linear cart `$4000-$BFFF` |
| 32K main 위험 | banks 0/1/2/3 default 매핑 가정 — emulator 의존 | linear, 보장됨 |

**plain 32K 가 본질적으로 안전. 우리는 plain 시도 안 함.** ← KEY GAP.

### Iter 22 — bias writes 의 실제 위치 (prototype_05 분석)

prototype_05 ROM map:
- `__Exit = $403D`, `l_dcal = $4043`, `banked_call = $405F`
- bias writes (`32 00 60`/`80`/`a0` × 9개 = 27 bytes) 가 `$4044-$405E` 사이 = `l_dcal` 와 `banked_call` 사이
- **dead code** — startup → __main 사이에 실행 안 됨 (l_dcal 가 jp(hl) 로 끝나서 fall-through 없음)

bias writes 자체는 GT hang 직접 원인 아님. 그러나 **Konami 매퍼 활성화** 자체가 boot bank 상태를 emulator-dependent 로 만듦.

### Iter 23 — kingsvalley 의 ubox API 사용 (prototype_05 와 비교)

prototype_05 가 안 쓰는 ubox 호출 (kingsvalley 만):
- `ubox_set_user_isr` (mplayer tick 등록)
- `ubox_wait_for/wait_vsync` (frame wait)
- `ubox_write_vm` (VRAM 쓰기 — map 데이터)
- `ubox_select_ctl/read_ctl` (조이패드)
- `ubox_update` (cross-platform stub)
- `ubox_finalize/init_game_system/load_*/render_background` (모두 `!__SDCC` 가드 = MSX 미실행)

MSX-active 추가 호출 = `set_user_isr/wait_for/wait_vsync/write_vm/select_ctl/read_ctl/update`.

이 중 `ubox_set_user_isr` 가 잠재 위험 (HTIMI 콜백 = mplayer_play). 우리 1차 시도에서 disable 했지만 stuck 동일. 결과적으로 **함수 자체는 무관, 32K 매퍼 설정이 핵심**.

### Iter 24 — 라이브러리에 매퍼 가정 無

ubox/spman/mplayer/ap z88dk port 모두 `MAPPER_*`/`banked_call`/`PUT_P2`/`__current_bank` 참조 0건. **모두 매퍼-agnostic**. Konami pragma 제거해도 라이브러리 기능 정상.

### Iter 25 — 가설 정리: 매퍼 타입이 단일 변수

|         | main 사이즈 | ubox_isr 위치 | 매퍼 | GT 결과 |
|---------|-----------|-------------|------|--------|
| 12 예제 | ≤16K | page 1 (`$5xxx`) | plain 16K | OK |
| prototype_05 | ≤16K main + BANK_01 | page 1 | Konami 32K | OK |
| 원본 kingsvalley | ~30K (linear) | page 2 (`$8xxx`) | plain 32K | OK |
| 우리 1차 시도 | 31K (linear) | page 2 (`$9xxx`) | **Konami** 32K | **FAIL** |

→ **유일한 변수 = 매퍼 타입 (plain vs Konami)**. 32K linear main + page-2 ISR 의 조합 자체는 원본이 이미 GT 에서 작동하므로 무관 (Round-C Iter 28/32 에서 codex 가 byte-level 로 재확인).

원본 kingsvalley 도 page 2 에 ISR 가 있는데 작동하는 건 **plain cart slot mapping** 덕분 (page 2 가 항상 cart, mapper-controlled bank swap 없음).

### Iter 26 — 최종 결론 (Round-B)

**유일하게 시도 안 한 조합 = plain 32K (no Konami pragma)**. 원본 ROM 과 같은 cart 타입.

다음 실행 단계 (Phase A 재정의):

| Step | 액션 |
|------|------|
| A-1 | game source: `__SDCC → MSX_BUILD` (5 sed), `enemy.h/knife.h` rename, mid-block decl 3곳 리팩터 |
| A-2 | `akm_stub.asm` (mplayer/AKM symbol 모두 ret) |
| A-3 | `compile_phaseA.sh`: `+msx -subtype=rom -compiler=sccz80 -SO2 -create-app -DMSX_BUILD` (★ NO `-pragma-define:MAPPER_KONAMI=1`) |
| A-4 | `run_phaseA.sh`: `openmsx -machine ${MACHINE:-Panasonic_FS-A1GT} -carta build/kings.rom` (★ NO `-romtype` flag) |
| A-5 | 빌드 → GT 부팅 검증. 타이틀 화면 보이면 **PHASE A 성공** |
| A-6 | 실패 시: 단계적으로 ap_uncompress / spman / 게임 로직 isolate 해서 좁힘 |

Phase A 성공 후 Phase B (AKM 음악 통합) 진행.

---

## 7. 심화 분석 (Round-C, Iter 27-37, codex 교차검증)

> ※ Round-A (§0 Iter 1-20) + Round-B (§6 Iter 21-26) 누적 일련번호 계속.
> 본 라운드는 codex 교차검증 + 원본 ROM 의 byte-level 분석 + binary 비교 절차 정의.

### Iter 27 — 원본 ROM startup byte-level 디스어셈블

`kings_original.rom` offset 0x10 부터:
```
$4010: F3                 di
$4011: 31 80 F3           ld sp, $F380          ; SDCC 하드코드
$4014: FB                 ei
$4015: CD 38 01           call RSLREG
$4018: 0F 0F E6 03         rrca; rrca; and $03
$401C: 4F                 ld c, a
$401D: C6 C1 6F 26 FC     add a, $C1; ld l, a; ld h, $FC  ; opcode C6 = ADD A,n (LD A,n 은 3E nn)
$4022: 7E E6 80 B1 4F     ld a, (hl); and $80; or c; ld c, a
$4027: 2C 2C 2C 2C        inc l × 4              ; SDCC 패턴 (z88dk: inc hl × 4)
$402B: 7E E6 0C B1        ld a, (hl); and $0c; or c
$402F: 26 80 CD 24 00     ld h, $80; call ENASLT
$4034: AF 32 DB F3        xor a; ld (CLIKSW), a  ; ★ key click 끄기
$4038: CD B9 BA           call gsinit ($BAB9)     ; page 2!
$403B: CD 6D 9D           call _main ($9D6D)      ; page 2!
$403E: 76 18 FD           halt; jr -3
```

**핵심**: 원본 ROM 도 _main / gsinit 모두 page 2 영역에 위치. SDCC custom crt0 와 z88dk crt0 본질 동일 (RSLREG → SLTTBL → ENASLT).

### Iter 28 — codex 교차검증: ubox_isr in page 2 가설 무효화

codex 가 원본 ROM 안의 ubox_isr signature (`f5 dd e5 fd e5 c5 e5 d5`) 를 grep:
- **원본 `kings_original.rom`: ubox_isr at file offset 0x75FC = MSX `$B5FC`** (page 2!)
- 원본 GT 작동 확인됨

→ "ISR in page 2 = GT 비호환" 가설 **무효**.

### Iter 29 — codex 의 sharper hypothesis: Konami 매퍼 + page 2 write

`-romtype Konami` 환경에서 [openMSX RomKonami.cc:63-66]:
```cpp
void RomKonami::writeMem(uint16_t address, byte value, ...) {
    if (0x6000 <= address && address < 0xC000) {
        bankSwitch(address >> 13, value);
    }
}
```

→ **`$6000-$BFFF` 범위로의 모든 memory write 가 bank-switch trigger**. plain cart 에선 같은 write 가 ignored (ROM 영역).

원본 (plain): 안전.
prototype_05 (Konami 32K, main ≤16K): 안전 — main 16K 가 cart bank 0+1 (`$4000-$7FFF`, Konami 의 8K-bank × 2개) 에 fit, `$8000-$BFFF` 영역에 BANK_01 데이터만 (rodata, write 없음). 게다가 prototype_05 의 bias writes 는 dead code (Iter 22).
**우리 (Konami 32K, main 31K linear)**: code/data 가 `$4000-$BFFF` spread, 어떤 write 가 `$6000-$BFFF` 로 가면 mapper trigger.

### Iter 30 — kingsvalley game source 의 매퍼-trigger 가능 store 검색

`grep -rE "0x[6-9aAbB][0-9a-fA-F]{3}"` on game/src/*.c, *.h: **0건**. game source 자체에는 `$6000-$BFFF` 영역 직접 메모리 참조 없음. `VIDEO_MEMORY_ADDRESS = 0x1800` (VRAM, 메모리 아님)뿐.

### Iter 31 — 라이브러리 .asm 의 매퍼-trigger 가능 store

`Library/MSX/{ubox,spman,mplayer,ap}-z88dk/src/`: **0건**. 라이브러리 자체엔 `ld ($6xxx-$Bxxx), reg` 패턴 없음.

→ codex 가설의 "매퍼 trigger write" 가 source-level 에 없음. 그러나 **sccz80 가 컴파일 결과 emit 하는 pattern 에 잠복 가능** — 빌드 후 ROM 디스어셈블로만 검증 가능.

### Iter 32 — 원본 ROM 의 ubox_isr position 확정

bytes at file offset 0x75FC:
```
$B5FC: F5 DD E5 FD E5 C5 E5 D5  ; push af/ix/iy/bc/hl/de
$B604: AF 32 F6 F3 32 F7 F3      ; xor a; ld (SCNCNT/REPCNT), a
$B60B: 21 4A D4                  ; ld hl, $D44A (ubox_isr_wait_tick BSS)
$B60E: 34 23 34 23 23           ; inc (hl); inc hl × 2 + inc (hl)
```

우리 z88dk port `ubox_isr.asm` 의 prologue 와 byte-level identical (단 alt-register 보존 부분은 우리가 추가).

→ 원본 SDCC ubox 와 우리 z88dk port 의 ISR 구조 동일. ISR 자체 호환.

### Iter 33 — openMSX 매퍼 자동 감지

[openMSX RomPlain.cc:115] `guessHelper`:
- `AB` header + valid entry vector → plain cart 후보
- 32K cart + `init=$4010` → guessLocation `$4000` placement → **자동 plain mapper 사용**

[openMSX RomKonami.cc:63] `writeMem`:
- 0x6000-0xBFFF write → `bankSwitch(addr >> 13, value)`

→ `-romtype` flag 안 주면 openMSX 는 hash DB 또는 heuristics 로 판단. 원본 32K plain cart 는 `Plain` 으로 자동 매핑. 우리 빌드도 `-romtype` 빼면 자동으로 Plain 매핑 가능성 높음.

### Iter 34 — Binary 비교 절차 (PLAN 에 추가)

z88dk 로 plain 32K 빌드 후 다음 비교 수행:

#### B-1. 사이즈 + 헤더

```bash
# 1. ROM 사이즈 (정확히 32768)
wc -c build/kings.rom /tmp/kv_refs/kings_original.rom

# 2. 카트 헤더 (둘 다 "AB" + entry $4010)
xxd -l 16 build/kings.rom
xxd -l 16 /tmp/kv_refs/kings_original.rom
```

#### B-2. Startup sequence 비교

```bash
# entry 후 64 byte 비교 (DI / SP init / RSLREG / ENASLT / call gsinit / call _main)
xxd -s 0x10 -l 64 build/kings.rom > /tmp/our_startup.hex
xxd -s 0x10 -l 64 /tmp/kv_refs/kings_original.rom > /tmp/orig_startup.hex
diff /tmp/our_startup.hex /tmp/orig_startup.hex
```
**기대**: byte-exact 일치는 어려우나 (z88dk 는 `inc hl×4`, SDCC 는 `inc l×4` 등) **함수 sequence 동일** (DI → SP → RSLREG → SLTTBL 인덱싱 → ENASLT → init → call _main).

#### B-3. 매퍼-trigger pattern scan

```python
# 우리 ROM 의 모든 ld (nn), reg (nn ∈ $6000-$BFFF) 검출
data = open('build/kings.rom', 'rb').read()
for i in range(len(data)-2):
    if data[i] in (0x32, 0x22):  # ld (nn), a / ld (nn), hl
        addr = data[i+1] | (data[i+2] << 8)
        if 0x6000 <= addr < 0xC000:
            print(f'offset 0x{i:04X}: ld (${addr:04X}), {"a" if data[i]==0x32 else "hl"}')
```

**Plain cart 라면 이 write 들 모두 무해 (ROM area write = ignored). Konami 라면 매퍼 trigger.**

#### B-4. 핵심 심볼 비교

```bash
# 우리 z88dk 빌드의 .map 에서:
grep -E "^_main\b|^_ubox_init_isr\b|^ubox_isr\b|^_mplayer_init\b|^_ap_uncompress\b|^__BSS_head\b|^__BSS_END_tail\b" build/kings.map

# 원본 ROM 의 동일 심볼 (signature grep으로 위치 추출):
# ubox_isr signature: f5 dd e5 fd e5 c5 e5 d5
# main entry: 32 fa d1 21 ff d1 (specific to kingsvalley main)
```

**기대**: 우리 빌드와 원본의 심볼 위치는 다를 수 있음 (z88dk libc 추가 코드 등). 그러나 **두 빌드 모두 모든 심볼이 32K 안 ($4000-$BFFF) 에 fit + BSS 가 $C000+ 에 fit + stack 안전 영역**.

#### B-5. 매퍼 인프라 부재 확인

우리 plain 32K 빌드에 다음 NONE:
- `bias writes` (Konami pragma 결과물): ld ($6000)/(\$8000)/(\$A000), a 시퀀스 9개 — Konami 없으면 emit 안 됨
- `banked_call` 메커니즘 (~50B 코드): 없음
- `__current_bank` BSS byte: 없음
- `PUT_P2` / `__far_map_bank`: 없음

```bash
# Konami 인프라 잔재 검사
grep -E "banked_call|__current_bank|PUT_P2|__far_map_bank" build/kings.map
# 기대: 0건
```

### Iter 35 — Phase A 명세 검증

| 항목 | 명세 | 가정 검증 |
|------|------|---------|
| ZCCFLAGS | `+msx -subtype=rom -compiler=sccz80 -SO2 -create-app -DMSX_BUILD` | NO `-pragma-define:MAPPER_KONAMI=1` |
| openmsx 호출 args (run_phaseA.sh) | `-machine $MACHINE -carta $ROM` | NO `-romtype` flag |
| ROM 크기 | 32 KiB | msxrom appmake: main_length 16K~32K → 32K cart 산출 |
| 매퍼 | openMSX auto-detect → Plain | RomPlain.cc heuristics 검증 (AB header + entry $4010) |
| 32K main 가능성 | z88dk msxrom.c:54 의 `len = 0x8000` 분기 | 검증됨 |
| ubox_isr 위치 | page 2 (likely) | 무관 — 원본도 page 2, plain cart 면 항상 visible |

### Iter 36 — kingsvalley source code 의 추가 버그 발견 (Phase A 실행 전 사전 픽스 필요)

분석 중 source code 본 결과 **GT hang 과 별개로 두 군데 버그**:

**B1. `VIDEO_MEMORY_ADDRESS = 0` (non-SDCC branch, main.c:38)**
```c
#if !defined(__SDCC)
uint8_t* VIDEO_MEMORY_ADDRESS = 0;          // ← 0! 잘못됨
#else
uint8_t* VIDEO_MEMORY_ADDRESS = 0x1800;     // ← VRAM name table base
#endif
```
z88dk 빌드는 `!defined(__SDCC)` 분기 → `VIDEO_MEMORY_ADDRESS=0`. 그러면 `ubox_write_vm(VIDEO_MEMORY_ADDRESS, ...)` 가 VRAM offset 0 (pattern table 영역) 에 맵 데이터 기록 → 화면 깨짐.

**픽스**: 0 → 0x1800 또는 SDCC/non-SDCC 분기 통일.

**B2. AKM ROM buffer 위치 = `$C000` (akm_ubox.asm:6)**
```asm
PLY_AKM_ROM_Buffer = #c000
```
$C000 = z88dk MSX ROM build 의 BSS 시작 영역. kingsvalley BSS 약 5KiB. AKM 의 64-byte runtime buffer 가 BSS 와 겹치면 fatal.

**검증 필요**: build/kings.map 의 `__BSS_head` / `__BSS_END_tail` 위치. $C000-$C040 buffer vs BSS 첫 $40 byte 충돌 여부.

### Iter 37 — 최종 PLAN 정합성

Round-A/B/C 누적 분석은 **단일 결정적 가설 = Konami pragma 사용 자체가 GT hang 원인** 으로 수렴.

근거:
1. 원본 kings_original.rom (plain 32K, SDCC custom crt0): **GT 작동** (Iter 27)
2. 원본도 main/ubox_isr 가 page 2 영역 — page-2 ISR 가설 무효 (Iter 28, 32)
3. game source / 라이브러리 .asm 어디에도 `$6000-$BFFF` 영역 직접 store 없음 (Iter 30, 31)
4. openMSX `-romtype Konami` 시 `[$6000, $C000)` 모든 write 가 bank-switch trigger (Iter 29, 33)
5. plain 32K cart 는 openMSX heuristics 로 자동 RomPlain 매핑 (Iter 33)

남은 검증 항목 (Phase A 빌드 후):
- `__BSS_END_tail < $F380` (stack 충돌 방지)
- sccz80 emit 가 `$6000-$BFFF` write 생성 안 함 (binary 비교 B-3, Iter 34)
- AKM `$C000` buffer vs BSS 충돌 여부 (Iter 36/B2 — Phase B 진입 시점에 처리)
- `VIDEO_MEMORY_ADDRESS=0` 픽스 (Iter 36/B1)

**결론**: Phase A 실행 (plain 32K + B1 픽스, B2 는 Phase B 시점) → GT 검증. 그 후 Iter 34 binary 비교 절차로 결과 정합성 검증.

---

## 8. 작업 순서 (요약, v2 final)

### Phase A — minimal viable (음악 없음) ✅ **완료**

1. ☑ Reference baseline 확인 — `./run_reference.sh` 로 원본 ROM 정상 동작 확인
2. ☑ kingsvalley game source 패치
   - main.c: 5× `__SDCC` 가드를 `MSX_BUILD || __SDCC` 로 확장
   - main.h: 1× `__SDCC` 가드 동일 처리
   - main.c:38 `VIDEO_MEMORY_ADDRESS = 0` → `(uint8_t*)0x1800` (Iter 36/B1)
   - generated/{enemy,knife}.h → enemy_data.h/knife_data.h + data.c/game.c 호출자 갱신
   - enemy.c:49, trap.c:17, game_util.c:127: struct compound init → field-by-field 할당 (sccz80 호환)
3. ☑ `akm_stub.asm` 작성 (mplayer/AKM 심볼 모두 ret + `_SONG`/`_EFFECTS` zero arrays)
4. ☑ `compile_phaseA.sh`: **NO Konami pragma**, plain 32K cart, spman 은 source 직접 컴파일
5. ☑ `run_phaseA.sh`: **NO `-romtype`**, plain cart auto-detect
6. ☑ 빌드 → 32,768 B plain ROM 산출. GT 검증 — 타이틀 + 게임 진행 OK
7. ☑ Iter 34 binary 비교 절차 통과 (B-1~B-5)

### Phase B — AKM 음악

1. ☐ `game/src/akm.z80` 에 `ORG #...` 추가 (Phase A 성공 후 BSS 위 안전 주소 결정)
2. ☐ akm_bridge.asm + INCBIN 통합
3. ☐ main.c 에 `mplayer_engine_load()` 호출 추가
4. ☐ 음악/효과음 검증

---

## 9. 참고 자료

**Reference ROMs (절대 기준)**: `/tmp/kv_refs/kings_original.rom` 외 2개 — 32 KiB plain cart, GT 호환 검증됨 (GitHub release).

**Working z88dk references**:
- `Examples/prototype_05_MSX_ROM_MSXDOS/` — Konami 매퍼 32K, GT 작동
- `Examples/ubox_example_z88dk/examples/{01..12}/` — 16 KiB plain cart 또는 .com, GT/openMSX 작동

**z88dk MSX rom 모드 자료**:
- `/opt/z88dk/lib/target/msx/classic/rom.asm` — startup + 매퍼 conditional defc
- `/opt/z88dk/src/appmake/msxrom.c` — appmake bank packing logic

**메모리 노트**:
- MEMORY.md → "z88dk Calling Conventions"
- `reference_ubox_z88dk_examples.md`
- `reference_kingsvalley_z88dk.md` (1차 시도 기록 — 무효화)
