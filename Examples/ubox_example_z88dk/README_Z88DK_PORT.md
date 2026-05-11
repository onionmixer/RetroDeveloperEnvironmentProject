# ubox_example_z88dk — z88dk port status

상위 `Examples/ubox_example_z88dk/` 는 원래 SDCC 기반 ubox-msx-lib 예제이지만,
본 워크스페이스에서는 **z88dk(`/opt/z88dk/bin/zcc` + sccz80)** 와
**`Library/MSX/ubox-msx-lib-z88dk/`** (z88dk 포트 + MSX-DOS2 BIOS fix)
를 사용해서 **각 예제를 ROM과 MSX-DOS2 양쪽 모드로 동시 빌드**하도록
포팅했다. 자세한 작업 계획·근거는 루트 `PLAN_UBOX_EXAMPLES.md` 참조.

작성일: 2026-04-29 (Phase 1-3 완료 시점)

## 포팅 상태 (12 예제)

| Dir | TARGET_BASE | DOS_NAME | ROM | DOS COM | 비고 |
|-----|------------|----------|------|---------|------|
| 01_hello         | `01_hello`         | `HELLO`    | 16 KiB | 5,506 B  | pilot, ubox만 |
| 02_consolehello  | `02_consolehello`  | `CONHELLO` | 16 KiB | 2,892 B  | ubox 미사용 (콘솔 BIOS) |
| 03_timer         | `03_timer`         | `TIMER`    | 16 KiB | 6,508 B  | user_isr → halt-poll |
| 04_clibrary      | `04_clibrary`      | `CLIB`     | 16 KiB | 6,742 B  | 자체 printf.c 제거 (z88dk 빌트인 사용) |
| 05_music         | `05_music`         | `MUSIC`    | 16 KiB | 8,928 B  | **mplayer-z88dk REAL** (AKM @$A000) |
| 06_sound         | `06_sound`         | `SOUND`    | 16 KiB | 7,568 B  | **mplayer-z88dk REAL** (AKM @$A000) |
| 07_snakebyte     | `07_snakebyte`     | `SNAKE`    | 16 KiB | 13,117 B | malloc heap pragma |
| 08_socoban       | `08_socoban`       | `SOKOBAN`  | 16 KiB | 10,674 B | game.c/util.c 멀티파일 |
| 09_tetris        | `09_tetris`        | `TETRIS`   | 16 KiB | 9,550 B  | **util.c `#ifdef MSXDOS`** ISR 가드 |
| 10_breakout      | `10_breakout`      | `BREAKOUT` | 16 KiB | 9,346 B  | **spman 의존** + util.c 가드 |
| 11_log           | `11_log`           | `LOG`      | 16 KiB | 10,970 B | malloc heap pragma |
| 12_debugger      | `12_debugger`      | `DEBUGGER` | 16 KiB | 5,584 B  | ubox만 |

**12/12 빌드 완료**. 01_hello, 10_breakout 사용자 emulator 검증 완료.
**05/06은 `Library/MSX/mplayer-z88dk/` (real Arkos 2 AKM port)로 link되어
음악/효과음이 출력됩니다** (build-level 검증 OK; runtime 사용자 검증 대기).
포팅 전략 + 자세한 메모리 배치: `Library/MSX/mplayer-z88dk/README.md`.

(이전의 `Library/MSX/mplayer-z88dk-stub/`는 silent placeholder로 두며,
real 포팅 회귀 시 fallback 옵션으로 활용 가능.)

## 디렉터리 별 신규 산출물 (예제당)

```
examples/<NN>_<name>/
├── src/
│   ├── main.c              # ROM 변형 (대부분 upstream 그대로)
│   ├── main_dos.c          # DOS 변형 (NEW — int main + ubox_wait halt-override)
│   ├── (기타 .c, .h)        # 09/10 의 util.c 등에 #ifdef MSXDOS 가드
│   └── ...
├── compile.sh              # NEW — ROM 빌드 (z88dk +msx -subtype=rom)
├── compile_dos.sh          # NEW — DOS 빌드 (-subtype=msxdos2 -DMSXDOS)
├── run_openmsx.sh          # NEW — cartridge 실행
├── run_openmsx_dos.sh      # NEW — /tmp 작업 디스크 staging + MSX-DOS2 부팅
└── build/, build_dos/      # gitignore (Examples/ubox_example_z88dk/.gitignore에 등록)
```

## 빌드/실행 한 줄 가이드

```bash
cd Examples/ubox_example_z88dk/examples/01_hello
./compile.sh build       # ROM (16 KiB plain cartridge → build/01_hello.rom)
./compile_dos.sh build   # DOS COM → build_dos/01_hello
./run_openmsx.sh         # cartridge 부팅
./run_openmsx_dos.sh     # MSX-DOS2 부팅 후 HELLO 입력
```

### 통합 메뉴 launcher

```bash
cd Examples/ubox_example_z88dk
./run_openmsx_menu.sh                # 12개 중 인터랙티브 선택 + ROM/DOS 모드
./run_openmsx_menu.sh 09 rom         # 직접: 9번(tetris) ROM
./run_openmsx_menu.sh tetris dos     # 짧은 이름 + DOS 모드
./run_openmsx_menu.sh 10_breakout dos
```

빌드 안 된 산출물은 자동 빌드 후 실행. 인덱스/별칭/디렉터리명 모두 키로 사용 가능.

## 핵심 패턴 (prototype_05 사례 차용)

### Canonical ZCCFLAGS

```bash
# ROM
+msx -subtype=rom -compiler=sccz80 -SO2 -create-app
-I<ubox-msx-lib-z88dk/include> -I<example/src>
-L<ubox-msx-lib-z88dk/lib> -lubox

# DOS
+msx -subtype=msxdos2 -compiler=sccz80 -SO2 -create-app
-I<ubox-msx-lib-z88dk/include> -I<example/src>
-L<ubox-msx-lib-z88dk/lib> -lubox
-DMSXDOS                                  # ROM/DOS 분기 매크로
```

`Konami mapper(-pragma-define:MAPPER_KONAMI=1)` 는 32 KiB 초과 ROM에만
필요. 본 12개 예제는 모두 16 KiB 이내라 plain cartridge.

### DOS 측 `ubox_wait()` halt-override

라이브러리 `ubox_wait` (`Library/MSX/ubox-msx-lib-z88dk/src/ubox/ubox_wait.asm`)
는 HTIMI($FD9F) 훅 의존이라 MSX-DOS2 환경에서 fail. `main_dos.c` 또는
DOS-side .c에 동명 함수를 정의하면 z88dk 링커가 사용자 정의를 우선 채택:

```c
void ubox_wait(void)
{
    __asm
    halt
    halt
    __endasm;
}
```

map 파일로 검증 가능: ROM build → `_ubox_wait` 가 `src/ubox/ubox_wait.asm`
출처. DOS build → `src/main_dos.c::ubox_wait` 출처.

### `#ifndef MSXDOS` 패턴

util.c 등 ROM/DOS 양쪽에서 공유하는 파일에 ISR 호출 있으면 가드:

```c
void InitEnvironment()
{
#ifndef MSXDOS
    ubox_init_isr(2);
#endif
    /* ... */
#ifndef MSXDOS
    ubox_set_user_isr(my_isr);
#endif
    /* ... */
}
```

## 인프라 패치 — 본 포팅 중 적용된 우회

### `CLIB_MALLOC_HEAP_SIZE` pragma (07_snakebyte, 11_log)

z88dk의 `malloc-classic` 은 `_heap` 심볼을 사용자가 정의해야 link 가능.
간단한 우회로 ZCCFLAGS에 추가:
```bash
ZCCFLAGS="$ZCCFLAGS -pragma-define:CLIB_MALLOC_HEAP_SIZE=4096"
```
4 KiB heap 자동 셋업.

### 04_clibrary `printf.c` 제거

upstream `src/printf.c` 는 nested function pointer typedef를 sccz80 파서가
거부함 (`error: Missing token, expecting ( got v`). z88dk 표준 라이브러리가
`printf`/`sprintf` 제공하므로 src/printf.c 를 SRCS에서 제외, `main.c` 의
`#include "printf.h"` 를 `#include <stdio.h>` 로 교체.

### `ubox_wait_vsync()` 매크로 인라인화 (spman.c)

z88dk 포트 `ubox.h` 의 `ubox_wait_vsync()` 매크로 정의가 sccz80 incompatible:
```c
#define ubox_wait_vsync() do { __asm halt __endasm; } while(0)
```
은 컴파일 시 `Unknown symbol: __asm` 발생. 우회로 spman.c 의 매크로 호출
2곳 (L103/116) 을 직접 inline asm 으로 치환:
```c
__asm
halt
__endasm;
```
포팅 카피 `Library/MSX/spman-z88dk/src/spman/spman.c` 에만 적용 (upstream
미수정). 향후 `ubox-msx-lib-z88dk` 의 매크로 자체를 sccz80-호환 형태로
수정하는 게 본질적 fix.

## 신규 라이브러리

### `Library/MSX/spman-z88dk/` (10_breakout)
sprite manager 의존성. `Library/MSX/ubox-msx-lib-z88dk` 와 달리 단일 C
파일이라 별도 `.lib` 빌드 안 하고 **소스 직접 inclusion** 방식 사용
(사유: sccz80 standalone `-c` 모드가 ubox_wait_vsync 매크로 expansion에서
fail). 자세한 사용법은 `Library/MSX/spman-z88dk/README.md`.

### `Library/MSX/mplayer-z88dk/` (05_music, 06_sound) — REAL 포팅
**실제 Arkos 2 AKM player 포팅**. 8개 SDCC asasm wrappers를 z80asm
syntax로 변환 + AKM player를 in-tree `rasm` (1.20)으로 ORG=$A000에서
어셈블 → bridge.asm `BINARY` directive로 INCBIN → 런타임에
`mplayer_engine_load()` 가 LDIR로 RAM page 2 ($A000)에 복사하면 AKM
내부 절대주소 점프가 정상 동작. Disark 부재 우회. 자세한 메모리
배치 + 빌드 파이프라인: `Library/MSX/mplayer-z88dk/README.md`.

### `Library/MSX/mplayer-z88dk-stub/` (legacy fallback)
이전에 사용된 silent no-op stub. real 포팅 회귀 시 fallback 옵션으로
보존. 음악 없는 빌드 매트릭스 확보 용도.

## 보류된 작업

- **mplayer (Arkos 2 AKM player)**: ~2000+줄 rasm-매크로 의존 z80asm.
  z88dk z80asm 매크로 시스템으로의 재작성이 multi-iteration 작업 단위.
  본 PLAN의 Phase 3-C/D 로 명시.
- **`ubox_wait_vsync` 매크로 본질적 fix**: 현재는 spman.c 카피본에서만
  우회. `Library/MSX/ubox-msx-lib-z88dk/include/ubox.h` 자체의 매크로
  정의를 sccz80-호환으로 교정하는 후속 작업 권장.
- **컴파일 경고 잔존**: `char *` → `const uint8_t *` 암시 변환 (ubox 함수
  시그니처 const 강화 영향). 모든 예제에서 무해, 향후 cleanup 후보.

## 참고

- `PLAN_UBOX_EXAMPLES.md` (루트) — 작업 계획 + 위험 분석
- `Examples/prototype_05_MSX_ROM_MSXDOS/{compile,compile_dos,run_openmsx,run_openmsx_dos}.sh` — canonical reference
- `Library/MSX/ubox-msx-lib-z88dk/` — z88dk ubox 포트
- `Library/MSX/spman-z88dk/` — z88dk spman 포트 (이번 작업)
- `DEVELOPER_MSX_HOWTO.md` §3 (z88dk 경로)
