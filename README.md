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
│   ├── x68000/                        #   X68KTutorials
│   └── Retro68/                       #   Macintosh 68K/PowerPC 크로스 툴체인
├── RetroDeveloperEnvironmentDisktool/ # 디스크 이미지 조작 도구
├── RetroDeveloperEnvironmentMonitor/  # 에뮬레이터 디버그 모니터
├── Examples/                          # 튜토리얼 / 예제 (Tutorial_*, prototype_*)
├── diskwork/                          # 디스크 이미지 작업 영역
│   ├── bootdisk/                      #   부팅 가능 시스템 디스크 (플랫폼별)
│   └── emptydisk/                     #   빈/사전 포맷 템플릿 (플랫폼별)
├── resource/                          # 참고 자료 / 레퍼런스
│   ├── Macintosh/                     #   QuickDraw 원본 + Mac ROM 배치 위치
│   └── x68000/                        #   기술 자료, 라이브러리
├── resource_extra/                    # 부가 도구 (zxcc 등)
├── specs/                             # 스펙 / 작업 기록
└── tools/                             # 자동화 스크립트
```

## 서브모듈

이 프로젝트는 다음 19개의 git 서브모듈로 구성되어 있습니다.

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

## 빠른 시작

### 주요 도구 빌드

```bash
# rdedisktool
cd RetroDeveloperEnvironmentDisktool
mkdir -p build && cd build
cmake .. && make -j"$(nproc)"

# rdemonitor
cd ../../RetroDeveloperEnvironmentMonitor
make

# snow Macintosh 에뮬레이터 (Rust 1.95.0 pinned via rust-toolchain.toml)
sudo apt install libasound2-dev pkg-config
# rustup이 없으면: curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain none
cd ../Emulator/macintosh/snow && cargo build --release

# Retro68 Mac 크로스 툴체인 (out-of-source 빌드, ~20-30분)
sudo apt install cmake libgmp-dev libmpfr-dev libmpc-dev libboost-all-dev bison flex texinfo ruby
mkdir -p ../../../Library/Retro68-build && cd ../../../Library/Retro68-build
../Retro68/build-toolchain.bash --no-ppc
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

이 프로젝트의 자체 구성요소(워크스페이스 스크립트, 문서, 자체 도구 `rdedisktool` / `rdemonitor` 등)는 [MIT License](LICENSE)로 배포됩니다.

**각 서브모듈은 각자의 라이선스를 따릅니다.** 사용·수정·재배포 시 반드시 해당 서브모듈의 `LICENSE` / `COPYING` / `README` 파일을 확인하세요. 주요 사례:

| 서브모듈 | 라이선스 |
|----------|---------|
| `Library/Retro68` | GPL3+ (libretro 런타임은 GCC 런타임 예외 포함) |
| `Emulator/macintosh/snow` | MIT |
| `Emulator/x68000/mame` | BSD-3-Clause / GPL-2.0+ (구성 요소별) |
| `resource/Macintosh/QuickDraw` | Apple 원본 (NOASSERTION, 재배포·파생물 제한) |
| 그 외 | 각 서브모듈의 LICENSE 파일 참조 |

또한 다음 자료는 저작권상 본 저장소에 **포함되지 않으며**, 사용자가 합법적인 경로로 직접 확보해 각 디렉터리의 README가 안내하는 위치에 배치해야 합니다.

- Macintosh ROM (`resource/Macintosh/rom/`)
- 클래식 Mac System 6/7 부팅 디스크 (`diskwork/bootdisk/macintosh/`)
- MPW Universal Interfaces (필요 시 `Library/Retro68/InterfacesAndLibraries/`)
- 그 외 각 플랫폼의 BIOS/ROM/System 자료
