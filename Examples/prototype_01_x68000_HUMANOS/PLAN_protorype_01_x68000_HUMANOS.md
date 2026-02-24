# PLAN_protorype_01_x68000_HUMANOS

## 0) 구현 현황 업데이트 (2026-02-24)

현재 저장소 구현 기준으로 아래 항목이 완료되었다.

- 20x20 렌더/카메라/이동/충돌 기본 루프
- Door/Stair yes/no 전환
- Box effect -> item 2단계 메시지
- 도움말(`H`) 파일 로드 + 스크롤(`HELP.TXT`, W/S/A/D, Q/H 종료)
- 몬스터 `$` 렌더 + 상태머신 patrol/chase/return(LOS/range) + `M` 토글
- 몬스터 충돌 시 되돌리기/전환 직후 안전 스폰
- 상태 메시지 2줄 큐(최근/이전) + 긴 문구 축약
- `compile.sh all` 경로에서 host/run68/disktest 자동 검증
- `rdedisktool` 최신 기준(X68000: `xdf/dim`, FS: `human68k`) 반영 완료
- FDD1 준비 흐름을 `빈 이미지 생성 -> Human68k 포맷 -> info/validate 검증 -> 파일 주입`으로 고정
- 인식 실패 시 boot disk 복제 fallback 제거, 실패 즉시 중단 정책 반영

미완료(계획상 2차):
- 사운드/그래픽 모드 확장

---

## 1) 검토 범위 및 결론 요약

본 문서는 아래 3개 범위를 상세 검토한 뒤, `prototype/01`의 핵심 플레이 루프를 X68000 Human68k 대상(`./Examples/prototype_01_x68000_HUMANOS/`)으로 이식하기 위한 실행 계획을 정리한다.

- 원본 구현 검토: `/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01`
- X68000 참고 라이브러리 검토: `./Library/x68000/X68KTutorials`
- 참조 계획 문서: `./Examples/prototype_01_AppleII_prodos/PLAN_protorype_01_AppleII_prodos.md`

핵심 결론:
- 이번 버전 1차 목표는 원본 `prototype/01`의 상호작용 계약을 유지하면서, X68000 Human68k에서 **20x20 가시 맵**으로 동작하는 텍스트 기반 플레이 루프를 구축하는 것이다.
- `Library/x68000`는 이번 버전에서 직접 링크/사용하지 않되, IOCS/DOS 호출 패턴, 파일 입출력 관례, 프로젝트 구조 참조용으로 활용한다.
- 런타임 JSON 파싱은 메모리/복잡도 리스크가 크므로, 기존 AppleII/MSX 계획과 동일하게 **호스트 전처리(JSON -> C 정적 데이터)**를 기본 경로로 채택한다.
- 실행 자동화는 `run_px68k_humanos.sh`를 기준으로, 별도 스크립트에서 **작업용 XDF 생성 -> Human68k 포맷/검증 -> 파일 주입 -> FDD1 장착 부팅** 흐름으로 구성한다.

---

## 2) prototype/01 상세 리뷰

### 2.1 아키텍처 요약

원본은 C99 모듈 분리형 구조이며 `src/main.c` 루프는 다음 순서로 구성된다.

1. `room_001~003.json` 로딩
2. Door/Stair 링크 해석 (`room_resolve_connections`)
3. 터미널 raw 모드 초기화
4. 플레이어/뷰포트/도움말 초기화
5. 반복 루프
- 뷰포트 렌더
- 입력 처리
- 이동/충돌 분기
- Door/Stair/Box 프롬프트
- 몬스터 이동/충돌

### 2.2 상수/구조체 계약

`src/engine.h` 기준 핵심 상수:
- `ROOM_WIDTH=100`, `ROOM_HEIGHT=100`
- `VIEWPORT_W=30`, `VIEWPORT_H=20`
- `NUM_ROOMS=3`
- `MAX_DOORS=4`, `MAX_STAIRS=2`, `MAX_BOXES=10`, `MAX_MONSTERS=10`

핵심 상태 계약:
- `MoveResult`: `MOVE_OK`, `MOVE_BLOCKED`, `MOVE_BLOCKED_BOX`, `MOVE_DOOR_PENDING`, `MOVE_STAIR_PENDING`
- 렌더 우선순위: Player(`!`) > Monster(`$`) > Room tile
- 타일 규약: `.` `#` `@` `<` `>` `%`

### 2.3 이동/전환/상호작용 핵심

`player_try_move()`는 타일 타입에 따라 상태를 반환하고, 실제 전환은 caller(`main.c`)에서 프롬프트를 거쳐 수행한다.

- Wall 또는 범위 밖: 차단
- Box: 이동 차단 + yes/no 프롬프트
- Door/Stair: pending 상태 반환 후 진입 확인
- Door 전환: target door 기준으로 실내 floor를 탐색하는 안전 로직 포함

### 2.4 이식 시 유지/변경 포인트

유지:
- 룸 3개 연결 구조
- Door/Stair yes/no 계약
- Box effect -> items 2단계 표시
- 좌표/층 표시

변경:
- 가시 맵 크기 `30x20 -> 20x20` (이번 계획의 필수 반영 사항)
- raw termios/ANSI 의존 제거, Human68k 콘솔 출력 방식으로 치환
- JSON 런타임 파싱 제거(전처리 데이터 로딩)

---

## 3) Library/x68000 상세 리뷰 (직접 의존 없음)

### 3.1 구성 요약

`./Library/x68000/X68KTutorials`는 다수의 독립 예제로 구성되며, 다음 성격이 확인된다.

- 텍스트/기초 출력: `HelloWorld`
- IOCS/인터럽트/CRT 제어: `Anatomy`, `Interrupts`
- 파일 입출력: `FileOperations`
- 그래픽/타일/스프라이트: `MarioKart`, `Tetris68`, `CastleLand`
- 에셋 파이프라인: `x68AssetsComp`

### 3.2 이번 과제에 유효한 참조점

- `HelloWorld/main.c`: `_dos_c_print`, `_dos_exit` 기반 최소 실행 패턴
- `Anatomy/main.c`: `_iocs_*`와 `_dos_*`의 공존 패턴, 초기화/종료 시퀀스
- `FileOperations/main.c`: Human68k 파일 생성/열기/읽기/삭제 호출 관례

### 3.3 이번 버전에서 직접 미사용하는 이유

- 예제 대부분이 개별 튜토리얼 목적이며 프로젝트 공용 정적 라이브러리 형태가 아님
- 과제 1차 목표는 텍스트 기반 프로토타입이며, 타일/스프라이트 파이프라인 도입은 범위 초과
- 저장소 내 기존 X68000 툴체인/실행 경로(`Examples/Tutorial_x68000_01`, `run_px68k_humanos.sh`)를 우선 재사용하는 편이 리스크가 낮음

---

## 4) 목표 구현 정의 (prototype_01_x68000_HUMANOS)

### 4.1 필수 범위 (MVP)

1차에서 반드시 포함:
- 룸 데이터 3개(원본 기반)
- 플레이어 이동 + 벽 충돌
- 카메라 추적
- **가시 맵 20x20 렌더링**
- 상태줄(현재 room/x/y/z)
- Door/Stair yes/no 전환
- Box effect/item 2단계 출력
- Human68k에서 실행 가능한 `.x` 산출물

### 4.2 이번 버전 제외 범위

- 사운드/스프라이트/그래픽 모드 전환
- 런타임 JSON 파싱

---

## 5) 20x20 변경 요구 반영 상세

원본 대비 필수 변경:
- `VIEWPORT_W: 30 -> 20`
- `VIEWPORT_H: 20 -> 20` (높이는 유지, 폭은 20으로 축소)

검토 포인트:
1. 카메라 중심 계산: `player - VIEW/2`
2. clamp 범위: `0..(ROOM_W-VIEW_W)`, `0..(ROOM_H-VIEW_H)`
3. 상태/프롬프트 라인 길이 제한(20열 맵 + 하단 메시지 구역)
4. 기존 테스트 기대값(카메라, 화면 경계) 갱신

권장 레이아웃:
- Row 0~19: 맵 20x20
- Row 21: `R:001 X:nn Y:nn Z:n`
- Row 22: 프롬프트/이벤트 메시지
- Row 23: 키 가이드

---

## 5.1) 20x20 변경 영향 상세 체크리스트

코드 상수:
1. `VIEW_W`, `VIEW_H` 단일 정의 사용 여부
2. 하드코딩된 `30`, `20` 잔존 여부 검색(`rg -n "30|VIEWPORT_W|VIEWPORT_H"`)

카메라/렌더:
1. `cam_x`, `cam_y` clamp 경계가 `ROOM - VIEW`를 참조하는지
2. 렌더 루프가 `row/col: 0..19`를 유지하는지
3. 맵 경계(0,0 / 99,99)에서 화면 깨짐이 없는지

프롬프트/UI:
1. 상태줄/프롬프트 라인이 맵 출력 라인과 충돌하지 않는지
2. 프롬프트 길이 초과 시 잘림/분할 정책이 있는지
3. yes/no 모달에서 일반 이동 입력이 차단되는지

테스트/문서 동기화:
1. 카메라 테스트 기대값이 20x20 기준인지
2. README 조작키/화면 설명이 20x20으로 갱신되었는지
3. PLAN/코드/테스트의 viewport 값이 일치하는지

---

## 6) 구현 기술 선택

### 6.1 빌드/실행 경로

- 컴파일러: `m68k-xelf-gcc` (저장소 `Toolchain/x68000/toolchain/m68k-xelf`)
- 실행(빠른 검증): `run68`
- GUI 검증: `px68k-onionmixer` + Human68k (`run_px68k_humanos.sh`)

### 6.2 데이터 전략

기본 전략:
- `prototype/01/data/*.json`을 호스트 도구로 `src/room_data.c/h`로 변환
- 타깃 바이너리는 정적 배열만 참조

이점:
- 메모리/코드량 절감
- 파서 버그 리스크 감소
- 플랫폼별 동작 일관성 증가

---

## 7) 프로젝트 구조 계획

대상 경로:
- `./Examples/prototype_01_x68000_HUMANOS/`

계획 파일:
- `PLAN_protorype_01_x68000_HUMANOS.md` (본 문서)
- `run_px68k_humanos_fdd1_diskadd.sh` (FDD1 자동 장착 실행 스크립트)

향후 구현 파일(계획):
- `README.txt`
- `compile.sh`
- `src/main.c`
- `src/engine.h`
- `src/game.c`, `src/game.h`
- `src/render.c`, `src/render.h`
- `src/logic.c`, `src/logic.h`
- `src/room_data.c`, `src/room_data.h`
- `tools/json_to_room_data_x68000.py`
- `tests/host/test_camera.c`
- `tests/host/test_room_links.py`

---

## 8) 실행 스크립트 설계 (요구사항 6 반영)

요구 흐름:
1. 비어있는 X68000 이미지 생성
2. 이미지를 Human68k로 포맷/초기화
3. 이미지에 프로그램 파일 주입
4. 해당 이미지를 FDD1로 장착
5. `run_px68k_humanos.sh` 경로를 이용해 px68k 실행

스크립트 정책:
- 기본 실행 스크립트: `run_px68k_humanos_fdd1_diskadd.sh`
- `rdedisktool create <disk.xdf> -f xdf --force`로 빈 이미지 생성
- `rdedisktool create <disk.xdf> -f xdf --fs human68k --force`로 포맷(초기화)
- `rdedisktool info/validate`로 Human68k 인식 및 무결성 검증(실패 시 즉시 중단)
- 파일 삽입 후 `BOOT_DISK=... FDD1_DISK=... ./run_px68k_humanos.sh`로 실행

검증 단계:
1. `bash -n` 문법 점검
2. 생성 이미지 `rdedisktool info`에서 `File System: Human68k` 확인
3. 생성 이미지 `rdedisktool validate` 통과 확인
4. 대상 파일이 이미지에 삽입되었는지 확인
5. px68k 부팅 시 FDD0/FDD1 경로 출력 확인

---

## 9) 단계별 실행 계획

## Phase 0: 베이스라인 정리

1. 디렉터리/스크립트/문서 생성
2. 빌드 도구 탐색 함수 통일
3. 경로 탐색 우선순위 확정

산출:
- 실행 가능한 FDD1 장착 스크립트
- 초기 계획 문서

## Phase 1: 데이터 파이프라인

1. `json_to_room_data_x68000.py` 작성
2. 입력 JSON 검증(100x100, 링크 정합성)
3. `room_data.c/h` 생성 자동화

산출:
- 런타임 JSON 파서 없는 정적 데이터

## Phase 2: 20x20 렌더/카메라

1. 20x20 렌더 루프 구현
2. 카메라 중심+clamp 구현
3. 상태줄/프롬프트 출력

검증:
- `(0,0)`, `(50,50)`, `(99,99)` 경계 테스트 통과

## Phase 3: 이동/상호작용

1. WASD 입력
2. 벽/범위 차단
3. Door/Stair yes/no
4. Box 2단계 메시지

검증:
- 원본과 동일한 상태 전이 계약 유지

## Phase 4: 빌드/실행 자동화

1. `compile.sh` 작성( clean/build/test/run/all )
2. run68 스모크 테스트
3. FDD1 이미지 자동 주입 + px68k 실행 테스트

---

## 9.1) 즉시 착수 작업 우선순위

1. `tools/json_to_room_data_x68000.py` 작성 및 `room_data.c/h` 생성
2. `engine.h`/`render.c`에 20x20 카메라+렌더 골격 반영
3. `logic.c`에 이동/충돌/Door/Stair/Box 분기 구현
4. `compile.sh` + `run_px68k_humanos_fdd1_diskadd.sh` 통합 스모크 실행
5. 테스트/README/PLAN 동기화 점검

---

## 10) 테스트 매트릭스 (개요)

호스트 테스트:
1. JSON 변환기 검증 성공
2. 카메라 좌표 테스트 성공
3. Door/Stair 링크 양방향 검증 성공

타깃(run68) 테스트:
1. 프로그램 정상 시작
2. 좌표/룸 상태 출력 정상
3. 이동/충돌 기본 동작 정상

타깃(px68k) 테스트:
1. Human68k 부팅 성공
2. FDD1 삽입 이미지 인식
3. 주입된 실행 파일 실행 확인
4. 20x20 뷰 렌더 확인

---

## 11) 위험요소 및 대응

1. `rdedisktool` 구버전/구빌드 사용 시 X68000 create/fs 미지원 가능성
- 대응: `rdedisktool help create`에서 `X68000: xdf, dim` / `human68k` 노출 여부를 사전 확인하고, 미지원 버전이면 툴 업데이트 후 진행

2. X68000 콘솔 I/O 구현 세부 차이
- 대응: 1차는 최소 텍스트 출력 루틴부터 고정 후 단계 확장

3. 100x100 정적 데이터로 인한 바이너리 크기 증가
- 대응: 문자열 테이블 중복 제거, 필요 시 2차 압축 도입

---

## 12) 완료 조건 (Definition of Done)

아래를 모두 만족하면 이번 계획 범위를 완료로 본다.

1. `PLAN_protorype_01_x68000_HUMANOS.md`가 현재 저장소 구조/도구 기준으로 현실적인 단계 계획을 제공한다.
2. `run_px68k_humanos_fdd1_diskadd.sh`가
- 작업용 XDF 빈 이미지 생성 + Human68k 포맷/검증,
- 파일 주입,
- FDD1 장착,
- px68k 실행을 자동화한다.
3. 문서에서 **20x20 가시 맵 요구**가 상수/테스트/작업 단계에 일관되게 반영된다.
4. `Library/x68000`는 직접 의존 없이 참조용으로만 사용한다는 정책이 명확히 명시된다.

---

## 13) 개발 환경/빌드 전제 보강

필수 도구:
- `m68k-xelf-gcc` (`Toolchain/x68000/toolchain/m68k-xelf/bin/m68k-xelf-gcc`)
- `run68` (`Toolchain/x68000/toolchain/m68k-xelf/bin/run68`)
- `bash`, `rg`
- 디스크 삽입용 `rdedisktool`
- GUI 실행용 `run_px68k_humanos.sh`

필수 런타임 자산:
- `diskwork/bootdisk/x68000/HUMAN302.XDF`
- `Emulator/x68000/px68k-onionmixer/iplrom.dat`
- `Emulator/x68000/px68k-onionmixer/cgrom.dat`
- `Emulator/x68000/px68k-onionmixer/px68k-onionmixer`

권장 버전 정책:
- `m68k-xelf-gcc --version`을 `compile.sh` 시작 시 출력
- `run68 --help` 또는 `run68` 실행 가능 여부 체크
- `rdedisktool --version` 출력 기록

기본 빌드 커맨드(계획):
1. `cd ./Examples/prototype_01_x68000_HUMANOS`
2. `./compile.sh all`
3. 산출물 `prototype_01.x` 확인

---

## 14) 데이터 변환 규격(JSON -> C) 보강

`tools/json_to_room_data_x68000.py` 산출 계약:
- 입력:
  - `/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/data/room_001.json`
  - `/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/data/room_002.json`
  - `/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/data/room_003.json`
- 출력:
  - `src/room_data.h`
  - `src/room_data.c`
- 실패 조건:
  - Room 크기 불일치
  - 타일 규약 위반
  - Door/Stair 링크 불일치
  - `player_start`가 floor가 아님

C 구조 제안(고정 길이):
```c
#define ROOM_COUNT 3
#define ROOM_W 100
#define ROOM_H 100
#define VIEW_W 20
#define VIEW_H 20

typedef struct { unsigned char x, y, wall_side, target_room, target_index; } DoorX68;
typedef struct { unsigned char x, y, type, target_room, target_index; } StairX68;
typedef struct { unsigned char x, y, placed_by_id, effect_id, item_begin, item_count; } BoxX68;
```

검증 규칙:
1. `grid`는 100행, 각 행 100문자 강제
2. Door 좌표 타일이 `@`인지 검증
3. Stair 좌표 타일이 `<` 또는 `>`인지 검증
4. Box 좌표 타일이 `%`인지 검증
5. 모든 target 링크가 유효 인덱스로 환산되는지 검증

---

## 15) 20x20 화면/입력 계약 보강

권장 화면 레이아웃(텍스트 중심):
- Row 0~19: 가시 맵 20x20
- Row 21: `R:<id> X:<xx> Y:<yy> Z:<z>`
- Row 22: 이벤트/프롬프트(최대 60자)
- Row 23: 키 가이드(`WASD 1/2 Q`)

프롬프트 규칙:
- yes/no 프롬프트는 48자 이내
- 긴 텍스트는 2단계 표시(예: effect 후 item)
- 상태줄과 프롬프트 줄은 항상 clear 후 출력

입력 우선순위:
1. 종료 모달(`Q`)
2. 상호작용 모달(door/stair/box)
3. 일반 이동(`WASD`)

키맵 기본안:
- 이동: `W A S D`
- 확인: `1` yes, `2` no
- 종료모달: `1` quit, `0` return
- 도움말: `H` (구현 완료)
- 몬스터 이동 토글: `M` (구현 완료)

---

## 16) 테스트 매트릭스 보강

역할 구분:
- `10)`은 테스트 범주/축 정의(개요)
- `16)`은 각 축의 합격 기준/회귀 규칙(상세)

호스트 자동 테스트:
1. `json_to_room_data_x68000.py --validate` 성공
2. 카메라 계산 `(0,0)`, `(50,50)`, `(99,99)` 기대값 일치
3. Door/Stair 링크 양방향 정합성 확인

타깃(run68) 스모크 테스트:
1. 프로그램 기동 및 즉시 크래시 없음
2. 상태줄 출력 확인
3. 이동/벽 충돌 동작 확인
4. Door/Stair/Box 분기 메시지 확인

타깃(px68k) 테스트:
1. Human68k 부팅 성공
2. FDD1 작업 디스크 삽입 확인
3. 주입된 실행 파일 실행 확인
4. 20x20 화면 렌더 확인

회귀 체크(매 빌드):
1. `compile.sh all` 성공
2. `run_px68k_humanos_fdd1_diskadd.sh` 문법 체크 성공
3. README의 실행 절차와 스크립트 동작 일치

---

## 17) 구현 중 의사결정 포인트(사전 확정 필요)

1. 몬스터 포함 시점
- 현재: 상태머신(CHASE/RETURN/LOS) + 충돌 시 RETURNING 전환 구현
- 잔여 옵션: 고급 추격 튜닝(우선순위/가중치) 여부

2. 데이터 저장 방식
- 옵션 A: 정적 C 배열(권장)
- 옵션 B: 런타임 파일 로딩

3. 콘솔 구현 방식
- 옵션 A: `_dos_c_print` 기반 단순 출력(권장)
- 옵션 B: IOCS 커서 제어 적극 사용

4. 실행 확인 우선순위
- 옵션 A: run68 우선 후 px68k 확인(권장)
- 옵션 B: px68k만 사용

---

## 18) 메모리/성능 예산 가이드 (보강)

목표:
- 1차는 안정 동작 우선
- 화면 전체 redraw 허용

권장 예산:
- 정적 데이터 + 코드 + 스택 사용량을 빌드 로그로 추적
- 문자열 중복 제거(`effect`, `item`, 안내문)
- 대형 임시 버퍼 최소화

실행 규칙:
1. 긴 동적 문자열 조합 최소화
2. 자동 변수 대형 배열 지양
3. 입력 루프에서 busy loop 금지

---

## 19) 모듈 인터페이스 고정안 (보강)

초기 인터페이스:
- `engine.h`: 상수/enum/구조체
- `game.c`: 게임 루프, 입력 분기, 상태 전환
- `render.c`: 20x20 렌더, 상태/프롬프트 출력
- `logic.c`: 이동/충돌/전환 판정
- `room_data.c`: 정적 룸 데이터

원칙:
1. 렌더 모듈은 상태를 읽기만 한다.
2. 로직 모듈은 출력 함수를 호출하지 않는다.
3. 입력 모듈은 액션 변환에 집중한다.

---

## 20) 오류 처리/운영 정책 (보강)

치명 오류:
- 룸 데이터 불일치
- target 링크 손상
- 산출물/자산 파일 누락

처리:
- 짧은 오류 메시지 출력 후 종료
- 종료 코드 비정상 반환

비치명 오류:
- 예상 외 키 입력
- 상호작용 대상 미발견

처리:
- 상태줄 경고 후 루프 지속

---

## 21) 마일스톤/검수 체크 (보강)

M1 (데이터/렌더):
- JSON 전처리 성공
- 20x20 렌더 + 상태줄 출력

M2 (플레이어 루프):
- 이동/충돌/카메라 추적
- 입력 계약 고정

M3 (상호작용):
- Door/Stair/Box yes/no 루프 구현
- 문구 길이 제한 준수

M4 (배포형 실행):
- `compile.sh all` 성공
- run68 스모크 통과
- px68k FDD1 실행 스크립트 통합 확인

---

## 22) 타깃/호환성 매트릭스 (보강)

1차 공식 타깃:
- X68000 Human68k (px68k-onionmixer)

보조 타깃:
- run68 (빠른 기능 검증)

호환성 정책:
1. 1차는 Human68k + px68k 동작 보장
2. 실기(X68000 실제 HW) 검증은 2차 범위
3. 고급 IOCS/그래픽 모드 의존 기능은 2차로 이관

---

## 23) 저장 데이터/상수 명세 확정안 (보강)

타일 인코딩:
- `.` floor
- `#` wall
- `@` door
- `<` stair down
- `>` stair up
- `%` box
- `!` player(런타임 표시)
- `$` monster(런타임 표시, 상태머신 이동 구현)

상수 확정:
- `ROOM_W=100`, `ROOM_H=100`
- `VIEW_W=20`, `VIEW_H=20`
- `ROOM_COUNT=3`

좌표 규약:
- `x`: 0~99
- `y`: 0~99
- 접근: `grid[y][x]`

---

## 24) 입력/루프 타이밍 정책 (보강)

루프 정책:
1. 기본은 blocking 입력
2. 입력 대기 중 busy loop 금지
3. 모달 진입 시 일반 이동 차단

응답성 목표:
- 키 입력 후 화면 갱신까지 체감 지연 150ms 이내

반복 입력 정책:
- OS/에뮬레이터 autorepeat 의존
- 1차는 게임 내 repeat 가속 로직 미도입

---

## 25) 릴리즈/운영 체크리스트 (보강)

릴리즈 전 필수:
1. `./compile.sh all` 성공
2. 산출물 `.x` 파일 크기/생성 시간 기록
3. FDD1 disk-add 스크립트 실행 경로 재검증
4. README/PLAN 실행 절차 일치 확인
5. DoD 항목 전부 체크

릴리즈 산출물:
- `prototype_01.x`
- `README.txt`
- `compile.sh`
- `run_px68k_humanos_fdd1_diskadd.sh`

---

## 26) 범위 제외 항목 명시 (보강)

1차 범위 제외:
1. 사운드/스프라이트/그래픽 모드 기능
2. 런타임 JSON 파싱
3. 실기 전용 최적화

원칙:
- 20x20 핵심 루프 안정성을 우선
- 제외 항목은 2차 계획으로 이관

---

## 27) 구현 작업 분할(WBS) 및 예상 산출 (보강)

WBS-1 데이터:
1. 변환기 뼈대 작성
2. 입력 검증 구현
3. `room_data.c/h` 생성
4. 변환 결과 검증

WBS-2 코어:
1. 상수/구조체 확정
2. 20x20 렌더 구현
3. 입력 액션 변환
4. 이동/충돌 로직 구현

WBS-3 상호작용:
1. Door 전환
2. Stair 전환
3. Box effect/item 2단계
4. yes/no 공통 모달

WBS-4 빌드/실행:
1. `compile.sh` 완성
2. run68 테스트 고정
3. FDD1 disk-add 자동 실행
4. 문서/체크리스트 정리

---

## 28) 트러블슈팅 가이드 (보강)

증상: px68k 부팅 실패
- 점검 1: `iplrom.dat`, `cgrom.dat` 존재 여부
- 점검 2: `HUMAN302.XDF` 경로 유효성
- 점검 3: `run_px68k_humanos.sh` 실행권한

증상: FDD1 파일 삽입 실패
- 점검 1: `rdedisktool info`에서 `File System: Human68k` 확인
- 점검 2: strict add 실패 시 off 모드 재시도 로그 확인
- 점검 3: target 파일명 8.3 규약 점검

증상: 화면 깨짐/좌표 이상
- 점검 1: 20x20 루프 범위(0~19)
- 점검 2: `grid[y][x]` 축 일치
- 점검 3: 상태줄/프롬프트 row 충돌 여부

---

## 29) 코드 규약 및 리뷰 체크포인트 (보강)

코드 규약:
1. 상수는 `UPPER_SNAKE_CASE`
2. 좌표는 `x/y`, 카메라는 `cam_x/cam_y`
3. 모달 상태 변수 이름 통일
4. 메시지 문자열 중복 제거

리뷰 체크포인트:
1. 20x20 상수 단일 정의 참조 여부
2. 배열 접근 전 범위 체크 여부
3. 모달 상태에서 이동 입력 차단 여부
4. README/PLAN/실행 스크립트의 키맵 일치 여부

---

## 30) 변경관리/추적 정책 (보강)

변경관리 원칙:
1. 기능 변경 시 PLAN 섹션 동시 갱신
2. 파일 포맷/키맵 변경 시 호환성 영향 명시
3. 범위 추가 시 제외 항목에서 이력 이동

작업 추적 권장:
- 커밋 메시지에 WBS 태그 사용
- 버그 수정 시 재현/해결 절차 기록

---

## 31) 인수 테스트 시나리오(최종) (보강)

시나리오 A: 기본 이동/충돌
1. 시작 후 20x20 맵 표시
2. 빈 타일 4방향 이동
3. 벽 충돌 시 좌표 불변

시나리오 B: Door 전환
1. Door 인접 이동
2. `1` 입력 시 room 전환
3. `2` 입력 시 취소 확인

시나리오 C: Stair 전환
1. Stair 인접 이동
2. yes/no 분기 각각 확인
3. 전환 좌표 유효성 확인

시나리오 D: Box 상호작용
1. Box 접근 시 이동 차단
2. yes 시 effect -> item 2단계 노출
3. no 시 상태 유지 확인

시나리오 E: Help/Monster 보조 기능
1. `H` 입력 시 HELP.TXT 스크롤 표시 후 복귀(파일 없으면 내장 도움말)
2. `M` 입력 시 몬스터 이동 on/off 토글 반영
3. 몬스터 충돌 시 되돌리기 메시지/좌표 복원 확인

합격 기준:
- A~E 모두 통과
- 크래시/무한루프/빈 메시지 없음

---

## 32) 일정/진척 기준선 (보강)

권장 일정(예: 5일):
1. Day 1: 전처리/검증 파이프라인
2. Day 2: 20x20 렌더/입력
3. Day 3: 이동/충돌/카메라
4. Day 4: Door/Stair/Box 통합
5. Day 5: run68+px68k 검증, 문서 고정

지연 기준:
- 각 Day 핵심 산출물 미완료 시 다음 단계 착수 보류
- 우선순위는 `20x20 안정 동작 > 기능 확장`

---

## 33) 개발환경 셀프체크 스크립트 요구사항 (보강)

`tools/check_env.sh` 권장 점검:
1. `command -v m68k-xelf-gcc`
2. `command -v run68`
3. `command -v rdedisktool`
4. `test -x ./run_px68k_humanos.sh`
5. ROM/디스크 필수 파일 존재 확인

실행 정책:
1. `compile.sh` 시작 시 `tools/check_env.sh`를 우선 호출
2. 필수 항목 실패 시 즉시 종료
3. 선택 항목(예: GUI 자산)은 warning 후 진행 여부 안내

---

## 34) 백아웃/복구 계획 (보강)

백아웃 정책:
1. WBS 단위 커밋 유지
2. 마지막 인수 테스트 통과 커밋 기록
3. 회귀 시 직전 안정 커밋 기준으로 단계적 재적용

복구 체크:
1. `compile.sh all` 재성공
2. 인수 테스트 A~D 재통과
3. 실패 원인/대응을 PLAN 또는 작업 로그에 기록

---

## 부록 A) X68000 실행 체인 운영 요약

표준 실행 순서:
1. `./compile.sh build`
2. `./run_px68k_humanos_fdd1_diskadd.sh`
3. Human68k 프롬프트에서 프로그램 실행

스크립트 정책:
- FDD0: `HUMAN302.XDF` (부팅)
- FDD1: 작업용 이미지(빈 XDF 생성 + Human68k 포맷/검증)
- 파일 삽입: `rdedisktool add`

실패 대응:
1. `rdedisktool` 미탐지: `RDEDISKTOOL=/abs/path/rdedisktool` 지정
2. ROM 누락: `IPL_ROM`, `CG_ROM` 환경변수 지정
3. 부팅디스크 경로 문제: `BOOT_DISK=/abs/path/HUMAN302.XDF` 지정

---

## 부록 B) 참고 경로

- 원본 엔진: `/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/src`
- 원본 데이터: `/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/data`
- X68000 라이브러리: `./Library/x68000/X68KTutorials`
- 공용 실행 스크립트: `./run_px68k_humanos.sh`
- 본 문서/스크립트 경로: `./Examples/prototype_01_x68000_HUMANOS`
