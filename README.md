# Retro Developer Environment Project

이 저장소는 Apple II, MSX, X68000 개발/빌드/디스크 작업/에뮬레이터 실행을 한 워크스페이스에서 처리하기 위한 통합 환경입니다.

## 핵심 구성

- 디스크 이미지 도구: `RetroDeveloperEnvironmentDisktool`
- 디버그 모니터: `RetroDeveloperEnvironmentMonitor`
- 에뮬레이터:
  - `Emulator/AppleWin`
  - `Emulator/openMSX`
  - `Emulator/x68000`
- 툴체인:
  - `Toolchain/MSX/HITECH_TOOLCHAIN`
  - `Toolchain/x68000`
- 라이브러리/참고 자산:
  - `Library/AppleII`
  - `Library/MSX`
  - `Library/x68000`

## MSX 도구 재배치 요약

MSX 뱅킹/패킹 도구는 다음 구조를 사용합니다.

- 실행 엔트리:
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/*`
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/PACKING/*`
- 소스:
  - `Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/*`
  - `Toolchain/MSX/HITECH_TOOLCHAIN/source/PACKING/*`
- MSX 자동화 스크립트:
  - `tools/msx/*.sh`

운영 원칙:
- MSX 컴파일/빌드/패킹은 C 도구(`bltoolc`, `pack-nonmapper`)를 기본 경로로 사용
- Python 도구는 fallback 또는 검증(parity/리포트) 용도로만 사용

## 부팅 디스크 경로

- Apple II: `diskwork/bootdisk/AppleII/`
- MSX: `diskwork/bootdisk/msx/`
- X68000: `diskwork/bootdisk/x68000/`

## 빠른 시작

### 1) 주요 도구 빌드

```bash
# rdedisktool
cd RetroDeveloperEnvironmentDisktool
mkdir -p build && cd build
cmake ..
make -j"$(nproc)"

# rdemonitor
cd ../../RetroDeveloperEnvironmentMonitor
make
```

### 2) 플랫폼별 실행 스크립트

프로젝트 루트에서:

```bash
./run_applewin_dos33.sh
./run_applewin_prodos.sh
./run_openmsx_msxdos2.sh
./run_px68k_humanos.sh
```

### 3) 최신 튜토리얼 빌드/검증

```bash
# MSX z88dk MSX-DOS2
cd Examples/Tutorial_msx_z88dk_01 && ./compile.sh all
./run_openmsx_tutorial_msx_z88dk_01.sh

# MSX Hi-Tech C ROM (원문 개념의 네이티브 변환)
cd ../Tutorial_msx_hitech_rom_01 && ./compile.sh all

# 생성물:
# - HELLO48.rom: BL_ROM 원본
# - HELLO48_NONMAPPER.rom: openMSX -carta용 AB 헤더 패키징 결과
# - 목적: C 코드(main) -> ROM 변환 -> openMSX Hello World 표시 proof

# MSX Hi-Tech C ROM (BLMKRULE 기반 CFG 경로)
cd ../Tutorial_msx_hitech_rom_02 && ./compile.sh all

# 생성물:
# - TUTORIAL_ROM2.rom: BL_ROM 원본(48KB)
# - TUTORIAL_ROM2_NONMAPPER.rom: openMSX -carta용 AB 헤더 패키징 결과
# - 목적: BLMKRULE/generated .MK 기반 ROM 생성 검증

# MSX z88dk ROM
cd ../Tutorial_msx_z88dk_rom_01 && ./compile.sh all
./run_openmsx_tutorial_msx_z88dk_rom_01.sh

# X68000 run68 스모크
cd ../Tutorial_x68000_01 && ./compile.sh all
```

## 문서 안내

- 공통 가이드: `DEVELOPER_COMMON_HOWTO.md`
- Apple II: `DEVELOPER_AppleII_HOWTO.md`
- MSX: `DEVELOPER_MSX_HOWTO.md`
- MSX Hi-Tech C ROM proof 튜토리얼: `Examples/Tutorial_msx_hitech_rom_01/README.md`
- MSX Hi-Tech C ROM BLMKRULE 튜토리얼: `Examples/Tutorial_msx_hitech_rom_02/README.md`
- MSX z88dk ROM 튜토리얼: `Examples/Tutorial_msx_z88dk_rom_01/README.md`
- X68000: `DEVELOPER_X68000_HOWTO.md`
- 디스크 작업: `FLOPPY_IMAGE_OPERATION.md`
- 요구사항/설치: `RetroDeveloperEnvironmentProject_REQUIREMENTS.md`

## 스펙/작업 기록

- `specs/RetroDeveloperEnvironmentProject_OUTPUT_SPEC_V01.md`
- `specs/MSX_TEST_WORK_REPORT.md`

## 주의사항

- MSX-DOS 디스크 부팅은 FDD 지원 머신/ROM 환경이 필요합니다.
- X68000 Human68k 실행 시 `iplrom.dat`, `cgrom.dat`, `diskwork/bootdisk/x68000/HUMAN302.XDF` 확인이 필요합니다.
- Apple II `apple2flat` 경로(`Library/AppleII/apple2flat`)는 실험적(WIP)입니다.
- `Library/AppleII/souless_apple2`, `Library/MSX/ubox-msx-lib-1.2.0`, `Library/x68000/X68KTutorials`는 외부 라이브러리/참고 자산 경로입니다.
- Hi-Tech C ROM 튜토리얼은 원문(DOS/Windows) 절차를 Linux 네이티브 파이프라인으로 치환한 구현입니다.
- z88dk ROM은 `subtype=rom/rom2`로 타깃을 선택합니다.

## BLMKRULE (Linux) quick check

MSX Hi-Tech C CFG 기반 generated `.MK` 동작 검증:

```bash
./tools/msx/test_blmkrule_mk_pipeline.sh
```

- app-mode0 (`0HELLO`) generated `.MK` build/cleanup 확인
- app-mode2 (`2HELLO`) generated `.MK`에서 Gate3 경로 연동 확인
