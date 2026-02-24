# DEVELOPER MSX HOWTO

이 문서는 현재 저장소에서 실제 사용 가능한 MSX 개발 경로만 정리합니다.

## 1. 공통 전제

- 프로젝트 루트: `05_RetroDeveloperEnvironmentProject`
- 기본 에뮬레이터 실행 스크립트: `./run_openmsx_msxdos2.sh`
- 기본 부팅 디스크: `diskwork/bootdisk/msx/msxdos23.dsk`

MSX 관련 도구 경로:
- 스크립트: `tools/msx/*.sh`
- BANKING: `Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/*`
- PACKING: `Toolchain/MSX/HITECH_TOOLCHAIN/bin/PACKING/*`
- BANKING 소스: `Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/*`
- PACKING 소스: `Toolchain/MSX/HITECH_TOOLCHAIN/source/PACKING/*`

## 2. Hi-Tech C 경로

### 2.1 툴체인 위치

- 루트: `Toolchain/MSX/HITECH_TOOLCHAIN`
- 컴파일러/어셈블러/링커:
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/cpp_new3`
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/p1x3`
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/cgen3`
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/optim3`
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/zasx3`
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/linq3`

### 2.2 권장 스크립트

```bash
./tools/msx/test_sharksym_lib_compile.sh
./tools/msx/build_sharksym_wa.sh
./tools/msx/build_sharksym_phase5_wa.sh
```

### 2.3 ROM 모드(뱅킹/패킹)

- 기본 경로: C 네이티브 도구(`bltoolc`) 우선
- Python 스크립트: fallback/검증 용도

ROM 회귀 테스트:
```bash
./tools/msx/test_bltoolc_phase6_mkrule_build_rom.sh
./tools/msx/test_bltoolc_phase6_mkrule_build_rom_matrix.sh
```

## 3. z88dk(zcc) 경로

### 3.1 기본 환경

```bash
export PATH=/opt/z88dk/bin:$PATH
export ZCCCFG=/opt/z88dk/lib/config
```

### 3.2 MSX-DOS 예제

```bash
cd Examples/Tutorial_msx_z88dk_01
./compile.sh all
```

산출물:
- `build/HELLO.COM`
- `build/Tutorial_msx_z88dk_01.dsk`

### 3.3 ROM 예제

```bash
cd Examples/Tutorial_msx_z88dk_rom_01
./compile.sh all
```

산출물:
- `build/HELLO_ROM_Z88DK.rom`

## 4. Hi-Tech C 튜토리얼

### 4.1 MSX-DOS 튜토리얼

```bash
cd Examples/Tutorial_msx_hitech_01
./compile.sh all
```

### 4.2 ROM 튜토리얼(01)

```bash
cd Examples/Tutorial_msx_hitech_rom_01
./compile.sh all
```

openMSX 실행 예:
```bash
DISPLAY=:1 OPENMSX_SYSTEM_DATA="$HOME/.openMSX/share" OPENMSX_DISABLE_SDL_JOYSTICK=1 SDL_AUDIODRIVER=dummy \
../../Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx \
-machine Panasonic_FS-A1GT \
-carta ./build/HELLO48_NONMAPPER.rom \
-romtype normal
```

### 4.3 ROM 튜토리얼(02)

```bash
cd Examples/Tutorial_msx_hitech_rom_02
./compile.sh all
```

### 4.4 ROM 튜토리얼(03)

```bash
cd Examples/Tutorial_msx_hitech_rom_03
./compile.sh all
```

개별 실행 스크립트:
- `run_openmsx_0hello_rom.sh`
- `run_openmsx_0hello_stay_rom.sh`
- `run_openmsx_0hello_stay_diag_rom.sh`
- `run_openmsx_0bgm_rom.sh`
- `run_openmsx_0hangul_rom.sh`
- `run_openmsx_0hanime_rom.sh`
- `run_openmsx_0tetris_rom.sh`
- 전체: `run_openmsx_all.sh`

## 5. run_openmsx_msxdos2.sh 사용

실행:
```bash
./run_openmsx_msxdos2.sh
```

오버라이드 가능 환경변수:
- `OPENMSX`, `OPENMSX_SHARE`, `BOOT_DISK`, `MACHINE`

실행 전 체크:
1. `Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx` 존재
2. `diskwork/bootdisk/msx/msxdos23.dsk` 존재
3. GT BIOS ROM 설치 확인 (`~/.openMSX/share/systemroms/machines/panasonic/`)

## 6. 관련 문서

- `DEVELOPER_COMMON_HOWTO.md`
- `FLOPPY_IMAGE_OPERATION.md`
- `specs/MSX_TEST_WORK_REPORT.md`
- `Examples/Tutorial_msx_z88dk_01/README.md`
- `Examples/Tutorial_msx_z88dk_rom_01/README.md`
- `Examples/Tutorial_msx_hitech_01/README.md`
- `Examples/Tutorial_msx_hitech_rom_01/README.md`
- `Examples/Tutorial_msx_hitech_rom_02/README.md`
- `Examples/Tutorial_msx_hitech_rom_03/README.md`
