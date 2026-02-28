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

### 4.3 맵 타일 비트패턴 (8종)

```c
// [인덱스 0] TILE_EMPTY — 빈 공간, FG=1(Black), BG=0(Transparent)
// Color: 0x10
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }  // 완전 빈 타일

// [인덱스 1] TILE_FLOOR — 희소 도트, FG=12(DarkGreen), BG=0(Transparent)
// Color: 0xC0
{ 0b00000000,  // ........
  0b00000000,  // ........
  0b00100000,  // ..#.....
  0b00000000,  // ........
  0b00000000,  // ........
  0b00000010,  // ......#.
  0b00000000,  // ........
  0b00000000 } // ........

// [인덱스 2] TILE_WALL — 벽돌, FG=14(Gray), BG=0(Transparent)
// Color: 0xE0
{ 0b11111111,  // ████████
  0b00000000,  // ........
  0b11111111,  // ████████
  0b10001000,  // █...█...
  0b11111111,  // ████████
  0b00000000,  // ........
  0b11111111,  // ████████
  0b00010001 } // ...█...█

// [인덱스 3] TILE_DOOR_H — 수평 문, FG=5(LightBlue), BG=0(Transparent)
// Color: 0x50
{ 0b00000000,  // ........
  0b11111111,  // ████████
  0b00000000,  // ........
  0b00000000,  // ........
  0b00000000,  // ........
  0b00000000,  // ........
  0b11111111,  // ████████
  0b00000000 } // ........

// [인덱스 4] TILE_DOOR_V — 수직 문, FG=5(LightBlue), BG=0(Transparent)
// Color: 0x50
{ 0b00100100,  // ..#..#..
  0b00100100,  // ..#..#..
  0b00100100,  // ..#..#..
  0b00100100,  // ..#..#..
  0b00100100,  // ..#..#..
  0b00100100,  // ..#..#..
  0b00100100,  // ..#..#..
  0b00100100 } // ..#..#..

// [인덱스 5] TILE_STAIR_DN — 하향 계단 ▼, FG=9(LightRed), BG=0(Transparent)
// Color: 0x90
{ 0b00000000,  // ........
  0b00000000,  // ........
  0b01111110,  // .######.
  0b00111100,  // ..####..
  0b00011000,  // ...##...
  0b00000000,  // ........
  0b01111110,  // .######.
  0b00000000 } // ........

// [인덱스 6] TILE_STAIR_UP — 상향 계단 ▲, FG=3(LightGreen), BG=0(Transparent)
// Color: 0x30
{ 0b00000000,  // ........
  0b01111110,  // .######.
  0b00000000,  // ........
  0b00011000,  // ...##...
  0b00111100,  // ..####..
  0b01111110,  // .######.
  0b00000000,  // ........
  0b00000000 } // ........

// [인덱스 7] TILE_BOX — 상자, FG=11(LightYellow), BG=0(Transparent)
// Color: 0xB0
{ 0b00000000,  // ........
  0b01111110,  // .######.
  0b01000010,  // .#....#.
  0b01000010,  // .#....#.
  0b01011010,  // .#.##.#.
  0b01000010,  // .#....#.
  0b01111110,  // .######.
  0b00000000 } // ........
```

### 4.4 테두리 타일 비트패턴 (8종)

모든 테두리: FG=15(White), BG=0(Transparent), Color=`0xF0`

```c
// [인덱스 16] TILE_BORDER_TL — 좌상단 ┌
{ 0b00000000,  // ........
  0b00000000,  // ........
  0b00111111,  // ..######
  0b00100000,  // ..#.....
  0b00100000,  // ..#.....
  0b00100000,  // ..#.....
  0b00100000,  // ..#.....
  0b00100000 } // ..#.....

// [인덱스 17] TILE_BORDER_TR — 우상단 ┐
{ 0b00000000,  // ........
  0b00000000,  // ........
  0b11111100,  // ######..
  0b00000100,  // .....#..
  0b00000100,  // .....#..
  0b00000100,  // .....#..
  0b00000100,  // .....#..
  0b00000100 } // .....#..

// [인덱스 18] TILE_BORDER_BL — 좌하단 └
{ 0b00100000,  // ..#.....
  0b00100000,  // ..#.....
  0b00100000,  // ..#.....
  0b00100000,  // ..#.....
  0b00100000,  // ..#.....
  0b00111111,  // ..######
  0b00000000,  // ........
  0b00000000 } // ........

// [인덱스 19] TILE_BORDER_BR — 우하단 ┘
{ 0b00000100,  // .....#..
  0b00000100,  // .....#..
  0b00000100,  // .....#..
  0b00000100,  // .....#..
  0b00000100,  // .....#..
  0b11111100,  // ######..
  0b00000000,  // ........
  0b00000000 } // ........

// [인덱스 20] TILE_BORDER_H — 수평선 ─
{ 0b00000000,  // ........
  0b00000000,  // ........
  0b11111111,  // ████████
  0b00000000,  // ........
  0b00000000,  // ........
  0b00000000,  // ........
  0b00000000,  // ........
  0b00000000 } // ........

// [인덱스 21] TILE_BORDER_V — 좌측 수직선 │ (bit5)
{ 0b00100000,  // ..#.....
  0b00100000,  // ..#.....
  0b00100000,  // ..#.....
  0b00100000,  // ..#.....
  0b00100000,  // ..#.....
  0b00100000,  // ..#.....
  0b00100000,  // ..#.....
  0b00100000 } // ..#.....

// [인덱스 22] TILE_BORDER_VR — 우측 수직선 │ (bit2)
{ 0b00000100,  // .....#..
  0b00000100,  // .....#..
  0b00000100,  // .....#..
  0b00000100,  // .....#..
  0b00000100,  // .....#..
  0b00000100,  // .....#..
  0b00000100,  // .....#..
  0b00000100 } // .....#..

// [인덱스 23] TILE_BORDER_HB — 하단 수평선 ─ (row 5, BL/BR과 정렬)
// 상단 H(row 2)와 별개 — 하단 테두리에서 BL/BR 코너와 수평선이 연결되도록 row 5에 배치
{ 0b00000000,  // ........
  0b00000000,  // ........
  0b00000000,  // ........
  0b00000000,  // ........
  0b00000000,  // ........
  0b11111111,  // ████████  (row 5 — BL/BR과 동일 위치)
  0b00000000,  // ........
  0b00000000 } // ........
```

> **테두리 정렬 참고**: 좌측 수직선(V, TL, BL)은 bit5에, 우측 수직선(VR, TR, BR)은
> bit2에 배치하여 좌우 코너와 각각 정렬되도록 한다.
> **상/하단 수평선 분리**: 상단 테두리는 H(row 2) + TL/TR(row 2)로 정렬하고,
> 하단 테두리는 HB(row 5) + BL/BR(row 5)로 정렬한다. 하나의 H 타일로 양쪽을
> 겸용하면 하단 코너와 3픽셀 어긋나므로, 반드시 HB를 별도 사용해야 한다.

### 4.5 폰트 타일 비트패턴 기준

폰트 타일은 §4.2.3의 설계 기준(5×7 글리프, 고정폭)에 따라 Claude Code가 생성한다.
여기서는 대표 문자 몇 개의 예시만 제시한다.

```c
// 폰트: 5x7 글리프 in 8x8 cell (좌측 정렬, 우측 2col + 하단 1row 비움)
// 모든 폰트 Color: 0xF0 (FG=15 White, BG=0 Transparent)

// [인덱스 65] 'A'
{ 0b01110000,  // .###....
  0b10001000,  // #...#...
  0b10001000,  // #...#...
  0b11111000,  // #####...
  0b10001000,  // #...#...
  0b10001000,  // #...#...
  0b10001000,  // #...#...
  0b00000000 } // ........

// [인덱스 48] '0'
{ 0b01110000,  // .###....
  0b10001000,  // #...#...
  0b10011000,  // #..##...
  0b10101000,  // #.#.#...
  0b11001000,  // ##..#...
  0b10001000,  // #...#...
  0b01110000,  // .###....
  0b00000000 } // ........

// [인덱스 46] '.'
{ 0b00000000,  // ........
  0b00000000,  // ........
  0b00000000,  // ........
  0b00000000,  // ........
  0b00000000,  // ........
  0b01100000,  // .##.....
  0b01100000,  // .##.....
  0b00000000 } // ........
```

> **구현 시 나머지 ~67개 글리프**는 위 규칙(5×7, 좌측 정렬, bit7~bit3 사용)에 따라
> Claude Code가 일괄 생성한다. MSX BIOS 내장 폰트 스타일을 참고하되 정확히 복제하지는 않는다.

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

### 6.3 모듈별 상세 설계

#### engine.h — 변경사항

```c
// 뷰포트 상수 변경
#define VIEW_W  10   // (기존 12)
#define VIEW_H  10   // (기존 7)

// GameState — prototype_01의 status[80] 필드 제거
// (상태 메시지는 render.c 내부 static 버퍼로 이동하여
//  도움말 복귀/룸 전환 시 render_redraw_all()에서 자동 복원)
// MoveResult 등 나머지 구조체는 prototype_01과 동일
```

#### render.c — 신규 (SCREEN 2 전용)

```c
// 초기화 — 호출 순서가 중요함 (아래 순서 준수)
// 1. ubox_set_mode(2)         — SCREEN 2 전환 (BIOS INIGRP)
// 1a. ubox_set_colors(15,1,1) — FORCLR=15(White), BAKCLR=1(Black), BDRCLR=1(Black)
//     BIOS CHGCLR(0x0062) 호출 → VDP R7 하위 4비트에 backdrop=1(Black) 설정.
//     이것이 없으면 MSX-DOS의 잔여 VDP R7 값(청/백)이 유지되어
//     color 0(Transparent)인 모든 타일 BG가 검정이 아닌 엉뚱한 색으로 표시됨.
//     (ubox 예제 게임도 set_mode 직후에 set_colors 호출)
// 2. ubox_wvdp(1, 0xE0)      — 8×8 스프라이트 오버라이드 (INIGRP 이후 필수!)
// 3. ubox_disable_screen()    — 셋업 중 깜빡임 방지
// 4. ubox_set_tiles/colors    — 타일 패턴+색상 VRAM 로드
// 5. ubox_fill_screen()       — 화면 클리어
// 6. ubox_set_sprite_pat8()   — 스프라이트 패턴 등록
// 7. render_draw_border()     — 테두리 그리기
// 8. ubox_init_isr(2)         — ISR 시작 (HTIMI 훅 설치)
// ※ enable_screen()은 여기서 호출하지 않음 — 아래 main.c 초기화 시퀀스 참조
//
// **주의: render_init()는 하드웨어 초기화만 수행하며, 화면은 OFF 상태로 유지.**
// main.c에서 render_init() 직후 render_redraw_all(&g)을 호출하여
// 맵/상태/키힌트/플레이어를 렌더링한 뒤 enable_screen()까지 처리한다.
// (render_redraw_all 내부에서 ubox_enable_screen() 호출)
//
// 만약 render_init() 내에서 enable_screen()을 호출하면:
// - 사용자는 테두리만 있는 빈 화면을 1프레임 보게 됨
// - 키 힌트(row 23)가 render_redraw_all()에서만 출력되므로 첫 프레임에 누락
// - prototype_01도 첫 render_draw()가 메인 루프 내에서 호출되나,
//   clrscr()+전체 재렌더링이므로 문제 없었음. prototype_02는 부분 갱신이므로
//   초기 화면 구성을 명시적으로 수행해야 함.
void render_init(void);

// 맵 렌더링
void render_draw_map(const GameState *gs);  // 10×10 뷰포트 타일 갱신
void render_update_player(const GameState *gs); // 플레이어 스프라이트 위치 갱신

// 텍스트 렌더링
void render_print(uint8_t x, uint8_t y, const char *text);  // 타일 기반 텍스트 출력
void render_print_n(uint8_t x, uint8_t y, const char *text, uint8_t maxlen); // 길이 제한

// UI 갱신
void render_update_status(const GameState *gs);  // 좌표, 방이름, 상태 메시지(내부 static 버퍼) 표시
    // **주의: prototype_01은 clrscr()+print_padded_line(40자 패딩)으로 이전 텍스트가
    //   자동 클리어되지만, prototype_02는 타일 기반 부분 갱신이므로 명시적 클리어 필수.**
    //
    // 내부 시퀀스:
    //   1. 상태 메시지 dirty 체크 (render_set_status()가 dirty flag 설정)
    //   2. dirty이면 render_clear_area(13, 6, 19, 5) — STATUS 영역 클리어
    //      (미클리어 시 "Enter door? 1=Yes 0=No" → "Moved." 변경 시 rows 7-9 잔상)
    //   3. sprintf(buf, "X:%3d  Y:%3d", gs->x, gs->y); render_print(13, 2, buf);
    //      → "X: 49  Y: 49" (우측정렬, §3.1 ASCII 레이아웃과 일치)
    //   4. sprintf(buf, "Z:%d", ...);  render_print(13, 3, buf);
    //   5. render_print(0, 0, room_name);
    //   6. render_print_wrap(13, 6, status_buf, 19, 4);
    //   7. dirty flag 클리어
    // z88dk 표준 라이브러리의 sprintf() 사용 (prototype_01에서도 사용 확인)
void render_set_status(const char *msg);         // 상태 메시지 설정 (내부 static char[40] 버퍼에 저장 + dirty flag 설정)
void render_clear_area(uint8_t x, uint8_t y, uint8_t w, uint8_t h); // 영역 클리어 — 내부적으로 ubox_put_tile(TILE_EMPTY=0)을 w×h회 호출
void render_redraw_all(const GameState *gs);     // 전체 화면 재렌더링 (도움말 복귀/룸 전환 시)

// 프롬프트 (prototype_01의 render_draw() 전체 갱신 대신 부분 갱신)
// render_prompt_yes_no 내부 — STATUS 영역(13,6) 사용 (짧은 확인 메시지):
//   1. render_clear_area(13, 6, 19, 5)  — 상태 영역 클리어
//   2. render_print_wrap(13, 6, msg, 19, 3) — 메시지 줄바꿈 출력
//   3. render_print(13, 9, "1=Yes 0=No")  — Y/N 안내 별도 출력
//   4. wait_yes_no()  — 키 대기
//   5. input_reset_timer()  — key_timer 리셋
// render_wait_any_key 내부 — MESSAGE 영역(0,14) 사용 (효과/아이템 등 긴 텍스트):
//   prototype_01은 단일 status line에 전체 redraw였으나,
//   prototype_02는 효과/아이템 텍스트가 54자+ 가능하므로 19열 상태 영역 대신
//   32열×8행 메시지 영역을 사용한다.
//   1. render_clear_area(0, 14, 32, 8)   — 메시지 영역 클리어
//   2. render_print_wrap(0, 14, msg, 32, 7) — 메시지 줄바꿈 출력 (32열 활용)
//   3. render_print(0, 22, "[Any key]")  — 안내 표시 (row 22, 키힌트 바로 위)
//   4. wait_any_key()  — 키 대기
//   5. render_clear_area(0, 14, 32, 9)   — 메시지 영역 + row 22 클리어 (잔상 제거)
//      (32×9 = rows 14~22, row 22의 "[Any key]" 텍스트도 포함하여 클리어)
//   6. input_reset_timer()  — key_timer 리셋
uint8_t render_prompt_yes_no(const char *msg);   // 1=예, 0=아니오 반환
void render_wait_any_key(const char *msg);        // 아무 키 대기

// 화면 전환
void render_cleanup(void);       // SCREEN 0 복귀 (종료 시)
```

#### render.c — char→tile 매핑

문(`@`)의 수평/수직 타일 구분을 위해 인접 셀을 참조하는 버전을 사용한다.
(룸 그리드에 2타일 분량의 `@`가 이미 배치되어 있으므로, 좌우 인접 확인으로 방향 판별 가능)

```c
static uint8_t char_to_map_tile(uint8_t gx, uint8_t gy, const char grid[ROOM_H][ROOM_W+1]) {
    char c = grid[gy][gx];
    switch (c) {
        case '.': return TILE_FLOOR;
        case '#': return TILE_WALL;
        case '@':
            // 좌우에도 '@'가 있으면 수평, 아니면 수직
            if ((gx > 0 && grid[gy][gx-1] == '@') ||
                (gx < ROOM_W-1 && grid[gy][gx+1] == '@'))
                return TILE_DOOR_H;
            return TILE_DOOR_V;
        case '<': return TILE_STAIR_DN;
        case '>': return TILE_STAIR_UP;
        case '%': return TILE_BOX;
        default:  return TILE_EMPTY;
    }
}
```

#### logic.c — 변경사항

- `VIEW_W`, `VIEW_H` 상수만 변경 (10×10)
- 카메라 클램핑 로직:
  ```c
  cam_x = clamp(player_x - VIEW_W/2, 0, ROOM_W - VIEW_W);  // [0, 90]
  cam_y = clamp(player_y - VIEW_H/2, 0, ROOM_H - VIEW_H);  // [0, 90]
  ```
- 나머지 로직(이동, 문/계단/상자 판정)은 동일

#### main.c — 변경사항

- `conio.h` 의존 제거
- 입력 모듈(`input.c/h`) 분리: `input_read()` 함수로 통합 (§7.3 참조)
- ubox 키보드 입력으로 교체 (`input_read()` — §7.3 참조)
- **초기화 시퀀스** (render_init → 초기 화면 구성 → 메인 루프):
  ```c
  // 초기화
  GameState g;
  uint8_t cmd;         // C89: 블록 상단 선언
  logic_init(&g);                         // 게임 상태 초기화
  render_init();                          // HW 초기화 (화면 OFF 상태 유지)
  logic_update_camera(&g);               // 초기 카메라 위치 계산
  render_redraw_all(&g);                 // 맵+상태+키힌트+플레이어 렌더링 + 화면 ON
  // ※ render_init()는 enable_screen을 호출하지 않으므로,
  //    render_redraw_all()이 최초 화면 구성과 enable_screen을 모두 처리한다.
  //    이로써 테두리만 보이는 빈 화면 프레임이 방지된다.
  ```
- **메인 루프 전체 구조** (prototype_01의 render_draw→cgetc→process 패턴 대응):
  ```c
  // prototype_01: render_draw(&g) → cgetc() → process (render-input-update)
  // prototype_02: 동일 패턴이나 non-blocking + 부분 갱신으로 전환
  while (g.running) {
      ubox_wait();                        // 1. 프레임 동기화
      cmd = input_read();                 // 2. 논블로킹 키 스캔

      // 3. 입력 처리 (이동, 상호작용, 도움말, 종료)
      if (cmd == INPUT_UP)    logic_try_move(&g, 0, -1);
      else if (cmd == INPUT_DOWN)  logic_try_move(&g, 0, 1);
      else if (cmd == INPUT_LEFT)  logic_try_move(&g, -1, 0);
      else if (cmd == INPUT_RIGHT) logic_try_move(&g, 1, 0);
      // ... (문/계단/상자 상호작용, 도움말, 종료 처리)

      // 4. 렌더링 (매 프레임, 입력 처리 후)
      logic_update_camera(&g);            // 카메라 추적
      render_draw_map(&g);                // 맵 타일 갱신 (더티 타일만, §A.5)
      render_update_player(&g);           // 스프라이트 위치
      render_update_status(&g);           // 좌표 + 상태 메시지 (dirty 시만 클리어)
  }
  ```
  > **참고**: prototype_01은 render_draw()가 clrscr()+전체 재렌더링이므로 순서가
  > 자유로우나, prototype_02는 부분 갱신이므로 "입력→상태 변경→렌더링" 순서를
  > 지켜야 변경 결과가 같은 프레임에 반영된다.
- 상호작용 프롬프트를 `render_prompt_yes_no()` 으로 통일 (§7.4 대기 함수 참조)
- **`render_set_status()` API 변경**: prototype_01의 `render_set_status(&g, msg)` →
  `render_set_status(msg)` (GameState 파라미터 제거, §6.3 engine.h 참조).
  main.c 내 13곳의 호출부 모두 수정 필요 (예: `render_set_status(&g, "Moved.")` → `render_set_status("Moved.")`)

#### help.c — 변경사항

- 텍스트 출력을 `render_print()` 으로 교체
- 스크롤 로직은 동일 (render_print로 행 단위 출력)
- 화면 전환: 도움말 진입 시 맵 영역 저장 불필요 (전체 화면 도움말 → 복귀 시 재렌더링)
- **도움말 진입 시 화면 클리어 필수**:
  prototype_01은 `help_render()` 내부에서 `clrscr()`를 호출하여 이전 화면을 클리어한다.
  prototype_02에서는 `ubox_fill_screen(0)`으로 전체 Name Table을 TILE_EMPTY(0)로 채워
  맵/상태/키힌트 등 모든 기존 타일을 제거한 뒤 도움말 텍스트를 출력해야 한다.
- **스프라이트 숨기기 필수**: 도움말 진입 시 플레이어 스프라이트를 숨겨야 한다
  (스프라이트는 타일 레이어 위에 렌더링되므로, 미숨김 시 빨간 스프라이트가 텍스트 위에 겹침).
  진입 시 `sprite_attr.y = 0xD0` 설정, 복귀 시 `render_redraw_all()` 내 `render_update_player()`가 복원.
- **도움말 진입 시퀀스**:
  1. `ubox_fill_screen(0)` — 전체 화면 클리어
  2. `sprite_attr.y = 0xD0` — 스프라이트 숨기기
  3. 도움말 텍스트 렌더링 + 스크롤 처리
  4. 복귀 시 `render_redraw_all(&g)` — 맵/상태/키힌트/플레이어 전체 복원

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

### 7.3 입력 처리 구현

```c
// input.h
#define INPUT_NONE   0
#define INPUT_UP     1
#define INPUT_DOWN   2
#define INPUT_LEFT   3
#define INPUT_RIGHT  4
#define INPUT_YES    5
#define INPUT_NO     6
#define INPUT_HELP   7
#define INPUT_QUIT   8
#define INPUT_SPACE  9

uint8_t input_read(void);
void input_reset_timer(void);  // 프롬프트 복귀 후 key_timer 리셋 (§7.4 참조)

// input.c
#include "ubox.h"

#define KEY_REPEAT_DELAY  6  // ~200ms (at 30fps = ubox_init_isr(2) with 60Hz)

static uint8_t key_timer = 0;

// key_timer 외부 리셋 함수 (프롬프트 복귀 후 즉시 입력 수신용)
void input_reset_timer(void) {
    key_timer = 0;
}

uint8_t input_read(void) {
    uint8_t row0, row2, row3, row4, row5, row8;
    uint8_t result = INPUT_NONE;

    // 키 반복 딜레이: ubox_tick 기반으로 감소
    if (key_timer > 0) {
        key_timer--;
        return INPUT_NONE;
    }

    // 필요한 행만 스캔 (6개 행)
    row0 = ubox_read_keys(0);  // 0, 1
    row2 = ubox_read_keys(2);  // A
    row3 = ubox_read_keys(3);  // D, H
    row4 = ubox_read_keys(4);  // Q
    row5 = ubox_read_keys(5);  // S, W
    row8 = ubox_read_keys(8);  // SPACE

    // 우선순위: 이동 > 상호작용 > UI
    if (row5 & UBOX_MSX_KEY_W)     result = INPUT_UP;
    else if (row5 & UBOX_MSX_KEY_S) result = INPUT_DOWN;
    else if (row2 & UBOX_MSX_KEY_A) result = INPUT_LEFT;
    else if (row3 & UBOX_MSX_KEY_D) result = INPUT_RIGHT;
    else if (row0 & 0x02)           result = INPUT_YES;   // KEY_1
    else if (row0 & 0x01)           result = INPUT_NO;    // KEY_0
    else if (row3 & UBOX_MSX_KEY_H) result = INPUT_HELP;
    else if (row4 & UBOX_MSX_KEY_Q) result = INPUT_QUIT;
    else if (row8 & UBOX_MSX_KEY_SPACE) result = INPUT_SPACE;

    if (result != INPUT_NONE)
        key_timer = KEY_REPEAT_DELAY;

    return result;
}
```

### 7.4 프롬프트 대기 함수

prototype_01의 블로킹 `cgetc()` 루프를 논블로킹 폴링으로 교체:

```c
// render_prompt_yes_no 내부 대기 루프
uint8_t wait_yes_no(void) {
    uint8_t row0;
    // 키 해제 대기 (이전 입력이 남아있을 경우 즉시 반응 방지)
    for (;;) {
        ubox_wait();
        row0 = ubox_read_keys(0);
        if (!(row0 & 0x03)) break;  // '0','1' 모두 떼짐
    }
    // 새 키 입력 대기
    for (;;) {
        ubox_wait();  // 프레임 동기화
        row0 = ubox_read_keys(0);
        if (row0 & 0x02) return 1;  // '1' = Yes
        if (row0 & 0x01) return 0;  // '0' = No
    }
}

// render_wait_any_key 내부 대기 루프
void wait_any_key(void) {
    // 현재 눌린 키가 떼질 때까지 대기
    uint8_t i;
    uint8_t any_pressed;
    for (;;) {
        ubox_wait();
        any_pressed = 0;
        for (i = 0; i < 9; i++) {
            if (ubox_read_keys(i)) { any_pressed = 1; break; }
        }
        if (!any_pressed) break;  // 모든 키 떼짐
    }
    // 새 키 입력 대기
    for (;;) {
        ubox_wait();
        for (i = 0; i < 9; i++) {
            if (ubox_read_keys(i)) return;
        }
    }
}
```

> **주의**: `wait_yes_no()`/`wait_any_key()`는 `input_read()`를 사용하지 않으므로
> 대기 중 `key_timer`가 감소하지 않는다. 프롬프트 복귀 후 잔여 타이머로 인해
> 최대 ~200ms 입력 무시가 발생할 수 있다. `render_prompt_yes_no()`와
> `render_wait_any_key()` 래퍼 함수가 리턴 직전에 `input_reset_timer()`를
> 호출하여 즉시 입력을 받을 수 있게 처리한다 (§7.3 input.h 참조).

---

## 8. 빌드 시스템

### 8.1 compile.sh

prototype_01과 동일한 z88dk 빌드 흐름을 따른다.

```bash
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
TARGET_BASE="PROTO02"
DISK_IMAGE="$BUILD_DIR/prototype_02_MSX_MSXDOS.dsk"
RDEDISKTOOL="$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool"

# z88dk 경로
if [[ -x "/opt/z88dk/bin/zcc" ]]; then
    ZCC="/opt/z88dk/bin/zcc"
else
    ZCC="$(command -v zcc 2>/dev/null || true)"
fi

# ubox 라이브러리 경로 (z88dk 포팅 버전)
UBOX_DIR="$PROJECT_ROOT/Library/MSX/ubox-msx-lib-z88dk"
UBOX_INC="$UBOX_DIR/include"
UBOX_LIB="$UBOX_DIR/lib"

# z88dk 컴파일 플래그
#   +msx -subtype=msxdos2 : MSX-DOS2 타겟 (내장 CRT, .COM 직접 생성)
#   -compiler=sccz80      : sccz80 C 컴파일러
#   -SO2                  : 최적화 레벨 2
#   -create-app           : .COM 실행파일 직접 생성
ZCCFLAGS="+msx -subtype=msxdos2 -compiler=sccz80 -SO2 -create-app"
ZCCFLAGS="$ZCCFLAGS -I$UBOX_INC -I$SCRIPT_DIR/src"
ZCCFLAGS="$ZCCFLAGS -L$UBOX_LIB -lubox"

# C 소스 파일 목록
SRCS="src/main.c src/logic.c src/render.c src/input.c src/help.c src/room_data.c"

build() {
    mkdir -p "$BUILD_DIR"
    # z88dk는 한 번의 zcc 호출로 컴파일+링크+.COM 생성
    "$ZCC" $ZCCFLAGS -o "$BUILD_DIR/$TARGET_BASE" $SRCS
    echo "Built: $BUILD_DIR/$TARGET_BASE.com ($(stat -c%s "$BUILD_DIR/$TARGET_BASE.com") bytes)"
}
```

> **핵심 차이점 (vs prototype_01)**:
> - `-I$UBOX_INC` 로 ubox 헤더 참조
> - `-L$UBOX_LIB -lubox` 로 ubox 라이브러리 링크
> - 별도 CRT0 어셈블, HEX→COM 변환 과정 불필요

**표준 타겟 및 dispatch** (prototype_01과 동일 구조):

```bash
clean() {
    rm -rf "$BUILD_DIR"
}

run_emulator() {
    "$SCRIPT_DIR/run_openmsx_prototype_02_msx_msxdos_diskaddtest.sh"
}

usage() {
    echo "Usage: $0 {clean|build|disk|test|run|all}"
}

case "${1:-all}" in
    clean) clean ;;
    build) check_z88dk; check_ubox; build ;;
    disk)  check_z88dk; check_ubox; build; create_disk ;;
    test)  echo "No automated tests yet" ;;
    run)   run_emulator ;;
    all)   clean; check_z88dk; check_ubox; build; create_disk ;;
    *)     usage; exit 1 ;;
esac
```

### 8.2 ubox 라이브러리 사전 빌드

compile.sh에서 ubox z88dk 라이브러리가 빌드되어 있는지 확인:

```bash
check_ubox() {
    if [[ ! -f "$UBOX_LIB/ubox.lib" ]]; then
        echo "Building ubox-msx-lib-z88dk..."
        make -C "$UBOX_DIR"
    fi
}
```

### 8.3 디스크 이미지 생성

MSX-DOS 2.3 부트디스크를 복사하여 사용 (prototype_01과 다른 방식):
> **참고**: prototype_01은 `rdedisktool create -f msxdsk --fs msxdos --force`로 빈 디스크를
> 생성하지만, prototype_02는 DOS 시스템 파일이 포함된 부트디스크를 복사한다.
> 부트디스크 방식은 에뮬레이터 ROM 확장에 의존하지 않아 더 자급적이다.

```bash
create_disk() {
    cp "$PROJECT_ROOT/diskwork/bootdisk/msx/msxdos23.dsk" "$DISK_IMAGE"
    # --bootdisk-mode strict: MSX-DOS 시스템 파일(MSXDOS2.SYS, COMMAND2.COM 등) 보호
    # DEVELOPER_MSX_HOWTO.md §3.2 디스크 검증 패턴 참조
    "$RDEDISKTOOL" --bootdisk-mode strict add "$DISK_IMAGE" "$BUILD_DIR/$TARGET_BASE.com"
    echo "Disk: $DISK_IMAGE"
    "$RDEDISKTOOL" list "$DISK_IMAGE"
}
```

### 8.4 에뮬레이터 실행 스크립트

`run_openmsx_prototype_02_msx_msxdos_diskaddtest.sh` — prototype_01과 동일한 패턴으로
부트디스크를 임시 경로에 복사한 뒤 .COM을 주입하고 openMSX를 실행한다.

> **참고**: `create_disk()`(§8.3)은 `build/` 디렉토리에 배포용 디스크 이미지를 생성하고,
> 이 실행 스크립트는 원본 부트디스크를 `/tmp`에 복사하여 에뮬레이터를 실행한다.
> 두 경로 모두 원본 `diskwork/bootdisk/msx/msxdos23.dsk`를 직접 수정하지 않는다.
> (DEVELOPER_MSX_HOWTO.md §5 `run_openmsx_msxdos2.sh` 사용법 참조)

```bash
#!/usr/bin/env bash
# Prototype 02 MSX/MSX-DOS launcher
#
# Flow:
#  1) (optional) build program
#  2) copy MSX-DOS boot disk to a writable work path
#  3) add compiled COM file using rdedisktool
#  4) run openMSX with the copied boot disk
#
# Environment overrides:
#   RDEDISKTOOL    — rdedisktool 바이너리 경로
#   BOOT_DISK_SRC  — 원본 부트디스크 경로 (기본: diskwork/bootdisk/msx/msxdos23.dsk)
#   PROGRAM_FILE   — 주입할 .COM 파일 경로 (기본: build/PROTO02.COM)
#   PROGRAM_NAME   — 디스크 내 파일명 (기본: PROTO02.COM)
#   WORK_DIR       — 임시 디스크 작업 디렉토리
#   AUTO_BUILD     — 1이면 실행 전 compile.sh build 자동 수행

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

first_existing_file() {
    local p; for p in "$@"; do [[ -f "$p" ]] && printf '%s\n' "$p" && return 0; done; return 1
}
first_existing_exec() {
    local p; for p in "$@"; do [[ -x "$p" ]] && printf '%s\n' "$p" && return 0; done; return 1
}

RDEDISKTOOL="${RDEDISKTOOL:-}"
BOOT_DISK_SRC="${BOOT_DISK_SRC:-}"
PROGRAM_FILE="${PROGRAM_FILE:-}"
PROGRAM_NAME="${PROGRAM_NAME:-PROTO02.COM}"
WORK_DIR="${WORK_DIR:-/tmp/prototype_02_msx_msxdos}"
AUTO_BUILD="${AUTO_BUILD:-0}"

# rdedisktool 탐색 (build_local → build → PATH)
if [[ -z "$RDEDISKTOOL" ]]; then
    RDEDISKTOOL="$(first_existing_exec \
        "$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build_local/rdedisktool" \
        "$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool" \
        "$(command -v rdedisktool 2>/dev/null || true)" \
    )" || RDEDISKTOOL="$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build_local/rdedisktool"
fi

# 부트디스크 원본 경로
if [[ -z "$BOOT_DISK_SRC" ]]; then
    BOOT_DISK_SRC="$(first_existing_file \
        "$PROJECT_ROOT/diskwork/bootdisk/msx/msxdos23.dsk" \
    )" || BOOT_DISK_SRC="$PROJECT_ROOT/diskwork/bootdisk/msx/msxdos23.dsk"
fi

# 컴파일된 프로그램 경로
if [[ -z "$PROGRAM_FILE" ]]; then
    PROGRAM_FILE="$(first_existing_file \
        "$SCRIPT_DIR/build/PROTO02.COM" \
        "$SCRIPT_DIR/build/PROTO02.com" \
    )" || PROGRAM_FILE="$SCRIPT_DIR/build/PROTO02.COM"
fi

# 자동 빌드 (선택)
if [[ "$AUTO_BUILD" == "1" ]]; then
    if [[ -x "$SCRIPT_DIR/compile.sh" ]]; then
        echo "[run] building program first (compile.sh build)"
        "$SCRIPT_DIR/compile.sh" build
    else
        echo "Error: AUTO_BUILD=1 but compile.sh is not executable"
        exit 1
    fi
fi

# 사전 조건 검증
[[ -x "$RDEDISKTOOL" ]] || { echo "Error: rdedisktool not found at $RDEDISKTOOL"; exit 1; }
[[ -f "$BOOT_DISK_SRC" ]] || { echo "Error: boot disk not found at $BOOT_DISK_SRC"; exit 1; }
[[ -f "$PROGRAM_FILE" ]] || { echo "Error: program not found at $PROGRAM_FILE (run ./compile.sh build)"; exit 1; }

# Step 1: 부트디스크를 임시 경로로 복사 (원본 보호)
mkdir -p "$WORK_DIR"
TEST_BOOT_DISK="$WORK_DIR/prototype_02_msxdos_$(date +%Y%m%d_%H%M%S).dsk"
cp "$BOOT_DISK_SRC" "$TEST_BOOT_DISK"
echo "[run] copied boot disk: $BOOT_DISK_SRC -> $TEST_BOOT_DISK"

# Step 2: 이전 버전 제거 + 새 .COM 주입
echo "[run] remove previous $PROGRAM_NAME if exists"
"$RDEDISKTOOL" --bootdisk-mode off delete "$TEST_BOOT_DISK" "$PROGRAM_NAME" >/dev/null 2>&1 || true

echo "[run] add program: $PROGRAM_FILE -> $PROGRAM_NAME"
"$RDEDISKTOOL" --bootdisk-mode strict add \
    "$TEST_BOOT_DISK" "$PROGRAM_FILE" "$PROGRAM_NAME"

# Step 3: openMSX 실행 (BOOT_DISK 환경변수로 부팅 디스크 지정)
echo "[run] launching openMSX with Disk A: $TEST_BOOT_DISK"
exec env BOOT_DISK="$TEST_BOOT_DISK" "$PROJECT_ROOT/run_openmsx_msxdos2.sh"
```

**스크립트 흐름 상세**:

| 단계 | 동작 | 원본 보호 |
|------|------|----------|
| Step 1 | `msxdos23.dsk` → `/tmp` 복사 | 원본 부트디스크 변경 없음 |
| Step 2a | `rdedisktool --bootdisk-mode off delete` | 이전 .COM 제거 (시스템 파일 무관) |
| Step 2b | `rdedisktool --bootdisk-mode strict add` | .COM 주입 (시스템 파일 덮어쓰기 차단) |
| Step 3 | `BOOT_DISK=... run_openmsx_msxdos2.sh` | 프로젝트 루트 실행 스크립트에 위임 |

> **`--bootdisk-mode` 플래그 설명** (DEVELOPER_MSX_HOWTO.md §3.2 참조):
> - `strict`: MSX-DOS 시스템 파일(MSXDOS2.SYS, COMMAND2.COM 등)을 보호.
>   동일 이름 파일 추가 시 오류 반환. 부트디스크에 사용자 파일 추가 시 안전.
> - `off`: 보호 해제. delete 명령에서 사용자 파일만 제거할 때 사용.

> **openMSX 실행 전 필요 조건** (DEVELOPER_MSX_HOWTO.md §5 참조):
> 1. openMSX 바이너리: `Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx`
> 2. 부트디스크: `diskwork/bootdisk/msx/msxdos23.dsk`
> 3. GT BIOS ROM: `~/.openMSX/share/systemroms/machines/panasonic/` 에 설치

### 8.5 빌드→디스크→실행 전체 워크플로

```
┌─────────────────────────────────────────────────────────────────┐
│  소스 코드                                                       │
│  src/main.c, logic.c, render.c, input.c, help.c, room_data.c   │
└──────────────────────┬──────────────────────────────────────────┘
                       │  ./compile.sh build
                       │  (zcc +msx -subtype=msxdos2 ... → PROTO02.COM)
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│  build/PROTO02.COM   (MSX-DOS 실행파일, ~45KB)                   │
└──────────────────────┬──────────────────────────────────────────┘
                       │  ./compile.sh disk
                       │  (cp msxdos23.dsk → build/*.dsk)
                       │  (rdedisktool --bootdisk-mode strict add)
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│  build/prototype_02_MSX_MSXDOS.dsk                              │
│  ├── MSXDOS2.SYS    (시스템)                                     │
│  ├── COMMAND2.COM    (시스템)                                     │
│  └── PROTO02.COM     (게임)                                      │
└──────────────────────┬──────────────────────────────────────────┘
                       │  ./compile.sh run
                       │  (run_openmsx_prototype_02_msx_msxdos_diskaddtest.sh)
                       │  → cp msxdos23.dsk → /tmp/
                       │  → rdedisktool add PROTO02.COM
                       │  → BOOT_DISK=/tmp/... run_openmsx_msxdos2.sh
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│  openMSX (Panasonic_FS-A1GT)                                    │
│  Disk A: /tmp/prototype_02_msxdos/prototype_02_msxdos_*.dsk     │
│                                                                  │
│  부팅 후 MSX-DOS 프롬프트에서:                                     │
│  A:\> PROTO02                                                    │
│  → SCREEN 2 전환 → 게임 실행                                      │
│  → Q키 종료 → SCREEN 0 복귀 → MSX-DOS 프롬프트                    │
└─────────────────────────────────────────────────────────────────┘
```

**일상적 개발 사이클**:

```bash
# 방법 1: 단계별 실행
./compile.sh build          # 컴파일만
./compile.sh disk           # 빌드 + 디스크 생성
./compile.sh run            # 에뮬레이터 실행

# 방법 2: 일괄 실행
./compile.sh all            # clean → build → disk

# 방법 3: 자동 빌드 + 실행 (빠른 테스트용)
AUTO_BUILD=1 ./run_openmsx_prototype_02_msx_msxdos_diskaddtest.sh
```

> **openMSX 에뮬레이터 조작** (DEVELOPER_MSX_HOWTO.md §5 참조):
> - `F9` — OSD 메뉴 (디스크 교체 등)
> - `F12` — 일시정지/재개
> - 부팅 후 `DIR` 명령으로 디스크 내 파일 목록 확인
> - `PROTO02` 입력으로 게임 실행

---

## 9. 구현 단계

### Phase 1: 기본 프레임워크 (SCREEN 2 + 타일 렌더링)

**목표**: SCREEN 2에서 타일 기반 화면 출력 확인

1. 폴더 구조 생성 (`Examples/prototype_02_MSX_MSXDOS/`)
2. ubox-msx-lib-z88dk 빌드 확인 (라이브러리 포팅 선행 완료 전제)
3. 타일 데이터 정의 (`tiles.h` — 벽, 바닥, 빈 타일)
4. 폰트 데이터 정의 (`font.h` — 숫자, 대문자 기본셋)
5. `render_init()` 구현 — SCREEN 2 전환, 타일셋 로드
6. `render_print()` 구현 — 타일 기반 문자열 출력
7. `render_draw_map()` 스텁 — 10×10 고정 패턴 출력
8. `main.c` 최소 루프 — 초기화 → 화면 출력 → 대기
9. `compile.sh` 작성 (`zcc +msx -subtype=msxdos2`) + 빌드 확인
10. openMSX에서 SCREEN 2 화면 출력 확인

**산출물**: 정적 타일 화면이 표시되는 .COM 파일

### Phase 2: 맵 렌더링 + 카메라

**목표**: 100×100 룸에서 10×10 뷰포트 렌더링

1. `room_data.c/h` 복사 (prototype_01에서)
2. `engine.h` 적용 (VIEW_W=10, VIEW_H=10)
3. `logic.c` 이식 — 카메라 로직 (VIEW 상수만 변경)
4. `render_draw_map()` 완성 — char→tile 변환 + 뷰포트 렌더링
5. 테두리 타일 렌더링
6. 좌표/룸이름 표시
7. 카메라 이동 테스트 (고정 위치에서 맵 확인)

**산출물**: 룸 맵이 10×10 뷰포트에 타일로 표시됨

### Phase 3: 플레이어 + 이동

**목표**: 키보드 입력으로 플레이어 이동

1. 스프라이트 초기화 + 플레이어 스프라이트 등록
2. 입력 처리 구현 (`ubox_read_keys` 기반)
3. `logic_try_move()` 연결
4. 카메라 추적 + 맵 재렌더링
5. 스프라이트 위치 갱신
6. 키 반복 딜레이 적용
7. 벽/상자 충돌 확인

**산출물**: WASD로 던전 탐색 가능

### Phase 4: 상호작용 (문/계단/상자)

**목표**: prototype_01과 동일한 상호작용 구현

1. 프롬프트 UI (`render_prompt_yes_no`)
2. 문(Door) 상호작용 + 룸 전환
3. 계단(Stair) 상호작용 + 층 이동
4. 상자(Box) 상호작용 + 효과/아이템 표시
5. 상태 메시지 표시 영역 구현
6. 메시지 영역 (효과 텍스트, 아이템 목록) 구현

**산출물**: 완전한 상호작용 시스템

### Phase 5: 도움말 + 마무리

**목표**: 도움말 화면 + 최종 정리

1. 도움말 화면 구현 (타일 기반 텍스트 스크롤)
2. 종료(Q) 처리 + SCREEN 0 복귀
3. 키 힌트 라인 표시
4. 엣지 케이스 테스트 (맵 경계, 룸 전환)
5. compile.sh 전체 타겟 구현 (clean, build, disk, test, run, all)
6. 에뮬레이터 실행 스크립트 작성
7. README.txt 작성

**산출물**: 완성된 prototype_02

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

### 11.1 메모리 예산 (MSX-DOS TPA ≈ 55KB)

| 영역 | 추정 크기 | 산출 근거 |
|------|----------|----------|
| 코드 (main+logic+render+input+help) | ~6KB | prototype_01 기준 + SCREEN 2 렌더러 추가분 |
| ubox.lib 링크 코드 | ~2KB | VDP/ISR/키보드 BIOS 래퍼 (Z80 asm, 소형) |
| room_data (그리드) | 30.3KB | 3 × 100 × 101 bytes (실측) |
| room_data (테이블) | ~0.9KB | Door/Stair/Box 구조체 + 문자열 테이블 |
| 타일셋 (패턴+색상) | ~4KB | 256 타일 × 16 bytes (전체 슬롯) |
| 스프라이트 데이터 | 8B | 8×8 패턴 1개 |
| 스택 + 전역변수 | ~2KB | GameState + 입력 버퍼 + 도움말 버퍼 |
| **합계** | **~45KB** | TPA 내 수용 가능 (여유 ~10KB) |

> **주의**: room_data 그리드가 전체의 ~67%를 차지한다. TPA가 부족할 경우:
> 1. 룸 3개 → 2개로 축소 (10KB 절감)
> 2. 그리드 크기 100×100 → 50×50 (22KB 절감)
> 3. RLE 압축 후 런타임 해제 (구현 복잡도 증가)
>
> **호환성**: room_data.c는 순수 ISO C89/C99 코드로, z88dk sccz80에서
> **수정 없이** 직접 컴파일 가능 (prototype_01에서 검증됨).

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

prototype_01은 40열, prototype_02는 32열 (SCREEN 2 타일 폭).

**영향받는 요소**:
- 상태 메시지 영역: 19자 폭 (col 13~31)
- 메시지 영역: 32자 폭 (col 0~31)
- 도움말: 32자 폭 (기존 40자 → 32자로 축소)

**장문 메시지 처리 — 단어 단위 줄바꿈 함수**:

```c
// 최대 width자로 줄바꿈하여 여러 줄 출력
// 반환값: 사용한 행 수
uint8_t render_print_wrap(uint8_t x, uint8_t y, const char *text,
                          uint8_t width, uint8_t max_rows) {
    uint8_t row = 0;
    uint8_t col = 0;
    const char *next_space;
    uint8_t word_len;

    while (*text && row < max_rows) {
        // 단어 끝 찾기
        next_space = text;
        while (*next_space && *next_space != ' ') next_space++;
        word_len = (uint8_t)(next_space - text);

        // 단어가 width를 초과하면 width에서 잘라냄 (방어 코드)
        if (word_len > width) word_len = width;

        if (col + word_len > width && col > 0) {
            // 다음 줄로
            row++;
            col = 0;
            if (row >= max_rows) break;  // 영역 초과 방지
        }
        // 현재 줄에 단어 출력
        render_print_n(x + col, y + row, text, word_len);
        col += word_len;
        text = next_space;
        if (*text == ' ') { text++; col++; }  // 공백 건너뛰기
    }
    // row가 max_rows에 도달하여 break된 경우 보정
    return (row >= max_rows) ? max_rows : row + 1;
}
```

> **주의**: `render_print_wrap()`은 단어 사이 공백을 타일로 출력하지 않고 `col++`만 한다.
> 따라서 출력 대상 영역을 사전에 `render_clear_area()`로 초기화해야 이전 텍스트 잔상이 남지 않는다.

**상태 영역(19자) 대응**: 긴 상태 메시지는 줄바꿈:
- `"Enter Crystal Cavern?"` (22자) → `"Enter Crystal"` + `"Cavern?"` (2줄)
- `"Open box by Goblin Trickster?"` (30자) → 2줄

**효과 메시지(32자) 대응**:
- `"Dust swirls as the lid opens. Nothing unusual happens."` (54자) → 2줄
- 메시지 영역 8행이므로 효과+아이템 모두 수용 가능

### A.4 도움말 시스템 32열 적응

prototype_01의 도움말은 `HELP_LINE_WIDTH=41` (40열+null).
prototype_02에서 변경 필요:

```c
// help.h 변경
#define HELP_LINE_WIDTH   33    // 32열 + null terminator (버퍼 크기)
#define HELP_DISPLAY_ROWS 23    // SCREEN 2: 24행 - 1행(스크롤 안내) = 23행

// help.c 변경 — WRAP_WIDTH도 반드시 변경해야 함!
// prototype_01은 HELP_LINE_WIDTH(41)과 별도로 WRAP_WIDTH(40)을 정의한다.
// wrap_and_add() 함수가 WRAP_WIDTH를 직접 참조하므로, 이 값도 32로 변경 필수.
#define WRAP_WIDTH        32    // 줄바꿈 폭 (= HELP_LINE_WIDTH - 1)
```

도움말 텍스트 중 32자 초과 행은 `wrap_and_add()`에 의해 자동 줄바꿈된다.
기존 도움말 원문 중 32자에서 부자연스럽게 잘리는 행이 없는지 텍스트 검토도 필요.

### A.5 VRAM 갱신 최적화 — 더티 타일 추적

매 프레임 100타일(10×10) 전체를 갱신하면 `ubox_put_tile()` × 100회 BIOS 호출이 발생한다.

**최적화 1: 변경분만 갱신**

```c
static uint8_t prev_map[MAP_VIEW_H][MAP_VIEW_W];

void render_draw_map(const GameState *gs) {
    uint8_t dy, dx, tile;
    for (dy = 0; dy < MAP_VIEW_H; dy++) {
        for (dx = 0; dx < MAP_VIEW_W; dx++) {
            tile = char_to_map_tile(gs->cam_x + dx, gs->cam_y + dy, ...);
            if (tile != prev_map[dy][dx]) {
                ubox_put_tile(MAP_ORIGIN_X + dx, MAP_ORIGIN_Y + dy, tile);
                prev_map[dy][dx] = tile;
            }
        }
    }
}
```

- 플레이어가 이동하지 않으면 갱신 0회
- 카메라 미이동(뷰포트 내 이동): 맵 타일 변경 없으므로 갱신 0회
- 카메라 이동 시: prev_map은 화면좌표 기반이므로 인접 타일이 동일 타입일 때만 스킵.
  동일 지형이 많은 맵(벽/바닥 반복)에서 효과적이나, worst case는 100타일 전체 갱신
- 룸 전환/도움말 복귀 시: `render_redraw_all()` 호출 → 전체 갱신 (prev_map 초기화)

**Full Redraw 시퀀스** (룸 전환, 도움말 복귀 시):
```c
void render_redraw_all(const GameState *gs) {
    ubox_disable_screen();           // 화면 OFF — 깜빡임 방지
    ubox_fill_screen(0);             // 전체 화면 클리어 (도움말 텍스트 잔상 제거)
    memset(prev_map, 0xFF, sizeof(prev_map));  // 더티 캐시 무효화
    render_draw_border();            // 테두리 재렌더링
    render_draw_map(gs);             // 맵 전체 갱신 (100타일)
    render_update_status(gs);        // 좌표, 방이름, 상태 메시지 복원
    render_print(0, 23, "WASD:move 1/0:YN H:help Q:quit");  // 키 힌트
    render_update_player(gs);        // 스프라이트 위치
    ubox_enable_screen();            // 화면 ON
}
```

**최적화 2: VBlank 중 일괄 전송**

`ubox_write_vm()`으로 Name Table(0x1800)에 직접 쓰기:

```c
// 한 행 10타일을 VRAM에 직접 쓰기
uint8_t row_buf[MAP_VIEW_W];
for (dx = 0; dx < MAP_VIEW_W; dx++)
    row_buf[dx] = char_to_map_tile(...);
ubox_write_vm((uint8_t *)(0x1800 + (MAP_ORIGIN_Y + dy) * 32 + MAP_ORIGIN_X),
              MAP_VIEW_W, row_buf);
```

이 방식이 `ubox_put_tile()` 10회 호출보다 빠르다 (BIOS 오버헤드 감소).

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

`ubox_init_isr()`는 내부적으로:
1. `HTIMI_HOOK` (0xFD9F)에 커스텀 핸들러를 등록
2. `SCNCNT` (0xF3F6) = 0 으로 설정하여 **BIOS 키보드 스캐닝을 비활성화**
3. `REPCNT` (0xF3F7) = 0 으로 설정하여 키 반복 비활성화

**MSX-DOS 영향**:
- BIOS 키보드 비활성화 후 MSX-DOS의 키 입력이 동작하지 않음
- `render_cleanup()` 에서 **BIOS 키보드를 복원해야** MSX-DOS 프롬프트가 정상 작동

```c
void render_cleanup(void) {
    // 스프라이트 숨기기 (모든 필드 명시 초기화)
    struct sprite_attr hide;
    hide.y = 0xD0;      // 종료 마커
    hide.x = 0;
    hide.pattern = 0;
    hide.attr = 0;
    ubox_set_sprite_attr(&hide, 0);

    // HTIMI 훅 복원 (필수!)
    // ubox_init_isr()가 HTIMI_HOOK(0xFD9F)에 JP ubox_isr을 설치했으므로,
    // 프로그램 종료 전 반드시 복원해야 한다. 미복원 시 TPA 재사용 후
    // VBlank 인터럽트가 쓰레기 코드로 점프하여 시스템 크래시.
    // MSX BIOS 기본 HTIMI 훅은 RET(0xC9) 한 바이트.
    __asm
    di
    ld a, #0xC9            ; RET opcode
    ld (#0xFD9F), a        ; HTIMI_HOOK 기본값 복원
    ei
    __endasm;

    // BIOS INITXT (0x006C) 호출로 TEXT 모드 + 키보드 상태 일괄 복원
    // INITXT는 SCREEN 0 전환, VDP 레지스터 초기화, SCNCNT/REPCNT 복원을 모두 처리한다.
    __asm
    call 0x006C            ; BIOS INITXT — SCREEN 0 + 키보드 복원
    __endasm;
}
```

> **폴백 (INITXT 사용 불가 시)**: `ubox_set_mode(0)` + SCNCNT(0xF3F6)=1, REPCNT(0xF3F7)=50 직접 쓰기.

### A.8 빌드 스크립트 — z88dk 설치 검증

```bash
check_z88dk() {
    if [[ -z "$ZCC" || ! -x "$ZCC" ]]; then
        echo "Error: z88dk zcc not found. Install z88dk or set path."
        echo "  Expected: /opt/z88dk/bin/zcc"
        exit 1
    fi
    echo "z88dk: $("$ZCC" --version 2>&1 | head -1)"
}
```

### A.9 compile.sh 소스 파일 목록 보강

z88dk는 단일 `zcc` 호출로 컴파일+링크를 처리한다. 소스 파일 누락 방지를 위한 체크리스트:

```bash
# 전체 소스 파일 — input.c 포함 확인
SRCS="src/main.c src/logic.c src/render.c src/input.c src/help.c src/room_data.c"

# 빌드 (CRT0은 z88dk 내장, 별도 지정 불필요)
"$ZCC" $ZCCFLAGS -o "$BUILD_DIR/$TARGET_BASE" $SRCS
```

### A.10 사전 검증 단계 (Phase 0)

> **주의**: Phase 0의 모든 단계는 `ubox-msx-lib-z88dk` 포팅이 완료된 상태를 전제한다.
> 포팅이 미완료이면 Phase 0~5 전체가 차단된다. 반드시 별도 포팅 계획을 먼저 완료할 것.

Phase 1 착수 전에 수행할 기술 검증:

1. **ubox-msx-lib-z88dk 빌드 확인**
   ```bash
   cd Library/MSX/ubox-msx-lib-z88dk && make
   ls -la lib/ubox.lib  # 파일 존재 확인
   ```

2. **최소 main() MSX-DOS 테스트** (SCREEN 2 없이)
   ```c
   // test_main.c — 최소 실행 확인
   void main(void) {
       // 아무것도 안 하고 종료
   }
   ```
   ```bash
   /opt/z88dk/bin/zcc +msx -subtype=msxdos2 -compiler=sccz80 -create-app \
       -o test test_main.c
   # → test.com 생성 확인, openMSX에서 실행
   ```

3. **SCREEN 2 전환 테스트** (ubox 연동)
   ```c
   // test_screen2.c — §6.3 render_init() 핵심 순서 검증
   // ※ 실제 render_init()는 enable_screen을 호출하지 않음 (render_redraw_all이 처리).
   //    이 테스트는 단독 실행이므로 enable_screen을 직접 호출한다.
   #include "ubox.h"
   void main(void) {
       ubox_set_mode(2);              // step 1: SCREEN 2
       ubox_set_colors(15, 1, 1);     // step 1a: backdrop=Black
       ubox_disable_screen();         // step 3: 셋업 중 깜빡임 방지
       ubox_fill_screen(0);           // step 5: 화면 클리어
       ubox_init_isr(2);              // step 8: ISR 시작
       ubox_enable_screen();          // (테스트용 — 실제 코드에서는 render_redraw_all이 호출)
       // 3초 대기 (30fps × 3초 = 90 ticks)
       ubox_wait_for(90);
       // 복귀 — HTIMI 훅 복원 필수
       __asm
       di
       ld a, #0xC9
       ld (#0xFD9F), a
       ei
       call 0x006C
       __endasm;
   }
   ```
   ```bash
   UBOX_DIR="../../Library/MSX/ubox-msx-lib-z88dk"
   /opt/z88dk/bin/zcc +msx -subtype=msxdos2 -compiler=sccz80 -create-app \
       -I$UBOX_DIR/include -L$UBOX_DIR/lib -lubox \
       -o test_s2 test_screen2.c
   # → test_s2.com을 디스크에 넣어 openMSX에서 SCREEN 2 전환 확인
   ```

4. **room_data.c 링크 테스트** (메모리 용량 확인)
   ```bash
   /opt/z88dk/bin/zcc +msx -subtype=msxdos2 -compiler=sccz80 -create-app \
       -I$UBOX_DIR/include -L$UBOX_DIR/lib -lubox \
       -o test_full test_screen2.c room_data.c
   # → .com 파일 크기 확인 (TPA 55KB 이내인지)
   ```

이 4단계를 통과하면 Phase 1 착수가 안전하다.
