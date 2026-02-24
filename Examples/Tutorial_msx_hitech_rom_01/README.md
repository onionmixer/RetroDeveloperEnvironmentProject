# Tutorial_msx_hitech_rom_01

이 예제의 목적은 다음을 증명하는 것입니다.

- Hi-Tech C로 작성한 C 코드가 실제 MSX ROM으로 빌드되고,
- openMSX에서 `Hello World`가 보이는 형태로 재현된다.

## 0) 사전 조건

1. Hi-Tech C 도구 사용 가능
- `Toolchain/MSX/HITECH_TOOLCHAIN/bin` 내부 도구가 실행 가능해야 합니다.

2. openMSX 실행 환경
- openMSX 바이너리: `Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx`
- 시스템 데이터 경로: 기본값 `~/.openMSX/share`
- GT 머신 ROM 사용 가능 상태에서 `Panasonic_FS-A1GT` 실행 가능

3. 디스플레이 환경
- 기본 예시는 `DISPLAY=:1` 기준입니다.

## 1) 5분 Quick Path (권장)

```bash
cd Examples/Tutorial_msx_hitech_rom_01
./compile.sh all
```

생성물 확인:
- `build/HELLO48.rom` (BL_ROM 원본, `ROM ` 시그니처)
- `build/HELLO48_NONMAPPER.rom` (openMSX 카트리지 실행용, `AB` 헤더)

openMSX 실행:

```bash
DISPLAY=:1 OPENMSX_SYSTEM_DATA="$HOME/.openMSX/share" OPENMSX_DISABLE_SDL_JOYSTICK=1 SDL_AUDIODRIVER=dummy \
../../Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx \
-machine Panasonic_FS-A1GT \
-carta ./build/HELLO48_NONMAPPER.rom \
-romtype normal
```

기대 결과(증명 성공 기준):
- BIOS 이후 파란 배경에서 `Hello World` 텍스트 확인
- 문자열 출력 후 프로그램은 안정 대기(화면 고정)

## 2) 코드 작성/ROM 변환 흐름

1. `MAIN.C`에서 `main()` 구현
2. 어셈블리 헬퍼는 `MAIN_HELPER.AS`에서 분리 관리
   - `rom_print_hello`, `rom_debug_loop`, `rom_wait_loop`
3. `compile.sh`가 Hi-Tech C/ASM 파이프라인 실행
   - `cpp_new3 -> p1x3 -> cgen3 -> (optim3|skip) -> zasx3 -> linq3 -> objtohex`
4. HEX를 48KB ROM 이미지(`HELLO48.rom`)로 매핑
5. 기본 경로: `bltoolc pack-nonmapper`로 `AB` 헤더 카트리지 ROM 생성
   (Python 패커는 fallback/검증 용도)
   - 출력: `HELLO48_NONMAPPER.rom`

즉, "C 프로그램 작성 -> 빌드 -> 패키징 -> openMSX 실행"이 한 번에 연결됩니다.

구조 메모:
- `MAIN.C`는 C 로직 중심으로 유지합니다.
- 어셈블리 상세 구현은 `MAIN_HELPER.AS`에 분리하여 관리합니다.

## 3) 기본/회귀/디버그 경로

기본 증명 경로:
- `ROM_ENTRY_MODE=main-pure` (기본값)
- `main()`이 `Hello World` 출력 책임을 가짐

회귀(fallback) 경로:
```bash
cd Examples/Tutorial_msx_hitech_rom_01
ROM_ENTRY_MODE=loop ./compile.sh all
```

디버그 경로(시각 진단용):
```bash
cd Examples/Tutorial_msx_hitech_rom_01
DEBUG_BORDER=1 ./compile.sh all
```

디버그 모드 기대 결과:
- `Hello World`가 보임
- 화면 색상이 주기적으로 전환됨

참고:
- 디버그 경로는 "증명 경로"가 아니라 진단 경로입니다.

## 4) 정적 검증

```bash
cd Examples/Tutorial_msx_hitech_rom_01
./compile.sh verify-main-pure
./compile.sh verify-fallback
```

검증 항목:
- `_main` 진입 경로 고정 검증
- fallback 엔트리 분기/대기 루프 검증
- ROM 크기/시그니처 검증
- `Hello World` CHPUT opcode 패턴 검증

## 5) 운영 스크립트

단계별 실행:
```bash
cd Examples/Tutorial_msx_hitech_rom_01
./test_openmsx_step.sh default
./test_openmsx_step.sh default-verified
./test_openmsx_step.sh fallback
./test_openmsx_step.sh fallback-verified
```

검증 의미:
- `default-verified`: `verify-main-pure` 정적 검증 후 실행
- `fallback-verified`: `verify-fallback` 정적 검증 후 실행

연속 실행:
```bash
cd Examples/Tutorial_msx_hitech_rom_01
./test_openmsx_all_steps.sh
```

## 6) 문제 진단 체크리스트

1. 빌드 산출물 확인
- `build/HELLO48_NONMAPPER.rom` 파일 존재/크기 49152 bytes

2. 실행 인자 확인
- `-machine Panasonic_FS-A1GT`
- `-carta ./build/HELLO48_NONMAPPER.rom`
- `-romtype normal`

3. 관찰 확인
- 문자열: `Hello World`
- 줄바꿈: `CR/LF` 적용
- 상태:
  - 기본/폴백: 안정 대기
  - 디버그: 화면 색상 주기 전환
