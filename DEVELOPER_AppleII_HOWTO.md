# DEVELOPER Apple II HOWTO

이 문서는 이 저장소에서 Apple II 개발을 진행하는 두 가지 경로를 정리합니다.

1. DOS 3.3 경로 (`.do/.dsk`)
2. ProDOS 경로 (`.po`)

각 경로는 디스크 포맷, 실행 명령, 디렉터리 정책이 다르므로 섞지 않고 독립적으로 사용합니다.

---

## 0. 공통 개요

- 프로젝트 루트:
  - `05_RetroDeveloperEnvironmentProject`
- Apple II 에뮬레이터:
  - `Emulator/AppleWin/build/sa2`
- 디스크 조작 도구:
  - `RetroDeveloperEnvironmentDisktool/build/rdedisktool`
- C 컴파일러:
  - `cl65` (cc65)

핵심 원칙:
- 컴파일 결과(`HELLO`)는 AppleSingle 헤더를 포함하므로, 디스크에 넣기 전 `HELLO_RAW`로 헤더 제거가 필요합니다.
- `rdedisktool add` 시 Apple II 바이너리는 보통 `--type B --addr 0x0803`를 지정합니다.

---

## 1. 경로 A: Apple II DOS 3.3

### 1.1 대상

- 파일 시스템: DOS 3.3
- 프로그램 디스크 확장자: `.do` 또는 `.dsk`
- 부팅 디스크:
  - `diskwork/bootdisk/AppleII/dos33.dsk`

### 1.2 실행 스크립트

- 기본 실행 스크립트:
  - `./run_applewin_dos33.sh`
- 내부 기본 장착:
  - D1: `diskwork/bootdisk/AppleII/dos33.dsk`
  - D2: `Examples/Tutorial_apple_dos33_01/Tutorial_apple_dos33_01.do`

### 1.3 예제 개발 절차

작업 디렉터리:
```bash
mkdir -p Examples/Tutorial_apple_dos33_01
cd Examples/Tutorial_apple_dos33_01
```

소스(`hello.c`):
```c
#include <stdio.h>
#include <conio.h>

int main(void)
{
    clrscr();
    printf("hello world\n");
    printf("\nPress any key to exit...\n");
    cgetc();
    return 0;
}
```

컴파일:
```bash
cl65 -t apple2 -o HELLO hello.c
```

AppleSingle 헤더 제거(58바이트):
```bash
dd if=HELLO of=HELLO_RAW bs=1 skip=58
```

디스크 생성:
```bash
../../RetroDeveloperEnvironmentDisktool/build/rdedisktool \
  create Tutorial_apple_dos33_01.do -f do --fs dos33
```

바이너리 추가:
```bash
../../RetroDeveloperEnvironmentDisktool/build/rdedisktool \
  add Tutorial_apple_dos33_01.do ./HELLO_RAW HELLO --type B --addr 0x0803
```

### 1.4 에뮬레이터 실행

프로젝트 루트에서:
```bash
./run_applewin_dos33.sh
```

또는 직접 실행:
```bash
./Emulator/AppleWin/build/sa2 \
  --d1 ./diskwork/bootdisk/AppleII/dos33.dsk \
  --d2 ./Examples/Tutorial_apple_dos33_01/Tutorial_apple_dos33_01.do
```

에뮬레이터 내 명령:
```text
CATALOG,D2
BRUN HELLO,D2
```

### 1.5 주의사항

- DOS 3.3 실행은 드라이브 지정이 중요하므로 `,D2`를 붙여 실행하는 습관을 권장합니다.
- `--addr`를 빠뜨리면 실행 주소가 어긋나 프로그램이 동작하지 않을 수 있습니다.

---

## 2. 경로 B: Apple II ProDOS

### 2.1 대상

- 파일 시스템: ProDOS
- 프로그램 디스크 확장자: `.po`
- 부팅 디스크(기본):
  - `diskwork/bootdisk/AppleII/prodos242.dsk`

### 2.2 실행 스크립트

- 기본 실행 스크립트:
  - `./run_applewin_prodos.sh`
- 내부 기본 장착:
  - D1: `diskwork/bootdisk/AppleII/prodos242.dsk`
  - D2: `Examples/Tutorial_apple_prodos_01/Tutorial_apple_prodos_01.po`

### 2.3 예제 개발 절차

작업 디렉터리:
```bash
mkdir -p Examples/Tutorial_apple_prodos_01
cd Examples/Tutorial_apple_prodos_01
```

소스(`hello.c`):
```c
#include <stdio.h>
#include <conio.h>

int main(void)
{
    clrscr();
    printf("hello world\n");
    printf("\nPress any key to exit...\n");
    cgetc();
    return 0;
}
```

컴파일:
```bash
cl65 -t apple2 -o HELLO hello.c
```

AppleSingle 헤더 제거:
```bash
dd if=HELLO of=HELLO_RAW bs=1 skip=58
```

디스크 생성(볼륨명 지정):
```bash
../../RetroDeveloperEnvironmentDisktool/build/rdedisktool \
  create Tutorial_apple_prodos_01.po -f po --fs prodos -n TUTORIAL01
```

바이너리 추가:
```bash
../../RetroDeveloperEnvironmentDisktool/build/rdedisktool \
  add Tutorial_apple_prodos_01.po ./HELLO_RAW HELLO --type B --addr 0x0803
```

### 2.4 에뮬레이터 실행

프로젝트 루트에서:
```bash
./run_applewin_prodos.sh
```

또는 직접 실행:
```bash
./Emulator/AppleWin/build/sa2 \
  --d1 ./diskwork/bootdisk/AppleII/prodos242.dsk \
  --d2 ./Examples/Tutorial_apple_prodos_01/Tutorial_apple_prodos_01.po
```

에뮬레이터 내 명령:
```text
CAT,D2
PREFIX,D2
-HELLO
```

대체 실행:
```text
BRUN /D2/HELLO
```

### 2.5 주의사항

- ProDOS는 서브디렉터리를 지원하므로 파일 경로 기반 운영이 가능합니다.
- 실행 명령은 DOS 3.3과 다를 수 있으므로 `PREFIX` 설정 여부를 먼저 확인합니다.

---

## 3. 디스크 조작 빠른 참조

디스크 생성:
```bash
# DOS 3.3
rdedisktool create mydisk.do -f do --fs dos33

# ProDOS
rdedisktool create mydisk.po -f po --fs prodos -n MYDISK
```

파일 추가:
```bash
rdedisktool add mydisk.do ./HELLO_RAW HELLO --type B --addr 0x0803
rdedisktool add mydisk.po ./HELLO_RAW HELLO --type B --addr 0x0803
```

검증:
```bash
rdedisktool list mydisk.do
rdedisktool list mydisk.po
rdedisktool info mydisk.do -v
rdedisktool info mydisk.po -v
```

---

## 4. run_applewin_* 스크립트 사용법

### 4.1 run_applewin_dos33.sh

실행:
```bash
./run_applewin_dos33.sh
```

환경변수 오버라이드:
- `APPLEWIN`, `BOOT_DISK`, `PROGRAM_DISK`

우선순위:
1. 환경변수 지정값
2. 스크립트 기본 후보 경로 자동 탐색

체크 항목:
- `Emulator/AppleWin/build/sa2` 존재
- `diskwork/bootdisk/AppleII/dos33.dsk` 존재
- `Examples/Tutorial_apple_dos33_01/Tutorial_apple_dos33_01.do` 존재

### 4.2 run_applewin_prodos.sh

실행:
```bash
./run_applewin_prodos.sh
```

환경변수 오버라이드:
- `APPLEWIN`, `BOOT_DISK`, `PROGRAM_DISK`

우선순위:
1. 환경변수 지정값
2. 스크립트 기본 후보 경로 자동 탐색

체크 항목:
- `Emulator/AppleWin/build/sa2` 존재
- `diskwork/bootdisk/AppleII/prodos242.dsk` 존재
- `Examples/Tutorial_apple_prodos_01/Tutorial_apple_prodos_01.po` 존재

### 4.3 자주 발생하는 오류

- `AppleWin not found`:
  - AppleWin 빌드 경로 확인 (`Emulator/AppleWin/build/sa2`)
- `Boot disk not found`:
  - bootdisk 경로 확인
- `Program disk not found`:
  - 튜토리얼 디스크 생성/파일명 확인

---

## 5. 빠른 시작

### 5.1 DOS 3.3
```bash
mkdir -p Examples/Tutorial_apple_dos33_01
cd Examples/Tutorial_apple_dos33_01
cat > hello.c << 'END'
#include <stdio.h>
int main(void){ printf("hello world\n"); return 0; }
END
cl65 -t apple2 -o HELLO hello.c
dd if=HELLO of=HELLO_RAW bs=1 skip=58
../../RetroDeveloperEnvironmentDisktool/build/rdedisktool create Tutorial_apple_dos33_01.do -f do --fs dos33
../../RetroDeveloperEnvironmentDisktool/build/rdedisktool add Tutorial_apple_dos33_01.do ./HELLO_RAW HELLO --type B --addr 0x0803
cd ../../
./run_applewin_dos33.sh
```

### 5.2 ProDOS
```bash
mkdir -p Examples/Tutorial_apple_prodos_01
cd Examples/Tutorial_apple_prodos_01
cat > hello.c << 'END'
#include <stdio.h>
int main(void){ printf("hello world\n"); return 0; }
END
cl65 -t apple2 -o HELLO hello.c
dd if=HELLO of=HELLO_RAW bs=1 skip=58
../../RetroDeveloperEnvironmentDisktool/build/rdedisktool create Tutorial_apple_prodos_01.po -f po --fs prodos -n TUTORIAL01
../../RetroDeveloperEnvironmentDisktool/build/rdedisktool add Tutorial_apple_prodos_01.po ./HELLO_RAW HELLO --type B --addr 0x0803
cd ../../
./run_applewin_prodos.sh
```

---

## 6. 표준 테스트 시나리오 (이관)

### 6.1 DOS 3.3 체크리스트

| 단계 | 작업 | 기준 명령/결과 |
|------|------|----------------|
| 0 | 문서 확인 | `DEVELOPER_COMMON_HOWTO.md`, `FLOPPY_IMAGE_OPERATION.md` |
| 1 | 대상 선택 | Apple II DOS 3.3 |
| 2 | 작업 폴더 | `Examples/Tutorial_apple_dos33_01` |
| 3 | 컴파일 | `cl65 -t apple2 -o HELLO hello.c` + `dd ... skip=58` |
| 4 | 디스크 생성 | `rdedisktool create ... -f do --fs dos33` |
| 5 | 바이너리 추가 | `rdedisktool add ... --type B --addr 0x0803` |
| 6 | 에뮬레이터 실행 | `./run_applewin_dos33.sh` |
| 7 | 런타임 확인 | `CATALOG,D2` 후 `BRUN HELLO,D2` |

### 6.2 ProDOS 체크리스트

| 단계 | 작업 | 기준 명령/결과 |
|------|------|----------------|
| 0 | 문서 확인 | `DEVELOPER_COMMON_HOWTO.md`, `FLOPPY_IMAGE_OPERATION.md` |
| 1 | 대상 선택 | Apple II ProDOS |
| 2 | 작업 폴더 | `Examples/Tutorial_apple_prodos_01` |
| 3 | 컴파일 | `cl65 -t apple2 -o HELLO hello.c` + `dd ... skip=58` |
| 4 | 디스크 생성 | `rdedisktool create ... -f po --fs prodos -n TUTORIAL01` |
| 5 | 바이너리 추가 | `rdedisktool add ... --type B --addr 0x0803` |
| 6 | 에뮬레이터 실행 | `./run_applewin_prodos.sh` |
| 7 | 런타임 확인 | `CAT,D2`, `PREFIX,D2`, `-HELLO` |

### 6.3 DOS 3.3 / ProDOS 실행 차이

| 항목 | DOS 3.3 | ProDOS |
|------|---------|--------|
| 확장자 | `.do`, `.dsk` | `.po` |
| 실행 명령 | `BRUN HELLO,D2` | `-HELLO` 또는 `BRUN /D2/HELLO` |
| 디렉터리 | 미지원 | 지원 |

---

## 7. Apple2Flat 프레임워크 분석 요약

경로: `Library/AppleII/apple2flat`

### 7.1 성격

- `cc65` 기반 Apple II C/ASM 프레임워크입니다.
- 상위 목표:
  - DOS/BASIC 의존성 최소화
  - 16-sector 디스크/테이프 부팅 경로 제공
  - 텍스트/그래픽/사운드/입력 유틸리티 제공
- 원문 `readme.md` 기준으로 아직 일반 사용 준비 전(Work in progress) 상태입니다.

### 7.2 현재 저장소 상태에서 확인된 핵심 포인트

- `a2f/`, `a2f_cc65/` 라이브러리 소스와 데모 소스(`a2f_demo.c`)가 포함되어 있습니다.
- 기본 `make` 타겟은 `disk`이며, 산출물은 `temp/`에 생성됩니다.
  - `temp/a2f_demo.dsk`, `temp/a2f_demo.sym`
  - `make tape` 시 `temp/a2f_demo.wav`
- 라이브러리 산출물:
  - `temp/a2f_disk.lib`
  - `temp/a2f_tape.lib`
  - `temp/a2f_cc65.lib`

### 7.3 빌드 전 주의사항 (중요)

- `makefile.common`이 기본적으로 내부 경로 `./cc65/bin/{cc65,ca65,ld65,ar65}`를 기대합니다.
- 현재 저장소에는 `Library/AppleII/apple2flat/cc65/` 디렉터리가 없으므로, 기본값 그대로는 빌드가 실패합니다.

Linux에서 외부 cc65를 사용하는 권장 예시:

```bash
cd Library/AppleII/apple2flat
make \
  CC65="$(command -v cc65)" \
  CA65="$(command -v ca65)" \
  LD65="$(command -v ld65)" \
  AR65="$(command -v ar65)"
```

테이프 타겟:

```bash
cd Library/AppleII/apple2flat
make tape \
  CC65="$(command -v cc65)" \
  CA65="$(command -v ca65)" \
  LD65="$(command -v ld65)" \
  AR65="$(command -v ar65)"
```

### 7.4 API 범주(헤더 기준)

`a2f.h`에 다음 범주의 API/상수가 정의되어 있습니다.

- 시스템 감지/ASSERT/BRK (`system_type`, `FATAL`, `ASSERT`)
- 키보드 입력 (`kb_get`, `kb_new`, `kb_field`)
- 패들 입력 (`paddle0_poll`, `paddleb_poll`)
- 사운드/뮤직 (`sound_*`, `music_*`)
- 디스크 읽기 (`disk_read`, `disk_error`)
- 비디오 모드/색상 상수 및 드로잉 지원

실무 적용 시 권장:
- 본 저장소 기본 튜토리얼 경로는 계속 `cc65 + rdedisktool + run_applewin_*`를 기준으로 유지
- `apple2flat`은 별도 실험 트랙으로 분리해 검증 로그와 함께 사용

---

## 8. 관련 문서

- `DEVELOPER_MSX_HOWTO.md`
- `DEVELOPER_COMMON_HOWTO.md`
- `FLOPPY_IMAGE_OPERATION.md`
- `RetroDeveloperEnvironmentProject_REQUIREMENTS.md`
- `Library/AppleII/apple2flat/readme.md`
