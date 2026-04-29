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

디스크 생성 검증 패턴(권장):
```bash
# 1) create 종료코드 확인
./RetroDeveloperEnvironmentDisktool/build/rdedisktool create /tmp/msx_test.dsk -f msxdsk --fs msxdos --force

# 2) info 결과 파일시스템 문자열 확인
./RetroDeveloperEnvironmentDisktool/build/rdedisktool info /tmp/msx_test.dsk | rg -q "File System: MSX-DOS"
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

ROM 실행 스크립트:
- `run_openmsx_rom.sh`

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
- `Examples/Tutorial_msx_hitech_01/compile.sh`
- `Examples/Tutorial_msx_hitech_rom_01/README.md`
- `Examples/Tutorial_msx_hitech_rom_02/README.md`

## 7. 외부 참고 프로젝트 (resource/MSX/)

MSX 개발 시 코드/패턴을 참고할 수 있는 외부 오픈소스 프로젝트들이 `resource/MSX/` 아래에 독립 클론으로 보관되어 있습니다(submodule 아님, 워크스페이스 측에선 untracked로 둡니다 — 개인 reference 용).

| 디렉터리 | 출처 | 사용 시점 |
|---------|------|----------|
| `resource/MSX/kingsvalley` | [pdpdds/kingsvalley](https://github.com/pdpdds/kingsvalley) | 왕가의 계곡 1 C 재구현. **게임 로직 / 스테이지 데이터 구조** 참고. SDCC 빌드 사례. |
| `resource/MSX/ubox_example` | [pdpdds/ubox_example](https://github.com/pdpdds/ubox_example) | Juan J. Martinez의 `ubox` 라이브러리 사용 예. 본 워크스페이스의 `Library/MSX/ubox-msx-lib-z88dk` (z88dk 포팅판)와 짝. **VRAM/스프라이트/사운드 호출 패턴** 참고. |
| `resource/MSX/noborunoca` | [h1romas4/noborunoca](https://github.com/h1romas4/noborunoca) | z88dk-msx-template 기반의 완성된 게임. **GitHub Actions 빌드 파이프라인 + 32 KiB ROM 패키징** 사례. |
| `resource/MSX/z88dk-msx-template` | [h1romas4/z88dk-msx-template](https://github.com/h1romas4/z88dk-msx-template) | z88dk(`zcc +msx`) 기반 빌드 템플릿. **CMake 통합 + GDB(z88dk-gdb) 디버깅 흐름** 참고. |

이들 클론은 직접 빌드/링크하지 않습니다. 패턴 학습용 reading material로만 사용하세요. 우리 빌드 흐름은 `Toolchain/MSX/HITECH_TOOLCHAIN/`(Hi-Tech C) + system z88dk(`/opt/z88dk/bin/zcc`) + `Library/MSX/ubox-msx-lib-z88dk/` 조합을 사용합니다.
