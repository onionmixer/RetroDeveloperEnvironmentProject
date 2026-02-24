# PLAN_protorype_01_MSX_MSXDOS

## 1) 검토 범위 및 결론 요약

본 문서는 아래 3개 소스군을 상세 검토한 뒤, `prototype/01`의 핵심 플레이 루프를 MSX-DOS 대상(`./Examples/prototype_01_MSX_MSXDOS/`)으로 이식하기 위한 실행 계획을 정리한다.

- 원본 구현 검토: `/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01`
- MSX 라이브러리 검토: `./Library/MSX/ubox-msx-lib-1.2.0`
- 참조 계획 문서: `./Examples/prototype_01_AppleII_prodos/PLAN_protorype_01_AppleII_prodos.md`

핵심 결론:
- 이번 버전 1차 목표는 **100x100 원본 룸 데이터 기반 + 12x7 가시 맵**을 MSX-DOS에서 안정 동작시키는 것.
- `prototype/01` 원본의 상호작용 계약(이동/충돌, Door/Stair yes/no, Box 2단계 메시지)을 유지한다.
- 런타임 JSON 파싱은 MSX-DOS 메모리/코드 복잡도 리스크가 커서, **호스트 전처리(JSON -> C 정적 데이터)** 방식을 기본으로 채택한다.
- `Library/MSX`의 `ubox`는 강력하지만 Screen 2(타일/스프라이트) + ROM 성격이 강하므로, 본 과제 1차는 **MSX-DOS 텍스트 중심(z88dk + conio)**으로 진행하고, 2차에서 `ubox` 기반 그래픽 모드 확장을 검토한다.

---

## 2) prototype/01 상세 리뷰

### 2.1 아키텍처 요약

원본은 C99 모듈 분리형 구조이며 `main.c`에서 다음 순서로 동작한다.

1. Room JSON 3개 로딩 (`room_load`)
2. Room 간 Door/Stair 자동 연결 (`room_resolve_connections`)
3. 터미널 raw mode 초기화 (`terminal_enable_raw_mode`)
4. Player/Viewport/Help 초기화
5. 게임 루프
- 렌더 (`viewport_render`)
- 입력 (`input_read`)
- 이동 처리 (`player_try_move`)
- Door/Stair/Box 프롬프트 분기
- 몬스터 업데이트/충돌

### 2.2 상수/구조체 계약

`engine.h` 핵심 상수:
- `ROOM_WIDTH=100`, `ROOM_HEIGHT=100`
- `VIEWPORT_W=30`, `VIEWPORT_H=20` (MSX 이식 시 12x7으로 변경)
- `NUM_ROOMS=3`
- `MAX_DOORS=4`, `MAX_STAIRS=2`, `MAX_BOXES=10`, `MAX_MONSTERS=10`

핵심 계약:
- `MoveResult`: `MOVE_OK / MOVE_BLOCKED / MOVE_BLOCKED_BOX / MOVE_DOOR_PENDING / MOVE_STAIR_PENDING`
- `Room`: grid + doors/stairs/boxes/monsters + player_start
- `Player`: `(x, y, current_room)`
- `Viewport`: `(cam_x, cam_y)`

### 2.3 렌더링/입력/모달 계약

렌더링(`viewport.c`):
- `VIEWPORT_W x VIEWPORT_H` 루프
- 표시 우선순위: Player(`!`) > Monster(`$`) > Grid tile
- 하단 3줄: 좌표/도움말 힌트/상태 또는 프롬프트

입력(`input.c`):
- 이동: `W/A/S/D`
- 도움말: `H`
- 종료 모달: `Q` -> `1(quit)/0(return)`
- 일반 yes/no: `1/2`

### 2.4 이동/상호작용 핵심

`player_try_move()` 반환 기반 상태 기계:
- 벽(`#`) 또는 범위 밖 -> `MOVE_BLOCKED`
- 상자(`%`) -> `MOVE_BLOCKED_BOX`
- 문(`@`) -> `MOVE_DOOR_PENDING`
- 계단(`<`,`>`) -> `MOVE_STAIR_PENDING`
- 바닥(`.`) -> 좌표 갱신 + `MOVE_OK`

Door/Stair 전환:
- `target_room` + `target_door/target_stair` 링크로 이동
- Door 전환 시 목적지 문 안쪽 floor를 탐색하는 안전 로직 포함

Box 상호작용:
- 이동은 막고 프롬프트로 진입
- yes 선택 시 `effect` 표시 -> 키 입력 대기 -> `items` 목록 표시

### 2.5 Monster/Help 관련 판단

Monster(`monster.c`):
- Patrol/Chase/Returning 상태 머신
- LOS(Bresenham), Manhattan range, 2턴당 이동

Help(`help.c`):
- 폭 30 기준 wrap + scroll

이번 버전 판단:
- 12x7 소형 뷰 + MSX-DOS 메모리 제약을 고려하여 1차에서 Monster/Help 전체 이식은 제외하고, 핵심 플레이 루프 완성 후 2차 확장 대상으로 둔다.

---

## 3) `Library/MSX` 상세 리뷰 (이번 버전 기준)

### 3.1 라이브러리 구성

`Library/MSX/ubox-msx-lib-1.2.0`은 다음 3개 중심 컴포넌트로 구성된다.
- `ubox`: BIOS 래퍼 (화면/키/VRAM/tile/sprite)
- `spman`: 스프라이트/패턴 관리자
- `mplayer`: Arkos 기반 음악/효과

지원 성격:
- Screen 2(타일 그래픽) + 카트리지 ROM 워크플로우에 최적화
- 빌드 체인은 SDCC 중심

### 3.2 이번 과제에 유효한 부분

직접 활용 가능한 지식/자산:
- 입력 추상화 방식 (`ubox_read_keys`, 컨트롤 비트)
- 타일/맵 처리 패턴 (`game/src/game.c`의 `is_map_blocked`, map data 구조)
- 데이터 전처리 도구 (`tools/map.py`, `png2tiles.py`)

즉시 채택 보류:
- Screen 2 타일 렌더 전환
- `spman`, `mplayer` 의존
- SDCC 전체 툴체인 고정

보류 이유:
- 현재 목표는 MSX-DOS 텍스트 기반 프로토타입 구현
- 저장소 기존 MSX-DOS 튜토리얼은 `z88dk(+msxdos2)` 경로가 이미 정비됨

### 3.3 저장소 내 MSX-DOS 실행 패턴

확인한 참조 스크립트:
- 루트: `run_openmsx_msxdos2.sh`, `run_openmsx_msxdos2_diskaddtest.sh`
- 예제: `Examples/Tutorial_msx_z88dk_01/compile.sh`

핵심 패턴:
1. 프로그램(`.COM`) 빌드
2. boot disk 또는 작업용 disk에 `rdedisktool add`
3. openMSX 실행 (`-machine Panasonic_FS-A1GT -diska <disk>`)

본 계획/스크립트도 동일 패턴을 따른다.

---

## 4) 목표 구현 정의 (prototype_01_MSX_MSXDOS)

### 4.1 필수 범위 (MVP)

반드시 포함:
- Room 3개(원본 기반) 데이터 사용
- 플레이어 이동 + 벽 충돌
- 카메라 추적 뷰포트
- **가시 맵 12x7 렌더링**
- 좌표/Room/Z 상태 라인
- Door/Stair 진입 yes/no
- Box 진입 시 effect -> items 2단계 출력
- MSX-DOS에서 실행 가능한 `.COM` 산출

### 4.2 제외 범위 (이번 버전)

- 몬스터 AI 상태머신 전체
- Help 스크롤 시스템
- 런타임 JSON 문자열 파싱
- 사운드/스프라이트/그래픽 모드(Screen 2)

---

## 5) 12x7 변경 요구 반영 상세

원본 대비 반드시 바꿀 상수/정책:
- `VIEWPORT_W: 30 -> 10`
- `VIEWPORT_H: 20 -> 10`

영향 포인트:
- 카메라 중앙 계산: `player - VIEW/2`
- 경계 clamp: `0..(ROOM_W-VIEW_W)`, `0..(ROOM_H-VIEW_H)`
- 하단 상태줄 길이 제한(40열 텍스트 기준)
- 프롬프트 문구 축약 필요
- 테스트 기대값(카메라/렌더 범위) 갱신

권장 화면 레이아웃(텍스트 40열):
- Row 0~6: 12x7 맵
- Row 11: `R:001 X:nn Y:nn Z:n`
- Row 12: 상태/프롬프트(최대 38자)
- Row 13: 키가이드 `WASD 1/2 Q`

---

## 6) 구현 기술 선택

### 6.1 1차 선택 (권장)

- 컴파일러: `z88dk (zcc)`
- 타겟: `+msx -subtype=msxdos2`
- UI: `conio` 텍스트 기반

선택 이유:
- 저장소 내 기존 예제(`Tutorial_msx_z88dk_01`)와 일치
- `.COM` 산출/디스크 삽입/실행 루틴이 이미 검증됨
- 12x7 텍스트 프로토타입 목표와 정확히 부합

### 6.2 2차 확장 선택

- `Library/MSX/ubox` 도입
- Screen 2 타일 렌더 + 스프라이트로 전환
- map 전처리 도구(`map.py`) 통합

---

## 7) 프로젝트 구조 계획

대상 경로:
- `./Examples/prototype_01_MSX_MSXDOS/`

생성/유지 파일 계획:
- `PLAN_protorype_01_MSX_MSXDOS.md` (본 문서)
- `README.txt`
- `compile.sh`
- `run_openmsx_prototype_01_msx_msxdos_diskaddtest.sh`
- `src/engine.h`
- `src/main.c`
- `src/render.c`, `src/render.h`
- `src/logic.c`, `src/logic.h`
- `src/room_data.c`, `src/room_data.h` (전처리 결과)
- `tools/json_to_room_data_msx.py`
- `tests/host/test_camera.c`
- `tests/host/test_room_links.py`

---

## 8) 데이터 변환 전략 (JSON -> C 정적 배열)

입력:
- `/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/data/room_001.json`
- `/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/data/room_002.json`
- `/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/data/room_003.json`

출력:
- `src/room_data.h`
- `src/room_data.c`

전처리 단계 검증 규칙:
1. 모든 룸이 `100x100`인지 확인
2. Door/Box 2타일 연속성 확인
3. Stair 타입/좌표 정합성 확인
4. `player_start`가 floor(`.`)인지 확인
5. 룸 링크가 양방향으로 성립하는지 확인

런타임 정책:
- 타깃 코드에서는 파일 I/O + JSON 파서 제거
- 정적 배열 참조만 수행

---

## 9) 모듈 인터페이스 고정안

### 9.1 engine.h

상수/enum/구조체 통합:
- `ROOM_W=100`, `ROOM_H=100`
- `VIEW_W=12`, `VIEW_H=7`
- `ROOM_COUNT=3`
- `MoveResult` 및 Door/Stair/Box/Player/Viewport 구조

### 9.2 logic.c

역할:
- 이동 가능 여부 판정
- Door/Stair/Box 타일 분기
- room 전환 연산

원칙:
- 출력 함수 직접 호출 금지
- 상태 변경만 수행

### 9.3 render.c

역할:
- 12x7 맵 출력
- 상태줄/프롬프트 출력

원칙:
- 로직 상태 읽기 전용

### 9.4 main.c

역할:
- 메인 루프
- 입력 액션 분기
- 모달 yes/no 흐름 제어

---

## 10) 단계별 실행 계획

## Phase 0: 골격/빌드 파이프

1. 디렉토리 구조 생성
2. `compile.sh` 골격 작성 (`clean/build/disk/test/run/all`)
3. z88dk 타깃 빌드 확인

산출:
- 빈 루프라도 `.COM` 생성 + 디스크 삽입 가능

## Phase 1: 12x7 렌더/카메라

1. 카메라 clamp 함수
2. 12x7 출력 루프
3. 상태줄 표시

검증:
- `(0,0)`, `(50,50)`, `(99,99)` 주변에서 화면 범위 이상 없음

## Phase 2: 이동/충돌

1. WASD 입력
2. 벽/범위 차단
3. 바닥 이동

검증:
- 벽 충돌 시 좌표 불변
- 유효 이동 시 좌표 갱신

## Phase 3: Door/Stair/Box 상호작용

1. 타일 판별(`@`, `<`, `>`, `%`)
2. yes/no 모달 루프
3. 전환/취소 분기
4. Box 2단계 메시지

검증:
- yes/no 동작 모두 확인
- 취소 시 위치 불변
- 잘못된 링크에서 안전 메시지

## Phase 4: 통합/문서/스크립트

1. `README.txt` 정리
2. 실행 스크립트 정리
3. 테스트 매트릭스 체크

---

## 11) 실행 스크립트 계획 (bootdisk copy + add + emulator)

필수 동작:
1. 부팅 디스크 원본(`msxdos23.dsk`) 확인
2. `/tmp` 작업 디렉토리에 디스크 복사
3. 빌드 산출물(`HELLO.COM` 또는 지정 파일) 추가
4. `run_openmsx_msxdos2.sh` 호출로 openMSX 실행

환경변수 인터페이스(권장):
- `RDEDISKTOOL`
- `BOOT_DISK_SRC`
- `PROGRAM_FILE`
- `PROGRAM_NAME` (기본 `HELLO.COM`)
- `WORK_DIR`
- `AUTO_BUILD` (기본 0, 필요 시 1로 빌드 자동화)

오류 정책:
- 필수 파일/실행파일 누락 시 즉시 종료
- 디스크 add 실패 시 종료

---

## 12) 테스트 계획

### 12.1 호스트 검증

1. 전처리 스크립트 `--validate`
2. 카메라 계산 단위 테스트
3. 링크 정합 테스트

### 12.2 타깃 스모크 (openMSX)

1. 부팅 후 프로그램 실행
2. 12x7 렌더 정상
3. 이동/벽 충돌
4. Door yes/no
5. Stair yes/no
6. Box effect/item 표시

### 12.3 회귀 체크

1. `compile.sh all` 성공
2. `.COM` 파일 생성 및 크기 > 0
3. 디스크 이미지 `rdedisktool list/validate` 통과

---

## 13) 리스크 및 대응

리스크 1: 텍스트 I/O 성능/깜빡임
- 대응: 전체 clear 최소화, 행 단위 갱신 우선

리스크 2: 데이터 용량 과다
- 대응: 문자열 테이블화, 필요 시 룸 데이터 압축(2차)

리스크 3: 도구 의존성(z88dk/rdedisktool/openMSX)
- 대응: `compile.sh`와 실행 스크립트에 경로 자동 탐색 + 명확한 오류 메시지

리스크 4: 12x7 변경 누락
- 대응: 상수/렌더/테스트/문서 항목 동시 체크리스트 운용

---

## 14) Definition of Done

아래 조건 충족 시 이번 버전 완료:
1. `prototype_01_MSX_MSXDOS`에서 `.COM` 빌드 성공
2. openMSX에서 실행되고 **12x7 가시 맵** 표시
3. 이동/충돌 + Door/Stair/Box 상호작용 정상
4. bootdisk copy + add + 실행 스크립트 동작
5. README/PLAN 내용이 실제 스크립트/코드와 일치

---

## 15) 즉시 착수 우선순위

1. `tools/json_to_room_data_msx.py` 작성 및 3개 룸 전처리
2. `src/`의 12x7 렌더 + 이동 루프 최소 구현
3. Door/Stair/Box 모달 처리 추가
4. `compile.sh` + 실행 스크립트로 디스크 삽입/실행 자동화

---

## 16) 참고 분석 메모 (요약)

- 원본 `prototype/01`은 런타임 JSON 파싱 + raw terminal 기반이며, MSX-DOS에서는 그대로 이식하기보다 데이터 전처리 방식이 현실적이다.
- `Library/MSX/ubox`는 매우 유용한 MSX 그래픽/입력 기반을 제공하지만 이번 버전 목표(텍스트 12x7 프로토타입)보다 범위가 크므로 즉시 의존하지 않는다.
- 저장소의 `run_openmsx_msxdos2_diskaddtest.sh` 패턴을 그대로 계승해 운영 복잡도를 낮춘다.

---

## 17) 개발 환경/빌드 전제 보강

필수 도구:
- `z88dk` (`zcc`)
- `rdedisktool`
- `openMSX`
- `bash`, `cp`, `stat`

권장 환경 변수:
- `PATH=/opt/z88dk/bin:$PATH`
- `ZCCCFG=/opt/z88dk/lib/config`

버전 정책:
- `compile.sh`에서 `zcc --version` 출력
- `rdedisktool --version` 또는 `rdedisktool help` 출력
- 툴 버전 변경 시 PLAN/README 동시 갱신

기본 빌드 커맨드:
1. `cd ./Examples/prototype_01_MSX_MSXDOS`
2. `./compile.sh all`
3. `./run_openmsx_prototype_01_msx_msxdos_diskaddtest.sh`

---

## 18) 데이터 변환 규격(JSON -> C) 보강

`tools/json_to_room_data_msx.py` 산출 계약:
- 입력: room_001~003 JSON
- 출력: `src/room_data.h`, `src/room_data.c`
- 실패 시 0 이외 종료코드 반환

C 구조 제안:
```c
#define ROOM_COUNT 3
#define ROOM_W 100
#define ROOM_H 100
#define VIEW_W 10
#define VIEW_H 10

typedef struct { unsigned char x, y, orientation, wall_side, target_room, target_index; } DoorMsx;
typedef struct { unsigned char x, y, type, target_room, target_index; } StairMsx;
typedef struct { unsigned char x, y, orientation, placed_by_id, item_begin, item_count, effect_id; } BoxMsx;
typedef struct { unsigned char x, y, z; } PlayerStartMsx;
```

검증 규칙:
1. `grid[100][100]` 길이/문자 제약 확인
2. Door와 Box는 2타일 연속성 확인
3. Stair의 `<`/`>` 타입 정합 확인
4. 링크 인덱스가 `0..ROOM_COUNT-1` 범위인지 확인
5. `player_start`가 floor인지 확인

---

## 19) 12x7 화면/입력 계약 보강

권장 텍스트 UI:
- Row 0~6: 월드 뷰(12x7)
- Row 10: 구분 라인(선택)
- Row 11: 좌표/Room/Z
- Row 12: 상태/프롬프트
- Row 13: 키 가이드

입력 키맵:
1. 이동: `W/A/S/D`
2. 확인: `1=yes`, `2=no`
3. 종료 모달: `Q`, `1=quit`, `0=return`
4. 확장 후보: `I/J/K/L`, ESC 종료

문구 길이 제한:
- 상태줄 최대 38자
- 프롬프트 최대 30자 권장
- 긴 문자열은 페이지 분할

---

## 20) 테스트 매트릭스 보강

호스트 자동 테스트:
1. JSON 전처리 `--validate`
2. 카메라 계산 단위 테스트
3. 룸 링크 상호 참조 테스트
4. 문자열 테이블 인덱스 범위 테스트

타깃 스모크 테스트:
1. 부팅 후 프로그램 실행
2. 12x7 출력 정상
3. 벽 충돌 시 좌표 불변
4. Door/Stair yes/no 분기
5. Box effect -> item 2단계 표시
6. 종료 모달 정상 동작

회귀 체크:
1. `compile.sh all`
2. `.COM` 용량 > 0
3. 디스크 이미지 `list/validate` 통과
4. run 스크립트 기본값 동작

---

## 21) 구현 중 의사결정 포인트(사전 확정 필요)

1. 몬스터 포함 시점
- 옵션 A: 1차 제외(권장)
- 옵션 B: 정적 표시만
- 옵션 C: 상태머신 부분 이식

2. 렌더 방식
- 옵션 A: conio redraw(권장)
- 옵션 B: 부분 갱신(dirty rows)
- 옵션 C: ubox Screen 2 전환

3. 문자열 저장 방식
- 옵션 A: 문자열 테이블 ID 치환(권장)
- 옵션 B: 원문 문자열 직접 내장

4. 실행 자동화
- 옵션 A: `AUTO_BUILD=0` 기본(현재 권장)
- 옵션 B: `AUTO_BUILD=1` 고정(팀 운영 정책에 따라 선택)

---

## 22) 메모리/성능 예산 가이드

목표:
- 1차는 안정 동작 우선
- 2차에서 성능 최적화

권장 규칙:
1. 대형 자동 변수 사용 금지
2. 문자열 버퍼는 정적/고정 길이 사용
3. full clear 최소화, 행 단위 출력 우선
4. 데이터는 전처리 단계에서 압축/정규화

성능 기준:
- 키 입력 후 화면 반영 지연 체감 최소화
- 프롬프트 전환 시 출력 깨짐 없음

---

## 23) 모듈 인터페이스 검수 체크 (보강)

중복 정의 방지를 위해 모듈 정의 자체는 **9절을 단일 원본(source of truth)** 으로 사용한다.

검수 체크:
1. 새 함수 추가 시 9절 인터페이스 표와 동시 갱신
2. 렌더 모듈에서 게임 상태 쓰기(write) 금지
3. 로직 모듈에서 화면 출력 함수 호출 금지
4. 입력 변환 레이어와 게임 로직 레이어 분리 유지
5. `room_data` 포맷 변경 시 전처리 스크립트와 테스트 동시 갱신

---

## 24) 오류 처리/운영 정책

치명 오류:
- 룸 데이터 불일치
- 링크 인덱스 오류
- 필수 파일 미존재

처리:
- 상태줄/stderr에 짧은 메시지 출력 후 종료

비치명 오류:
- 잘못된 키 입력
- 상호작용 대상 누락

처리:
- 경고 메시지 후 루프 지속

디버그 정책:
- `#define DEBUG` 시 좌표/카메라/room 인덱스 출력
- 릴리즈에서는 디버그 메시지 비활성

---

## 25) 마일스톤/검수 체크

M1 (데이터/렌더):
- 전처리 성공
- 12x7 렌더 표시

M2 (이동/충돌):
- WASD 이동/벽 충돌 완료
- 좌표 일관성 확인

M3 (상호작용):
- Door/Stair/Box yes/no 흐름 완료
- 취소 분기 확인

M4 (패키징/실행):
- `compile.sh all` 성공
- bootdisk copy + add + openMSX 실행 성공

검수 항목:
1. 12x7 요구가 코드/문서/테스트에 일치
2. 런타임 JSON 파서 코드 없음
3. 실패 메시지 비어있지 않음

---

## 26) 타깃/호환성 매트릭스

1차 공식 타깃:
- openMSX + `Panasonic_FS-A1GT`
- MSX-DOS2 boot disk (`msxdos23.dsk`)

확인 대상:
1. 다른 openMSX machine profile
2. GT BIOS 경로가 다른 환경
3. 실기 전송(선택)

호환성 정책:
1. 1차는 openMSX 실행 우선 보장
2. 실기 호환은 2차 검증
3. ROM 모드는 범위 외

---

## 27) 저장 데이터/상수 명세 확정안

타일 인코딩:
- `.` floor
- `#` wall
- `@` door
- `<` stair down
- `>` stair up
- `%` box
- `!` player(렌더 전용)

상수:
- `ROOM_W=100`, `ROOM_H=100`
- `VIEW_W=12`, `VIEW_H=7`
- `ROOM_COUNT=3`

좌표 규약:
- `x: 0~99`, `y: 0~99`
- 접근: `grid[y][x]`

---

## 28) 입력/루프 타이밍 정책

루프:
1. 기본 blocking 입력
2. 입력 없을 때 busy loop 금지
3. 모달 상태에서 입력 우선 처리

우선순위:
1. 종료 모달
2. Door/Stair/Box 모달
3. 일반 이동

목표:
- 입력 후 화면 반영이 지연되지 않도록 유지

---

## 29) 릴리즈/운영 체크리스트

릴리즈 전:
1. `./compile.sh all` 성공
2. `.COM` 생성/용량 기록
3. 디스크 삽입/실행 스크립트 재검증
4. README/PLAN 동기화 확인

릴리즈 산출물:
1. `HELLO.COM` (또는 프로젝트 산출명)
2. `README.txt`
3. `compile.sh`
4. 실행 스크립트

운영 규칙:
1. 키맵 변경 시 문서 동시 수정
2. 디스크 삽입 옵션 변경 시 run 스크립트 동시 수정

---

## 30) 범위 제외 항목 명시

1차 범위 제외:
1. 몬스터 상태머신 완전 이식
2. Help 스크롤 시스템 완전 이식
3. ubox Screen 2 그래픽 전환
4. 사운드(mplayer) 통합
5. ROM 타깃 동시 지원

원칙:
- 제외 항목은 추후 2차 백로그로만 이동
- 1차 범위에 임의 편입 금지

---

## 31) 구현 작업 분할(WBS) 및 예상 산출

WBS-1: 데이터 파이프
- `json_to_room_data_msx.py`
- `room_data.[ch]`

WBS-2: 코어 루프
- `engine.h`, `main.c`, `logic.c`, `render.c`

WBS-3: 빌드/실행 자동화
- `compile.sh`
- `run_openmsx_prototype_01_msx_msxdos_diskaddtest.sh`

WBS-4: 테스트/문서
- `tests/host/*`
- `README.txt`, `PLAN`

---

## 32) 트러블슈팅 가이드

증상: `zcc not found`
- 조치: PATH/ZCCCFG 확인

증상: `rdedisktool not executable`
- 조치: build 경로 확인 또는 환경변수 지정

증상: openMSX 실행 실패
- 조치: `run_openmsx_msxdos2.sh`와 BIOS 경로 확인

증상: 실행 후 프로그램 미표시
- 조치: 디스크 내 파일명(`PROGRAM_NAME`)과 확장자 확인

---

## 33) 변경관리/추적 정책 + 코드 리뷰 체크포인트

변경 원칙:
1. 스크립트 인터페이스 변경 시 PLAN 즉시 반영
2. 상수(`VIEW_W/H`) 변경 시 테스트 기대값 동시 갱신
3. 범위 변경은 `범위 제외` 섹션 이력화

리뷰 체크포인트:
1. 문서와 구현 기본값 불일치 여부
2. 타깃 실행 경로 누락 여부
3. 테스트 항목 누락 여부
4. 에러 메시지가 사용자 조치 가능한 형태인지 여부

코드 규약:
1. 데이터 상수는 `engine.h`에 집중 선언
2. 매직 넘버 대신 상수/enum 사용
3. 문자열 길이 상한을 명시하고 버퍼 오버런 방지
4. 입력/로직/렌더 모듈 간 양방향 의존 금지
5. 12x7 요구 변경 시 코드+테스트+문서 3종 동시 수정

---

## 34) 인수 테스트 시나리오(최종)

시나리오 A: 기본 기동
1. 스크립트 실행
2. 부팅 후 프로그램 실행
3. 12x7 표시 확인

시나리오 B: 이동/충돌
1. 벽 방향 이동 시 정지 확인
2. 바닥 이동 시 좌표 갱신 확인

시나리오 C: 상호작용
1. Door yes/no 확인
2. Stair yes/no 확인
3. Box effect/item 순차 표시 확인

시나리오 D: 종료
1. 종료 모달 진입
2. return/quit 분기 확인

---

## 35) 일정/진척 기준선

Day 1:
- 데이터 전처리 스크립트/검증

Day 2:
- 12x7 렌더 + 카메라

Day 3:
- 이동/충돌 + 상호작용

Day 4:
- 빌드/디스크/실행 자동화

Day 5:
- 테스트/문서/인수 점검

---

## 36) 개발환경 셀프체크 스크립트 요구사항

체크 항목:
1. `command -v zcc`
2. `command -v rdedisktool` 또는 로컬 빌드 경로
3. `command -v openmsx` 또는 로컬 빌드 경로
4. boot disk 파일 존재

출력 정책:
- OK/FAIL 요약
- 실패 시 수정 가이드 1줄 제공

---

## 37) 백아웃/복구 계획

복구 기준:
1. 빌드 실패가 2회 이상 연속 발생
2. 디스크 삽입 후 부팅 실패 재현
3. 핵심 상호작용 회귀 발생

복구 절차:
1. 마지막 안정 태그/커밋 기준으로 원인 격리
2. 데이터 전처리 산출물 재생성
3. run 스크립트 경로/변수 초기화
4. 스모크 테스트 최소 세트 재실행

---

## 38) rdedisktool + openMSX 실행 계획 (보강)

실행 시퀀스:
1. boot disk 원본 확인
2. 작업 디스크로 복사
3. 기존 동일 파일 삭제(있으면)
4. 새 `.COM` 추가
5. `BOOT_DISK=<작업디스크>`로 openMSX 실행

검증 포인트:
1. 복사된 디스크 경로 로그 출력
2. add 성공 코드 확인
3. openMSX 호출 인자 확인

운영 메모:
- 현재 `compile.sh`는 구현되어 있으며, run 스크립트 기본값은 `AUTO_BUILD=0`이다.
- 반복 실행 시 빌드 생략을 원하면 기본값 0을 유지하고, 항상 최신 바이너리를 넣고 싶으면 `AUTO_BUILD=1`로 실행한다.

실행 예시:
```bash
cd ./Examples/prototype_01_MSX_MSXDOS

# 1) 수동 빌드 후 실행
./compile.sh build
PROGRAM_FILE=./HELLO.COM \
./run_openmsx_prototype_01_msx_msxdos_diskaddtest.sh

# 2) 실행 시 자동 빌드
AUTO_BUILD=1 \
PROGRAM_FILE=./build/HELLO.COM \
./run_openmsx_prototype_01_msx_msxdos_diskaddtest.sh
```

정합성 운영 규칙:
1. `README.txt`와 본 문서의 실행 예시는 동일하게 유지
2. run 스크립트 기본값 변경 시 11절/38절 동시 수정
3. viewport 상수 변경 시 테스트/문서 동시 수정

---

## 부록 A) 참고 경로

- 원본 프로토타입: `/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01`
- MSX 라이브러리: `./Library/MSX/ubox-msx-lib-1.2.0`
- 참조 문서: `./Examples/prototype_01_AppleII_prodos/PLAN_protorype_01_AppleII_prodos.md`
- 루트 실행 스크립트: `./run_openmsx_msxdos2.sh`
- 본 문서 대상 디렉토리: `./Examples/prototype_01_MSX_MSXDOS/`
