# Tutorial_msx_hitech_rom_02

이 예제는 `Tutorial_msx_hitech_rom_01`과 달리, Linux용 뱅킹툴 경로(`Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLMKRULE`)를 사용해
CFG 기반으로 Hi-Tech C ROM을 생성하는 튜토리얼입니다.

핵심 목표:
- `app-mode=0`, `rom-mode=2` CFG를 통해 48KB BL_ROM 이미지를 생성
- 같은 빌드 체인에서 AB 헤더 NONMAPPER 카트리지 ROM까지 생성
- openMSX `-carta`로 실행 검증 가능

## 1) 빌드

```bash
cd Examples/Tutorial_msx_hitech_rom_02
./compile.sh all
```

생성물:
- `build/TUTORIAL_ROM2.rom` (48KB, BL_ROM 포맷, `ROM ` 시그니처)
- `build/TUTORIAL_ROM2_NONMAPPER.rom` (48KB, AB 카트리지 포맷)
- `build/TUTORIAL_ROM2_pack_report.json`

## 2) openMSX 실행

```bash
cd Examples/Tutorial_msx_hitech_rom_02
DISPLAY=:1 ./compile.sh run-openmsx
```

동일 명령(풀 경로):
```bash
DISPLAY=:1 OPENMSX_SYSTEM_DATA="$HOME/.openMSX/share" OPENMSX_DISABLE_SDL_JOYSTICK=1 SDL_AUDIODRIVER=dummy \
../../Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx \
-machine Panasonic_FS-A1GT \
-carta ./build/TUTORIAL_ROM2_NONMAPPER.rom \
-romtype normal
```

## 3) 파일 구성

- `TUTORIAL_ROM2.CFG`: 뱅킹툴 입력 CFG (`app-mode=0`, `rom-mode=2`)
- `ROMSTART.AS`: ROM 엔트리(스택/화면 초기화 + main 진입)
- `MAIN.C`: `main(void)` 진입부
- `MAIN_HELPER.AS`: `INITXT/CLS/CHPUT` 기반 출력 루틴
- `compile.sh`: BLMKRULE -> generated `.MK` -> build -> ROM 검증

## 4) 구현 메모

이 예제는 `bltoolc mkrule-build`(C 네이티브) ROM 경로를 기본으로 사용합니다.
- app-mode0 + rom-mode(1/2)에서
  - `linq3 -Ptext=4000H...` 링크 + `psect(data/bss)` 반영
  - `objtohex` 변환
  - HEX -> BL_ROM (`.rom`) 매핑
  - 맵 기반 ROM fixup
    - 엔트리 `CALL/JP` 주소 보정
    - `_main` 내부 helper `CALL` 주소 보정
  - 기본: `bltoolc pack-nonmapper`로 AB 카트리지 ROM 생성
  - Python 패커는 fallback/검증 용도
    - `safe-no-loader` 프로파일
    - `INIT=startup(0x4106)` / no secondary header

최종 확인 상태:
- openMSX에서 BIOS 이후 `Hello World` 정상 텍스트 출력 확인

문제 해결 이력(요약):
- 증상: 파란 화면 정지, 깨진 문자, 흰색 사각형
- 원인:
  - ROM 링크 시 `data/bss` 배치 누락
  - ROM 엔트리/헬퍼 호출 주소의 상위바이트 누락(`0x00xx`)
- 해결:
  - 링크 `psect` 반영
  - 맵 기반 주소 fixup 적용
