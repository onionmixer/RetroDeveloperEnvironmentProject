# DEVELOPER X68000 HOWTO

이 문서는 저장소의 X68000 개발 환경을 기준으로 개발/컴파일/실행 절차를 정리합니다.

주요 경로는 2가지입니다.

1. Human68k 실기/GUI 에뮬레이션 경로 (`px68k-onionmixer`)
2. 빠른 코드 검증 경로 (`run68`)

---

## 0. 공통 개요

- X68000 작업 루트:
  - `Emulator/x68000`
- 크로스 툴체인(내장):
  - `Toolchain/x68000/toolchain/m68k-xelf`
- 튜토리얼 리소스:
  - `Toolchain/x68000/X68KTutorials`
- 예제 소스:
  - `Emulator/x68000/src/*.c`
- 빌드 산출물:
  - `Emulator/x68000/build/*.x`
- 디스크 도구:
  - `RetroDeveloperEnvironmentDisktool/build/rdedisktool`

핵심 원칙:
- 빌드 자체는 `m68k-xelf-gcc`로 진행합니다.
- 하드웨어 의존 기능(그래픽/사운드/입력/CRTC/DMA)은 `px68k` 같은 GUI 에뮬레이터에서 검증합니다.
- 단순 C/DOS 기능은 `run68`으로 빠르게 1차 검증 가능합니다.

---

## 1. 경로 A: Human68k + px68k-onionmixer

### 1.1 대상

- 실행 환경:
  - `Emulator/x68000/px68k-onionmixer/px68k-onionmixer`
- 부팅 디스크 기본값:
  - `Emulator/x68000/px68k-onionmixer/HUMAN302.XDF`
- ROM 파일:
  - `Emulator/x68000/px68k-onionmixer/iplrom.dat`
  - `Emulator/x68000/px68k-onionmixer/cgrom.dat`

### 1.2 빌드

프로젝트 루트에서:
```bash
cd Emulator/x68000
export PATH="$(pwd)/../../Toolchain/x68000/toolchain/m68k-xelf/bin:$PATH"
make
```

개별 예제 빌드:
```bash
cd Emulator/x68000
export PATH="$(pwd)/../../Toolchain/x68000/toolchain/m68k-xelf/bin:$PATH"
make build/hello.x
make build/hello_dos.x
make build/hello_file.x
```

CPU 타겟 변경 예시:
```bash
cd Emulator/x68000
export PATH="$(pwd)/../../Toolchain/x68000/toolchain/m68k-xelf/bin:$PATH"
make CPU=68030
```

### 1.3 실행 (px68k)

권장 실행 스크립트:
```bash
./run_px68k_humanos.sh
```

직접 실행:
```bash
./Emulator/x68000/px68k-onionmixer/px68k-onionmixer \
  --iplrom ./Emulator/x68000/px68k-onionmixer/iplrom.dat \
  --cgrom ./Emulator/x68000/px68k-onionmixer/cgrom.dat \
  --fdd0  ./Emulator/x68000/px68k-onionmixer/HUMAN302.XDF
```

### 1.4 검증 대상 예시

- `hello.x`, `hello_dos.x`, `hello_file.x`: 기본 기능/파일 I/O
- `hello_gfx.x`, `hello_sprite.x`, `hello_bg.x`, `hello_crtc.x`: 그래픽/CRTC
- `hello_snd.x`, `hello_adpcm.x`, `hello_midi.x`: 사운드
- `hello_key.x`, `hello_mouse.x`, `hello_joy.x`: 입력 장치

참고:
- 하드웨어 의존 예제는 `run68`에서 완전 동작하지 않을 수 있습니다.

---

## 2. 경로 B: run68 빠른 검증

### 2.1 대상

- 목적: 컴파일 결과의 빠른 1차 동작 확인
- 사용 명령:
  - `make run`
  - `make run-<name>`

### 2.2 실행 예시

```bash
cd Emulator/x68000
export PATH="$(pwd)/../../Toolchain/x68000/toolchain/m68k-xelf/bin:$PATH"
make run              # hello.x 빌드 + run68 실행
make run-hello_dos    # hello_dos.x 실행
make run-hello_file   # hello_file.x 실행
```

### 2.3 제한

- `run68`은 콘솔 중심 검증에 적합합니다.
- 그래픽/사운드/스프라이트/마우스/조이스틱/CRTC/DMA는 GUI 에뮬레이터에서 재검증해야 합니다.

---

## 3. 디스크 이미지 작업 (rdedisktool)

`rdedisktool`은 X68000 포맷 `.xdf`, `.dim`을 지원합니다.

도움말 확인:
```bash
./RetroDeveloperEnvironmentDisktool/build/rdedisktool --help
```

### 3.1 포맷 정보 확인

```bash
./RetroDeveloperEnvironmentDisktool/build/rdedisktool info ./Emulator/x68000/work.xdf
```

### 3.2 포맷 변환 예시

```bash
./RetroDeveloperEnvironmentDisktool/build/rdedisktool convert ./Emulator/x68000/work.xdf /tmp/work.dim -f dim
./RetroDeveloperEnvironmentDisktool/build/rdedisktool convert /tmp/work.dim /tmp/work_back.xdf -f xdf
```

### 3.3 생성 관련 주의사항

현재 환경 실측 기준으로 `create -f xdf/dim`은 파일 생성 자체는 가능하지만,
파일시스템이 `Unknown`으로 표시될 수 있습니다.

예시:
```bash
./RetroDeveloperEnvironmentDisktool/build/rdedisktool create /tmp/newdisk.xdf -f xdf --force
./RetroDeveloperEnvironmentDisktool/build/rdedisktool info /tmp/newdisk.xdf
```

권장:
- 실제 Human68k 실행용 디스크는 이미 준비된 부팅/작업 디스크(`HUMAN302.XDF`, `work.xdf`)를 기준으로 운용합니다.
- 빈 이미지 생성 직후 파일 추가가 실패하면(`Not enough space`) 파일시스템/초기화 상태를 먼저 확인합니다.

---

## 4. run_px68k_humanos.sh 사용법

이 스크립트는 X68000 Human68k 부팅을 위한 기본 런처입니다.

### 4.1 실행

```bash
./run_px68k_humanos.sh
```

환경변수 오버라이드 예시:
```bash
PX68K=./Emulator/x68000/px68k-onionmixer/px68k-onionmixer \
BOOT_DISK=./Emulator/x68000/work.xdf \
FDD1_DISK=/tmp/data_disk.xdf \
./run_px68k_humanos.sh
```

우선순위:
1. 환경변수(`PX68K`, `IPL_ROM`, `CG_ROM`, `BOOT_DISK`, `FDD1_DISK`)가 있으면 그 값을 사용
2. 없으면 스크립트가 `Emulator/x68000`와 `Toolchain/x68000` 후보 경로를 자동 탐색

### 4.2 내부에서 확인하는 경로

- 에뮬레이터 바이너리:
  - `Emulator/x68000/px68k-onionmixer/px68k-onionmixer`
- IPL ROM:
  - `Emulator/x68000/px68k-onionmixer/iplrom.dat`
- CG ROM:
  - `Emulator/x68000/px68k-onionmixer/cgrom.dat`
- 부팅 디스크(FDD0):
  - `Emulator/x68000/px68k-onionmixer/HUMAN302.XDF`
- 선택 디스크(FDD1, 옵션):
  - `FDD1_DISK` 환경변수로 주입 가능

### 4.3 자주 발생하는 오류

- `px68k-onionmixer not found`:
  - px68k 빌드 결과 파일 존재 여부 확인
- `IPL ROM not found`:
  - `iplrom.dat` 위치 확인
- `CG ROM not found`:
  - `cgrom.dat` 위치 확인
- `Boot disk not found`:
  - `HUMAN302.XDF` 위치 확인

### 4.4 실행 중 단축키

- `F12`: 메뉴(디스크 교체/설정)
- `F11`: 전체화면 토글

---

## 5. 빠른 시작

### 5.1 빌드 + run68 검증

```bash
cd Emulator/x68000
export PATH="$(pwd)/../../Toolchain/x68000/toolchain/m68k-xelf/bin:$PATH"
make info
make build/hello.x
make run-hello
```

### 5.2 px68k Human68k 부팅

```bash
cd /mnt/USERS/onion/DATA_ORIGN/Workspace/05_RetroDeveloperEnvironmentProject
./run_px68k_humanos.sh
```

### 5.3 Tutorial_x68000_01 (권장 입문)

`Examples/Tutorial_x68000_01`은 run68에서 바로 검증 가능한 최소 튜토리얼입니다.

```bash
cd Examples/Tutorial_x68000_01
./compile.sh all
```

2026-02-22 실측 기준 결과:
- 빌드 성공:
  - `build/hello.x`
  - `build/hello.x.elf`
- 실행 로그 생성:
  - `build/run68_output.log`
- 로그 검증 문자열:
  - `Hello, X68000 Tutorial!`
  - `Current drive: A:\`

참고(실측 동작):
- run68은 경로에 따라 다음 경고를 출력할 수 있으나 실행 자체는 정상입니다.
  - `PSP内の実行ファイル名を"A:\\PROG.X"に変更します。`
- `compile.sh`는 위 문자열 검증까지 통과해야 성공으로 판단합니다.

GUI 에뮬레이터(px68k) 연동 안내:
```bash
cd Examples/Tutorial_x68000_01
./compile.sh run
```

---

## 6. 표준 테스트 시나리오 (이관)

기존 통합 테스트 시나리오 문서에는 X68000 전용 섹션이 없었으므로, 본 문서에 X68000 표준 검증 템플릿을 정의합니다.

| 단계 | 작업 | 기준 명령/결과 |
|------|------|----------------|
| 0 | 문서 확인 | `DEVELOPER_COMMON_HOWTO.md`, `FLOPPY_IMAGE_OPERATION.md` |
| 1 | 타겟 선택 | X68000 Human68k (`px68k`) 또는 `run68` |
| 2 | 툴체인 설정 | `export PATH=.../Toolchain/x68000/toolchain/m68k-xelf/bin:$PATH` |
| 3 | 빌드/검증 | `cd Examples/Tutorial_x68000_01 && ./compile.sh all` |
| 4 | 디스크 준비(필요 시) | `rdedisktool info/convert`로 `.xdf/.dim` 확인 |
| 5 | 실행 | `./run_px68k_humanos.sh` 또는 `make run-*` |
| 6 | 런타임 확인 | `build/run68_output.log` 문자열 확인 + 필요 시 px68k 수동 검증 |

---

## 7. 관련 문서

- `Emulator/x68000/README.md`
- `Emulator/x68000/EXAMPLE_SUMMARY.md`
- `Emulator/x68000/px68k-onionmixer/README.md`
- `Toolchain/x68000/toolchain/m68k-xelf/README`
- `run_px68k_humanos.sh`
- `RetroDeveloperEnvironmentDisktool/TESTCASE.md`
- `FLOPPY_IMAGE_OPERATION.md`
