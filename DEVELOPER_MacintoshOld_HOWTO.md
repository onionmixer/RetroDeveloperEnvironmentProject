# DEVELOPER MacintoshOld HOWTO

이 문서는 현재 저장소에서 실제 사용 가능한 **클래식 Macintosh(System 6/7, 68K)** 개발 경로만 정리합니다. PowerPC / Mac OS 9+ / Mac OS X 는 다루지 않습니다(snow가 68K만 에뮬레이트합니다).

## 1. 공통 전제

- 프로젝트 루트: `05_RetroDeveloperEnvironmentProject`
- 기본 에뮬레이터 실행 스크립트: `./run_snow_mac.sh`
- 기본 모델: **Mac SE FDHD** (`MAC_MODEL=se_fdhd`, SuperDrive 800K + 1.44 MB)
- 기본 부팅 디스크: `diskwork/bootdisk/macintosh/system_608.img` (1.44 MB raw, System 6.0.8 trim 적용)

핵심 도구:
- 크로스 툴체인: `Library/Retro68` (m68k-apple-macos, gcc 12.2 + binutils 2.39)
- 빌드 출력 위치: `Library/Retro68-build/toolchain/` (out-of-source, gitignore)
- 디스크 도구: `RetroDeveloperEnvironmentDisktool/build/rdedisktool`
- 에뮬레이터: `Emulator/macintosh/snow/target/release/snowemu` (snow @v1.4.1)

ROM / 디스크 이미지 위치:
- ROM: `resource/Macintosh/rom/{plus,SE_FDHD,Classic,SE30,MacII}.rom` (사용자 제공, gitignore)
- System(부팅) 디스크: `diskwork/bootdisk/macintosh/system_608*.{img,image}` (저작권상 gitignore, README만 트래킹)
- 빈 디스크 템플릿: `diskwork/emptydisk/macintosh/empty_hfs_{800,1440}.img` (재생성 가능, 트래킹)
- 예제별 파생 디스크: 각 예제 디렉터리 내부 (예: `Examples/prototype_01_mac_finder/system_608_with_hello.img`, gitignore)

## 2. Retro68 크로스 툴체인

### 2.1 빌드 (1회)

```bash
sudo apt install cmake libgmp-dev libmpfr-dev libmpc-dev libboost-all-dev bison flex texinfo ruby
mkdir -p Library/Retro68-build
(cd Library/Retro68-build && ../Retro68/build-toolchain.bash --no-ppc) 2>&1 | tee Library/Retro68-build/build.log
```

- `--no-ppc`: snow가 PowerPC 미지원이라 절반 시간 절약 (~20–30분)
- 산출: `Library/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake` (CMake 툴체인 파일)

### 2.2 PATH 추가 (선택)

```bash
export PATH="$PWD/Library/Retro68-build/toolchain/bin:$PATH"
m68k-apple-macos-gcc --version
```

### 2.3 핵심 도구

| 도구 | 역할 |
|------|------|
| `m68k-apple-macos-gcc` | 68K Mac C/C++ 컴파일러 |
| `Rez` | `.r` resource 정의 → `.rsrc` (Apple Rez 호환) |
| `Elf2Mac` | ELF → Mac APPL 변환 (linker wrapper) |
| `MakePEF` / `MakeImport` | PowerPC 전용 (이번 빌드에선 미생성) |
| `LaunchAPPL` | 다양한 emu/실기로 앱 launch (minivmac/serial/ssh 등) |

헤더는 **Multiversal Interfaces** (`Library/Retro68/multiversal/`) 사용. Apple Universal Interfaces가 필요하면 `Library/Retro68/InterfacesAndLibraries/` 에 직접 배치(저작권상 미커밋).

## 3. 디스크 이미지 작업 (rdedisktool)

### 3.1 빌드 (1회)

```bash
cd RetroDeveloperEnvironmentDisktool && mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)
```

### 3.2 지원 컨테이너 (Mac, 모두 양방향)

| 포맷 | 확장자 | 용도 |
|------|--------|------|
| `mac_img` | `.img` / `.dsk` | raw 512B sector — snow가 가장 안정적으로 읽는 형태 |
| `mac_dc42` | `.image` / `.dc42` | Apple Disk Copy 4.2 (84B header + ROR32+BE16 checksum) |
| `mac_moof` | `.moof` | Applesauce MOOF (GCR 400K/800K + MFM 1.44M, snow가 변경분을 저장하는 형식) |

```bash
RDE=./RetroDeveloperEnvironmentDisktool/build/rdedisktool
$RDE convert mac.image mac.img -f mac_img       # DC42 → raw
$RDE convert mac.img mac.dc42 -f mac_dc42       # raw → DC42 (re-checksum)
$RDE convert mac.img mac.moof -f mac_moof       # raw → MOOF
$RDE convert game.moof game.img -f mac_img      # snow가 저장한 MOOF를 다시 raw로
```

### 3.3 HFS 볼륨 생성 / 검사

```bash
$RDE create mac.img -f mac_img --fs hfs -n MyVolume                 # 1440K (default)
$RDE create mac800.img -f mac_img --fs hfs -n V -g 80:2:10:512      # 800K
$RDE create mfs.img    -f mac_img --fs mfs -n V -g 80:1:10:512      # 400K MFS only

$RDE info  mac.img -v                         # File System: HFS, BootDisk yes/no, ...
$RDE list  mac.img -v                         # Mac에선 -v 가 MacTy/Creat/FFlg 컬럼 추가
$RDE list  mac.img "System Folder"            # MacRoman 이름은 따옴표 필수
```

### 3.4 Mac 앱 추가 (resource fork 보존)

`add` 의 Mac-only 옵션 두 가지:

```bash
# MacBinary 단일 파일 (Retro68의 .bin 출력)
$RDE --bootdisk-mode warn add --macbinary mac.img Hello.bin
# (target name 은 MacBinary Pascal name 에서 자동 추출)

# AppleDouble 페어 (Retro68의 Hello.APPL + %Hello.ad)
$RDE --bootdisk-mode warn add --apple-double mac.img Hello.APPL
# (sidecar 자동 탐색: ._X / %X / %stem.ad)
```

부팅 디스크에 변형이 필요할 때 `--bootdisk-mode warn`은 **safe-add 검증을 유지하면서 진행**하는 모드입니다(`strict`는 차단, `off`는 검증 없이 통과).

### 3.5 Mac 앱 추출 (round-trip)

```bash
$RDE extract mac.img "Hello" --macbinary    ./Hello.bin
$RDE extract mac.img "Hello" --apple-double ./out/        # 데이터 fork + ._Hello sidecar
# bare extract 는 데이터 fork만 가져옴 — Mac 앱은 사실상 무용지물
```

### 3.6 알려진 한계 (rdedisktool 0ebb152 시점)

- `mac_dc42`는 from-scratch `create` 불가 → `mac_img` 만든 뒤 `convert -f mac_dc42`
- HFS catalog는 leaf split + depth 1→2 root promotion 까지. Cascading index split 미구현
- Extents Overflow B-tree 는 read-only (write 보류)
- 800K MFS create는 12-bit allocation map 한계로 거부 (400K MFS 만 가능, 800K MFS는 read-only)

## 4. snow 에뮬레이터 실행

### 4.1 빌드 (1회)

```bash
sudo apt install libasound2-dev pkg-config
# rustup 필요 (rust-toolchain.toml이 1.95.0 자동 설치 처리):
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain none
cd Emulator/macintosh/snow && cargo build --release          # ~2분
```

### 4.2 실행 (run_snow_mac.sh)

```bash
./run_snow_mac.sh                         # 기본: SE FDHD + system_608.img
MAC_MODEL=plus ./run_snow_mac.sh          # Mac Plus(800K only) 로 전환
BOOT_DISK=other.img ./run_snow_mac.sh
EXTRA_DISKS=app1.dsk:app2.dsk ./run_snow_mac.sh   # 추가 floppy 슬롯
```

지원 모델: `plus | se_fdhd | classic | se30 | macii`. 시작 시 ROM 크기와 시그니처를 출력하므로 잘못된 ROM(특히 trailer 붙은 macmade ROM)을 즉시 발견 가능.

| 표준 ROM 시그니처 | 모델 | 표준 크기 |
|------|------|------|
| `4D1F8172` | Mac Plus | 131,072 (128 KiB) |
| `B306E171` | Mac SE FDHD | 262,144 (256 KiB) |

크기가 안 맞으면 `truncate -s <bytes> file.rom` 으로 trailer 잘라내면 됩니다.

### 4.3 snow 핵심 사실

- **단일 Floppy Drive**. snow의 SE FDHD 에뮬레이션은 내부 드라이브 1대 — `--floppy` 를 두 번 주면 두 번째가 첫 번째를 덮어씀(부팅 실패 + 자동 eject). **두 디스크 동시 사용은 GUI에서 동적 swap** 또는 **단일 합성 디스크** 로 처리.
- **MOOF write-back asymmetry**. snow가 변경된 floppy를 저장하면 항상 **MOOF 형식**. rdedisktool 0ebb152 부터 MOOF read/write 지원 — 라운드트립 가능.
- **fluxfox `.image` 오감지**: snow 번들 fluxfox가 일부 환경에서 DC42(`.image`)를 raw로 잘못 감지해 sector 0 에 헤더가 섞임. 대처: 마운트 전 `convert -f mac_img` 로 raw 변환.
- **headless 미지원** — egui GUI 또는 TUI 둘 다 디스플레이 필요.

## 5. 예제 — prototype_01_mac_finder

빌드부터 부팅까지 한 디렉터리 안에서 닫히는 표준 Mac 예제 패턴.

### 5.1 구성

```
Examples/prototype_01_mac_finder/
├── CMakeLists.txt          # add_application(Hello hello.c hello.r)
├── hello.c                 # InitGraf/Dialogs + GetNewDialog + ModalDialog 루프
├── hello.r                 # 'DLOG' (128) + 'DITL' (128: Button "Hello") + 'SIZE'
├── compile.sh              # build | disk | clean | test | all
├── run_snow_mac.sh         # BOOT_DISK 고정 후 프로젝트 루트 launcher 위임
├── README.md
├── build/                  # gitignore (cmake out-of-source)
└── system_608_with_hello.img  # gitignore (compile.sh disk 가 매번 재생성)
```

### 5.2 사용

```bash
cd Examples/prototype_01_mac_finder
./compile.sh all          # build (Retro68) → disk (rdedisktool) → test (rdedisktool list -v)
./run_snow_mac.sh         # snow에서 단일 합성 디스크 부팅
```

### 5.3 disk 단계 동작 (rdedisktool)

```bash
cp <repo>/diskwork/bootdisk/macintosh/system_608.img ./system_608_with_hello.img
$RDE --bootdisk-mode warn add --force --macbinary ./system_608_with_hello.img build/Hello.bin
$RDE list ./system_608_with_hello.img -v
# Hello (APPL/????/00) 와 System Folder 두 개만 root에 보여야 함
```

System 6.0.8 master는 미리 trim 된 상태(부팅 필수만 남기고 769 KB 여유 확보) — `diskwork/bootdisk/macintosh/system_608.img` 사용.

## 6. 관련 문서

- `DEVELOPER_COMMON_HOWTO.md`
- `Examples/prototype_01_mac_finder/README.md`
- `resource/Macintosh/rom/README.md` (ROM 배치 가이드)
- `diskwork/bootdisk/macintosh/README.md` (부팅 디스크 컨벤션)
- `diskwork/emptydisk/README.md` (전 플랫폼 빈 디스크 템플릿 인벤토리)
- `diskwork/emptydisk/macintosh/README.md` (Mac 빈 HFS 템플릿)
- 메모리 노트(향후 작업 시 참고): `reference_retro68.md`, `reference_snow_emulator.md`, `reference_rdedisktool_macintosh.md`, `reference_mac_prototype_workflow.md`

## 7. 외부 참고 자료 (resource/Macintosh/)

| 디렉터리 | 출처 | 용도 |
|---------|------|------|
| `resource/Macintosh/QuickDraw` | [jrk/QuickDraw](https://github.com/jrk/QuickDraw) | Apple QuickDraw 원본 (68K asm + Pascal, 17,101 LOC). PICT format/BitBlt 등 그래픽 동작을 사양 수준으로 확인할 때. **빌드/링크 금지(라이선스 NOASSERTION), 읽기 전용 reference.** |

향후 Mac 관련 reference 클론을 추가할 경우 `resource/Macintosh/` 아래에 두고 이 표를 갱신하면 됩니다.
