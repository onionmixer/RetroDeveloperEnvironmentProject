# PLAN — ubox z88dk examples 포팅 (✅ 완료)

`Library/MSX/ubox-msx-lib-z88dk` (z88dk 포트, MSX-DOS2 BIOS fix 포함) 으로
`Examples/ubox_example_z88dk/examples/` 12 예제를 ROM 카트리지 + MSX-DOS2 `.COM`
양쪽 모드로 빌드. **12/12 × ROM/DOS = 24/24 빌드 성공 + 사용자 GT 검증 완료**.

작성 2026-04-29 · 최종 2026-05-10.

---

## 1. 최종 결과 매트릭스

| Dir | TARGET_BASE | DOS_NAME | ROM | DOS COM | 추가 라이브러리 / 우회 |
|-----|------------|----------|------|---------|---------------------|
| 01_hello         | `01_hello`         | `HELLO`    | 16 KiB | ~5.5 KiB  | (ubox만) |
| 02_consolehello  | `02_consolehello`  | `CONHELLO` | 16 KiB | ~2.9 KiB  | (콘솔 BIOS) |
| 03_timer         | `03_timer`         | `TIMER`    | 16 KiB | ~6.5 KiB  | (ubox만) |
| 04_clibrary      | `04_clibrary`      | `CLIB`     | 16 KiB | ~6.7 KiB  | `src/printf.c` 제거 (z88dk 빌트인) |
| 05_music         | `05_music`         | `MUSIC`    | 16 KiB | ~9.0 KiB  | **mplayer-z88dk (real AKM)** |
| 06_sound         | `06_sound`         | `SOUND`    | 16 KiB | ~7.6 KiB  | **mplayer-z88dk (real AKM)** |
| 07_snakebyte     | `07_snakebyte`     | `SNAKE`    | 16 KiB | ~13 KiB   | malloc heap pragma |
| 08_socoban       | `08_socoban`       | `SOKOBAN`  | 16 KiB | ~10.7 KiB | game.c/util.c |
| 09_tetris        | `09_tetris`        | `TETRIS`   | 16 KiB | ~9.5 KiB  | util.c `#ifdef MSXDOS` ISR 가드 |
| 10_breakout      | `10_breakout`      | `BREAKOUT` | 16 KiB | ~9.3 KiB  | **spman-z88dk** + util.c 가드 |
| 11_log           | `11_log`           | `LOG`      | 16 KiB | ~11 KiB   | malloc heap pragma |
| 12_debugger      | `12_debugger`      | `DEBUGGER` | 16 KiB | ~5.6 KiB  | (ubox만) |

사용자 emulator 검증: 01_hello, 10_breakout, 05_music, 06_sound 부팅·소리·키 입력 OK.

---

## 2. Canonical ZCCFLAGS

```bash
# ROM (plain 16K cartridge — 32K 미만이라 Konami mapper 不要)
+msx -subtype=rom -compiler=sccz80 -SO2 -create-app
-I<UBOX_INC> -I<example/src> -L<UBOX_LIB> -lubox

# DOS (.COM)
+msx -subtype=msxdos2 -compiler=sccz80 -SO2 -create-app
-I<UBOX_INC> -I<example/src> -L<UBOX_LIB> -lubox
-DMSXDOS
```

`-SO2` (NOT `-O3`), `-DMSXDOS` (NOT `-DBUILD_DOS`).

### compile.sh 템플릿 (01_hello, 다른 예제도 동일 구조)

```bash
#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
TARGET_BASE="01_hello"
ZCC="${ZCC:-/opt/z88dk/bin/zcc}"
UBOX_DIR="$PROJECT_ROOT/Library/MSX/ubox-msx-lib-z88dk"
[[ -f "$UBOX_DIR/lib/ubox.lib" ]] || make -C "$UBOX_DIR"
ZCCFLAGS="+msx -subtype=rom -compiler=sccz80 -SO2 -create-app"
ZCCFLAGS="$ZCCFLAGS -I$UBOX_DIR/include -I$SCRIPT_DIR/src"
ZCCFLAGS="$ZCCFLAGS -L$UBOX_DIR/lib -lubox"
mkdir -p "$BUILD_DIR"
(cd "$SCRIPT_DIR" && "$ZCC" $ZCCFLAGS -m -o "$BUILD_DIR/$TARGET_BASE" src/main.c)
```

`run_openmsx_dos.sh` 는 `/tmp/<base>_dos/<TARGET>_<timestamp>.dsk` 에 부팅 디스크
사본 + COM 주입을 runtime staging (prototype_05 패턴 — stale `.dsk` 누적 방지).

---

## 3. ROM / DOS 분기 패턴

**큰 변형** = `<base>_dos.c` 별도 파일 (entry signature 또는 큰 메모리/데이터 차이).
**작은 분기** = `#ifdef MSXDOS` (1~2 줄).

### ISR 가드 (09_tetris, 10_breakout 처럼 ISR 호출이 보조 파일 안에 있는 경우)

```c
void InitEnvironment() {
#ifndef MSXDOS
    ubox_init_isr(2);
#endif
    /* ... */
#ifndef MSXDOS
    ubox_set_user_isr(my_isr);
#endif
}
```

이유: HTIMI 훅 (`$FD9F`) 은 MSX-DOS2 의 inter-slot context 에서 page 0 = BIOS ROM
매핑이라 TPA 의 ISR 코드 unreachable. ROM 전용.

### DOS-side `ubox_wait()` override

라이브러리 `_ubox_wait` 는 HTIMI 의존이라 DOS 에서 fail. `main_dos.c` 안에 user
정의를 두면 z88dk 링커가 우선 채택:

```c
void ubox_wait(void) { __asm halt __endasm; }   /* 1 frame */
```

map 파일로 검증: ROM map → `_ubox_wait` 가 `src/ubox/ubox_wait.asm` 출처;
DOS map → `src/main_dos.c::ubox_wait` 출처.

### 검증 체크리스트 (각 예제)

- ROM: 16,384 또는 32,768 bytes; `file build/<name>.rom` → `MSX ROM, init=0x4010`
- COM: `build_dos/<DOS_NAME>` 존재; `file` → `DOS executable (COM)`
- map 의 `_ubox_wait` 심볼이 ROM ↔ DOS 다른 출처 확인 (위)

---

## 4. 적용된 우회

- **`ubox_wait_vsync()` 매크로 incompatibility** — SDCC 의
  `do {__asm halt __endasm;} while(0)` 매크로가 sccz80 preprocessor 에서 fail (한
  줄로 collapse). Phase 7 에서 라이브러리를 함수화 (`_ubox_wait_vsync: halt; ret`)
  로 본질 fix.
- **z88dk classic malloc heap (07_snakebyte, 11_log)** — `_heap` undefined →
  ZCCFLAGS 에 `-pragma-define:CLIB_MALLOC_HEAP_SIZE=4096`.
- **04_clibrary 의 `src/printf.c`** — nested function pointer typedef 가 sccz80
  거부 → SRCS 에서 제외, `<stdio.h>` 표준 printf 사용.
- **DOS-mode ISR 가드** — `util.c` 등 보조 파일의 `ubox_init_isr` / `ubox_set_user_isr`
  호출에 `#ifndef MSXDOS` (09_tetris, 10_breakout).

---

## 5. spman-z88dk (Phase 3-A, 10_breakout 만 사용)

`Library/MSX/spman-z88dk/` — Juan J. Martinez 의 spman sprite manager z88dk 포트.
단일 C 파일, 직접 source inclusion 방식. 10_breakout 의 `compile.sh` 가 spman.c
를 link.

`Library/MSX/spman-z88dk/README.md` 참조.

---

## 6. mplayer-z88dk REAL (Arkos 2 AKM, 05_music + 06_sound)

`Library/MSX/mplayer-z88dk/` — 실제 Arkos 2 AKM player z88dk 포팅 (stub 아님).
**Disark 부재 우회** — rasm 1.20 in-tree 빌드 + INCBIN 전략.

### 빌드 파이프라인

1. **rasm 1.20** (`Examples/ubox_example_z88dk/tools/rasm/rasm_v0120.c`, 단일 C 파일) 으로 ORG-localized AKM player + 곡 데이터를 1.7~3.3 KiB 바이너리 blob + 심볼 파일 (`akm.sym`) 생성.
2. **z80asm bridge (`akm_bridge.asm`)** 가 `BINARY "akm.bin"` 으로 INCBIN + 12-byte LDIR loader (`mplayer_engine_load`).
3. **per-example 자동 심볼 추출** — `compile.sh` 가 `akm.sym` 파싱해 `build/song_bindings.asm` 생성 (DEFC `_SONG`, `_EFFECTS`, `_PLY_AKM_INIT`, `_PLY_AKM_PLAY` 등 7 entry).
4. **8개 mplayer wrapper** (SDCC asasm → z80asm 변환). `Library/MSX/mplayer-z88dk/src/wrappers/*.asm`.

### 메모리 배치 ($C200)

```
$C000-$C06E   z88dk BSS (ubox_isr_wait_ticks/tick, _ubox_tick, ubox_usr_isr)
$C06E-$C200   gap (~414 bytes)
$C200-$CF02   ★ AKM player + 곡/효과음 (LDIR 대상)
$CF02-$F380   free RAM (스택 여유 ~5 KiB)
$F380+        BIOS 시스템 변수
```

`$C200` = page 3 (`$C000-$FFFF`) 안 — MSX 전 모델에서 ROM/DOS 양쪽 RAM. 공통 사용.

### 핵심 함정 (포팅 중 발견, 모두 해결됨)

| # | 함정 | 해결 |
|---|-----|------|
| 1 | 초기 시도 `AKM_BASE=$A000` 묵음 — z88dk MSX cart CRT 가 `ENASLT` 로 page 2 (`$8000-$BFFF`) 를 cart slot 으로 매핑 (`rom.asm:65`). LDIR 쓰기 dropped | `AKM_BASE` 를 page 3 `$C200` 으로. ROM/DOS 둘 다 RAM |
| 2 | sccz80 vs SDCC calling convention mismatch — SDCC 는 8-bit 인자 1 byte right-to-left push, **sccz80 은 word(2 byte, zero-extend) left-to-right** | `mplayer_init` / `mplayer_play_effect` / `mplayer_play_effect_p` 3 wrapper 의 stack offset 수정 |
| 3 | `PLY_AKM_Rom=1` + `PLY_AKM_ROM_Buffer=#c000` → z88dk BSS 와 mutual destruction | `PLY_AKM_Rom` 비활성화 → AKM 가 RAM mode (self-modifying); LDIR 로 RAM 에 복사하므로 self-modify 가능 |
| 4 | AKM entry-point 주소 하드코딩 (`PLY_AKM_INIT=$A12A`) → 레이아웃 변경 시 어긋남 | `compile.sh` 가 매 빌드마다 `akm.sym` 에서 7 entry 자동 추출 → `song_bindings.asm` 생성 |
| 5 | DOS 박자 절반 — ROM ISR 매 VBlank `mplayer_play()` 호출 vs DOS `ubox_wait()` (= 2 halt) 후 호출 = 25/30 Hz | DOS main loop 재구조: 매 `halt` 마다 `mplayer_play()` 호출 + 키 입력은 2 frame 당 1회 |

### sccz80 wrapper 패턴 (참고)

```asm
; mplayer_init(uint8_t* song, uint8_t sub_song)
; sccz80 stack: sp+0..1 ret, sp+2..3 last arg (sub_song), sp+4..5 first arg (song)
_mplayer_init:
    ld ix, 2
    add ix, sp
    ld a, (ix+0)        ; sub_song (last pushed, low byte)
    ld l, (ix+2)        ; song lo
    ld h, (ix+3)        ; song hi
    ; ...
```

fastcall wrapper (`init_effects`, `is_sound_effect_on`, `stop_effect_channel`)
와 무인자 wrapper (`play`, `stop`) 는 영향 없음.

---

## 7. Phase 7 — ubox-msx-lib-z88dk 라이브러리 품질 개선

본 포팅 작업 중 발견된 라이브러리 자체 issue 4 건을 본질적으로 fix (2026-05-10):

| 변경 | 위치 |
|------|------|
| `ubox_wait_vsync` 매크로 → 함수화 (`_ubox_wait_vsync: halt; ret`). sccz80 preprocessor 호환 | `include/ubox.h` + `src/ubox/ubox_wait_vsync.asm` (신규) |
| ISR alternate register set (`AF'/BC'/DE'/HL'`) 보존. AKM 처럼 EXX/EX AF, AF' 쓰는 user_isr 가 main flow 의 alt set 을 corrupt 하던 잠복 버그 fix. 오버헤드 ~60 cycles | `src/ubox/ubox_isr.asm` |
| `ubox_init_isr(wait_ticks)` docstring — `wait_ticks` 가 `ubox_wait()` 만 throttle, user_isr 는 항상 매 VBlank fire 명시 | `include/ubox.h` |
| `ubox_wait` DOS-side override 패턴 docstring | `include/ubox.h` |

다운스트림 정리: `Library/MSX/spman-z88dk/src/spman/spman.c` 두 곳의 inline-asm
우회를 정상 `ubox_wait_vsync()` 호출로 복원.

회귀 검증: 12 예제 × ROM/DOS = 24/24 빌드 성공.

---

## 8. 학습된 교훈

향후 비슷한 z88dk + retro 작업에 참고:

1. **z88dk MSX cart CRT 가 page 2 를 cart slot 으로 매핑** — `$8000-$BFFF` 의 절반(`$A000+`) 은 16K cart 에서 unmapped. RAM 가정 코드는 page 3 (`$C000+`) 사용.
2. **sccz80 ≠ SDCC calling convention** — sccz80 은 8-bit 인자도 word zero-extend + left-to-right push. SDCC asm wrapper 그대로 가져오면 인자 매핑 어긋남.
3. **HTIMI 훅 (`$FD9F`) 은 ROM 전용** — MSX-DOS2 에선 page 0 = BIOS ROM 매핑이라 TPA 의 ISR 코드 unreachable.
4. **z88dk 링커는 user 정의를 라이브러리 위에 우선 채택** — DOS-mode `ubox_wait()` 같은 override 가 자연스럽게 동작.
5. **AKM player 는 매 VBlank tick 필수** — ISR 또는 polling 어느 쪽이든 50/60 Hz 미만이면 음악 박자가 비례 느려짐. `Examples/prototype_05` 의 ISR-driven 적용 결과 참조 (DEVELOPER_MSX_HOWTO.md §3.5.4).
6. **ISR 은 alt register set 보존 필수** — z80 표준 컨벤션. AKM 처럼 EXX 쓰는 라이브러리가 user_isr 로 들어오면 main flow alt set 을 corrupt.
7. **Disark 부재 우회** — `BINARY` directive INCBIN + LDIR-to-RAM + 심볼 자동 추출하면 SDCC asm 출력 단계 불필요.
8. **rasm 출력 entry 주소는 빌드 옵션마다 변함** — 하드코딩 금지. `.sym` 매 빌드 파싱으로 자동 추출.

---

## 9. 참고 자료

**Canonical reference**:
- `Examples/prototype_05_MSX_ROM_MSXDOS/` — z88dk + ubox + ROM/DOS dual-build 의 검증된 4-script 템플릿 원형.

**라이브러리**:
- `Library/MSX/ubox-msx-lib-z88dk/` — z88dk ubox 포트 (DOS2 BIOS fix + Phase 7 의 4 건 개선)
- `Library/MSX/spman-z88dk/` — z88dk spman 포트
- `Library/MSX/mplayer-z88dk/` — z88dk mplayer 포트 = real Arkos 2 AKM
- `Library/MSX/mplayer-z88dk-stub/` — silent placeholder (real 포팅 회귀 시 fallback)

**예제**:
- `Examples/ubox_example_z88dk/README_Z88DK_PORT.md` — per-example 상태표 + DOS_NAME 매핑 + 빌드/실행 가이드
- `Examples/ubox_example_z88dk/run_openmsx_menu.sh` — 통합 launcher (인덱스/별칭/디렉터리명 모두 키)

**관련 문서**:
- `DEVELOPER_MSX_HOWTO.md` §3.4 (12 예제 사용법), §3.5 (megaROM + ISR-driven 음악 패턴)
- `Examples/kingsvalley_z88dk/PLAN_MIGRATION_KINGSVALLY.md` (z88dk + Konami/ASCII16 마이그레이션 사례)
