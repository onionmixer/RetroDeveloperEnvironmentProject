# PLAN_protorype_01_AppleII_prodos

## 1) 검토 범위 및 결론 요약

본 문서는 아래 2개 소스군을 상세 검토한 뒤, `prototype/01`의 핵심 플레이 루프를 Apple II ProDOS 대상(`./Examples/prototype_01_AppleII_prodos/`)으로 이식하기 위한 실행 계획을 정리한다.

- 원본 구현 검토: `/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01`
- Apple II 참고 자산 검토: `./Library/AppleII/apple2flat`, `./Library/AppleII/souless_apple2`

핵심 결론:
- 1차 목표는 **30x20 터미널 뷰를 Apple II 텍스트 기반 10x10 가시 맵**으로 재구성하는 것.
- 입력/렌더/데이터 로딩을 Apple II 제약에 맞춰 단순화하되, 원본의 게임성 핵심(이동, 벽 충돌, Door/Stair 전환, Box 상호작용, 좌표표시)은 유지.
- 구현 경로는 `cc65 -t apple2` + `conio` 기반을 기본으로 하고, 필요 시 `apple2flat` API를 보조적으로 도입.
- `souless_apple2`는 대규모 DOS3.3/65C02 어셈블리 게임이므로 아키텍처 참조용으로만 사용.

---

## 2) prototype/01 상세 리뷰

### 2.1 아키텍처 개요

원본은 C99 모듈 분리형 엔진이며, `main.c`에서 아래 순환으로 동작한다.

1. Room JSON 3개 로드 (`room_load`)
2. Room 간 Door/Stair 자동 연결 해석 (`room_resolve_connections`)
3. raw terminal 초기화 (`termios`, ANSI escape)
4. 플레이어/뷰포트 초기화
5. 게임 루프
- viewport 업데이트/렌더
- 키 입력
- 이동 처리 + 결과 분기
- 문/계단/상자 프롬프트 처리
- 몬스터 이동/충돌 처리

### 2.2 상수/데이터 구조

`engine.h` 핵심 상수:
- Room: `100x100`
- Viewport: `30x20`
- Room 수: `3`
- Door/Stair/Box/Monster 최대치 고정 배열

핵심 구조체:
- `Room`: grid + doors/stairs/boxes/monsters + player_start
- `Player`: 좌표 + 현재 room index
- `Viewport`: cam_x, cam_y

### 2.3 렌더링/입력

렌더링 (`viewport.c`):
- 매 프레임 `VIEWPORT_H x VIEWPORT_W` 영역을 직접 구성
- 우선순위: Player(`!`) > Monster(`$`) > grid tile
- 하단에 좌표라인, help 힌트, info/prompt 라인 출력

입력 (`input.c`):
- `WASD` 이동, `Q` 종료모달, `H` 도움말, `Space` 도움말 복귀
- 확인 프롬프트: `1=yes`, `0=no`
- 종료 프롬프트: `1=yes`, `0=no`

### 2.4 이동/상호작용 계약

`player_try_move` 반환 계약:
- `MOVE_OK`: 일반 이동
- `MOVE_BLOCKED`: 벽/범위 차단
- `MOVE_BLOCKED_BOX`: 상자 앞 (프롬프트 필요)
- `MOVE_DOOR_PENDING`: 문 진입 의사 확인 필요
- `MOVE_STAIR_PENDING`: 계단 진입 의사 확인 필요

문/계단 전환:
- Door는 wall_side 반대편 Door와 연결되어 상대 Room으로 이동
- Stair는 down/up 쌍으로 연결

Box:
- `%` 2칸 박스 접근 시 이동 차단 후 프롬프트
- yes 시 effect 문구 -> item 목록 순서로 표시

Monster:
- PATROL/CHASE/RETURNING 상태 머신
- line-of-sight(Bresenham), manhattan range 기반 추격
- 플레이어 충돌 시 메시지 표출 + 몬스터 retreat

### 2.5 데이터 포맷

Room JSON은 고정 포맷:
- `grid[100]` 각 행 100자
- `doors/stairs/boxes/monsters/player_start`
- 파서는 외부 JSON 라이브러리 없이 문자열 검색 기반 lightweight 구현

주의점:
- 원본 로더는 파일 전체를 메모리로 읽고 수동 파싱하므로, Apple II에서는 동일 전략을 그대로 가져가기 어렵다.

### 2.6 테스트 관점

`test_verify.c`는 다음을 검증:
- Room 로딩/경계/오브젝트 배치/연결 정합성
- 뷰포트 카메라 계산

이식 시 이 검증 축을 Apple II용 단순 테스트(호스트 side + 타겟 side smoke)로 재편해야 한다.

---

## 3) Library/AppleII 상세 리뷰

## 3.1 `apple2flat` 분석

규모/성격:
- 파일 약 160개
- cc65 + 6502 asm 기반 프레임워크
- 목표: DOS/BASIC 의존 축소, 비디오/키보드/사운드/디스크 유틸 제공

본 과제 관련 유효 기능:
- 입력: `kb_new`, `kb_get`
- 텍스트 출력: `text_outs`, `text_printf`, `text_xy`, `text_window`, `video_cls`
- 화면 모드: `video_mode_text`, `video_page_select/flip`
- 디스크 읽기: `disk_read` (섹터 기반)

의미:
- 10x10 타일 렌더 + 상태라인 출력에는 충분한 API가 존재.
- 단, 현재 저장소의 튜토리얼(`Examples/Tutorial_apple_prodos_01`)은 `cl65 -t apple2` + `conio` 패턴이므로, 초기 구현은 conio 중심이 통합 리스크가 더 낮다.

## 3.2 `souless_apple2` 분석

규모/성격:
- 파일 약 3449개
- stage/scene 분할, 대량 아트/사운드 자산, 로더/모듈 분할, 65C02 중심
- README 기준 Apple IIe 128KB + DOS 3.3 중심

본 과제 적용성:
- 직접 재사용보다는 빌드/모듈 로딩 구조 참고용 가치가 큼
- 현재 목표(텍스트 기반 ProDOS 프로토타입) 대비 과대 구조
- 따라서 현재 단계에서는 의존 채택 비권장

### 3.3 저장소 내 ProDOS 튜토리얼 상태

`Examples/Tutorial_apple_prodos_01` 확인 결과:
- `hello.c` + `compile.sh` + `README.txt`
- 빌드: `cl65 -t apple2 -O -o HELLO hello.c`
- 후처리: `tail -c +59 HELLO > HELLO.tmp` (AppleSingle header 제거)
- 디스크 삽입 예: `rdedisktool add ... --type B --addr 0x0803`

즉, 본 과제 결과물도 이 흐름을 따르는 것이 저장소 일관성에 맞다.

---

## 4) 목표 구현 정의 (prototype_01_AppleII_prodos)

## 4.1 범위

필수 구현 (MVP):
- 100x100 맵 데이터 로드(또는 컴파일 시 포함)
- 플레이어 이동(WASD 또는 Apple II 친화 키)
- 벽 충돌
- 카메라 추적 뷰포트
- **가시 맵 10x10 렌더링**
- 좌표/현재 room 표시
- Door/Stair 전환 yes/no 프롬프트
- Box 접근 시 effect/item 텍스트 표시

선택 구현 (2차):
- (완료) 몬스터 AI
- (완료) Help 스크롤 뷰
- Quit 모달 정교화

## 4.2 10x10 변경 요구 반영

원본 대비 필수 변경점:
- `VIEWPORT_W: 30 -> 10`
- `VIEWPORT_H: 20 -> 10`
- 화면 하단 UI 라인 배치 재정의
- 도움말 wrap 폭 재설정(기존 30 기반 로직 분리)
- 카메라 중앙 계산 및 경계 클램핑 재검증
- 테스트 기대값(카메라/렌더 범위) 갱신

---

## 5) 구현 전략

## 5.1 기술 선택

1차 구현 선택:
- `cc65` + `conio.h` 기반
- 이유: 현재 Examples 흐름과 동일, 진입비용 최소, ProDOS 산출물 파이프라인 재사용 가능

보조 선택:
- 필요시 `apple2flat` 텍스트/키보드 루틴 일부 차용
- 조건: conio에서 성능/제약 이슈 발생 시

## 5.2 데이터 처리 전략

Apple II 제약 고려 시 JSON 런타임 파싱은 리스크가 큼. 따라서:
- 호스트 전처리 스크립트에서 JSON -> C 정적 배열(`const char room_grid[][101]`) 변환
- Door/Stair/Box/Monster 메타도 C 배열로 생성
- 타깃 바이너리에서는 파싱 없이 즉시 사용

장점:
- 메모리/코드량 절감
- 런타임 안정성 증가
- 디버깅 단순화

---

## 6) 단계별 실행 계획

## Phase 0: 프로젝트 골격 생성

대상 경로:
- `./Examples/prototype_01_AppleII_prodos/`

생성 파일(계획):
- `README.txt`
- `compile.sh`
- `src/main.c`
- `src/engine.h`
- `src/room_data.h` (전처리 산출물 include)
- `src/room_data.c`
- `src/game.c`
- `src/game.h`
- `tools/json_to_room_data.py`
- `tests/host/test_camera.c` (호스트 단위 테스트)
- `tests/host/test_room_data.py` (전처리 결과 검증)
- `data/` (원본 room json 복사 또는 링크 기준)

## Phase 1: 10x10 렌더 코어

구현 항목:
- viewport camera 계산 함수
- 10x10 타일 출력 함수
- 상태 라인 출력 함수 (room, x,y,z, prompt)

검증:
- 맵 경계(0,0 / 99,99)에서 깨짐 없는지
- 플레이어 중심 추적 + clamp 정상 여부

## Phase 2: 이동/충돌

구현 항목:
- 키 입력 루프
- 벽/범위 차단
- 일반 이동

검증:
- 연속 이동 시 좌표 일관성
- 비정상 키 무시

## Phase 3: Door/Stair/Box 상호작용

구현 항목:
- 목표 타일 판별
- yes/no 프롬프트 루틴
- room 전환 및 spawn 위치 계산
- box effect/item 2-step 표시

검증:
- 전환 실패시 안전 메시지
- 취소 입력 시 원위치 유지

## Phase 4: 빌드/패키징 자동화

`compile.sh` 계획:
1. `cl65 -t apple2 -O -o HELLO src/*.c`
2. `tail -c +59 HELLO > HELLO.tmp && mv HELLO.tmp HELLO`
3. 결과 용량 출력

디스크 탑재 가이드:
- ProDOS 이미지에 `HELLO`를 `--type B --addr 0x0803`로 삽입

## Phase 5: 테스트 정리

호스트 테스트:
- 전처리 스크립트가 100x100/오브젝트 수 제약 검증
- 카메라 계산 단위 테스트(가능하면 별도 C 호스트 빌드)

타깃 스모크:
- 부팅 후 렌더 정상
- 4방향 이동
- Door/Stair/Box 최소 1회씩 검증

---

## 7) 10x10 변경 영향 상세 체크리스트

코드 영향:
- `VIEWPORT_W/H` 상수
- 렌더 루프 범위
- 모달/프롬프트 텍스트 길이
- help wrap width
- 테스트 기대값

UX 영향:
- 한 화면 정보량 급감 -> 이동 빈도 증가
- 상태라인 가독성 중요도 상승
- 프롬프트 길이 제한 필요(짧은 문장 우선)

성능 영향:
- 프레임당 문자 출력량 감소(긍정)
- 카메라 업데이트는 동일 O(1)

---

## 8) 리스크 및 대응

리스크 1: JSON 런타임 파싱 과부하
- 대응: 전처리 기반 정적 데이터로 전환

리스크 2: 키 매핑 차이
- 대응: WASD + 보조키(화살표/ESC) 병행 처리

리스크 3: 메모리 한계
- 대응: 문자열 압축/상수 테이블화, 메시지 짧게 유지

리스크 4: ProDOS 실행 이미지 처리 실수
- 대응: compile 스크립트에 header strip 단계 고정

---

## 9) 완료 기준 (Definition of Done)

아래 조건 충족 시 1차 완료:
- `prototype_01_AppleII_prodos`에서 Apple II 대상 빌드 성공
- 실행 시 **10x10 맵 표시** 및 이동/충돌 동작
- Door/Stair/Box 상호작용 정상
- 산출물 `HELLO` 생성 + ProDOS 디스크 삽입 가이드 포함
- README에 조작키/제약/빌드 절차 문서화

---

## 10) 즉시 착수 작업 우선순위

1. `tools/json_to_room_data.py` 작성 (원본 room_001~003 변환)
2. `engine.h`/`game.c`에 10x10 카메라+렌더 골격 구현
3. 이동/충돌/프롬프트 루프 구현
4. `compile.sh`/`README.txt` 정리 후 타깃 스모크 테스트

---

## 11) 개발 환경/빌드 전제 보강

필수 도구:
- `cc65` (`cl65`, `ld65`) 설치
- `bash`, `tail`, `wc`
- 디스크 이미지 반영용 `rdedisktool` (실기/에뮬 테스트 시)

권장 버전 정책:
- `cc65` 버전은 문서에 명시하고 고정(예: `2.19` 또는 팀 표준 버전)
- 버전 차이로 바이너리 헤더/런타임 동작이 달라질 수 있으므로 `compile.sh`에 `cl65 --version` 출력 추가

기본 빌드 커맨드:
1. `cd ./Examples/prototype_01_AppleII_prodos`
2. `./compile.sh all`
3. 산출물 `HELLO` 확인

디스크 반영 예시(ProDOS):
1. `rdedisktool add <disk.po> ./HELLO HELLO --type B --addr 0x0803`
2. 에뮬레이터에서 `HELLO` 실행

---

## 12) 데이터 변환 규격(JSON -> C) 보강

`tools/json_to_room_data.py` 산출 계약:
- 입력: `room_001.json`, `room_002.json`, `room_003.json`
- 출력: `src/room_data.c`, `src/room_data.h`
- 실패 조건: 타일/크기/오브젝트 제약 위반 시 비정상 종료(0 이외 코드)

C 구조 제안(고정 길이):
```c
#define ROOM_COUNT 3
#define ROOM_W 100
#define ROOM_H 100
#define VIEW_W 10
#define VIEW_H 10

typedef struct { unsigned char x, y, orientation, wall_side, target_room, target_index; } DoorA2;
typedef struct { unsigned char x, y, type, target_room, target_index; } StairA2;
typedef struct { unsigned char x, y, orientation, placed_by_id, item_begin, item_count, effect_id; } BoxA2;
typedef struct { unsigned char x, y, z; } PlayerStartA2;
```

압축 규칙:
- 문자열은 가능한 한 ID 테이블(사전)로 치환 (`placed_by`, item name, effect)
- 맵은 `char[ROOM_H][ROOM_W+1]` 유지(디버깅 우선), 필요 시 2차에서 RLE 도입

검증 규칙(전처리 단계):
- Room 크기 100x100 강제
- Door/Box 2타일 연속성 검증
- Stair 타입(`<`/`>`) 정합 검증
- `player_start`가 Floor(`.`)인지 검증

---

## 13) 10x10 화면/입력 계약 보강

권장 텍스트 화면 레이아웃(40열 기준):
- Row 0~9: 맵 10x10
- Row 11: `R:<id> X:<xx> Y:<yy> Z:<z>`
- Row 12: 짧은 상태 메시지(최대 38자)
- Row 13: 키 가이드(`WASD H Q`)

프롬프트 문구 길이 제한:
- yes/no 프롬프트는 28자 이내 목표
- 긴 문구는 2페이지 표시 또는 축약 텍스트 사용

입력 우선순위:
1. 시스템 모달(quit/help)
2. 상호작용 모달(door/stair/box)
3. 일반 이동 입력

키맵 기본안:
- 이동: `W A S D` + 선택적으로 `I J K L`
- 확인: `1`(yes), `2`(no)
- 종료모달: `1`(quit), `0`(return)
- 도움말 토글: `H`

---

## 14) 테스트 매트릭스 보강

호스트 자동 테스트:
1. `json_to_room_data.py --validate` 성공
2. 카메라 계산: `(0,0)`, `(50,50)`, `(99,99)` 기준 기대값 일치
3. Door/Stair 링크가 양방향으로 성립

타깃 스모크 테스트:
1. 기동 후 맵 10x10이 깨지지 않고 표시됨
2. 벽 충돌 시 좌표 불변
3. Door yes/no 분기 정상
4. Stair yes/no 분기 정상
5. Box effect -> item 2단계 표시 정상

회귀 체크(매 빌드):
1. `HELLO` 파일 생성
2. 헤더 스트립 후 파일 크기 0보다 큼
3. README의 실행 방법이 현재 스크립트와 일치

---

## 15) 구현 중 의사결정 포인트(사전 확정 필요)

1. 몬스터 포함 시점
- 옵션 A: MVP 제외(권장)
- 옵션 B: 정적 몬스터만 표시
- 옵션 C: 원본 상태머신 일부 이식

2. 데이터 저장 방식
- 옵션 A: 정적 C 배열(권장)
- 옵션 B: 런타임 디스크 로딩

3. 렌더 방식
- 옵션 A: conio 단순 redraw(권장)
- 옵션 B: apple2flat page flip 적용

4. 키 표준
- 옵션 A: WASD 고정(권장)
- 옵션 B: WASD + IJKL 병행

---

## 16) 메모리/성능 예산 가이드 (보강)

목표:
- 1차는 안정 동작 우선, 2차에서 최적화
- 프레임당 전체 redraw를 허용하되 입력 지연이 체감되지 않아야 함

권장 예산:
- 정적 데이터 + 코드 + 스택 합산을 빌드 로그에서 추적
- 문자열 테이블은 중복 제거(아이템/effect/메시지)
- 런타임 버퍼는 고정 길이로 제한

실행 규칙:
1. 긴 임시 문자열 조합 금지 (`snprintf` 최소화)
2. 대형 자동 변수(스택) 금지, 전역/정적 버퍼 사용
3. 렌더링은 dirty-rect 없이 시작하되, 필요 시 행 단위 갱신으로 확장

---

## 17) 모듈 인터페이스 고정안 (보강)

초기 고정 인터페이스(권장):
- `engine.h`: 상수, 공용 enum/struct
- `game.c`: 루프, 입력 분기, 상태 전환
- `render.c`: 10x10 맵 + 상태줄 + 프롬프트 출력
- `logic.c`: 이동/충돌/door/stair/box 판정
- `room_data.c`: 전처리 산출 정적 데이터

인터페이스 원칙:
1. 렌더 모듈은 게임 상태를 읽기만 함(쓰기 금지)
2. 로직 모듈은 출력 함수를 직접 호출하지 않음
3. 입력 모듈은 키코드 -> 액션 변환만 담당

---

## 18) 오류 처리/운영 정책 (보강)

치명 오류:
- room 데이터 불일치, 링크 불일치, 인덱스 범위 오류
- 처리: 사용자에게 짧은 오류 메시지 출력 후 종료

비치명 오류:
- 상호작용 타겟 없음, 예상치 못한 키 입력
- 처리: 상태줄 경고 후 루프 지속

로그/디버그:
- `#define DEBUG` 시 상태줄에 `room/x/y/cam` 추가 표시
- 릴리즈 빌드에서는 DEBUG 문자열 제거

---

## 19) 마일스톤/검수 체크 (보강)

M1 (데이터/렌더):
- JSON 전처리 성공
- 10x10 맵 + 좌표/room 상태줄 출력

M2 (플레이어 루프):
- 이동/충돌/카메라 추적 완료
- 입력 키맵 고정

M3 (상호작용):
- Door/Stair/Box yes/no 루프 완료
- 프롬프트 길이 제한 준수

M4 (배포형 빌드):
- `compile.sh all` 1회 성공
- ProDOS 이미지 삽입 후 실행 확인
- README/PLAN 교차검토 완료

검수 항목:
1. 10x10 요구가 코드 상수/테스트/문서 모두에서 일치
2. 런타임 JSON 파싱 코드가 남아있지 않음
3. 실패 시 사용자 메시지가 비어있지 않음

---

## 20) 타깃/호환성 매트릭스 (보강)

1차 공식 타깃:
- Apple IIe + ProDOS (에뮬레이터 기준)

확인 대상(가능 시):
- AppleWin (Windows)
- linapple 또는 동급 에뮬레이터 (Linux)
- 실기 Apple IIe (선택)

호환성 정책:
1. 1차 릴리즈는 ProDOS 실행만 보장
2. DOS 3.3 부팅/실행은 범위 외
3. 65C02 전용 최적화는 2차로 이관

---

## 21) 저장 데이터/상수 명세 확정안 (보강)

타일 인코딩(권장):
- `.` floor, `#` wall, `@` door, `<` stair down, `>` stair up, `%` box
- 런타임 렌더 전용: `!` player, `$` monster

상수 확정:
- `ROOM_W=100`, `ROOM_H=100`
- `VIEW_W=10`, `VIEW_H=10`
- `ROOM_COUNT=3`

좌표 규약:
- `x`: 0~99 (좌->우)
- `y`: 0~99 (상->하)
- 맵 접근: `grid[y][x]`

전처리 생성물 규칙:
1. 모든 배열은 범위 상수 기반으로 선언
2. Room 인덱스는 `room_001 -> 0`, `room_002 -> 1`, `room_003 -> 2` 고정
3. 문자열 테이블 인덱스는 0을 `EMPTY` 예약값으로 사용

---

## 22) 입력/루프 타이밍 정책 (보강)

루프 정책:
1. 기본은 blocking 입력(`cgetc` 등) 기반
2. 입력 없는 상태에서 busy loop 금지
3. 메시지 표시 후 확인 입력 대기 시 타이머 갱신 없음

입력 디바운스:
- 같은 키 반복 입력은 OS/에뮬레이터 autorepeat에 의존
- 게임 코드에서 추가 repeat 가속 로직은 1차 범위 제외

응답성 기준(목표):
- 키 입력 후 다음 화면 갱신까지 체감 지연이 150ms 이내

---

## 23) 릴리즈/운영 체크리스트 (보강)

릴리즈 전 필수:
1. `./compile.sh all` 성공
2. `HELLO` 생성 및 파일 크기 기록
3. ProDOS 이미지 삽입 커맨드 재검증
4. README 키맵/빌드/실행 절차 최신화
5. PLAN 문서의 DoD 항목 체크 완료

릴리즈 산출물:
- `HELLO`
- `README.txt`
- `compile.sh`
- (선택) 샘플 `.po` 이미지 및 실행 스크린샷

운영 규칙:
- 버그 수정 시 `회귀 체크(매 빌드)` 3개 항목을 반드시 재실행
- 키맵 변경 시 README와 PLAN의 키맵 섹션을 동시에 갱신

---

## 24) 범위 제외 항목 명시 (보강)

1차 범위 제외:
1. 원본 `monster.c` 수준의 상태머신 완전 이식
2. 원본 도움말 스크롤 시스템 전체 이식
3. 런타임 JSON 파싱
4. 다중 디스크/리소스 스트리밍
5. 사운드 시스템 연동

범위 제외 원칙:
- MVP 안정성 및 10x10 핵심 플레이 검증을 우선
- 제외 항목은 2차 계획에서 별도 문서로 관리

---

## 25) 구현 작업 분할(WBS) 및 예상 산출 (보강)

WBS-1 데이터 파이프라인:
1. `json_to_room_data.py` 뼈대 작성
2. 입력 검증(크기/타일/링크) 구현
3. `room_data.c/.h` 생성기 구현
4. 샘플 데이터로 생성/검증 실행

WBS-2 런타임 코어:
1. `engine.h` 상수/구조체 확정
2. 10x10 렌더 루틴 구현
3. 입력 -> 액션 변환 구현
4. 이동/충돌 로직 구현

WBS-3 상호작용:
1. Door 전환
2. Stair 전환
3. Box effect/item 2단계 표시
4. 공통 yes/no 모달 루틴

WBS-4 빌드/문서:
1. `compile.sh` 완성
2. README 실행 가이드 정리
3. 회귀 체크리스트 실행
4. 릴리즈 산출물 정리

---

## 26) 트러블슈팅 가이드 (보강)

증상: `HELLO` 실행 시 즉시 종료/오동작
- 점검 1: header strip(`tail -c +59`) 누락 여부
- 점검 2: 삽입 주소 `--addr 0x0803` 사용 여부
- 점검 3: ProDOS 타입 `--type B` 지정 여부

증상: 화면 깨짐/좌표 출력 이상
- 점검 1: 10x10 렌더 범위(0~9) 인덱스 오프셋 확인
- 점검 2: `grid[y][x]` 축 사용 일치 여부
- 점검 3: 상태줄 row 상수 충돌 여부

증상: 문/계단 전환 실패
- 점검 1: 전처리 단계에서 target 링크 생성 여부
- 점검 2: target room/index 범위 검증 로그 확인
- 점검 3: 전환 후 spawn 좌표가 floor인지 확인

---

## 27) 코드 규약 및 리뷰 체크포인트 (보강)

코드 규약:
1. 상수는 `UPPER_SNAKE_CASE`
2. 좌표 변수는 `x/y`, 카메라는 `cam_x/cam_y` 고정
3. 함수 길이는 80줄 내외 권장(초과 시 분리)
4. 메시지 문자열은 중앙 테이블에 모아 중복 방지

리뷰 체크포인트:
1. 10x10 상수가 하드코딩 중복 없이 단일 정의를 참조하는지
2. 범위 체크 없이 배열 접근하는 코드가 없는지
3. 모달 상태에서 일반 이동 입력이 섞이지 않는지
4. README/PLAN/실제 키맵이 일치하는지

---

## 28) 변경관리/추적 정책 (보강)

변경관리 원칙:
1. 기능 변경 시 PLAN의 해당 섹션 번호를 함께 갱신
2. 키맵/파일 포맷 변경은 하위호환 여부를 명시
3. 범위 추가는 `범위 제외 항목`에서 이동 처리(삭제 금지, 이력화)

작업 추적(권장):
- 커밋 메시지에 WBS 번호 태그 사용
  예: `WBS-2: implement 10x10 renderer`
- 버그 수정 시 재현 절차 3줄 이상 기록

---

## 29) 인수 테스트 시나리오(최종) (보강)

시나리오 A: 기본 이동/충돌
1. 시작 후 10x10 맵 표시 확인
2. 빈 타일로 4방향 이동
3. 벽 방향 입력 시 좌표 불변 확인

시나리오 B: Door 전환
1. Door 인접 위치로 이동
2. `1` 입력 시 대상 room 전환 확인
3. 동일 절차에서 `2` 입력 시 전환 취소 확인

시나리오 C: Stair 전환
1. Stair 인접 위치에서 진입 프롬프트 호출
2. yes/no 각각 동작 확인
3. 전환 후 좌표가 stair/spawn 규칙과 일치하는지 확인

시나리오 D: Box 상호작용
1. Box 접근 시 이동 차단 + 프롬프트 확인
2. yes 선택 시 effect -> item 2단계 노출 확인
3. 취소 시 좌표/상태가 유지되는지 확인

합격 기준:
- A~D 전부 통과
- 크래시/무한루프/빈 메시지 없음

---

## 30) 일정/진척 기준선 (보강)

권장 일정(예: 5일 기준):
1. Day 1: WBS-1 완료 (전처리 + 검증)
2. Day 2: WBS-2 렌더/입력 완료
3. Day 3: WBS-2 이동/충돌 + WBS-3 일부
4. Day 4: WBS-3 완료 + 통합 테스트
5. Day 5: WBS-4 문서/릴리즈 정리

지연 판단 기준:
- 특정 Day 종료 시 핵심 산출물 미완료면 다음 단계 착수 금지
- 우선순위는 항상 `10x10 안정 동작 > 기능 확장`

---

## 31) 개발환경 셀프체크 스크립트 요구사항 (보강)

`tools/check_env.sh` 권장 항목:
1. `command -v cl65`
2. `command -v tail`
3. `command -v rdedisktool` (없으면 warning)
4. `cl65 --version` 출력
5. 작업 경로/권한 확인

실행 정책:
- `compile.sh` 시작 시 `check_env.sh` 호출
- 필수 도구 누락 시 즉시 실패(명확한 에러 메시지)

---

## 32) 백아웃/복구 계획 (보강)

목적:
- 신규 변경이 타깃 실행을 깨뜨릴 때 빠르게 직전 안정 상태로 복귀

정책:
1. 기능 단위 커밋 유지 (WBS 단위)
2. 빌드 성공 커밋 태그 또는 메모 유지
3. 회귀 발견 시 마지막 인수 테스트 통과 커밋으로 복귀 후 재적용

복구 체크:
1. `compile.sh all` 성공
2. 인수 테스트 A~D 재실행
3. 실패 원인/대응을 PLAN 또는 작업 로그에 기록

---

## 33) rdedisktool + AppleWin(D2 삽입) 실행 계획 (보강)

목표:
- 빌드된 `HELLO`를 ProDOS 프로그램 디스크(D2)에 삽입하고,
- ProDOS 부트 디스크(D1)와 함께 AppleWin을 실행한다.

표준 실행 순서:
1. `./compile.sh build`로 `HELLO` 생성
2. D2 원본 템플릿 디스크(`Tutorial_apple_prodos_01.po`)를 작업 디스크로 복사
3. `rdedisktool add --type B --addr 0x0803`로 `HELLO`를 D2에 삽입
4. AppleWin을 `--d1 <boot.po> --d2 <program.po>`로 실행

스크립트화:
- 경로: `./Examples/prototype_01_AppleII_prodos/run_applewin_prodos.sh`
- 역할:
1. `rdedisktool`/`AppleWin(sa2)` 자동 탐색
2. 필요 시 `compile.sh build` 자동 실행
3. D2 작업 디스크 자동 생성(`/tmp` 기본)
4. `HELLO`를 D2에 삽입 후 AppleWin 실행

기본 디스크 경로 정책:
- D1(boot): `./diskwork/bootdisk/AppleII/ProDOS_2_4_3.po` 우선
- D2(template): `./Examples/Tutorial_apple_prodos_01/Tutorial_apple_prodos_01.po` 우선

운영 커맨드(권장):
```bash
cd ./Examples/prototype_01_AppleII_prodos
./run_applewin_prodos.sh
```

AppleWin 기동 후 가이드:
- `CAT,D2` : D2 파일 확인
- `PREFIX,D2` : 기본 볼륨 전환
- `-HELLO` : 프로그램 실행

실패 대응:
1. `rdedisktool` 미탐지: `RDEDISKTOOL=/abs/path/rdedisktool` 지정
2. `sa2` 미탐지: `APPLEWIN=/abs/path/sa2` 지정
3. 디스크 경로 문제: `BOOT_DISK=... PROGRAM_DISK_TEMPLATE=...` 지정

---

## 부록 A) 참고 경로

- 원본 엔진: `/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/src`
- 원본 데이터: `/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/data`
- Apple2Flat: `./Library/AppleII/apple2flat`
- 대형 레퍼런스: `./Library/AppleII/souless_apple2`
- ProDOS 예제: `./Examples/Tutorial_apple_prodos_01`
- 계획 산출 경로: `./Examples/prototype_01_AppleII_prodos`
