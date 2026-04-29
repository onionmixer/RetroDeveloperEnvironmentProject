# PLAN: prototype_02_MSX_MSXDOS — SCREEN 2 그래픽 모드 전환

## 1. 프로젝트 개요

`prototype_01_MSX_MSXDOS`의 텍스트 모드 던전 탐색 게임을 MSX SCREEN 2(Graphic 2) 모드로 전환한다.
게임 로직(이동, 문/계단/상자 상호작용)은 동일하게 유지하고, 화면 렌더링만 타일 기반 그래픽으로 교체한다.

| 항목 | prototype_01 | prototype_02 |
|------|-------------|-------------|
| 화면 모드 | TEXT (40열) | SCREEN 2 (256×192, 32×24 타일) |
| 뷰포트 | 12×7 문자 | 10×10 타일 (80×80px) |
| 플레이어 | `!` 문자 | 8×8 스프라이트 |
| 타일 크기 | 해당 없음 | 8×8 픽셀 |
| 맵 크기 | 100×100 | 100×100 (동일) |
| 컴파일러 | z88dk (sccz80) | z88dk (sccz80) |
| 그래픽 라이브러리 | conio | ubox-msx-lib-z88dk (1.2.0 포팅) |
| 출력 형식 | .COM (MSX-DOS) | .COM (MSX-DOS) |

---

## 2. 기술 분석

### 2.1 SCREEN 2 핵심 구조 (doc_screen2_ghost.md 기반)

- **해상도**: 256×192 픽셀, 32×24 타일 그리드
- **팔레트**: 고정 16색 (TMS9918A)
- **색상 제약**: 8×1 라인 단위로 전경/배경 2색 제한
- **VRAM 배치**:

| 영역 | 주소 | 크기 | 용도 |
|------|------|------|------|
| Pattern Generator Table | 0x0000 | 6144B | 타일 비트패턴 (3섹션×256패턴) |
| Name Table | 0x1800 | 768B | 화면 타일 배치 (32×24) |
| Sprite Attribute Table | 0x1B00 | 128B | 스프라이트 위치/색 |
| Color Table | 0x2000 | 6144B | 타일 색상 (3섹션×256패턴) |
| Sprite Pattern Table | 0x3800 | 2048B | 스프라이트 패턴 |

- **스프라이트**: 최대 32개, 스캔라인당 4개 제한, 8×8 또는 16×16, 단색(투명 제외)
- **화면 3분할**: 상/중/하 각 64px(8타일 행)씩 독립 패턴/컬러 뱅크

### 2.2 ubox-msx-lib-z88dk 분석

원본 `ubox-msx-lib-1.2.0`은 SDCC 전용이므로, z88dk(sccz80 + z80asm) 호환으로 포팅한
`ubox-msx-lib-z88dk`를 사용한다. 포팅 상세는 별도 계획 참조.
포팅 위치: `Library/MSX/ubox-msx-lib-z88dk/`

**사용 가능 API**:
- `ubox_set_mode(2)` — SCREEN 2 전환
- `ubox_set_tiles(data)` / `ubox_set_tiles_colors(data)` — 256 타일셋 로드
  - **3섹션 자동 복제**: 내부적으로 2048바이트 입력 데이터를 VRAM 3개 섹션에 각각 복제하여
    총 6144바이트를 기록한다 (Pattern: 0x0000/0x0800/0x1000, Color: 0x2000/0x2800/0x3000).
    따라서 C 배열은 2048바이트(256×8)만 정의하면 전체 24행에서 동일 타일이 동작한다.
- `ubox_put_tile(x, y, tile)` — 개별 타일 배치
- `ubox_fill_screen(tile)` — 전체 화면 채우기
- `ubox_set_sprite_pat8(data, pattern)` — 8×8 스프라이트 패턴 등록
- `ubox_set_sprite_attr(attr, sprite)` — 스프라이트 속성 설정
- `ubox_init_isr(ticks)` — 인터럽트 핸들러 초기화
- `ubox_wait()` — 프레임 동기화
- `ubox_read_keys(row)` — 키보드 매트릭스 읽기
- `ubox_enable_screen()` / `ubox_disable_screen()` — 화면 ON/OFF
- `ubox_wvdp(reg, data)` — VDP 레지스터 직접 쓰기 (BIOS WRITEVDP 래퍼)
- `ubox_set_colors(fg, bg, border)` — 전경/배경/보더 색상 설정 (FORCLR/BAKCLR/BDRCLR)
- `ubox_write_vm(dst, len, src)` — VRAM 블록 쓰기 (dst=VRAM 주소, src=RAM 포인터)
- `ubox_wait_for(frames)` — 지정 프레임 수만큼 대기 (uint8_t, 최대 255)

**제약사항**:
- 원본 ubox는 ROM 카트리지 대상이나, z88dk MSX-DOS 서브타입과 조합 가능
- z88dk `+msx -subtype=msxdos2` 사용 시 **별도 CRT0 불필요** (내장 스타트업 사용)
- VDP 함수는 BIOS 호출 기반이므로 MSX-DOS 환경에서도 동작 가능
- spman(스프라이트 매니저)는 16×16 모드 전용 — 8×8 스프라이트는 직접 ubox 함수 사용

### 2.3 MSX-DOS 호환 전략

ubox 라이브러리의 VDP 함수는 MSX BIOS 엔트리포인트(WRTVRM=0x004D, RDVRM=0x004A 등)를 사용하므로
MSX-DOS 환경에서도 정상 동작한다.

**z88dk 내장 MSX-DOS 지원 활용**:

z88dk의 `+msx -subtype=msxdos2` 옵션은:
1. MSX-DOS .COM 형식 스타트업 코드를 **자동 제공** (커스텀 CRT0 불필요)
2. TPA 스택 포인터 설정, BSS 초기화, main() 호출, 종료 처리를 내장
3. `main()` 리턴 시 자동으로 MSX-DOS에 복귀

**추가 필요 처리**:
- **SCREEN 복원**: `main()` 종료 전 `render_cleanup()` 호출하여 SCREEN 0 복귀 + 스프라이트 클리어
- `atexit()` 등록 또는 `main()` return 직전에 명시적 호출

> **참고**: prototype_01과 동일한 z88dk 툴체인을 사용하므로 빌드 흐름이 동일하다.
> 커스텀 CRT0를 작성할 필요가 없어 구현 복잡도가 감소한다.

---

## 3. 화면 레이아웃 설계

### 3.1 전체 레이아웃 (32×24 타일)

```
Col:  0         1         2         3
      01234567890123456789012345678901
Row 0: Room: Abandoned Hall
    1: +----------+
    2: |..........| X: 49  Y: 49
    3: |..........| Z: 0
    4: |....!.....| (← 10×10 맵 뷰포트, 열 1~10)
    5: |..........|
    6: |..........| [Status Area]
    7: |..........|
    8: |..........| Enter door?
    9: |..........| 1=Yes 0=No
   10: |..........|
   11: |..........|
   12: +----------+
   13:
   14: [Message / Item List Area]
   15:
   ...
   22:
   23: WASD:move 1/0:YN H:help Q:quit
```
> **참고**: 위 ASCII 아트에서 `.`은 맵 타일 1개를 나타낸다 (공백 없이 10개 = 10열).
> 좌표 열 0=좌측 테두리, 열 1~10=맵, 열 11=우측 테두리.
> 정확한 타일 좌표는 §3.2 영역 정의 참조.

### 3.2 영역 정의

| 영역 | 타일 좌표 | 크기 | 용도 |
|------|----------|------|------|
| 룸 이름 | (0,0)-(31,0) | 32×1 | 현재 방 이름 표시 |
| 맵 테두리 | (0,1)-(11,12) | 12×12 | 맵 뷰포트 프레임 |
| 맵 내부 | (1,2)-(10,11) | 10×10 | 던전 맵 타일 |
| 좌표 표시 | (13,2)-(31,3) | 19×2 | X, Y, Z 좌표 |
| 상태 메시지 | (13,6)-(31,10) | 19×5 | 상태/프롬프트 영역 |
| 메시지 영역 | (0,14)-(31,21) | 32×8 | 효과/아이템 목록 표시 |
| 키 힌트 | (0,23)-(31,23) | 32×1 | 조작 안내 |

### 3.3 맵 뷰포트 좌표 상수

```c
#define MAP_ORIGIN_X    1   // 맵 타일 시작 X (화면 타일 좌표)
#define MAP_ORIGIN_Y    2   // 맵 타일 시작 Y
#define MAP_VIEW_W     10   // 맵 뷰포트 너비 (타일 수)
#define MAP_VIEW_H     10   // 맵 뷰포트 높이 (타일 수)
#define BORDER_X        0   // 테두리 시작 X
#define BORDER_Y        1   // 테두리 시작 Y
```

---

## 4. 타일 설계

### 4.1 타일 목록

모든 타일은 **투명(색상 0) + 단색** 구성. 8×8 정방형.

#### 맵 타일 (인덱스 0~15)

| 인덱스 | 이름 | 색상 | 패턴 설명 |
|--------|------|------|-----------|
| 0 | TILE_EMPTY | 투명+검정(1) | 완전 빈 타일 (화면 밖/빈 공간) |
| 1 | TILE_FLOOR | 투명+진한녹색(12) | 바닥 — 희소 도트 패턴 (2개 픽셀 산포) |
| 2 | TILE_WALL | 투명+회색(14) | 벽 — 벽돌 패턴 (교차 가로줄) |
| 3 | TILE_DOOR_H | 투명+밝은파랑(5) | 수평 문 — 가로 이중선 |
| 4 | TILE_DOOR_V | 투명+밝은파랑(5) | 수직 문 — 세로 이중선 |
| 5 | TILE_STAIR_DN | 투명+밝은빨강(9) | 하향 계단 — 하향 화살표 ▼ |
| 6 | TILE_STAIR_UP | 투명+밝은녹색(3) | 상향 계단 — 상향 화살표 ▲ |
| 7 | TILE_BOX | 투명+밝은노랑(11) | 상자 — 작은 사각 상자 모양 |

#### 테두리 타일 (인덱스 16~23)

| 인덱스 | 이름 | 색상 | 패턴 설명 |
|--------|------|------|-----------|
| 16 | TILE_BORDER_TL | 투명+흰색(15) | 좌상단 모서리 ┌ |
| 17 | TILE_BORDER_TR | 투명+흰색(15) | 우상단 모서리 ┐ |
| 18 | TILE_BORDER_BL | 투명+흰색(15) | 좌하단 모서리 └ |
| 19 | TILE_BORDER_BR | 투명+흰색(15) | 우하단 모서리 ┘ |
| 20 | TILE_BORDER_H | 투명+흰색(15) | 수평 테두리 ─ |
| 21 | TILE_BORDER_V | 투명+흰색(15) | 좌측 수직 테두리 │ (bit5) |
| 22 | TILE_BORDER_VR | 투명+흰색(15) | 우측 수직 테두리 │ (bit2) |
| 23 | TILE_BORDER_HB | 투명+흰색(15) | 하단 수평 테두리 ─ (row5) |

#### 폰트 타일 (인덱스 32~127)

| 범위 | 내용 | 색상 |
|------|------|------|
| 32 | 공백 (SPACE) | 투명 |
| 33 | `!` 느낌표 | 투명+흰색(15) |
| 39 | `'` 아포스트로피 | 투명+흰색(15) |
| 40~41 | `(` `)` 괄호 | 투명+흰색(15) |
| 43 | `+` 더하기 | 투명+흰색(15) |
| 44 | `,` 쉼표 | 투명+흰색(15) |
| 45 | `-` 하이픈 | 투명+흰색(15) |
| 46 | `.` 마침표 | 투명+흰색(15) |
| 47 | `/` 슬래시 | 투명+흰색(15) |
| 48~57 | 숫자 `0`~`9` | 투명+흰색(15) |
| 58 | `:` 콜론 | 투명+흰색(15) |
| 61 | `=` 등호 | 투명+흰색(15) |
| 63 | `?` 물음표 | 투명+흰색(15) |
| 65~90 | 대문자 `A`~`Z` | 투명+흰색(15) |
| 91 | `[` 좌 대괄호 | 투명+흰색(15) |
| 93 | `]` 우 대괄호 | 투명+흰색(15) |
| 97~122 | 소문자 `a`~`z` (대문자 패턴 복제) | 투명+흰색(15) |

**필요 폰트 문자 총 수**: ~70자 (A-Z 26 + a-z 26 + 0-9 10 + 특수 ~10)

> **설계 원칙**: 타일 인덱스를 ASCII 코드와 일치시켜 문자열→타일 변환을 단순화한다.
> `tile_index = (uint8_t)character` 로 직접 매핑 가능.
> 미정의 인덱스(34~38 등)는 빈 타일(TILE_EMPTY)과 동일 패턴으로 채운다.

**게임 텍스트 점검 — 실제 사용되는 문자 확인**:
- 룸 이름: `Abandoned Hall`, `Crystal Cavern`, `Shadow Labyrinth`
- 상태: `Moved.`, `Blocked.`, `Continue.`, `Door cancelled.` 등
- 프롬프트: `Enter Crystal Cavern?`, `Open box by Goblin Trickster?`
- 효과: `Dust swirls as the lid opens. Nothing unusual happens.` (54자)
- 아이템: `Gold Coin`, `Mana Crystal`, `Poison Vial` 등
- → 필요 특수문자: `.` `?` `!` `'` `,` `-` `(` `)` `:` `=` `/` `[` `]` `+` SPACE

### 4.2 타일 데이터 생성 제약 조건

#### 4.2.1 하드웨어 제약 (TMS9918A SCREEN 2)

- **타일 크기**: 8×8 픽셀, 1타일 = pattern 8바이트 + color 8바이트
- **8×1 라인 2색 규칙**: 타일의 **각 가로줄(8px)마다** 전경색(FG)과 배경색(BG) **2색만** 사용 가능.
  한 줄 안에 3색 이상을 넣을 수 없다. 타일 설계 시 이 제약을 반드시 준수해야 한다.
- **Color Table 바이트 형식**: `(상위 4bit: FG색 번호) | (하위 4bit: BG색 번호)`
  - 예: FG=14(Gray), BG=0(Transparent) → `0xE0`
  - 예: FG=15(White), BG=0(Transparent) → `0xF0`
- **팔레트**: 고정 16색, 변경 불가. 아래 참조표 사용:

| 번호 | 색상명 | 용도 (본 프로젝트) |
|---:|--------|-----------------|
| 0 | Transparent | 배경색 (모든 타일 BG) |
| 1 | Black | TILE_EMPTY FG |
| 3 | Light Green | TILE_STAIR_UP FG |
| 5 | Light Blue | TILE_DOOR_H/V FG |
| 8 | Red | 플레이어 스프라이트 |
| 9 | Light Red | TILE_STAIR_DN FG |
| 11 | Light Yellow | TILE_BOX FG |
| 12 | Dark Green | TILE_FLOOR FG |
| 14 | Gray | TILE_WALL FG |
| 15 | White | 테두리/폰트 FG |

#### 4.2.2 배열 구조 규칙

**Pattern 배열** (`ubox_set_tiles(const uint8_t *tiles)`용):
```c
// 256타일 × 8바이트 = 2048바이트, 연속 배열
// ubox_set_tiles()가 이 2048바이트를 VRAM 3섹션에 자동 복제 (§2.2 참조)
// tileset_patterns[tile_index * 8 + row] = 해당 타일의 row번째 줄 비트패턴
// bit 7(MSB) = 좌측 픽셀, bit 0(LSB) = 우측 픽셀
// 1 = FG색 픽셀, 0 = BG색 픽셀
const uint8_t tileset_patterns[256 * 8];
```

**Color 배열** (`ubox_set_tiles_colors(const uint8_t *colors)`용):
```c
// 256타일 × 8바이트 = 2048바이트, 연속 배열
// ubox_set_tiles_colors()가 이 2048바이트를 VRAM 3섹션에 자동 복제 (§2.2 참조)
// tileset_colors[tile_index * 8 + row] = 해당 타일의 row번째 줄 색상
// 형식: (FG << 4) | BG
const uint8_t tileset_colors[256 * 8];
```

**미사용 타일 슬롯** (인덱스 8~15, 24~31, 34~38, 59~60, 62, 64, 92, 94~96, 123~255):
- pattern: all `0x00` (빈 픽셀)
- color: all `0x00` (Transparent/Transparent)

**스프라이트 패턴**은 별도 영역(VRAM 0x3800)이므로 타일셋 배열에 포함하지 않는다.
`ubox_set_sprite_pat8()` 함수로 개별 등록한다.

#### 4.2.3 폰트 설계 기준

- **고정폭(monospace)**: 모든 글자가 동일한 8×8 셀을 차지
- **글리프 크기**: 5×7 픽셀 글리프 + 우측 1px 여백 + 하단 1px 여백
  - 즉, 8×8 셀 내에서 좌상단 6×7 영역에 글리프를 배치하고 우측 2열, 하단 1행은 비움
  - 이 방식으로 타일을 나란히 놓으면 자연스러운 문자 간격이 형성됨
- **색상**: 일괄 `0xF0` (FG=15 White, BG=0 Transparent), 모든 줄 동일
- **소문자**: 대문자 패턴을 그대로 복제 (소문자 전용 글리프 미설계)
- **참고 폰트**: MSX BIOS 내장 폰트(6×8 매트릭스) 스타일 참조, 8×8 셀에 맞게 조정

#### 4.2.4 생성 방식

**Claude Code가 C 배열로 직접 생성**:
- 모든 타일(맵, 테두리, 폰트)의 8바이트 비트패턴 + 8바이트 색상을
  C 헤더 파일(`tiles.h`, `font.h`)에 직접 정의한다.
- PNG 이미지나 외부 변환 도구(png2tiles.py 등)를 사용하지 않는다.
- 각 타일은 §4.1 타일 목록의 패턴 설명 + §4.2.1~4.2.3 제약 조건을 기반으로 설계한다.

**생성 파일 및 대상**:

| 파일 | 내용 | 타일 수 |
|------|------|------:|
| `tiles.h` | 맵 타일 8종(인덱스 0~7) + 테두리 타일 8종(인덱스 16~23) — pattern + colors | 16 |
| `font.h` | 폰트 타일(인덱스 32~122, ASCII 매핑) — pattern만 (색상 일괄 0xF0) | ~70 |
| `render.c` 내 | 256타일 전체를 결합한 `tileset_patterns[2048]` + `tileset_colors[2048]` (아래 주의 참조) | 256 |
| `render.c` 내 | 플레이어 스프라이트 `player_sprite[8]` (§5 참조) | 1 |

> **메모리 최적화**: tileset_patterns/tileset_colors 배열은 **런타임 조립 함수 대신
> 정적 초기화(const 배열)로 직접 정의**해야 한다. 런타임 조립 시 tiles.h/font.h의
> 소스 배열 + 조립 결과 배열이 RAM에 동시 존재하여 ~4KB 낭비.
> 구현 방법: render.c에서 `const uint8_t tileset_patterns[2048] = { ... };` 내부에
> `#include "tiles.h"` / `#include "font.h"` 를 삽입하여 컴파일 타임에 단일 배열로 결합.
> VRAM 전송 후에는 이 배열이 불필요하나, const이므로 코드 영역에 위치하여
> BSS/힙과 분리됨 (z88dk MSX-DOS는 전체가 RAM이지만 오버레이 최적화의 여지 남김).

### 4.3 타일 비트패턴 (구현 완료)

맵 타일 8종(인덱스 0-7), 테두리 타일 8종(인덱스 16-23), 폰트 타일(인덱스 32-122)의
비트패턴과 색상은 `src/tiles.h` 및 `src/font.h`에 구현 완료.

**설계 원칙 요약**:
- 맵 타일: §4.1 색상 규칙에 따라 각 타일 FG 단색 + BG=Transparent
- 테두리: 좌측 수직선(V, TL, BL)=bit5, 우측(VR, TR, BR)=bit2로 정렬. 상단 H(row 2), 하단 HB(row 5) 분리
- 폰트: 5×7 글리프, 8×8 셀 좌측 정렬, 우측 2col+하단 1row 비움, Color=0xF0

---

## 5. 스프라이트 설계

### 5.1 플레이어 스프라이트

- **크기**: 8×8 픽셀
- **색상**: 빨강(8) — 단색 + 투명
- **패턴**: 간단한 인간 실루엣

```c
// 플레이어 스프라이트 (8×8)
const uint8_t player_sprite[8] = {
    0b00011000,  // ...##...  (머리)
    0b00011000,  // ...##...
    0b00111100,  // ..####..  (상체)
    0b01011010,  // .#.##.#.  (팔)
    0b00011000,  // ...##...  (허리)
    0b00111100,  // ..####..  (하체)
    0b00100100,  // ..#..#..  (다리)
    0b00100100,  // ..#..#..
};
```

### 5.2 스프라이트 위치 계산

```c
// 플레이어의 화면상 스프라이트 좌표 (픽셀 단위)
sprite_x = (player_x - cam_x) * 8 + MAP_ORIGIN_X * 8;
sprite_y = (player_y - cam_y) * 8 + MAP_ORIGIN_Y * 8 - 1; // Y는 -1 오프셋 (TMS9918 규칙)
```

### 5.3 스프라이트 사용 방식

- **spman 미사용**: 8×8 모드이므로 ubox 직접 함수 사용
- VDP 레지스터 1 설정: `ubox_wvdp(1, 0xE0)` — BIOS `INIGRP`가 설정하는 기본값은 16×16 스프라이트이므로 8×8으로 오버라이드 필요
  ```
  0xE0 = 1110_0000
    Bit 7 = 1: 16KB VRAM (VRAM size control)
    Bit 6 = 1: Screen display ON (screen output control)
    Bit 5 = 1: VBlank interrupt enable
    Bit 1 = 0: 8×8 스프라이트 (1이면 16×16)
    Bit 0 = 0: Sprite magnification OFF
  ```
- `ubox_set_sprite_pat8(player_sprite, 0)` — 패턴 0번에 등록
- `ubox_set_sprite_attr(&player_attr, 0)` — 스프라이트 0번에 속성 설정

---

## 6. 모듈 아키텍처

### 6.1 파일 구조

```
Examples/prototype_02_MSX_MSXDOS/
├── compile.sh                          # 빌드 스크립트 (z88dk)
├── run_openmsx_prototype_02_msx_msxdos_diskaddtest.sh
├── README.txt
├── src/
│   ├── main.c                          # 메인 루프 + 상호작용
│   ├── engine.h                        # 게임 상태 구조체/상수
│   ├── logic.c                         # 게임 로직 (prototype_01에서 이식)
│   ├── logic.h                         #
│   ├── render.c                        # SCREEN 2 렌더링 (신규)
│   ├── render.h                        #
│   ├── input.c                         # 키보드 입력 처리 (신규)
│   ├── input.h                         #
│   ├── tiles.h                         # 타일 데이터 정의
│   ├── font.h                          # 폰트 타일 데이터
│   ├── help.c                          # 도움말 화면 (타일 기반)
│   ├── help.h                          #
│   ├── room_data.h                     # 룸 데이터 (prototype_01에서 복사)
│   └── room_data.c                     # 룸 데이터 (prototype_01에서 복사)
├── generated/                          # (빌드 시 생성)
├── build/                              # (빌드 시 생성)
└── tests/
    └── host/
        └── test_camera.c              # 카메라 유닛 테스트 (뷰포트 10×10 적용)
```

> **참고**: z88dk 내장 MSX-DOS CRT(`-subtype=msxdos2`)를 사용하므로
> 별도 `crt0_msxdos.z80` 파일이 불필요하다.

### 6.2 모듈 의존 관계

```
main.c
  ├── engine.h (상수, 구조체)
  ├── logic.c/h (게임 로직 — prototype_01과 동일)
  │   └── room_data.c/h (맵 데이터)
  ├── render.c/h (SCREEN 2 렌더링)
  │   ├── tiles.h (타일 패턴/색상 데이터)
  │   ├── font.h (폰트 데이터)
  │   └── room_data.h (render_draw_map이 룸 그리드 참조)
  ├── input.c/h (키보드 입력 — ubox_read_keys 래퍼)
  └── help.c/h (도움말 화면)
      └── render.c/h (텍스트 출력 함수 공유)
```

### 6.3 모듈별 상세 설계 (구현 완료)

각 모듈의 구현은 `src/` 디렉토리의 실제 소스 파일 참조. 주요 설계 결정만 기록:

#### engine.h
- VIEW_W=10, VIEW_H=10 (prototype_01은 12×7)
- GameState에서 status[80] 제거 → render.c 내부 static 버퍼로 이동

#### render.c / render_dos.c
- **초기화 순서**: set_mode(2) → set_colors(15,1,1) → wvdp(1,0xE0) → disable_screen → set_tiles → fill_screen → set_sprite → draw_border → init_isr. **enable_screen은 render_redraw_all()에서 호출** (빈 화면 프레임 방지)
- **부분 갱신**: dirty flag 기반 상태 메시지 갱신, 더티 타일 추적(prev_map)
- **프롬프트**: render_prompt_yes_no → STATUS 영역(13,6), render_wait_any_key → MESSAGE 영역(0,14)
- **char→tile**: `@` 문자의 H/V 구분은 좌우 인접 셀 확인으로 판별
- render_set_status(msg) — GameState 파라미터 없는 API

#### logic.c
- 카메라 클램핑: `clamp(player - VIEW/2, 0, ROOM - VIEW)`. 나머지 로직은 prototype_01과 동일

#### main.c / main_dos.c
- 초기화: logic_init → render_init → logic_update_camera → render_redraw_all
- 메인 루프: ubox_wait → input_read → 입력 처리 → camera → draw_map → update_player → update_status
- "입력→상태 변경→렌더링" 순서 필수 (부분 갱신)

#### help.c
- 진입: ubox_fill_screen(0) → sprite_attr.y=0xD0 → 텍스트 렌더링
- 복귀: render_redraw_all() → 전체 복원

---

## 7. 입력 처리

### 7.1 입력 모델 변경 (cgetc → ubox_read_keys)

prototype_01은 `cgetc()` (블로킹, 한 번에 1문자)를 사용한다.
prototype_02는 `ubox_read_keys(row)` (논블로킹, 키보드 매트릭스 행 스캔)으로 전환한다.

**`ubox_read_keys(row)` 동작 원리**:
- MSX BIOS `SNSMAT` (0x0141) 호출 후 결과를 비트반전(CPL)
- 반환값: 해당 행에서 **눌린 키 = 1**, 안 눌린 키 = 0
- 인자: 행 번호 (0~8)

### 7.2 키보드 매트릭스 상세 매핑

게임에서 사용하는 키의 **정확한** 행/비트 위치 (ubox.h 기준):

| 키 | 기능 | 행(row) | 비트마스크 | ubox 상수 |
|----|------|---------|-----------|-----------|
| 0 | 취소(아니오) | **0** | 0x01 (bit 0) | `UBOX_MSX_KEY_0` |
| 1 | 확인(예) | **0** | 0x02 (bit 1) | `UBOX_MSX_KEY_1` |
| A | 좌 이동 | **2** | 0x40 (bit 6) | `UBOX_MSX_KEY_A` |
| D | 우 이동 | **3** | 0x02 (bit 1) | `UBOX_MSX_KEY_D` |
| H | 도움말 | **3** | 0x20 (bit 5) | `UBOX_MSX_KEY_H` |
| Q | 종료 | **4** | 0x40 (bit 6) | `UBOX_MSX_KEY_Q` |
| S | 아래 이동 | **5** | 0x01 (bit 0) | `UBOX_MSX_KEY_S` |
| W | 위 이동 | **5** | 0x10 (bit 4) | `UBOX_MSX_KEY_W` |
| SPACE | 도움말 나가기 | **8** | 0x01 (bit 0) | `UBOX_MSX_KEY_SPACE` |

> **주의**: WASD 키가 행 2, 3, 5에 분산되어 있다.
> 이동 키를 모두 읽으려면 최소 3개 행(2, 3, 5)을 스캔해야 한다.

### 7.3 입력 처리 구현 (구현 완료)

`src/input.c` / `src/input.h` 참조. 주요 설계:

- `input_read()`: 6개 행(0,2,3,4,5,8) 스캔, 우선순위 이동 > 상호작용 > UI
- `KEY_REPEAT_DELAY=6` (~200ms at 30fps), key_timer로 반복 제어
- `input_reset_timer()`: 프롬프트 복귀 후 즉시 입력 수신용

### 7.4 프롬프트 대기 함수 (구현 완료)

`src/render.c` / `src/render_dos.c` 참조.

- `wait_yes_no()`: 키 해제 대기 → row0 스캔 (1=Yes, 0=No)
- `wait_any_key()`: 전체 키 해제 대기 → 아무 키 입력 대기
- 프롬프트 래퍼가 리턴 직전 `input_reset_timer()` 호출

---

## 8. 빌드 시스템

### 8.1 빌드 시스템 (구현 완료)

`compile.sh`, `compile_dos.sh`, `run_openmsx.sh`, `run_openmsx_dos.sh` 참조.

**핵심 빌드 설정**:
- ROM: `zcc +msx -subtype=rom -compiler=sccz80 -SO2 -create-app`
- DOS: `zcc +msx -subtype=msxdos2 -compiler=sccz80 -SO2 -create-app`
- ubox 링크: `-I$UBOX_INC -L$UBOX_LIB -lubox`
- 별도 CRT0 불필요 (z88dk 내장)

**표준 타겟**: `clean | build | disk | run | all`

### 8.2 디스크 이미지 생성

- DOS: `msxdos23.dsk` 부트디스크를 복사 → `rdedisktool --bootdisk-mode strict add`로 .COM 주입
- ROM: 디스크 불필요 (32KB ROM 이미지 직접 생성)

### 8.3 에뮬레이터 실행 흐름

1. 부트디스크 → `/tmp` 복사 (원본 보호)
2. `rdedisktool --bootdisk-mode off delete` → `strict add` (이전 버전 교체)
3. `BOOT_DISK=/tmp/... run_openmsx_msxdos2.sh` 실행

### 8.4 개발 사이클

```bash
./compile.sh all              # clean → build → disk (ROM)
./compile_dos.sh all          # clean → build → disk (DOS)
./run_openmsx.sh              # ROM 에뮬레이터 실행
./run_openmsx_dos.sh          # DOS 에뮬레이터 실행
AUTO_BUILD=1 ./run_openmsx_dos.sh  # 자동 빌드 + 실행
```

---

## 9. 구현 단계 (전체 완료)

모든 Phase가 구현 완료되었으며, ROM/DOS 듀얼 빌드로 확장됨.

| Phase | 목표 | 상태 |
|-------|------|------|
| Phase 1 | SCREEN 2 + 타일 렌더링 프레임워크 | 완료 |
| Phase 2 | 30×24 맵 렌더링 + 10×10 뷰포트 카메라 | 완료 |
| Phase 3 | 플레이어 스프라이트 + WASD 이동 | 완료 |
| Phase 4 | 문/계단/상자 상호작용 + 프롬프트 | 완료 |
| Phase 5 | 도움말 + 몬스터 + ROM/DOS 분리 + 마무리 | 완료 |

---

## 10. 테스트 전략

### 10.1 빌드 테스트

```bash
./compile.sh build    # z88dk 컴파일 성공 확인 (.com 생성)
./compile.sh disk     # 디스크 이미지 생성 확인
./compile.sh run      # 에뮬레이터 부팅 확인
```

### 10.2 기능 테스트

| 항목 | 검증 내용 |
|------|-----------|
| 화면 초기화 | SCREEN 2 전환, 타일셋 로드, 테두리 표시 |
| 맵 렌더링 | 10×10 뷰포트에 올바른 타일 표시 |
| 플레이어 이동 | WASD 입력, 충돌 판정, 카메라 추적 |
| 문 상호작용 | 프롬프트 표시, 룸 전환 |
| 계단 상호작용 | 프롬프트 표시, Z 레벨 변경 |
| 상자 상호작용 | 효과 메시지 → 아이템 목록 2단계 표시 |
| 도움말 | H키 진입, 스크롤, SPACE 복귀 |
| 종료 | Q키, SCREEN 0 복귀, MSX-DOS 프롬프트 |

### 10.3 호스트 테스트

```bash
# 카메라 클램핑 테스트 (VIEW_W=10, VIEW_H=10으로 수정)
gcc -DVIEW_W=10 -DVIEW_H=10 -o test_camera tests/host/test_camera.c
./test_camera
```

---

## 11. 리스크 및 대응

| 리스크 | 영향 | 대응 |
|--------|------|------|
| **ubox z88dk 포팅 미완료** | **전체 계획 차단** | **본 프로젝트의 최상위 선행 의존성. 별도 포팅 계획 완료 후 Phase 0 검증 통과 필수. 포팅 실패 시 대안: MSXgl 또는 직접 BIOS 호출 래퍼 작성** |
| ubox z88dk 포팅 링크 오류 | 빌드 실패 | 포팅 계획의 검증 단계(어셈블+링크 테스트)에서 사전 확인 |
| sccz80 호출 규약 불일치 | 런타임 크래시 | 포팅 시 스택 레이아웃 검증, 디버거로 SP 추적 |
| VRAM 업데이트 속도 | 맵 전체 갱신 시 깜빡임 | `ubox_disable_screen()` 후 일괄 갱신, 또는 변경 타일만 갱신 |
| 메모리 부족 (room_data 37KB) | TPA 초과 | 룸 데이터 압축 또는 오버레이 분할 검토 |
| 키 입력 누락 | 반응 불량 | ISR 기반 키 상태 폴링 주기 조정 |
| 폰트 타일 수 부족 | 소문자/특수문자 표시 불가 | 대문자만 사용, 최소 특수문자셋으로 제한 |

### 11.1 메모리 예산 (MSX-DOS2 TPA ≈ 48.9KB)

> 실측 기준: PROTO02.COM = 18,346 bytes, PROTO02.rom raw = 15,325 bytes
> BSS = 178 bytes (PROTO02_BSS.bin), DATA = 2,233 bytes (PROTO02_DATA.bin)

| 영역 | 추정 크기 | 산출 근거 |
|------|----------|----------|
| 코드 (main+logic+render+input+help+monster) | ~8.7KB | 빌드 결과 역산 (18,346 - 데이터 합계) |
| ubox.lib 링크 코드 | ~2KB | VDP/키보드/스프라이트 BIOS 래퍼 (Z80 asm) |
| z88dk 런타임/CRT | ~0.4KB | CRT0 + stdlib 스텁 |
| g_room_grids (그리드) | 2.2KB | 3 × 24 × 31 bytes (30×24 맵, null 포함) |
| 룸 메타데이터 (구조체) | ~0.6KB | Door/Stair/Box/Monster 구조체 배열 |
| 문자열 테이블 | ~0.35KB | placed_by + effects + names + items + 포인터 |
| 타일셋 (패턴+색상) | ~4.1KB | tileset_patterns[2048] + colors[2048] + sprite[8] |
| BSS (전역변수) | ~0.2KB | GameState + 입력 버퍼 (빌드 결과: 178B) |
| 런타임 스택 | ~1KB | z88dk 기본 스택 (SP 초기값 기준) |
| **합계** | **~19.5KB** | COM=18,346 + BSS=178 + 스택≈1,024 |
| **TPA 여유** | **~29.4KB** | 48,896 - 19,500 |

> **룸 추가 영향**: 룸당 ~935 bytes 증가 (그리드 744 + 메타 191).
> 외부화 없이 ~33룸이 TPA 한계. Appendix B Phase 1 외부화 후 ~170룸까지 가능.
>
> **ROM 빌드**: 32KB ROM 내 15,325 bytes 사용 중, ~21룸이 32KB 한계.
>
> 상세 분석은 Appendix B §B.0 참조.

---

## 12. 참고 자료

- **prototype_01 소스**: `./Examples/prototype_01_MSX_MSXDOS/src/`
- **ubox-msx-lib (z88dk 포팅)**: `./Library/MSX/ubox-msx-lib-z88dk/`
- **ubox 원본 (SDCC, 참고용)**: `./Library/MSX/ubox-msx-lib-1.2.0/`
- **ubox 예제 게임 (참고용)**: `./Library/MSX/ubox-msx-lib-1.2.0/game/`
- **SCREEN 2 기술 문서**: `/mnt/USERS/onion/DATA_ORIGN/Workspace/06_MSX_DATA/datas/doc_screen2_ghost.md`
- **TMS9918A 팔레트**: doc_screen2_ghost.md §5 색상 코드 표
- **ubox API 레퍼런스**: `./Library/MSX/ubox-msx-lib-z88dk/include/ubox.h`
- **ubox z88dk 포팅 계획**: 별도 계획 파일 참조
- **MSX 개발 환경 가이드**: `./DEVELOPER_MSX_HOWTO.md` (z88dk/Hi-Tech C 경로, 디스크 검증 패턴, openMSX 실행법)
- **prototype_01 실행 스크립트**: `./Examples/prototype_01_MSX_MSXDOS/run_openmsx_prototype_01_msx_msxdos_diskaddtest.sh`
- **프로젝트 공통 실행 스크립트**: `./run_openmsx_msxdos2.sh`, `./run_openmsx_msxdos2_diskaddtest.sh`

---

## Appendix A: 검토 보강사항

본 부록은 플랜 초안 대비 교차 검증에서 발견된 보강 사항을 정리한다.

### A.1 문/상자 2타일 오브젝트 렌더링 전략

prototype_01의 문(`@`)과 상자(`%`)는 `orientation` 필드에 따라 2×1(수평) 또는 1×2(수직)로 배치된다.
룸 그리드 자체에 이미 2타일 분량의 `@`/`%` 문자가 배치되어 있으므로,
렌더러는 각 셀을 독립적으로 변환하면 된다.

문(`@`)의 수평/수직 시각 구분은 인접 타일 확인 방식으로 처리한다.
**최종 구현은 §6.3 `char_to_map_tile()` 참조** (인접 셀 기반 H/V 분기 버전으로 확정).

### A.2 스프라이트 Y좌표 -1 오프셋 설명

TMS9918A 스프라이트 좌표 규칙:
- **Y=0xFF(255)** 가 화면 최상단 라인(line 0)에 해당
- **Y=0** 은 화면 두 번째 라인(line 1)
- 따라서 타일 좌표와 맞추려면 Y에서 1을 빼야 한다

```c
// 정확한 스프라이트 좌표 계산
struct sprite_attr player_attr;
player_attr.x = (gs->x - gs->cam_x) * 8 + MAP_ORIGIN_X * 8;
player_attr.y = (gs->y - gs->cam_y) * 8 + MAP_ORIGIN_Y * 8 - 1;  // TMS9918 보정
player_attr.pattern = 0;
player_attr.attr = 8;  // 색상: Red(8)
```

> **추가 주의**: Y=0xD0(208)은 스프라이트 테이블 종료 마커이다.
> 스프라이트 0의 Y가 208이면 모든 스프라이트가 비표시된다.
> 플레이어가 화면 하단에 있을 때 이 값과 충돌하지 않는지 확인 필요.
> (MAP_ORIGIN_Y=2 기준 최대 Y = 2*8+9*8-1 = 87, 안전 범위)

### A.3 32열 텍스트 줄바꿈 처리

prototype_01은 40열, prototype_02는 32열. `render_print_wrap()` 함수로 단어 단위 줄바꿈 구현 완료 (`src/render.c` 참조).

- 상태 영역: 19자 폭 (col 13~31), 메시지 영역: 32자 폭 (col 0~31)
- 공백은 타일로 출력하지 않으므로 `render_clear_area()` 사전 초기화 필수

### A.4 도움말 시스템 32열 적응

- `HELP_LINE_WIDTH=33` (32열+null), `HELP_DISPLAY_ROWS=23`, `WRAP_WIDTH=32`
- 32자 초과 행은 `wrap_and_add()`에 의해 자동 줄바꿈

### A.5 VRAM 갱신 최적화 — 더티 타일 추적

`src/render.c`, `src/render_dos.c` 참조. 구현 완료.

- **더티 타일**: `prev_map[10][10]` 캐시로 변경분만 `ubox_put_tile()` 갱신
- **Full Redraw**: disable_screen → fill_screen(0) → prev_map 무효화(0xFF) → border+map+status+player → enable_screen
- **VBlank 일괄 전송**: `ubox_write_vm()` 으로 Name Table 직접 쓰기 (put_tile보다 빠름)

### A.6 기능 완전성 체크리스트

prototype_01의 모든 기능이 prototype_02에서 재현되는지 확인:

| # | 기능 | prototype_01 구현 | prototype_02 대응 | 상태 |
|---|------|------------------|------------------|------|
| 1 | WASD 이동 | cgetc() + switch | input_read() + ubox_read_keys | §7.3 |
| 2 | 벽 충돌 차단 | logic_try_move() | 동일 (VIEW 상수만 변경) | §6.3 |
| 3 | 상자 충돌 차단 | logic_get_tile()=='%' | 동일 | 변경 없음 |
| 4 | 문 상호작용 | logic_find_door_at() + 프롬프트 | 동일 + render_prompt_yes_no | §7.4 |
| 5 | 문 룸 전환 | logic_door_transition() | 동일 | 변경 없음 |
| 6 | 계단 상호작용 | logic_find_stair_at() + 프롬프트 | 동일 | 변경 없음 |
| 7 | 계단 Z레벨 이동 | logic_stair_transition() | 동일 | 변경 없음 |
| 8 | 상자 열기 | logic_find_box_at() + 2단계 표시 | 동일 + render_wait_any_key | §7.4 |
| 9 | 카메라 추적 | logic_update_camera() | 동일 (VIEW=10×10) | §6.3 |
| 10 | 상태 메시지 | render_set_status() → 40열 | render_print() → 19열(줄바꿈) | §A.3 |
| 11 | 좌표/룸 표시 | printf 직접 출력 | render_print() 타일 출력 | §3.2 |
| 12 | 키 힌트 | 하드코딩 문자열 | render_print() row 23 | §3.2 |
| 13 | 도움말(H) | help_render() 40열 | render_print() 32열 | §A.4 |
| 14 | 도움말 스크롤 | help_scroll() W/S | 동일 (input_read 사용) | §7.3 |
| 15 | 종료(Q) | 프롬프트 → running=0 | 동일 + render_cleanup | §A.7 |
| 16 | 플레이어 표시 | '!' 문자 | 8×8 스프라이트 (빨강) | §5 |

### A.7 ubox ISR 초기화 — MSX-DOS 환경 주의사항

`src/render.c`의 `render_cleanup()` 참조. 구현 완료.

**핵심 요약**:
- `ubox_init_isr()`는 HTIMI_HOOK(0xFD9F) 설치 + BIOS 키보드 비활성화(SCNCNT=0)
- 종료 시 반드시 복원: HTIMI_HOOK에 RET(0xC9) 기록 → BIOS INITXT(0x006C) 호출
- 미복원 시: TPA 재사용 후 VBlank가 쓰레기 코드로 점프 → 시스템 크래시
- **DOS 빌드(render_dos.c)**: ISR 미사용, halt 기반 대기이므로 이 문제 없음

### A.8-A.10 빌드 검증 (통과 완료)

Phase 0 사전 검증(ubox 빌드, MSX-DOS 최소 테스트, SCREEN 2 전환, room_data 링크) 모두 통과.
`compile.sh`, `compile_dos.sh`에 check_z88dk/check_ubox 검증 포함.

---

## Appendix B: 맵/타일 데이터 외부 파일 분리 (DOS 빌드 전용)

> 분석일: 2026-03-01
> 목적: 맵 그리드 데이터와 타일 패턴/색상을 MSX-DOS 디스크 파일로 분리하여
> 더 많은 룸 추가와 룸별 타일셋 교체를 가능하게 한다.
> 참고: Apple II prototype_02_AppleII_prodos Appendix B의 MSX 대응 구현.
> **구현 상태: Phase 1 완료** (2026-03-01). ROM/DOS 빌드 모두 정상 동작 확인.
> 실측 결과: COM 18,346 → 17,334 bytes (-1,012B), 디스크 데이터 ROOM00-02 + TILES0 정상 로드.

### B.0 COM/ROM 크기 분석 및 외부화 동기

> 실측 기준: PROTO02.COM = 18,346 bytes, PROTO02.rom = 32,768 bytes (raw 15,325 bytes)
> BSS = 178 bytes, DATA = 2,233 bytes

#### B.0.1 COM 바이너리 구성 분석

| 구성 요소 | 바이트 | 비율 | 비고 |
|-----------|--------|------|------|
| **tileset 배열** | **4,096** | **22.3%** | tileset_patterns[2048] + tileset_colors[2048] |
| **g_room_grids** | **2,232** | **12.2%** | 3룸 × 24행 × 31B (null 포함) |
| 룸 메타데이터 (구조체) | ~570 | 3.1% | doors(72) + stairs(30) + boxes(330) + monsters(48) + counts(12) + names(72) + z/start(9) |
| 문자열 테이블 | ~355 | 1.9% | placed_by + box_effects + monster_names + item_names + 포인터 배열 |
| 게임 코드 (C 모듈) | ~8,700 | 47.4% | logic + render + input + help + monster + main |
| ubox.lib 링크 코드 | ~2,000 | 10.9% | VDP/키보드/스프라이트 BIOS 래퍼 |
| z88dk 런타임/CRT | ~400 | 2.2% | CRT0, stdlib 스텁 |
| **합계** | **~18,353** | **100%** | (±7 bytes 반올림 오차) |

> **핵심 관찰**: 초기화 데이터(tileset + grids + metadata + strings)가 COM의 ~40%를 차지.
> 룸 추가 시 데이터 증가가 COM 크기의 지배 요인이 된다.

#### B.0.2 TPA 한계 분석

```
MSX-DOS2 TPA 메모리 맵:
$0100 ┬─────────────────── COM 로드 시작
      │  CODE + DATA (18,346 bytes)
$47AA ├─────────────────── COM 끝
      │  BSS (178 bytes)
$485C ├─────────────────── BSS 끝
      │  (가용 힙/스택 영역)
      │  ~29,600 bytes 여유
$BFFF ┴─────────────────── TPA 상한 (page 3 시작 직전)

TPA 총 크기: $BFFF - $0100 + 1 = 48,896 bytes
현재 사용: ~18,524 bytes (COM + BSS)
현재 여유: ~30,372 bytes
```

#### B.0.3 룸 증가에 따른 COM 크기 예측

룸당 증가량:

| 항목 | 룸당 증가 |
|------|----------|
| g_room_grids | +744 bytes (24 × 31, null 포함) |
| g_room_names | +24 bytes |
| g_room_z + start_x + start_y | +3 bytes |
| g_doors (MAX_DOORS=4, DoorMsx=6B) | +24 bytes |
| g_stairs (MAX_STAIRS=2, StairMsx=5B) | +10 bytes |
| g_boxes (MAX_BOXES=10, BoxMsx=11B) | +110 bytes |
| g_monsters (MAX_MONSTERS=4, MonsterDef=4B) | +16 bytes |
| count 배열 4개 | +4 bytes |
| **룸당 총 증가** | **~935 bytes** (그리드 744 + 메타 191) |

**외부화 없는 경우** (모든 데이터 인라인):

| 룸 수 | COM 크기 (추정) | TPA 여유 | 상태 |
|--------|----------------|----------|------|
| 3 (현재) | 18,346 | 30,372 | OK |
| 10 | 24,891 | 23,827 | OK |
| 20 | 34,241 | 14,477 | 주의 |
| 30 | 43,591 | 5,127 | 위험 |
| **33** | **46,396** | **2,322** | **임계** |
| 35 | 48,261 | 457 | 거의 불가 |

**Phase 1 외부화 후** (그리드만 디스크 분리, 메타 COM 잔류):

| 룸 수 | COM 크기 (추정) | TPA 여유 | 상태 |
|--------|----------------|----------|------|
| 3 | ~16,539 | 32,179 | OK |
| 50 | ~25,543 | 23,175 | OK |
| 100 | ~35,093 | 13,625 | OK |
| 150 | ~44,643 | 4,075 | 위험 |
| **170** | **~48,463** | **255** | **임계** |

> Phase 1만으로 최대 룸 수가 33 → **~170**으로 5배 확장.

#### B.0.4 ROM 빌드 크기 예측

```
32KB ROM (32,768 bytes):
$4000 ┬─────────────────── ROM 시작
      │  CODE + DATA (15,325 bytes)
$7BED ├─────────────────── 현재 끝
      │  17,443 bytes 여유
$BFFF ┴─────────────────── ROM 끝
```

ROM은 디스크 접근 불가 → 모든 데이터 인라인 필수:

| 룸 수 | raw 크기 | ROM 여유 | 상태 |
|--------|----------|----------|------|
| 3 (현재) | 15,325 | 17,443 | OK |
| 10 | 21,870 | 10,898 | OK |
| 15 | 26,545 | 6,223 | OK |
| 20 | 31,220 | 1,548 | 위험 |
| **21** | **32,155** | **613** | **임계** |
| 22 | 33,090 | -322 | 초과 |

> 32KB ROM 최대: **~21룸**. 그 이상은 MegaROM 또는 그리드 압축 필요.

### B.1 핵심 제약

**ROM 빌드는 디스크 접근 불가** → DOS 빌드만 외부화 대상.
ROM 빌드는 현재와 동일하게 인라인 데이터 유지.

### B.1a 리소스 분류 — COM 내장 vs 디스크 분리 전체 목록

#### B.1a.1 Phase 1 분류표 (본 Appendix B 구현 범위)

| 데이터 | 바이트 | 현재 위치 | Phase 1 후 위치 | 이유 |
|--------|--------|----------|----------------|------|
| g_room_grids | 2,232 | COM (RODATA) | **디스크 (ROOM00-02)** | 룸당 744B, 최대 확장 대상 |
| 맵 타일 0-8 (pattern+color) | 144 | COM (tileset 배열 내) | **디스크 (TILES0) → 런타임 덮어쓰기** | 룸별 타일셋 교체 인프라 |
| tileset_patterns[2048] | 2,048 | COM (RODATA) | **COM 잔류 (DATA, mutable)** | 보더/폰트 기본값 보유, 맵타일 영역만 덮어쓰기 |
| tileset_colors[2048] | 2,048 | COM (RODATA) | **COM 잔류 (DATA, mutable)** | 동일 |
| 룸 메타데이터 전체 | ~570 | COM (RODATA) | **COM 잔류** | 191B/룸, 170룸까지 TPA 내 |
| 문자열 테이블 전체 | ~355 | COM (RODATA) | **COM 잔류** | 전체 공유, 증가율 낮음 |
| player_sprite | 8 | COM (RODATA) | **COM 잔류** | 단일 스프라이트 |
| 게임 코드 | ~8,700 | COM (CODE) | **COM 잔류** | 분리 불가 |
| ubox.lib + CRT | ~2,400 | COM (CODE) | **COM 잔류** | 분리 불가 |

#### B.1a.2 Phase 2 후보 (본 Appendix 범위 아님)

| 데이터 | 바이트 | Phase 2 위치 | 트리거 조건 |
|--------|--------|-------------|------------|
| tileset 전체 배열 2개 | 4,096 | 디스크 (TILESET 파일) | 복수 풀 타일셋이 필요할 때 |
| 보더 타일 (16-23) | 128 | 디스크 (TILESET에 포함) | tileset 전체 외부화 시 |
| 폰트 타일 (32-127) | 1,536 | 디스크 (TILESET에 포함) | tileset 전체 외부화 시 |

#### B.1a.3 Phase 3 후보

| 데이터 | 바이트 | Phase 3 위치 | 트리거 조건 |
|--------|--------|-------------|------------|
| g_doors + g_stairs + g_boxes + g_monsters | 룸당 ~160B | 디스크 (RMETAnn 파일) | 150룸 이상 |
| g_room_names | 룸당 24B | 디스크 (RMETAnn에 포함) | 동시 외부화 |
| 문자열 테이블 | ~355+ | 디스크 (STRINGS 파일) | 300+ 고유 문자열 |

### B.2 현재 데이터 구조 분석

#### B.2.1 그리드 데이터 (room_data.c)

```c
const char g_room_grids[ROOM_COUNT][ROOM_H][ROOM_W + 1] = {
    {   /* Room 0: Abandoned Hall */
        "##############################",
        "#...............##............#",
        /* ... 24 rows × 30 chars + null terminator */
    },
    /* ... 3 rooms total */
};
```

| 항목 | 값 |
|------|------|
| 룸 크기 | 30×24 타일 |
| 룸당 바이트 | 720 bytes (30 × 24, null terminator 제외) |
| 총 그리드 데이터 | 2,160 bytes (3룸 × 720B) |
| 배열 크기 (null 포함) | 2,232 bytes (3 × 24 × 31) |
| 문자 인코딩 | `.`=floor, `#`=wall, `@`=door, `<`=stair_dn, `>`=stair_up, `%`=box |

#### B.2.2 g_room_grids 접근점 완전 목록

| 파일:행 | 함수 | 접근 방식 | 변경 필요 |
|---------|------|----------|----------|
| logic.c:32 | `logic_get_tile()` | `g_room_grids[room][y][x]` 직접 | YES (`#ifdef MSXDOS`) |
| render.c:102 | `render_draw_map()` | `g_room_grids[gs->room]` 직접 | NO (ROM 전용 파일) |
| render_dos.c:119 | `render_draw_map()` | `g_room_grids[gs->room]` 직접 | **YES** |
| render_dos.c:42-43 | `char_to_map_tile()` | `grid[gy][gx]` 파라미터 간접 | **YES** (flat 버퍼 비호환) |
| monster.c:62,87 | `monster_update_all` 등 | `logic_get_tile()` 경유 | NO (간접참조, 변경 불필요) |

#### B.2.3 타일 데이터 (tiles.h → render.c/render_dos.c)

```c
static const uint8_t tileset_patterns[2048] = {
    TILE_PATTERNS_DATA,     /* tiles 0-31: 256 bytes */
    FONT_PATTERNS_DATA,     /* tiles 32-127: 768 bytes */
    /* tiles 128-255: zeros */
};
static const uint8_t tileset_colors[2048] = {
    TILE_COLORS_DATA,       /* tiles 0-31: 256 bytes */
    FONT_COLORS_DATA,       /* tiles 32-127: 768 bytes */
    /* tiles 128-255: zeros */
};
```

| 항목 | 값 |
|------|------|
| 맵 타일 (인덱스 0-8) | 9종 × 8B pattern + 8B color = 144 bytes |
| 몬스터 타일 (인덱스 8) | 맵 타일에 포함 |
| 보더 타일 (인덱스 16-23) | 8종 × 16B = 128 bytes (바이너리 유지) |
| 폰트 타일 (인덱스 32-127) | 96종 × 16B = 1,536 bytes (바이너리 유지) |
| VRAM 업로드 | `ubox_set_tiles()` / `ubox_set_tiles_colors()` |
| VRAM 업로드 호출 위치 | `render_init()` (render_dos.c:67-68) — **render_redraw_all()에서는 미호출** |

외부화 대상: **맵 타일 0-8** (144 bytes). 보더/폰트는 바이너리 유지.

### B.3 설계

#### B.3.1 전략: 컴파일 플래그 분기 (`-DMSXDOS`)

기존 ROM/DOS 분리 패턴 활용:

| 파일 | ROM 빌드 | DOS 빌드 |
|------|----------|----------|
| main.c | 사용 | — |
| main_dos.c | — | 사용 |
| render.c | 사용 | — |
| render_dos.c | — | 사용 |
| room_data.c | 사용 (인라인 그리드) | — |
| room_data_dos.c | — | 사용 (파일 로딩) |
| logic.c | 사용 (`#ifdef MSXDOS` 분기) | 사용 |

#### B.3.2 파일 I/O

z88dk MSX-DOS2에서 `open()/read()/close()` (`<fcntl.h>`) 사용.
z88dk의 MSX-DOS2 지원은 CP/M 호환 BDOS 기반으로 안정적일 것으로 예상.

> **참고**: Apple II에서 cc65 `<fcntl.h>` 파일 I/O가 실패하여 직접 ProDOS MLI ASM으로
> 대체한 경험이 있음. z88dk에서 동일 문제 발생 시 대안:
> 1. `fopen()/fread()/fclose()` (stdio.h)
> 2. 직접 MSX-DOS2 BDOS 호출 (function $43/$48/$45)

MSX-DOS는 CP/M 호환이므로 현재 디렉토리의 파일을 직접 이름으로 접근 가능.
Apple II처럼 볼륨 prefix 문제가 없음.

### B.4 파일 포맷

#### B.4.1 ROOMnn (그리드 바이너리)

```
파일 크기: 720 bytes (30 × 24)
레이아웃: 행 우선 (row-major), null terminator 없음

offset 0x000: row 0 (30 chars)
offset 0x01E: row 1 (30 chars)
...
offset 0x2C2: row 23 (30 chars)
```

기존 `g_room_grids`와 동일한 ASCII 인코딩 사용.

현재 각 룸의 크기:
```
ROOM00: 720 bytes (Abandoned Hall)
ROOM01: 720 bytes (Crystal Cavern)
ROOM02: 720 bytes (Shadow Labyrinth)
합계: 2,160 bytes
```

#### B.4.2 TILESn (타일셋 바이너리)

```
파일 크기: 144 bytes
레이아웃:
  [0..71]   = pattern data for tiles 0-8 (9종 × 8 bytes)
  [72..143] = color data for tiles 0-8 (9종 × 8 bytes)
```

타일 인덱스 순서:
```
[0] TILE_EMPTY     [1] TILE_FLOOR     [2] TILE_WALL
[3] TILE_DOOR_H    [4] TILE_DOOR_V    [5] TILE_STAIR_DN
[6] TILE_STAIR_UP  [7] TILE_BOX       [8] TILE_MONSTER
```

Player 스프라이트, Border 타일(16-23), Font 타일(32-127)은 바이너리 유지.

### B.5 메모리 변화 (DOS 빌드) — 정밀 분석

#### B.5.1 COM 파일 크기 변화 (항목별)

| 항목 | 바이트 변화 | 비고 |
|------|-------------|------|
| g_room_grids 제거 | **-2,232** | 3룸 × 24 × 31 RODATA 제거 |
| grid_load_room() 코드 | +~350 | 파일명 생성 + open/read/close + 캐시 로직 (z88dk open() 3인자 + fcntl/unistd 오버헤드) |
| render_load_tileset() 코드 | +~350 | 파일명 생성 + open/read/close + memcpy + VRAM 재업로드 (동일 오버헤드) |
| grid_buf_get() + char_to_map_tile 재작성 | +~80 | 시그니처 변경 + flat 버퍼 인덱싱 |
| main_dos.c 호출 추가 | +~40 | render_load_tileset 호출 3회 |
| g_room_tileset_id[3] | +3 | 1B × 3룸 |
| g_loaded_room_grid + g_loaded_tileset | +2 | 캐시 상태 변수 |
| tileset 배열 const→mutable | 0 | RODATA→DATA 이동 (크기 동일, 섹션만 변경) |
| **COM 파일 순 변화** | **~-1,012** | |

| 항목 | 바이트 변화 | 비고 |
|------|-------------|------|
| g_grid_buffer[720] BSS | +720 | 현재 룸 그리드 버퍼 (COM 파일에 불포함) |
| g_loaded_room_grid BSS | +1 | 캐시 상태 |
| **BSS 순 변화** | **+721** | |

#### B.5.2 크기 비교 요약

| 항목 | 변경 전 | 변경 후 | 차이 |
|------|---------|---------|------|
| COM 파일 크기 | 18,346 | 17,334 | **-1,012** |
| BSS 크기 | 178 | ~899 | +721 |
| 런타임 메모리 총 사용 | 18,524 | ~18,233 | -291 |
| TPA 여유 | 30,372 | ~30,663 | +291 |

#### B.5.3 룸 확장 시 COM 증가 예산 (Phase 1 후)

Phase 1 외부화 후, 룸 추가 시 COM에 남는 메타데이터만 증가:

| 항목 | 룸당 증가 |
|------|----------|
| g_room_names | +24 bytes |
| g_room_z + start_x + start_y | +3 bytes |
| g_doors (4 slots × DoorMsx=6B) | +24 bytes |
| g_stairs (2 slots × StairMsx=5B) | +10 bytes |
| g_boxes (10 slots × BoxMsx=11B) | +110 bytes |
| g_monsters (4 slots × MonsterDef=4B) | +16 bytes |
| count 배열 4개 + tileset_id | +5 bytes |
| **룸당 메타데이터 증가** | **~192 bytes** |

> g_room_grids는 디스크 파일(ROOMnn)로 분리 → COM 크기에 영향 없음.
> 744 bytes/룸 → 0 bytes/룸 절감 효과. (B.0.3 예측표 참조)

#### B.5.4 디스크 파일 크기

| 파일 | 크기 | 룸 추가 시 |
|------|------|-----------|
| ROOMnn (각) | 720 bytes | +720 bytes/룸 |
| TILESn (각) | 144 bytes | +144 bytes/신규 타일셋 |
| PROTO02.COM | 17,334 | +192 bytes/룸 (메타데이터) |

> MSX-DOS 720KB 디스크 = 737,280 bytes. 시스템 파일(~40KB) 제외 ~697KB 가용.
> ROOM 파일만으로도 ~968개 룸 수용 가능 (디스크 용량은 사실상 제약 아님).

> **const→mutable 참고**: `static const uint8_t tileset_patterns[2048]` →
> `static uint8_t tileset_patterns[2048]` 변경 시, z88dk sccz80에서 초기값이 있는
> static 배열은 DATA 섹션에 배치됨. 크기는 동일하나 런타임에 memcpy로 부분 덮어쓰기 가능.

### B.6 변경 파일 목록

| 파일 | 작업 | 빌드 대상 |
|------|------|-----------|
| `src/room_data.h` | `g_room_tileset_id[]` 선언, `#ifndef MSXDOS` 가드, DOS 전용 선언 | 공유 |
| `src/room_data.c` | `g_room_tileset_id[]` 배열 추가 | ROM (최소 변경) |
| `src/room_data_dos.c` | **신규**: 메타데이터만 + grid_load_room() | DOS |
| `src/logic.c` | `#ifdef MSXDOS` 분기 (logic_get_tile) | 공유 |
| `src/render_dos.c` | char_to_map_tile/render_draw_map 재작성, render_load_tileset() 추가, tileset 배열 mutable화 | DOS |
| `src/render.h` | `#ifdef MSXDOS` 로 render_load_tileset() 선언 추가 | 공유 |
| `src/main_dos.c` | 초기화/룸 전환 시 render_load_tileset() 호출 추가 | DOS |
| `compile_dos.sh` | `-DMSXDOS` 플래그, room_data_dos.c, 디스크에 데이터 파일 추가 | DOS |
| `tools/gen_room_bin.py` | **신규**: room_data.c → ROOM00-02 바이너리 추출 | 빌드 도구 |
| `tools/gen_tileset_msx.py` | **신규**: tiles.h → TILES0 바이너리 추출 | 빌드 도구 |
| `src/render.c` | **변경 없음** | ROM |
| `src/main.c` | **변경 없음** | ROM |
| `src/monster.c` | **변경 없음** | 공유 |
| `compile.sh` | **변경 없음** | ROM |

### B.7 코드 변경 상세

#### B.7.1 room_data.h — 가드 + 선언 추가

현재 room_data.h:50의 `extern const char g_room_grids[...]` 선언에 가드 추가,
tileset_id와 DOS 전용 선언 추가:

```c
/* --- 변경: g_room_grids 선언에 가드 추가 (line 50 근처) --- */
#ifndef MSXDOS
extern const char g_room_grids[ROOM_COUNT][ROOM_H][ROOM_W + 1];
#endif

/* --- 추가: g_room_z 선언 근처 (line 52 근처) --- */
extern const unsigned char g_room_tileset_id[ROOM_COUNT];

/* --- 추가: #endif 직전 (line 77 근처) --- */
#ifdef MSXDOS
extern char g_grid_buffer[ROOM_H * ROOM_W];
extern unsigned char g_loaded_room_grid;
extern void grid_load_room(unsigned char room);
#endif
```

**근거**:
- DOS 빌드에서 `g_room_grids`가 존재하지 않으므로 선언 자체를 `#ifndef MSXDOS`로 숨김
- `g_grid_buffer`와 `grid_load_room()`은 DOS 전용이므로 `#ifdef MSXDOS` 내
- `g_room_tileset_id`는 ROM/DOS 공통이므로 가드 없이 선언
- logic.c에서 별도 extern 선언 불필요 (이 헤더에서 모두 제공)

#### B.7.2 room_data.c — tileset_id 배열 (ROM용)

```c
/* g_room_z 정의 근처에 추가 */
const unsigned char g_room_tileset_id[ROOM_COUNT] = {0, 0, 0};
```

기존 g_room_grids 및 메타데이터는 변경 없음.

#### B.7.3 room_data_dos.c — DOS 전용 (신규)

room_data.c를 복사하되 `g_room_grids[3][24][31]` 배열 삭제.
파일 로딩 기능 추가:

```c
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

/* 현재 룸 그리드를 보유하는 런타임 버퍼 */
char g_grid_buffer[ROOM_H * ROOM_W];  /* 720 bytes BSS */
unsigned char g_loaded_room_grid = 0xFF;

void grid_load_room(unsigned char room)
{
    char fname[7];  /* "ROOM00\0" */
    int fd, n;

    if (room == g_loaded_room_grid) return;

    /* 파일명 생성: ROOM00, ROOM01, ... */
    fname[0] = 'R'; fname[1] = 'O'; fname[2] = 'O'; fname[3] = 'M';
    fname[4] = '0' + (room / 10);
    fname[5] = '0' + (room % 10);
    fname[6] = '\0';

    /* z88dk sccz80의 open()은 3인자 필수: open(fname, O_RDONLY, 0) */
    fd = open(fname, O_RDONLY, 0);
    if (fd >= 0) {
        n = read(fd, g_grid_buffer, ROOM_W * ROOM_H);
        close(fd);
        if (n == ROOM_W * ROOM_H) {
            g_loaded_room_grid = room;
            return;
        }
    }

    /* 에러 폴백: 전체 벽으로 채움 */
    memset(g_grid_buffer, '#', ROOM_W * ROOM_H);
    g_loaded_room_grid = room;
}
```

나머지 메타데이터 (doors, stairs, boxes, monsters, names, items 등)는 room_data.c와 동일.

#### B.7.4 logic.c — 조건부 그리드 접근

`logic_get_tile()` 수정. room_data.h에서 `g_grid_buffer`, `grid_load_room()` 선언이
이미 제공되므로 logic.c 상단에 별도 extern 선언 불필요:

```c
/* logic_get_tile() 수정 */
char logic_get_tile(unsigned char room, unsigned char x, unsigned char y)
{
    if (room >= ROOM_COUNT || x >= ROOM_W || y >= ROOM_H) return '#';
#ifdef MSXDOS
    grid_load_room(room);
    return g_grid_buffer[y * ROOM_W + x];
#else
    return g_room_grids[room][y][x];
#endif
}
```

`logic_try_move()`와 `logic_door_transition()`은 `logic_get_tile()`을 통해
간접적으로 그리드에 접근하므로 추가 변경 불필요.

#### B.7.5 render_dos.c — 3개 변경

**변경 1**: tileset 배열을 `const` → mutable

```c
/* 변경 전 */
static const uint8_t tileset_patterns[2048] = { ... };
static const uint8_t tileset_colors[2048] = { ... };

/* 변경 후 */
static uint8_t tileset_patterns[2048] = { ... };
static uint8_t tileset_colors[2048] = { ... };
```

**변경 2**: `char_to_map_tile()` + `render_draw_map()` 재작성

현재 `char_to_map_tile()`은 2D 배열 `const char grid[ROOM_H][ROOM_W + 1]`을
파라미터로 받고, `render_draw_map()`이 `g_room_grids[gs->room]`을 전달.
flat 버퍼 `g_grid_buffer[ROOM_H * ROOM_W]` (null terminator 없음, 30B/행)과 타입 비호환.

해결: grid 파라미터 제거, `g_grid_buffer[]` 직접 접근.

```c
#include <fcntl.h>
#include <unistd.h>

/* 헬퍼: flat 버퍼에서 (gx,gy) 위치의 문자를 반환 */
static char grid_buf_get(uint8_t gx, uint8_t gy)
{
    return g_grid_buffer[(uint16_t)gy * ROOM_W + gx];
}

/* 변경 후 char_to_map_tile — grid 파라미터 제거 */
static uint8_t char_to_map_tile(uint8_t gx, uint8_t gy)
{
    char c = grid_buf_get(gx, gy);
    switch (c) {
        case '.': return TILE_FLOOR;
        case '#': return TILE_WALL;
        case '@':
            if ((gx > 0 && grid_buf_get(gx - 1, gy) == '@') ||
                (gx < ROOM_W - 1 && grid_buf_get(gx + 1, gy) == '@'))
                return TILE_DOOR_H;
            return TILE_DOOR_V;
        case '<': return TILE_STAIR_DN;
        case '>': return TILE_STAIR_UP;
        case '%': return TILE_BOX;
        default:  return TILE_EMPTY;
    }
}

/* 변경 후 render_draw_map — g_room_grids 제거, grid_load_room 호출 */
void render_draw_map(const GameState *gs)
{
    uint8_t vx, vy;
    uint8_t gx, gy;

    grid_load_room(gs->room);  /* 캐시 히트 시 즉시 반환 */

    for (vy = 0; vy < MAP_VIEW_H; vy++) {
        gy = gs->cam_y + vy;
        for (vx = 0; vx < MAP_VIEW_W; vx++) {
            gx = gs->cam_x + vx;
            ubox_put_tile(MAP_ORIGIN_X + vx, MAP_ORIGIN_Y + vy,
                          char_to_map_tile(gx, gy));
        }
    }
}
```

> **주의**: `grid_buf_get()`에서 `(uint16_t)gy * ROOM_W` 캐스트는 Z80의 8비트 연산
> 오버플로 방지용. `gy`(최대 23) × `ROOM_W`(30) = 690 → uint8_t 범위 초과.

**변경 3**: `render_load_tileset()` 함수 추가 — VRAM 재업로드 필수

> **핵심**: `render_redraw_all()` (render_dos.c:266-277)은 `ubox_set_tiles()`를
> 호출하지 않음. `ubox_set_tiles()`는 `render_init()` (line 67)에서만 호출됨.
> 따라서 `render_load_tileset()` 내부에서 직접 VRAM 업로드해야 함.

```c
static unsigned char g_loaded_tileset = 0xFF;

void render_load_tileset(unsigned char tileset_id)
{
    unsigned char buf[144];
    char fname[7];  /* "TILES0\0" */
    int fd;

    if (tileset_id == g_loaded_tileset) return;

    fname[0] = 'T'; fname[1] = 'I'; fname[2] = 'L';
    fname[3] = 'E'; fname[4] = 'S';
    fname[5] = '0' + tileset_id;
    fname[6] = '\0';

    /* z88dk sccz80의 open()은 3인자 필수: open(fname, O_RDONLY, 0) */
    fd = open(fname, O_RDONLY, 0);
    if (fd >= 0) {
        if (read(fd, buf, 144) == 144) {
            memcpy(&tileset_patterns[0], &buf[0], 72);   /* patterns tiles 0-8 */
            memcpy(&tileset_colors[0], &buf[72], 72);     /* colors tiles 0-8 */
            g_loaded_tileset = tileset_id;
        }
        close(fd);
    }

    /* VRAM 재업로드 — render_redraw_all()은 ubox_set_tiles()를 호출하지 않으므로
     * 여기서 직접 VRAM에 반영해야 함 */
    ubox_set_tiles(tileset_patterns);
    ubox_set_tiles_colors(tileset_colors);
}
```

#### B.7.6 render.h — render_load_tileset 선언 추가

```c
/* render_cleanup() 선언 근처에 추가 */
#ifdef MSXDOS
void render_load_tileset(unsigned char tileset_id);
#endif
```

ROM 빌드에는 render_load_tileset()이 존재하지 않으므로 `#ifdef MSXDOS` 가드.
main_dos.c에서 호출 시 선언 필요.

#### B.7.7 main_dos.c — 초기화/룸 전환 호출

**초기화 시퀀스** (main() 함수 상단):

현재 순서: `logic_init → logic_update_camera → monster_init_room → render_init → render_redraw_all`

변경 후:
```c
logic_init(&g);
logic_update_camera(&g);
monster_init_room(g.room);
move_counter = 0;
render_init();                                    /* ubox_set_tiles() → VRAM 초기 업로드 */
render_load_tileset(g_room_tileset_id[g.room]);   /* ★ 추가: 디스크 로드 → VRAM 재업로드 */
render_redraw_all(&g);                            /* render_draw_map() → grid_load_room() 자동 */
```

> `render_init()`이 먼저 VRAM 초기화 (기본 타일셋 업로드).
> 직후 `render_load_tileset()`이 디스크에서 맵 타일을 로드하여 VRAM 덮어쓰기.
> `render_redraw_all()` → `render_draw_map()` → `grid_load_room(0)` 최초 호출.

**룸 전환** (do_door, do_stair):

`render_redraw_all(&g)` 직전에 `render_load_tileset()` 추가:
```c
/* do_door(): 기존 render_redraw_all(&g) (line 51) 직전에 추가 */
render_load_tileset(g_room_tileset_id[g.room]);
render_redraw_all(&g);

/* do_stair(): 기존 render_redraw_all(&g) (line 85) 직전에 추가 */
render_load_tileset(g_room_tileset_id[g.room]);
render_redraw_all(&g);
```

> grid_load_room()은 render_draw_map() 내부에서 자동 호출됨 (캐시 미스 시).
> render_load_tileset()도 캐시 체크하므로 동일 tileset_id면 디스크 접근 없음.

#### B.7.8 compile_dos.sh — 빌드 변경

```bash
# 변경 1: MSXDOS 플래그 추가
ZCCFLAGS="$ZCCFLAGS -DMSXDOS"

# 변경 2: SRCS에서 room_data.c → room_data_dos.c
SRCS="src/main_dos.c src/logic.c src/render_dos.c src/input.c src/help.c \
      src/room_data_dos.c src/monster.c"

# 변경 3: 데이터 파일 생성 단계 추가
gen_data() {
    python3 tools/gen_room_bin.py --input src/room_data.c --out-dir "$BUILD_DIR"
    python3 tools/gen_tileset_msx.py --out-dir "$BUILD_DIR"
}

# 변경 4: disk()에 데이터 파일 추가
disk() {
    # ... 기존 COM 추가 ...
    for f in "$BUILD_DIR"/ROOM??; do
        [ -f "$f" ] || continue
        fname="$(basename "$f")"
        "$RDEDISKTOOL" add "$DSK" "$f" "$fname"
    done
    for f in "$BUILD_DIR"/TILES?; do
        [ -f "$f" ] || continue
        fname="$(basename "$f")"
        "$RDEDISKTOOL" add "$DSK" "$f" "$fname"
    done
}
```

### B.8 빌드 도구

#### B.8.1 tools/gen_room_bin.py — 그리드 바이너리 추출

`src/room_data.c`를 파싱하여 `g_room_grids` 문자열 리터럴에서
각 룸의 ASCII 그리드를 추출, 행 우선 바이너리 파일로 출력.

```
입력: src/room_data.c
출력: build_dos/ROOM00 (720 bytes)
      build_dos/ROOM01 (720 bytes)
      build_dos/ROOM02 (720 bytes)
```

#### B.8.2 tools/gen_tileset_msx.py — 타일셋 바이너리 생성

`src/tiles.h`의 TILE_PATTERNS_DATA/TILE_COLORS_DATA에서
맵 타일 (인덱스 0-8) 의 패턴/색상 데이터를 추출, 바이너리 파일로 출력.

```
입력: src/tiles.h (또는 데이터 하드코딩)
출력: build_dos/TILES0 (144 bytes = 72B patterns + 72B colors)
```

### B.9 MSX-DOS 디스크 파일 구조

#### B.9.1 Phase 1 디스크 레이아웃

```
MSX-DOS2 디스크 (720KB = 737,280 bytes):
├── MSXDOS2.SYS        (MSX-DOS2 커널, ~30KB)
├── COMMAND2.COM       (명령 프로세서, ~8KB)
├── PROTO02.COM        (게임 실행파일, ~16.5KB)
├── TILES0             (타일셋 0: 맵 타일 pattern+color, 144 bytes)
├── ROOM00             (룸 0 그리드, 720 bytes)
├── ROOM01             (룸 1 그리드, 720 bytes)
└── ROOM02             (룸 2 그리드, 720 bytes)

디스크 사용: ~57.7KB (전체의 8%)
가용 잔여: ~662KB
```

#### B.9.2 50룸 확장 시 예측

```
MSX-DOS2 디스크 (720KB):
├── MSXDOS2.SYS        (~30KB)
├── COMMAND2.COM       (~8KB)
├── PROTO02.COM        (~25.5KB — 50룸 메타데이터 포함)
├── TILES0-3           (4종 타일셋, 144B × 4 = 576 bytes)
├── ROOM00-49          (50룸 그리드, 720B × 50 = 36,000 bytes ≈ 35.2KB)
└── (기타 데이터 파일)

디스크 사용: ~100KB (전체의 14%)
가용 잔여: ~620KB
```

#### B.9.3 파일명 규약

| 패턴 | 범위 | 크기 | MSX-DOS 8.3 호환 |
|------|------|------|------------------|
| ROOMnn | ROOM00-ROOM99 | 720B | Yes (6문자, 확장자 없음) |
| TILESn | TILES0-TILES9 | 144B | Yes (6문자, 확장자 없음) |

> ROOM 파일은 100개 (00-99)까지 현재 명명규칙으로 지원.
> 100룸 이상 시: `RMnnn` (5문자) 또는 서브디렉토리 사용 (MSX-DOS2 지원) 검토.

### B.10 초기화 시퀀스 (DOS 빌드)

```
main()
  ├── logic_init(&g)                           ← g.room=0, g.x/y 설정
  ├── logic_update_camera(&g)
  ├── monster_init_room(0)
  ├── render_init()
  │     └── ubox_set_tiles(tileset_patterns)   ← 컴파일 시점 기본 타일셋 VRAM 업로드
  ├── render_load_tileset(g_room_tileset_id[0]) ★
  │     ├── open("TILES0") → read(144B) → close
  │     ├── memcpy → tileset_patterns[0..71], tileset_colors[0..71]
  │     └── ubox_set_tiles() + ubox_set_tiles_colors()  ← VRAM 재업로드
  ├── render_redraw_all(&g)
  │     ├── render_draw_map(&g)
  │     │     ├── grid_load_room(0)  ← 캐시 미스
  │     │     │     └── open("ROOM00") → read(720B) → close → g_grid_buffer
  │     │     └── char_to_map_tile(gx, gy)  ← g_grid_buffer 직접 접근
  │     ├── render_draw_monsters(&g)
  │     ├── render_update_player(&g)
  │     └── render_update_status(&g)
  └── 게임 루프 진입
```

### B.10.1 룸 전환 시퀀스

```
do_door() / do_stair()
  ├── logic_door_transition(&g, di)
  │     └── logic_get_tile(target_room, ...) → grid_load_room(target_room)
  │           ← target room 그리드 자동 로드 (logic_get_tile 내부)
  ├── monster_init_room(g.room)
  ├── render_load_tileset(g_room_tileset_id[g.room])  ★
  │     ├── 캐시 히트 → 즉시 반환 (동일 tileset_id)
  │     └── 캐시 미스 → 디스크 로드 → VRAM 재업로드
  └── render_redraw_all(&g)
        └── render_draw_map(&g)
              └── grid_load_room(g.room)  ← 이미 로드됨 (캐시 히트)
```

### B.11 제약 사항

| 제약 | 설명 |
|------|------|
| ROM 빌드 불가 | ROM 카트리지는 디스크 접근 불가, 인라인 데이터 유지 |
| MSX-DOS 8.3 파일명 | ROOM00-99, TILES0-9 형식 (MSX-DOS 호환) |
| 디스크 접근 지연 | 룸 전환 시 ~0.1-0.3초 (720B 읽기, 턴 기반이므로 허용) |
| z88dk fcntl 미검증 | MSX-DOS2 BDOS 기반이므로 대부분 정상이나, 실패 시 stdio 또는 직접 BDOS 대안 |
| 코드 유지보수 | room_data.c와 room_data_dos.c 동기화 필요 (메타데이터 부분) |
| VRAM 업로드 필수 | render_load_tileset() 내부에서 ubox_set_tiles()+ubox_set_tiles_colors() 호출 필수. render_redraw_all()은 VRAM 업로드 안 함 |
| #ifdef MSXDOS 최소화 | 공유 파일 중 logic.c와 room_data.h만 `#ifdef MSXDOS` 사용. 나머지는 파일 분리로 해결 |
| z88dk open() 3인자 | z88dk sccz80의 open()은 mode 인자 필수. 2인자 호출 시 컴파일 에러. `open(fname, O_RDONLY, 0)` 형식 사용 |

### B.12 Apple II 대비 차이점

| 항목 | Apple II | MSX |
|------|----------|-----|
| 빌드 대상 | 단일 (ProDOS) | 듀얼 (ROM + DOS) |
| 그리드 형식 | 니블 패킹 + RLE 압축 | ASCII chars (비압축) |
| 그리드 크기 | 100×100 (5,000B 디컴프레스) | 30×24 (720B) |
| 타일 포맷 | 8B pattern only (7px HGR) | 8B pattern + 8B color (8px VDP) |
| 타일 로드 대상 | RAM 버퍼 직접 참조 | VRAM 업로드 (ubox_set_tiles) |
| 파일 I/O | 직접 ProDOS MLI ASM | z88dk fcntl (BDOS 래핑) |
| prefix 문제 | ON_LINE으로 절대 경로 구성 | 없음 (CP/M 현재 디렉토리) |

### B.13 ROM 빌드 영향 검증

| 파일 | ROM 빌드 영향 | 이유 |
|------|--------------|------|
| render.c | **없음** | DOS 전용 파일 render_dos.c만 변경, render.c 불변 |
| main.c | **없음** | DOS 전용 파일 main_dos.c만 변경, main.c 불변 |
| logic.c | **무영향** | `#ifdef MSXDOS` 내부만 변경, ROM 빌드에서 기존 코드 경로 유지 |
| room_data.h | **최소** | `g_room_tileset_id[]` 선언 추가. `#ifndef MSXDOS` 가드는 ROM 빌드에서 기존 g_room_grids 선언 유지 |
| room_data.c | **최소** | `g_room_tileset_id[]` 배열 추가 (3바이트) |
| monster.c | **없음** | logic_get_tile() 경유만, 직접 g_room_grids 접근 없음 |
| compile.sh | **없음** | DOS 전용 compile_dos.sh만 변경 |
| render.h | **무영향** | `#ifdef MSXDOS` 가드 내 선언만 추가 |

### B.14 불변 파일 목록

Appendix B 구현 시 **변경하지 않는** 파일:

- `src/render.c` — ROM 전용 렌더러, g_room_grids 인라인 접근 유지
- `src/main.c` — ROM 전용 메인, render_load_tileset 불필요
- `src/monster.c` — logic_get_tile() 경유만, 직접 그리드 접근 없음
- `src/engine.h` — GameState 구조체 변경 없음
- `src/input.c`, `src/input.h` — 입력 처리, 데이터 접근 없음
- `src/help.c`, `src/help.h` — 도움말 표시, 데이터 접근 없음
- `src/tiles.h` — 타일 정의 매크로, ROM/DOS 공통 유지
- `src/font.h` — 폰트 정의 매크로
- `src/logic.h` — 함수 선언만, 변경 불필요
- `src/monster.h` — 구조체/함수 선언만
- `compile.sh` — ROM 빌드 스크립트, 변경 없음

### B.15 구현 단계 계획

| Step | 내용 | 변경 파일 | 상태 |
|------|------|----------|------|
| 1 | room_data.h 가드 + 선언 추가 | `src/room_data.h` | 완료 |
| 2 | room_data.c tileset_id 배열 (ROM) | `src/room_data.c` | 완료 |
| 3 | room_data_dos.c 신규 (DOS) | `src/room_data_dos.c` | 완료 |
| 4 | logic.c 조건부 그리드 접근 | `src/logic.c` | 완료 |
| 5 | render_dos.c char_to_map_tile/render_draw_map 재작성 + render_load_tileset 추가 | `src/render_dos.c` | 완료 |
| 6 | render.h render_load_tileset 선언 | `src/render.h` | 완료 |
| 7 | main_dos.c 초기화/전환 호출 | `src/main_dos.c` | 완료 |
| 8 | gen_room_bin.py 신규 | `tools/gen_room_bin.py` | 완료 |
| 9 | gen_tileset_msx.py 신규 | `tools/gen_tileset_msx.py` | 완료 |
| 10 | compile_dos.sh 수정 | `compile_dos.sh` | 완료 |
| 11 | ROM 빌드 테스트 (compile.sh all) | — | 완료 |
| 12 | DOS 빌드 테스트 (compile_dos.sh all) | — | 완료 |

### B.16 결론

**실용적 방향은 "DOS 빌드에서 그리드+타일 외부화, ROM 빌드는 현상 유지"**.

#### Phase 1 (본 Appendix B 구현 범위) 효과 요약

| 항목 | 변경 전 | 변경 후 |
|------|---------|---------|
| COM 크기 | 18,346 bytes | 17,334 bytes (**-1,012**) |
| COM 내 데이터 비율 | ~40% | ~31% |
| 최대 룸 수 (DOS) | ~33룸 | **~155룸** |
| 디스크 접근 | 없음 | 룸 전환 시 720B 읽기 (~0.1초) |
| 룸별 타일셋 교체 | 불가 | **가능** (TILES0-9) |

> 실측 결과 반영 (2026-03-01)

#### 외부화 단계 로드맵 요약

| Phase | 트리거 | 외부화 대상 | COM 절감 | 최대 룸 수 |
|-------|--------|-----------|---------|-----------|
| **Phase 1** (본 계획) | 즉시 | 그리드 + 맵 타일 | -1,012B | ~155룸 |
| Phase 2 (B.17) | 복수 타일셋 or 150룸+ | tileset 전체 배열 | -4,096B 추가 | ~190룸 |
| Phase 3 (B.18) | 150룸+ | 룸 메타데이터 | -191B/룸 | 사실상 무제한 |

#### ROM 빌드 한계

| ROM 크기 | 최대 룸 수 | 비고 |
|----------|-----------|------|
| 32KB (표준) | ~21룸 | 현재 빌드 방식 |
| MegaROM | 수백 룸 | z88dk MegaROM 매퍼 필요 (B.19 참조) |

> Phase 1만으로도 3룸 → 155룸은 실질적으로 완성형 게임 수준의 컨텐츠량이며,
> Phase 2/3는 그 시점에 필요 여부를 재평가한다.

---

### B.17 Phase 2 외부화 로드맵 — 타일셋 전체 분리

> 이 섹션은 Phase 1 완료 후, 필요 시 구현할 추가 외부화 단계를 문서화한다.
> Phase 1의 설계와 독립적이며, Phase 1 완료 시점에 재평가한다.

#### B.17.1 동기

Phase 1 후에도 `tileset_patterns[2048]` + `tileset_colors[2048]` = **4,096 bytes**가
COM 바이너리에 남아 있다. Phase 1 후 COM의 ~25%를 차지하는 최대 단일 데이터 블록이다.

현재 tileset 배열 내부 구조:
```
[0-8]     맵 타일     : 144B (pattern 72 + color 72) — Phase 1에서 TILES0로 런타임 덮어쓰기
[9-15]    빈 갭       : 112B × 2 = 224B (제로)
[16-23]   보더 타일   : 128B (pattern 64 + color 64) — 정적, 바이너리 유지
[24-31]   빈 갭       : 128B × 2 = 256B (제로)
[32-127]  폰트 타일   : 1,536B (pattern 768 + color 768) — 정적, 바이너리 유지
[128-255] 빈          : 2,048B (제로)
```

실제 의미 있는 데이터: 144B(맵) + 128B(보더) + 1,536B(폰트) = **1,808 bytes**.
나머지 2,288 bytes는 제로 패딩.

#### B.17.2 전체 외부화 전략

배열을 초기값 없이 선언 (BSS), 부팅 시 디스크에서 전체 로드:

```c
/* Phase 2 변경 */
static uint8_t tileset_patterns[2048];  /* BSS — 제로 초기화 */
static uint8_t tileset_colors[2048];    /* BSS — 제로 초기화 */
```

디스크 파일:
```
TILESET (4,096 bytes)
  [0..2047]    = tileset_patterns 전체 (맵+보더+폰트 포함)
  [2048..4095] = tileset_colors 전체
```

`render_init()` 직후 전체 로드:
```c
void render_load_full_tileset(void) {
    int fd = open("TILESET", O_RDONLY);
    if (fd >= 0) {
        read(fd, tileset_patterns, 2048);
        read(fd, tileset_colors, 2048);
        close(fd);
    }
    ubox_set_tiles(tileset_patterns);
    ubox_set_tiles_colors(tileset_colors);
}
```

이후 룸 전환 시 기존 `render_load_tileset()`으로 맵 타일 영역만 부분 덮어쓰기.

#### B.17.3 메모리 효과

| 항목 | Phase 1 후 | Phase 2 후 | 차이 |
|------|-----------|-----------|------|
| COM DATA | tileset 4,096B | 0B | **-4,096** |
| COM BSS | grid 720B | grid 720 + tileset 4,096 = 4,816B | +4,096 (BSS) |
| COM 파일 크기 | ~16,539 | **~12,443** | **-4,096** |
| 런타임 메모리 | 동일 | 동일 | 0 (DATA→BSS 이동일 뿐) |

> COM 파일이 ~12KB로 줄어들지만, 런타임 TPA 사용량은 동일.
> BSS는 COM 로드 후 시스템이 제로 초기화. 부팅 시 4KB 디스크 읽기 추가 (~0.2초).

#### B.17.4 트리거 조건

Phase 2는 다음 중 하나 충족 시 구현:
1. 복수 풀 타일셋 (보더+폰트가 다른 셋)이 필요할 때
2. COM 크기 절감이 필요할 때 (예: 150룸+ 에서 TPA 압박)
3. 타일셋 파일을 게임 외부 도구로 편집/교체하는 워크플로우 필요 시

#### B.17.5 Phase 1 호환성

Phase 2는 Phase 1의 `render_load_tileset()` 인프라를 그대로 활용:
- Phase 1의 TILES0 (144B 맵 타일 덮어쓰기) → Phase 2에서도 동일하게 유지
- TILESET 파일은 초기 전체 로드용, TILES0은 룸별 맵 타일 교체용
- `g_room_tileset_id[]` 매핑은 동일

---

### B.18 Phase 3 외부화 로드맵 — 룸 메타데이터 분리

> 이 섹션은 150룸 이상 시나리오를 위한 최종 외부화 단계를 문서화한다.

#### B.18.1 동기

Phase 1 후 COM 내 룸 메타데이터 증가율은 **192 bytes/룸**.
150룸에서 COM이 ~44.5KB → TPA 위험구간 진입.
메타데이터를 디스크로 분리하면 룸 수 제약이 사실상 소멸.

#### B.18.2 외부화 대상

| 배열 | 룸당 크기 | 총 (150룸) |
|------|----------|-----------|
| g_doors[ROOM_COUNT][4] (DoorMsx=6B) | 24B | 3,600B |
| g_stairs[ROOM_COUNT][2] (StairMsx=5B) | 10B | 1,500B |
| g_boxes[ROOM_COUNT][10] (BoxMsx=11B) | 110B | 16,500B |
| g_monsters[ROOM_COUNT][4] (MonsterDef=4B) | 16B | 2,400B |
| count 배열 4개 | 4B | 600B |
| g_room_names | 24B | 3,600B |
| g_room_z + start_x + start_y | 3B | 450B |
| **합계** | **191B** | **28,650B** |

#### B.18.3 파일 포맷 (안)

```
RMETAnn (1파일/룸, ~191 bytes)
  [0]       door_count (1B)
  [1..24]   doors[4] × DoorMsx(6B) = 24B
  [25]      stair_count (1B)
  [26..35]  stairs[2] × StairMsx(5B) = 10B
  [36]      box_count (1B)
  [37..146] boxes[10] × BoxMsx(11B) = 110B
  [147]     monster_count (1B)
  [148..163] monsters[4] × MonsterDef(4B) = 16B
  [164..187] room_name[24]
  [188]     room_z (1B)
  [189]     start_x (1B)
  [190]     start_y (1B)
  합계: 191 bytes
```

런타임 버퍼: `g_meta_buffer[191]` (BSS, 현재 룸 메타만 보유).
접근 패턴: `g_grid_buffer`와 동일한 캐시 패턴 (룸 전환 시 로드).

#### B.18.4 구현 영향도

| 파일 | 변경 규모 | 내용 |
|------|----------|------|
| room_data_dos.c | 대규모 | 모든 const 배열 제거, meta_load_room() 추가 |
| logic.c | 중간 | g_doors/g_stairs 참조를 버퍼 경유로 변경 |
| monster.c | 중간 | g_monsters/g_monster_count 참조 변경 |
| render_dos.c | 소규모 | g_room_names 참조 변경 |
| main_dos.c | 소규모 | meta_load_room() 호출 추가 |

> **주의**: Phase 3는 공유 파일(logic.c, monster.c)의 `#ifdef MSXDOS` 사용이
> 크게 증가한다. ROM 빌드와의 코드 분기가 복잡해지므로
> logic_dos.c, monster_dos.c 파일 분리를 고려해야 한다.

#### B.18.5 트리거 조건

- 150룸 이상으로 확장 시 (COM이 ~44KB 초과 예상 시)
- Phase 1 + Phase 2를 먼저 완료한 후 검토

---

### B.19 ROM 빌드 크기 제약 분석

> ROM 빌드는 디스크 접근이 불가하므로 데이터 외부화와 독립적인 분석이 필요하다.

#### B.19.1 현재 상태

| 항목 | 값 |
|------|------|
| ROM 파일 크기 | 32,768 bytes (32KB, 패딩 포함) |
| 실제 사용량 (raw) | 15,325 bytes |
| 잔여 공간 | 17,443 bytes |
| 룸당 증가량 | ~935 bytes (그리드 744 + 메타 191) |

#### B.19.2 32KB ROM 한계 분석

ROM에서는 모든 데이터가 인라인이므로:

| 룸 수 | raw 크기 | ROM 여유 | 상태 |
|--------|----------|----------|------|
| 3 (현재) | 15,325 | 17,443 | OK |
| 10 | 21,870 | 10,898 | OK |
| 15 | 26,545 | 6,223 | OK |
| 20 | 31,220 | 1,548 | 위험 |
| **21** | **32,155** | **613** | **임계** |
| 22 | 33,090 | -322 | 초과 |

> **32KB ROM 최대: ~21룸**

#### B.19.3 32KB 초과 시 선택지

| 방안 | 최대 ROM 크기 | 구현 복잡도 | 호환성 |
|------|-------------|-----------|--------|
| MegaROM (ASC16) | 256KB (16×16KB 뱅크) | 높음 | 대부분 MSX2 |
| MegaROM (ASC8) | 2MB (256×8KB 뱅크) | 높음 | 대부분 MSX2 |
| 그리드 RLE 압축 | 32KB 유지 | 중간 | 완전 호환 |
| 그리드 크기 축소 | 32KB 유지 | 낮음 | 완전 호환 |

> ROM 빌드에서 20룸 이상이 필요할 경우, z88dk의 MegaROM 지원 또는
> 그리드 RLE 압축을 검토. 본 Appendix B 범위 밖이며 별도 분석 필요.

#### B.19.4 Phase 1이 ROM 빌드에 미치는 영향

Phase 1 구현은 ROM 빌드에 실질적 영향 없음:
- `compile.sh` 변경 없음
- ROM은 `-DMSXDOS` 플래그 없이 빌드 → 모든 `#ifdef MSXDOS` 블록 비활성
- `room_data.c`에 `g_room_tileset_id[3]` 추가 = +3 bytes (무시 가능)
- 상기 ROM 크기 예측표에 이 +3 bytes는 이미 포함
