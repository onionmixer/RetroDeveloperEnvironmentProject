# PLAN: prototype_02_AppleII_prodos — HGR 그래픽 모드 전환

## 1. 프로젝트 개요

`prototype_01_AppleII_prodos`의 텍스트 모드(conio.h, 40×24) 던전 탐색 게임을 Apple II HGR(Hi-Res Graphics, 280×192) 모드로 전환한다.
게임 로직(이동, 문/계단/상자/몬스터)은 동일하게 유지하고, 화면 렌더링만 타일 기반 그래픽으로 교체한다.
동시에 monolithic `game.c`(1023줄)를 MSX prototype_02 패턴에 따라 모듈 분리한다.

> Apple II는 ROM 카트리지를 사용할 수 없으므로 ProDOS 디스크 실행 전용으로 계획한다.

| 항목 | prototype_01 | prototype_02 |
|------|-------------|-------------|
| 화면 모드 | TEXT (40×24 conio) | HGR (280×192, 40×24 타일 그리드) |
| 뷰포트 | 10×10 문자 | 10×10 타일 (70×80px) |
| 플레이어 | `!` 문자 | 7×8 타일 (소프트웨어 렌더링) |
| 몬스터 | `$` 문자 | 7×8 몬스터 타일 |
| 타일 크기 | 해당 없음 | 7×8 픽셀 (HGR 1바이트 = 7px) |
| 맵 크기 | 100×100 | 100×100 (동일) |
| 맵 저장 | 인라인 `grid_packed[100][50]` | RLE 압축 (15KB→1.5KB, 90% 절감) |
| 컴파일러 | cc65 (cl65) | cc65 (cl65) |
| 그래픽 라이브러리 | conio.h | 자체 HGR 루틴 |
| 코드 구조 | game.c 모놀리식 | render/logic/monster/input/help 분리 |
| 바이너리 크기 | ~46KB (BRUN 초과) | ~33KB (BRUN 이내, HIMEM=$9600) |
| 출력 형식 | ProDOS Binary | ProDOS Binary (동일) |

---

## 2. 기술 분석

### 2.1 Apple II HGR 핵심 구조

- **해상도**: 280×192 픽셀 (40 바이트/행 × 192행)
- **타일 그리드**: 40열 × 24행 (7px × 8px 타일)
- **메모리**: Page 1 = $2000-$3FFF (8KB)
- **색상**: 비트 7이 팔레트 선택 (0=보라/초록, 1=파랑/주황), 인접 비트 ON=흰색

**HGR 바이트 구조**:
```
비트:  7  6  5  4  3  2  1  0
       │  └────────────────────── 7개 픽셀 데이터
       └─ 팔레트 선택 (0=보라/초록, 1=파랑/주황)

비트 0 = 화면상 가장 왼쪽 픽셀
비트 6 = 화면상 가장 오른쪽 픽셀
비트 7 = 팔레트 선택 (본 프로젝트에서는 항상 0)

인접 비트 ON → 흰색,  단일 비트 ON → 색상(보라/초록),  비트 OFF → 검정
```

> **색상 전략**: 비트 7=0 고정, 인접 비트를 쌍으로 켜면 흰색.
> 단일 비트만 켜면 보라(짝수 열) 또는 초록(홀수 열) — 타일 디자인 시 의도적 활용 가능.
> 본 프로젝트는 주로 흰색(인접 비트 쌍)을 사용하되, 바닥/계단 등에서 색상 비트를 활용한다.

**행 주소 인터리브**: HGR 메모리 배치가 비선형적(3 그룹 × 8 서브행 × 8 블록)
```
Row 주소 = $2000 + (group × $28) + (subrow × $400) + (eightrow × $80)
  group    = row / 64       (0-2)
  subrow   = row % 8        (0-7)
  eightrow = (row / 8) % 8  (0-7)

예시:
  Row  0 → $2000  (group=0, subrow=0, eightrow=0)
  Row  1 → $2400  (group=0, subrow=1, eightrow=0)
  Row  2 → $2800  (group=0, subrow=2, eightrow=0)
  ...
  Row  8 → $2080  (group=0, subrow=0, eightrow=1)
  Row 64 → $2028  (group=1, subrow=0, eightrow=0)
```

실용적 접근: **192 엔트리 워드 테이블** (384 바이트, souless_apple2의 videoOffset.s 참조)

**소프트 스위치** (souless_apple2 video.s 참조):

| 주소 | 이름 | 기능 | 접근 방법 |
|------|------|------|----------|
| $C000 | 80STORE_CLR | 80STORE 비활성화 (IIe/IIc) | `*(char*)0xC000 = 0` **쓰기 전용** |
| $C00C | 80COL_CLR | 80열 비활성화 (IIe/IIc) | `*(char*)0xC00C = 0` **쓰기 전용** |
| $C050 | TXTCLR | 그래픽 모드 전환 | `*(char*)0xC050 = 0` 쓰기 권장 (cc65 -O 안전) |
| $C051 | TXTSET | 텍스트 모드 복귀 | `*(char*)0xC051 = 0` 쓰기 권장 |
| $C052 | MIXCLR | 전체 화면 그래픽 | `*(char*)0xC052 = 0` 쓰기 권장 |
| $C053 | MIXSET | 혼합 모드 (하단 4줄 텍스트) | `*(char*)0xC053 = 0` 쓰기 권장 |
| $C054 | PAGE1 | 페이지 1 표시 | `*(char*)0xC054 = 0` 쓰기 권장 |
| $C057 | HIRES | 고해상도 모드 | `*(char*)0xC057 = 0` 쓰기 권장 |
| $C05F | DHIRES_CLR | Double Hi-Res 비활성화 (IIe/IIc) | `*(char*)0xC05F = 0` 쓰기 권장 |

> **HGR 모드 진입 시퀀스** (souless_apple2 video.s `init_hgr` 참조):
> ```c
> // Apple IIe/IIc 호환성: Double Hi-Res, 80열, 80STORE 비활성화
> // $C000(80STORE), $C00C(80COL)은 쓰기 전용 스위치 — 읽기로는 동작하지 않음
> *(volatile char*)0xC05F = 0;  // DHIRES_CLR — Double Hi-Res 비활성화
> *(volatile char*)0xC00C = 0;  // 80COL_CLR — 80열 비활성화
> *(volatile char*)0xC000 = 0;  // 80STORE_CLR — 80STORE 비활성화
> // HGR 모드 진입 (POKE 패턴: cc65에서 volatile 읽기는 -O 시 제거될 수 있으므로 쓰기 사용)
> *(volatile char*)0xC050 = 0;  // TXTCLR — 그래픽 모드
> *(volatile char*)0xC052 = 0;  // MIXCLR — 전체 화면
> *(volatile char*)0xC054 = 0;  // PAGE1
> *(volatile char*)0xC057 = 0;  // HIRES
> ```

### 2.2 MSX prototype_02와의 비교

| 항목 | MSX prototype_02 | Apple II prototype_02 |
|------|-------------------|----------------------|
| 화면 모드 | VDP SCREEN 2 (256×192) | HGR (280×192) |
| 타일 크기 | 8×8 픽셀 | 7×8 픽셀 (HGR 1바이트 = 7px) |
| 타일 그리드 | 32×24 | 40×24 |
| 타일 데이터 | pattern 8B + color 8B = 16B/tile | pattern 8B만 (색상은 비트 구조로 결정) |
| 타일 그리기 | `ubox_put_tile()` (VDP I/O) | `draw_tile()` (RAM 직접 쓰기) |
| 타일셋 로드 | `ubox_set_tiles()` → VRAM 3섹션 복제 | 불필요 (타일 패턴은 C 배열, 그릴 때 참조) |
| 스프라이트 | VDP 하드웨어 스프라이트 | 소프트웨어 타일 (지우기→그리기) |
| 폰트 | 타일 인덱스 32-127 매핑 | 커스텀 비트맵 폰트 (font.h, 인덱스 32-122) |
| 입력 | `ubox_read_keys()` (논블로킹 매트릭스) | `cgetc()` (블로킹, $C000/$C010) |
| 프레임 동기화 | `ubox_wait()` (VDP 인터럽트) | 불필요 (턴 기반, 블로킹 입력) |
| 룸 데이터 | 문자 그리드 `char[30][101]` | RLE 압축 니블 그리드 (런타임 $0900 디컴프레스) |
| 룸 구조 | 분리 배열 (g_room_grids, g_doors 등) | 통합 구조체 (RoomDef) |
| 몬스터 AI | 단순화 (IDLE/CHASE/RETURNING) | 전체 (PATROL/CHASE/RETURNING + LOS + 웨이포인트) |

### 2.3 souless_apple2 참조 사항

`Library/AppleII/souless_apple2/`의 주요 참조 코드:

- **`common/source/video.s`**: HGR 초기화(소프트 스위치), 화면 클리어, 페이지 전환
  - `init_hgr`: $C050/$C052/$C054/$C057 소프트 스위치 시퀀스
  - `clearScreen`: $2000-$5FFF 제로 채우기 (Page 1+2)
  - `eval_dest_addressByXY`: 행 주소 계산 (vidOffsetTbl 참조)
- **`common/source/videoOffset.s`**: 행 주소 룩업 테이블 (64 서브그룹 엔트리)
  - 본 프로젝트는 C 배열로 192 엔트리 전체를 사전 계산하여 사용
- **`common/source/joystick.s`**: 버튼 읽기 ($C061/$C062), 패들 ($FB1E PREAD)
  - 턴 기반이므로 조이스틱 대신 키보드 사용
- **`common/source/sprite.s`**: 스프라이트 렌더링 패턴 (7비트 정렬)
- **`common/source/div7Tbl.s`**: 7으로 나누기 룩업 (바이트 오프셋 변환)
  - 타일 좌표 → HGR 바이트 오프셋 변환 시 참조 가능

### 2.4 키보드 입력 (HGR 모드에서)

Apple II 키보드는 비디오 모드와 독립적인 하드웨어:
- `$C000`: 키보드 데이터 레지스터 (비트 7 = 키 사용 가능, 비트 0-6 = ASCII)
- `$C010`: 스트로브 클리어 (아무 값 쓰기)

cc65의 `cgetc()`는 $C000/$C010을 직접 읽으므로 **HGR 모드에서도 정상 동작**.
턴 기반 게임이므로 블로킹 입력(`cgetc()`)이면 충분하다.

**MSX와의 입력 모델 차이**:
- MSX prototype_02: 논블로킹 `input_read()` + `ubox_wait()` 프레임 루프 + 키 반복 딜레이
- Apple II prototype_02: 블로킹 `cgetc()` + 키 매핑 → prototype_01과 동일 패턴 유지
- Apple II는 입력 대기 중 CPU가 대기하므로 프레임 동기화가 불필요

### 2.5 Apple II prototype_01 데이터 구조 분석

#### 2.5.1 Packed Nibble Grid + RLE 압축 (vs MSX Char Grid)

prototype_01 Apple II는 100×100 맵을 **니블 패킹**으로 저장했으나,
prototype_02에서는 BASIC.SYSTEM의 BRUN 메모리 제한($9600)에 맞추기 위해
**RLE 압축**을 적용한다. 니블 패킹된 그리드를 PackBits 방식 RLE로 압축하여
ROM 데이터에 저장하고, 런타임에 $0900 버퍼로 디컴프레스한다.

```c
// engine.h — Apple II prototype_02 (RLE 압축 적용)
typedef struct {
    // ...
    const unsigned char *grid_rle;       // RLE 압축된 그리드 데이터 포인터
    unsigned int grid_rle_size;          // 압축 데이터 크기 (바이트)
    // ...
} RoomDef;
```

**RLE 형식** (PackBits 변형):
- MSB=1: 반복 실행, count = (cmd & 0x7F) + 2 (범위 2-129)
- MSB=0: 리터럴 실행, count = cmd + 1 (범위 1-128)

**압축 효과**: 3룸 그리드 15,000B → ~1,504B (90% 압축, ~13.5KB 절감)

| 항목 | Apple II (prototype_01) | Apple II (prototype_02) | MSX (prototype_02) |
|------|------------------------|------------------------|---------------------|
| 저장 형식 | `uint8_t[100][50]` (니블 패킹) | RLE 압축 포인터 + 크기 | `char[30][101]` (문자 그리드) |
| ROM 룸당 크기 | 5,000 바이트 | ~500 바이트 (RLE) | 3,030 바이트 |
| 런타임 접근 | 직접 배열 접근 | $0900 버퍼로 디컴프레스 후 접근 | 직접 문자 비교 |
| 타일 접근 | `(x&1) ? (b&0x0F) : (b>>4)` | 동일 (디컴프레스 후) | `grid[y][x]` |
| 타일 코드 | enum TileCode (0-5 숫자) | 동일 | 문자 (`.#@<>%`) |
| 맵 크기 | 100×100 | 100×100 (동일) | 30×24 |

**디컴프레스 버퍼**: $0900-$1C87 (5,000 바이트)
- STARTUP 영역($0803-$0833)과 HGR 페이지($2000) 사이의 빈 RAM 활용
- 한 번에 1개 룸만 디컴프레스, 룸 전환 시 자동 교체

**타일 코드 변환** (prototype_02에서 RLE 디컴프레스 + 버퍼 참조로 변경):
```c
#define GRID_BUFFER ((unsigned char *)0x0900)
static unsigned char g_loaded_room = 0xFF;

// RLE 디컴프레서 (PackBits 변형)
static void rle_decompress(const unsigned char *src, unsigned int src_len,
                           unsigned char *dst) { ... }

// 자동 디컴프레스 (이미 로드된 룸이면 스킵)
void logic_decompress_room(unsigned char room)
{
    if (room == g_loaded_room) return;
    rle_decompress(g_rooms[room].grid_rle, g_rooms[room].grid_rle_size,
                   GRID_BUFFER);
    g_loaded_room = room;
}

TileCode logic_get_tile_code(unsigned char room, unsigned char x, unsigned char y)
{
    unsigned char b, n;
    logic_decompress_room(room);
    b = GRID_BUFFER[(unsigned int)y * (ROOM_W / 2) + (x >> 1)];
    n = (unsigned char)((x & 1) ? (b & 0x0F) : (b >> 4));
    if (n > TILE_BOX) return TILE_WALL;
    return (TileCode)n;
}
```

> **prototype_02 결정**: Apple II prototype_02는 prototype_01의 packed nibble 형식을
> 유지하되, **RLE 압축**을 적용하여 ROM 데이터 크기를 90% 절감한다.
> 런타임에 $0900 버퍼로 자동 디컴프레스하므로, render.c/monster.c 등
> `logic_get_tile_code()`를 호출하는 코드는 변경 불필요하다.

#### 2.5.2 RoomDef 통합 구조체

Apple II prototype_02는 모든 룸 데이터를 단일 `RoomDef` 구조체에 통합.
prototype_01의 인라인 `grid_packed[100][50]` 대신 **RLE 압축 데이터 포인터**를 사용:

```c
typedef struct {
    const char *id;                          // 룸 ID 문자열
    const char *name;                        // 룸 이름 (표시용)
    signed char z_level;                     // 층 레벨
    const unsigned char *grid_rle;           // RLE 압축된 그리드 포인터
    unsigned int grid_rle_size;              // 압축 데이터 크기
    unsigned char door_count;                // 문 개수
    DoorDef doors[MAX_DOORS];                // 문 데이터
    unsigned char stair_count;               // 계단 개수
    StairDef stairs[MAX_STAIRS];             // 계단 데이터
    unsigned char box_count;                 // 상자 개수
    BoxDef boxes[MAX_BOXES];                 // 상자 데이터
    unsigned char monster_count;             // 몬스터 개수
    MonsterDef monsters[MAX_MONSTERS];       // 몬스터 데이터
    unsigned char player_start_x;            // 시작 위치
    unsigned char player_start_y;
} RoomDef;

extern const RoomDef g_rooms[ROOM_COUNT];    // room_data.c에서 정의
```

MSX prototype_02는 이를 분리 배열(`g_room_grids`, `g_doors[][]`, `g_stairs[][]` 등)로
저장하지만, Apple II는 통합 구조체를 유지한다.
그리드 데이터는 별도의 `static const unsigned char room_N_grid_rle[]` 배열로
RLE 압축되어 저장되며, RoomDef의 `grid_rle` 포인터가 이를 참조한다.

#### 2.5.3 MonsterRuntime 구조체

```c
// game.c 내 static (prototype_02에서는 monster.h로 이동)
typedef struct {
    unsigned char x;              // 현재 X 위치
    unsigned char y;              // 현재 Y 위치
    unsigned char state;          // MONSTER_PATROL / CHASE / RETURNING
    unsigned char detect_enabled; // 감지 활성화 (RETURNING 후 비활성)
    unsigned char patrol_wp_idx;  // 순찰 웨이포인트 인덱스 (0-3)
} MonsterRuntime;

typedef struct {
    MonsterRuntime monsters[MAX_MONSTERS];
} RoomRuntime;

static RoomRuntime g_runtime[ROOM_COUNT];  // 전 룸 상태 유지 (룸 전환 시 보존)
```

> **MSX와의 차이**: MSX는 `MonsterRT g_monsters_rt[MAX_MONSTERS]`로 현재 룸만 관리하고,
> `monster_init_room()`으로 룸 전환 시 재초기화한다.
> Apple II는 `RoomRuntime g_runtime[ROOM_COUNT]`로 **전 룸 상태를 유지**하므로
> 다른 방에서 돌아와도 몬스터 위치가 보존된다.

#### 2.5.4 MoveResult 열거형

```c
// game.c 내 (prototype_02에서는 engine.h로 이동)
typedef enum {
    MOVE_OK = 0,
    MOVE_BLOCKED,
    MOVE_DOOR,
    MOVE_STAIR,
    MOVE_BOX
} MoveResult;
```

> MSX prototype_02의 `MOVE_BLOCKED_BOX`, `MOVE_DOOR_PENDING`, `MOVE_STAIR_PENDING`과 대응.
> Apple II 버전은 BOX를 별도 결과로 분리하되, 이동 후가 아닌 이동 시도 시 판정.

#### 2.5.5 주요 Static 변수 (모듈 분리 시 이동 대상)

| 변수 | 현재 위치 | 이동 대상 | 설명 |
|------|----------|----------|------|
| `g_status[MSG_BUF]` | game.c static | render.c static | 상태 메시지 버퍼 |
| `g_runtime[ROOM_COUNT]` | game.c static | monster.c extern | 전 룸 몬스터 상태 |
| `g_help_scroll_offset` | game.c static | help.c static | 도움말 스크롤 위치 |
| `k_help_raw[]` | game.c static | help.c static | 도움말 텍스트 데이터 |
| `k_spaces40` | game.c static | 삭제 | HGR에서 불필요 |
| `k_tile_chars[]` | game.c static | 삭제 | 문자 모드 전용 |

---

## 3. 화면 레이아웃 설계

### 3.1 전체 레이아웃 (40×24 타일)

```
Col:  0         1         2         3
      0123456789012345678901234567890123456789

Row 0: Room: Abandoned Hall
Row 1: +---------+
Row 2: |..........|  X: 49  Y: 49
Row 3: |..........|  Z: 0
Row 4: |..........|
Row 5: |..........|
Row 6: |..........|  [상태 메시지]
Row 7: |..........|  [워드 랩핑]
Row 8: |..........|  [최대 5줄]
Row 9: |..........|
Row10: |..........|
Row11: |..........|
Row12: +---------+
Row13:
Row14: [메시지 영역 — 효과/아이템 표시]
...
Row22: [Press any key]
Row23: WASD:move H:help Q:quit
```

> **참고**: 위 ASCII 아트에서 `.`은 맵 타일 1개를 나타낸다 (10개 = 10열).
> 타일 좌표 열 0=좌측 테두리, 열 1~10=맵, 열 11=우측 테두리.
> 40열이므로 MSX의 32열보다 상태 영역이 넓음 (27열 vs 19열).

### 3.2 영역 정의

| 영역 | 타일 좌표 | 크기 | 용도 |
|------|----------|------|------|
| 룸 이름 | (0,0)-(39,0) | 40×1 | 현재 방 이름 표시 |
| 맵 테두리 | (0,1)-(11,12) | 12×12 | 맵 뷰포트 프레임 |
| 맵 내부 | (1,2)-(10,11) | 10×10 | 던전 맵 타일 |
| 좌표 표시 | (13,2)-(39,3) | 27×2 | X, Y, Z 좌표 |
| 상태 메시지 | (13,6)-(39,10) | 27×5 | 상태/프롬프트 영역 |
| 메시지 영역 | (0,14)-(39,21) | 40×8 | 효과/아이템 목록 표시 |
| 키 힌트 | (0,23)-(39,23) | 40×1 | 조작 안내 |

### 3.3 뷰포트 좌표 상수

```c
#define MAP_ORIGIN_X    1   // 맵 타일 시작 X (화면 타일 좌표)
#define MAP_ORIGIN_Y    2   // 맵 타일 시작 Y
#define MAP_VIEW_W     10   // 맵 뷰포트 너비 (타일 수)
#define MAP_VIEW_H     10   // 맵 뷰포트 높이 (타일 수)
#define BORDER_X        0   // 테두리 시작 X
#define BORDER_Y        1   // 테두리 시작 Y
#define STATUS_X       13   // 상태 메시지 시작 X
#define STATUS_Y        6   // 상태 메시지 시작 Y
#define STATUS_W       27   // 상태 메시지 폭 (40-13)
#define STATUS_H        5   // 상태 메시지 높이
#define MSG_AREA_X      0   // 메시지 영역 시작 X
#define MSG_AREA_Y     14   // 메시지 영역 시작 Y
#define MSG_AREA_W     40   // 메시지 영역 폭
#define MSG_AREA_H      8   // 메시지 영역 높이
```

---

## 4. 타일 설계

### 4.1 타일 목록

모든 타일은 7px × 8행, 타일당 8바이트. 비트 7=0 (보라/초록 팔레트, 인접 비트=흰색).

#### 맵 타일 (TileCode 인덱스 0~7)

| 인덱스 | 이름 | 패턴 설명 | 시각 특성 |
|--------|------|-----------|----------|
| 0 | TILE_FLOOR | 희소 도트 2개 | 어두운 바닥 (단일 비트=보라/초록) |
| 1 | TILE_WALL | 벽돌 패턴 | 밝은 벽 (인접 비트=흰색) |
| 2 | TILE_DOOR_H | 수평 이중선 | 밝은 문 |
| 3 | TILE_DOOR_V | 수직 이중선 | 밝은 문 |
| 4 | TILE_STAIR_DN | 하향 화살표 ▼ | 계단 |
| 5 | TILE_STAIR_UP | 상향 화살표 ▲ | 계단 |
| 6 | TILE_BOX | 작은 상자 | 닫힌 사각형 |
| 7 | TILE_EMPTY | 완전 빈 타일 | 검정 |

> **인덱스 매핑 주의**: Apple II prototype_01의 TileCode enum과 1:1 대응하되,
> 순서가 다르다 — prototype_01은 FLOOR=0, WALL=1, DOOR=2, STAIR_DOWN=3, STAIR_UP=4, BOX=5.
> prototype_02에서는 DOOR를 H/V로 분리하고 EMPTY를 추가하여 재배치한다.

#### 테두리 타일 (인덱스 8~13)

| 인덱스 | 이름 | 패턴 설명 |
|--------|------|-----------|
| 8 | TILE_BORDER_TL | 좌상단 모서리 ┌ |
| 9 | TILE_BORDER_TR | 우상단 모서리 ┐ |
| 10 | TILE_BORDER_BL | 좌하단 모서리 └ |
| 11 | TILE_BORDER_BR | 우하단 모서리 ┘ |
| 12 | TILE_BORDER_H | 수평 테두리 ─ |
| 13 | TILE_BORDER_V | 수직 테두리 │ |

#### 오브젝트 타일 (인덱스 14~15)

| 인덱스 | 이름 | 패턴 설명 |
|--------|------|-----------|
| 14 | TILE_PLAYER | 인간 실루엣 |
| 15 | TILE_MONSTER | 생물 형상 |

#### 폰트 타일 (인덱스 32~122)

| 범위 | 내용 |
|------|------|
| 32 | 공백 (SPACE) — 빈 타일 |
| 33 | `!` 느낌표 |
| 39 | `'` 아포스트로피 |
| 44 | `,` 쉼표 |
| 45 | `-` 하이픈 |
| 46 | `.` 마침표 |
| 47 | `/` 슬래시 |
| 48~57 | 숫자 `0`~`9` |
| 58 | `:` 콜론 |
| 61 | `=` 등호 |
| 63 | `?` 물음표 |
| 65~90 | 대문자 `A`~`Z` |
| 91 | `[` 좌괄호 |
| 93 | `]` 우괄호 |
| 97~122 | 소문자 `a`~`z` (대문자 패턴 복제) |

> **설계 원칙**: 타일 인덱스를 ASCII 코드와 일치시켜 문자열→타일 변환을 단순화한다.
> `tile_index = (unsigned char)character` 로 직접 매핑 가능.
> 미정의 인덱스(34~38, 40~43 등)는 빈 타일(0x00×8)로 채운다.

**게임 텍스트 점검 — 실제 사용되는 문자**:
- 룸 이름: `Abandoned Hall`, `Crystal Cavern`, `Shadow Labyrinth`
- 상태: `Moved.`, `Blocked.`, `Continue.`, `Door cancelled.`
- 프롬프트: `Enter Crystal Cavern?`, `Open box?`, `Use stairs?`
- 효과: `Dust swirls as the lid opens. Nothing unusual happens.` (54자)
- 아이템: `Gold Coin`, `Mana Crystal`, `Poison Vial`
- → 필요 특수문자: `.` `?` `!` `'` `,` `-` `:` `=` `/` `[` `]` SPACE

### 4.2 타일 데이터 생성 제약 조건

#### 4.2.1 하드웨어 제약 (Apple II HGR)

- **타일 크기**: 7px × 8행, 1타일 = pattern 8바이트 (별도 색상 데이터 없음)
- **바이트 = 픽셀**: HGR 1바이트가 화면 7픽셀에 직접 대응, 타일 너비 1바이트
- **비트 순서**: bit 0 = 좌측, bit 6 = 우측, bit 7 = 팔레트(항상 0)
- **색상 규칙** (bit 7 = 0인 경우):
  - 인접 2비트 ON → 흰색 (고휘도, 벽/테두리/문자에 사용)
  - 단일 비트 ON (짝수 열) → 보라색
  - 단일 비트 ON (홀수 열) → 초록색
  - 비트 OFF → 검정
- **유효 바이트 범위**: 0x00~0x7F (bit 7=0 고정)

#### 4.2.2 MSX 타일 시스템과의 비교

| 항목 | MSX (TMS9918A) | Apple II (HGR) |
|------|---------------|----------------|
| 타일 데이터 구성 | pattern 8B + color 8B = 16B | pattern 8B만 |
| 색상 지정 | 행별 FG/BG 2색 (color byte) | 비트 패턴으로 결정 (별도 색상 없음) |
| 타일셋 저장 | VRAM (6144B×2, 3섹션 복제) | RAM 내 C 배열 (그릴 때 참조) |
| 타일셋 로드 | `ubox_set_tiles()` 호출 1회 | 불필요 (메모리 직접 쓰기) |
| 그리기 방식 | Name Table에 타일 인덱스 쓰기 | HGR 프레임버퍼에 8바이트 직접 쓰기 |

#### 4.2.3 폰트 설계 기준

- **고정폭(monospace)**: 모든 글자가 동일한 7px×8행 셀
- **글리프 크기**: 5×7 픽셀 글리프, 좌측 정렬 (bit 0-4 사용, bit 5-6 비움)
  - 우측 2px 여백으로 타일을 나란히 놓으면 자연스러운 문자 간격 형성
  - 하단 1행은 비움 (디센더 공간 확보)
- **색상**: 글리프 비트가 인접 쌍으로 ON되면 흰색, 단일 ON이면 보라/초록
  - 5×7 폰트에서 수평 획은 대부분 인접 비트 → 흰색으로 표시
  - 수직 획의 단일 비트는 보라/초록 색상으로 표시 (수용 가능)
- **소문자**: 대문자 패턴을 그대로 복제 (소문자 전용 글리프 미설계)
- **참고**: MSX prototype_02의 font.h와 동일 글리프 디자인 사용, bit 7을 0으로 변환

#### 4.2.4 생성 방식

**Claude Code가 C 배열로 직접 생성**:
- 모든 타일(맵, 테두리, 오브젝트, 폰트)의 8바이트 비트패턴을
  C 헤더 파일(`tiles.h`, `font.h`)에 직접 정의한다.
- PNG 이미지나 외부 변환 도구를 사용하지 않는다.
- 비트 7=0을 유지하는지 반드시 확인한다 (0x00~0x7F 범위).

**생성 파일 및 대상**:

| 파일 | 내용 | 타일 수 | 바이트 |
|------|------|------:|------:|
| `tiles.h` | 맵 타일 8종 + 테두리 6종 + 오브젝트 2종 | 16 | 128 |
| `font.h` | 폰트 타일 (ASCII 32~122, 91엔트리) | 91 | 728 |
| `render.c` 내 | 전체 타일 포인터 테이블 + HGR 행 주소 테이블 | — | 384 + α |

### 4.3 맵 타일 비트패턴 (8종)

```c
// [인덱스 0] TILE_FLOOR — 희소 도트 (바닥)
// 단일 비트 → 보라/초록 점이 산포된 어두운 바닥
{ 0x00,  // .......
  0x00,  // .......
  0x04,  // ..#....   (bit2 = 3번째 픽셀)
  0x00,  // .......
  0x00,  // .......
  0x20,  // .....#.   (bit5 = 6번째 픽셀)
  0x00,  // .......
  0x00 } // .......

// [인덱스 1] TILE_WALL — 벽돌 패턴
// 인접 비트 쌍 → 흰색 벽돌, 간격 → 검정 모르타르
{ 0x7F,  // #######   (전체 흰색 — 모르타르 라인)
  0x63,  // ##...##   (좌우 벽돌)
  0x63,  // ##...##
  0x7F,  // #######   (모르타르 라인)
  0x1C,  // ..###..   (중앙 벽돌, 오프셋)
  0x1C,  // ..###..
  0x7F,  // #######   (모르타르 라인)
  0x00 } // .......

// [인덱스 2] TILE_DOOR_H — 수평 문
// 가로 이중선 패턴
{ 0x00,  // .......
  0x7F,  // #######
  0x00,  // .......
  0x00,  // .......
  0x00,  // .......
  0x00,  // .......
  0x7F,  // #######
  0x00 } // .......

// [인덱스 3] TILE_DOOR_V — 수직 문
// 세로 이중선 패턴
{ 0x14,  // ..#.#..   (bit2, bit4)
  0x14,  // ..#.#..
  0x14,  // ..#.#..
  0x14,  // ..#.#..
  0x14,  // ..#.#..
  0x14,  // ..#.#..
  0x14,  // ..#.#..
  0x14 } // ..#.#..

// [인덱스 4] TILE_STAIR_DN — 하향 계단 ▼
{ 0x00,  // .......
  0x00,  // .......
  0x3E,  // .#####.
  0x1C,  // ..###..
  0x08,  // ...#...
  0x00,  // .......
  0x3E,  // .#####.   (밑줄 강조)
  0x00 } // .......

// [인덱스 5] TILE_STAIR_UP — 상향 계단 ▲
{ 0x00,  // .......
  0x3E,  // .#####.   (윗줄 강조)
  0x00,  // .......
  0x08,  // ...#...
  0x1C,  // ..###..
  0x3E,  // .#####.
  0x00,  // .......
  0x00 } // .......

// [인덱스 6] TILE_BOX — 상자
{ 0x00,  // .......
  0x3E,  // .#####.
  0x22,  // .#...#.
  0x22,  // .#...#.
  0x2A,  // .#.#.#.   (잠금 장식)
  0x22,  // .#...#.
  0x3E,  // .#####.
  0x00 } // .......

// [인덱스 7] TILE_EMPTY — 완전 빈 타일
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
```

### 4.4 테두리 타일 비트패턴 (6종)

```c
// [인덱스 8] TILE_BORDER_TL — 좌상단 ┌
// 수직선 bit2, 수평선 bit2-6 → TR/V와 일관성 유지
{ 0x00,  // .......
  0x00,  // .......
  0x7C,  // ..#####   (bit2-6 = 수평선, TR과 동일)
  0x04,  // ..#....   (bit2 = 수직선, V와 동일)
  0x04,  // ..#....
  0x04,  // ..#....
  0x04,  // ..#....
  0x04 } // ..#....

// [인덱스 9] TILE_BORDER_TR — 우상단 ┐
{ 0x00,  // .......
  0x00,  // .......
  0x7C,  // ..#####   (bit2-6 = 우측 5px + 수평선)
  0x04,  // ..#....   (bit2 = 수직선)
  0x04,  // ..#....
  0x04,  // ..#....
  0x04,  // ..#....
  0x04 } // ..#....

// [인덱스 10] TILE_BORDER_BL — 좌하단 └
// 수직선 bit2, 수평선 bit2-6 → BR/V와 일관성 유지
{ 0x04,  // ..#....   (bit2 = 수직선, V와 동일)
  0x04,  // ..#....
  0x04,  // ..#....
  0x04,  // ..#....
  0x04,  // ..#....
  0x7C,  // ..#####   (bit2-6 = 수평선, BR과 동일)
  0x00,  // .......
  0x00 } // .......

// [인덱스 11] TILE_BORDER_BR — 우하단 ┘
{ 0x04,  // ..#....
  0x04,  // ..#....
  0x04,  // ..#....
  0x04,  // ..#....
  0x04,  // ..#....
  0x7C,  // ..#####
  0x00,  // .......
  0x00 } // .......

// [인덱스 12] TILE_BORDER_H — 수평선 ─
// 상단 테두리용 (row 2)과 하단 테두리용 (row 5) 겸용
// bit 2-6 사용 → 모서리 타일(TL/TR/BL/BR)의 수평선과 정렬
{ 0x00,  // .......
  0x00,  // .......
  0x7C,  // ..#####   (bit2-6, 모서리와 동일)
  0x00,  // .......
  0x00,  // .......
  0x7C,  // ..#####   (하단용, BL/BR row5와 정렬)
  0x00,  // .......
  0x00 } // .......

// [인덱스 13] TILE_BORDER_V — 수직선 │
// bit 2 (column 2) — 모든 모서리 타일(TL/TR/BL/BR)과 동일 위치
{ 0x04,  // ..#....   (bit2)
  0x04,  // ..#....
  0x04,  // ..#....
  0x04,  // ..#....
  0x04,  // ..#....
  0x04,  // ..#....
  0x04,  // ..#....
  0x04 } // ..#....
```

> **테두리 간소화**: MSX는 좌측 V(bit5)와 우측 VR(bit2) + 상단 H(row2)와 하단 HB(row5)를
> 별도 타일로 분리하여 8종을 사용한다. Apple II는 6종으로 통합한다.
> 모든 타일의 수직선은 **bit 2 (column 2)로 통일**, 수평선은 **bit 2-6**으로 통일하여
> 좌/우 측 모두 동일한 V 타일을 사용한다. TL/BL/TR/BR 모두 bit 2 수직선,
> bit 2-6 수평선으로 시각적 연속성이 보장된다.

### 4.5 오브젝트 타일 비트패턴 (2종)

```c
// [인덱스 14] TILE_PLAYER — 인간 실루엣
// 하드웨어 스프라이트 없으므로 맵 타일로 렌더링
{ 0x08,  // ...#...   (머리)
  0x08,  // ...#...
  0x1C,  // ..###..   (상체)
  0x2A,  // .#.#.#.   (팔)
  0x08,  // ...#...   (허리)
  0x1C,  // ..###..   (하체)
  0x14,  // ..#.#..   (다리)
  0x14 } // ..#.#..

// [인덱스 15] TILE_MONSTER — 생물 형상
{ 0x14,  // ..#.#..   (뿔/촉수)
  0x3E,  // .#####.   (머리)
  0x2A,  // .#.#.#.   (눈)
  0x3E,  // .#####.
  0x1C,  // ..###..   (몸통)
  0x2A,  // .#.#.#.   (다리)
  0x14,  // ..#.#..
  0x00 } // .......
```

### 4.6 폰트 타일 비트패턴 기준

폰트 타일은 §4.2.3의 설계 기준(5×7 글리프, 좌측 정렬)에 따라 생성한다.
여기서는 대표 문자 몇 개의 예시를 제시한다.

```c
// 폰트: 5×7 글리프 in 7×8 cell (좌측 정렬, 우측 2px + 하단 1row 비움)
// bit 0 = 좌측, bit 4 = 우측, bit 5-6 = 비움, bit 7 = 0

// [인덱스 65] 'A'
{ 0x0E,  // .###...
  0x11,  // #...#..
  0x11,  // #...#..
  0x1F,  // #####..
  0x11,  // #...#..
  0x11,  // #...#..
  0x11,  // #...#..
  0x00 } // .......

// [인덱스 48] '0' — 슬래시 장식 (상→하: 우→좌 대각선)
{ 0x0E,  // .###...
  0x11,  // #...#..
  0x19,  // #..##..   (bit0,3,4 → 우상단 대각선)
  0x15,  // #.#.#..   (bit0,2,4 → 중앙)
  0x13,  // ##..#..   (bit0,1,4 → 좌하단 대각선)
  0x11,  // #...#..
  0x0E,  // .###...
  0x00 } // .......

// [인덱스 46] '.'
{ 0x00,  // .......
  0x00,  // .......
  0x00,  // .......
  0x00,  // .......
  0x00,  // .......
  0x06,  // .##....
  0x06,  // .##....
  0x00 } // .......

// [인덱스 58] ':'
{ 0x00,  // .......
  0x06,  // .##....
  0x06,  // .##....
  0x00,  // .......
  0x00,  // .......
  0x06,  // .##....
  0x06,  // .##....
  0x00 } // .......

// [인덱스 63] '?'
{ 0x0E,  // .###...
  0x11,  // #...#..
  0x10,  // ....#..
  0x08,  // ...#...
  0x04,  // ..#....
  0x00,  // .......
  0x04,  // ..#....
  0x00 } // .......
```

> **구현 시 나머지 ~68개 글리프**는 위 규칙(5×7, bit 0-4, bit 7=0)에 따라
> Claude Code가 일괄 생성한다. MSX prototype_02의 font.h 패턴을 참조하되,
> MSX의 bit 7(MSB)=좌측 → Apple II의 bit 0(LSB)=좌측으로 비트 순서를 반전한다.

---

## 5. 모듈 아키텍처

### 5.1 파일 구조

```
Examples/prototype_02_AppleII_prodos/
├── src/
│   ├── main.c          — 진입점 + 게임 루프
│   ├── render.h        — 렌더링 인터페이스 (MSX와 동일 API)
│   ├── render.c        — HGR 렌더링 구현
│   ├── logic.h         — 게임 로직 인터페이스
│   ├── logic.c         — 이동, 카메라, 문/계단/상자 전환
│   ├── monster.h       — 몬스터 AI 인터페이스
│   ├── monster.c       — 몬스터 FSM (PATROL/CHASE/RETURNING)
│   ├── input.h         — 입력 인터페이스
│   ├── input.c         — 키보드 읽기 (cgetc 래퍼)
│   ├── help.h          — 도움말 인터페이스
│   ├── help.c          — HGR 도움말 화면
│   ├── engine.h        — 공유 타입, 상수, MoveResult
│   ├── room_data.h     — 룸 데이터 선언
│   ├── room_data.c     — 룸 그리드 + 메타데이터 (생성됨)
│   ├── tiles.h         — HGR 타일 패턴 (7×8, 16종)
│   └── font.h          — HGR 폰트 패턴 (ASCII 32-122, 91엔트리)
├── tools/
│   ├── json_to_room_data.py  — 데이터 생성기 (prototype_01에서 복사)
│   └── check_env.sh          — 환경 검증
├── compile.sh
├── run_applewin_prodos.sh
└── build/
```

### 5.2 game.c 분해 매핑

prototype_01의 `game.c` (1023줄)에서 각 모듈로 추출:

| game.c 함수 | 줄 | 이동 대상 | 변경 사항 |
|------------|-----|----------|----------|
| `logic_get_tile_code()` | 82-88 | logic.c → `logic_get_tile_code` | 1st param: `const RoomDef *room` → `unsigned char room` (인덱스) |
| `room_tile_char()` | 90-93 | 삭제 | HGR에서 불필요 |
| `update_camera()` | 532-544 | logic.c → `logic_update_camera` | GameState 포인터 인자 |
| `try_move()` | 738-758 | logic.c → `logic_try_move` | MoveResult 반환 |
| `get_door_at()` | 685-703 | logic.c → `logic_find_door` | DoorDef 포인터 반환 |
| `get_stair_at()` | 705-716 | logic.c → `logic_find_stair` | StairDef 포인터 반환 |
| `get_box_at()` | 718-736 | logic.c → `logic_find_box` | BoxDef 포인터 반환 |
| `do_door_transition()` | 760-802 | logic.c → `logic_do_door` | 반환 0/1 |
| `do_stair_transition()` | 804-815 | logic.c → `logic_do_stair` | 반환 0/1 |
| `box_interaction()` | 817-846 | main.c (UI) | render 함수 사용 |
| `abs_i()`, `has_line_of_sight()` | 269-332 | monster.c | 내부 함수 |
| `get_patrol_waypoint()` | 334-359 | monster.c | 내부 함수 |
| `monster_*()` 전체 | 361-505 | monster.c | 외부 인터페이스 분리 |
| `init_runtime()` | 491-505 | monster.c → `monster_init_all` | 전 룸 초기화 |
| `render_*()` 전체 | 546-631 | render.c | 완전 재작성 (HGR) |
| `set_status()` | 211-219 | render.c → `render_set_status` | 내부 static 버퍼 |
| `set_encounter_status()` | 221-242 | monster.c → `monster_format_encounter_msg` | 메시지 포맷 분리, main.c에서 호출 |
| `k_help_raw[]`, `help_*()` | 33-209 | help.c | 텍스트+스크롤 로직 |
| `show_help()` | 662-683 | help.c → `help_show` | HGR 렌더링 |
| `prompt_yes_no()` | 633-648 | render.c → `render_prompt_yes_no` | HGR 텍스트 + cgetc |
| `wait_any_key_msg()` | 650-660 | render.c → `render_wait_any_key` | HGR 텍스트 + cgetc |
| `prompt_quit()` | 848-862 | main.c | render_prompt_yes_no 사용 |
| `game_run()` 루프 | 864-1023 | main.c → `main` | 분해 |

### 5.3 모듈 의존 관계

```
main.c
  ├── engine.h (상수, GameState, MoveResult, 타입)
  ├── logic.c/h (게임 로직)
  │   ├── engine.h
  │   └── room_data.c/h (맵 데이터)
  ├── render.c/h (HGR 렌더링)
  │   ├── engine.h
  │   ├── tiles.h (타일 비트패턴)
  │   ├── font.h (폰트 비트패턴)
  │   ├── room_data.h (render_draw_map이 룸 그리드 참조)
  │   ├── logic.h (tile_code_to_pattern → logic_get_tile_code 호출)
  │   ├── monster.h (render_draw_monsters → monster_index_at 호출)
  │   └── input.h (render_prompt_yes_no, render_wait_any_key에서 사용)
  ├── input.c/h (키보드 입력)
  ├── monster.c/h (몬스터 AI)
  │   ├── engine.h
  │   └── room_data.h (MonsterDef, g_rooms 참조)
  └── help.c/h (도움말 화면)
      ├── render.c/h (텍스트 출력 함수 공유)
      └── input.h (스크롤 키 입력)
```

### 5.4 모듈별 상세 설계

#### engine.h — 변경사항

```c
#ifndef ENGINE_H_INCLUDED
#define ENGINE_H_INCLUDED

// prototype_01에서 그대로 유지
#define ROOM_W 100
#define ROOM_H 100
#define VIEW_W 10           // prototype_01과 동일 (MSX는 변경했으나 Apple II는 동일)
#define VIEW_H 10
#define ROOM_COUNT 3
#define MAX_DOORS 4
#define MAX_STAIRS 2
#define MAX_BOXES 10
#define MAX_MONSTERS 10
#define MAX_ITEMS_PER_BOX 5
#define MSG_BUF 64
#define INVALID_INDEX 255

// 뷰포트 좌표 상수 (신규 추가)
#define MAP_ORIGIN_X 1
#define MAP_ORIGIN_Y 2

// MoveResult (game.c에서 이동)
typedef enum {
    MOVE_OK = 0,
    MOVE_BLOCKED,
    MOVE_DOOR,
    MOVE_STAIR,
    MOVE_BOX
} MoveResult;

// MonsterState (그대로 유지)
typedef enum {
    MONSTER_PATROL = 0,
    MONSTER_CHASE,
    MONSTER_RETURNING
} MonsterState;

// GameState (running 필드 추가)
typedef struct {
    unsigned char room;
    unsigned char x;
    unsigned char y;
    unsigned char cam_x;
    unsigned char cam_y;
    unsigned char running;   // 신규: 메인 루프 종료 플래그
} GameState;

// RoomDef: grid_packed 대신 RLE 압축 포인터 사용
// const unsigned char *grid_rle;     // RLE 압축된 그리드 포인터
// unsigned int grid_rle_size;        // 압축 데이터 크기
// 나머지 타입 (DoorDef, StairDef, BoxDef, MonsterDef, TileCode)은
// prototype_01 engine.h에서 그대로 복사
#endif
```

#### render.h — 렌더링 인터페이스

MSX prototype_02의 render.h와 **동일한 API** + Apple II 전용 추가:

```c
#ifndef RENDER_H
#define RENDER_H
#include "engine.h"

void render_init(void);
void render_draw_map(const GameState *gs);
void render_draw_border(void);
void render_update_player(const GameState *gs);
void render_draw_monsters(const GameState *gs);
void render_print(unsigned char x, unsigned char y, const char *text);
void render_print_n(unsigned char x, unsigned char y, const char *text,
                    unsigned char maxlen);
void render_print_wrap(unsigned char x, unsigned char y, const char *text,
                       unsigned char w, unsigned char max_rows);
void render_clear_area(unsigned char x, unsigned char y,
                       unsigned char w, unsigned char h);
void render_update_status(const GameState *gs);
void render_set_status(const char *msg);
void render_redraw_all(const GameState *gs);
void render_cleanup(void);
void render_clear_screen(void);  // HGR 전체 클리어 (help.c에서 호출)

// 프롬프트 함수 (MSX는 main.c 내부, Apple II는 render.c에 통합)
unsigned char render_prompt_yes_no(const char *msg);
void render_wait_any_key(const char *msg);

#endif
```

#### render.c — HGR 렌더링 구현 상세

```c
// ========== HGR 행 주소 룩업 테이블 (192 엔트리 × 2바이트 = 384바이트) ==========
// 각 엔트리: HGR Page 1 기준 절대 주소 ($2000-$3FFF)
static const unsigned int hgr_row[192] = {
    // Row 0-7: group 0, eightrow 0
    0x2000, 0x2400, 0x2800, 0x2C00, 0x3000, 0x3400, 0x3800, 0x3C00,
    // Row 8-15: group 0, eightrow 1
    0x2080, 0x2480, 0x2880, 0x2C80, 0x3080, 0x3480, 0x3880, 0x3C80,
    // ... (192 엔트리, 빌드 시 사전 계산)
};

// ========== 핵심 내부 함수 ==========

// HGR 화면 클리어 ($2000-$3FFF 8KB 제로 채우기)
static void hgr_clear(void) {
    memset((void*)0x2000, 0, 0x2000);
}

// 타일 1개 그리기 (tx, ty = 타일 좌표, tile_data = 8바이트 패턴)
static void draw_tile(unsigned char tx, unsigned char ty,
                      const unsigned char *tile_data) {
    unsigned char r;
    unsigned int base_row = (unsigned int)ty * 8;  // 타일 행 → 픽셀 행
    for (r = 0; r < 8; r++) {
        unsigned char *addr = (unsigned char *)(hgr_row[base_row + r]);
        addr[tx] = tile_data[r];
    }
}

// 타일 1개 지우기 (검정으로 채움)
static void clear_tile(unsigned char tx, unsigned char ty) {
    unsigned char r;
    unsigned int base_row = (unsigned int)ty * 8;
    for (r = 0; r < 8; r++) {
        unsigned char *addr = (unsigned char *)(hgr_row[base_row + r]);
        addr[tx] = 0x00;
    }
}

// ========== 상태 메시지 관리 ==========

static char status_buf[40];     // 상태 메시지 버퍼
static unsigned char status_dirty;  // dirty 플래그

void render_set_status(const char *msg) {
    unsigned char i;
    for (i = 0; i < 39 && msg[i]; i++)
        status_buf[i] = msg[i];
    status_buf[i] = '\0';
    status_dirty = 1;
}

// ========== 초기화 시퀀스 ==========
// render_init()는 하드웨어 초기화만 수행하며, 화면 내용은 그리지 않는다.
// main.c에서 render_init() → logic_update_camera() → render_redraw_all() 순으로 호출.
void render_init(void) {
    // 1. Apple IIe/IIc 호환성 (souless_apple2 init_hgr 참조)
    // $C000(80STORE), $C00C(80COL)은 쓰기 전용 — 읽기로는 동작하지 않음
    *(volatile char*)0xC05F = 0;  // DHIRES_CLR
    *(volatile char*)0xC00C = 0;  // 80COL_CLR
    *(volatile char*)0xC000 = 0;  // 80STORE_CLR
    // 2. HGR 모드 진입 (cc65 -O에서 volatile 읽기 제거 방지를 위해 쓰기 사용)
    *(volatile char*)0xC050 = 0;  // TXTCLR
    *(volatile char*)0xC052 = 0;  // MIXCLR
    *(volatile char*)0xC054 = 0;  // PAGE1
    *(volatile char*)0xC057 = 0;  // HIRES
    // 3. 화면 클리어
    hgr_clear();
    // 4. 상태 초기화
    status_buf[0] = '\0';
    status_dirty = 1;
}

// ========== 플레이어 렌더링 (소프트웨어 스프라이트) ==========
// MSX: VDP 하드웨어 스프라이트로 타일 위에 오버레이
// Apple II: 이전 위치에 맵 타일 복원 → 새 위치에 플레이어 타일 그리기
//
// 카메라 이동 시: render_draw_map()이 전체 맵을 다시 그리므로
//   이전 플레이어 위치는 암시적으로 복원됨.
// 카메라 미이동 시: 이전 위치의 맵 타일을 직접 복원해야 함.
//
// 단순화: render_draw_map() → render_draw_monsters() → render_update_player()
//   순서로 매번 호출하면 맵 타일이 먼저 그려지고 그 위에 오브젝트가 오버레이됨.
//   이 방식은 매 턴 100+α 타일을 그리지만, 턴 기반이므로 성능 문제 없음.
```

#### logic.h — 게임 로직 인터페이스

```c
#ifndef LOGIC_H
#define LOGIC_H
#include "engine.h"

void logic_init(GameState *g);
void logic_update_camera(GameState *g);

// RLE 디컴프레스 (룸 그리드를 $0900 버퍼로 전개, 캐시 동작)
void logic_decompress_room(unsigned char room);

// TileCode 반환 (RLE 디컴프레스 후 $0900 버퍼에서 추출)
TileCode logic_get_tile_code(unsigned char room,
                             unsigned char x, unsigned char y);

// 이동 시도 (목표 좌표를 tx,ty에 반환, GameState는 갱신하지 않음)
MoveResult logic_try_move(const GameState *g, signed char dx, signed char dy,
                          unsigned char *tx, unsigned char *ty);

// 오브젝트 찾기 (찾으면 포인터 반환, 못 찾으면 NULL)
const DoorDef *logic_find_door(unsigned char room,
                               unsigned char x, unsigned char y);
const StairDef *logic_find_stair(unsigned char room,
                                 unsigned char x, unsigned char y);
const BoxDef *logic_find_box(unsigned char room,
                             unsigned char x, unsigned char y);

// 룸/층 전환 (성공 시 GameState 갱신, 1 반환)
unsigned char logic_do_door(GameState *g, const DoorDef *door);
unsigned char logic_do_stair(GameState *g, const StairDef *stair);

#endif
```

> **MSX prototype_02와의 차이**:
> - MSX: `logic_try_move()`가 성공 시 내부에서 g->x, g->y를 갱신
> - Apple II: `logic_try_move()`는 tx, ty에 목표 좌표를 반환하고,
>   main.c에서 결과에 따라 g->x, g->y를 갱신 (prototype_01 패턴 유지)
> - MSX: `logic_find_door_at()` → 인덱스(int) 반환
> - Apple II: `logic_find_door()` → DoorDef 포인터 반환 (prototype_01 패턴 유지)

#### monster.h — 몬스터 AI 인터페이스

```c
#ifndef MONSTER_H
#define MONSTER_H
#include "engine.h"

// 런타임 몬스터 상태 (game.c MonsterRuntime에서 이동)
typedef struct {
    unsigned char x;
    unsigned char y;
    unsigned char state;          // MONSTER_PATROL / CHASE / RETURNING
    unsigned char detect_enabled; // 감지 활성화 여부
    unsigned char patrol_wp_idx;  // 순찰 웨이포인트 (0-3)
} MonsterRuntime;

typedef struct {
    MonsterRuntime monsters[MAX_MONSTERS];
} RoomRuntime;

// 전 룸 몬스터 상태 (룸 전환 시에도 보존)
extern RoomRuntime g_runtime[ROOM_COUNT];

// 전체 초기화 (게임 시작 시 1회)
void monster_init_all(void);

// 현재 룸 몬스터 전체 업데이트 (2턴마다 1회)
void monster_update_all(unsigned char room,
                        unsigned char px, unsigned char py);

// 충돌 검사 + 충돌 시 RETURNING 상태로 전환
// 반환: 충돌 몬스터 인덱스 (없으면 -1)
int monster_check_collision(const GameState *st);

// 인카운터 메시지 포맷 ("Encountered {name}!")
// prototype_01 set_encounter_status()에서 추출
void monster_format_encounter_msg(char *buf, unsigned char buf_size,
                                  unsigned char room, unsigned char idx);

// 특정 좌표에 몬스터가 있는지 확인 (맵 렌더링용)
int monster_index_at(unsigned char room,
                     unsigned char x, unsigned char y);

#endif
```

> **Apple II vs MSX 몬스터 AI 차이**:
>
> | 항목 | MSX prototype_02 | Apple II prototype_02 |
> |------|-------------------|----------------------|
> | 상태 | IDLE/CHASE/RETURNING | PATROL/CHASE/RETURNING |
> | 순찰 | 없음 (IDLE=제자리) | 4방향 웨이포인트 순찰 |
> | LOS | 없음 (거리만 판정) | Bresenham 시선 검사 |
> | 룸 상태 | 현재 룸만 (`g_monsters_rt[]`) | 전 룸 (`g_runtime[ROOM_COUNT]`) |
> | 감지 조건 | `dist <= range` | `dist <= range && LOS(x0,y0,x1,y1)` |
> | 탈추적 | `dist > range*2` | `dist > range*2 || (dist > range && !LOS)` |

**몬스터 상태 전이 다이어그램**:

```
                  ┌──────────────────────────────┐
                  │                              │
   [게임 시작]    ▼                              │
       ┌─── PATROL ◄──────────────────────┐     │
       │    (4방향 웨이포인트 순찰)          │     │
       │                                   │     │
       │  detect_enabled=1                 │     │
       │  && dist <= range                 │     │
       │  && has_line_of_sight()           │     │
       ▼                                   │     │
     CHASE ─────────────────────────► RETURNING  │
     (플레이어 추격)                    (홈 복귀)   │
       │    dist > range*2                 │     │
       │    OR (dist > range && !LOS)      │     │
       │                                   │     │
       │  [플레이어와 충돌]                 │     │
       └───────────────────────────────────┘     │
                  detect_enabled=0               │
                                                 │
            [홈 위치 도달]                        │
            detect_enabled=1                     │
            patrol_wp_idx=0                      │
            ─────────────────────────────────────┘
```

#### 5.4.1 몬스터 인카운터 시스템

prototype_01(04_Game/prototype/01)의 몬스터 인카운터 시스템을 그대로 이식한다.

**몬스터 종 데이터** (JSON → room_data.c로 생성):

| 룸 | 몬스터 ID | 종 이름 | Range | Home (X,Y) |
|-----|-----------|---------|-------|-------------|
| Room 1: Abandoned Hall | monster_01 | Cave Spider | 5 | (68,44) |
| | monster_02 | Dark Slime | 4 | (44,28) |
| | monster_03 | Shadow Rat | 4 | (47,23) |
| | monster_04 | Goblin Scout | 7 | (21,47) |
| | monster_05 | Skeleton Guard | 6 | (25,51) |
| | monster_06 | Stone Golem | 5 | (46,30) |
| | monster_07 | Bat Swarm | 6 | (42,47) |
| Room 2: Crystal Cavern | monster_01 | Phantom Wisp | 8 | (41,41) |
| | monster_02 | Skeleton Guard | 6 | (30,58) |
| | monster_03 | Shadow Rat | 4 | (62,66) |
| | monster_04 | Goblin Scout | 7 | (38,49) |
| Room 3: Shadow Labyrinth | monster_01 | Cave Spider | 5 | (92,48) |
| | monster_02 | Shadow Rat | 4 | (88,34) |
| | monster_03 | Dark Slime | 4 | (91,34) |

> 총 14체, 8종. Room 1이 7체로 가장 밀집. Range 4~8 (Manhattan 거리).

**인카운터 메커니즘** (prototype_01 game.c 917-943줄):

1. **이중 충돌 검사** — 플레이어 이동 성공(MOVE_OK) 시 2단계 검사:
   - **1차**: 플레이어가 몬스터 위치로 이동한 경우 (`monster_check_collision`)
   - **2차**: 몬스터가 플레이어 위치로 이동한 경우 (2턴마다 `monster_update_all` 후 재검사)
   - 1차에서 충돌이 이미 발생했으면 2차 검사는 생략 (`collided` 플래그)

2. **Move Counter** — 플레이어가 MOVE_OK로 이동할 때마다 `move_counter++`:
   - `move_counter >= 2` 이면 `monster_update_all()` 호출 + 2차 충돌 검사
   - 룸 전환(문/계단) 시 `move_counter = 0`으로 리셋
   - 몬스터는 플레이어 2턴당 1턴 이동 (플레이어 이동 속도의 절반)

3. **충돌 처리** — `monster_check_collision()`:
   - 충돌 몬스터를 `MONSTER_RETURNING` 상태로 전환
   - `detect_enabled = 0` 설정 (홈 복귀 중 재감지 방지)
   - 충돌 몬스터 인덱스 반환 (메시지 포맷용)

4. **인카운터 메시지** — `monster_format_encounter_msg()`:
   - 포맷: `"Encountered {name}!"` (예: `"Encountered Cave Spider!"`)
   - prototype_01의 `set_encounter_status()` (game.c 221-242줄) 에서 추출
   - `render_set_status()`를 통해 상태 영역(13,6)에 표시

```
인카운터 시퀀스 (main.c 게임 루프 내):

  MOVE_OK 처리:
    st.x = tx; st.y = ty;
    render_set_status("Moved.");
    map_dirty = 1;

    ── 1차 충돌 검사 (플레이어 → 몬스터) ──
    hit = monster_check_collision(&st);
    if (hit >= 0) {
        monster_format_encounter_msg(msg, sizeof(msg), st.room, hit);
        render_set_status(msg);
        collided = 1;
    }

    ── move_counter 처리 ──
    move_counter++;
    if (move_counter >= 2) {
        move_counter = 0;
        monster_update_all(st.room, st.x, st.y);

        ── 2차 충돌 검사 (몬스터 → 플레이어) ──
        if (!collided) {
            hit = monster_check_collision(&st);
            if (hit >= 0) {
                monster_format_encounter_msg(msg, sizeof(msg), st.room, hit);
                render_set_status(msg);
            }
        }
    }
```

> **prototype/01 vs prototype_01 메시지 차이**:
> - prototype/01 (PC): `"Encountered {name}! The creature retreats..."` (monster_format_collision_msg)
> - prototype_01 (Apple II): `"Encountered {name}!"` (set_encounter_status)
> - prototype_02 (Apple II): `"Encountered {name}!"` — prototype_01 Apple II 형식 유지
>   (27열 상태 영역에서 긴 메시지는 줄바꿈 필요, 짧은 형식이 적합)

#### input.h — 입력 인터페이스

```c
#ifndef INPUT_H
#define INPUT_H

// 입력 결과 코드 (MSX의 비트마스크와 달리 enum 사용)
#define INPUT_NONE   0
#define INPUT_UP     1
#define INPUT_DOWN   2
#define INPUT_LEFT   3
#define INPUT_RIGHT  4
#define INPUT_HELP   5
#define INPUT_QUIT   6
#define INPUT_YES    7
#define INPUT_NO     8

// 블로킹 키 입력 (cgetc 래퍼)
// 반환: INPUT_* 코드
unsigned char input_read_blocking(void);

// 예/아니오 입력 대기 (1=예, 0=아니오)
unsigned char input_wait_yes_no(void);

// 아무 키 대기
void input_wait_any_key(void);

#endif
```

> **MSX vs Apple II 입력 모델**:
>
> | 항목 | MSX prototype_02 | Apple II prototype_02 |
> |------|-------------------|----------------------|
> | 읽기 방식 | 논블로킹 `input_read()` | 블로킹 `input_read_blocking()` |
> | 키 반복 | 소프트웨어 딜레이 (KEY_REPEAT_DELAY) | 하드웨어 키 반복 (Apple II 내장) |
> | 프레임 동기화 | `ubox_wait()` 필수 | 불필요 (블로킹 대기) |
> | 프롬프트 | `wait_yes_no()` 논블로킹 폴링 | `input_wait_yes_no()` 블로킹 |
> | 키 매핑 | 매트릭스 행/비트 스캔 | ASCII 코드 비교 |

#### input.c — 키보드 입력 구현

```c
#include "input.h"
#include <conio.h>

// Apple II 키보드: cgetc()는 $C000/$C010을 읽으며 HGR 모드에서도 동작
// 턴 기반이므로 블로킹 입력이 자연스러움

unsigned char input_read_blocking(void) {
    char key = cgetc();

    switch (key) {
        case 'w': case 'W': case 'i': case 'I': return INPUT_UP;
        case 's': case 'S': case 'k': case 'K': return INPUT_DOWN;
        case 'a': case 'A': case 'j': case 'J': return INPUT_LEFT;
        case 'd': case 'D': case 'l': case 'L': return INPUT_RIGHT;
        case 'h': case 'H': return INPUT_HELP;
        case 'q': case 'Q': return INPUT_QUIT;
        case '1': return INPUT_YES;
        case '0': return INPUT_NO;
        default: return INPUT_NONE;
    }
}

unsigned char input_wait_yes_no(void) {
    for (;;) {
        char c = cgetc();
        if (c == '1') return 1;
        if (c == '0') return 0;
    }
}

void input_wait_any_key(void) {
    (void)cgetc();
}
```

#### help.h/c — 도움말 시스템

```c
// help.h
#ifndef HELP_H
#define HELP_H
#include "engine.h"

// 도움말 표시 (종료 요청 시 1 반환)
unsigned char help_show(const GameState *gs);

#endif
```

**도움말 진입/복귀 시퀀스**:

```
도움말 진입:
  1. render_clear_screen() — HGR 화면 전체 클리어 (맵/상태/키힌트 모두 제거)
  2. 도움말 텍스트를 render_print()으로 HGR에 직접 출력
  3. 스크롤 안내 표시: "W/S scroll  SPACE return  Q quit"
  4. 키 입력 루프:
     - W/S: 스크롤 → 재렌더링
     - SPACE: 복귀 (return 0)
     - Q: 종료 확인 → prompt_quit() → (return 1 or 재렌더링)

도움말 복귀:
  1. render_redraw_all(&gs) — 맵/상태/키힌트/플레이어 전체 복원
```

> **MSX와의 차이**: MSX는 도움말 진입 시 스프라이트를 숨겨야 하지만 (Y=0xD0),
> Apple II는 하드웨어 스프라이트가 없으므로 `render_clear_screen()`만으로 충분하다.
> `hgr_clear()`는 render.c 내 static 함수이므로, 외부 모듈은 `render_clear_screen()` 래퍼를 사용한다.

**도움말 텍스트 40열 대응**: Apple II는 40열이므로 prototype_01의 도움말 텍스트(38자 래핑)를
거의 그대로 사용 가능. MSX의 32열 제한과 달리 잘림 문제가 없다.

```c
// 도움말 원문 중 최장 행 예시:
// "  When you approach a door, stair, or box," (43자)
// → 40열에서 줄바꿈 처리 필요 (기존 help_line_at 함수의 WRAP_WIDTH=38 사용)
```

#### main.c — 게임 루프 상세 설계

```c
// ========== 초기화 시퀀스 ==========
void main(void) {
    GameState st;
    unsigned char move_counter = 0;

    // 1. 몬스터 전 룸 초기화
    monster_init_all();

    // 2. 게임 상태 초기화
    st.room = 0;
    st.x = g_rooms[0].player_start_x;
    st.y = g_rooms[0].player_start_y;
    st.cam_x = 0;
    st.cam_y = 0;
    st.running = 1;

    // 3. 렌더링 초기화 (HGR 모드 진입)
    render_init();

    // 4. 초기 상태 설정
    render_set_status("Ready.");
    logic_update_camera(&st);

    // 5. 전체 화면 렌더링
    render_redraw_all(&st);

    // ========== 메인 루프 ==========
    // prototype_01 패턴 유지: 블로킹 입력 → 처리 → 부분 렌더링
    while (st.running) {
        unsigned char map_dirty = 0;
        unsigned char hud_dirty = 0;
        unsigned char full_dirty = 0;
        unsigned char tx = 0, ty = 0;
        MoveResult mr;
        unsigned char inp;

        // 1. 블로킹 입력 대기
        inp = input_read_blocking();

        // 2. 입력 처리
        if (inp == INPUT_QUIT) {
            if (render_prompt_yes_no("Quit? 1=yes 0=no"))
                st.running = 0;
            else {
                render_set_status("Continue.");
                hud_dirty = 1;
            }
        }
        else if (inp == INPUT_HELP) {
            if (help_show(&st))
                st.running = 0;
            else {
                render_set_status("Ready.");
                full_dirty = 1;  // 도움말 복귀 → 전체 리드로우
            }
        }
        else if (inp >= INPUT_UP && inp <= INPUT_RIGHT) {
            signed char dx = 0, dy = 0;
            if (inp == INPUT_UP)    dy = -1;
            if (inp == INPUT_DOWN)  dy = 1;
            if (inp == INPUT_LEFT)  dx = -1;
            if (inp == INPUT_RIGHT) dx = 1;

            mr = logic_try_move(&st, dx, dy, &tx, &ty);

            if (mr == MOVE_OK) {
                unsigned char collided = 0;
                int hit;
                char msg[MSG_BUF];

                st.x = tx;
                st.y = ty;
                render_set_status("Moved.");
                map_dirty = 1;

                // 1차 충돌 검사: 플레이어가 몬스터 위치로 이동
                hit = monster_check_collision(&st);
                if (hit >= 0) {
                    monster_format_encounter_msg(msg, sizeof(msg),
                                                 st.room, (unsigned char)hit);
                    render_set_status(msg);
                    collided = 1;
                }

                // move_counter: 2턴마다 몬스터 업데이트
                move_counter++;
                if (move_counter >= 2) {
                    move_counter = 0;
                    monster_update_all(st.room, st.x, st.y);

                    // 2차 충돌 검사: 몬스터가 플레이어 위치로 이동
                    if (!collided) {
                        hit = monster_check_collision(&st);
                        if (hit >= 0) {
                            monster_format_encounter_msg(msg, sizeof(msg),
                                                         st.room, (unsigned char)hit);
                            render_set_status(msg);
                        }
                    }
                }
            } else if (mr == MOVE_BLOCKED) {
                render_set_status("Blocked.");
                hud_dirty = 1;
            } else if (mr == MOVE_DOOR) {
                const DoorDef *door = logic_find_door(st.room, tx, ty);
                if (door && render_prompt_yes_no("Enter room? 1=yes 0=no")) {
                    if (logic_do_door(&st, door)) {
                        render_set_status("Room changed.");
                        move_counter = 0;
                    } else {
                        render_set_status("No connected room.");
                    }
                    full_dirty = 1;
                } else {
                    render_set_status("Canceled.");
                    hud_dirty = 1;
                }
            } else if (mr == MOVE_STAIR) {
                const StairDef *stair = logic_find_stair(st.room, tx, ty);
                if (stair && render_prompt_yes_no("Use stairs? 1=yes 0=no")) {
                    if (logic_do_stair(&st, stair)) {
                        render_set_status("Floor changed.");
                        move_counter = 0;
                    } else {
                        render_set_status("No connected stair.");
                    }
                    full_dirty = 1;
                } else {
                    render_set_status("Canceled.");
                    hud_dirty = 1;
                }
            } else if (mr == MOVE_BOX) {
                const BoxDef *box = logic_find_box(st.room, tx, ty);
                if (box && render_prompt_yes_no("Open box? 1=yes 0=no")) {
                    // Step 1: 효과 메시지
                    render_wait_any_key(box->effect);
                    // Step 2: 아이템 목록
                    // (logic_format_box_items로 문자열 생성 후 표시)
                    render_set_status("Box closed.");
                    full_dirty = 1;
                } else {
                    render_set_status("Canceled.");
                    hud_dirty = 1;
                }
            }
            hud_dirty = 1;
        }

        if (!st.running) break;

        // 3. 카메라 갱신
        logic_update_camera(&st);

        // 4. 렌더링
        if (full_dirty) {
            render_redraw_all(&st);
            continue;
        }
        if (map_dirty) {
            // 맵 전체 리드로우 (턴 기반이므로 항상 전체 갱신)
            render_draw_map(&st);
            render_draw_monsters(&st);
            render_update_player(&st);
        }
        if (hud_dirty || map_dirty) {
            render_update_status(&st);
        }
    }

    // ========== 종료 ==========
    render_cleanup();
}
```

> **Dirty 플래그 렌더링 전략**:
>
> | 플래그 | 설정 조건 | 렌더링 동작 |
> |--------|----------|------------|
> | `full_dirty` | 도움말 복귀, 프롬프트 후 | `render_redraw_all()` (전체 클리어+재구성) |
> | `map_dirty` | 이동 성공, 룸 전환 | `render_draw_map()` + `render_draw_monsters()` + `render_update_player()` |
> | `hud_dirty` | 상태 메시지 변경 | `render_update_status()` |
>
> prototype_01은 `render_map(&st)` 내에서 플레이어(`!`)와 몬스터(`$`)를 맵과 함께 출력했다.
> prototype_02는 맵 타일 → 몬스터 타일 → 플레이어 타일 순서로 분리 렌더링한다.

---

## 6. 입력 처리

### 6.1 입력 모델 (cgetc 기반, 블로킹)

prototype_01과 동일한 블로킹 입력 모델을 유지한다.
MSX prototype_02의 논블로킹 모델(`ubox_read_keys` + `ubox_wait`)과 달리,
Apple II는 턴 기반이므로 `cgetc()` 블로킹 대기가 자연스럽다.

**키 매핑 테이블**:

| 키 | ASCII | 기능 | INPUT_* 코드 |
|----|-------|------|-------------|
| W, w, I, i | 0x57/0x77/0x49/0x69 | 위 이동 | INPUT_UP (1) |
| S, s, K, k | 0x53/0x73/0x4B/0x6B | 아래 이동 | INPUT_DOWN (2) |
| A, a, J, j | 0x41/0x61/0x4A/0x6A | 왼쪽 이동 | INPUT_LEFT (3) |
| D, d, L, l | 0x44/0x64/0x4C/0x6C | 오른쪽 이동 | INPUT_RIGHT (4) |
| H, h | 0x48/0x68 | 도움말 | INPUT_HELP (5) |
| Q, q | 0x51/0x71 | 종료 | INPUT_QUIT (6) |
| 1 | 0x31 | 예 (확인) | INPUT_YES (7) |
| 0 | 0x30 | 아니오 (취소) | INPUT_NO (8) |

> **IJKL 대체 키**: prototype_01과 동일하게 IJKL을 WASD 대체키로 지원.
> Apple II 키보드 배열에서 WASD가 어색할 수 있으므로 유용.

### 6.2 프롬프트 대기 함수

**render_prompt_yes_no 내부 시퀀스** (상태 영역 사용):
1. `render_clear_area(STATUS_X, STATUS_Y, STATUS_W, STATUS_H)` — 상태 영역 클리어
2. `render_print_wrap(STATUS_X, STATUS_Y, msg, STATUS_W, 3)` — 메시지 줄바꿈 출력
3. `render_print(STATUS_X, STATUS_Y+4, "1=Yes 0=No")` — Y/N 안내
4. `input_wait_yes_no()` — '1' 또는 '0' 대기
5. 결과 반환 (1=예, 0=아니오)

**render_wait_any_key 내부 시퀀스** (메시지 영역 사용):
1. `render_clear_area(MSG_AREA_X, MSG_AREA_Y, MSG_AREA_W, MSG_AREA_H)` — 메시지 영역 클리어
2. `render_print_wrap(MSG_AREA_X, MSG_AREA_Y, msg, MSG_AREA_W, 7)` — 메시지 출력 (40열 활용)
3. `render_print(0, 22, "[Press any key]")` — 안내 (row 22)
4. `input_wait_any_key()` — 아무 키 대기
5. `render_clear_area(MSG_AREA_X, MSG_AREA_Y, MSG_AREA_W, MSG_AREA_H+1)` — 잔상 클리어
   (40×9 = rows 14~22, "[Press any key]" 포함)

> **40열 vs 32열**: Apple II는 40열이므로 MSX의 32열보다 긴 텍스트를 한 줄에 표시 가능.
> 효과 메시지 `"Dust swirls as the lid opens. Nothing unusual happens."` (54자)도
> 40열에서 2줄이면 충분 (MSX 32열에서는 2줄 필요).

---

## 7. 빌드 시스템

### 7.1 compile.sh 전체 구조

prototype_01과 동일한 cc65 빌드 파이프라인. 소스 파일 목록만 확장.

```bash
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
PROGRAM_NAME="${PROGRAM_NAME:-HELLO}"
PROGRAM_FILE="$SCRIPT_DIR/HELLO"
DISK_IMAGE="${DISK_IMAGE:-$BUILD_DIR/prototype_02_appleii_prodos.po}"
RDEDISKTOOL="${RDEDISKTOOL:-}"
PROGRAM_DISK_TEMPLATE="${PROGRAM_DISK_TEMPLATE:-}"
BOOT_DISK="${BOOT_DISK:-}"
cd "$SCRIPT_DIR"

# ... (first_existing_file, first_existing_exec — prototype_01과 동일) ...

clean() {
    rm -f HELLO HELLO.tmp
    rm -rf "$BUILD_DIR"
}

gen_data() {
    python3 ./tools/json_to_room_data.py \
        --source /mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/data \
        --out-dir ./src
}

build() {
    ./tools/check_env.sh
    gen_data
    mkdir -p "$BUILD_DIR"

    echo "[1/3] Building HELLO"
    # prototype_01: cl65 -t apple2 -O -o HELLO src/main.c src/game.c src/room_data.c
    # prototype_02: game.c → 6개 모듈로 분리 + RLE 그리드 압축
    # apple2-hgr-ext.cfg: 커스텀 링커 — HGR 회피 + __HIMEM__=$9600 (BASIC.SYSTEM 호환)
    # RLE 압축으로 바이너리가 $9600 이내에 수용됨 (33KB, 여유 ~3.2KB)
    cl65 -t apple2 -C src/apple2-hgr-ext.cfg -O -o HELLO \
        src/main.c src/render.c src/logic.c src/monster.c \
        src/input.c src/help.c src/room_data.c

    echo "[2/3] Stripping AppleSingle header"
    tail -c +59 HELLO > HELLO.tmp
    mv HELLO.tmp HELLO

    echo "Built: $SCRIPT_DIR/HELLO ($(wc -c < HELLO) bytes)"
}

disk() {
    resolve_paths
    # ... (prototype_01과 동일: 템플릿 복사 → rdedisktool add) ...
    "$RDEDISKTOOL" --bootdisk-mode strict add --type B --addr 0x0803 \
        "$DISK_IMAGE" "$PROGRAM_FILE" "$PROGRAM_NAME"
    echo "Disk image ready: $DISK_IMAGE"
}

case "${1:-all}" in
    build) build ;;
    disk) disk ;;
    run) build; disk; AUTO_BUILD=0 "$SCRIPT_DIR/run_applewin_prodos.sh" ;;
    clean) clean ;;
    all) clean; build; disk ;;
    *) usage; exit 1 ;;
esac
```

**빌드 플래그 설명**:

| 플래그 | 의미 |
|--------|------|
| `-t apple2` | Apple II 타겟 (cc65 내장 라이브러리) |
| `-C src/apple2-hgr-ext.cfg` | 커스텀 HGR 링커 (CODE $4000부터 배치, $2000-$3FFF 회피, HIMEM=$9600) |
| `-O` | 최적화 활성화 (코드 크기 감소) |
| `-o HELLO` | 출력 파일명 (AppleSingle 형식) |
| `tail -c +59` | AppleSingle 헤더 58바이트 제거 → 순수 바이너리 |
| `--type B` | ProDOS 파일 타입: Binary ($06) |
| `--addr 0x0803` | 로드 주소: $0803 (ProDOS TPA 시작) |

### 7.2 run_applewin_prodos.sh

```bash
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# AppleWin (sa2) 경로 탐색
APPLEWIN="${APPLEWIN:-$(first_existing_exec \
    "$PROJECT_ROOT/Emulator/AppleWin/build/sa2" \
    "$(command -v sa2 2>/dev/null || true)" \
)}"

BOOT_DISK="${BOOT_DISK:-$PROJECT_ROOT/diskwork/bootdisk/AppleII/ProDOS_2_4_3.po}"
DISK_IMAGE="${DISK_IMAGE:-$SCRIPT_DIR/build/prototype_02_appleii_prodos.po}"

exec "$APPLEWIN" --d1 "$BOOT_DISK" --d2 "$DISK_IMAGE"
```

### 7.3 빌드→디스크→실행 전체 워크플로

```
┌──────────────────────────────────────────────────────────────┐
│  소스 코드                                                     │
│  src/main.c, render.c, logic.c, monster.c,                    │
│  input.c, help.c, room_data.c                                 │
└────────────────────┬─────────────────────────────────────────┘
                     │  ./compile.sh build
                     │  (cl65 -t apple2 -C apple2-hgr-ext.cfg -O → HELLO)
                     │  (tail -c +59 → AppleSingle 헤더 제거)
                     ▼
┌──────────────────────────────────────────────────────────────┐
│  HELLO (ProDOS Binary, ~33KB, RLE 압축 적용)                    │
└────────────────────┬─────────────────────────────────────────┘
                     │  ./compile.sh disk
                     │  (cp ProDOS template → build/*.po)
                     │  (rdedisktool --bootdisk-mode strict add --type B --addr 0x0803)
                     ▼
┌──────────────────────────────────────────────────────────────┐
│  build/prototype_02_appleii_prodos.po                         │
│  ├── PRODOS        (시스템)                                    │
│  ├── BASIC.SYSTEM  (시스템)                                    │
│  └── HELLO         (게임, BIN $0803)                           │
└────────────────────┬─────────────────────────────────────────┘
                     │  ./compile.sh run
                     │  (sa2 --d1 boot.po --d2 game.po)
                     ▼
┌──────────────────────────────────────────────────────────────┐
│  AppleWin (sa2)                                               │
│  Drive 1: ProDOS boot disk                                    │
│  Drive 2: prototype_02 game disk                              │
│                                                               │
│  부팅 후 BASIC.SYSTEM 프롬프트에서:                               │
│  BRUN HELLO,D2                                                │
│  → HGR 모드 전환 → 게임 실행                                    │
│  → Q키 종료 → TEXT 모드 복귀                                    │
└──────────────────────────────────────────────────────────────┘
```

**일상적 개발 사이클**:

```bash
# 방법 1: 단계별 실행
./compile.sh build          # 컴파일만
./compile.sh disk           # 빌드 + 디스크 생성
./compile.sh run            # 빌드 + 디스크 + 에뮬레이터

# 방법 2: 일괄 실행
./compile.sh all            # clean → build → disk

# 방법 3: 클린 빌드
./compile.sh clean && ./compile.sh all
```

### 7.4 환경 변수

| 변수 | 기본값 | 용도 |
|------|--------|------|
| `PROGRAM_NAME` | `HELLO` | 디스크 내 파일명 |
| `DISK_IMAGE` | `build/prototype_02_appleii_prodos.po` | 출력 디스크 이미지 |
| `RDEDISKTOOL` | (자동 탐색) | rdedisktool 바이너리 경로 |
| `BOOT_DISK` | `diskwork/bootdisk/AppleII/ProDOS_2_4_3.po` | 부트 디스크 |
| `PROGRAM_DISK_TEMPLATE` | Tutorial_apple_prodos_01.po 또는 BOOT_DISK | 디스크 템플릿 |
| `APPLEWIN` | (자동 탐색) | sa2 바이너리 경로 |

---

## 8. 메모리 예산

### 8.1 상세 예산 (RLE 압축 적용 후)

```
구성 요소                    바이트        비율      비고
──────────────────────────────────────────────────────────
■ ROM 데이터 (바이너리 내 코드+데이터, $0803-$9600)
──────────────────────────────────────────────────────────
HGR 행 주소 테이블           384  (192×2)   1.2%
타일 패턴 (16종)             128  (16×8)    0.4%
폰트 패턴 (91엔트리, 73정의)  728  (91×8)    2.2%
render.c 코드              ~1,500          4.5%
logic.c 코드               ~1,000          3.0%    RLE 디컴프레서 포함
monster.c 코드             ~1,500          4.5%
input.c 코드                ~200           0.6%
help.c 코드                 ~500           1.5%
main.c 코드                 ~700           2.1%
도움말 텍스트 문자열          ~800           2.4%
상태/메시지 문자열            ~300           0.9%
룸 데이터 (3룸, RLE 압축)   3,589          10.8%   ← RLE로 대폭 감소
  grid_rle: 3룸 합계 ~1,504B (원본 15,000B → 90% 압축)
  구조체 메타 (3×295B):        885B
    DoorDef[4]×8B + StairDef[2]×6B + BoxDef[10]×19B
    + MonsterDef[10]×5B + 포인터/카운트 11B = 295B/룸
  문자열 리터럴 (이름,효과,아이템): ~1,200B
런타임 상태                  ~500           1.5%
  RoomRuntime[3]: 3×10×5 = 150B
  GameState, 버퍼: ~350B
cc65 런타임 라이브러리       ~1,000         3.0%
스택                         ~256           0.8%
──────────────────────────────────────────────────────────
합계 (코드+데이터)        ~13,085 바이트   (CODE+RODATA+DATA+BSS)
──────────────────────────────────────────────────────────

■ 실행 시 예약 영역 (코드와 분리)
──────────────────────────────────────────────────────────
디컴프레스 버퍼              5,000          $0900-$1C87 (LOWCODE 영역 내)
HGR Page 1 (프레임버퍼)     8,192          $2000-$3FFF (코드와 분리)
──────────────────────────────────────────────────────────

■ 실측 세그먼트 맵 (빌드 결과)
──────────────────────────────────────────────────────────
STARTUP+LOWCODE:  $0803-$0833  (49 바이트)
디컴프레스 버퍼:   $0900-$1C87  (5,000 바이트, 런타임 전용)
HGR 프레임버퍼:    $2000-$3FFF  (8,192 바이트, 예약)
CODE:              $4000-$72CC  (~13,005 바이트)
RODATA:            $72CD-$8828  (~5,468 바이트)
DATA+BSS:          $8829-$8941  (~281 바이트)
스택:              $8E00-$95FF  (~2,048 바이트)
──────────────────────────────────────────────────────────
바이너리 끝:       $8942
바이너리 크기:     33,088 바이트 (AppleSingle 헤더 제거 후)
──────────────────────────────────────────────────────────

BRUN 한계:        $9600 (BASIC.SYSTEM 시작)
여유:             $9600 - $8942 = 3,262 바이트 (9.4%)
```

> **RLE 압축 효과**: prototype_01 방식(grid_packed 인라인)이었다면 바이너리가 ~46KB로
> BASIC.SYSTEM의 BRUN 한계($9600)를 초과하여 "NO BUFFERS AVAILABLE" 오류가 발생했다.
> RLE 압축으로 그리드 데이터를 15,000B → 1,504B (90%)로 줄여 33KB에 수용했다.

### 8.2 HGR 프레임버퍼와 코드 영역 공존

**문제**: Apple II HGR Page 1은 $2000-$3FFF (8KB)를 사용한다.
ProDOS TPA는 $0803-$BEFF이므로 HGR 영역이 TPA 내에 포함된다.
cc65 링커는 기본적으로 $0803부터 코드를 배치하므로, 코드가 $2000을 넘으면
HGR 프레임버퍼와 충돌한다.

**해결**: cc65의 기본 Apple II 링커(`apple2.cfg`)는 `$0803`부터 연속 배치하므로
HGR 영역을 침범한다. **반드시 `apple2-hgr-ext.cfg` 커스텀 링커 설정을 사용**해야 한다.

`apple2-hgr-ext.cfg`는 다음과 같이 메모리를 분할한다:
- LOWCODE: $0803-$1FFF (6,141 바이트, STARTUP + 일부 코드)
- HGR: $2000-$3FFF (8,192 바이트, 프레임버퍼 예약)
- CODE/RODATA/DATA: $4000-$95FF (~22,016 바이트, 메인 코드+데이터)
- __HIMEM__=$9600 (BASIC.SYSTEM 호환, BRUN으로 로드 가능)

**디컴프레스 버퍼 배치** ($0900-$1C87):
LOWCODE 영역 내 STARTUP($0803-$0833) 뒤의 빈 RAM을 활용.
RLE 디컴프레서가 이 영역에 100×50=5,000바이트의 니블 그리드를 전개한다.
코드/데이터와 충돌하지 않음 (CODE는 $4000부터 시작).

### 8.3 BRUN 메모리 한계와 RLE 압축

**문제**: BASIC.SYSTEM은 $9600-$BEFF에 상주한다.
`BRUN HELLO,D2`로 실행할 때, 바이너리가 $9600 이상으로 확장되면
BASIC.SYSTEM의 메모리/버퍼를 덮어써서 "NO BUFFERS AVAILABLE" 오류가 발생한다.

**해결**: RLE 압축을 적용하여 룸 그리드 데이터를 90% 축소.

| 항목 | 압축 전 (prototype_01 방식) | 압축 후 (prototype_02) |
|------|---------------------------|----------------------|
| 그리드 ROM 크기 | 15,000B (3×100×50) | ~1,504B (RLE) |
| 바이너리 크기 | ~46,447B | ~33,088B |
| 바이너리 끝 주소 | ~$BD91 (BEFF 초과) | ~$8942 |
| BRUN 한계 내? | **아니오** (NO BUFFERS AVAILABLE) | **예** (여유 3.2KB) |
| 런타임 비용 | 없음 (직접 접근) | 5,000B 버퍼 + 디컴프레스 시간 |

> RLE 압축의 런타임 비용은 미미하다. 룸 전환 시 1회 디컴프레스(~5,000바이트)에
> ~5ms (1MHz 6502 기준). 같은 룸 내 이동에서는 캐시 히트로 디컴프레스 생략.

### 8.4 추가 완화 전략 (예비)

| 리스크 | 영향 | 완화 방법 |
|--------|------|----------|
| 기능 추가로 여유 소진 | 3.2KB 여유 부족 | 도움말 텍스트 축소 (~400B 절감) |
| 코드 크기 증가 | 링크 오류 | 몬스터 AI 단순화 (LOS 제거로 ~500B 절감) |
| 폰트 크기 728B | 미미한 영향 | 소문자 제거 시 범위 32~93 → 62엔트리 (232B 절감) |

> 현재 3.2KB 여유가 있으므로 추가 완화 전략은 예비용이다.

---

## 9. 구현 단계

### Step 1: 프로젝트 스켈레톤

**목표**: 빈 HGR 화면이 표시되는 실행 파일

1. `Examples/prototype_02_AppleII_prodos/` 디렉토리 생성
2. prototype_01에서 `tools/`, `src/room_data.h`, `src/engine.h` 복사
3. `engine.h` 수정: MoveResult, MAP_ORIGIN_*, running 필드 추가
4. `compile.sh`, `run_applewin_prodos.sh` 작성
5. 모든 헤더 파일 스텁 생성 (render.h, logic.h, monster.h, input.h, help.h)
6. 빈 `main()` + render_init() 스텁으로 빌드 확인
7. AppleWin에서 HGR 모드 진입 + 검은 화면 확인

**산출물**: HGR 검은 화면을 표시하는 HELLO 바이너리

### Step 2: HGR 기반 렌더링

**목표**: HGR 프레임버퍼에 타일 그리기

1. `render.c`에 HGR 초기화 루틴 구현 (소프트 스위치, hgr_clear)
2. 192엔트리 행 주소 테이블 작성 (§2.1 수식으로 사전 계산)
3. `draw_tile()` 내부 함수 구현 (8바이트 → 8개 HGR 행)
4. `clear_tile()` 구현
5. 테스트: 몇 개 타일을 화면에 그려서 확인

**산출물**: HGR 화면에 개별 타일 렌더링 가능

### Step 3: 폰트 및 텍스트 렌더링

**목표**: HGR에서 문자열 출력

1. `font.h` — 91엔트리 생성 (73정의 글리프, 5×7 in 7×8, bit 7=0)
   - MSX font.h 기반, 비트 순서 반전 (MSB→LSB 좌측)
2. `render_print()` 구현 — 문자 → 폰트 타일 인덱스 → draw_tile
3. `render_print_n()` 구현 — 길이 제한 버전
4. `render_print_wrap()` 구현 — 워드 랩핑
5. `render_clear_area()` 구현 — 영역 클리어
6. 테스트: HGR 화면에 "Hello World" 텍스트 렌더링 확인

**산출물**: HGR 화면에 비트맵 텍스트 출력 가능

### Step 4: 타일 디자인 및 맵 렌더링

**목표**: 10×10 맵 뷰포트 표시

1. `tiles.h` — 16종 타일 패턴 설계 (§4.3, §4.4, §4.5)
2. `logic.c` — RLE 디컴프레서 + `logic_decompress_room()` + `logic_get_tile_code()`, `logic_update_camera()` 추출
   - 디컴프레스 버퍼: `#define GRID_BUFFER ((unsigned char *)0x0900)`
   - 자동 디컴프레스: `g_loaded_room` 캐시로 불필요한 재디컴프레스 방지
3. render.c에 `tile_code_to_pattern()` — TileCode → 타일 비트패턴 포인터 변환
4. `render_draw_border()` 구현
5. `render_draw_map()` 구현 — RLE 디컴프레스 → 니블 추출 → HGR 타일
6. `render_update_status()` 구현 — 좌표, 방이름, 상태 메시지
7. 최소 `main.c` — 초기화 + 정적 맵 뷰 렌더링
8. 룸 데이터 생성 (`python3 tools/json_to_room_data.py` — RLE 압축 포함)
9. 테스트: 10×10 맵 뷰포트 + 테두리 + 상태 영역 표시 확인

**산출물**: 정적 맵 뷰가 HGR에 표시됨

### Step 5: 플레이어 이동 및 입력

**목표**: WASD로 던전 탐색 가능

1. `input.c` — 블로킹 키보드 읽기 (cgetc + 키 매핑)
2. `logic.c` — `logic_try_move()` 추출 + 카메라 업데이트 연결
3. `render_update_player()` 구현 — 플레이어 타일 그리기
4. `render_set_status()`, `render_update_status()` — 상태 메시지 dirty 플래그
5. 게임 루프 연결 (입력 → 이동 → 카메라 → 렌더링)
6. 테스트: WASD 이동, 카메라 추적, 벽 충돌, 상태 메시지

**산출물**: WASD 이동 + 카메라 추적이 동작하는 게임

### Step 6: 몬스터 AI

**목표**: prototype_01과 동일한 몬스터 시스템

1. `monster.c` — prototype_01 game.c에서 몬스터 함수 전체 추출
   - `monster_init_all()`: 전 룸 초기화
   - `monster_update_all()`: 상태 머신 (PATROL/CHASE/RETURNING)
   - `monster_check_collision()`: 충돌 감지 + RETURNING 전환
   - `monster_format_encounter_msg()`: 인카운터 메시지 포맷 (§5.4.1)
   - `monster_index_at()`: 좌표 조회 (렌더링용)
   - 내부: `has_line_of_sight()`, `get_patrol_waypoint()`, `monster_step_toward()`
2. `render_draw_monsters()` — 몬스터 타일 렌더링
3. 게임 루프에 인카운터 시스템 통합 (§5.4.1):
   - 이중 충돌 검사 (플레이어→몬스터, 몬스터→플레이어)
   - `move_counter`: 2턴마다 `monster_update_all()` 호출
   - 룸 전환 시 `move_counter = 0` 리셋
4. 충돌 시 `monster_format_encounter_msg()` → `render_set_status()` 연결
5. 테스트: 순찰, 추격, 귀환, 충돌 메시지, 룸 전환 후 복귀

**산출물**: 완전한 몬스터 시스템 (순찰, LOS 추격, 귀환)

### Step 7: 문/계단/상자 상호작용

**목표**: prototype_01과 동일한 상호작용

1. `logic.c` — 문/계단/상자 찾기 + 전환 함수 추출
2. `render_prompt_yes_no()` — HGR 텍스트 + `input_wait_yes_no()`
3. `render_wait_any_key()` — HGR 텍스트 + `input_wait_any_key()`
4. 문: 프롬프트 → `logic_do_door()` → 룸 전환 → `render_redraw_all()`
5. 계단: 프롬프트 → `logic_do_stair()` → 층 전환 → `render_redraw_all()`
6. 상자: 프롬프트 → 효과 메시지 → 아이템 목록 (2단계)
7. 게임 루프에 모든 상호작용 연결
8. 테스트: 방 전환, 층 전환, 상자 열기/취소

**산출물**: 완전한 상호작용 시스템

### Step 8: 도움말 화면

**목표**: H키로 도움말 표시/복귀

1. `help.c` — 도움말 텍스트 (k_help_raw[], prototype_01에서 복사)
2. 도움말 렌더링: hgr_clear → render_print 반복 → 스크롤 안내
3. 스크롤: W/S로 이전/다음 줄 (help_scroll 함수, prototype_01 로직 유지)
4. 복귀: SPACE → `render_redraw_all()` (전체 리드로우)
5. 종료: Q → prompt_quit → 게임 종료
6. 테스트: H키 → 도움말 → 스크롤 → 복귀 시 화면 완전 복원

**산출물**: 완전한 도움말 시스템

### Step 9: 마무리

**목표**: 최종 정리 및 스모크 테스트

1. 종료 확인 프롬프트 (`render_prompt_yes_no("Quit?")`)
2. `render_cleanup()` — TEXT 모드 복귀
   ```c
   void render_cleanup(void) {
       // TEXT 모드 복귀 (소프트 스위치, 쓰기 사용)
       *(volatile char*)0xC051 = 0;  // TXTSET — 텍스트 모드
   }
   ```
3. 타일 패턴 시각적 미세 조정 (AppleWin 확인)
4. `compile.sh` 최종 정리 (모든 타겟: clean/build/disk/run/all)
5. `run_applewin_prodos.sh` 작성
6. 전체 스모크 테스트:
   - 부팅 → HGR 모드 전환 확인
   - 3개 방 탐색 (문/계단으로 전환)
   - 모든 오브젝트 상호작용 (문, 계단, 상자)
   - 몬스터 순찰/추격/충돌/귀환
   - 도움말 진입/스크롤/복귀
   - Q키 종료 → TEXT 모드 복귀

**산출물**: 완성된 prototype_02

---

## 10. 테스트 전략

### 10.1 빌드 테스트

```bash
./compile.sh build    # cc65 컴파일 성공 확인 (HELLO 생성)
./compile.sh disk     # 디스크 이미지 생성 확인
./compile.sh run      # AppleWin 부팅 확인
```

### 10.2 기능 테스트

| # | 항목 | 검증 내용 |
|---|------|-----------|
| 1 | 화면 초기화 | HGR 모드 전환, 검은 화면 → 맵+테두리 표시 |
| 2 | 텍스트 렌더링 | 비트맵 폰트로 문자열 출력 (좌표, 방이름, 상태) |
| 3 | 맵 렌더링 | 10×10 뷰포트에 올바른 타일 표시 |
| 4 | 플레이어 이동 | WASD/IJKL 입력, 충돌 판정, 카메라 추적 |
| 5 | 몬스터 순찰 | 4방향 웨이포인트 이동 확인 |
| 6 | 몬스터 추격 | LOS 범위 진입 시 추격 시작 |
| 7 | 몬스터 충돌 | 충돌 메시지 + RETURNING 상태 전환 |
| 8 | 문 상호작용 | 프롬프트 표시, 1=예 → 룸 전환, 0=아니오 → 취소 |
| 9 | 계단 상호작용 | 프롬프트 표시, Z 레벨 변경 |
| 10 | 상자 상호작용 | 효과 메시지 → 아이템 목록 2단계 표시 |
| 11 | 도움말 | H키 진입, W/S 스크롤, SPACE 복귀, 화면 완전 복원 |
| 12 | 종료 | Q키 → 확인 프롬프트 → TEXT 모드 복귀 |
| 13 | 상태 메시지 | dirty 플래그로 변경 시만 클리어+재출력, 잔상 없음 |
| 14 | 장문 메시지 | 27열(상태)/40열(메시지) 워드 랩핑 정상 동작 |

### 10.3 회귀 테스트

prototype_01의 모든 기능이 prototype_02에서 재현되는지 확인:

| # | 기능 | prototype_01 구현 | prototype_02 대응 |
|---|------|------------------|------------------|
| 1 | WASD 이동 | cgetc() + switch | input_read_blocking() (동일 패턴) |
| 2 | 벽 충돌 | logic_get_tile_code() == WALL | logic_get_tile_code() == WALL |
| 3 | 문 상호작용 | get_door_at() + prompt | logic_find_door() + render_prompt_yes_no |
| 4 | 문 룸 전환 | do_door_transition() | logic_do_door() |
| 5 | 계단 상호작용 | get_stair_at() + prompt | logic_find_stair() + render_prompt_yes_no |
| 6 | 상자 열기 | box_interaction() (2단계) | render_wait_any_key() (효과) + (아이템) |
| 7 | 몬스터 순찰 | PATROL + waypoint | 동일 (추출만) |
| 8 | 몬스터 추격 | CHASE + LOS | 동일 (추출만) |
| 9 | 몬스터 충돌 | RETURNING + 메시지 | 동일 (추출만) |
| 10 | 카메라 추적 | update_camera() | logic_update_camera() |
| 11 | 도움말 스크롤 | help_scroll() W/S | 동일 (추출만) |
| 12 | 종료 | prompt_quit() → 1=종료 | render_prompt_yes_no() → running=0 |
| 13 | 룸 상태 보존 | g_runtime[ROOM_COUNT] | 동일 (monster.h로 이동) |
| 14 | 플레이어 표시 | '!' 문자 | TILE_PLAYER 타일 |
| 15 | 몬스터 표시 | '$' 문자 | TILE_MONSTER 타일 |

---

## 11. 리스크 및 대응

| 리스크 | 영향 | 대응 |
|--------|------|------|
| HGR 프레임버퍼-코드 충돌 | 렌더링 깨짐 / 크래시 | `apple2-hgr-ext.cfg` 커스텀 링커 사용 필수 (§7.1, §8.2 참조) |
| 타일 색상 아티팩트 | 의도치 않은 보라/초록 표시 | bit 7=0 확인, 인접 비트 쌍으로 흰색 보장 |
| 폰트 가독성 | 5×7 글리프가 작게 보임 | 40열 타일이므로 prototype_01 텍스트와 동일 크기 |
| 메모리 부족 | 링크 오류 | RLE 압축 적용으로 해결 (§8.3). 추가 완화: §8.4 |
| cgetc() HGR 동작 | 입력 불가 | $C000 직접 읽기로 대체 (cc65 내부적으로 동일) |
| HGR 행 주소 오류 | 화면 깨짐 | 192 엔트리 테이블 사전 검증 (계산식 대조) |
| AppleSingle 헤더 크기 변경 | 바이너리 깨짐 | `tail -c +59` 값 확인 (cc65 버전별 차이 없음) |

---

## 12. 수정/생성 파일 목록

| 파일 | 상태 | 설명 |
|------|------|------|
| `src/main.c` | **신규** | 진입점 + 게임 루프 |
| `src/render.h` | **신규** | 렌더링 인터페이스 |
| `src/render.c` | **신규** | HGR 렌더링 (행 테이블, 타일 그리기, 텍스트) |
| `src/logic.h` | **신규** | 게임 로직 인터페이스 |
| `src/logic.c` | **신규** | 이동, 카메라, 전환 (game.c에서 추출) + RLE 디컴프레서 |
| `src/monster.h` | **신규** | 몬스터 AI 인터페이스 + MonsterRuntime 타입 |
| `src/monster.c` | **신규** | 몬스터 FSM (game.c에서 추출, 전체 AI 유지) |
| `src/input.h` | **신규** | 입력 인터페이스 (INPUT_* 코드) |
| `src/input.c` | **신규** | 키보드 읽기 (cgetc 래퍼) |
| `src/help.h` | **신규** | 도움말 인터페이스 |
| `src/help.c` | **신규** | HGR 도움말 화면 |
| `src/engine.h` | **수정** | MoveResult, MAP_ORIGIN_*, running 추가 |
| `src/room_data.h` | 복사 | prototype_01에서 그대로 |
| `src/room_data.c` | 생성 | json_to_room_data.py로 생성 |
| `src/tiles.h` | **신규** | HGR 타일 패턴 16종 (128 바이트) |
| `src/font.h` | **신규** | HGR 폰트 91엔트리, 73정의 (728 바이트) |
| `compile.sh` | **신규** | 빌드 스크립트 (cc65) |
| `run_applewin_prodos.sh` | **신규** | 에뮬레이터 실행 스크립트 |
| `tools/json_to_room_data.py` | **수정** | prototype_01 기반 + RLE 압축 (`rle_compress()`) 추가 |
| `tools/check_env.sh` | 복사 | prototype_01에서 그대로 |

---

## 13. 검증 방법

1. `./compile.sh all` — 빌드 성공, HELLO 바이너리 생성
2. AppleWin 실행 → HGR 모드 전환, 화면 표시 확인
3. 10×10 맵 뷰포트 + 테두리 + 상태 영역 레이아웃 확인
4. WASD 이동, 벽 충돌, 카메라 추적 확인
5. 몬스터 순찰/추격/귀환 동작 확인
6. 문/계단/상자 상호작용 (1=예, 0=아니오) 확인
7. H키 도움말 → 스크롤 → 복귀 전체 리드로우 확인
8. Q키 종료 → TEXT 모드 복귀 확인
9. 3개 방 전체 탐색 스모크 테스트

---

## 14. 참고 자료

- **prototype_01 소스**: `./Examples/prototype_01_AppleII_prodos/src/`
- **prototype_02 MSX 소스**: `./Examples/prototype_02_MSX_ROM_MSXDOS/src/`
- **MSX prototype_02 플랜**: `./PLAN_prototype_02_MSX_ROM_MSXDOS.md`
- **souless_apple2 라이브러리**: `./Library/AppleII/souless_apple2/`
  - `common/source/video.s` — HGR 초기화, 클리어
  - `common/source/videoOffset.s` — 행 주소 테이블
  - `common/source/sprite.s` — 7비트 렌더링 패턴
- **ProDOS 부트 디스크**: `./diskwork/bootdisk/AppleII/ProDOS_2_4_3.po`
- **rdedisktool**: `./RetroDeveloperEnvironmentDisktool/`
  - CLI.cpp에 ProDOS 타입명 지원 추가 (`--type SYS`, `--type BIN`, `0xFF` 등)
- **AppleWin (sa2)**: `./Emulator/AppleWin/`
- **prototype_01 빌드 스크립트**: `./Examples/prototype_01_AppleII_prodos/compile.sh`
- **프로젝트 공통 실행 스크립트**: `./run_applewin_prodos.sh`

---

## Appendix A: 검토 보강사항

### A.1 문/상자 2타일 오브젝트 렌더링 전략

prototype_01의 문(`@`)과 상자(`%`)는 `orientation` 필드에 따라 2×1(수평) 또는 1×2(수직)로 배치된다.
룸 그리드 자체에 이미 2타일 분량의 데이터가 배치되어 있으므로 (TileCode=DOOR 2개 연속),
렌더러는 각 셀을 독립적으로 변환하면 된다.

**문의 수평/수직 시각 구분** — 인접 셀 확인 방식:

```c
static const unsigned char *tile_code_to_pattern(TileCode code,
    unsigned char room, unsigned char gx, unsigned char gy) {
    switch (code) {
        case TILE_FLOOR:     return tile_floor;
        case TILE_WALL:      return tile_wall;
        case TILE_DOOR: {
            // 인접 셀에도 DOOR가 있으면 수평, 아니면 수직
            if ((gx > 0 && logic_get_tile_code(room, gx-1, gy) == TILE_DOOR) ||
                (gx < ROOM_W-1 && logic_get_tile_code(room, gx+1, gy) == TILE_DOOR))
                return tile_door_h;
            return tile_door_v;
        }
        case TILE_STAIR_DOWN: return tile_stair_dn;
        case TILE_STAIR_UP:   return tile_stair_up;
        case TILE_BOX:        return tile_box;
        default:              return tile_empty;
    }
}
```

### A.2 소프트웨어 스프라이트 — 지우기→그리기 패턴 상세

**플레이어 렌더링 시퀀스** (매 턴, `render_update_player` 내):

```
Case 1: 카메라 미이동 (뷰포트 내 이동)
  1. 이전 위치에 맵 타일 복원 (또는 몬스터 타일)
  2. 새 위치에 플레이어 타일 그리기

Case 2: 카메라 이동 (뷰포트 스크롤)
  1. render_draw_map()이 전체 맵을 다시 그림 → 이전 플레이어 암시적 제거
  2. render_draw_monsters()가 몬스터 타일 오버레이
  3. 새 위치에 플레이어 타일 그리기

Case 3: 룸 전환 / 도움말 복귀
  1. render_redraw_all()이 전체 화면 재구성 → 위 Case 2와 동일
```

> **단순화 선택**: 턴 기반이므로 매 턴 render_draw_map() + render_draw_monsters() +
> render_update_player()를 항상 호출하는 방식으로 단순화할 수 있다.
> 10×10 맵 + 몬스터 + 플레이어 = ~110 타일 × 8바이트 = ~880바이트 쓰기.
> 1MHz 6502에서 ~15ms, 체감 불가.

### A.3 40열 텍스트 줄바꿈 처리

Apple II prototype_02는 40열이므로 MSX의 32열보다 여유롭다.

**영향받는 요소**:
- 상태 메시지 영역: 27자 폭 (col 13~39) — MSX 19자보다 넓음
- 메시지 영역: 40자 폭 (col 0~39) — MSX 32자보다 넓음
- 도움말: 40자 폭 (prototype_01과 동일)

**장문 메시지 처리**:
- `"Enter Crystal Cavern?"` (22자) → 27열 상태 영역에 1줄로 표시 가능
- `"Dust swirls as the lid opens."` (31자) → 40열 메시지 영역에 1줄 가능
- `"Open box by Goblin Trickster?"` (30자) → 27열 상태 영역에서 2줄

### A.4 HGR 행 주소 테이블 생성

192 엔트리 워드 테이블을 C 코드로 사전 계산:

```c
// 생성 코드 (컴파일 타임 계산은 불가능하므로 정적 초기화)
static const unsigned int hgr_row[192] = {
    // 공식: $2000 + (row/64)*$28 + (row%8)*$400 + ((row/8)%8)*$80
    // Row  0: $2000 + 0*$28 + 0*$400 + 0*$80 = $2000
    // Row  1: $2000 + 0*$28 + 1*$400 + 0*$80 = $2400
    // ...
    // 전체 192개 값을 hex 리터럴로 나열
    0x2000, 0x2400, 0x2800, 0x2C00, 0x3000, 0x3400, 0x3800, 0x3C00,  // rows 0-7
    0x2080, 0x2480, 0x2880, 0x2C80, 0x3080, 0x3480, 0x3880, 0x3C80,  // rows 8-15
    0x2100, 0x2500, 0x2900, 0x2D00, 0x3100, 0x3500, 0x3900, 0x3D00,  // rows 16-23
    0x2180, 0x2580, 0x2980, 0x2D80, 0x3180, 0x3580, 0x3980, 0x3D80,  // rows 24-31
    0x2200, 0x2600, 0x2A00, 0x2E00, 0x3200, 0x3600, 0x3A00, 0x3E00,  // rows 32-39
    0x2280, 0x2680, 0x2A80, 0x2E80, 0x3280, 0x3680, 0x3A80, 0x3E80,  // rows 40-47
    0x2300, 0x2700, 0x2B00, 0x2F00, 0x3300, 0x3700, 0x3B00, 0x3F00,  // rows 48-55
    0x2380, 0x2780, 0x2B80, 0x2F80, 0x3380, 0x3780, 0x3B80, 0x3F80,  // rows 56-63
    0x2028, 0x2428, 0x2828, 0x2C28, 0x3028, 0x3428, 0x3828, 0x3C28,  // rows 64-71
    0x20A8, 0x24A8, 0x28A8, 0x2CA8, 0x30A8, 0x34A8, 0x38A8, 0x3CA8,  // rows 72-79
    0x2128, 0x2528, 0x2928, 0x2D28, 0x3128, 0x3528, 0x3928, 0x3D28,  // rows 80-87
    0x21A8, 0x25A8, 0x29A8, 0x2DA8, 0x31A8, 0x35A8, 0x39A8, 0x3DA8,  // rows 88-95
    0x2228, 0x2628, 0x2A28, 0x2E28, 0x3228, 0x3628, 0x3A28, 0x3E28,  // rows 96-103
    0x22A8, 0x26A8, 0x2AA8, 0x2EA8, 0x32A8, 0x36A8, 0x3AA8, 0x3EA8,  // rows 104-111
    0x2328, 0x2728, 0x2B28, 0x2F28, 0x3328, 0x3728, 0x3B28, 0x3F28,  // rows 112-119
    0x23A8, 0x27A8, 0x2BA8, 0x2FA8, 0x33A8, 0x37A8, 0x3BA8, 0x3FA8,  // rows 120-127
    0x2050, 0x2450, 0x2850, 0x2C50, 0x3050, 0x3450, 0x3850, 0x3C50,  // rows 128-135
    0x20D0, 0x24D0, 0x28D0, 0x2CD0, 0x30D0, 0x34D0, 0x38D0, 0x3CD0,  // rows 136-143
    0x2150, 0x2550, 0x2950, 0x2D50, 0x3150, 0x3550, 0x3950, 0x3D50,  // rows 144-151
    0x21D0, 0x25D0, 0x29D0, 0x2DD0, 0x31D0, 0x35D0, 0x39D0, 0x3DD0,  // rows 152-159
    0x2250, 0x2650, 0x2A50, 0x2E50, 0x3250, 0x3650, 0x3A50, 0x3E50,  // rows 160-167
    0x22D0, 0x26D0, 0x2AD0, 0x2ED0, 0x32D0, 0x36D0, 0x3AD0, 0x3ED0,  // rows 168-175
    0x2350, 0x2750, 0x2B50, 0x2F50, 0x3350, 0x3750, 0x3B50, 0x3F50,  // rows 176-183
    0x23D0, 0x27D0, 0x2BD0, 0x2FD0, 0x33D0, 0x37D0, 0x3BD0, 0x3FD0,  // rows 184-191
};
```

> **검증**: Row 191 = $3FD0. 마지막 바이트 = $3FD0 + 39 = $3FF7.
> HGR Page 1 끝 = $3FFF. 모든 행이 $2000-$3FFF 범위 내.

### A.5 render_cleanup — TEXT 모드 복귀

```c
void render_cleanup(void) {
    // TEXT 모드 복귀 (소프트 스위치, 쓰기 사용)
    *(volatile char*)0xC051 = 0;  // TXTSET — 텍스트 모드

    // Apple II는 MSX와 달리 ISR 훅 복원이 불필요.
    // cgetc() 사용으로 키보드 상태도 별도 복원 불필요.
    // TEXT 페이지($0400-$07FF)에 잔상이 있을 수 있으나,
    // ProDOS가 프롬프트를 다시 그리므로 문제 없음.
}
```

> **MSX와의 차이**: MSX는 `render_cleanup()`에서 HTIMI 훅 복원, SCNCNT/REPCNT 복원,
> 스프라이트 숨기기 등 복잡한 정리가 필요하다. Apple II는 소프트 스위치 1개로 충분.

### A.6 TileCode 인덱스 ↔ 타일 패턴 매핑

prototype_01의 `TileCode` enum(0-5)과 prototype_02의 타일 패턴 인덱스를 매핑:

```c
// TileCode (engine.h, prototype_01 유지)    → 타일 패턴 인덱스 (tiles.h)
// TILE_FLOOR     (0)                        → 0 (tile_floor)
// TILE_WALL      (1)                        → 1 (tile_wall)
// TILE_DOOR      (2)                        → 2 또는 3 (H/V 분기)
// TILE_STAIR_DOWN(3)                        → 4 (tile_stair_dn)
// TILE_STAIR_UP  (4)                        → 5 (tile_stair_up)
// TILE_BOX       (5)                        → 6 (tile_box)

// 추가 (TileCode에 없음):
// TILE_EMPTY                                → 7 (tile_empty)
// TILE_BORDER_*                             → 8-13
// TILE_PLAYER                               → 14
// TILE_MONSTER                              → 15
```

> `render_draw_map()` 내부에서 `logic_get_tile_code()` → `tile_code_to_pattern()` 변환을 수행.
> 문(TILE_DOOR)은 §A.1의 인접 셀 확인으로 H/V 분기.

---

## Appendix B: 맵/타일 데이터 외부 파일 분리 가능성 분석

> 분석일: 2026-03-01
> 목적: 맵 데이터와 타일 데이터를 ProDOS 디스크 상의 별도 파일로 분리하여
> 더 많은 룸 또는 시각적 다양성을 확보할 수 있는지 검토한다.

### B.1 현재 메모리 레이아웃

```
주소 범위           크기         용도
──────────────────────────────────────────────────
$0803-$0833        48 bytes     STARTUP (cc65 런타임 초기화)
$0834-$08FF        204 bytes    미사용 (STARTUP~GRID_BUFFER 간극)
$0900-$1C87        5,000 bytes  GRID_BUFFER (디컴프레스된 100×50 패킹 그리드)
$1C88-$1FFF        888 bytes    미사용 (GRID_BUFFER~HGR 간극)
$2000-$3FFF        8,192 bytes  HGR Page 1 프레임버퍼
$4000-$72CC        ~13KB        CODE 세그먼트
$72CD-$8828        ~5.5KB       RODATA 세그먼트
$8829-$898E        ~360 bytes   DATA + BSS 세그먼트
$8E00-$95FF        2,048 bytes  C 스택 (하향 성장)
$9600-$BEFF        보호 영역    BASIC.SYSTEM
$BF00-$BFFF        보호 영역    ProDOS 글로벌 페이지
──────────────────────────────────────────────────
바이너리 크기: 33,088 bytes
BRUN 한계까지 여유: ~3,543 bytes
```

### B.2 RODATA 구성 분석

| 데이터 | 크기 | RODATA 비율 | 외부화 가능 |
|--------|------|-------------|-------------|
| RLE 룸 그리드 (3룸) | ~1,564 bytes | 28.6% | **가능** |
| 타일 패턴 (16종 × 8B) | 128 bytes | 2.3% | **가능** |
| 폰트 패턴 (91자 × 8B) | 728 bytes | 13.3% | 가능 (선택) |
| HGR 행 주소 테이블 (192 × 2B) | 384 bytes | 7.0% | 유지 (성능) |
| 룸 메타데이터 (문/계단/상자/몬스터) | ~1,500 bytes | 27.4% | 가능하나 복잡 |
| 문자열 상수 (상태 메시지 등) | ~1,100 bytes | 20.1% | 유지 |
| **합계** | **~5,468 bytes** | 100% | |

### B.3 외부화 시 메모리 배치안

GRID_BUFFER($0900) 뒤의 미사용 영역($1C88-$1FFF, 888 bytes)을 활용:

```
$0900-$1C87  GRID_BUFFER (5,000 bytes) — 디컴프레스된 맵 그리드 (기존과 동일)
$1C88-$1CFF  TILE_BUFFER (120 bytes)   — 현재 활성 타일셋 (15종 × 8B)
$1D00-$1FFF  FILE_BUFFER (768 bytes)   — 디스크 파일 읽기 임시 버퍼
$2000-$3FFF  HGR 프레임버퍼             — 변경 없음
```

- RLE 데이터가 최대 ~600 bytes이므로 FILE_BUFFER(768 bytes)에 충분히 수용
- 파일에서 FILE_BUFFER로 읽은 뒤 GRID_BUFFER로 디컴프레스
- 타일셋은 TILE_BUFFER에 로드 후 렌더러가 참조

### B.4 ProDOS 디스크 파일 구조안

```
ProDOS 디스크 (140KB):
├── PRODOS            (부트 커널)
├── BASIC.SYSTEM      (BASIC 인터프리터)
├── HELLO             (게임 바이너리, ~28-30KB)
├── TILES0            (타일셋 0: 동굴 테마, 128 bytes)
├── TILES1            (타일셋 1: 성채 테마, 128 bytes)
├── TILES2            (타일셋 2: 숲 테마, 128 bytes)
├── ROOM00            (룸 0: RLE 그리드, ~450 bytes)
├── ROOM01            (룸 1: ~564 bytes)
├── ROOM02            (룸 2: ~550 bytes)
├── ...
└── ROOMnn            (추가 룸, 디스크 용량 한도까지)
```

140KB ProDOS 디스크에서 PRODOS(~32KB) + BASIC.SYSTEM(~10KB) + HELLO(~30KB) = ~72KB 사용.
**잔여 ~68KB** → 평균 500 bytes/룸 기준 **~136개 룸** 저장 가능.

### B.5 파일 로딩 방식: ProDOS MLI 직접 호출

cc65의 `<stdio.h>` (fopen/fread/fclose)는 ~3KB 코드 추가로 여유 공간을 초과할 위험.
대신 **ProDOS MLI(Machine Language Interface) 직접 호출**로 최소화:

```
ProDOS MLI 엔트리: $BF00 (JSR $BF00)
OPEN   ($C8): 파일 열기, 참조번호 반환
READ   ($CA): 데이터 읽기
CLOSE  ($CC): 파일 닫기
```

MLI 호출에는 파일당 **1,024 bytes I/O 버퍼**가 필요.
이 버퍼는 FILE_BUFFER($1D00)와 별도로, $0834-$08FF(204 bytes)는 부족하므로
스택 영역 하단($8E00 근처)을 임시 사용하거나, 추가 배치가 필요.

**대안**: cc65의 저수준 `open()`/`read()`/`close()` (`<fcntl.h>`) 사용 시
stdio보다 경량 (~800-1,200 bytes). 이 방법이 가장 실용적.

### B.6 코드 비용 vs 절약

| 항목 | 바이트 변화 |
|------|-------------|
| 파일 로더 추가 (cc65 fcntl 기반) | +800 ~ +1,200 |
| RLE 룸 그리드 데이터 제거 | -1,564 |
| 타일 패턴 데이터 제거 | -128 |
| 파일명 문자열/경로 처리 코드 | +200 ~ +300 |
| **순 절약 (맵+타일만)** | **약 -200 ~ -700 bytes** |

룸 메타데이터까지 외부화할 경우:

| 항목 | 바이트 변화 |
|------|-------------|
| 바이너리 파서 코드 추가 | +300 ~ +500 |
| 룸 메타데이터 제거 | -1,500 |
| 문자열 로딩 버퍼 | +200 (BSS) |
| **순 절약 (전체 외부화)** | **약 -1,000 ~ -1,700 bytes** |

### B.7 타일셋 교체의 이점

현재: 16종 타일 패턴이 바이너리에 고정 → 모든 룸이 동일한 비주얼.

외부화 시: 룸별로 다른 타일셋 파일을 지정 가능.

```
TILES0: 동굴 (현재 기본 — 벽돌 벽, 돌 바닥)
TILES1: 성채 (석조 벽, 타일 바닥, 철문)
TILES2: 숲   (나무 벽, 풀 바닥, 나무문)
```

룸 전환 시 타일셋 ID가 변경되면 TILES 파일 로드 → TILE_BUFFER 갱신.
동일 타일셋이면 스킵 (캐시).

### B.8 제약 사항

| 제약 | 설명 |
|------|------|
| **맵 크기 확대 불가** | GRID_BUFFER($0900-$1C87)가 5,000 bytes 고정. 100×100이 최대. 150×150은 11,250 bytes → $0900-$1FFF(5,888B) 초과 |
| **디스크 접근 지연** | 룸 전환 시 ~1-2초 디스크 읽기. 턴 기반이므로 허용 가능하나 체감됨 |
| **MLI I/O 버퍼** | 파일 열기 시 1,024 bytes I/O 버퍼 필요. 메모리 배치 주의 |
| **경로 규칙** | ProDOS 파일명은 대문자 15자, 볼륨명 포함 시 `/VOLUME/FILENAME` |
| **에러 처리** | 파일 미발견 시 대응 필요 (기본 룸으로 폴백 등) |

### B.9 더 큰 맵을 위한 대안 (참고)

100×100 한계를 넘기려면 근본적인 아키텍처 변경이 필요:

| 전략 | 맵 크기 | 난이도 | 비고 |
|------|---------|--------|------|
| 현재 (임베디드) | 100×100, 3룸 | — | 현재 상태 |
| 파일 분리 (MLI) | 100×100, 수십 룸 | 중간 | 본 분석 대상 |
| Aux Memory ($C000 뱅킹) | ~150×150 | 높음 | Apple IIe/IIc 전용, 소프트 스위치로 64KB 보조 RAM 접근 |
| 청크 로딩 | 무제한 (스트리밍) | 매우 높음 | 화면에 보이는 영역만 디스크에서 로드, 복잡한 캐시 필요 |

### B.10 결론

**실용적 방향은 "동일 크기(100×100) 맵 + 룸 수 확장 + 타일셋 다양화"**.

- 맵 데이터 + 타일 데이터 외부화는 기술적으로 **실현 가능**
- 바이너리 ~200-700 bytes 절약, 메타데이터 포함 시 ~1,000-1,700 bytes
- 룸 수를 3개 → 수십~100개 이상으로 확장 가능
- 룸별 타일셋 교체로 시각적 다양성 확보
- 맵 크기 자체의 확대는 GRID_BUFFER 크기 제한으로 현재 구조에서 불가
- 구현 시 cc65 `<fcntl.h>` 기반이 코드 비용 대비 가장 실용적
