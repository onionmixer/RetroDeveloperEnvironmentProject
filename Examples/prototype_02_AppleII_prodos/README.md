# prototype_02_AppleII_prodos

Apple II HGR (Hi-Res Graphics, 280x192) 던전 탐색 게임.
`prototype_01_AppleII_prodos`의 텍스트 모드를 타일 기반 그래픽으로 전환하고,
monolithic `game.c`를 모듈 분리한 버전.

맵 그리드(RLE)와 타일 패턴은 ProDOS 디스크 파일로 분리되어 있으며,
런타임에 직접 ProDOS MLI 호출로 로드한다.

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
BRUN PROTO02,D2
```

### 빌드 요구사항

- **cc65** (`cl65`, `ca65`, `ld65`) — `apt install cc65`
- **Python 3** — 룸 데이터 생성기 (`tools/json_to_room_data.py`), 타일셋 생성기 (`tools/gen_tileset.py`)
- **rdedisktool** — 디스크 이미지 생성 (프로젝트 내 빌드)
- **AppleWin (sa2)** — 에뮬레이터 실행 시

## 프로젝트 구조

```
prototype_02_AppleII_prodos/
├── src/
│   ├── main.c              진입점 + 게임 루프
│   ├── engine.h            공유 타입, 상수, 데이터 구조체
│   ├── render.c / render.h HGR 렌더링 (타일, 폰트, HUD, 타일셋 로딩)
│   ├── logic.c / logic.h   게임 로직 (이동, 카메라, RLE 디컴프레서)
│   ├── monster.c / monster.h  몬스터 AI (PATROL/CHASE/RETURNING FSM)
│   ├── input.c / input.h   키보드 입력 (cgetc 래퍼)
│   ├── help.c / help.h     도움말 화면
│   ├── room_data.c / room_data.h  룸 메타데이터 (자동 생성, RLE 데이터 미포함)
│   ├── prodos_prefix.s     ProDOS MLI 직접 호출 (파일 로딩, prefix 설정)
│   └── apple2-hgr-ext.cfg  cc65 커스텀 링커 설정
├── tools/
│   ├── json_to_room_data.py  JSON → C 메타데이터 + RLE 바이너리 변환기
│   ├── gen_tileset.py        타일 패턴 → 72B 바이너리 생성기
│   └── check_env.sh          빌드 환경 검증
├── compile.sh               빌드 스크립트
├── run_applewin_prodos.sh   에뮬레이터 실행 스크립트
└── build/                   빌드 산출물 (생성됨)
    ├── ROOM00, ROOM01, ROOM02   RLE 압축 맵 그리드 바이너리
    └── TILES0                   타일셋 비트패턴 바이너리 (72B)
```

## 메모리 맵

```
$0000-$007F  Zero Page (cc65 런타임)
$0080-$009F  Zero Page 변수
$0800-$0BFF  ProDOS MLI I/O 버퍼 (1,024B, 파일 열기 시 사용)
$0803-$0833  STARTUP (cc65 런타임 초기화, 49 바이트)
$0900-$1C87  GRID_BUFFER (RLE 디컴프레스 버퍼, 5,000 바이트)
$1C88-$1CCF  TILE_BUFFER (활성 타일셋, 9종 × 8B = 72 바이트)
$1CD0-$1FFF  FILE_BUFFER (디스크 파일 읽기 임시 버퍼, 816 바이트)
$2000-$3FFF  HGR 프레임버퍼 Page 1 (8,192 바이트)
$4000+       CODE 세그먼트 (~13KB)
             RODATA 세그먼트 (~4KB, 폰트+HGR테이블+문자열+메타데이터)
             DATA + BSS 세그먼트
$8E00-$95FF  C 스택 (2KB, 하향 성장)
$9600-$BEFF  BASIC.SYSTEM (보존, 덮어쓰기 금지)
$BF00-$BFFF  ProDOS 글로벌 페이지
```

바이너리 크기: **32,111 바이트** (BRUN 한계 $9600 이내, 여유 ~4.2KB)

**MLI I/O 버퍼($0800)와 GRID_BUFFER($0900) 겹침 해결:**
파일이 열려 있는 동안 $0800-$0BFF는 MLI 전용이므로, 초기화 시
타일셋 로딩(TILES→TILE_BUFFER)을 룸 데이터 로딩(ROOM→FILE_BUFFER→GRID_BUFFER) 전에
수행하여 I/O 버퍼 해제 후 GRID_BUFFER에 안전하게 디컴프레스한다.

## ProDOS 파일 I/O

### 직접 MLI 호출 (prodos_prefix.s)

cc65의 C 라이브러리 파일 I/O(`<fcntl.h>`)를 사용하지 않고,
ProDOS MLI(Machine Language Interface, $BF00)를 6502 어셈블리로 직접 호출한다.

**이유**: cc65 `open()/read()/close()`가 이 프로젝트 구성에서 정상 동작하지 않아
직접 MLI 호출로 대체함. 이 방식이 코드 크기도 더 작다.

**제공 함수:**

| 함수 | 프로토타입 | 기능 |
|------|-----------|------|
| `_set_prefix_from_boot_device` | `void set_prefix_from_boot_device(void)` | DEVNUM($BF30)으로 부트 볼륨명 감지, prefix 설정, 경로 캐시 |
| `_prodos_load_room` | `unsigned int __fastcall__ prodos_load_room(unsigned char room)` | ROOMnn 파일을 FILE_BUFFER($1CD0)에 로드, 읽은 바이트 수 반환 |
| `_prodos_load_tileset` | `unsigned char __fastcall__ prodos_load_tileset(unsigned char id)` | TILESn 파일을 TILE_BUFFER($1C88)에 로드, 성공=1/실패=0 |
| `_prodos_get_last_error` | `unsigned char prodos_get_last_error(void)` | 마지막 MLI 에러코드 반환 (0=성공) |

**절대 경로 구성**: ON_LINE($C5)으로 볼륨명을 얻어 `/VOLNAME/ROOMnn` 형태의
절대 경로를 빌드. SET_PREFIX 성공 여부와 무관하게 동작한다.

## RLE 그리드 압축

### 배경

100x100 맵 3개의 니블 패킹 그리드(3 x 100 x 50 = 15,000 바이트)를 인라인으로
저장하면 바이너리가 ~46KB로 BASIC.SYSTEM의 BRUN 한계($9600)를 초과한다.

### 해결: PackBits 방식 RLE 압축 + 외부 파일

룸 그리드 데이터를 PackBits 변형 RLE로 압축하여 ProDOS 디스크 파일(ROOMnn)로 저장하고,
런타임에 FILE_BUFFER($1CD0)로 읽은 뒤 GRID_BUFFER($0900)로 디컴프레스한다.

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
RLE 압축 후:       ~1,504 바이트 (ROOM00-02 파일 합계)
압축률:            ~90% (13.5KB 절감)
```

### 데이터 흐름

```
[빌드 타임]
  JSON 룸 파일
    → json_to_room_data.py --binary-dir build/
      → 니블 패킹 (100x100 타일 → 100x50 바이트)
        → rle_compress()
          → build/ROOM00, build/ROOM01, build/ROOM02 (바이너리 파일)
          → src/room_data.c (메타데이터만, RLE 배열 없음)
  gen_tileset.py
    → build/TILES0 (72 바이트 타일셋 바이너리)

[런타임]
  set_prefix_from_boot_device()
    → ON_LINE($C5) → 볼륨명 캐시 ("/VOLNAME/")

  render_load_tileset(tileset_id)
    → prodos_load_tileset(id)
      → MLI OPEN/READ/CLOSE "/VOLNAME/TILESn"
        → TILE_BUFFER ($1C88, 72 바이트)

  logic_get_tile_code(room, x, y)
    → logic_decompress_room(room)     // 캐시 미스 시만 실행
      → prodos_load_room(room)
        → MLI OPEN/READ/CLOSE "/VOLNAME/ROOMnn"
          → FILE_BUFFER ($1CD0)
      → rle_decompress(FILE_BUFFER, n, GRID_BUFFER)
        → GRID_BUFFER ($0900-$1C87, 5,000 바이트)
    → GRID_BUFFER[y * 50 + (x >> 1)]  // 니블 추출
```

### 디컴프레스 버퍼

- GRID_BUFFER: `$0900-$1C87` (5,000 바이트, 100행 x 50 패킹 바이트)
- FILE_BUFFER: `$1CD0-$1FFF` (816 바이트, RLE 파일 읽기 임시 저장)
- TILE_BUFFER: `$1C88-$1CCF` (72 바이트, 활성 타일셋)
- 한 번에 1개 룸만 보유, 룸 전환 시 자동 교체
- `g_loaded_room` / `g_loaded_tileset` 캐시로 동일 데이터 재로딩 방지

### RoomDef 구조체

```c
typedef struct {
    const char *id;
    const char *name;
    signed char z_level;
    unsigned char tileset_id;        /* 타일셋 파일 번호 (0=TILES0, ...) */
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
- 맵 타일 (8종): TILE_BUFFER에서 로드 (외부 TILES 파일)
- UI 타일 (8종): Player, Monster, Border 6종은 바이너리 내 static const 유지
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

| 모듈 | 역할 |
|------|------|
| main.c | 게임 루프, 상호작용 시퀀스 |
| render.c | HGR 렌더링, 텍스트, HUD, 타일셋 로딩 |
| logic.c | 이동, 카메라, RLE 디컴프레서, 문/계단/상자 |
| monster.c | 몬스터 FSM (PATROL/CHASE/RETURNING), LOS |
| input.c | 키보드 읽기 |
| help.c | 도움말 화면 (스크롤) |
| room_data.c | 3룸 메타데이터 (자동 생성, RLE 데이터 미포함) |
| prodos_prefix.s | ProDOS MLI 직접 호출 (ON_LINE, SET_PREFIX, OPEN, READ, CLOSE) |

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
| `PROGRAM_NAME` | `PROTO02` | 디스크 내 파일명 |
| `DISK_IMAGE` | `build/prototype_02_appleii_prodos.po` | 출력 디스크 이미지 |
| `RDEDISKTOOL` | (자동 탐색) | rdedisktool 경로 |
| `BOOT_DISK` | ProDOS_2_4_3.po | 부트 디스크 |
| `APPLEWIN` | (자동 탐색) | sa2 경로 |

## ProDOS 디스크 파일 구성

```
ProDOS 디스크 (140KB = 280 블록):
├── PRODOS            (부트 커널)
├── BASIC.SYSTEM      (BASIC 인터프리터)
├── PROTO02           (게임 바이너리, type=BIN, 32,111B)
├── TILES0            (타일셋 0, type=BIN, 72B → 1 블록)
├── ROOM00            (룸 0 RLE 그리드, type=BIN, ~512B → 1 블록)
├── ROOM01            (룸 1 RLE 그리드, type=BIN, ~548B → 2 블록)
└── ROOM02            (룸 2 RLE 그리드, type=BIN, ~512B → 1 블록)
```

추가 룸 용량: 잔여 ~127 블록 → 평균 1블록/룸 기준 **~127개 룸** 추가 가능.

## prototype_01 대비 변경점

| 항목 | prototype_01 | prototype_02 |
|------|-------------|-------------|
| 화면 모드 | TEXT 40x24 (conio.h) | HGR 280x192 (자체 렌더링) |
| 플레이어/몬스터 | `!` / `$` 문자 | 7x8 비트맵 타일 |
| 코드 구조 | game.c 단일 파일 | 8개 모듈 분리 (C 7개 + ASM 1개) |
| 그리드 저장 | 인라인 `grid_packed[100][50]` | 외부 ProDOS 파일 (ROOMnn) + RLE 압축 |
| 타일 패턴 | 해당 없음 | 외부 ProDOS 파일 (TILESn), 룸별 타일셋 교체 가능 |
| 파일 I/O | 없음 | 직접 ProDOS MLI 호출 (6502 ASM) |
| 바이너리 크기 | ~46KB (BRUN 초과) | ~32KB (BRUN 이내) |
| 바이너리 이름 | HELLO | PROTO02 |
| 텍스트 출력 | conio.h `cputsxy()` | 커스텀 비트맵 폰트 렌더링 |
