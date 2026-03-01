# prototype_02_MSX_ROM_MSXDOS

z88dk + ubox-msx-lib-z88dk를 사용한 MSX SCREEN 2 타일 기반 로그라이크 프로토타입.
하나의 소스 코드 베이스에서 **ROM 카트리지**와 **MSX-DOS2 .COM** 두 가지 형태로 빌드한다.

## 빌드 및 실행

### 사전 요구사항

- z88dk (`/opt/z88dk/bin/zcc`)
- ubox-msx-lib-z88dk (`Library/MSX/ubox-msx-lib-z88dk/`) — 자동 빌드됨
- rdedisktool (DOS 디스크 이미지 생성용)
- openMSX (실행용)

### ROM 버전

```bash
./compile.sh all          # clean → build → verify
./run_openmsx.sh          # openMSX에서 ROM 카트리지 실행
```

- 출력: `build/PROTO02.rom` (32KB, AB 시그니처)
- zcc 플래그: `+msx -subtype=rom`
- 소스: `main.c`, `render.c` 사용

### MSX-DOS2 버전

```bash
./compile_dos.sh all      # clean → build (data gen + compile) → disk → verify
./run_openmsx_dos.sh      # MSX-DOS2 부트디스크에서 실행
```

- 출력: `build_dos/PROTO02.COM` (~17KB) + `build_dos/PROTO02.dsk`
- zcc 플래그: `+msx -subtype=msxdos2 -DMSXDOS`
- 소스: `main_dos.c`, `render_dos.c`, `room_data_dos.c` 사용 (나머지는 공유)
- 데이터 파일: `ROOM00-02` (룸 그리드, 720B/개), `TILES0` (타일셋, 144B) → 디스크에 분리
- 실행: MSX-DOS 프롬프트에서 `PROTO02` 입력

---

## 소스 파일 구조

### 공유 소스 (ROM / DOS 공통)

| 파일 | 역할 |
|------|------|
| `logic.c / logic.h` | 게임 로직 (이동, 충돌, 방 전환) |
| `input.c / input.h` | 키보드 입력 (WASD, 키 리피트) |
| `help.c / help.h` | 도움말 화면 |
| `room_data.c / room_data.h` | 방 메타데이터, 문/계단 연결 (ROM용: 그리드 포함) |
| `monster.c / monster.h` | 몬스터 AI (IDLE/CHASE/RETURNING) |
| `engine.h` | GameState 구조체, 상수 |
| `tiles.h` | 타일 패턴/색상 데이터 |
| `font.h` | 폰트 패턴/색상 데이터 |
| `render.h` | 렌더링 함수 선언 (공통 인터페이스) |

### 버전별 분기 소스

| ROM 버전 | DOS 버전 | 차이점 |
|----------|----------|--------|
| `main.c` | `main_dos.c` | `void main()` vs `int main()` + `return 0` |
| `render.c` | `render_dos.c` | ISR 훅, ubox_wait, cleanup, 그리드 접근 방식 |
| `room_data.c` | `room_data_dos.c` | ROM: 그리드 인라인 / DOS: 디스크 로드 (`grid_load_room()`) |

### 빌드 도구

| 파일 | 용도 |
|------|------|
| `tools/gen_room_bin.py` | `room_data.c`에서 룸 그리드 추출 → `ROOM00-02` 바이너리 |
| `tools/gen_tileset_msx.py` | `tiles.h`에서 맵 타일 데이터 추출 → `TILES0` 바이너리 |

DOS 빌드 시 `compile_dos.sh`가 자동 실행. 생성된 파일은 디스크 이미지에 포함된다.

---

## ROM vs DOS: 핵심 기술 차이

### MSX 메모리 맵 비교

```
ROM 빌드:
  Page 0 ($0000-$3FFF): BIOS ROM ← 항상 매핑
  Page 1 ($4000-$7FFF): 사용자 ROM (코드/데이터)
  Page 2 ($8000-$BFFF): 사용자 ROM (코드/데이터)
  Page 3 ($C000-$FFFF): RAM (시스템 영역, 스택)

MSX-DOS2 빌드:
  Page 0 ($0000-$3FFF): RAM (TPA — 사용자 코드, $0100부터) ← BIOS 아님!
  Page 1 ($4000-$7FFF): RAM (TPA 계속)
  Page 2 ($8000-$BFFF): RAM
  Page 3 ($C000-$FFFF): RAM (시스템 영역, BDOS, 스택)
```

### 문제의 핵심: BIOS 호출

MSX-DOS2에서는 page 0에 BIOS ROM이 없으므로, BIOS 루틴 주소를 직접 호출하면
엉뚱한 코드(사용자 프로그램 코드 또는 FCB 영역)를 실행하게 된다.

**예시:** `call $0141` (SNSMAT, 키보드 읽기) → DOS 환경에서는 $0141이 TPA 내부이므로
사용자 프로그램의 임의 코드를 실행함.

---

## ubox-msx-lib-z88dk 라이브러리 사용 가이드

### 라이브러리 개요

ubox-msx-lib-z88dk는 MSX SCREEN 2 (TMS9918 그래픽 모드 2) 개발을 위한 라이브러리로,
원본 SDCC용 ubox-msx-lib-1.2.0을 z88dk z80asm으로 포팅한 것이다.

- 위치: `Library/MSX/ubox-msx-lib-z88dk/`
- 빌드: `make -C Library/MSX/ubox-msx-lib-z88dk`
- 링크: `-I<path>/include -L<path>/lib -lubox`
- 헤더: `#include <ubox.h>`

### BIOS 호출 처리 (ROM + DOS 호환)

라이브러리의 모든 BIOS 호출은 세 가지 방식으로 처리되며, ROM과 DOS 빌드 모두에서 동작한다.

#### 방식 1: msxbios 래퍼 (CALSLT 인터-슬롯 콜)

RAM 포인터를 사용하지 않고, 리턴값이 중요하지 않은 BIOS 호출에 사용한다.
z88dk CRT가 제공하는 `msxbios` 심볼을 통해 CALSLT($001C)로 BIOS ROM 슬롯을 호출한다.

```asm
; 패턴: EXTERN msxbios 선언 후
ld ix, BIOS_ADDR
jp msxbios          ; 또는 call msxbios (리턴 필요 시)
```

**msxbios 내부 구현 (z88dk CRT):**
```asm
msxbios:
    ld iy, ($FCC0)    ; EXPTBL — BIOS ROM 슬롯 ID
    call $001C        ; CALSLT — 인터-슬롯 콜
    ei
    ret
```

**적용 대상 (10개 BIOS 루틴):**

| BIOS 루틴 | 주소 | 기능 | 사용 파일 |
|-----------|------|------|----------|
| CHGMOD | $005F | 화면 모드 변경 | ubox_set_mode.asm |
| ENASCR | $0044 | 화면 표시 ON | ubox_enable_screen.asm |
| DISSCR | $0041 | 화면 표시 OFF | ubox_disable_screen.asm |
| WRTVDP | $0047 | VDP 레지스터 쓰기 | ubox_wvdp.asm |
| FILVRM | $0056 | VRAM 채우기 | ubox_fill_screen.asm |
| WRTVRM | $004D | VRAM 1바이트 쓰기 | ubox_put_tile.asm |
| RDVRM | $004A | VRAM 1바이트 읽기 | ubox_get_tile.asm |
| CHGCLR | $0062 | 색상 변경 | ubox_set_colors.asm |
| WRTVRM | $004D | 스프라이트 패턴 쓰기 | ubox_set_sprite_pat8_flip.asm |
| WRTVRM | $004D | 스프라이트 패턴 쓰기 | ubox_set_sprite_pat16_flip.asm |

#### 방식 2: 직접 VDP I/O (RAM↔VRAM 전송)

**문제:** CALSLT가 BIOS ROM을 page 0에 매핑하면, page 0($0000-$3FFF)에 위치한
사용자 RAM 데이터가 BIOS ROM으로 가려진다. LDIRVM은 RAM 포인터(HL)에서 데이터를
읽으므로, 타일 패턴 등 page 0에 로드된 데이터 대신 BIOS ROM 내용을 읽게 된다.

**해결:** VDP I/O 포트($98/$99)를 직접 접근하여 BIOS를 경유하지 않는다.

```asm
; ubox_vdp_direct.asm — 공유 루틴
ubox_ldirvm_direct:        ; RAM → VRAM 복사
    ld a, e
    di
    out ($99), a            ; VRAM 주소 하위
    ld a, d
    or $40                  ; bit 6 = write mode
    out ($99), a            ; VRAM 주소 상위
wr_loop:
    ld a, (hl)
    out ($98), a            ; 데이터 쓰기
    inc hl
    dec bc
    ld a, b
    or c
    jr nz, wr_loop
    ei
    ret
```

**적용 대상:**

| BIOS 루틴 | 주소 | 대체 루틴 | 사용 파일 |
|-----------|------|----------|----------|
| LDIRVM | $005C | ubox_ldirvm_direct | ubox_set_tiles.asm (×3) |
| LDIRVM | $005C | ubox_ldirvm_direct | ubox_set_tiles_colors.asm (×3) |
| LDIRVM | $005C | ubox_ldirvm_direct | ubox_write_vm.asm (×2) |
| LDIRMV | $0059 | ubox_ldirmv_direct | ubox_read_vm.asm (×1) |

**증상 (수정 전):** 화면에 타일 패턴이 깨져서 출력됨 (BIOS ROM 바이트를 패턴 데이터로 사용).

#### 방식 3: 직접 PPI/PSG I/O (키보드/조이스틱 입력)

**문제:** SNSMAT($0141), RDPSG($0096) 등은 A 레지스터로 결과를 리턴하는데,
CALSLT가 슬롯 복원 과정에서 A 레지스터를 덮어쓸 수 있다.

**해결:** PPI(8255) 및 PSG(AY-3-8910) I/O 포트를 직접 접근한다.

```asm
; SNSMAT 대체 — 직접 PPI I/O
; A = 키보드 행 번호 (0-10), 리턴 A = 키 상태 (active LOW)
snsmat_direct:
    ld b, a             ; 행 번호 보존
    di
    in a, ($AA)         ; PPI C 레지스터 읽기
    and $F0             ; 행 선택 비트(0-3) 클리어
    or b                ; 행 번호 설정
    out ($AA), a        ; PPI C 레지스터 쓰기
    in a, ($A9)         ; PPI B 레지스터 — 키 상태 읽기
    ei
    ret
```

**I/O 포트 맵:**

| 장치 | 포트 | 용도 |
|------|------|------|
| PPI 8255 | $A9 (read) | 키보드 열 데이터 (active LOW) |
| PPI 8255 | $AA (write) | 키보드 행 선택 (bits 0-3) |
| PSG AY-3-8910 | $A0 (write) | 레지스터 선택 |
| PSG AY-3-8910 | $A1 (write) | 레지스터 쓰기 |
| PSG AY-3-8910 | $A2 (read) | 레지스터 읽기 |

**적용 대상:**

| BIOS 루틴 | 주소 | 사용 파일 | 호출 횟수 |
|-----------|------|----------|----------|
| SNSMAT | $0141 | ubox_read_keys.asm | 1 |
| SNSMAT | $0141 | ubox_read_ctl.asm | 3 |
| WRTPSG | $0093 | ubox_read_ctl.asm | 1 |
| RDPSG | $0096 | ubox_read_ctl.asm | 1 |

**증상 (수정 전):** 키보드 입력이 전혀 반응하지 않음.

### ISR (인터럽트 서비스 루틴) 제약사항

#### ROM 빌드에서의 ISR

`ubox_init_isr(2)`를 호출하면 HTIMI 훅($FD9F)에 ISR을 설치한다.
VDP 인터럽트마다 ISR이 호출되어 프레임 카운터를 증가시키고,
`ubox_wait()`는 이 카운터를 폴링하여 N프레임 대기한다.

```c
// render.c (ROM 버전)
void render_init(void)
{
    ...
    ubox_init_isr(2);       // ← HTIMI 훅에 ISR 설치 (2프레임 대기)
}
```

#### DOS 빌드에서의 ISR 문제

MSX-DOS2의 인터럽트 처리 흐름:

```
VDP 인터럽트 → $0038 (RAM) → BIOS 인터-슬롯 콜 → BIOS ROM 인터럽트 핸들러
→ HTIMI 훅 ($FD9F) → JP ubox_isr
```

**문제:** HTIMI 훅이 호출되는 시점에 page 0은 **BIOS ROM으로 매핑**되어 있다.
`ubox_isr` 코드는 $0100+ (TPA, page 0)에 위치하므로, JP 대상 주소에는
사용자 코드가 아닌 BIOS ROM 내용이 있다. ISR이 실행되지 않아 프레임 카운터가
증가하지 않고, `ubox_wait()`가 영원히 리턴하지 않는다.

**증상:** 화면은 정상 출력되나 입력에 전혀 반응하지 않음 (hang).

#### DOS 빌드에서의 해결

`ubox_init_isr()` 호출을 제거하고, `ubox_wait()`를 `halt` 기반으로 오버라이드한다.
C 코드에서 같은 이름의 함수를 정의하면 링커가 라이브러리 버전 대신 사용한다.

```c
// render_dos.c (DOS 버전)
void render_init(void)
{
    ...
    // ubox_init_isr() 호출하지 않음!
}

// 라이브러리의 ubox_wait()를 오버라이드
void ubox_wait(void)
{
    __asm
    halt        // VDP 인터럽트 1회 대기
    halt        // VDP 인터럽트 2회 대기
    __endasm;
}
```

`halt` 명령은 다음 인터럽트까지 CPU를 정지시킨다.
인터럽트 핸들러($0038)는 ISR 훅과 무관하게 정상 실행되므로,
`halt`는 항상 정상적으로 해제된다.

---

## DOS 버전 포크 가이드

ROM 프로젝트에서 DOS 버전을 만들 때 필요한 변경사항 체크리스트:

### 1. main.c → main_dos.c

```c
// ROM 버전
void main(void)
{
    ...
    render_cleanup();       // halt 루프 (리턴하지 않음)
}

// DOS 버전
int main(void)             // ← int 리턴
{
    ...
    render_cleanup();       // SCREEN 0 복귀 후 리턴
    return 0;               // ← DOS에 정상 종료 알림
}
```

### 2. render.c → render_dos.c

변경 필수 항목:

| 항목 | ROM (render.c) | DOS (render_dos.c) |
|------|----------------|-------------------|
| ISR 초기화 | `ubox_init_isr(2)` | **호출하지 않음** |
| ubox_wait | 라이브러리 버전 (ISR 기반) | `halt;halt` 오버라이드 |
| cleanup | halt 무한루프 | `ubox_set_mode(0)` 후 리턴 |
| 그리드 접근 | `g_room_grids[][]` 인라인 | `grid_load_room()` 디스크 로드 |
| 타일셋 로드 | 컴파일 시점 정적 | `render_load_tileset()` 디스크 로드 |

### 3. compile.sh → compile_dos.sh

| 항목 | ROM | DOS |
|------|-----|-----|
| zcc -subtype | `rom` | `msxdos2` |
| 소스 파일 | `main.c`, `render.c` | `main_dos.c`, `render_dos.c` |
| 출력물 | `.rom` (32KB 카트리지) | `.COM` (CP/M 실행파일) |
| 추가 단계 | 없음 | rdedisktool로 .dsk 생성 |

### 4. run 스크립트

| ROM | DOS |
|-----|-----|
| `openmsx -cart PROTO02.rom` | MSX-DOS2 부트디스크에 .COM 추가 후 `-diska` |

---

## 인라인 ASM 사용 시 주의사항

### DOS 환경에서 BIOS 직접 호출 금지

C 코드의 인라인 ASM에서도 BIOS 주소를 직접 호출하면 안 된다:

```c
// 잘못된 예 (DOS에서 동작하지 않음)
__asm
xor a
call $005F      ; CHGMOD — page 0에 BIOS ROM이 없음!
__endasm;

// 올바른 예 — ubox 함수 사용
ubox_set_mode(0);
```

### VDP I/O 직접 접근

ubox 함수로 해결되지 않는 VDP 작업이 필요한 경우:

```c
// VDP 상태 레지스터 읽기
__asm
in a, ($99)     // VDP status register (bit 7 = frame flag)
__endasm;

// VRAM에 1바이트 쓰기 (VDP 포트 직접)
__asm
ld a, <addr_low>
di
out ($99), a
ld a, <addr_high>
or $40          // write mode
out ($99), a
ld a, <data>
out ($98), a
ei
__endasm;
```

---

## ubox 라이브러리 수정 이력 요약

원본 SDCC 버전에서 z88dk 포팅 + MSX-DOS2 호환을 위해 수정된 파일 목록:

| 파일 | 수정 내용 |
|------|----------|
| ubox_vdp_direct.asm | **신규** — ldirvm_direct / ldirmv_direct 직접 VDP I/O |
| ubox_set_mode.asm | msxbios 래퍼 |
| ubox_enable_screen.asm | msxbios 래퍼 |
| ubox_disable_screen.asm | msxbios 래퍼 |
| ubox_wvdp.asm | msxbios 래퍼 |
| ubox_fill_screen.asm | msxbios 래퍼 |
| ubox_put_tile.asm | msxbios 래퍼 |
| ubox_get_tile.asm | msxbios 래퍼 (call) |
| ubox_set_colors.asm | msxbios 래퍼 |
| ubox_set_sprite_pat8_flip.asm | msxbios 래퍼 |
| ubox_set_sprite_pat16_flip.asm | msxbios 래퍼 |
| ubox_set_tiles.asm | 직접 VDP I/O (ubox_ldirvm_direct) |
| ubox_set_tiles_colors.asm | 직접 VDP I/O (ubox_ldirvm_direct) |
| ubox_write_vm.asm | 직접 VDP I/O (ubox_ldirvm_direct) |
| ubox_read_vm.asm | 직접 VDP I/O (ubox_ldirmv_direct) |
| ubox_read_keys.asm | 직접 PPI I/O |
| ubox_read_ctl.asm | 직접 PPI/PSG I/O |

수정하지 않은 파일: `ubox_isr.asm`, `ubox_wait.asm`, `ubox_wait_for.asm`,
`ubox_reset_tick.asm`, `ubox_select_ctl.asm`, `ubox_set_sprite_attr.asm`,
`ubox_set_sprite_pat8.asm`, `ubox_set_sprite_pat16.asm`, `ubox_set_user_isr.asm`,
`ubox_get_vsync_freq.asm`
