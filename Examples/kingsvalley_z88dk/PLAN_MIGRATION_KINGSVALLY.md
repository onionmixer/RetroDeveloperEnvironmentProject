# PLAN — King's Valley (kingsvalley) z88dk 포팅 (재검토 v2)

**상태**: **Phase A + B + C(CALSLT) + C-Konami 변형 완료 ✓** (2026-05-11, 사용자 GT 검증).
`./run_phaseB.sh` (= `build_phaseB/kings.rom`, ASCII16 정식 배포) 가 `./run_reference.sh` (= 원본 SDCC `kings_original.rom`) 와 **가장 근접한 동작** 확인.
`./run_phaseC.sh` (= `build_phaseC/kings.rom`, Konami 매퍼 변형 production) 도 부팅/음악/플레이 정상 동작 확인.

**작성**: 2026-05-10 시작, 2026-05-11 최종 갱신 (Phase C(CALSLT) + Phase C-Konami 변형 + prototype_06 학습 패턴 통합).

---

## 0. 빠른 참조 (TL;DR)

### 최종 산출물

| 빌드 스크립트 | 산출 ROM | 컴파일러 | 매퍼 | 음악 |
|---------------|---------|----------|------|-----|
| `compile_phaseA.sh build` | `build_phaseA/kings.rom` (32 KiB) | z88dk + sccz80 | plain | ✗ (akm_stub) |
| `compile_phaseB.sh build` | `build_phaseB/kings.rom` (64 KiB) ★ | z88dk + sccz80 | ASCII16 | ✓ (AKM) |
| `compile_phaseC.sh build` | `build_phaseC/kings.rom` (64 KiB) ★★ | z88dk + sccz80 | **Konami** | ✓ (AKM) |
| `build_sdcc.sh` | `game/build/kings_sdcc.rom` (32 KiB) | SDCC | plain | ✗ (stub) |
| `compile_phaseB_variant.sh {variant}` | `build_phaseB_<variant>/kings.rom` | z88dk + sccz80 | ASCII16 | ✓ | 진단 archive |

★ **정식 배포** = `compile_phaseB.sh build` → `build_phaseB/kings.rom`. `run_reference.sh` 와 byte-동등 거동.
★★ **Konami 변형** = `compile_phaseC.sh build` → `build_phaseC/kings.rom`. ASCII16 대신 Konami 매퍼 사용 (prototype_06 학습 결과). 동일 게임/음악 동작. main 16K fit 유지 위해 추가 `#pragma bank 1` 분리 + `ap.asm` BANK_01 라우팅 + frames 분리 필요 (§10 참조).

### GT 실행

```bash
cd Examples/kingsvalley_z88dk
./run_phaseB.sh             # 정식 배포 ROM (z88dk Phase B, ASCII16 + 음악)
./run_phaseC.sh             # Konami 매퍼 변형 (z88dk Phase B 와 동일 게임, 매퍼만 교체)
./run_phaseA.sh             # 음악 없는 빌드 (z88dk Phase A, plain 32K)
./run_sdcc.sh               # SDCC 빌드 (byte-level 비교 baseline)
./run_reference.sh          # 원본 SDCC ROM (pdpdds/ubox_example v1.0 release)
```

### 핵심 결론 3가지

1. **plain cart 가 정답** — 원본 SDCC `kings_original.rom` 도 plain 32K. 1차 시도 `MAPPER_KONAMI=1` 가 GT hang 의 원인이었음 (openMSX RomKonami.cc 의 `[$6000, $C000)` 영역 모든 write 가 bank-switch trigger).
2. **64K 음악 통합 = ASCII16 매퍼 + BANK_02 + bank-1 mount shim** — AKM blob (3.5 KiB) 을 BANK_02 의 RODATA_2 에 두고 startup 시 LDIR. ASCII16 의 `$7000` trigger 는 sccz80 emit 와 충돌 안 함. `compile_phaseB.sh` 가 cart 의 dead-code 영역 ($4044+) 에 8-byte shim 주입해 crt0_init 의 DATA-LDIR 전 bank 1 mount.
3. **잔여 멈칫의 본질 원인 = ubox-msx-lib-z88dk 의 CALSLT 사용 + sccz80 codegen overhead** — 정량:
   - CALSLT: 호출당 ~220 cy. `compile_phaseA/B.sh` 가 `msxbios` → `msxbios_fast` sed-rewrite + `jp (ix)` 2-byte stub 으로 우회. ROM 모드 한정 안전. 도구 사용 시 1100-2200 cy 절감 = **멈칫 ~50% 감소** (사용자 GT 검증).
   - sccz80 codegen: SDCC 대비 평균 +5-7%, 도구 path 핵심 함수 (character.c +24%, character_move.c +31%) 가 frame budget 잠식. 컴파일러-level 한계로 수용.

### 핵심 결론 (Konami 변형 추가, §10)

4. **Konami 64K 변형도 가능** — 1차 시도 실패 원인은 매퍼 자체가 아니라 "main code 가 $4000-$BFFF spread 되어 sccz80 emit 의 write 가 `[$6000, $C000)` 매퍼 trigger 일으킬 가능성" 이었음. main 16K fit + page-2 ($8000-$BFFF) 를 BANK_01 (rodata 만) 로 채우는 layout 이면 안전. prototype_06_MSX_ROM_KONAMI_64K 학습 결과를 적용:
   - `#pragma bank 1` 분리: data.c / character.c / enemy.c / gate.c / frames_bank.c(신규)
   - `ap.asm` (aplib) 를 `SECTION code_user → CODE_1` sed-rewrite 로 BANK_01 라우팅 (main 16K fit 의 마지막 picksaw)
   - akm_bridge_konami.asm: Konami dual register ($8000+$A000) 로 BANK_02 swap (Phase B 의 ASCII16 단일 `$7000` 변형)
   - **crt0 pre-init shim 불필요** — openMSX RomKonami::reset 가 deterministic 하게 bank 2,3 을 $8000-$BFFF 에 mount (Phase B 의 ASCII16 와 다름).

### 진단 history (학습 자료)

본 문서의 §1 (1차 시도 실패) 부터 §7 (Round-C 심화 분석, Iter 27-37) 까지는
사용자의 1차 Konami pragma 시도가 실패한 이유를 추적 + 검증하는 진단 history.
새 MSX z88dk 포팅 작업에서 비슷한 hang 발생 시 참조.

§7 Iter 18 의 **binary 비교 절차 (B-1~B-5)** 가 다른 z88dk MSX cart 디버깅에도
재사용 가능한 절차.

---

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

- `Examples/kingsvalley_z88dk/akm_stub.asm` — 모든 mplayer 심볼 + `_SONG`, `_EFFECTS` stub (no-op return)
- `Examples/kingsvalley_z88dk/compile_phaseA.sh` — **plain 32K cart** (NO `-pragma-define:MAPPER_KONAMI=1`)
- `Examples/kingsvalley_z88dk/run_phaseA.sh` — `-machine Panasonic_FS-A1GT -carta <rom>` (NO `-romtype` flag, plain cart auto-detect)
- `Examples/kingsvalley_z88dk/.gitignore` 갱신 (`build/`)
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

## 4. v2 PLAN — Phase C (도구 사용 멈칫 진단 + CALSLT fix) ✅ 완료

Phase B 작동 검증 후 사용자가 보고한 새 증상: **도구 입수/사용 시 1-frame
멈칫이 매번 발생**. 원본 SDCC 빌드 (`run_reference.sh`) 에선 안 보이는 현상.

### C-0. 증상 좁히기 — Phase A 도 동일

사용자 검증: `./run_phaseA.sh` (plain 32K, akm_stub, 음악 없음) 에서도 도구
사용 시 동일한 멈칫. → 다음 가설들 일괄 **무관**:

- mplayer/AKM player (Phase A 는 stub)
- ASCII16 매퍼 (Phase A 는 plain 32K)
- mplayer_engine_load 의 RAM-stub LDIR trick (Phase A 는 호출 안 함)
- crt0 pre-init bank-1 shim (Phase A 는 적용 안 함)

→ 멈칫은 z88dk Phase A 와 B 가 **공통으로 가진 무엇** 이 원인. 즉 게임
source 또는 라이브러리 (ubox/spman) port 의 어딘가.

### C-1. variant 진단 빌드 (4개) — 가설 검증

`compile_phaseB_variant.sh {baseline|no_altreg|diei_efx|no_vdp_diei|no_calslt}`
스크립트로 변경 단일 가설 검증.

| Variant | 가설 | 검증 결과 |
|---------|------|----------|
| `baseline` | 비교 기준 | (현재 정식 빌드) |
| `no_altreg` | ubox_isr 의 alt-register set 보존이 +104 cy/ISR overhead | ✗ 동일 — 영향 미미 |
| `diei_efx` | `mplayer_play_effect_p` 의 `di/ei` 부재로 AKM internal state race | ✗ 동일 — race 무관 |
| `no_vdp_diei` | `ubox_vdp_direct.asm` 의 `di/ei` 가 VRAM transfer 중 ISR 차단 | ✗ 동일 — 영향 미미 |
| **`no_calslt`** | `msxbios` 의 CALSLT 가 매 BIOS 호출에 ~220 cy overhead | **✓ 사용자 GT 검증 — 멈칫 ~50% 감소** |

진단 variant 들의 archive: `Examples/kingsvalley_z88dk/variants/`.
- `msxbios_no_calslt.asm` — 정식 fix 의 stub (production)
- `ubox_isr_no_altreg.asm`, `ubox_vdp_direct_no_diei.asm`, `mplayer_play_effect_p_diei.asm` — 학습 자료

### C-2. CALSLT 우회 정식 fix (commit a70b070)

**원인 정량**:

ubox-msx-lib-z88dk 의 모든 BIOS wrapper (`ubox_put_tile`, `ubox_get_tile`,
`ubox_fill_screen` 등 12개) 가 `msxbios` 경유. `msxbios` 는 z88dk classic
msx_crt0.asm 에서 `ld iy, ($FCC0); call $001C (CALSLT); ei; ret` 으로
구현. CALSLT 자체 inter-slot 처리 ~150 cy + setup/restore ~70 cy =
**호출당 ~220 cy overhead**.

SDCC 원본 ubox 는 `jp WRTVRM` 같이 BIOS routine 직접 jp — 0 cy overhead.

도구 사용 frame 의 추가 부담:
```
ubox_put_tile × 5-10회        = 1100-2200 cy (1.7-3.4%)
ubox_get_tile (매 frame 충돌체크) = 660-1100 cy 매 frame 추가
```

Z80 3.58MHz frame budget = 59,667 cy. 도구 사용 시 burst 가 frame budget
임계점 초과 → frame skip = "멈칫" 으로 인식.

**왜 z88dk port 가 CALSLT 사용**: MSX-DOS2 모드 호환 — DOS .COM 에선
page 0 이 user TPA 라 BIOS routine 직접 접근 불가, CALSLT 필수.
ROM cart 모드는 page 0 = BIOS slot 이라 CALSLT 불필요지만 z88dk port 는
단일화 위해 통일 사용.

**Fix**:

`compile_phaseA.sh` + `compile_phaseB.sh` 가 ubox source 를 build dir 로
sed-rewrite:
- `EXTERN msxbios` → `EXTERN msxbios_fast`
- `jp msxbios` → `jp msxbios_fast`
- `call msxbios` → `call msxbios_fast`

그리고 `variants/msxbios_no_calslt.asm` 의 stub 을 link:

```asm
msxbios_fast:
    jp (ix)        ; IX = BIOS routine address, jump directly (~8 cy)
```

호출당 ~193 cy 절약. 사용자 GT 검증: 멈칫 ~50% 감소 + `run_reference.sh`
와 가장 근접한 상태 확인.

**ubox-msx-lib-z88dk 자체는 변경 없음** — 다른 12 예제 (ubox_example_z88dk)
영향 없음. kingsvalley 만 ROM 모드의 fast path 사용.

### C-3. SDCC 빌드 직접 비교 — sccz80 codegen 정량 (잔여 50% 멈칫 설명)

`build_sdcc.sh` 가 같은 game source 를 SDCC 로 빌드 (mplayer stub). 결과
`game/build/kings_sdcc.rom` 과 우리 z88dk Phase A 빌드 byte-level 비교.

**ROM size 비교** (`./run_reference.sh` 와 동일 게임 logic):

| | SDCC | z88dk Phase A | 차이 |
|---|------|--------------|------|
| 32K cart padding 포함 | 32,768 B | 32,768 B | (동일) |
| 실제 사용 영역 (non-0/FF byte) | **28.4 KiB** | **30.3 KiB** | **+1.9 KiB (+6.7%)** |

**모듈별 `_CODE` size 비교** (SDCC `.rel` 의 `_CODE` area vs z88dk `.map` 의 code_compiler):

| 모듈 | SDCC (B) | z88dk (B) | 차이 (%) |
|------|----------|-----------|---------|
| **character.c** | 3,689 | **4,584** | **+24%** ⚠️ 도구 사용 path 핵심 |
| **character_move.c** | 1,150 | **1,501** | **+31%** ⚠️ 캐릭터 이동/방향 |
| game.c | 4,079 | 4,426 | +9% |
| jewel.c | 324 | 385 | +19% |
| trap.c | 446 | 494 | +11% |
| enemy.c | 2,175 | 2,284 | +5% |
| knife.c | 1,563 | 1,646 | +5% |
| main.c | 686 | 697 | +2% |
| gate.c | 1,386 | 1,392 | 0% |
| game_util.c | 1,679 | 1,517 | -10% |
| player.c | 1,825 | 1,730 | -5% |
| pushdoor.c | 408 | 333 | -18% |
| **합** | ~20,000 | ~21,000 | **+~5% 평균** |

**원인**: sccz80 vs SDCC 의 ABI 및 codegen 차이.
- sccz80 의 `uint8_t` arg = 16-bit zero-extended (vs SDCC 의 1-byte packed)
- sccz80 의 caller-cleanup ABI 가 더 큰 prologue/epilogue
- character.c 처럼 복잡한 state machine + 많은 함수 호출이 누적 영향

**결론**: 잔여 ~50% 멈칫은 sccz80 자체의 한계로 수용. hot path 의 직접
asm 작성 외 회피 불가. 우리는 그 단계까지 진행 안 함 (사용자 수용).

### C-4. 최종 검증 (사용자, 2026-05-11)

- `./run_phaseB.sh` (= `build_phaseB/kings.rom`, **정식 빌드**) 가
  `build_phaseB_no_calslt/kings.rom` 와 **byte-identical** (`diff` 0 line).
- 사용자 GT 검증: `./run_phaseB.sh` 가 `./run_reference.sh` (원본 SDCC) 와
  **가장 근접한 동작**. 도구 사용 시 멈칫이 거의 보이지 않음 (남아도 미미).
- Phase B 완료 선언.

---

## 5. 위험 / 미결 — 모두 해결 ✅

| 항목 | 해결 방법 | 위치 |
|---|---|---|
| z88dk plain 32K cart 의 page 2 매핑 | ENASLT 는 매퍼 무관하게 호출됨 — Phase A 빌드 검증 | §7 Iter 17, §2 |
| Game code SDCC-isms 잠복 | sccz80 빌드 시 발견된 것: struct compound init (3개), header collision (2개), VIDEO_MEMORY_ADDRESS 가드 — 모두 patch 적용 | §2 A-2 |
| ROM 사이즈 32 KiB 초과 (Phase B) | ASCII16 64K cart + BANK_02 에 AKM blob | §3 |
| AKM blob 추가 후 사이즈 초과 | 64K cart 로 해결 + mplayer wrapper 5개만 link (32K 부담 절감 시도) | §3 |
| Konami 매퍼가 GT 에서 hang 일으킨 root cause | plain cart 로 회피 (1차 시도 무효화) | §1, §6 Iter 21-26 |
| Phase B 의 ASCII16 multi-bank 부팅 실패 | crt0 pre-init shim 주입 ($4044 dead-code 영역 8 byte) 로 bank 1 mount in advance | §3, akm_bridge_kv.asm |
| 도구 사용 시 멈칫 (사용자 발견) | CALSLT 우회 fix (~50% 감소) + sccz80 codegen 한계로 잔여 수용 | §4 (Phase C) |

**현재 미결 항목 = 없음**. 잔여 ~50% 멈칫은 sccz80 컴파일러 한계로 수용.

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

### Phase B — AKM 음악 ✅ **완료**

1. ☑ `game/src/akm.z80` 에 `ORG #D500` 추가 (BSS_END_tail $D490 위, stack $F380 미만 안전 영역)
2. ☑ akm_bridge_kv.asm — RAM-stub LDIR trick (bank 2 unmount 시 자기 unmap 회피)
3. ☑ main.c 에 `mplayer_engine_load()` 호출 추가
4. ☑ 음악/효과음 GT 검증

### Phase C — 잔여 멈칫 진단 + CALSLT fix ✅ **완료** (commit a70b070)

도구 입수/사용 시 1-frame 멈칫 — Phase A 도 동일 발생 (mplayer/매퍼/AKM race 모두 무관).
4 variant 빌드 검증 후 결정적 원인 확인:

1. ☑ **CALSLT 우회 fix** — ubox-msx-lib-z88dk 의 모든 BIOS wrapper 가 `msxbios` 경유 (CALSLT inter-slot call, ~220 cy/호출 overhead). SDCC 원본은 `jp WRTVRM` 직접 (0 cy). ROM 모드에선 page 0 = BIOS slot 이라 CALSLT 불필요.
   - `compile_phaseA/B.sh` 가 ubox source 의 `msxbios` → `msxbios_fast` sed-rewrite + `variants/msxbios_no_calslt.asm` 의 `jp (ix)` 2-byte stub link
   - 호출당 ~193 cy 절약, 도구 사용 시 1100-2200 cy/frame 절감 (1.7-3.4%)
   - 사용자 GT 검증: 멈칫 ~50% 감소, `run_reference.sh` 와 가장 근접한 상태 확인 ✓
2. ☑ 잔여 ~50% 멈칫 = sccz80 codegen 한계
   - SDCC 직접 빌드 (`build_sdcc.sh` → `kings_sdcc.rom`) vs z88dk Phase A 정량 비교
   - 도구 path 핵심: **character.c +24% (3,689→4,584B), character_move.c +31% (1,150→1,501B)**
   - 평균 game C 코드 size +5-7%, 컴파일러 자체 한계로 수용

### Phase C-Konami — Konami 매퍼 변형 (production) ✅ **완료** (2026-05-11)

prototype_06_MSX_ROM_KONAMI_64K 학습 결과를 kingsvalley 에 적용. ASCII16 (Phase B) 대신 Konami 매퍼 사용. 게임 로직/음악/CALSLT fix 모두 Phase B 와 동일, 매퍼 + bank layout 만 변형. 상세는 §10.

1. ☑ `#pragma bank 1` 으로 module/data 분리:
   - data.c (~7.5 KiB rodata), character.c, enemy.c, gate.c
   - frames_bank.c (신규) — game.c 에서 6 const arrays (`enemy_reborn_frames`/`walk_frames`/...) 분리
2. ☑ `ap.asm` BANK_01 라우팅 — `compile_phaseC.sh` 가 `SECTION code_user → CODE_1` sed-rewrite 후 직접 link (라이브러리 `-lap` 대신). main 16K boundary fit 의 결정타.
3. ☑ `akm_bridge_konami.asm` — Konami dual register ($8000+$A000) 로 BANK_02 swap-in/LDIR/restore (Phase B 의 ASCII16 `$7000` 단일 register 변형). RAM-stub trick 동일.
4. ☑ `compile_phaseC.sh` / `run_phaseC.sh` — `-pragma-define:MAPPER_KONAMI=1`, openMSX `-romtype Konami`. Phase B 의 crt0 pre-init shim 은 불필요 (Konami reset 이 deterministic).
5. ☑ GT 검증: 부팅/타이틀/플레이/음악/효과음 정상 (사용자 확인 2026-05-11).

### 최종 산출물

| 스크립트 | ROM | 용도 |
|---------|-----|------|
| `compile_phaseA.sh` | `build_phaseA/kings.rom` (32K plain, 음악 없음) | minimal viable boot |
| `compile_phaseB.sh` | `build_phaseB/kings.rom` (64K ASCII16, 음악 있음) | **정식 배포** |
| `compile_phaseC.sh` | `build_phaseC/kings.rom` (64K Konami, 음악 있음) | **Konami 변형 production** |
| `build_sdcc.sh` | `game/build/kings_sdcc.rom` (32K plain, SDCC + mplayer stub) | byte-level 비교 baseline |
| `run_phaseA.sh` / `run_phaseB.sh` / `run_phaseC.sh` / `run_sdcc.sh` / `run_reference.sh` | (각 ROM 실행) | GT 검증 |
| `compile_phaseB_variant.sh` + `variants/` | 4개 진단 variant | 가설 검증 archive (재발 시 참조) |

`compile_phaseB.sh` 의 정식 빌드 = `build_phaseB_no_calslt` variant 와 **byte-identical** (사용자 검증 확인). `run_reference.sh` (원본 SDCC) 와 가장 근접한 동작.

---

## 9. 참고 자료

**Reference ROMs (절대 기준)**:
- `Examples/kingsvalley_z88dk/ref_roms/kings_original.rom` — 32 KiB plain cart, GT 호환 검증됨 (pdpdds/ubox_example v1.0 release). `run_reference.sh` 가 자동 download.
- 우리 정식 빌드 `build_phaseB/kings.rom` 도 이 ROM 과 가장 근접한 동작 (사용자 검증 확인).

**Working z88dk references**:
- `Examples/prototype_05_MSX_ROM_MSXDOS/` — Konami 매퍼 32K (main ≤16K), GT 작동
- `Examples/ubox_example_z88dk/examples/{01..12}/` — 16 KiB plain cart 또는 .com, GT/openMSX 작동

**z88dk MSX rom 모드 자료**:
- `/opt/z88dk/lib/target/msx/classic/rom.asm` — startup + 매퍼 conditional defc
- `/opt/z88dk/lib/target/msx/classic/megarom.asm` — BANK_xx section 정의 (multi-bank cart)
- `/opt/z88dk/lib/target/msx/classic/msx_crt0.asm` — `msxbios` CALSLT wrapper 의 정의 (§4 Phase C 의 fix 대상)
- `/opt/z88dk/lib/crt/classic/crt_copy_data_section.inc` — DATA 섹션 복사 (§7 Iter 36/B 의 bank-1 mount shim 이 회피하는 코드)
- `/opt/z88dk/src/appmake/msxrom.c` — appmake bank packing logic

**openMSX 매퍼 source**:
- `Emulator/openMSX/src/memory/RomKonami.cc` — Konami 매퍼 `[$6000, $C000)` 모든 write 가 bank switch trigger (§7 Iter 29)
- `Emulator/openMSX/src/memory/RomAscii16kB.cc` — ASCII16 매퍼 reset 이 bank 0 을 page 1 + page 2 양쪽에 mirror (§ Phase B 의 bank-1 mount shim 이 회피하는 동작)
- `Emulator/openMSX/src/memory/RomPlain.cc` — 32K plain cart 자동 감지 heuristics (§7 Iter 33)

**도구 (외부)**:
- `Disark` — Arkos Tracker 2 의 도구 (linux64 v2.0.1 zip 의 `tools/Disark`). `build_sdcc.sh` 가 사용. 설치: `wget https://www.julien-nevo.com/arkostracker/release/2.0.1/linux64/Arkos%20Tracker%202%20Linux64.zip` → unzip → tools/Disark 를 `/tmp/disarkbin/` 등에 배치.

**메모리 노트 (자동 memory)**:
- `MEMORY.md` → "z88dk Calling Conventions (sccz80)" — sccz80 ABI 의 stack 추출 패턴
- `reference_ubox_z88dk_examples.md` — 12 ubox 예제 포팅 + spman-z88dk 신설
- `reference_kingsvalley_z88dk.md` — kingsvalley 포팅 최종 상태 (Konami 함정 + CALSLT 우회 fix + run_reference 근접 확인)

**Git commits (kingsvalley submodule)**:
- `be18a76` — Phase A/B 포팅 + source patches
- `a70b070` — CALSLT 우회 정식 fix
- `3fad8ff` — SDCC 비교 baseline (build_sdcc.sh / run_sdcc.sh)

**Git commits (workspace)**:
- `9c64d91` — PLAN_MIGRATION v2 (Round-A/B/C, Iter 1-37)
- `6039fcf` — Phase B 완료 (run_reference baseline 도달)

---

## 10. Phase C-Konami — Konami 매퍼 변형 (상세)

### 10.1 배경 / 동기

§1 의 1차 시도 (Konami pragma 적용) 가 GT hang 으로 실패한 본질 원인은
"main 31K linear + sccz80 emit 의 `[$6000, $C000)` write 가 매퍼 trigger
가능" 이었음 (§7 Iter 29). 1차 시도 때는 그 점을 좁히지 못해 단순히
"Konami 매퍼 피하자" 로 결론, Phase B 의 ASCII16 로 우회.

`prototype_06_MSX_ROM_KONAMI_64K` (z88dk + Konami 64K + AKM, 학습용)
작업에서 다음을 확인:
- main code 를 16K ($4000-$7FFF) 안에 fit 시키면 매퍼-trigger write 위험 영역
  ($6000-$BFFF) 중 page 2 영역 ($8000-$BFFF) 에 main code 가 spread 되지
  않음.
- 페이지 2 는 boot default 로 BANK_01 (cart bank 2,3) 이 mount 되므로 거기
  rodata 만 두면 cross-bank call 없이 main code 가 직접 read 가능.

이 패턴을 kingsvalley 에 옮긴 것이 Phase C-Konami.

### 10.2 Layout

```
$4000-$5FFF  bank 0  — main code 의 처음 8K  (Konami fixed)
$6000-$7FFF  bank 1  — main code 의 나머지 8K (Konami initial bank 1)
$8000-$9FFF  bank 2  ┐  BANK_01 = data.c rodata + character.c + enemy.c
$A000-$BFFF  bank 3  ┘            + gate.c + frames_bank.c + ap.asm
                                  (Konami initial bank 2,3, RomKonami::reset
                                   deterministic; pre-init shim 불필요)
$D500-$E2B6  AKM body — mplayer_engine_load() 가 startup 시 BANK_02 (bank 4+5)
              에서 LDIR. body 위치는 Phase B 와 동일.
$F100         RAM stub (22 byte) — mplayer_engine_load 의 bank-swap routine
              (Konami dual register $8000+$A000 trick; bank 4+5 mount → LDIR
              → bank 2+3 복원).
$F380↓        stack
```

cart 영역:
- main bank ($4000-$BFFF, 16K): main code + rodata, 16K 안 fit (`Adding
  main bank 0x00 (1279 bytes free)`)
- BANK_01 (cart bank 2,3, 16K): data + 분리 module + ap.asm. 거의 full 사용
  (~226 byte free)
- BANK_02 (cart bank 4,5, 16K): AKM blob (~3.5K) + bridge. 여유 충분

### 10.3 main 16K fit 을 위한 분리 결정

z88dk megaROM 의 main bank 평가 = code + rodata 모두 16K 안 fit 해야
`Main ROM code is > 16kb` warning 없음. 본래 분리 안 한 상태에서 main 의
끝은 `$9B67` (22.8K, 6.8K over). 점진적으로 다음을 BANK_01 으로 분리:

| 모듈 | 분리 이유 | size 영향 |
|------|----------|----------|
| data.c | 가장 큰 단일 rodata (~7.5K) — map_summary 포함 | -7.5K |
| character.c | code+rodata, 도구 path 핵심이지만 BANK_01 boot default mount 라 cross-call 부담 없음 | -4.4K |
| enemy.c | enemy state machine, 동일 이유 | -2.2K |
| gate.c | `_gate_tiles` rodata (48 byte) 가 main 끝 page-2 영역 침범 | -1.3K |
| frames_bank.c (신규) | game.c 의 const arrays 6개 (`walk_frames` 등) 만 별도 .c 로 떼낸 후 `#pragma bank 1`. game.h 의 extern 선언은 변경 없음 | -16 byte (작지만 main rodata 결정타) |
| `ap.asm` | aplib decompressor (`ap_uncompress` 등 ~140 byte). z88dk 의 `-lap` link 시 `SECTION code_user` 라 main bank 끝 ($8000-$808B) 침범. `compile_phaseC.sh` 가 `SECTION code_user → CODE_1` sed-rewrite 후 직접 link → main 16K fit 의 마지막 picksaw | -140 byte |

분리 안 한 모듈 (main bank 유지): main.c, game.c, game_util.c, character_move.c, knife.c, item.c, jewel.c, player.c, pushdoor.c, trap.c.

⚠️ **BANK_01 매우 빠듯** (~226 byte free). 게임 코드 추가 시 BANK_01 overflow
가능성. 그 경우 BANK_03 신설 또는 위 분리 모듈 일부를 다른 bank 로 재배치
필요.

### 10.4 핵심 신규 파일

- `Examples/kingsvalley_z88dk/akm_bridge_konami.asm` — Phase B 의
  `akm_bridge_kv.asm` 의 Konami 변형. ASCII16 의 단일 `$7000` register
  write 를 Konami 의 `$8000` + `$A000` dual register write 로 변형. 본문
  RAM-stub LDIR trick (bank 2 unmount 시 자기 unmap 회피) 는 동일. AKM
  blob 은 `SECTION RODATA_2` 로 BANK_02 에 INCBIN.
- `Examples/kingsvalley_z88dk/game/src/frames_bank.c` — game.c 에서 떼낸 6개
  const arrays (`enemy_reborn_frames`, `walk_frames`, `stair_frames`,
  `attack_frames`, `knife_frames`, `digging_frames`). `#pragma bank 1`
  guard. game.h 의 extern 선언은 변경 없음 (재컴파일만 영향).
- `Examples/kingsvalley_z88dk/compile_phaseC.sh` — `-pragma-define:MAPPER_KONAMI=1`,
  ap.asm sed-rewrite (`SECTION code_user → CODE_1`) 후 `prepare_ap_banked`
  로 BANK_01 라우팅. `-lap` 명시 제거. Phase B 의 crt0 pre-init shim
  (`compile_phaseB.sh` 의 8-byte $4044 patch) 은 불필요 — Konami reset 이
  deterministic.
- `Examples/kingsvalley_z88dk/run_phaseC.sh` — openMSX `-romtype Konami`.

### 10.5 source 변경 (game/src)

기존 Phase A/B 의 patch (§8 Phase A.2) 위에 추가:

| 파일 | 변경 |
|------|------|
| `game/src/data.c` | 맨 위에 `#ifdef __Z88DK / #pragma bank 1 / #endif` |
| `game/src/character.c` | 동일 |
| `game/src/enemy.c` | 동일 |
| `game/src/gate.c` | 동일 |
| `game/src/game.c` | 6개 frames const array 정의 제거 (extern 만 game.h 에 유지) |
| `game/src/frames_bank.c` | 신규. `#pragma bank 1` + 6개 const arrays 옮김. `<ubox.h>` + `"game.h"` include (uint8_t 정의용) |

모든 `#pragma bank 1` 는 `__Z88DK` guard — SDCC build 시 영향 없음.

### 10.6 Phase B vs Phase C-Konami 비교

| 항목 | Phase B (ASCII16) | Phase C-Konami |
|------|-------------------|----------------|
| 매퍼 | ASCII16 (`$7000` register) | Konami (`$8000`+`$A000` dual register) |
| cart 크기 | 64 KiB | 64 KiB |
| 음악 | AKM (`mplayer_engine_load` LDIR from BANK_02) | 동일 |
| main 크기 | linear 30K (cart bank 0-1 + 0-1 mirror) | **strict 16K** (cart bank 0+1 page 1) |
| BANK_01 | data.c 만 (rodata ~7.5K) | data + character + enemy + gate + frames + ap (~14.1K) |
| crt0 pre-init shim | **필요** (ASCII16 reset 의 page1=page2 mirror 회피) | **불필요** (Konami reset deterministic) |
| BANK_02 | AKM blob | 동일 |
| CALSLT fix | 적용 | 적용 (동일 sed-rewrite + msxbios_fast stub) |
| GT 동작 | run_reference 와 가장 근접 | 부팅/플레이/음악 정상 (사용자 확인) |

Phase B 와 Phase C-Konami 의 게임 코드 자체는 동일. 차이는 cart layout
+ 매퍼 + bank 분리 방식만.

### 10.7 prototype_06 에서 학습한 사항 (메모)

- `prototype_06_MSX_ROM_KONAMI_64K` 는 workspace gitignore 정책에 따라
  local-only (commit 안 됨). 학습 자료로만 보존.
- 핵심 학습:
  1. **ubox 입력은 active-LOW**. `ubox_select_ctl()` 반환 `0xFF` = idle, 비트
     CLEAR = pressed. `if (ctl & MASK)` 는 반대 의미. `MEMORY.md` 의
     `feedback_ubox_input_active_low.md` 에 별도 기록.
  2. **secondary_colors 배열 size mismatch** — LDIR 64 byte 에 `[8]` 배열
     사용은 stack/BSS 인접 데이터 덮어쓰기. 무한 stripe screen 의 흔한 원인.
  3. **`#pragma bank N`** 은 sccz80 에서 file 단위로 code + rodata 모두를
     `CODE_N` + `RODATA_1` (N=1 일 때) 으로 라우팅. partial 분리 (rodata 만)
     불가 → const arrays 만 분리하려면 별도 .c 파일로 떼야 함.
  4. **Konami `RomKonami::reset`** 가 deterministic 하게 bank 2,3 을
     `$8000-$BFFF` 에 mount. ASCII16 의 `RomAscii16kB::reset` 처럼 page1=page2
     mirror 안 함 → pre-init shim 불필요.
  5. **aplib (`-lap`)** 은 `SECTION code_user` 라 main bank 끝에 link. 16K
     boundary 침범 회피 위해 `SECTION CODE_N` 으로 sed-rewrite 후 직접 link.

### 10.8 향후 작업 / 미결

- BANK_01 의 free space 226 byte. 게임 로직 추가 (예: 새 enemy 타입, 추가
  도구) 시 overflow 가능. 그 시점에 BANK_03 신설 (bank 6+7 = 16K) 권장.
  Konami 64K cart 의 4 bank slot 중 1 개 (BANK_03) 가 미사용 상태로 남아
  있음.
- Phase B vs Phase C-Konami 의 hitch 비교는 사용자가 GT 에서 별도 평가.
  CALSLT fix 적용 후 두 빌드 모두 `run_reference` 근접하므로 본질 차이는
  미미할 가능성. 정량 비교 필요시 진단 variant 빌드 추가 가능.
- `crt0` pre-init shim 불필요한 점, Konami 가 boot bank 정합성에서 ASCII16
  보다 다루기 쉬운 점은 다른 z88dk MSX 64K cart 작업에도 적용 가능. 비슷한
  레퍼런스 작업 시 Phase C-Konami 의 `compile_phaseC.sh` 가 참고 자료.
