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
| 맵 저장 | 인라인 `grid_packed[100][50]` | 외부 ProDOS 파일(ROOMnn) + RLE 압축 |
| 타일 패턴 | 해당 없음 | 외부 ProDOS 파일(TILESn), 룸별 교체 가능 |
| 파일 I/O | 없음 | 직접 ProDOS MLI 호출 (6502 ASM) |
| 컴파일러 | cc65 (cl65) | cc65 (cl65) |
| 그래픽 라이브러리 | conio.h | 자체 HGR 루틴 |
| 코드 구조 | game.c 모놀리식 | render/logic/monster/input/help + prodos_prefix.s 분리 |
| 바이너리 크기 | ~46KB (BRUN 초과) | ~32KB (BRUN 이내, HIMEM=$9600) |
| 바이너리 이름 | HELLO | PROTO02 |
| 출력 형식 | ProDOS Binary | ProDOS Binary (동일) |

---

## 2. 기술 분석 (구현 완료 — 요약)

### 2.1 Apple II HGR 핵심 구조

- 해상도 280×192, 40 바이트/행, Page 1 = $2000-$3FFF (8KB)
- 타일: 7px×8행, 1바이트=7픽셀, bit 7=팔레트(항상 0), 인접 비트 ON→흰색
- 행 주소 비선형 인터리브 → 192엔트리 워드 테이블로 해결
- 소프트 스위치: $C050(TXTCLR), $C052(MIXCLR), $C054(PAGE1), $C057(HIRES)
- IIe/IIc 호환: $C05F(DHIRES_CLR), $C00C(80COL_CLR), $C000(80STORE_CLR) 선행 필요

### 2.2 주요 기술 결정 사항

| 항목 | 결정 |
|------|------|
| 키보드 입력 | `cgetc()` 블로킹 (HGR 모드에서도 $C000/$C010 정상 동작) |
| 그리드 저장 | PackBits 변형 RLE 압축 (MSB=1: 반복, MSB=0: 리터럴) |
| 맵 데이터 | 외부 ProDOS 파일(ROOMnn) + 런타임 로딩 (직접 MLI 호출) |
| 타일 패턴 | 외부 ProDOS 파일(TILESn) + 런타임 로딩, 룸별 교체 가능 |
| 디컴프레스 | $0900-$1C87 버퍼 (5,000B), `g_loaded_room` 캐시로 중복 방지 |
| 스프라이트 | 소프트웨어 방식 (이전 위치 맵 타일 복원 → 새 위치 타일 그리기) |
| 텍스트 출력 | 커스텀 비트맵 폰트 (91엔트리, 5×7 글리프 in 7×8 셀) |
| 몬스터 AI | prototype_01 전체 유지 (PATROL/CHASE/RETURNING + Bresenham LOS) |
| 룸 상태 | `RoomRuntime g_runtime[ROOM_COUNT]` — 전 룸 보존 |
| 파일 I/O | 직접 ProDOS MLI ASM (`prodos_prefix.s`), cc65 C 라이브러리 미사용 |

---

## 3. 화면 레이아웃 (구현 완료 — 요약)

```
Row 0:  룸 이름 (40열)
Row 1:  +---------+ (테두리)
Row 2-11: |10×10 맵|  X:nn Y:nn Z:n / 상태 메시지
Row12:  +---------+
Row13-22: 메시지 영역 (효과/아이템)
Row23:  WASD:move H:help Q:quit
```

| 영역 | 타일 좌표 | 크기 |
|------|----------|------|
| 맵 뷰포트 | (1,2)-(10,11) | 10×10 |
| 좌표/상태 | (13,2)-(39,10) | 27×9 |
| 메시지 영역 | (0,14)-(39,21) | 40×8 |

---

## 4. 타일 설계 (구현 완료 — 요약)

16종 타일 + 91엔트리 폰트, 모두 7×8 bit 7=0.

- 맵 타일 (8종): 외부 TILES 파일에서 로드 → TILE_BUFFER ($1C88)
  - FLOOR, WALL, DOOR_H, DOOR_V, STAIR_DN, STAIR_UP, BOX, EMPTY
- UI 타일 (8종): 바이너리 내 static const 유지
  - Player, Monster, Border 6종 (TL, TR, BL, BR, H, V)
- 폰트: ASCII 32-122, 대문자 패턴을 소문자에 복제
- 문(TILE_DOOR): 인접 셀 DOOR 확인으로 H/V 자동 분기

구현 파일: `src/render.c` (UI 타일+폰트+포인터 테이블), `src/prodos_prefix.s` (타일셋 로딩)

---

## 5. 모듈 아키텍처 (구현 완료 — 요약)

### 5.1 파일 구조

```
src/
├── main.c          — 진입점 + 게임 루프
├── render.h/c      — HGR 렌더링 (행 테이블, draw_tile, 텍스트, 프롬프트, 타일셋 로딩)
├── logic.h/c       — RLE 디컴프레서, 이동, 카메라, 문/계단/상자
├── monster.h/c     — 몬스터 FSM (PATROL/CHASE/RETURNING, LOS, 충돌)
├── input.h/c       — 키보드 (cgetc 래퍼, INPUT_* 코드)
├── help.h/c        — 도움말 화면 (스크롤, HGR 텍스트)
├── engine.h        — 공유 타입 (GameState, MoveResult, TileCode, RoomDef 등)
├── room_data.h/c   — 룸 메타데이터 (json_to_room_data.py 생성, RLE 데이터 미포함)
├── prodos_prefix.s — ProDOS MLI 직접 호출 (ON_LINE, SET_PREFIX, OPEN, READ, CLOSE)
└── apple2-hgr-ext.cfg — 커스텀 링커 (HGR 회피, HIMEM=$9600)
```

### 5.2 주요 API

| 모듈 | 핵심 함수 |
|------|----------|
| render | `render_init()`, `render_load_tileset()`, `render_draw_map()`, `render_draw_monsters()`, `render_update_player()`, `render_print()`, `render_print_wrap()`, `render_prompt_yes_no()`, `render_redraw_all()`, `render_cleanup()` |
| logic | `logic_decompress_room()`, `logic_get_tile_code()`, `logic_try_move()`, `logic_update_camera()`, `logic_find_door/stair/box()`, `logic_do_door/stair()` |
| monster | `monster_init_all()`, `monster_update_all()`, `monster_check_collision()`, `monster_format_encounter_msg()`, `monster_index_at()` |
| input | `input_read_blocking()`, `input_wait_yes_no()`, `input_wait_any_key()` |
| help | `help_show()` |
| prodos_prefix.s | `set_prefix_from_boot_device()`, `prodos_load_room()`, `prodos_load_tileset()`, `prodos_get_last_error()` |

### 5.3 게임 루프 흐름

블로킹 입력 → 이동/상호작용 처리 → 1차 충돌 검사 → 2턴마다 몬스터 업데이트 → 2차 충돌 검사 → 카메라 갱신 → dirty 플래그 기반 렌더링

### 5.4 몬스터 인카운터

- 3상태 FSM: PATROL(4방향 웨이포인트) → CHASE(Bresenham LOS) → RETURNING(홈 복귀)
- 이중 충돌 검사: 플레이어→몬스터 + 몬스터→플레이어
- 2턴 타이밍: `move_counter >= 2` 시 `monster_update_all()` 호출
- 충돌 시: RETURNING 전환 + detect_enabled=0 + "Encountered {name}!" 메시지
- 14체 8종 (Room 1: 7체, Room 2: 4체, Room 3: 3체)

---

## 6-7. 입력/빌드 시스템 (구현 완료 — 요약)

- **입력**: cgetc() 블로킹, WASD/IJKL 이동, H 도움말, Q 종료, 1/0 확인/취소
- **빌드**: `cl65 -t apple2 -C apple2-hgr-ext.cfg -O` → `tail -c +59` → `rdedisktool add --type B --addr 0x0803`
- **실행**: `./compile.sh run` → sa2 --d1 boot.po --d2 game.po → `BRUN PROTO02,D2`

---

## 8. 메모리 예산

```
$0800-$0BFF    1,024B   MLI I/O 버퍼 (파일 열기 시 사용, prodos_prefix.s에서 관리)
$0803-$0833    48B      STARTUP
$0900-$1C87    5,000B   GRID_BUFFER (디컴프레스된 100×50 그리드)
$1C88-$1CCF    72B      TILE_BUFFER (활성 타일셋, 9종 × 8B)
$1CD0-$1FFF    816B     FILE_BUFFER (디스크 파일 읽기 임시 버퍼)
$2000-$3FFF    8,192B   HGR Page 1 프레임버퍼
$4000+         ~13KB    CODE
               ~4KB     RODATA (폰트 728B + HGR테이블 384B + 문자열 + 메타데이터)
               ~360B    DATA+BSS (+ prodos_prefix.s BSS 변수)
$8E00-$95FF    2,048B   C 스택

바이너리: 32,111B, BRUN 한계까지 여유 ~4.2KB
RLE 압축 효과: 15,000B → 1,504B (90% 절감, ~13.5KB 절약)
외부 파일 분리 효과: RLE 1,572B + 타일 64B 제거, MLI 코드 +~600B → 순 절약 ~1,000B
```

---

## 9. 구현 단계 (전체 완료)

| Step | 내용 | 상태 |
|------|------|------|
| 1 | 프로젝트 스켈레톤 + HGR 검은 화면 | 완료 |
| 2 | HGR 렌더링 (행 테이블, draw_tile) | 완료 |
| 3 | 폰트 + 텍스트 렌더링 (render_print) | 완료 |
| 4 | 타일 디자인 + 맵 렌더링 + RLE 디컴프레서 | 완료 |
| 5 | 플레이어 이동 + 입력 + 카메라 | 완료 |
| 6 | 몬스터 AI (PATROL/CHASE/RETURNING + LOS) | 완료 |
| 7 | 문/계단/상자 상호작용 | 완료 |
| 8 | 도움말 화면 (스크롤, 복귀 리드로우) | 완료 |
| 9 | 마무리 (종료 프롬프트, TEXT 복귀, 스모크 테스트) | 완료 |

---

## 10. 테스트 항목

| # | 항목 | 상태 |
|---|------|------|
| 1 | HGR 모드 전환 + 맵/테두리 표시 | 확인 |
| 2 | 비트맵 폰트 텍스트 렌더링 | 확인 |
| 3 | WASD 이동, 벽 충돌, 카메라 추적 | 확인 |
| 4 | 몬스터 순찰/추격/충돌/귀환 | 확인 |
| 5 | 문/계단/상자 상호작용 (1=예, 0=아니오) | 확인 |
| 6 | 도움말 (H→스크롤→SPACE 복귀) | 확인 |
| 7 | 종료 (Q→확인→TEXT 복귀) | 확인 |
| 8 | 3개 방 전체 탐색 스모크 테스트 | 확인 |
| 9 | 외부 파일 로딩 (ROOM00-02, TILES0) | 확인 |
| 10 | 룸 전환 시 타일셋 로딩/캐시 | 확인 |

---

## 11. 리스크 및 대응

| 리스크 | 대응 |
|--------|------|
| HGR 프레임버퍼-코드 충돌 | `apple2-hgr-ext.cfg` 커스텀 링커 사용 |
| 타일 색상 아티팩트 | bit 7=0 고정, 인접 비트 쌍으로 흰색 보장 |
| 메모리 부족 | RLE 압축 + 외부 파일 분리로 해결 |
| cc65 C 라이브러리 파일 I/O 비동작 | 직접 ProDOS MLI ASM으로 대체 (`prodos_prefix.s`) |
| I/O 버퍼-GRID_BUFFER 겹침 | 초기화 순서 제어 (타일셋→룸 데이터) |
| D2 prefix 문제 | ON_LINE으로 볼륨명 취득 후 절대 경로 구성 |

---

## 12. 수정/생성 파일 목록

| 파일 | 상태 |
|------|------|
| `src/main.c`, `render.h/c`, `logic.h/c`, `monster.h/c`, `input.h/c`, `help.h/c` | 신규 (구현 완료) |
| `src/engine.h` | 수정 (MoveResult, MAP_ORIGIN_*, running, tileset_id 추가) |
| `src/prodos_prefix.s` | 신규 (ProDOS MLI 직접 호출, 구현 완료) |
| `src/room_data.h/c` | 생성 (json_to_room_data.py, 메타데이터만) |
| `src/apple2-hgr-ext.cfg` | 신규 (커스텀 링커) |
| `compile.sh`, `run_applewin_prodos.sh` | 신규 (구현 완료) |
| `tools/json_to_room_data.py` | 수정 (RLE 압축 + --binary-dir 바이너리 출력) |
| `tools/gen_tileset.py` | 신규 (타일셋 72B 바이너리 생성) |

---

## 13. 참고 자료

- prototype_01 소스: `Examples/prototype_01_AppleII_prodos/src/`
- prototype_02 MSX 소스: `Examples/prototype_02_MSX_ROM_MSXDOS/src/`
- souless_apple2: `Library/AppleII/souless_apple2/common/source/` (video.s, videoOffset.s, sprite.s)
- ProDOS 부트 디스크: `diskwork/bootdisk/AppleII/ProDOS_2_4_3.po`
- rdedisktool: ProDOS 타입명 지원 (`--type SYS/BIN/TXT`, `0xFF`)

---

## Appendix B: 맵/타일 데이터 외부 파일 분리 (구현 완료)

> 분석일: 2026-03-01
> 구현 완료일: 2026-03-01
> 목적: 맵 데이터와 타일 데이터를 ProDOS 디스크 상의 별도 파일로 분리하여
> 더 많은 룸 또는 시각적 다양성을 확보한다.

### B.1 구현 결과 요약

| 항목 | 변경 전 | 변경 후 |
|------|---------|---------|
| 바이너리 크기 | 33,088B | 32,111B (-977B) |
| 바이너리 이름 | HELLO | PROTO02 |
| RLE 맵 데이터 | room_data.c 인라인 (1,572B) | 외부 파일 ROOM00-02 |
| 타일 패턴 | render.c static const (64B) | 외부 파일 TILES0 (72B) |
| 파일 I/O | 없음 | 직접 ProDOS MLI ASM (prodos_prefix.s) |
| RoomDef 필드 | grid_rle, grid_rle_size | tileset_id |
| 디스크 파일 수 | 1 (HELLO) | 5 (PROTO02 + ROOM00-02 + TILES0) |
| 추가 룸 용량 | 0 (바이너리 크기 한계) | ~127룸 (디스크 잔여 용량) |

### B.2 메모리 배치 (구현 후)

```
주소 범위           크기         용도
──────────────────────────────────────────────────
$0800-$0BFF        1,024 bytes  MLI I/O 버퍼 (prodos_prefix.s, 파일 열기 시 사용)
$0803-$0833        48 bytes     STARTUP (cc65 런타임 초기화)
$0900-$1C87        5,000 bytes  GRID_BUFFER (디컴프레스된 100×50 패킹 그리드)
$1C88-$1CCF        72 bytes     TILE_BUFFER (활성 타일셋, 9종 × 8B)
$1CD0-$1FFF        816 bytes    FILE_BUFFER (디스크 파일 읽기 임시 버퍼)
$2000-$3FFF        8,192 bytes  HGR Page 1 프레임버퍼
$4000+             ~13KB        CODE 세그먼트 (+ prodos_prefix.s)
                   ~4KB         RODATA 세그먼트 (RLE/타일 제거 후)
                   ~360 bytes   DATA + BSS 세그먼트
$8E00-$95FF        2,048 bytes  C 스택 (하향 성장)
$9600-$BEFF        보호 영역    BASIC.SYSTEM
$BF00-$BFFF        보호 영역    ProDOS 글로벌 페이지
──────────────────────────────────────────────────
바이너리 크기: 32,111 bytes
BRUN 한계까지 여유: ~4,200 bytes
```

**MLI I/O 버퍼($0800)와 GRID_BUFFER($0900) 겹침 해결:**

`prodos_prefix.s`에서 I/O 버퍼를 $0800에 고정 배치한다.
I/O 버퍼와 GRID_BUFFER가 겹치지만, 초기화 순서를 다음과 같이 제어하여 안전:

```
1. render_load_tileset() → OPEN/READ(TILE_BUFFER $1C88)/CLOSE (I/O 버퍼 해제)
2. logic_init() → logic_decompress_room()
   → OPEN/READ(FILE_BUFFER $1CD0)/CLOSE → rle_decompress → GRID_BUFFER ($0900)
```

### B.3 ProDOS 디스크 파일 구조

```
ProDOS 디스크 (140KB = 280 블록 × 512B):
├── PRODOS            (부트 커널, type=$FF, ~32KB = 64 블록)
├── BASIC.SYSTEM      (BASIC 인터프리터, type=$FF, ~10KB = 20 블록)
├── PROTO02           (게임 바이너리, type=$06/BIN, 32,111B, 로드 주소 $0803)
├── TILES0            (타일셋 0, type=$06, 72 bytes → 1 블록, 로드 주소 $1C88)
├── ROOM00            (룸 0: RLE 그리드, type=$06, ~512B → 1 블록, 로드 주소 $1CD0)
├── ROOM01            (룸 1: RLE 그리드, type=$06, ~548B → 2 블록, 로드 주소 $1CD0)
└── ROOM02            (룸 2: RLE 그리드, type=$06, ~512B → 1 블록, 로드 주소 $1CD0)
```

### B.4 파일 I/O 구현: 직접 ProDOS MLI 호출 (prodos_prefix.s)

#### B.4.1 cc65 C 라이브러리 파일 I/O 불채택 사유

당초 계획(B.6.1)에서는 cc65의 `<fcntl.h>` + `apple2-iobuf-0800.o` 조합을 권장했으나,
실제 구현에서 `open()/read()/close()` C 함수가 정상 동작하지 않았다.

**시도한 접근:**
1. `chdir("/볼륨명")` + 상대 경로 open → 실패
2. 6502 ASM으로 ON_LINE→chdir() 호출 → 실패
3. 6502 ASM으로 ON_LINE→SET_PREFIX MLI 호출 + C open() → 실패

정확한 원인은 미확인이나, cc65 런타임의 파일 I/O 초기화, I/O 버퍼 할당,
또는 ProDOS FCB 관리 코드와의 충돌이 의심됨.

#### B.4.2 채택: 직접 ProDOS MLI ASM (prodos_prefix.s)

cc65 C 라이브러리를 완전히 우회하여, ProDOS MLI($BF00)를 6502 어셈블리로 직접 호출.
결과적으로 코드 크기도 더 작고 동작이 안정적이다.

**구현 파일**: `src/prodos_prefix.s` (~420 lines)

**제공 함수:**

| Export | C 프로토타입 | 기능 |
|--------|-------------|------|
| `_set_prefix_from_boot_device` | `void set_prefix_from_boot_device(void)` | DEVNUM→ON_LINE→SET_PREFIX, 볼륨명 캐시 |
| `_prodos_load_room` | `unsigned int __fastcall__ prodos_load_room(unsigned char room)` | ROOMnn → FILE_BUFFER($1CD0), 반환: 읽은 바이트 수 |
| `_prodos_load_tileset` | `unsigned char __fastcall__ prodos_load_tileset(unsigned char id)` | TILESn → TILE_BUFFER($1C88), 반환: 1=성공, 0=실패 |
| `_prodos_get_last_error` | `unsigned char prodos_get_last_error(void)` | 마지막 MLI 에러코드 |

**핵심 설계:**

- **절대 경로 구성**: ON_LINE($C5)으로 DEVNUM 디바이스의 볼륨명을 취득하여
  `fname_buf`에 `/VOLNAME/` 형태로 캐시. 이후 파일명 로딩 시 `/VOLNAME/ROOMnn` 등
  절대 경로를 빌드하여 SET_PREFIX 의존성을 제거.

- **I/O 버퍼**: `IO_BUF = $0800` 고정. apple2-iobuf-0800.o와 동일 위치이나
  cc65 C 라이브러리를 사용하지 않으므로 충돌 없음.

- **OPEN/READ/CLOSE 시퀀스** (`do_orc` 내부 루틴):
  1. OPEN($C8): pathname=fname_buf, io_buffer=$0800 → ref_num 취득
  2. READ($CA): ref_num, data_buffer/request_count는 호출자가 설정
  3. CLOSE($CC): ref_num으로 파일 닫기 (에러 시에도 반드시 실행)
  4. 반환: AX = trans_count (읽은 바이트 수), 에러 시 0

- **6502 분기 제한 대응**: `beq`/`bne`/`bcs`/`bcc`는 -128..+127 범위 한정.
  장거리 분기는 `bne label; jmp target; label:` 반전 패턴으로 해결.

#### B.4.3 MLI 호출 규약

ProDOS MLI 엔트리: `$BF00`. 6502 코드에서 다음 패턴으로 호출:

```asm
        JSR $BF00       ; MLI 진입
        .byte $C8       ; 호출 번호 (OPEN)
        .word param_list ; 파라미터 목록 주소 (little-endian)
        BCC success     ; C=0 성공, C=1 실패 (A=에러코드)
```

### B.5 파일 포맷 상세

#### B.5.1 ROOM 파일 (순수 RLE 바이너리)

파일 내용 = RLE 압축 바이트 스트림 (헤더 없음). 디컴프레스 → 5,000 bytes (100행 × 50 패킹 바이트).

```
ROOM00: ~512 bytes (1블록)
ROOM01: ~548 bytes (2블록)
ROOM02: ~512 bytes (1블록)
합계: ~1,572 bytes → 디스크 4블록(2,048B)
```

**로딩 흐름:**
```
prodos_load_room(room_number)
  → MLI OPEN/READ/CLOSE "/VOLNAME/ROOMnn"
    → FILE_BUFFER($1CD0) ← 읽은 RLE 바이트 (반환값 n = 크기)
rle_decompress(FILE_BUFFER, n, GRID_BUFFER)
  → GRID_BUFFER($0900-$1C87, 5,000 바이트)
```

#### B.5.2 TILES 파일 (타일 비트패턴)

9종 타일 × 8 bytes = 72 bytes.

```
TILES0 파일 내용 (72 bytes):
  offset 0x00: tile_floor[8]       (인덱스 0, TILE_FLOOR)
  offset 0x08: tile_wall[8]        (인덱스 1, TILE_WALL)
  offset 0x10: tile_door_h[8]      (인덱스 2, TILE_DOOR 수평)
  offset 0x18: tile_door_v[8]      (인덱스 3, TILE_DOOR 수직)
  offset 0x20: tile_stair_dn[8]    (인덱스 4, TILE_STAIR_DOWN)
  offset 0x28: tile_stair_up[8]    (인덱스 5, TILE_STAIR_UP)
  offset 0x30: tile_box[8]         (인덱스 6, TILE_BOX)
  offset 0x38: tile_empty[8]       (인덱스 7, TILE_EMPTY/default)
  offset 0x40: 예비[8]             (인덱스 8, 향후 확장용)
```

**로딩 흐름:**
```
prodos_load_tileset(tileset_id)
  → MLI OPEN/READ/CLOSE "/VOLNAME/TILESn"
    → TILE_BUFFER($1C88) ← 72 bytes 직접 로드
```

Player(`tile_player`), Monster(`tile_monster`), Border 6종 — 바이너리 내 static const 유지.

### B.6 코드 변경 사항 (구현 완료)

#### B.6.1 engine.h — RoomDef 구조체

```c
/* grid_rle, grid_rle_size 제거, tileset_id 추가 */
typedef struct {
    const char *id;
    const char *name;
    signed char z_level;
    unsigned char tileset_id;        /* 타일셋 파일 번호 (0=TILES0, ...) */
    unsigned char door_count;
    DoorDef doors[MAX_DOORS];
    /* ... 이하 동일 ... */
} RoomDef;
```

#### B.6.2 logic.c — logic_decompress_room()

```c
/* 직접 ProDOS MLI 호출 (prodos_prefix.s) */
unsigned int __fastcall__ prodos_load_room(unsigned char room);

void logic_decompress_room(unsigned char room)
{
    unsigned int n;
    if (room == g_loaded_room) return;
    n = prodos_load_room(room);
    if (n > 0) {
        rle_decompress(FILE_BUFFER, n, GRID_BUFFER);
        g_loaded_room = room;
        return;
    }
    memset(GRID_BUFFER, 0x11, 5000);  /* 에러 폴백: 전체 벽 */
    g_loaded_room = room;
}
```

#### B.6.3 render.c — 타일셋 로딩

```c
/* 직접 ProDOS MLI 호출 (prodos_prefix.s) */
unsigned char __fastcall__ prodos_load_tileset(unsigned char tileset_id);

#define TILE_BUFFER ((const unsigned char *)0x1C88)
static unsigned char g_loaded_tileset = 0xFF;

void render_load_tileset(unsigned char tileset_id)
{
    if (tileset_id == g_loaded_tileset) return;
    if (prodos_load_tileset(tileset_id)) {
        g_loaded_tileset = tileset_id;
    }
}
```

`tile_code_to_pattern()` — `tile_floor` 등 static const 참조를 `TILE_BUFFER[N*8]` 오프셋 참조로 변경.

#### B.6.4 main.c — 초기화/전환

```c
void main(void)
{
    set_prefix_from_boot_device();
    /* 타일셋을 룸 데이터보다 먼저 로드 (I/O 버퍼 겹침 회피) */
    render_load_tileset(g_rooms[0].tileset_id);
    monster_init_all();
    logic_init(&st);    /* 내부에서 logic_decompress_room(0) 호출 */
    render_init();
    /* ... */
}
```

룸 전환 (MOVE_DOOR/MOVE_STAIR): `logic_do_door()`/`logic_do_stair()` 성공 후
`render_load_tileset(g_rooms[st.room].tileset_id)` 호출.

#### B.6.5 room_data.c — 메타데이터만 유지

`room_N_grid_rle[]` 배열 3개 전체 삭제 (1,572 bytes RODATA 절약).
`g_rooms[]` 초기화에서 `.grid_rle`/`.grid_rle_size` → `.tileset_id=0`.

#### B.6.6 빌드 파이프라인 변경

**compile.sh:**
- `gen_data()`: `json_to_room_data.py --binary-dir "$BUILD_DIR"` → build/ROOM00-02 바이너리 생성
- `gen_tileset()`: `gen_tileset.py --out-dir "$BUILD_DIR"` → build/TILES0 생성
- `build()`: `cl65` 링크에 `src/prodos_prefix.s` 추가 (apple2-iobuf-0800.o는 불필요)
- `disk()`: ROOM?? 및 TILES? 파일을 디스크 이미지에 추가하는 루프 추가

### B.7 코드 비용 vs 절약 (실측)

| 항목 | 바이트 변화 | 비고 |
|------|-------------|------|
| prodos_prefix.s (MLI ASM) | +~600 | OPEN/READ/CLOSE + ON_LINE + SET_PREFIX |
| logic_decompress_room() 변경 | -~50 | C open/read/close 제거, ASM 호출로 대체 |
| render_load_tileset() + tile_code_to_pattern() 변경 | -~20 | static const 삭제, ASM 호출로 대체 |
| room_N_grid_rle[] 3개 삭제 | **-1,572** | 512+548+512 bytes RODATA |
| 맵 타일 static const 8개 삭제 | **-64** | tile_floor ~ tile_empty |
| RoomDef grid_rle/size → tileset_id | -9 | (4B-1B) × 3룸 |
| apple2-iobuf-0800.o 미사용 | -~100 | cc65 I/O 버퍼 할당 코드 불필요 |
| **순 절약** | **~977 bytes** | 33,088B → 32,111B |

### B.8 ProDOS MLI 에러코드 (파일 로딩 관련)

| 코드 | 의미 | 대응 |
|------|------|------|
| `$27` | I/O error | prodos_get_last_error()로 확인 |
| `$40` | Invalid pathname syntax | 코딩 오류 (디버그) |
| `$44` | Path not found | prefix/볼륨명 확인 |
| `$46` | File not found | 디스크에 파일 미포함 |
| `$4C` | End of file | trans_count 확인 (정상) |
| `$56` | Bad buffer address | I/O 버퍼 페이지 정렬 확인 |

### B.9 제약 사항

| 제약 | 설명 |
|------|------|
| 맵 크기 확대 불가 | GRID_BUFFER 5,000B 고정, 100×100이 최대 |
| 디스크 접근 지연 | 룸 전환 시 ~0.5-1초 (1-2블록 읽기, 턴 기반이므로 허용) |
| 동시 열기 | I/O 버퍼 1개 → 동시 1파일만 (순차 open/read/close 패턴) |
| ProDOS 파일명 | A-Z, 0-9, `.` 만 허용, 최대 15자 |
| cc65 C 라이브러리 미사용 | 직접 MLI 호출로 대체, C open()/read()/close() 사용 불가 |

### B.10 타일셋 교체

룸별로 다른 타일셋 파일을 지정 가능:

```
TILES0: 동굴 (현재 기본 — 벽돌 벽, 돌 바닥)
TILES1: 성채 (향후 — 석조 벽, 타일 바닥)
TILES2: 숲   (향후 — 나무 벽, 풀 바닥)
```

룸 전환 시 `tileset_id`가 변경되면 TILES 파일 로드 → TILE_BUFFER 갱신.
동일 타일셋이면 `g_loaded_tileset` 캐시로 스킵.

### B.11 구현 단계 (전체 완료)

| Step | 내용 | 상태 |
|------|------|------|
| 1 | engine.h RoomDef 변경 (grid_rle/size→tileset_id) | 완료 |
| 2 | room_data.c RLE 배열 삭제 + tileset_id 추가 | 완료 |
| 3 | prodos_prefix.s 구현 (직접 MLI 호출) | 완료 |
| 4 | logic.c 파일 로딩 전환 | 완료 |
| 5 | render.c 타일셋 외부 로딩 전환 | 완료 |
| 6 | render.h 선언 추가 | 완료 |
| 7 | main.c 초기화/전환 수정 | 완료 |
| 8 | tools/gen_tileset.py 신규 작성 | 완료 |
| 9 | tools/json_to_room_data.py --binary-dir 추가 | 완료 |
| 10 | compile.sh 빌드/디스크 수정 | 완료 |

### B.12 결론

**맵 데이터 + 타일 데이터 외부화 완료.**

- 직접 ProDOS MLI ASM으로 안정적인 파일 로딩 구현
- cc65 C 라이브러리 `<fcntl.h>` 미사용 (비동작 문제 회피)
- 바이너리 크기 977 bytes 절약 (33,088B → 32,111B)
- 룸 수 3개 → **최대 ~130개 룸**으로 확장 가능 (140KB 디스크 기준)
- 룸별 타일셋 교체 인프라 구축 (현재 TILES0 1종, 향후 확장 가능)
- 맵 크기 자체의 확대는 GRID_BUFFER 크기 제한으로 현재 구조에서 불가
