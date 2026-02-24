# RetroDeveloperEnvironmentProject REQUIREMENTS

## 프로젝트 구성 요소

| 구성 요소 | 설명 | 빌드 필요 |
|-----------|------|----------|
| rdedisktool | 디스크 이미지 조작 도구 (Apple II/MSX/X68000) | O (CMake) |
| rdemonitor | 에뮬레이터 디버그 모니터 | O (Make) |
| AppleWin (sa2) | Apple II 에뮬레이터 (Linux) | O (CMake) |
| openMSX | MSX 에뮬레이터 | O (Make) |
| px68k-onionmixer | X68000 에뮬레이터 | O (Make) |
| HITECH_TOOLCHAIN | Hi-Tech C Z80 네이티브 크로스 컴파일러 | O (Make) |
| z88dk (zcc/sccz80) | MSX용 Z80 크로스 컴파일러 | 패키지/소스 설치 |
| cc65 | Apple II용 크로스 컴파일러 | 패키지 설치 |
| apple2flat | Apple II 저수준 C/ASM 프레임워크 (실험적) | 선택 |
| X68000 m68k-xelf toolchain | X68000 크로스 컴파일러 | 저장소 포함 |
| zxcc | CP/M 에뮬레이터 (Hi-Tech C DOS 버전 실행용) | 선택 |

---

## Ubuntu 22.04: 환경 구축

### 1. 기본 빌드 도구 준비

```bash
sudo apt update
sudo apt install build-essential cmake git make pkg-config python3 python3-pip
```

### 2. 플랫폼별 컴파일러 설치

```bash
sudo apt install cc65
```

- MSX 경로는 기본적으로 `z88dk`(`zcc`, `sccz80`)를 사용합니다.
- 본 저장소의 현재 MSX 포팅 작업은 SDCC를 대상에서 제외합니다.

z88dk 설치 예시(소스 배치형):
```bash
wget https://github.com/z88dk/z88dk/releases/download/v2.4/z88dk-src-2.4.tgz
tar -xf z88dk-src-2.4.tgz
sudo mv z88dk /opt/z88dk
echo 'export Z88DK=/opt/z88dk' | sudo tee /etc/profile.d/z88dk.sh
echo 'export PATH=$Z88DK/bin:$PATH' | sudo tee -a /etc/profile.d/z88dk.sh
source /etc/profile.d/z88dk.sh
```

---

## 프로젝트 하위 컴포넌트 빌드

### 3. rdedisktool 빌드

```bash
cd RetroDeveloperEnvironmentDisktool
mkdir -p build && cd build
cmake ..
make -j"$(nproc)"
```

- 빌드 결과: `RetroDeveloperEnvironmentDisktool/build/rdedisktool`
- 상세: `RetroDeveloperEnvironmentDisktool/README.md`, `FLOPPY_IMAGE_OPERATION.md`

### 4. rdemonitor 빌드

```bash
sudo apt install libncurses-dev libcjson-dev
cd RetroDeveloperEnvironmentMonitor
make
```

- 빌드 결과: `RetroDeveloperEnvironmentMonitor/rdemonitor`
- 상세: `RetroDeveloperEnvironmentMonitor/README.md`

### 5. AppleWin (sa2) 빌드

```bash
sudo apt install libsdl2-dev libsdl2-image-dev libpcap-dev \
  libyaml-dev libminizip-dev libboost-dev
cd Emulator/AppleWin
mkdir -p build && cd build
cmake ..
make -j"$(nproc)"
```

- 빌드 결과: `Emulator/AppleWin/build/sa2`

### 6. openMSX 빌드

```bash
sudo apt install libsdl2-dev libsdl2-ttf-dev tcl-dev libpng-dev \
  libogg-dev libvorbis-dev libtheora-dev libglew-dev
cd Emulator/openMSX
make -j"$(nproc)"
```

- 빌드 결과: `Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx`
- MSX-DOS 경로에서는 GT BIOS ROM이 필요합니다.

### 7. HITECH_TOOLCHAIN 빌드

```bash
cd Toolchain/MSX/HITECH_TOOLCHAIN
make -j"$(nproc)"
```

- 빌드 결과: `Toolchain/MSX/HITECH_TOOLCHAIN/bin/*`

### 8. X68000 환경 빌드

```bash
# 예제/테스트 빌드
cd Emulator/x68000
export PATH="$(pwd)/../../Toolchain/x68000/toolchain/m68k-xelf/bin:$PATH"
make

# px68k-onionmixer 빌드(필요 시)
cd ../x68000/px68k-onionmixer
make
```

- 크로스 툴체인: `Toolchain/x68000/toolchain/m68k-xelf`
- 튜토리얼/참고: `Toolchain/x68000/X68KTutorials`

### 9. Apple2Flat (선택, 실험적)

`Toolchain/AppleII/apple2flat`은 cc65 기반 Apple II 프레임워크입니다.

- 상태: WIP (일반 사용 전 단계)
- 기본 타겟: `disk` (`make` 기본)
- 주요 산출물(성공 시):
  - `Toolchain/AppleII/apple2flat/temp/a2f_demo.dsk`
  - `Toolchain/AppleII/apple2flat/temp/a2f_demo.sym`
  - `Toolchain/AppleII/apple2flat/temp/a2f_disk.lib`
  - `Toolchain/AppleII/apple2flat/temp/a2f_tape.lib`
  - `Toolchain/AppleII/apple2flat/temp/a2f_cc65.lib`

중요:
- 기본 `makefile.common`은 `Toolchain/AppleII/apple2flat/cc65/bin/*`을 기대합니다.
- 현재 저장소에는 해당 `cc65/` 디렉터리가 없으므로, 시스템 cc65를 명시 오버라이드해야 합니다.

```bash
cd Toolchain/AppleII/apple2flat
make \
  CC65="$(command -v cc65)" \
  CA65="$(command -v ca65)" \
  LD65="$(command -v ld65)" \
  AR65="$(command -v ar65)"
```

### 10. zxcc 설치 (선택)

```bash
sudo dpkg -i zxcc/zxcc_0.5.7-1_amd64.deb
```

또는 소스 빌드:
```bash
cd zxcc/ZXCC
mkdir -p build && cd build
cmake ..
make
sudo make install
```

---

## 실행 스크립트 (권장)

| 스크립트 | 대상 | 주요 환경변수 |
|---------|------|--------------|
| `run_applewin_dos33.sh` | Apple II DOS 3.3 | `APPLEWIN`, `BOOT_DISK`, `PROGRAM_DISK` |
| `run_applewin_prodos.sh` | Apple II ProDOS | `APPLEWIN`, `BOOT_DISK`, `PROGRAM_DISK` |
| `run_openmsx_msxdos2.sh` | openMSX + MSX-DOS2 | `OPENMSX`, `OPENMSX_SHARE`, `BOOT_DISK`, `MACHINE` |
| `run_px68k_humanos.sh` | px68k + Human68k | `PX68K`, `IPL_ROM`, `CG_ROM`, `BOOT_DISK`, `FDD1_DISK` |

---

## 설치/환경 검증

```bash
# 컴파일러
cl65 --version
/opt/z88dk/bin/zcc --version

# 프로젝트 도구
./RetroDeveloperEnvironmentDisktool/build/rdedisktool --version
./RetroDeveloperEnvironmentMonitor/rdemonitor --help

# 에뮬레이터 바이너리
./Emulator/AppleWin/build/sa2 --help
./Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx --version
./Emulator/x68000/px68k-onionmixer/px68k-onionmixer --help

# 스크립트 문법 검증
bash -n ./run_applewin_dos33.sh
bash -n ./run_applewin_prodos.sh
bash -n ./run_openmsx_msxdos2.sh
bash -n ./run_px68k_humanos.sh
```

---

## 프로젝트 디렉터리 구조 (요약)

```text
05_RetroDeveloperEnvironmentProject/
├── Emulator/
│   ├── AppleWin/
│   ├── openMSX/
│   └── x68000/
├── RetroDeveloperEnvironmentDisktool/
├── RetroDeveloperEnvironmentMonitor/
├── Toolchain/
│   ├── AppleII/
│   ├── MSX/HITECH_TOOLCHAIN/
│   └── x68000/
├── Examples/
├── diskwork/
├── specs/
│   ├── MSX_TEST_WORK_REPORT.md
│   └── RetroDeveloperEnvironmentProject_OUTPUT_SPEC_V01.md
├── DEVELOPER_AppleII_HOWTO.md
├── DEVELOPER_MSX_HOWTO.md
├── DEVELOPER_X68000_HOWTO.md
├── FLOPPY_IMAGE_OPERATION.md
├── DEVELOPER_COMMON_HOWTO.md
├── run_applewin_dos33.sh
├── run_applewin_prodos.sh
├── run_openmsx_msxdos2.sh
├── run_px68k_humanos.sh
└── RetroDeveloperEnvironmentProject_REQUIREMENTS.md
```

---

## 관련 문서

- `DEVELOPER_AppleII_HOWTO.md`
- `DEVELOPER_MSX_HOWTO.md`
- `DEVELOPER_X68000_HOWTO.md`
- `FLOPPY_IMAGE_OPERATION.md`
- `specs/RetroDeveloperEnvironmentProject_OUTPUT_SPEC_V01.md`

---

## 변경 이력

| 버전 | 날짜 | 변경 내용 |
|------|------|----------|
| V1.0 | 2025-01-02 | 초안 작성 - 크로스 컴파일러 설치 가이드 |
| V2.0 | 2026-02-19 | 전면 갱신 - 프로젝트 구성요소 빌드 가이드, 디렉터리 구조, 검증 방법 추가 |
| V2.1 | 2026-02-22 | `specs/` 문서 이동 반영, X68000(px68k/toolchain) 반영, MSX 경로를 zcc 중심으로 정리 |
