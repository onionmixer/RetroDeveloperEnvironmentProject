# RetroDeveloperEnvironmentProject REQUIREMENTS

## 프로젝트 구성 요소

| 구성 요소 | 설명 | 빌드 필요 |
|-----------|------|----------|
| rdedisktool | 디스크 이미지 조작 도구 (Apple II/MSX/X68000/Macintosh) | O (CMake) |
| rdemonitor | 에뮬레이터 디버그 모니터 | O (Make) |
| AppleWin (sa2) | Apple II 에뮬레이터 (Linux) | O (CMake) |
| openMSX | MSX 에뮬레이터 | O (Make) |
| px68k-onionmixer | X68000 에뮬레이터 | O (Make) |
| snow | 클래식 Macintosh 에뮬레이터 (68K, Rust) | O (Cargo, rust-toolchain.toml 로 Rust 1.95 pinned) |
| HITECH_TOOLCHAIN | Hi-Tech C Z80 네이티브 크로스 컴파일러 | O (Make) |
| z88dk (zcc/sccz80) | MSX용 Z80 크로스 컴파일러 | 패키지/소스 설치 |
| Library/MSX/ubox-msx-lib-z88dk | z88dk 용 ubox 라이브러리 (MSX-DOS2 BIOS fix) | O (Make, 각 예제 빌드 시 자동) |
| Library/MSX/spman-z88dk | z88dk 용 sprite manager (10_breakout 등에서 사용) | 라이브러리, source-include |
| Library/MSX/mplayer-z88dk | Arkos 2 AKM player z88dk REAL port (05_music/06_sound) | O (각 예제 빌드 시 자동) |
| Library/MSX/ap-z88dk | aplib decompressor z88dk port (kingsvalley 등에서 사용) | O (Make, 자동) |
| cc65 | Apple II용 크로스 컴파일러 | 패키지 설치 |
| apple2flat | Apple II 저수준 C/ASM 프레임워크 (실험적) | 선택 |
| X68000 m68k-xelf toolchain | X68000 크로스 컴파일러 | 저장소 포함 |
| Retro68 | Macintosh 68K / PowerPC 크로스 툴체인 (gcc 12.2 + binutils 2.39 + Rez/Elf2Mac) | O (out-of-source 빌드 ~30-60 분) |
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

Macintosh (Retro68 + snow 사용 시) 추가 의존:
```bash
# Retro68 빌드 의존 (gcc 12.2 + binutils 2.39 + Rez)
sudo apt install cmake libgmp-dev libmpfr-dev libmpc-dev libboost-all-dev \
  bison flex texinfo ruby

# snow 빌드 의존 (Rust 1.95 via rustup)
sudo apt install libasound2-dev pkg-config
# rustup 미설치 시:
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain none
```

⚠️ Macintosh ROM 은 저작권상 본 저장소에 포함되지 않습니다. 사용자가 직접
`resource/Macintosh/rom/` 에 배치 (자세한 명명 컨벤션은 그 디렉터리의 README 참조).

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
- 튜토리얼/참고: `Library/x68000/X68KTutorials`
- Human68k 부팅 디스크(기본): `diskwork/bootdisk/x68000/HUMAN302.XDF`

### 9. Apple2Flat (선택, 실험적)

`Library/AppleII/apple2flat`은 cc65 기반 Apple II 프레임워크입니다.

- 상태: WIP (일반 사용 전 단계)
- 기본 타겟: `disk` (`make` 기본)
- 주요 산출물(성공 시):
  - `Library/AppleII/apple2flat/temp/a2f_demo.dsk`
  - `Library/AppleII/apple2flat/temp/a2f_demo.sym`
  - `Library/AppleII/apple2flat/temp/a2f_disk.lib`
  - `Library/AppleII/apple2flat/temp/a2f_tape.lib`
  - `Library/AppleII/apple2flat/temp/a2f_cc65.lib`

중요:
- 기본 `makefile.common`은 `Library/AppleII/apple2flat/cc65/bin/*`을 기대합니다.
- 현재 저장소에는 해당 `cc65/` 디렉터리가 없으므로, 시스템 cc65를 명시 오버라이드해야 합니다.

```bash
cd Library/AppleII/apple2flat
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

### 11. snow 빌드 (Macintosh 에뮬레이터)

```bash
# 의존: libasound2-dev pkg-config + rustup (위 §2 참조)
cd Emulator/macintosh/snow
cargo build --release
```

- 빌드 결과: `Emulator/macintosh/snow/target/release/snowemu` (~36 MB)
- 첫 빌드 ~2 분. rust-toolchain.toml 로 Rust 1.95.0 pinned (rustup 가 자동 설치)
- 기본 모델: Mac SE FDHD (`MAC_MODEL=se_fdhd` 또는 `run_snow_mac.sh` 의 default)
- ROM 필요: `resource/Macintosh/rom/SE_FDHD.rom` (또는 다른 모델별 ROM)

### 12. Retro68 빌드 (Macintosh 크로스 툴체인, 선택)

```bash
# 의존: cmake libgmp-dev libmpfr-dev libmpc-dev libboost-all-dev bison flex texinfo ruby (§2 참조)
mkdir -p Library/Retro68-build
cd Library/Retro68-build
../Retro68/build-toolchain.bash --no-ppc   # 68K만 (PowerPC 생략, ~30 분)
# 또는 PowerPC 도 (Carbon 빌드, ~60 분):
# ../Retro68/build-toolchain.bash
```

- 빌드 결과: `Library/Retro68-build/toolchain/bin/m68k-apple-macos-gcc` 등
- 첫 빌드: binutils + gcc 를 두 번 빌드 (m68k + ppc) → 매우 길음
- PATH 추가: `export PATH="$PWD/toolchain/bin:$PATH"`
- 산출: `.APPL` + `.bin` (MacBinary) + `.dsk` (raw HFS, `rdedisktool` + snow 와 호환)
- ⚠️ MPW Universal Interfaces 는 재배포 불가 — 직접 `Library/Retro68/InterfacesAndLibraries/` 에 배치

---

## 실행 스크립트 (권장)

| 스크립트 | 대상 | 주요 환경변수 |
|---------|------|--------------|
| `run_applewin_dos33.sh` | Apple II DOS 3.3 | `APPLEWIN`, `BOOT_DISK`, `PROGRAM_DISK` |
| `run_applewin_prodos.sh` | Apple II ProDOS | `APPLEWIN`, `BOOT_DISK`, `PROGRAM_DISK` |
| `run_openmsx_msxdos2.sh` | openMSX + MSX-DOS2 | `OPENMSX`, `OPENMSX_SHARE`, `BOOT_DISK`, `MACHINE` |
| `run_openmsx_rom.sh` | openMSX + MSX ROM 카트 | `OPENMSX`, `OPENMSX_SHARE`, `MACHINE` |
| `run_px68k_humanos.sh` | px68k + Human68k | `PX68K`, `IPL_ROM`, `CG_ROM`, `BOOT_DISK`, `FDD1_DISK` |
| `run_snow_mac.sh` | snow + 클래식 Macintosh (기본 SE FDHD) | `SNOW`, `MAC_MODEL`, `MAC_ROM`, `BOOT_DISK`, `EXTRA_DISKS` |

- `run_px68k_humanos.sh` 기본 `BOOT_DISK`는 `diskwork/bootdisk/x68000/HUMAN302.XDF`를 우선 사용합니다.
- `run_snow_mac.sh` 기본 모델 `MAC_MODEL=se_fdhd`. 다른 모델: `plus`, `se`, `classic`, `se30`, `macii`.

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
./Emulator/macintosh/snow/target/release/snowemu --help

# Retro68 크로스 컴파일러 (Mac, 선택)
./Library/Retro68-build/toolchain/bin/m68k-apple-macos-gcc --version

# 스크립트 문법 검증
bash -n ./run_applewin_dos33.sh
bash -n ./run_applewin_prodos.sh
bash -n ./run_openmsx_msxdos2.sh
bash -n ./run_openmsx_rom.sh
bash -n ./run_px68k_humanos.sh
bash -n ./run_snow_mac.sh
```

---

## 프로젝트 디렉터리 구조 (요약)

```text
05_RetroDeveloperEnvironmentProject/
├── Emulator/
│   ├── AppleWin/                    # Apple II
│   ├── openMSX/                     # MSX
│   ├── x68000/                      # X68000 (mame, px68k-onionmixer, run68x)
│   └── macintosh/snow/              # Macintosh 68K (Rust)
├── RetroDeveloperEnvironmentDisktool/
├── RetroDeveloperEnvironmentMonitor/
├── Toolchain/
│   ├── MSX/HITECH_TOOLCHAIN/        # Hi-Tech C Z80
│   └── x68000/toolchain/m68k-xelf/  # X68000 m68k-xelf GCC
├── Library/
│   ├── AppleII/
│   │   ├── apple2flat/
│   │   └── souless_apple2/
│   ├── MSX/                         # z88dk 가족 (자체 git 아님, workspace 흡수)
│   │   ├── ubox-msx-lib-z88dk/      #   ubox z88dk port (MSX-DOS2 BIOS fix + Phase 7)
│   │   ├── spman-z88dk/             #   sprite manager z88dk port
│   │   ├── mplayer-z88dk/           #   Arkos 2 AKM real port
│   │   ├── mplayer-z88dk-stub/      #   silent placeholder
│   │   ├── ap-z88dk/                #   aplib decompressor z88dk port
│   │   └── MSXgl/                   #   AKG/AKM source 출처 (engine/src/arkos/)
│   ├── x68000/
│   │   └── X68KTutorials/
│   └── Retro68/                     # Macintosh 68K / PowerPC 크로스 툴체인
├── Examples/
│   ├── Tutorial_*                   # Hi-Tech / z88dk 튜토리얼
│   ├── prototype_*                  # 자체 프로토타입 (01/02 tracked, 03+ local-only)
│   ├── kingsvalley_z88dk/           # King's Valley z88dk 포팅 (Phase A/B/C)
│   └── ubox_example_z88dk/          # ubox 12 예제 z88dk 포팅
├── diskwork/
│   ├── bootdisk/                    # 플랫폼별 부팅 디스크
│   └── emptydisk/                   # 빈 / 사전 포맷 템플릿
├── resource/                        # 참고 자료 / 외부 reference
│   ├── Macintosh/                   #   QuickDraw 원본 + Mac ROM 배치 위치
│   ├── MSX/                         #   kingsvalley, ubox_example, noborunoca, z88dk-msx-template (외부 reference)
│   └── x68000/
├── resource_extra/                  # 부가 도구 (zxcc 등)
├── specs/                           # 스펙 / 작업 기록
├── tools/                           # 자동화 스크립트
├── DEVELOPER_COMMON_HOWTO.md
├── DEVELOPER_AppleII_HOWTO.md
├── DEVELOPER_MSX_HOWTO.md           # §3.5 (megaROM + ISR-driven 음악) + §3.6 (SDCC↔z88dk 차이)
├── DEVELOPER_X68000_HOWTO.md
├── DEVELOPER_MacintoshOld_HOWTO.md  # Retro68 → rdedisktool → snow 흐름
├── FLOPPY_IMAGE_OPERATION.md
├── REQUIREMENTS.md                  # (본 문서)
├── README.md
├── run_applewin_dos33.sh / _prodos.sh
├── run_openmsx_msxdos2.sh / _rom.sh
├── run_px68k_humanos.sh
└── run_snow_mac.sh
```

---

## 관련 문서

- `README.md` — 프로젝트 개요
- `DEVELOPER_COMMON_HOWTO.md` — 공통 개발 가이드
- `DEVELOPER_AppleII_HOWTO.md`
- `DEVELOPER_MSX_HOWTO.md` — §3.5 megaROM/매퍼/CALSLT/음악/50Hz vs 60Hz, §3.6 SDCC↔z88dk 차이
- `DEVELOPER_X68000_HOWTO.md`
- `DEVELOPER_MacintoshOld_HOWTO.md`
- `FLOPPY_IMAGE_OPERATION.md`
- `specs/RetroDeveloperEnvironmentProject_OUTPUT_SPEC_V01.md`
- `Examples/kingsvalley_z88dk/PLAN_MIGRATION_KINGSVALLY.md` — z88dk + Konami/ASCII16 사례
- `Examples/ubox_example_z88dk/PLAN_UBOX_EXAMPLES.md` — 12 예제 z88dk 포팅 + Arkos 2 AKM 빌드 파이프라인
- `Examples/prototype_05_MSX_ROM_MSXDOS/MSX_MUSIC_PSG.md` — ISR-driven AKG 음악 reference
- `Examples/prototype_05_MSX_ROM_MSXDOS/SPEC_ROM_KONAMI_MAPPER.md` — Konami 매퍼 spec + 학습

---

## 변경 이력

| 버전 | 날짜 | 변경 내용 |
|------|------|----------|
| V1.0 | 2025-01-02 | 초안 작성 - 크로스 컴파일러 설치 가이드 |
| V2.0 | 2026-02-19 | 전면 갱신 - 프로젝트 구성요소 빌드 가이드, 디렉터리 구조, 검증 방법 추가 |
| V2.1 | 2026-02-22 | `specs/` 문서 이동 반영, X68000(px68k/toolchain) 반영, MSX 경로를 zcc 중심으로 정리 |
| V2.2 | 2026-05-11 | Macintosh (snow + Retro68) 추가, MSX z88dk 라이브러리 가족 추가 (ubox/spman/mplayer/ap), kingsvalley_z88dk + ubox_example_z88dk 워크스페이스 흡수 반영, 디렉터리 구조 + 검증 + 관련 문서 전면 갱신 |
