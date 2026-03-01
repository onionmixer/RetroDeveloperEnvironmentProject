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
| 디컴프레스 | $0900-$1C87 버퍼 (5,000B), `g_loaded_room` 캐시로 중복 방지 |
| 스프라이트 | 소프트웨어 방식 (이전 위치 맵 타일 복원 → 새 위치 타일 그리기) |
| 텍스트 출력 | 커스텀 비트맵 폰트 (91엔트리, 5×7 글리프 in 7×8 셀) |
| 몬스터 AI | prototype_01 전체 유지 (PATROL/CHASE/RETURNING + Bresenham LOS) |
| 룸 상태 | `RoomRuntime g_runtime[ROOM_COUNT]` — 전 룸 보존 |

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

16종 타일 (128B) + 91엔트리 폰트 (728B), 모두 7×8 bit 7=0.

- 맵: FLOOR, WALL, DOOR_H, DOOR_V, STAIR_DN, STAIR_UP, BOX, EMPTY (8종)
- 테두리: TL, TR, BL, BR, H, V (6종, bit 2 수직/bit 2-6 수평 통일)
- 오브젝트: PLAYER, MONSTER (2종)
- 폰트: ASCII 32-122, 대문자 패턴을 소문자에 복제
- 문(TILE_DOOR): 인접 셀 DOOR 확인으로 H/V 자동 분기

구현 파일: `src/tiles.h` (맵+테두리+오브젝트), `src/font.h` (폰트), `src/render.c` (포인터 테이블)

---

## 5. 모듈 아키텍처 (구현 완료 — 요약)

### 5.1 파일 구조

```
src/
├── main.c          — 진입점 + 게임 루프
├── render.h/c      — HGR 렌더링 (행 테이블, draw_tile, 텍스트, 프롬프트)
├── logic.h/c       — RLE 디컴프레서, 이동, 카메라, 문/계단/상자
├── monster.h/c     — 몬스터 FSM (PATROL/CHASE/RETURNING, LOS, 충돌)
├── input.h/c       — 키보드 (cgetc 래퍼, INPUT_* 코드)
├── help.h/c        — 도움말 화면 (스크롤, HGR 텍스트)
├── engine.h        — 공유 타입 (GameState, MoveResult, TileCode, RoomDef 등)
├── room_data.h/c   — 룸 그리드 + 메타데이터 (json_to_room_data.py 생성)
├── tiles.h         — 타일 비트패턴 16종
├── font.h          — 폰트 비트패턴 91엔트리
└── apple2-hgr-ext.cfg — 커스텀 링커 (HGR 회피, HIMEM=$9600)
```

### 5.2 주요 API

| 모듈 | 핵심 함수 |
|------|----------|
| render | `render_init()`, `render_draw_map()`, `render_draw_monsters()`, `render_update_player()`, `render_print()`, `render_print_wrap()`, `render_prompt_yes_no()`, `render_redraw_all()`, `render_cleanup()` |
| logic | `logic_decompress_room()`, `logic_get_tile_code()`, `logic_try_move()`, `logic_update_camera()`, `logic_find_door/stair/box()`, `logic_do_door/stair()` |
| monster | `monster_init_all()`, `monster_update_all()`, `monster_check_collision()`, `monster_format_encounter_msg()`, `monster_index_at()` |
| input | `input_read_blocking()`, `input_wait_yes_no()`, `input_wait_any_key()` |
| help | `help_show()` |

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
- **실행**: `./compile.sh run` → sa2 --d1 boot.po --d2 game.po → `BRUN HELLO,D2`

---

## 8. 메모리 예산

```
$0803-$0833    48B      STARTUP
$0900-$1C87    5,000B   GRID_BUFFER (디컴프레스된 100×50 그리드)
$2000-$3FFF    8,192B   HGR Page 1 프레임버퍼
$4000-$72CC    ~13KB    CODE
$72CD-$8828    ~5.5KB   RODATA (RLE ~1.5KB + 타일 128B + 폰트 728B + HGR테이블 384B + 문자열)
$8829-$8941    ~360B    DATA+BSS
$8E00-$95FF    2,048B   C 스택
$9600-$BEFF    보호      BASIC.SYSTEM

바이너리: 33,088B, BRUN 한계까지 여유 ~3.2KB
RLE 압축 효과: 15,000B → 1,504B (90% 절감, ~13.5KB 절약)
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

---

## 11. 리스크 및 대응

| 리스크 | 대응 |
|--------|------|
| HGR 프레임버퍼-코드 충돌 | `apple2-hgr-ext.cfg` 커스텀 링커 사용 |
| 타일 색상 아티팩트 | bit 7=0 고정, 인접 비트 쌍으로 흰색 보장 |
| 메모리 부족 | RLE 압축으로 해결. 추가 완화: 도움말 축소, LOS 제거, 소문자 제거 |

---

## 12. 수정/생성 파일 목록

| 파일 | 상태 |
|------|------|
| `src/main.c`, `render.h/c`, `logic.h/c`, `monster.h/c`, `input.h/c`, `help.h/c` | 신규 (구현 완료) |
| `src/engine.h` | 수정 (MoveResult, MAP_ORIGIN_*, running 추가) |
| `src/tiles.h`, `src/font.h` | 신규 (구현 완료) |
| `src/room_data.h/c` | 생성 (json_to_room_data.py) |
| `src/apple2-hgr-ext.cfg` | 신규 (커스텀 링커) |
| `compile.sh`, `run_applewin_prodos.sh` | 신규 (구현 완료) |
| `tools/json_to_room_data.py` | 수정 (RLE 압축 추가) |

---

## 13. 참고 자료

- prototype_01 소스: `Examples/prototype_01_AppleII_prodos/src/`
- prototype_02 MSX 소스: `Examples/prototype_02_MSX_ROM_MSXDOS/src/`
- souless_apple2: `Library/AppleII/souless_apple2/common/source/` (video.s, videoOffset.s, sprite.s)
- ProDOS 부트 디스크: `diskwork/bootdisk/AppleII/ProDOS_2_4_3.po`
- rdedisktool: ProDOS 타입명 지원 (`--type SYS/BIN/TXT`, `0xFF`)

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
