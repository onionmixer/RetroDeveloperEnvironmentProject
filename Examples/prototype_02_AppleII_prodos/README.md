# prototype_02_AppleII_prodos

Apple II HGR (Hi-Res Graphics, 280x192) 던전 탐색 게임.
`prototype_01_AppleII_prodos`의 텍스트 모드를 타일 기반 그래픽으로 전환하고,
monolithic `game.c`를 모듈 분리한 버전.

## 빌드 및 실행

```bash
# 전체 빌드 (clean + compile + disk image)
./compile.sh all

# 빌드 + 디스크 + AppleWin 실행
./compile.sh run

# 개별 단계
./compile.sh build    # 컴파일만
./compile.sh disk     # 디스크 이미지 생성
./compile.sh clean    # 빌드 산출물 제거
```

부팅 후 BASIC.SYSTEM 프롬프트에서:
```
BRUN HELLO,D2
```

### 빌드 요구사항

- **cc65** (`cl65`, `ca65`, `ld65`) — `apt install cc65`
- **Python 3** — 룸 데이터 생성기 (`tools/json_to_room_data.py`)
- **rdedisktool** — 디스크 이미지 생성 (프로젝트 내 빌드)
- **AppleWin (sa2)** — 에뮬레이터 실행 시

## 프로젝트 구조

```
prototype_02_AppleII_prodos/
├── src/
│   ├── main.c              진입점 + 게임 루프
│   ├── engine.h            공유 타입, 상수, 데이터 구조체
│   ├── render.c / render.h HGR 렌더링 (타일, 폰트, HUD)
│   ├── logic.c / logic.h   게임 로직 (이동, 카메라, RLE 디컴프레서)
│   ├── monster.c / monster.h  몬스터 AI (PATROL/CHASE/RETURNING FSM)
│   ├── input.c / input.h   키보드 입력 (cgetc 래퍼)
│   ├── help.c / help.h     도움말 화면
│   ├── room_data.c / room_data.h  룸 그리드 + 메타데이터 (자동 생성)
│   └── apple2-hgr-ext.cfg  cc65 커스텀 링커 설정
├── tools/
│   ├── json_to_room_data.py  JSON → C 룸 데이터 변환기 (RLE 압축)
│   └── check_env.sh          빌드 환경 검증
├── compile.sh               빌드 스크립트
├── run_applewin_prodos.sh   에뮬레이터 실행 스크립트
└── build/                   빌드 산출물 (생성됨)
```

## 메모리 맵

```
$0000-$007F  Zero Page (cc65 런타임)
$0080-$009F  Zero Page 변수
$0803-$0833  STARTUP (cc65 런타임 초기화, 49 바이트)
$0900-$1C87  RLE 디컴프레스 버퍼 (5,000 바이트, 런타임 전용)
$2000-$3FFF  HGR 프레임버퍼 Page 1 (8,192 바이트)
$4000-$72CC  CODE 세그먼트 (~13KB)
$72CD-$8828  RODATA 세그먼트 (~5.5KB, RLE 압축 룸 데이터 포함)
$8829-$8941  DATA + BSS 세그먼트
$8E00-$95FF  C 스택 (2KB, 하향 성장)
$9600-$BEFF  BASIC.SYSTEM (보존, 덮어쓰기 금지)
$BF00-$BFFF  ProDOS 글로벌 페이지
```

바이너리 크기: **33,088 바이트** (BRUN 한계 $9600 이내, 여유 ~3.2KB)

## RLE 그리드 압축

### 배경

100x100 맵 3개의 니블 패킹 그리드(3 x 100 x 50 = 15,000 바이트)를 인라인으로
저장하면 바이너리가 ~46KB로 BASIC.SYSTEM의 BRUN 한계($9600)를 초과한다.
이로 인해 `BRUN HELLO,D2` 실행 시 **"NO BUFFERS AVAILABLE"** 오류가 발생한다.

### 해결: PackBits 방식 RLE 압축

룸 그리드 데이터를 PackBits 변형 RLE로 압축하여 ROM에 저장하고,
런타임에 $0900 버퍼로 디컴프레스한다.

**압축 형식:**
```
MSB=1: 반복 실행  → count = (cmd & 0x7F) + 2  (범위 2-129)
                    다음 1바이트를 count회 반복
MSB=0: 리터럴 실행 → count = cmd + 1           (범위 1-128)
                    다음 count바이트를 그대로 복사
```

**압축 효과:**
```
원본 (니블 패킹):  15,000 바이트 (3룸 x 100행 x 50바이트)
RLE 압축 후:       ~1,504 바이트
압축률:            ~90% (13.5KB 절감)
```

### 데이터 흐름

```
[빌드 타임]
  JSON 룸 파일
    → json_to_room_data.py
      → 니블 패킹 (100x100 타일 → 100x50 바이트)
        → rle_compress()
          → static const unsigned char room_N_grid_rle[] = { ... };
          → RoomDef.grid_rle = room_N_grid_rle;
          → RoomDef.grid_rle_size = sizeof(room_N_grid_rle);

[런타임]
  logic_get_tile_code(room, x, y)
    → logic_decompress_room(room)     // 캐시 미스 시만 실행
      → rle_decompress(grid_rle, grid_rle_size, GRID_BUFFER)
        → GRID_BUFFER ($0900-$1C87, 5,000 바이트)
    → GRID_BUFFER[y * 50 + (x >> 1)]  // 니블 추출
```

### 디컴프레스 버퍼

- 위치: `$0900-$1C87` (STARTUP 뒤, HGR 페이지 앞의 빈 RAM)
- 크기: 5,000 바이트 (100행 x 50 패킹 바이트)
- 한 번에 1개 룸만 보유, 룸 전환 시 자동 교체
- `g_loaded_room` 캐시로 동일 룸 재디컴프레스 방지

### RoomDef 구조체

```c
typedef struct {
    const char *id;
    const char *name;
    signed char z_level;
    const unsigned char *grid_rle;   // RLE 압축된 그리드 포인터
    unsigned int grid_rle_size;      // 압축 데이터 크기
    unsigned char door_count;
    DoorDef doors[MAX_DOORS];
    unsigned char stair_count;
    StairDef stairs[MAX_STAIRS];
    unsigned char box_count;
    BoxDef boxes[MAX_BOXES];
    unsigned char monster_count;
    MonsterDef monsters[MAX_MONSTERS];
    unsigned char player_start_x;
    unsigned char player_start_y;
} RoomDef;
```

## HGR 렌더링

### 타일 시스템

- 타일 크기: 7px x 8행 (HGR 1바이트 = 7픽셀, 비트 7=팔레트)
- 타일 그리드: 40열 x 24행 (280x192 전체 화면)
- 타일 데이터: 8바이트/타일, render.c에 static 배열로 내장
- 폰트: ASCII 32-122 (91엔트리, 73글리프 정의), render.c에 내장

### 화면 레이아웃 (40x24 타일)

```
Row 0:  룸 이름 (40열)
Row 1:  +----------+
Row 2:  | 10x10    |  X: nn  Y: nn
Row 3:  | 맵       |  Z: n
Row 4:  | 뷰포트   |
...     |          |  [상태 메시지]
Row11:  |          |
Row12:  +----------+
Row13-22: 메시지 영역 (효과/아이템)
Row23:  WASD:move H:help Q:quit
```

### 소프트 스위치 (HGR 모드)

```c
*(volatile char*)0xC05F = 0;  // DHIRES_CLR
*(volatile char*)0xC00C = 0;  // 80COL_CLR
*(volatile char*)0xC000 = 0;  // 80STORE_CLR
*(volatile char*)0xC050 = 0;  // TXTCLR → 그래픽 모드
*(volatile char*)0xC052 = 0;  // MIXCLR → 전체 화면
*(volatile char*)0xC054 = 0;  // PAGE1
*(volatile char*)0xC057 = 0;  // HIRES
```

### HGR 행 주소 인터리브

HGR 메모리 배치가 비선형적이므로 192엔트리 룩업 테이블 사용:
```
Row 주소 = $2000 + (row/64)*$28 + (row%8)*$400 + ((row/8)%8)*$80
```

## 게임 시스템

### 모듈 구성

| 모듈 | 크기 | 역할 |
|------|------|------|
| main.c | ~1.5KB | 게임 루프, 상호작용 시퀀스 |
| render.c | ~3.4KB + 1.3KB rodata | HGR 렌더링, 텍스트, HUD |
| logic.c | ~2.4KB | 이동, 카메라, RLE 디컴프레서, 문/계단/상자 |
| monster.c | ~4.1KB | 몬스터 FSM (PATROL/CHASE/RETURNING), LOS |
| input.c | ~184B | 키보드 읽기 |
| help.c | ~1.2KB | 도움말 화면 (스크롤) |
| room_data.c | ~2.9KB rodata | 3룸 RLE 그리드 + 메타데이터 (자동 생성) |

### 몬스터 AI

3상태 FSM (Finite State Machine):
- **PATROL**: 4방향 웨이포인트 순찰
- **CHASE**: Bresenham LOS(Line of Sight) 기반 추격
- **RETURNING**: 홈 위치로 복귀 (감지 비활성)

플레이어 2턴당 몬스터 1턴 이동. 3룸 전체 상태 보존 (`RoomRuntime[ROOM_COUNT]`).

### 입력

블로킹 방식 (`cgetc()`). Apple II 키보드 하드웨어는 비디오 모드와 독립적.

| 키 | 기능 |
|----|------|
| W/I | 위 이동 |
| S/K | 아래 이동 |
| A/J | 왼쪽 이동 |
| D/L | 오른쪽 이동 |
| H | 도움말 |
| Q | 종료 |
| 1/0 | 예/아니오 (프롬프트) |

## 링커 설정 (apple2-hgr-ext.cfg)

cc65 기본 `apple2-hgr.cfg` 기반, `__HIMEM__=$9600`으로 설정:

- STARTUP: $0803 (ProDOS BIN 로드 주소)
- HGR: $2000-$3FFF (프레임버퍼 예약, 코드 배치 금지)
- CODE: $4000부터 배치 (HGR 영역 회피)
- __HIMEM__=$9600 (BASIC.SYSTEM 호환, BRUN으로 로드 가능)
- __FILETYPE__=$0006 (ProDOS BIN)

## 환경 변수

| 변수 | 기본값 | 용도 |
|------|--------|------|
| `PROGRAM_NAME` | `HELLO` | 디스크 내 파일명 |
| `DISK_IMAGE` | `build/prototype_02_appleii_prodos.po` | 출력 디스크 이미지 |
| `RDEDISKTOOL` | (자동 탐색) | rdedisktool 경로 |
| `BOOT_DISK` | ProDOS_2_4_3.po | 부트 디스크 |
| `APPLEWIN` | (자동 탐색) | sa2 경로 |

## prototype_01 대비 변경점

| 항목 | prototype_01 | prototype_02 |
|------|-------------|-------------|
| 화면 모드 | TEXT 40x24 (conio.h) | HGR 280x192 (자체 렌더링) |
| 플레이어/몬스터 | `!` / `$` 문자 | 7x8 비트맵 타일 |
| 코드 구조 | game.c 단일 파일 | 6개 모듈 분리 |
| 그리드 저장 | 인라인 `grid_packed[100][50]` | RLE 압축 포인터 + 런타임 디컴프레스 |
| 바이너리 크기 | ~46KB (BRUN 초과) | ~33KB (BRUN 이내) |
| 텍스트 출력 | conio.h `cputsxy()` | 커스텀 비트맵 폰트 렌더링 |
