prototype_01_MSX_MSXDOS
=======================

목표
- prototype/01 핵심 루프를 MSX-DOS로 이식
- 100x100 룸에서 12x7 가시 맵 렌더
- Door/Stair/Box 상호작용 유지

키 조작
- W/A/S/D : 이동
- H       : 도움말 화면
- 1/0     : yes/no
- Q       : 종료 모달
- 화면 표시:
  WASD(move) 1/0(confirm)
  H(HELP) Q(quit)

빌드
1) z88dk 설치 확인
2) 다음 실행

   ./compile.sh all

산출물
- ./build/HELLO.COM
- ./HELLO.COM
- ./build/prototype_01_MSX_MSXDOS.dsk

에뮬 실행
- bootdisk 복사 + HELLO.COM 추가 + openMSX 실행

  ./run_openmsx_prototype_01_msx_msxdos_diskaddtest.sh

데이터 전처리
- 원본 JSON -> 정적 C 데이터 생성

  python3 ./tools/json_to_room_data_msx.py --output-dir ./src

호스트 검증
- 전처리 검증

  python3 ./tools/json_to_room_data_msx.py --validate
