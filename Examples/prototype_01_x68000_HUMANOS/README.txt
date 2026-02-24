prototype_01_x68000_HUMANOS
===========================

최신 상태 (2026-02-24)
- `rdedisktool` X68000 생성/포맷 경로 확인 완료
- FDD1 준비 절차: 빈 XDF 생성 -> Human68k 포맷 -> `info/validate` 검증 -> 파일 삽입
- boot disk 복제 fallback 경로 제거(실패 시 즉시 중단)

목표
- prototype/01 핵심 루프를 X68000 Human68k로 이식
- 100x100 룸에서 20x20 가시 맵 렌더
- Door/Stair/Box 상호작용 유지
- 몬스터 정적 표시(`$`) 및 플레이어 충돌 메시지
  - 플레이어 이동 기준 2턴당 1회 상태머신 기반 이동(patrol/chase/return)
  - LOS(시야) + range 기준 추격/복귀 전환
  - 이동 중 충돌 시 직전 위치로 되돌림
  - 전환 직후 충돌 시 안전 좌표로 재배치
  - 충돌 몬스터는 RETURNING으로 강제 전환
  - 몬스터는 floor(`.`) 타일만 순찰(문/계단/상자 회피)
  - 상태 메시지는 항상 같은 줄에 overwrite
  - 긴 상태 메시지는 자동 축약(`...`)

키 조작
- W/A/S/D : 이동
- H       : 도움말(HELP.TXT 스크롤 뷰, 파일 없으면 내장 도움말)
- M       : 몬스터 이동 on/off
- 1/0     : yes/no
- Q       : 종료 모달

도움말 스크롤 키
- W/S : 한 줄 위/아래
- Space/H : 도움말 종료
- Q : 종료 확인

빌드
1) 툴체인 확인
   ./tools/check_env.sh

2) 빌드 + 검증
   ./compile.sh all

산출물
- ./build/prototype_01.x
- ./prototype_01.x
- ./PROTO01.X

GUI 실행(px68k)
- 작업용 XDF 생성 -> Human68k 포맷 -> 파일 주입 -> FDD1 장착 후 실행

  ./compile.sh run

데이터 전처리
- 원본 JSON -> 정적 C 데이터 생성

  python3 ./tools/json_to_room_data_x68000.py --output-dir ./src

호스트 검증
- 전처리/링크/카메라/logic 전이 검증

  ./compile.sh test

- FDD1 disk-add 드라이런 검증(무GUI)

  ./compile.sh disktest

rdedisktool 요구사항
- `rdedisktool help create`에 `X68000: xdf, dim` / `X68000: human68k`가 표시되어야 함
- FDD1 작업 디스크는 빈 XDF 생성 후 Human68k 포맷/검증을 통과해야 함

검증 기준(권장)
- `./compile.sh disktest` 로그에 `File System: Human68k`가 보여야 함
- `fallback to boot disk copy` 문자열이 나오면 실패로 간주

디버그 빌드(선택)
- 몬스터 위상 출력:

  CFLAGS='-DDEBUG_MONSTER_PHASE' ./compile.sh build
