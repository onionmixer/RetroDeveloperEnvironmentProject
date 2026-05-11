# Retro Developer Environment Project

Apple II, MSX, X68000, 클래식 Macintosh(System 6/7, 68K) — 네 플랫폼의 크로스 개발, 빌드, 디스크 작업, 에뮬레이터 실행을 하나의 워크스페이스에서 처리하는 통합 레트로 개발 환경입니다.

> **테스트 환경**: 이 환경은 **Ubuntu 22.04 (LTS)** 에서 테스트·검증되었습니다. 다른 배포판/버전에서는 패키지명·경로가 다를 수 있으니 `REQUIREMENTS.md` 와 각 플랫폼 `DEVELOPER_*_HOWTO.md` 의 의존성 섹션을 참고하여 동등 패키지를 설치하세요.

## 프로젝트 구조

```text
RetroDeveloperEnvironmentProject/
├── Emulator/                          # 에뮬레이터
│   ├── AppleWin/                      #   Apple II
│   ├── openMSX/                       #   MSX
│   ├── x68000/                        #   X68000 (mame, px68k, run68x)
│   └── macintosh/                     #   Macintosh (snow)
├── Toolchain/                         # 크로스 컴파일러
│   ├── MSX/HITECH_TOOLCHAIN/          #   Hi-Tech C Z80
│   └── x68000/toolchain/             #   m68k-xelf GCC
├── Library/                           # 라이브러리 / 프레임워크
│   ├── AppleII/                       #   apple2flat, souless_apple2
│   ├── MSX/                           #   MSX z88dk 가족 + 외부 framework
│   │                                  #   - ubox-msx-lib-z88dk   (ubox z88dk port, DOS2 BIOS fix + Phase 7)
│   │                                  #   - spman-z88dk          (sprite manager z88dk port)
│   │                                  #   - mplayer-z88dk        (Arkos 2 AKM real z88dk port)
│   │                                  #   - mplayer-z88dk-stub   (silent fallback)
│   │                                  #   - ap-z88dk             (aplib decompressor z88dk port)
│   │                                  #   - ubox-msx-lib-1.2.0   (SDCC ubox 원본, reference)
│   │                                  #   - MSXgl/               (★ submodule, AKG/AKM 소스 참조 framework)
│   ├── x68000/                        #   X68KTutorials
│   └── Retro68/                       #   Macintosh 68K/PowerPC 크로스 툴체인
├── RetroDeveloperEnvironmentDisktool/ # 디스크 이미지 조작 도구
├── RetroDeveloperEnvironmentMonitor/  # 에뮬레이터 디버그 모니터
├── Examples/                          # 튜토리얼 / 예제 / 워크스페이스 흡수 프로젝트
│                                      #   - Tutorial_*                       (Hi-Tech / z88dk 튜토리얼)
│                                      #   - prototype_01_* / 02_*            (자체 프로토타입, tracked)
│                                      #   - prototype_05_MSX_ROM_MSXDOS      (MSX z88dk 주요 reference)
│                                      #   - prototype_03+ / 04 / 06+         (local-only)
│                                      #   - kingsvalley_z88dk                (King's Valley z88dk 포팅, 흡수)
│                                      #   - ubox_example_z88dk               (ubox 12 예제 z88dk 포팅, 흡수)
├── diskwork/                          # 디스크 이미지 작업 영역
│   ├── bootdisk/                      #   부팅 가능 시스템 디스크 (플랫폼별)
│   └── emptydisk/                     #   빈/사전 포맷 템플릿 (플랫폼별)
├── resource/                          # 참고 자료 / 레퍼런스 (외부)
│   ├── AppleII/                       #   prodos 등
│   ├── Macintosh/                     #   QuickDraw 원본 + Mac ROM 배치 위치
│   ├── MSX/                           #   kingsvalley, ubox_example, noborunoca, z88dk-msx-template (외부 reference)
│   └── x68000/                        #   기술 자료, 라이브러리
├── resource_extra/                    # 부가 도구 (zxcc 등)
├── specs/                             # 스펙 / 작업 기록
└── tools/                             # 자동화 스크립트
```

## 서브모듈

이 프로젝트는 다음 20개의 git 서브모듈로 구성되어 있습니다.

### Emulator — 에뮬레이터

| 서브모듈 | 설명 |
|----------|------|
| [`Emulator/AppleWin`](https://github.com/onionmixer/AppleWin) | AppleWin Linux 포크. 디버그 HTTP/Telnet 스트리밍 서버를 추가하여 웹 브라우저나 외부 도구에서 에뮬레이터 상태를 실시간 모니터링할 수 있습니다. |
| [`Emulator/openMSX`](https://github.com/onionmixer/openMSX) | openMSX 포크. Debug HTTP Server와 Debug Stream Server 기능을 추가하여 실시간 디버깅을 지원합니다. |
| [`Emulator/x68000/mame`](https://github.com/mamedev/mame) | MAME — 다목적 에뮬레이션 프레임워크. 수십 년간의 소프트웨어 역사를 보존하기 위해 빈티지 컴퓨터, 게임 콘솔, 아케이드 머신 등을 에뮬레이션합니다. |
| [`Emulator/x68000/px68k-onionmixer`](https://github.com/onionmixer/px68k-onionmixer) | Portable X68000 에뮬레이터. SDL2 기반의 Linux 데스크탑 환경에 맞게 개선되었으며, 다중 디스플레이 모드(15kHz/24kHz/31kHz), SCSI 디스크, 시리얼 포트를 지원합니다. |
| [`Emulator/x68000/run68x`](https://github.com/kg68k/run68x) | Human68k CUI 에뮬레이터. run68의 수정 버전으로 디버깅, 메모리 설정, UTF-8 인코딩을 지원하는 커맨드라인 실행 환경입니다. |
| [`Emulator/macintosh/snow`](https://github.com/twvd/snow) | 클래식 Macintosh 에뮬레이터(@v1.4.1, Rust). Mac 128K/Plus/SE/SE FDHD/Classic/II/SE30 등 68K 모델을 GUI/TUI로 에뮬레이트합니다. 기본 모델은 Mac SE FDHD(`MAC_MODEL=se_fdhd`). |

### Toolchain — 크로스 컴파일러

| 서브모듈 | 설명 |
|----------|------|
| [`Toolchain/MSX/HITECH_TOOLCHAIN`](https://github.com/onionmixer/HITECH_TOOLCHAIN) | Hi-Tech C v3.09 Z80 크로스 컴파일러의 Linux 네이티브 툴체인. 컴파일러 도구, 표준 C 라이브러리, MSX 전용 라이브러리를 포함합니다. |

### Library — 라이브러리 / 프레임워크

| 서브모듈 | 설명 |
|----------|------|
| [`Library/AppleII/apple2flat`](https://github.com/bbbradsmith/apple2flat) | CC65 기반 Apple II C/ASM 프레임워크. 제한 없는 메모리 접근, 표준 플로피/오디오 테이프 부팅, 그래픽·입력 유틸리티 라이브러리를 제공합니다. |
| [`Library/AppleII/souless_apple2`](https://github.com/gnos78/souless_apple2) | "Souless" — Apple II용 액션 플랫포머 게임. 다중 스테이지, 보스 전투, HGR 그래픽, Mockingboard/SD-Music 카드 오디오를 지원합니다. |
| [`Library/MSX/MSXgl`](https://github.com/aoineko-fr/MSXgl) | MSXgl SDCC 기반 MSX game library framework. AKG/AKM Arkos 2 player 의 source 출처 (`engine/src/arkos/`). 본 저장소의 z88dk 가족 (mplayer-z88dk 등) 이 이 source 를 z80asm 호환으로 변환해 사용. |
| [`Library/x68000/X68KTutorials`](https://github.com/FedericoTech/X68KTutorials) | X68000 개발 튜토리얼 및 예제 코드 모음. |
| [`Library/Retro68`](https://github.com/autc04/Retro68) | Macintosh 68K / PowerPC 크로스 툴체인(@v2019.8.2-439-g83b9c8d2c5). gcc 12.2 + binutils 2.39 + Rez/Elf2Mac/MakePEF + multiversal 헤더. 빌드는 `Library/Retro68-build/`(out-of-source, gitignore)에 들어갑니다. |

### 자체 도구

| 서브모듈 | 설명 |
|----------|------|
| [`RetroDeveloperEnvironmentDisktool`](https://github.com/onionmixer/RetroDeveloperEnvironmentDisktool) | 크로스 플랫폼 디스크 이미지 조작 도구(rdedisktool). Apple II, MSX, X68000 디스크 이미지의 파일 조작, 포맷 변환, XSA 압축, 디스크 생성을 지원합니다. |
| [`RetroDeveloperEnvironmentMonitor`](https://github.com/onionmixer/RetroDeveloperEnvironmentMonitor) | ncurses 기반 터미널 디버그 모니터(rdemonitor). AppleWin과 openMSX 에뮬레이터의 디버그 출력을 TCP 소켓을 통해 실시간으로 수신하여 4개 탭(Info, I/O, CPU, Memory)으로 표시합니다. |

### Resource — 참고 자료

| 서브모듈 | 설명 |
|----------|------|
| [`resource/Macintosh/QuickDraw`](https://github.com/jrk/QuickDraw) | Apple QuickDraw 원본 소스(@6377ec5, 68K asm + Pascal, 17,101 LOC). PICT 포맷·BitBlt 등 Mac 그래픽 동작 사양 확인용 reference-only(빌드/링크 금지, 라이선스 NOASSERTION). |
| [`resource/x68000/InsideX68000-errata`](https://github.com/kg68k/InsideX68000-errata) | "Inside X68000" 기술 레퍼런스 시리즈의 비공식 정오표. |
| [`resource/x68000/X68KTutorials`](https://github.com/FedericoTech/X68KTutorials) | X68000 개발 튜토리얼 및 예제 코드 모음. |
| [`resource/x68000/x68k_xsp`](https://github.com/yosshin4004/x68k_xsp) | X68000 스프라이트 관리 시스템. 스프라이트 더블러와 캐싱 기법으로 하드웨어 한계(512 스프라이트, 32,768 패턴)를 확장하며 트리플 버퍼링을 지원합니다. |
| [`resource/x68000/xdev68k`](https://github.com/yosshin4004/xdev68k) | GCC 기반 X68000 크로스 컴파일 환경. Linux, Windows(WSL), macOS 등 Unix 호환 호스트에서 X68K 네이티브 실행 파일을 생성할 수 있습니다. |

### Resource Extra — 부가 도구

| 서브모듈 | 설명 |
|----------|------|
| [`resource_extra/zxcc/zxcc-debian`](https://github.com/onionmixer/zxcc-debian) | ZXCC의 Debian/Ubuntu 패키징. John Elliott의 CP/M 2/3 에뮬레이터로, Linux/Unix/macOS에서 Hi-Tech C 등 CP/M 도구를 크로스 실행할 수 있게 합니다. |

## 흡수된 워크스페이스 / 비서브모듈 구성요소

위 19개 git 서브모듈 외에, 본 워크스페이스에는 **자체 git 없이 in-tree 로 흡수된**
다음 components 도 포함됩니다. 각자의 라이선스를 따르며 (§"라이선스" 참조), 일부는
upstream 의 fork 또는 z88dk 호환 port 입니다.

| 위치 | 설명 |
|------|------|
| `Library/MSX/ubox-msx-lib-z88dk/` | ubox library z88dk port. MSX-DOS2 BIOS fix + Phase 7 개선 |
| `Library/MSX/spman-z88dk/` | sprite manager z88dk port |
| `Library/MSX/mplayer-z88dk/` | Arkos 2 AKM player real z88dk port |
| `Library/MSX/mplayer-z88dk-stub/` | silent placeholder (mplayer 회귀 시 fallback) |
| `Library/MSX/ap-z88dk/` | aplib decompressor z88dk port (kingsvalley 등에서 사용) |
| `Library/MSX/ubox-msx-lib-1.2.0/` | ubox SDCC 원본 (reference, 빌드 안 함) |
| `Examples/kingsvalley_z88dk/` | King's Valley z88dk 포팅 (Phase A/B/C 마이그레이션, 자체 git 흡수 후 workspace tracked) |
| `Examples/ubox_example_z88dk/` | ubox 12 예제 z88dk 포팅 (자체 git 흡수, 10/12 ROM+DOS 완료) |
| `resource/MSX/{kingsvalley, ubox_example, noborunoca, z88dk-msx-template}` | 외부 reference 프로젝트 (local clone, 빌드 안 함) |

z88dk 자체 (`/opt/z88dk/bin/zcc`, `sccz80`) 는 본 저장소에 포함되지 않습니다 —
시스템에 별도 설치 (자세한 의존성/설치 가이드는 `REQUIREMENTS.md` §2).

## 빠른 시작

> 본 절은 **핵심 자체 도구 + Macintosh 환경** 최소 빌드 만 다룹니다. 에뮬레이터 빌드
> (AppleWin, openMSX, px68k-onionmixer) 와 z88dk / Hi-Tech C 환경 설정은
> `REQUIREMENTS.md` §3-12 참조.

### 주요 도구 빌드

```bash
# rdedisktool (의존: cmake)
cd RetroDeveloperEnvironmentDisktool
mkdir -p build && cd build
cmake .. && make -j"$(nproc)"

# rdemonitor (의존: libncurses-dev libcjson-dev)
sudo apt install libncurses-dev libcjson-dev
cd ../../RetroDeveloperEnvironmentMonitor
make

# snow Macintosh 에뮬레이터 (Rust 1.95.0 pinned via rust-toolchain.toml)
sudo apt install libasound2-dev pkg-config
# rustup이 없으면: curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain none
cd ../Emulator/macintosh/snow && cargo build --release

# Retro68 Mac 크로스 툴체인 (out-of-source 빌드, ~30-60분)
sudo apt install cmake libgmp-dev libmpfr-dev libmpc-dev libboost-all-dev bison flex texinfo ruby
mkdir -p ../../../Library/Retro68-build && cd ../../../Library/Retro68-build
../Retro68/build-toolchain.bash --no-ppc
```

### z88dk MSX 환경 (별도 설치 후)

```bash
# z88dk 가 /opt/z88dk 에 설치된 경우 (REQUIREMENTS.md §2 참조)
export PATH=/opt/z88dk/bin:$PATH
export ZCCCFG=/opt/z88dk/lib/config

# 검증
zcc --version    # sccz80 version 출력
```

### 플랫폼별 실행

```bash
./run_applewin_dos33.sh          # Apple II — DOS 3.3
./run_applewin_prodos.sh         # Apple II — ProDOS
./run_openmsx_msxdos2.sh         # MSX — MSX-DOS2
./run_openmsx_rom.sh             # MSX — ROM
./run_px68k_humanos.sh           # X68000 — Human68k
./run_snow_mac.sh                # Macintosh — System 6/7 (snow, 기본 SE FDHD)
```

> Macintosh ROM(저작권)은 사용자가 직접 `resource/Macintosh/rom/`에 배치해야 합니다 — 자세한 배치 컨벤션은 `resource/Macintosh/rom/README.md` 참고.

## 문서

| 문서 | 내용 |
|------|------|
| [`DEVELOPER_COMMON_HOWTO.md`](DEVELOPER_COMMON_HOWTO.md) | 공통 개발 가이드 |
| [`DEVELOPER_AppleII_HOWTO.md`](DEVELOPER_AppleII_HOWTO.md) | Apple II 개발 가이드 |
| [`DEVELOPER_MSX_HOWTO.md`](DEVELOPER_MSX_HOWTO.md) | MSX 개발 가이드 |
| [`DEVELOPER_X68000_HOWTO.md`](DEVELOPER_X68000_HOWTO.md) | X68000 개발 가이드 |
| [`DEVELOPER_MacintoshOld_HOWTO.md`](DEVELOPER_MacintoshOld_HOWTO.md) | 클래식 Macintosh(System 6/7, 68K) 개발 가이드 — Retro68 → rdedisktool → snow 흐름 |
| [`FLOPPY_IMAGE_OPERATION.md`](FLOPPY_IMAGE_OPERATION.md) | 디스크 이미지 작업 가이드 |
| [`REQUIREMENTS.md`](REQUIREMENTS.md) | 환경 구축 및 요구사항 |

## 라이선스

본 프로젝트의 **워크스페이스 자체 구성요소** (스크립트, 문서, 자체 도구
`rdedisktool` / `rdemonitor`, 워크스페이스 glue 코드) 는 [MIT License](LICENSE)
로 배포됩니다.

⚠️ **그 외 in-tree components 는 각자의 라이선스를 따릅니다** — 사용·수정·재배포 시
반드시 해당 디렉터리의 `LICENSE` / `COPYING` / `README` 를 확인하세요. 크게 두 부류:

### 서브모듈 (19개) 의 주요 라이선스

| 서브모듈 | 라이선스 |
|----------|---------|
| `Library/Retro68` | GPL3+ (libretro 런타임은 GCC 런타임 예외 포함) |
| `Emulator/macintosh/snow` | MIT |
| `Emulator/openMSX` (fork) | GPL-2.0+ |
| `Emulator/AppleWin` (fork) | GPL-2.0+ |
| `Emulator/x68000/mame` | BSD-3-Clause / GPL-2.0+ (구성 요소별) |
| `Library/MSX/MSXgl` | **CC BY-SA 4.0** (upstream Aoineko-Studio/MSXgl) |
| `resource/Macintosh/QuickDraw` | NOASSERTION (Apple 원본, license file 없음 — reference-only 권장) |
| 그 외 | 각 서브모듈의 LICENSE 파일 참조 |

### 흡수된 in-tree workspace 의 라이선스

| 위치 | 라이선스 | 비고 |
|------|---------|------|
| `Examples/kingsvalley_z88dk/` | MIT (LICENSE file 참조) | upstream pdpdds/kingsvalley fork |
| `Examples/ubox_example_z88dk/` | **GPL-2.0** | upstream pdpdds/ubox_example fork — z88dk port 작업 결과 포함 |
| `Library/MSX/ubox-msx-lib-1.2.0/` | MIT | Juan J. Martinez 의 ubox SDCC 원본 |
| `Library/MSX/ubox-msx-lib-z88dk/` | MIT (ubox 1.2.0 상속) | z88dk port |
| `Library/MSX/{spman, mplayer, ap}-z88dk/` | MIT 또는 upstream 상속 | 각 디렉터리 README 참조 |
| `resource/MSX/{kingsvalley, ubox_example, noborunoca, z88dk-msx-template}` | 각 upstream | reference-only, 빌드/링크 안 함 |

### 본 저장소에 포함되지 않는 자료 (사용자가 직접 배치)

저작권상 다음 자료는 본 저장소에 포함되지 않습니다 — 사용자가 합법적인 경로로
확보해 각 디렉터리의 README 가 안내하는 위치에 배치해야 합니다.

- Macintosh ROM (`resource/Macintosh/rom/`)
- 클래식 Mac System 6/7 부팅 디스크 (`diskwork/bootdisk/macintosh/`)
- MPW Universal Interfaces (필요 시 `Library/Retro68/InterfacesAndLibraries/`)
- 그 외 각 플랫폼의 BIOS / ROM / System 자료
