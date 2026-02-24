# X68000 Human68k Hello World 개발 계획

## 개요

SHARP X68000 컴퓨터의 Human68k OS용 Hello World 프로그램을 GCC 크로스 컴파일 환경에서 개발합니다.

## 개발 환경 선택

X68000용 GCC 크로스 컴파일러는 두 가지 주요 프로젝트가 있습니다:

| 프로젝트 | 특징 | 권장 대상 |
|---------|------|----------|
| [elf2x68k](https://github.com/yunkya2/elf2x68k) | 최신 GCC 13.4.0, 사전 빌드 바이너리 제공, Newlib 지원 | **Linux 사용자 (권장)** |
| [xdev68k](https://github.com/yosshin4004/xdev68k) | 소스 빌드, HAS060/HLK 링커 사용, 상세한 문서 | Windows/MSYS2 사용자 |

본 프로젝트에서는 **elf2x68k**를 사용합니다.

---

## 1. 크로스 컴파일러 설치 (elf2x68k)

### 1.1 사전 요구사항

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential git wget unzip

# 선택사항: 소스 빌드시 필요
sudo apt install texinfo libgmp-dev libmpfr-dev libmpc-dev
```

### 1.2 사전 빌드된 바이너리 설치 (권장)

```bash
# 1. 릴리스 페이지에서 Linux용 아카이브 다운로드
# https://github.com/yunkya2/elf2x68k/releases

# 2. 원하는 디렉토리에 압축 해제
mkdir -p ~/x68k-toolchain
cd ~/x68k-toolchain
wget https://github.com/yunkya2/elf2x68k/releases/latest/download/m68k-xelf-linux-x86_64.tar.gz
tar xzf m68k-xelf-linux-x86_64.tar.gz

# 3. 환경 변수 설정 (~/.bashrc 또는 ~/.zshrc에 추가)
export XELF_BASE=~/x68k-toolchain/m68k-xelf
export PATH=$XELF_BASE/bin:$PATH

# 4. 설정 적용
source ~/.bashrc
```

> 저장소 기본 경로 기준 사용 시:
> `XELF_BASE=/mnt/USERS/onion/DATA_ORIGN/Workspace/05_RetroDeveloperEnvironmentProject/Toolchain/x68000/toolchain/m68k-xelf`

### 1.3 XC 라이브러리 설치 (선택사항)

SHARP C Compiler PRO-68K v2.1 호환 라이브러리가 필요한 경우:

```bash
cd $XELF_BASE
./install-xclib.sh
```

### 1.4 설치 확인

```bash
m68k-xelf-gcc --version
# 출력 예: m68k-xelf-gcc (GCC) 13.4.0
```

---

## 2. 소스에서 빌드 (대안)

사전 빌드 바이너리 대신 소스에서 빌드하려는 경우:

```bash
# 1. 저장소 클론
git clone https://github.com/yunkya2/elf2x68k.git
cd elf2x68k

# 2. 빌드 (시간 소요: 수십 분 ~ 수 시간)
make all

# 3. 환경 변수 설정
export XELF_BASE=$(pwd)/m68k-xelf
export PATH=$XELF_BASE/bin:$PATH
```

---

## 3. Hello World 프로그램

### 3.1 프로젝트 구조

```
x68000/
├── README.md                  (본 문서)
├── src/
│   └── hello.c
├── Makefile
└── build/
    └── hello.x              (컴파일된 실행 파일)
```

### 3.2 소스 코드 (hello.c)

```c
/*
 * X68000 Human68k Hello World
 * Compiled with m68k-xelf-gcc (elf2x68k)
 */
#include <stdio.h>

int main(void)
{
    printf("Hello, X68000 World!\n");
    return 0;
}
```

### 3.3 Makefile

프로젝트 루트에 자동화된 빌드 시스템을 제공하는 Makefile이 포함되어 있습니다.

**주요 특징:**
- PATH 자동 설정 (별도 환경 변수 설정 불필요)
- src/*.c 파일 자동 감지 및 빌드
- CPU 타겟 선택 가능
- run68 에뮬레이터 연동

```makefile
# X68000 Human68k Cross Compile Makefile
# Toolchain: elf2x68k (m68k-xelf-gcc)

# Directories
TOOLCHAIN_DIR := $(CURDIR)/../../Toolchain/x68000/toolchain/m68k-xelf
SRC_DIR := src
BUILD_DIR := build

# Toolchain
export PATH := $(TOOLCHAIN_DIR)/bin:$(PATH)
CC := m68k-xelf-gcc
RUN68 := run68

# Compiler flags
CFLAGS := -O2 -Wall
LDFLAGS :=

# CPU target (68000, 68020, 68030, 68040, 68060)
CPU ?= 68000
CFLAGS += -m$(CPU)

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.c)
TARGETS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.x,$(SRCS))

# Default target
all: $(BUILD_DIR) $(TARGETS)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile .c to .x
$(BUILD_DIR)/%.x: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

# Run with run68 emulator
run: $(BUILD_DIR)/hello.x
	$(RUN68) $<

# Run specific target: make run-<name>
run-%: $(BUILD_DIR)/%.x
	$(RUN68) $<

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean
```

### 3.4 Makefile 사용법

#### 기본 명령어

| 명령어 | 설명 |
|--------|------|
| `make` | src/*.c를 모두 build/*.x로 컴파일 |
| `make run` | hello.x 빌드 후 run68으로 실행 |
| `make run-NAME` | NAME.x 빌드 후 실행 |
| `make clean` | build/ 디렉토리 삭제 |
| `make rebuild` | clean 후 다시 빌드 |
| `make info` | 툴체인 정보 출력 |
| `make help` | 도움말 출력 |

#### 빌드 예시

```bash
# 기본 빌드 (68000 타겟)
make

# 빌드 후 실행
make run

# 특정 프로그램 실행
make run-hello

# 68030 CPU 타겟으로 빌드
make CPU=68030

# 클린 빌드
make rebuild
```

#### CPU 타겟 옵션

```bash
make CPU=68000    # 기본, 모든 X68000 호환
make CPU=68020    # 68020 명령어 사용
make CPU=68030    # X68030, XVI 이상
make CPU=68040    # 68040 가속기
make CPU=68060    # 68060 가속기
```

### 3.5 컴파일 결과

```bash
$ make
mkdir -p build
m68k-xelf-gcc -O2 -Wall -m68000  -o build/hello.x src/hello.c

$ make run
run68 build/hello.x
Hello, X68000 World!
```

컴파일 결과물:
- `build/hello.x` - X68000에서 실행 가능한 X 포맷 파일
- `build/hello.x.elf` - 디버그 정보가 포함된 ELF 파일

---

## 4. 실행 방법

### 4.1 에뮬레이터에서 실행

| 에뮬레이터 | 플랫폼 | 링크 |
|-----------|--------|------|
| XM6 TypeG | Windows | http://www.intj.net/xm6/xm6g/index.htm |
| XEiJ | Java (크로스플랫폼) | https://stdkmd.net/xeij/ |
| run68 | CLI (Linux/Windows) | elf2x68k에 포함 |

### 4.2 run68으로 테스트 (CLI)

```bash
# run68이 elf2x68k에 포함되어 있음
run68 hello.x
# 출력: Hello, X68000 World!
```

### 4.3 실제 X68000 하드웨어

1. `hello.x` 파일을 플로피 디스크 이미지(XDF/DIM)에 복사
2. 에뮬레이터 또는 실제 하드웨어에서 부팅
3. Human68k 프롬프트에서 `hello` 실행

---

## 5. IOCS / DOS 콜 예제

X68000에서는 표준 C 라이브러리 외에도 IOCS와 DOS 콜을 직접 사용할 수 있습니다.

| 구분 | 설명 | 헤더 |
|------|------|------|
| **IOCS** | I/O Control System (BIOS), ROM에 내장, 하드웨어 직접 제어 | `<x68k/iocs.h>` |
| **DOS** | Human68k 운영체제 시스템 콜, 파일/프로세스/메모리 관리 | `<x68k/dos.h>` |

### 5.1 IOCS 콜 예제 (`src/hello_iocs.c`)

```c
/*
 * X68000 Human68k - IOCS Call Example
 * IOCS는 X68000의 BIOS로, 하드웨어를 직접 제어합니다.
 */
#include <x68k/iocs.h>

/* 문자열 출력 (IOCS B_PRINT) */
static void print_string(const char *str)
{
    _iocs_b_print(str);
}

/* 한 문자 출력 (IOCS B_PUTC) */
static void print_char(int c)
{
    _iocs_b_putc(c);
}

/* 개행 출력 */
static void print_newline(void)
{
    print_char('\r');
    print_char('\n');
}

int main(void)
{
    struct iocs_time time;

    /* 화면 클리어 */
    _iocs_b_clr_al();

    /* IOCS를 사용한 문자열 출력 */
    print_string("Hello, X68000 World! (via IOCS)");
    print_newline();

    /* 현재 시간 가져오기 (IOCS ONTIME) */
    time = _iocs_ontime();
    print_string("System time (1/100 sec): ");
    /* ... 숫자 출력 ... */

    /* 키 입력 대기 */
    _iocs_b_keyinp();

    return 0;
}
```

**주요 IOCS 함수:**

| 함수 | 설명 |
|------|------|
| `_iocs_b_print(str)` | 문자열 출력 |
| `_iocs_b_putc(c)` | 문자 출력 |
| `_iocs_b_clr_al()` | 화면 전체 클리어 |
| `_iocs_b_keyinp()` | 키 입력 대기 |
| `_iocs_b_keysns()` | 키 입력 확인 (비블로킹) |
| `_iocs_ontime()` | 시스템 시간 (1/100초) |
| `_iocs_timeget()` | BCD 형식 시간 |

### 5.2 DOS 콜 예제 (`src/hello_dos.c`)

```c
/*
 * X68000 Human68k - DOS Call Example
 * DOS 콜은 Human68k 운영체제가 제공하는 시스템 콜입니다.
 */
#include <x68k/dos.h>

int main(void)
{
    int date, time;
    int year, month, day;
    int hour, minute, second;
    int curdrv;
    char cwd[256];

    /* 화면 클리어 */
    _dos_c_cls_al();

    /* DOS 콜을 사용한 문자열 출력 */
    _dos_c_print("Hello, X68000 World! (via DOS)\r\n");

    /* 다른 출력 방법 */
    _dos_print("Using DOS PRINT function.\r\n");

    /* 현재 날짜 가져오기 */
    date = _dos_getdate();
    year = 1980 + (date & 0x7f);
    month = (date >> 7) & 0x0f;
    day = (date >> 11) & 0x1f;

    /* 현재 시간 가져오기 */
    time = _dos_gettime();
    hour = time & 0xff;
    minute = (time >> 8) & 0xff;
    second = (time >> 16) & 0xff;

    /* 현재 드라이브/디렉토리 */
    curdrv = _dos_curdrv();
    _dos_curdir(curdrv + 1, cwd);

    /* 키 입력 대기 */
    while (_dos_inkey() == 0)
        ;

    return 0;
}
```

**주요 DOS 함수:**

| 함수 | 설명 |
|------|------|
| `_dos_c_print(str)` | 콘솔 문자열 출력 |
| `_dos_print(str)` | 표준 출력 |
| `_dos_c_putc(c)` | 콘솔 문자 출력 |
| `_dos_c_cls_al()` | 화면 전체 클리어 |
| `_dos_getdate()` | 날짜 가져오기 |
| `_dos_gettime()` | 시간 가져오기 |
| `_dos_curdrv()` | 현재 드라이브 |
| `_dos_curdir(drv, buf)` | 현재 디렉토리 |
| `_dos_inkey()` | 키 입력 (비블로킹) |
| `_dos_malloc(size)` | 메모리 할당 |

### 5.3 빌드 및 실행

```bash
# 전체 빌드
make

# IOCS 예제 실행
make run-hello_iocs

# DOS 예제 실행
make run-hello_dos
```

**실행 결과 (IOCS):**
```
=== IOCS Call Example ===

Hello, X68000 World! (via IOCS B_PRINT)

Character output: IOCS

System time (1/100 sec from midnight): 35441
Time (BCD format): 0x01160314

Press any key to exit...
```

**실행 결과 (DOS):**
```
=== DOS Call Example ===

Hello, X68000 World! (via DOS C_PRINT)

This line uses DOS PRINT function.

Current Date: 2024-08-11 (Mon)
Current Time: 01:16:03
Current Drive: A:

Press any key to exit...
```

### 5.4 그래픽 출력 예제 (`src/hello_gfx.c`)

X68000의 강력한 그래픽 기능을 IOCS를 통해 사용하는 예제입니다.

```c
/*
 * X68000 Human68k - Graphics Example
 * IOCS 그래픽 함수를 사용한 기본 그래픽 출력 예제
 */
#include <x68k/iocs.h>

/* 색상 정의 (16색 팔레트 인덱스) */
#define COLOR_BLACK     0
#define COLOR_WHITE     7

/* 점 그리기 */
static void draw_point(int x, int y, int color)
{
    struct iocs_psetptr p;
    p.x = x;
    p.y = y;
    p.color = color;
    _iocs_pset(&p);
}

/* 선 그리기 */
static void draw_line(int x1, int y1, int x2, int y2, int color)
{
    struct iocs_lineptr l;
    l.x1 = x1;  l.y1 = y1;
    l.x2 = x2;  l.y2 = y2;
    l.color = color;
    l.linestyle = 0xFFFF;  /* 실선 */
    _iocs_line(&l);
}

/* 사각형 채우기 */
static void fill_box(int x1, int y1, int x2, int y2, int color)
{
    struct iocs_fillptr f;
    f.x1 = x1;  f.y1 = y1;
    f.x2 = x2;  f.y2 = y2;
    f.color = color;
    _iocs_fill(&f);
}

/* 원 그리기 */
static void draw_circle(int x, int y, int radius, int color)
{
    struct iocs_circleptr c;
    c.x = x;
    c.y = y;
    c.radius = radius;
    c.color = color;
    c.start = 0;      /* 시작 각도 */
    c.end = 0;        /* 끝 각도 (0 = 전체 원) */
    c.ratio = 256;    /* 종횡비 (256 = 1:1) */
    _iocs_circle(&c);
}

/* 그래픽 텍스트 출력 */
static void draw_text(int x, int y, const char *str, int color)
{
    struct iocs_symbolptr s;
    s.x1 = x;
    s.y1 = y;
    s.string_address = (const unsigned char *)str;
    s.mag_x = 1;      /* 가로 배율 */
    s.mag_y = 1;      /* 세로 배율 */
    s.color = color;
    s.font_type = 0;  /* 폰트 타입 */
    s.angle = 0;      /* 회전 각도 */
    _iocs_symbol(&s);
}

int main(void)
{
    /* 그래픽 모드 설정 (512x512, 16색) */
    _iocs_crtmod(12);
    _iocs_g_clr_on();     /* 화면 클리어 */
    _iocs_apage(0);       /* 액세스 페이지 */
    _iocs_vpage(0);       /* 표시 페이지 */

    /* 그래픽 데모 */
    draw_text(150, 20, "X68000 Graphics Demo", COLOR_WHITE);
    draw_line(50, 100, 200, 100, 2);      /* 빨간 선 */
    fill_box(100, 150, 200, 200, 4);      /* 녹색 사각형 */
    draw_circle(300, 175, 50, 1);         /* 파란 원 */

    /* 키 입력 대기 */
    _iocs_b_keyinp();

    /* 텍스트 모드로 복귀 */
    _iocs_crtmod(16);

    return 0;
}
```

**주요 그래픽 IOCS 함수:**

| 함수 | 구조체 | 설명 |
|------|--------|------|
| `_iocs_crtmod(mode)` | - | 화면 모드 설정 |
| `_iocs_g_clr_on()` | - | 그래픽 화면 클리어 |
| `_iocs_apage(page)` | - | 액세스 페이지 설정 (0-3) |
| `_iocs_vpage(page)` | - | 표시 페이지 설정 |
| `_iocs_pset(&p)` | `iocs_psetptr` | 점 그리기 |
| `_iocs_line(&l)` | `iocs_lineptr` | 선 그리기 |
| `_iocs_box(&b)` | `iocs_boxptr` | 사각형 테두리 |
| `_iocs_fill(&f)` | `iocs_fillptr` | 사각형 채우기 |
| `_iocs_circle(&c)` | `iocs_circleptr` | 원/타원/호 그리기 |
| `_iocs_symbol(&s)` | `iocs_symbolptr` | 그래픽 텍스트 출력 |

**화면 모드 (crtmod):**

| 모드 | 해상도 | 색상 | 용도 |
|------|--------|------|------|
| 0 | 256x256 | 16색 | 게임 |
| 4 | 512x512 | 16색 | 기본 그래픽 |
| 12 | 512x512 | 16색 | 그래픽 (권장) |
| 14 | 256x256 | 256색 | 고색상 |
| 16 | 768x512 | - | 텍스트 모드 |

**그래픽 구조체:**

```c
/* 선 그리기 구조체 */
struct iocs_lineptr {
    short x1, y1;           /* 시작점 */
    short x2, y2;           /* 끝점 */
    iocs_color_t color;     /* 색상 (팔레트 인덱스) */
    unsigned short linestyle; /* 선 스타일 (0xFFFF=실선) */
};

/* 원 그리기 구조체 */
struct iocs_circleptr {
    short x, y;             /* 중심점 */
    unsigned short radius;  /* 반지름 */
    iocs_color_t color;     /* 색상 */
    short start, end;       /* 호의 시작/끝 각도 (0=전체) */
    unsigned short ratio;   /* 종횡비 (256=1:1) */
};
```

### 5.5 그래픽 예제 실행

```bash
# 빌드
make

# run68은 CUI 전용이므로 그래픽 미지원
make run-hello_gfx  # 실행되지만 화면 출력 없음

# GUI 에뮬레이터에서 테스트 필요:
# - XM6 TypeG (Windows)
# - XEiJ (Java, 크로스플랫폼)
# - px68k / px68k-libretro (Linux/RetroArch)
```

### 5.6 사운드 출력 예제 (`src/hello_snd.c`)

X68000의 YM2151 (OPM) FM 음원을 사용한 사운드 출력 예제입니다.

**X68000 사운드 하드웨어:**

| 칩 | 타입 | 설명 |
|-----|------|------|
| YM2151 (OPM) | FM 음원 | 8채널, 4오퍼레이터 |
| MSM6258 | ADPCM | 음성/효과음 재생 |

```c
/*
 * X68000 Human68k - Sound Example
 * OPM (YM2151) FM 음원 사용
 */
#include <x68k/iocs.h>

/* OPM 레지스터 */
#define OPM_REG_KEYON   0x08    /* 키 온/오프 */
#define OPM_REG_PAN_FL  0x20    /* PAN/FL/CON */
#define OPM_REG_KC      0x28    /* Key Code (음정) */
#define OPM_REG_TL      0x60    /* Total Level (볼륨) */
#define OPM_REG_KS_AR   0x80    /* Attack Rate */
#define OPM_REG_D1L_RR  0xE0    /* Release Rate */

/* 음계 */
#define NOTE_C  0
#define NOTE_D  2
#define NOTE_E  4
#define NOTE_G  8
#define NOTE_A  10

/* OPM 레지스터 쓰기 */
static void opm_write(int reg, int data)
{
    while (_iocs_opmsns() & 0x80)  /* busy 대기 */
        ;
    _iocs_opmset(reg, data);
}

/* 악기 설정 (채널 0) */
static void setup_instrument(int ch)
{
    opm_write(OPM_REG_PAN_FL + ch, 0xC7);   /* L+R 출력 */
    opm_write(OPM_REG_TL + ch, 0x20);       /* 볼륨 */
    opm_write(OPM_REG_KS_AR + ch, 0x1F);    /* 빠른 어택 */
    opm_write(OPM_REG_D1L_RR + ch, 0x0F);   /* 릴리즈 */
}

/* 음 재생 */
static void play_note(int ch, int octave, int note)
{
    opm_write(OPM_REG_KC + ch, (octave << 4) | note);
    opm_write(OPM_REG_KEYON, 0x78 | ch);    /* 키 온 */
}

/* 음 정지 */
static void stop_note(int ch)
{
    opm_write(OPM_REG_KEYON, ch);           /* 키 오프 */
}

int main(void)
{
    int i;
    int melody[] = {NOTE_C, NOTE_D, NOTE_E, NOTE_G, NOTE_A};

    setup_instrument(0);

    /* 음계 연주 */
    for (i = 0; i < 5; i++) {
        play_note(0, 4, melody[i]);
        /* delay */
        stop_note(0);
    }

    return 0;
}
```

**주요 OPM 레지스터:**

| 레지스터 | 오프셋 | 설명 |
|----------|--------|------|
| `0x08` | - | 키 온/오프 (슬롯 마스크 + 채널) |
| `0x20` | +ch | PAN/FL/CON (출력/피드백/알고리즘) |
| `0x28` | +ch | KC (Key Code: 옥타브*16 + 음계) |
| `0x30` | +ch | KF (Key Fraction: 미세 조정) |
| `0x60` | +ch | TL (Total Level: 0=최대, 127=무음) |
| `0x80` | +ch | KS/AR (Key Scale/Attack Rate) |
| `0xE0` | +ch | D1L/RR (Decay Level/Release Rate) |

**IOCS 사운드 함수:**

| 함수 | 설명 |
|------|------|
| `_iocs_opmset(reg, data)` | OPM 레지스터 쓰기 |
| `_iocs_opmsns()` | OPM 상태 (bit7: busy) |
| `_iocs_adpcmout(buf, size, freq)` | ADPCM 재생 |
| `_iocs_adpcmsns()` | ADPCM 상태 |
| `_iocs_adpcmmod(mode)` | ADPCM 모드 설정 |

**키 코드 (KC) 음계:**

| 값 | 음 | 값 | 음 |
|----|----|----|-----|
| 0 | C | 8 | G |
| 1 | C# | 9 | G# |
| 2 | D | 10 | A |
| 4 | E | 11 | A# |
| 5 | F | 12 | B |
| 6 | F# | | |

### 5.7 사운드 예제 실행

```bash
# 빌드
make

# run68은 사운드 미지원
make run-hello_snd  # 실행되지만 소리 없음

# 실제 사운드 테스트는 GUI 에뮬레이터 필요
```

**실행 결과 (run68):**
```
=== X68000 Sound Demo (OPM/YM2151) ===

Initializing OPM...
Playing scale: Do Re Mi Fa Sol La Si Do

  Playing: C4
  Playing: D4
  Playing: E4
  ...

Playing chord (C major)...
Playing arpeggio...

Sound demo complete!
Press any key to exit...
```

### 5.8 키보드 입력 예제 (`src/hello_key.c`)

X68000의 키보드 입력 처리 예제입니다. IOCS와 DOS 양쪽 방식을 모두 다룹니다.

**X68000 키보드:**
- 109키 풀 키보드
- 스캔코드 + ASCII 코드 반환
- 특수키: SHIFT, CTRL, OPT, XF1-5 등

```c
/*
 * X68000 Human68k - Keyboard Input Example
 */
#include <x68k/iocs.h>
#include <x68k/dos.h>

/* 키 코드 매크로 */
#define SCANCODE(k)  (((k) >> 8) & 0xFF)
#define CHARCODE(k)  ((k) & 0xFF)

/* 특수 키 스캔코드 */
#define KEY_ESC     0x01
#define KEY_RETURN  0x1D
#define KEY_UP      0x3C
#define KEY_DOWN    0x3E
#define KEY_LEFT    0x3B
#define KEY_RIGHT   0x3D

int main(void)
{
    int key, scancode, charcode;

    /* IOCS: 키 입력 대기 (블로킹) */
    key = _iocs_b_keyinp();
    scancode = SCANCODE(key);
    charcode = CHARCODE(key);

    /* IOCS: 키 입력 확인 (비블로킹) */
    if (_iocs_b_keysns() != 0) {
        key = _iocs_b_keyinp();
    }

    /* DOS: 키 입력 (비블로킹) */
    key = _dos_inkey();
    if (key != 0) {
        /* 키가 눌림 */
    }

    /* DOS: 문자열 입력 */
    struct dos_inpptr inp;
    inp.max = 80;
    _dos_gets(&inp);
    /* inp.buffer에 입력된 문자열 */
    /* inp.length에 길이 */

    return 0;
}
```

**IOCS 키보드 함수:**

| 함수 | 반환값 | 설명 |
|------|--------|------|
| `_iocs_b_keyinp()` | 키 코드 | 키 입력 대기 (블로킹) |
| `_iocs_b_keysns()` | 0 또는 키 코드 | 키 입력 확인 (비블로킹) |

**DOS 키보드 함수:**

| 함수 | 반환값 | 설명 |
|------|--------|------|
| `_dos_inkey()` | 0 또는 키 코드 | 키 입력 (비블로킹) |
| `_dos_keysns()` | 0 또는 1 | 키 버퍼 확인 |
| `_dos_getc()` | 문자 | 문자 입력 (블로킹) |
| `_dos_gets(&inp)` | 결과 | 문자열 입력 (편집 가능) |
| `_dos_c_locate(x,y)` | - | 커서 위치 설정 |

**키 코드 구조:**

```c
/*
 * IOCS B_KEYINP / DOS INKEY 반환값:
 * 상위 바이트 (bit 8-15): 스캔코드
 * 하위 바이트 (bit 0-7):  ASCII 코드
 *
 * 예: 'a' 키 = 0x1E61
 *     스캔코드 0x1E, ASCII 0x61 ('a')
 */
int key = _iocs_b_keyinp();
int scan = (key >> 8) & 0xFF;  /* 스캔코드 */
int ascii = key & 0xFF;         /* ASCII */
```

**주요 스캔코드:**

| 코드 | 키 | 코드 | 키 |
|------|-----|------|-----|
| 0x01 | ESC | 0x35 | SPACE |
| 0x1D | RETURN | 0x0E | BACKSPACE |
| 0x0F | TAB | 0x39 | CAPS |
| 0x3C | ↑ | 0x3E | ↓ |
| 0x3B | ← | 0x3D | → |
| 0x63 | F1 | 0x64 | F2 |
| 0x65 | F3 | 0x66 | F4 |
| 0x67 | F5 | 0x68-72 | F6-F10 |

**문자열 입력 구조체:**

```c
struct dos_inpptr {
    unsigned char max;      /* 최대 입력 길이 */
    unsigned char length;   /* 실제 입력 길이 */
    char buffer[256];       /* 입력 버퍼 */
};
```

### 5.9 키보드 예제 실행

```bash
# 빌드
make

# 실행 (run68은 키보드 입력 제한적)
make run-hello_key

# 실제 테스트는 GUI 에뮬레이터 권장
```

**실행 결과:**
```
========================================
  X68000 Keyboard Input Demo
========================================

=== Demo 1: Basic Key Input (IOCS) ===
Press keys to see their codes. Press ESC to continue.

Key: 0x1E61  Scan: 0x1E  Char: 'a'
Key: 0x3C00  Scan: 0x3C  Char: 0x00  (UP)
Key: 0x0100  Scan: 0x01  Char: 0x00  (ESC)

=== Demo 2: Non-blocking Input (DOS) ===
Waiting... 1234

=== Demo 3: Arrow Keys ===
Position: (15, 7)
    *

=== Demo 4: String Input (DOS) ===
Enter your name: X68000
You entered: X68000
Length: 6 characters
```

### 5.10 마우스 입력 예제 (`src/hello_mouse.c`)

X68000은 전용 마우스 포트를 지원하며, IOCS를 통해 마우스 입력을 처리합니다.

**마우스 IOCS 함수:**

| 함수 | 설명 |
|------|------|
| `_iocs_ms_init()` | 마우스 초기화 |
| `_iocs_ms_getdt()` | 마우스 데이터 읽기 (이동량 + 버튼) |
| `_iocs_ms_curgt()` | 커서 위치 읽기 |
| `_iocs_ms_curst()` | 커서 위치 설정 |
| `_iocs_ms_curon()` | 마우스 커서 표시 |
| `_iocs_ms_curof()` | 마우스 커서 숨기기 |
| `_iocs_ms_limit()` | 이동 범위 제한 |
| `_iocs_ms_patst()` | 커서 패턴 설정 |

**`_iocs_ms_getdt()` 반환값 구조:**

```
비트 31-24: 버튼 상태 (bit0=왼쪽, bit1=오른쪽)
비트 23-16: Y 이동량 (signed 8-bit)
비트 15-8:  X 이동량 (signed 8-bit)
비트 7-0:   상태 정보
```

**주요 코드:**

```c
#include <x68k/iocs.h>

/* 마우스 버튼 비트 마스크 */
#define MOUSE_LEFT      0x01
#define MOUSE_RIGHT     0x02

/* 마우스 데이터 파싱 */
static int get_mouse_buttons(int data)
{
    return (data >> 24) & 0xFF;
}

static int get_mouse_dx(int data)
{
    signed char dx = (data >> 8) & 0xFF;
    return dx;
}

static int get_mouse_dy(int data)
{
    signed char dy = (data >> 16) & 0xFF;
    return dy;
}

/* 사용 예 */
int data = _iocs_ms_getdt();
int buttons = get_mouse_buttons(data);
int dx = get_mouse_dx(data);
int dy = get_mouse_dy(data);

if (buttons & MOUSE_LEFT) {
    /* 왼쪽 버튼 눌림 */
}
```

**데모 기능:**
1. **마우스 정보 표시** - Raw 데이터, 좌표, 버튼 상태
2. **텍스트 커서 이동** - 마우스로 '*' 커서 제어, 클릭으로 그리기
3. **버튼 이벤트 감지** - 클릭 다운/업 이벤트, 클릭 카운터
4. **드래그 감지** - 드래그 시작/진행/종료 감지

### 5.11 마우스 예제 실행

```bash
# 빌드
make

# 실행 (run68은 마우스 미지원)
# GUI 에뮬레이터(XM6, XEiJ, px68k)에서 테스트 필요
```

**예상 실행 결과:**
```
========================================
  X68000 Mouse Input Demo
========================================

=== Demo 1: Mouse Information ===
Raw: 0x01000502
Position: (384, 256)
Delta: dx=5, dy=2
Buttons: [LEFT]

=== Demo 2: Text Mode Cursor ===
(마우스로 '*' 커서 이동, 클릭으로 'O' 그리기)

=== Demo 3: Button Events ===
LEFT button DOWN!
LEFT button UP!
Left clicks: 5
Right clicks: 2

=== Demo 4: Drag Detection ===
Drag started at (200, 150)
Dragging... distance: (50, 30)
Drag ended! Total: (50, 30)
```

**참고:** run68은 CUI 에뮬레이터로 마우스를 지원하지 않습니다.
실제 마우스 테스트는 XM6, XEiJ, px68k 같은 GUI 에뮬레이터가 필요합니다.

### 5.12 조이스틱 입력 예제 (`src/hello_joy.c`)

X68000은 ATARI 호환 조이스틱 포트 2개를 지원합니다.

**조이스틱 IOCS 함수:**

| 함수 | 설명 |
|------|------|
| `_iocs_joyget(port)` | 조이스틱 상태 읽기 (port: 0 또는 1) |

**`_iocs_joyget()` 반환값 구조:**

```
비트 0: UP (0=눌림, 1=안눌림) - Active LOW
비트 1: DOWN
비트 2: LEFT
비트 3: RIGHT
비트 5: 버튼 A (트리거 1)
비트 6: 버튼 B (트리거 2)
```

**주요 코드:**

```c
#include <x68k/iocs.h>

/* 조이스틱 비트 정의 */
#define JOY_UP      0x01
#define JOY_DOWN    0x02
#define JOY_LEFT    0x04
#define JOY_RIGHT   0x08
#define JOY_BTN_A   0x20
#define JOY_BTN_B   0x40

/* Active LOW 체크 매크로 */
#define JOY_PRESSED(state, btn)  (!((state) & (btn)))

/* 사용 예 */
int joy = _iocs_joyget(0);  /* 포트 1 읽기 */

if (JOY_PRESSED(joy, JOY_UP)) {
    /* 위쪽 눌림 */
}
if (JOY_PRESSED(joy, JOY_BTN_A)) {
    /* 버튼 A 눌림 */
}

/* 8방향 감지 */
if (JOY_PRESSED(joy, JOY_UP) && JOY_PRESSED(joy, JOY_RIGHT)) {
    /* 오른쪽 위 (NE) */
}
```

**데모 기능:**
1. **조이스틱 상태 표시** - 2개 포트의 방향, 버튼 상태를 ASCII 아트로 표시
2. **캐릭터 이동** - 조이스틱으로 '@' 이동, 버튼으로 그리기
3. **버튼 콤보 감지** - A+B 동시 입력, 방향+버튼 콤보
4. **2인용 대전** - JOY1, JOY2로 각각 캐릭터 조작

**X68000 조이스틱 호환 기기:**
- 표준 ATARI 호환 조이스틱
- CYBER STICK (아날로그)
- X68000 순정 패드
- 메가드라이브 패드 (6버튼은 일부만 인식)

### 5.13 조이스틱 예제 실행

```bash
# 빌드
make

# 실행 (run68은 조이스틱 미지원)
# GUI 에뮬레이터에서 테스트 필요
```

**예상 실행 결과:**
```
========================================
  X68000 Joystick Input Demo
========================================

=== Demo 1: Joystick Status ===
JOY 1           JOY 2
  ^
 <o>            <o>
                 v
A:@  B:O        A:O  B:O
Raw:0x5E        Raw:0x7F

=== Demo 2: Character Movement ===
Pos: (25, 12)
    @

=== Demo 3: Button Combo Detection ===
Button A: [PRESSED]
Button B: [PRESSED]
*** COMBO! A+B ***
Combo count: 3
Direction: NE
Special: RIGHT + B (Dash!)

=== Demo 4: Two Player Mode ===
P1 Score: 2   P2 Score: 1
    1                    2
```

**참고:** run68은 CUI 에뮬레이터로 조이스틱을 지원하지 않습니다.
실제 테스트는 XM6, XEiJ, px68k 같은 GUI 에뮬레이터가 필요합니다.

### 5.14 스프라이트 예제 (`src/hello_sprite.c`)

X68000은 강력한 하드웨어 스프라이트 기능을 지원합니다.

**X68000 스프라이트 사양:**
- 최대 128개 스프라이트 (16x16 픽셀)
- 스프라이트당 16색 (16개 팔레트 블록)
- 하드웨어 우선순위 제어 (0-3)
- 수평/수직 반전
- BG (Background) 2면

**스프라이트 IOCS 함수:**

| 함수 | 설명 |
|------|------|
| `_iocs_sp_init()` | 스프라이트/BG 시스템 초기화 |
| `_iocs_sp_on()` | 스프라이트 표시 ON |
| `_iocs_sp_off()` | 스프라이트 표시 OFF |
| `_iocs_sp_defcg(code, size, addr)` | 패턴 정의 |
| `_iocs_sp_regst(num, x, y, code, attr, pri)` | 스프라이트 설정 |
| `_iocs_spalet(mode, num, color)` | 팔레트 설정 |
| `_iocs_sp_cgclr(code)` | 패턴 클리어 |

**`_iocs_sp_regst()` 파라미터:**

```
num:  스프라이트 번호 (0-127)
x, y: 화면 좌표
code: 패턴 코드 (0-255)
attr: 속성
      bit 0-3: 팔레트 블록 (0-15)
      bit 4: 수평 반전
      bit 5: 수직 반전
pri:  우선순위 (0-3, 높을수록 앞에 표시)
```

**주요 코드:**

```c
#include <x68k/iocs.h>

/* 스프라이트 패턴 (16x16, 4bpp = 128바이트) */
/* 각 바이트 = 2픽셀 (상위4비트=왼쪽, 하위4비트=오른쪽) */
static const unsigned char pattern[128] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* 라인 0 */
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,  /* 라인 1 */
    /* ... 16라인 */
};

/* 초기화 */
_iocs_sp_init();
_iocs_sp_on();

/* 패턴 정의 (code=0, size=1(16x16)) */
_iocs_sp_defcg(0, 1, pattern);

/* 팔레트 설정 (block*16 + index) */
_iocs_spalet(1, 1*16 + 15, 0xFFFF);  /* 블록1, 색15 = 흰색 */

/* 스프라이트 표시 */
_iocs_sp_regst(0, 128, 128, 0, 1, 3);  /* SP0, 좌표(128,128), 패턴0, 팔레트1, 우선순위3 */

/* 수평 반전 */
_iocs_sp_regst(1, 160, 128, 0, 1 | 0x10, 3);  /* attr bit4=H반전 */

/* 숨기기 (화면 밖으로) */
_iocs_sp_regst(0, 0, 256, 0, 0, 0);
```

**데모 기능:**
1. **기본 스프라이트** - 5가지 패턴 (사각형, 원, 화살표, 별, 캐릭터)
2. **스프라이트 이동** - 조이스틱으로 캐릭터 이동
3. **다중 애니메이션** - 8개 스프라이트 바운스 애니메이션
4. **우선순위** - 겹치는 스프라이트 우선순위 표시
5. **반전** - 수평/수직 반전 데모

### 5.15 스프라이트 예제 실행

```bash
# 빌드
make

# 실행 (run68은 스프라이트 미지원)
# GUI 에뮬레이터(XM6, XEiJ, px68k)에서 테스트 필요
```

**예상 실행 결과:**
```
========================================
  X68000 Sprite Demo
========================================

=== Demo 1: Basic Sprites ===
(5개의 다른 패턴 스프라이트 표시)
[□] [○] [→] [☆] [人]

=== Demo 2: Sprite Movement ===
Position: (128, 100)
(조이스틱으로 캐릭터 스프라이트 이동)

=== Demo 3: Multiple Sprite Animation ===
Frame: 256
(8개 스프라이트가 화면에서 바운스)

=== Demo 4: Sprite Priority ===
Priority order: SP0(3) > SP1(2) > SP2(1) > SP3(0)
(겹치는 스프라이트가 우선순위에 따라 표시)

=== Demo 5: Sprite Flip ===
Flip mode: H+V Flip
(화살표가 각각 정상/H반전/V반전/H+V반전으로 표시)
```

**참고:** run68은 CUI 에뮬레이터로 스프라이트를 지원하지 않습니다.
실제 테스트는 XM6, XEiJ, px68k 같은 GUI 에뮬레이터가 필요합니다.

### 5.16 BG 타일맵 예제 (`src/hello_bg.c`)

X68000은 2개의 BG (Background) 레이어를 지원하며, 하드웨어 스크롤이 가능합니다.

**X68000 BG 사양:**
- 2개의 BG 레이어 (BG0, BG1)
- 64x64 타일 맵 (8x8 픽셀 타일)
- 타일당 16색 (16개 팔레트)
- 하드웨어 스크롤 (512x512 범위)
- 스프라이트와 PCG 패턴 공유

**BG IOCS 함수:**

| 함수 | 설명 |
|------|------|
| `_iocs_sp_init()` | 스프라이트/BG 시스템 초기화 |
| `_iocs_bgctrlst(page, txsel, ctrl)` | BG 제어 설정 |
| `_iocs_bgscrlst(page, x, y)` | BG 스크롤 위치 설정 |
| `_iocs_bgtextst(page, x, y, data)` | 타일 설정 |
| `_iocs_bgtextcl(page, code)` | BG 전체 클리어 |
| `_iocs_sp_defcg(code, size, addr)` | 타일 패턴 정의 (스프라이트와 공유) |

**`_iocs_bgtextst()` 데이터 구조:**

```
bit 0-7:   패턴 코드 (0-255)
bit 8-11:  팔레트 번호 (0-15)
bit 12:    수평 반전
bit 13:    수직 반전
```

**주요 코드:**

```c
#include <x68k/iocs.h>

/* 타일 패턴 (8x8, 4bpp = 32바이트) */
static const unsigned char pattern_brick[32] = {
    0x22, 0x22, 0x22, 0x20,
    0x22, 0x22, 0x22, 0x20,
    0x22, 0x22, 0x22, 0x20,
    0x00, 0x00, 0x00, 0x00,
    /* ... 8라인 */
};

/* 초기화 */
_iocs_sp_init();
_iocs_sp_on();

/* 패턴 정의 (code=1, size=0(8x8)) */
_iocs_sp_defcg(1, 0, pattern_brick);

/* BG0 활성화 (page, txsel, ctrl) */
_iocs_bgctrlst(0, 0, 1);  /* BG0 ON */

/* 타일 배치 */
int data = 1 | (2 << 8);  /* 패턴1, 팔레트2 */
_iocs_bgtextst(0, 10, 10, data);

/* 스크롤 */
_iocs_bgscrlst(0, 100, 50);  /* BG0을 (100, 50)으로 스크롤 */

/* BG 전체 클리어 */
_iocs_bgtextcl(0, 0);  /* 패턴 0으로 클리어 */
```

**데모 기능:**
1. **기본 타일맵** - 8가지 타일 패턴 격자 표시
2. **스크롤** - 조이스틱으로 BG 스크롤 제어
3. **2개 레이어** - BG0/BG1 패럴랙스 스크롤
4. **맵 에디터** - 조이스틱으로 타일 배치
5. **플랫폼 맵** - 자동 스크롤 플랫폼 게임 스타일 맵

### 5.17 BG 타일맵 예제 실행

```bash
# 빌드
make

# 실행 (run68은 BG 미지원)
# GUI 에뮬레이터(XM6, XEiJ, px68k)에서 테스트 필요
```

**예상 실행 결과:**
```
========================================
  X68000 BG Tilemap Demo
========================================

=== Demo 1: Basic Tilemap ===
(8가지 타일 패턴이 격자로 표시)
[빈칸][채움][벽돌][잔디]
[물  ][하늘][구름][체크]

=== Demo 2: BG Scrolling ===
Scroll: (128, 64)
(체크보드 패턴이 스크롤)

=== Demo 3: Two BG Layers ===
BG0: 64  BG1: 16
(하늘+구름 배경 위에 잔디가 패럴랙스 스크롤)

=== Demo 4: Map Editor ===
Cursor: (16, 16) Tile: 2
(조이스틱으로 타일 배치)

=== Demo 5: Platform Map ===
Scroll: 128
(플랫폼 게임 스타일 맵이 자동 스크롤)
```

**참고:** run68은 CUI 에뮬레이터로 BG를 지원하지 않습니다.
실제 테스트는 XM6, XEiJ, px68k 같은 GUI 에뮬레이터가 필요합니다.

### 5.18 타이머 인터럽트 예제 (`src/hello_timer.c`)

X68000은 여러 타이머 소스를 지원합니다.

**X68000 타이머 종류:**
- **ontime**: 시스템 시간 (1/100초 단위)
- **Timer-D**: MFP 68901 범용 타이머
- **VDISP**: 수직 동기 인터럽트 (60Hz)
- **Raster**: 래스터 인터럽트

**타이머 IOCS 함수:**

| 함수 | 설명 |
|------|------|
| `_iocs_ontime()` | 시스템 시간 얻기 (struct iocs_time) |
| `_iocs_timerdst(handler, unit, count)` | Timer-D 설정 |
| `_iocs_vdispst(handler, flag, line)` | VDISP 인터럽트 설정 |
| `_iocs_timeget()` | 현재 시간 얻기 (BCD) |

**`_iocs_ontime()` 반환 구조체:**

```c
struct iocs_time {
    int sec;  /* 1/100초 단위 시간 */
    int day;  /* 일수 */
};
```

**`_iocs_timerdst()` 파라미터:**

```
handler: 인터럽트 핸들러 함수 (0이면 정지)
unit: 타이머 단위 (1-4)
  1 = 1/200초
  2 = 1/100초
  3 = 1/50초
  4 = 1/25초
count: 카운트 값 (1-256)
```

**주요 코드:**

```c
#include <x68k/iocs.h>

/* 시스템 시간 얻기 */
struct iocs_time t = _iocs_ontime();
int centisec = t.sec;  /* 1/100초 단위 */

/* Timer-D 인터럽트 핸들러 */
static volatile int timer_count = 0;

static void __attribute__((interrupt)) timer_handler(void)
{
    timer_count++;
}

/* Timer-D 시작 (약 1/6초마다) */
_iocs_timerdst((void *)timer_handler, 4, 4);

/* Timer-D 정지 */
_iocs_timerdst(0, 0, 0);

/* VDISP 인터럽트 (60Hz) */
static void __attribute__((interrupt)) vdisp_handler(void)
{
    /* 매 프레임마다 호출 */
}

_iocs_vdispst((void *)vdisp_handler, 0, 0);  /* 등록 */
_iocs_vdispst(0, 1, 0);  /* 해제 */
```

**데모 기능:**
1. **시스템 시간** - ontime으로 경과 시간 측정
2. **Timer-D** - 주기적 인터럽트 발생
3. **VDISP** - 60Hz 수직 동기 인터럽트
4. **스톱워치** - 시작/정지/랩/리셋 기능
5. **프레임 레이트** - 실제 FPS 측정
6. **주기적 이벤트** - 설정 가능한 간격으로 이벤트 발생

### 5.19 타이머 예제 실행

```bash
# 빌드
make

# 실행
make run-hello_timer
```

**예상 실행 결과:**
```
========================================
  X68000 Timer Interrupt Demo
========================================

=== Demo 1: System Time (ontime) ===
System time: 12345 (1/100 sec)
Elapsed: 5.23 seconds
Seconds passed: 5

=== Demo 2: Timer-D Interrupt ===
Timer-D count: 30
Approx time: 5.0 seconds

=== Demo 3: VDISP Interrupt (60Hz) ===
VDISP count: 300
Elapsed: 5.00 seconds (60Hz)
Frame rate: ~60 fps

=== Demo 4: Stopwatch ===
  0:05.23
Status: RUNNING
Lap times:
  Lap 1: 0:01.50
  Lap 2: 0:03.22

=== Demo 5: Frame Rate Counter ===
Total frames: 500
FPS: 98
Time: 5.10 sec
Animation: /

=== Demo 6: Periodic Events ===
*** EVENT #5 ***    [ON]
Interval: 1.0 seconds
Events: 5
Time to next: 45 (1/100s)
```

### 5.20 파일 입출력 예제 (`src/hello_file.c`)

Human68k는 MS-DOS 호환 파일 시스템을 사용합니다.

**Human68k 파일 시스템 특징:**
- FAT 파일 시스템 (FAT12/FAT16)
- 8.3 파일명 형식
- 드라이브 문자 (A:, B:, ...)
- 디렉토리 지원

**파일 I/O DOS 함수:**

| 함수 | 설명 |
|------|------|
| `_dos_create(name, attr)` | 파일 생성 |
| `_dos_open(name, mode)` | 파일 열기 |
| `_dos_close(handle)` | 파일 닫기 |
| `_dos_read(handle, buf, len)` | 파일 읽기 |
| `_dos_write(handle, buf, len)` | 파일 쓰기 |
| `_dos_seek(handle, offset, whence)` | 위치 이동 |
| `_dos_delete(name)` | 파일 삭제 |
| `_dos_files(buf, pattern, attr)` | 파일 검색 |
| `_dos_nfiles(buf)` | 다음 파일 검색 |
| `_dos_curdir(drive, buf)` | 현재 디렉토리 |
| `_dos_curdrv()` | 현재 드라이브 |
| `_dos_dskfre(drive, info)` | 디스크 여유 공간 |

**파일 열기 모드:**
```
0: 읽기 전용
1: 쓰기 전용
2: 읽기/쓰기
```

**주요 코드:**

```c
#include <x68k/dos.h>

/* 파일 쓰기 */
int handle = _dos_create("TEST.TXT", 0);
if (handle >= 0) {
    _dos_write(handle, "Hello!", 6);
    _dos_close(handle);
}

/* 파일 읽기 */
handle = _dos_open("TEST.TXT", 0);  /* 읽기 모드 */
if (handle >= 0) {
    char buf[256];
    int len = _dos_read(handle, buf, sizeof(buf));
    _dos_close(handle);
}

/* 파일 검색 */
struct dos_filbuf filbuf;
int result = _dos_files(&filbuf, "*.*", 0x37);
while (result >= 0) {
    /* filbuf.name, filbuf.filelen, filbuf.atr 사용 */
    result = _dos_nfiles(&filbuf);
}

/* 디스크 정보 */
struct dos_freeinf info;
_dos_dskfre(0, &info);  /* 0 = 현재 드라이브 */
/* info.free, info.max, info.sec, info.byte */
```

**데모 기능:**
1. **파일 쓰기** - 텍스트 파일 생성 및 쓰기
2. **파일 읽기** - 파일 내용 읽기 및 표시
3. **추가 쓰기** - 파일 끝에 내용 추가
4. **파일 정보** - 크기, 날짜, 속성 표시
5. **디렉토리 목록** - 현재 디렉토리 파일 목록
6. **파일 복사** - 파일 복사
7. **파일 삭제** - 파일 삭제
8. **드라이브 정보** - 여유 공간 및 용량 표시

### 5.21 파일 입출력 예제 실행

```bash
# 빌드
make

# 실행
make run-hello_file
```

**예상 실행 결과:**
```
========================================
  X68000 File I/O Demo
========================================

=== Demo 8: Drive Information ===
Current drive: A:
Cluster size: 1024 bytes (2 sectors x 512 bytes)
Free clusters: 1234 / 2880
Free space: 1264 KB
Total space: 2949 KB

=== Demo 5: Directory Listing ===
Current dir: \
Files in current directory:
---
  COMMAND.X     12345
  HUMAN.SYS     54321
  TEST.TXT      89
---
Total: 3 files shown.

=== Demo 1: Write File ===
Creating and writing to TEST.TXT
File created. Handle: 5
Written 78 bytes.
File closed.

=== Demo 2: Read File ===
Reading from TEST.TXT
File opened. Handle: 5
Contents:
---
Hello, X68000 File System!
This is a test file.
Written by hello_file.x
---
Read 78 bytes total.

=== Demo 4: File Information ===
File: TEST.TXT
Size: 94 bytes
Attr: 0x20 (A)
Date: 2025-01-12
Time: 17:05:30

=== Demo 7: Delete Files ===
Deleting TEST2.TXT... OK
Deleting TEST.TXT... OK
```

### 5.22 메모리 관리 예제 (`src/hello_mem.c`)

Human68k는 DOS 레벨의 메모리 관리 기능을 제공합니다.

**X68000 메모리 구조:**
- 68000 24비트 주소 공간 (16MB)
- 메인 RAM: 1MB ~ 12MB (모델에 따라)
- GVRAM, TVRAM, 스프라이트 RAM 등 별도 영역

**메모리 관리 DOS 함수:**

| 함수 | 설명 |
|------|------|
| `_dos_malloc(size)` | 메모리 할당 |
| `_dos_mfree(ptr)` | 메모리 해제 |
| `_dos_setblock(ptr, size)` | 블록 크기 변경 |
| `_dos_memcpy(dst, src, len)` | 메모리 복사 |
| `_dos_malloc2(size, mode)` | 확장 할당 |

**X68000 메모리 맵:**

```
0x000000-0x0BFFFF: Main RAM (최대 768KB)
0x0C0000-0x0DFFFF: Graphic VRAM (128KB)
0x0E0000-0x0E7FFF: Text VRAM (32KB)
0x0E8000-0x0EBFFF: CRTC/Video (16KB)
0x0EC0000-0x0ECFFFF: System I/O
0x0ED0000-0x0ED3FFF: Sprite/BG RAM (16KB)
0x0F00000-0x0FBFFFF: CGROM (768KB)
0x0FC0000-0x0FFFFFF: IPLROM (256KB)
0x100000-0xBFFFFF: Extended RAM (최대 12MB)
```

**주요 코드:**

```c
#include <x68k/dos.h>
#include <string.h>

/* 메모리 할당 */
void *ptr = _dos_malloc(4096);
if (ptr == NULL || (long)ptr < 0) {
    /* 할당 실패 */
}

/* 메모리 사용 */
memset(ptr, 0, 4096);

/* 블록 크기 변경 */
int result = _dos_setblock(ptr, 8192);
if (result < 0) {
    /* 실패시 새로 할당하고 복사 */
    void *new_ptr = _dos_malloc(8192);
    memcpy(new_ptr, ptr, 4096);
    _dos_mfree(ptr);
    ptr = new_ptr;
}

/* 메모리 복사 (DOS 함수) */
_dos_memcpy(dst, src, len);

/* 메모리 해제 */
_dos_mfree(ptr);
```

**데모 기능:**
1. **기본 할당** - 1KB 할당, 데이터 쓰기/읽기 검증
2. **다중 할당** - 여러 블록 할당, 간격 분석
3. **큰 메모리** - 최대 할당 가능 크기 탐색
4. **리사이즈** - setblock으로 블록 크기 변경
5. **메모리 복사** - _dos_memcpy 테스트
6. **메모리 맵** - X68000 메모리 구조 표시
7. **단편화** - 단편화 테스트 및 영향 확인

### 5.23 메모리 관리 예제 실행

```bash
# 빌드
make

# 실행
make run-hello_mem
```

**예상 실행 결과:**
```
========================================
  X68000 Memory Management Demo
========================================

=== Demo 1: Basic Memory Allocation ===
Allocating 1024 bytes...
Success! Address: 0x00012340
Writing test data...
Memory verification: OK
Freeing memory...
Done.

=== Demo 2: Multiple Allocations ===
Allocating 256 bytes... 0x00012340
Allocating 512 bytes... 0x00012450
Allocating 1024 bytes... 0x00012660
Allocating 2048 bytes... 0x00012A70
Allocating 4096 bytes... 0x00013280

Allocated blocks: 5
  Block 0: 0x00012340, 256 bytes
  Block 1: 0x00012450, 512 bytes
  Block 2: 0x00012660, 1024 bytes
  Block 3: 0x00012A70, 2048 bytes
  Block 4: 0x00013280, 4096 bytes

Block spacing analysis:
  Gap 0->1: 16 bytes (overhead)
  Gap 1->2: 16 bytes (overhead)

=== Demo 3: Large Memory Allocation ===
Finding maximum allocatable size...
Maximum allocatable: 524288 bytes (512 KB)
Allocating max size...
Success at 0x00020000
Testing memory (first/last 256 bytes)...
Verification: PASSED

=== Demo 6: Memory Map ===
X68000 Memory Map:
  0x000000-0x0BFFFF: Main RAM (up to 768KB)
  0x0C0000-0x0DFFFF: Graphic VRAM (128KB)
  0x0E0000-0x0E7FFF: Text VRAM (32KB)
  ...

=== Demo 7: Memory Fragmentation ===
Allocating 20 x 1KB blocks...
Allocated: 20 blocks
Freeing even blocks (creating holes)...
Freed 10 blocks
Trying to allocate 3KB block...
Failed (fragmentation?)
```

### 5.24 ADPCM 사운드 예제 (`src/hello_adpcm.c`)

X68000은 MSM6258 ADPCM 코덱을 통해 음성/PCM 재생을 지원합니다.

**X68000 ADPCM 사양:**
- MSM6258 ADPCM 코덱
- 4비트 ADPCM 인코딩 (2샘플/바이트)
- 샘플레이트: 3.9kHz, 5.2kHz, 7.8kHz, 10.4kHz, 15.6kHz
- 모노 오디오

**ADPCM IOCS 함수:**

| 함수 | 설명 |
|------|------|
| `_iocs_adpcmout(addr, size, freq)` | ADPCM 재생 |
| `_iocs_adpcminp(addr, size, freq)` | ADPCM 녹음 |
| `_iocs_adpcmmod(mode)` | 모드 설정 (0=정지) |
| `_iocs_adpcmsns()` | 상태 확인 |
| `_iocs_adpcmain()` | 체인 재생 |
| `_iocs_adpcmaot()` | 체인 출력 |

**샘플레이트 상수:**
```c
#define ADPCM_3_9KHZ    0   /* 3.9kHz */
#define ADPCM_5_2KHZ    1   /* 5.2kHz */
#define ADPCM_7_8KHZ    2   /* 7.8kHz */
#define ADPCM_10_4KHZ   3   /* 10.4kHz */
#define ADPCM_15_6KHZ   4   /* 15.6kHz */
```

**ADPCM 데이터 구조:**
```
4비트 ADPCM: 상위 4비트 = 첫 샘플, 하위 4비트 = 둘째 샘플
값 범위: 0-15 (8 = 중앙/무음)
```

**주요 코드:**

```c
#include <x68k/iocs.h>

/* ADPCM 데이터 버퍼 */
unsigned char *sample = _dos_malloc(4096);

/* 톤 생성 (사각파) */
for (int i = 0; i < 4096; i++) {
    int phase = (i * 2) / 8;  /* 주파수 분주 */
    int s1 = (phase % 2) ? 0x0F : 0x00;
    int s2 = ((phase + 1) % 2) ? 0x0F : 0x00;
    sample[i] = (s1 << 4) | s2;
}

/* ADPCM 재생 */
_iocs_adpcmout(sample, 4096, ADPCM_15_6KHZ);

/* 재생 완료 대기 */
while (_iocs_adpcmsns() & 0x01) {
    /* 대기 */
}

/* ADPCM 정지 */
_iocs_adpcmmod(0);

_dos_mfree(sample);
```

**데모 기능:**
1. **기본 재생** - 톤 생성 및 ADPCM 재생
2. **샘플레이트** - 5가지 샘플레이트 비교
3. **파형** - 사각파, 삼각파, 노이즈, 페이드아웃
4. **멜로디** - 도레미파솔라시도 음계 재생
5. **효과음** - 상승음, 하강음, 폭발음, 비프
6. **상태 모니터링** - 재생 상태 실시간 표시
7. **사운드보드** - 키 입력으로 효과음 재생

### 5.25 ADPCM 예제 실행

```bash
# 빌드
make

# 실행 (run68은 ADPCM 미지원)
# GUI 에뮬레이터에서 테스트 필요
```

**예상 실행 결과:**
```
========================================
  X68000 ADPCM Sound Demo
========================================

=== Demo 1: Basic ADPCM Playback ===
Generating tone...
Playing at 15.6kHz...
Playback complete.

=== Demo 2: Sample Rates ===
Same tone at different sample rates.
Playing at 3.9kHz...
Playing at 5.2kHz...
Playing at 7.8kHz...
Playing at 10.4kHz...
Playing at 15.6kHz...
Done.

=== Demo 3: Waveforms ===
Square wave...
Triangle wave...
Noise...
Fade out tone...
Done.

=== Demo 4: Melody (Do Re Mi...) ===
  C4...
  D4...
  E4...
  F4...
  G4...
  A4...
  B4...
  C5...
Done.

=== Demo 6: ADPCM Status Monitoring ===
Playing long sample...
Status: 0x01 [PLAYING]
Frame: 234
Playback finished after 456 frames.

=== Demo 7: Interactive Soundboard ===
Press 1-4 to play sounds, ESC to exit.
1: Beep
2: Noise
3: Soft tone
4: High tone
Playing: Beep
```

**참고:** run68은 CUI 에뮬레이터로 ADPCM을 지원하지 않습니다.
실제 테스트는 XM6, XEiJ, px68k 같은 GUI 에뮬레이터가 필요합니다.

### 5.26 게임 예제 (`src/hello_game.c`)

스프라이트, 입력, 사운드를 결합한 간단한 게임 "Star Catcher" 예제입니다.

**게임 개요:**
- 플레이어가 별을 모으고 폭탄을 피하는 게임
- 조이스틱/키보드로 조작
- 점수, 라이프, 레벨 시스템
- OPM 사운드 효과

**게임 구성 요소:**

```c
/* 게임 상태 */
#define STATE_TITLE     0   /* 타이틀 화면 */
#define STATE_PLAYING   1   /* 게임 진행 중 */
#define STATE_GAMEOVER  2   /* 게임 오버 */
#define STATE_PAUSED    3   /* 일시 정지 */

/* 게임 객체 */
#define MAX_STARS   8   /* 별 최대 개수 */
#define MAX_BOMBS   4   /* 폭탄 최대 개수 */
#define MAX_HEARTS  3   /* 최대 라이프 */

/* 스프라이트 번호 */
#define SPR_PLAYER  0       /* 플레이어 */
#define SPR_STAR    1       /* 별 (1-8) */
#define SPR_BOMB    16      /* 폭탄 (16-19) */
#define SPR_HEART   20      /* 하트 (20-22) */
```

**주요 코드 구조:**

```c
/* 플레이어 구조체 */
typedef struct {
    int x, y;           /* 위치 */
    int speed;          /* 이동 속도 */
} Player;

/* 게임 객체 구조체 */
typedef struct {
    int x, y;           /* 위치 */
    int active;         /* 활성화 상태 */
    int speed_y;        /* 낙하 속도 */
} GameObject;

/* 게임 상태 구조체 */
typedef struct {
    int state;          /* 게임 상태 */
    int score;          /* 점수 */
    int lives;          /* 남은 라이프 */
    int level;          /* 현재 레벨 */
    int stars_caught;   /* 잡은 별 수 */
    int frame;          /* 프레임 카운터 */
} GameState;
```

**충돌 감지 (AABB):**

```c
/* 경계 박스 충돌 감지 */
static int check_collision(int x1, int y1, int x2, int y2)
{
    int size1 = 14, size2 = 14;  /* 스프라이트 크기 (마진 제외) */

    if (x1 >= x2 + size2) return 0;  /* 왼쪽에 있음 */
    if (x1 + size1 <= x2) return 0;  /* 오른쪽에 있음 */
    if (y1 >= y2 + size2) return 0;  /* 위에 있음 */
    if (y1 + size1 <= y2) return 0;  /* 아래에 있음 */
    return 1;  /* 충돌! */
}
```

**입력 처리:**

```c
/* 조이스틱 + 키보드 입력 */
static void update_input(void)
{
    int joy = _iocs_joyget(0);
    int key = _iocs_b_keysns() ? _iocs_b_keyinp() : 0;
    int scan = (key >> 8) & 0xFF;

    /* 조이스틱 방향 */
    if (JOY_PRESSED(joy, JOY_LEFT) || scan == KEY_LEFT)
        player.x -= player.speed;
    if (JOY_PRESSED(joy, JOY_RIGHT) || scan == KEY_RIGHT)
        player.x += player.speed;
    if (JOY_PRESSED(joy, JOY_UP) || scan == KEY_UP)
        player.y -= player.speed;
    if (JOY_PRESSED(joy, JOY_DOWN) || scan == KEY_DOWN)
        player.y += player.speed;
}
```

**OPM 사운드 효과:**

```c
/* 사운드 효과 */
static void play_sound_star(void)
{
    /* 별 획득: 높은 음 */
    opm_write(OPM_REG_KC, 0x4A);      /* C5 */
    opm_write(OPM_REG_KEYON, 0x78);   /* 키 온 */
}

static void play_sound_bomb(void)
{
    /* 폭탄 충돌: 낮은 음 */
    opm_write(OPM_REG_KC, 0x22);      /* C3 */
    opm_write(OPM_REG_KEYON, 0x78);
}

static void play_sound_gameover(void)
{
    /* 게임 오버: 하강 멜로디 */
    int notes[] = {0x4A, 0x46, 0x42, 0x32};
    for (int i = 0; i < 4; i++) {
        opm_write(OPM_REG_KC, notes[i]);
        opm_write(OPM_REG_KEYON, 0x78);
        /* delay */
        opm_write(OPM_REG_KEYON, 0x00);
    }
}
```

**게임 루프:**

```c
int main(void)
{
    init_game();

    while (1) {
        switch (game.state) {
        case STATE_TITLE:
            show_title();
            if (start_pressed()) {
                game.state = STATE_PLAYING;
                reset_game();
            }
            break;

        case STATE_PLAYING:
            update_input();
            update_objects();
            check_collisions();
            render_game();

            if (game.lives <= 0) {
                play_sound_gameover();
                game.state = STATE_GAMEOVER;
            }
            break;

        case STATE_GAMEOVER:
            show_gameover();
            if (start_pressed()) {
                game.state = STATE_TITLE;
            }
            break;
        }

        /* 60fps 동기화 */
        _iocs_vsync();
        game.frame++;
    }
}
```

**데모 기능:**
1. **타이틀 화면** - 게임 제목, 조작 설명, 시작 대기
2. **게임 플레이** - 별 수집, 폭탄 회피, 점수 획득
3. **레벨 시스템** - 10개 별 수집마다 레벨업, 속도 증가
4. **라이프 시스템** - 하트 3개, 폭탄 충돌시 감소
5. **일시 정지** - ESC/스타트 버튼으로 일시 정지
6. **게임 오버** - 라이프 0시 게임 종료, 최종 점수 표시

**스프라이트 패턴:**
- 플레이어: 우주선 모양 (파란색)
- 별: 별 모양 (노란색)
- 폭탄: 원형 (빨간색)
- 하트: 하트 모양 (분홍색)

### 5.27 게임 예제 실행

```bash
# 빌드
make

# 실행 (run68은 스프라이트/사운드 미지원)
# GUI 에뮬레이터(XM6, XEiJ, px68k)에서 테스트 필요
```

**예상 실행 결과:**
```
========================================
       ★ STAR CATCHER ★
========================================

    Catch stars, avoid bombs!

Controls:
  Joystick/Arrow keys: Move
  A button/Z key: (reserved)
  ESC/START: Pause

  Press A button or SPACE to start!

--- PLAYING ---
Score: 150    Level: 2

(플레이어가 화면에서 이동하며 별을 수집)
(폭탄을 피하면서 점수 획득)
(레벨업시 별/폭탄 속도 증가)

♥♥♥  Lives: 3

*** LEVEL UP! ***  Level 3

--- GAME OVER ---
Final Score: 450
Level: 3
Stars caught: 45

Press any key for title...
```

**조작법:**
| 입력 | 동작 |
|------|------|
| 조이스틱/방향키 | 플레이어 이동 (8방향) |
| A버튼/스페이스 | 게임 시작 (타이틀) |
| ESC/스타트 | 일시 정지/재개 |

**게임 밸런스:**
- 레벨 1: 별 속도 2, 폭탄 속도 3
- 레벨 2: 별 속도 3, 폭탄 속도 4
- 레벨 3+: 속도 계속 증가

**참고:** run68은 CUI 에뮬레이터로 스프라이트/사운드를 지원하지 않습니다.
실제 게임 테스트는 XM6, XEiJ, px68k 같은 GUI 에뮬레이터가 필요합니다.

### 5.28 MIDI 예제 (`src/hello_midi.c`)

X68000의 RS-232C 포트를 통한 MIDI 출력 예제입니다.

**X68000 MIDI 사양:**
- RS-232C 포트를 31.25 kbaud로 설정하여 MIDI 통신
- 표준 MIDI 프로토콜 지원
- 16채널 멀티팀버
- GM (General MIDI) 호환 음원 연결 가능

**RS-232C MIDI 설정:**

```c
/*
 * _iocs_set232c 파라미터:
 * bit 0-1: Stop bits (0=1, 1=1.5, 2=2)
 * bit 2-3: Parity (0=none, 1=odd, 2=even)
 * bit 4-5: Data bits (0=5, 1=6, 2=7, 3=8)
 * bit 8-11: Baud rate (9 = 31250 = MIDI)
 *
 * MIDI: 8bit, 1stop, no parity, 31250baud = 0x0930
 */
#define MIDI_232C_MODE  0x0930

static void midi_init(void)
{
    _iocs_set232c(MIDI_232C_MODE);
}

static void midi_out(int data)
{
    while (_iocs_osns232c() == 0) { }  /* 송신 대기 */
    _iocs_out232c(data);
}
```

**MIDI 메시지 구조:**

| 상태 바이트 | 의미 | 데이터 |
|------------|------|--------|
| `0x80-0x8F` | Note Off | 노트번호, 벨로시티 |
| `0x90-0x9F` | Note On | 노트번호, 벨로시티 |
| `0xB0-0xBF` | Control Change | CC번호, 값 |
| `0xC0-0xCF` | Program Change | 프로그램번호 |
| `0xE0-0xEF` | Pitch Bend | LSB, MSB |

**주요 코드:**

```c
/* 노트 온 */
static void note_on(int channel, int note, int velocity)
{
    midi_out(0x90 | (channel & 0x0F));
    midi_out(note & 0x7F);
    midi_out(velocity & 0x7F);
}

/* 노트 오프 */
static void note_off(int channel, int note, int velocity)
{
    midi_out(0x80 | (channel & 0x0F));
    midi_out(note & 0x7F);
    midi_out(velocity & 0x7F);
}

/* 프로그램 체인지 (음색 변경) */
static void program_change(int channel, int program)
{
    midi_out(0xC0 | (channel & 0x0F));
    midi_out(program & 0x7F);
}

/* 컨트롤 체인지 */
static void control_change(int channel, int cc, int value)
{
    midi_out(0xB0 | (channel & 0x0F));
    midi_out(cc & 0x7F);
    midi_out(value & 0x7F);
}

/* 피치 벤드 (0-16383, 8192=center) */
static void pitch_bend(int channel, int value)
{
    midi_out(0xE0 | (channel & 0x0F));
    midi_out(value & 0x7F);           /* LSB */
    midi_out((value >> 7) & 0x7F);    /* MSB */
}
```

**GM 프로그램 번호 (일부):**

| 번호 | 악기 | 번호 | 악기 |
|------|------|------|------|
| 0 | Acoustic Piano | 24 | Acoustic Guitar |
| 4 | Electric Piano | 33 | Electric Bass |
| 16 | Drawbar Organ | 48 | String Ensemble |
| 56 | Trumpet | 73 | Flute |
| 80 | Square Lead | 81 | Sawtooth Lead |

**컨트롤 체인지 번호:**

| CC | 의미 | 범위 |
|----|------|------|
| 1 | Modulation | 0-127 |
| 7 | Volume | 0-127 |
| 10 | Pan | 0(L)-64(C)-127(R) |
| 11 | Expression | 0-127 |
| 64 | Sustain Pedal | 0-63(off), 64-127(on) |
| 91 | Reverb | 0-127 |
| 93 | Chorus | 0-127 |

**GM 드럼 노트 (채널 10):**

| 노트 | 악기 | 노트 | 악기 |
|------|------|------|------|
| 36 | Bass Drum | 38 | Snare |
| 42 | Hi-Hat (closed) | 46 | Hi-Hat (open) |
| 49 | Crash Cymbal | 51 | Ride Cymbal |
| 45 | Low Tom | 47 | Mid Tom |

**데모 기능:**
1. **기본 노트** - C 메이저 스케일 연주
2. **음색 변경** - 8가지 GM 악기 비교
3. **화음 연주** - C-F-G-C 코드 진행
4. **컨트롤 체인지** - 볼륨 페이드, 팬 이동
5. **피치 벤드** - 음정 상승/하강
6. **멀티채널** - 3채널 동시 연주 (피아노+베이스+스트링)
7. **드럼 패턴** - 채널 10 드럼 비트
8. **MIDI 입력** - 외부 MIDI 키보드 모니터링

### 5.29 MIDI 예제 실행

```bash
# 빌드
make

# 실행 (MIDI 음원 필요)
# X68000의 RS-232C 포트에 GM 호환 MIDI 음원 연결
```

**예상 실행 결과:**
```
========================================
  X68000 MIDI Demo
========================================

This demo requires a MIDI sound module.
Connect GM/GS compatible device to MIDI OUT.

Press any key to start...

Initializing MIDI (RS-232C at 31.25kbaud)...
Done.

Sending GM Reset...
Done.

=== Demo 1: Basic MIDI Notes ===
Playing C major scale on channel 1
  Note: 60
  Note: 62
  Note: 64
  Note: 65
  Note: 67
  Note: 69
  Note: 71
  Note: 72

=== Demo 2: Program Change (Instruments) ===
Same note with different instruments
  Piano (0)
  E.Piano (4)
  Organ (16)
  Guitar (24)
  Strings (48)
  Trumpet (56)
  Flute (73)
  Synth Lead (81)

=== Demo 6: Multi-channel Playback ===
3 instruments playing together
  Bar 1
  Bar 2
  Bar 3
  Bar 4

=== Demo 7: Drum Channel (Ch.10) ===
Basic drum pattern
  Fill in!

=== Demo 8: MIDI Input Monitor ===
Monitoring MIDI input for 10 seconds...
  RX: 0x90 (Ch.1 NoteOn)
  RX: 0x3C
  RX: 0x64
Received 3 bytes.

MIDI demo complete!
```

**연결 방법:**
1. X68000 RS-232C 포트에 MIDI 인터페이스 연결
2. MIDI 인터페이스 OUT을 GM 음원 IN에 연결
3. GM 음원의 오디오 출력을 스피커에 연결

**호환 음원:**
- Roland SC-55, SC-88, SD-90 등 GS 음원
- Yamaha MU 시리즈 XG 음원
- 소프트웨어 신디사이저 (PC 연결 시)

**참고:**
- run68은 RS-232C를 에뮬레이션하지 않아 MIDI 출력 불가
- GUI 에뮬레이터에서도 MIDI 지원은 제한적
- 실제 X68000 하드웨어 또는 MIDI 지원 에뮬레이터 필요

### 5.30 래스터 인터럽트 예제 (`src/hello_raster.c`)

특정 스캔라인에서 인터럽트를 발생시켜 다양한 시각 효과를 구현하는 예제입니다.

**래스터 인터럽트 용도:**
- 화면 분할 스크롤 (상/하 독립 스크롤)
- 물결/왜곡 효과
- 스캔라인별 팔레트 변경 (그라데이션)
- 물 반사 효과
- 패럴랙스 스크롤

**X68000 래스터 시스템:**

```
CRTC R09: 래스터 번호 레지스터 (인터럽트 발생 라인)
MFP GPIP bit6: CRTC 래스터 인터럽트
MFP GPIP bit4: VDISP (수직 동기)
```

**주요 레지스터 정의:**

```c
/* CRTC 레지스터 */
#define CRTC_BASE       0xE80000
#define CRTC_R09        (*(volatile unsigned short *)(CRTC_BASE + 0x12))  /* Raster */
#define CRTC_R12        (*(volatile unsigned short *)(CRTC_BASE + 0x18))  /* X scroll */
#define CRTC_R13        (*(volatile unsigned short *)(CRTC_BASE + 0x1A))  /* Y scroll */

/* MFP 레지스터 */
#define MFP_BASE        0xE88000
#define MFP_GPIP        (*(volatile unsigned char *)(MFP_BASE + 0x01))
#define MFP_IERA        (*(volatile unsigned char *)(MFP_BASE + 0x07))
#define MFP_IMRA        (*(volatile unsigned char *)(MFP_BASE + 0x13))
#define MFP_ISRA        (*(volatile unsigned char *)(MFP_BASE + 0x0F))
```

**래스터 인터럽트 핸들러:**

```c
static volatile int raster_line = 0;
static volatile short scroll_table[256];

static void __attribute__((interrupt)) raster_handler(void)
{
    /* 현재 라인의 스크롤 값 적용 */
    if (raster_line < 256) {
        CRTC_R12 = scroll_table[raster_line];
    }

    /* 다음 래스터 라인 설정 */
    raster_line++;
    if (raster_line >= 256) {
        raster_line = 0;
    }
    CRTC_R09 = raster_line;

    /* MFP 인터럽트 클리어 */
    MFP_ISRA &= ~0x40;
}
```

**인터럽트 설정/해제:**

```c
static void *old_vector = 0;

static void setup_raster_interrupt(void)
{
    /* 벡터 등록 (0x46 = CRTC raster) */
    old_vector = _iocs_b_intvcs(0x46, (void *)raster_handler);

    /* 초기 래스터 라인 */
    raster_line = 0;
    CRTC_R09 = 0;

    /* MFP 래스터 인터럽트 활성화 */
    MFP_IERA |= 0x40;
    MFP_IMRA |= 0x40;
}

static void cleanup_raster_interrupt(void)
{
    MFP_IMRA &= ~0x40;
    MFP_IERA &= ~0x40;
    if (old_vector) {
        _iocs_b_intvcs(0x46, old_vector);
    }
    CRTC_R12 = 0;  /* 스크롤 리셋 */
}
```

**VSync 대기:**

```c
/* MFP GPIP bit4 = VDISP */
static void wait_vsync(void)
{
    while (!(MFP_GPIP & 0x10)) { }  /* VBlank 끝 대기 */
    while (MFP_GPIP & 0x10) { }     /* VBlank 시작 대기 */
}
```

**물결 효과 계산:**

```c
/* 사인 테이블 기반 물결 스크롤 */
static void calc_wave_scroll(int amplitude, int frequency, int phase)
{
    for (int i = 0; i < 256; i++) {
        int angle = (i * frequency + phase) & 0xFF;
        scroll_table[i] = (sin_table[angle] * amplitude) >> 7;
    }
}
```

**데모 기능:**
1. **화면 분할** - 상단/하단 반대 방향 스크롤
2. **물결 효과** - 사인파 수평 왜곡
3. **그라데이션** - 스캔라인별 팔레트 변경
4. **다중 분할** - 4개 영역 독립 스크롤
5. **물 반사** - 풍경 + 물결치는 반사
6. **모니터링** - 래스터 인터럽트 통계
7. **인터랙티브** - 파라미터 조절 가능

### 5.31 래스터 인터럽트 예제 실행

```bash
# 빌드
make

# 실행 (run68은 래스터 인터럽트 미지원)
# GUI 에뮬레이터(XM6, XEiJ, px68k)에서 테스트 필요
```

**예상 실행 결과:**
```
========================================
  X68000 Raster Interrupt Demo
========================================

This demo shows raster interrupt effects:
- Split screen scrolling
- Wave distortion
- Per-line palette changes
- Water reflection

Press any key to start...

=== Demo 1: Split Screen Scroll ===
Upper half scrolls left, lower half scrolls right
(상단은 왼쪽으로, 하단은 오른쪽으로 스크롤)

=== Demo 2: Wave Effect ===
Sine wave horizontal distortion
(체크보드 패턴이 물결치듯 왜곡)

=== Demo 3: Raster Palette (Gradient) ===
Color changes per scanline
(애니메이션되는 무지개 그라데이션)

=== Demo 5: Water Ripple Effect ===
Simulating water reflection
(상단: 풍경, 하단: 물결치는 반사)

=== Demo 6: Raster Counter Monitor ===
Raster interrupts:
  Total count: 15360
  Per frame: 256
  Current line: 128

=== Demo 7: Interactive Wave ===
Controls:
  UP/DOWN: Amplitude
  LEFT/RIGHT: Frequency
  Z/X: Speed
Amp: 16  Freq: 8  Speed: 4
```

**효과 원리:**

| 효과 | 원리 |
|------|------|
| 화면 분할 | 특정 라인에서 스크롤 값 변경 |
| 물결 | 매 라인마다 사인파로 X 스크롤 |
| 그라데이션 | 매 라인마다 팔레트 색상 변경 |
| 물 반사 | 하단만 물결 + Y 반전 |

**주의사항:**
- 래스터 인터럽트는 매 스캔라인마다 발생하므로 핸들러는 최대한 빠르게 실행되어야 함
- 복잡한 계산은 메인 루프에서 테이블로 미리 계산
- run68은 래스터 인터럽트를 지원하지 않음

**참고:** run68은 CUI 에뮬레이터로 래스터 인터럽트/그래픽을 지원하지 않습니다.
실제 테스트는 XM6, XEiJ, px68k 같은 GUI 에뮬레이터가 필요합니다.

### 5.32 DMA 전송 예제 (`src/hello_dma.c`)

HD63450 DMA 컨트롤러를 사용한 고속 메모리 전송 예제입니다.

**X68000 DMA 사양:**
- HD63450 DMA 컨트롤러
- 4개의 독립 채널
- 메모리-메모리, 메모리-I/O 전송
- 버스트/사이클스틸 모드
- 8/16/32비트 전송 지원

**DMA 레지스터 맵:**

```c
#define DMAC_BASE       0xE84000
#define DMAC_CH(n)      (DMAC_BASE + (n) * 0x40)  /* 채널별 0x40 오프셋 */

/* 주요 레지스터 오프셋 */
#define DMAC_CSR        0x00    /* Channel Status (상태) */
#define DMAC_CER        0x01    /* Channel Error (에러) */
#define DMAC_DCR        0x04    /* Device Control */
#define DMAC_OCR        0x05    /* Operation Control */
#define DMAC_SCR        0x06    /* Sequence Control */
#define DMAC_CCR        0x07    /* Channel Control */
#define DMAC_MTC        0x0A    /* Memory Transfer Counter */
#define DMAC_MAR        0x0C    /* Memory Address */
#define DMAC_DAR        0x14    /* Device Address */
```

**레지스터 액세스 매크로:**

```c
#define DMAC_REG_B(ch, reg)  (*(volatile unsigned char *)(DMAC_CH(ch) + (reg)))
#define DMAC_REG_W(ch, reg)  (*(volatile unsigned short *)(DMAC_CH(ch) + (reg)))
#define DMAC_REG_L(ch, reg)  (*(volatile unsigned long *)(DMAC_CH(ch) + (reg)))
```

**메모리-메모리 전송 설정:**

```c
static void dma_setup_memcpy(int ch, void *dst, const void *src, int size)
{
    /* 채널 리셋 */
    DMAC_REG_B(ch, DMAC_CCR) = 0x10;  /* Software abort */
    while (DMAC_REG_B(ch, DMAC_CSR) & 0x08) { }  /* Active 대기 */

    /* Device Control: 버스트 모드, 16비트 포트 */
    DMAC_REG_B(ch, DMAC_DCR) = 0x00;

    /* Operation Control: Device->Memory, Long word, Auto request */
    DMAC_REG_B(ch, DMAC_OCR) = 0xA0;

    /* Sequence Control: 양쪽 주소 증가 */
    DMAC_REG_B(ch, DMAC_SCR) = 0x05;

    /* Function Codes: 슈퍼바이저 데이터 */
    DMAC_REG_B(ch, DMAC_MFC) = 0x05;
    DMAC_REG_B(ch, DMAC_DFC) = 0x05;

    /* 전송 카운터 (롱워드 단위) */
    DMAC_REG_W(ch, DMAC_MTC) = size / 4;

    /* 주소 설정 */
    DMAC_REG_L(ch, DMAC_MAR) = (unsigned long)dst;
    DMAC_REG_L(ch, DMAC_DAR) = (unsigned long)src;
}

static void dma_start(int ch)
{
    DMAC_REG_B(ch, DMAC_CCR) = 0x80;  /* Start */
}

static int dma_wait(int ch)
{
    while (1) {
        unsigned char status = DMAC_REG_B(ch, DMAC_CSR);
        if (status & 0x80) return 0;   /* Complete */
        if (status & 0x10) return -1;  /* Error */
    }
}
```

**CSR (Channel Status Register) 비트:**

| 비트 | 이름 | 설명 |
|------|------|------|
| 7 | COC | Channel Operation Complete |
| 6 | BLC | Block Transfer Complete |
| 5 | NDT | Normal Device Termination |
| 4 | ERR | Error |
| 3 | ACT | Channel Active |

**OCR (Operation Control Register):**

| 비트 | 설명 |
|------|------|
| 7 | Direction (0=Mem→Dev, 1=Dev→Mem) |
| 5-4 | Size (00=byte, 01=word, 10=long) |
| 3-2 | Chain mode |
| 1-0 | Request generation |

**SCR (Sequence Control Register):**

| 비트 | 설명 |
|------|------|
| 3-2 | Memory address (00=fixed, 01=inc, 10=dec) |
| 1-0 | Device address (00=fixed, 01=inc, 10=dec) |

**에러 코드 (CER):**

| 코드 | 의미 |
|------|------|
| 0x00 | No error |
| 0x01 | Configuration error |
| 0x02 | Timing error |
| 0x05 | Address error |
| 0x06 | Bus error |
| 0x09 | Software abort |

**데모 기능:**
1. **기본 전송** - 메모리-메모리 DMA 복사 및 검증
2. **속도 비교** - CPU memcpy vs DMA 성능 비교
3. **다중 채널** - 4개 채널 동시 전송
4. **크기별 테스트** - 64~16384 바이트 전송 테스트
5. **레지스터 덤프** - 모든 채널 레지스터 상태 표시
6. **블록 채우기** - 패턴으로 메모리 초기화

### 5.33 DMA 예제 실행

```bash
# 빌드
make

# 실행 (run68은 DMA 미지원)
# GUI 에뮬레이터(XM6, XEiJ, px68k)에서 테스트 필요
```

**예상 실행 결과:**
```
========================================
  X68000 DMA Transfer Demo
========================================

HD63450 DMA Controller Features:
- 4 independent channels
- Memory-to-memory transfer
- Burst and cycle-steal modes
- Up to 8/16/32-bit transfers

=== Demo 1: Basic Memory Transfer ===
Source buffer: 0x00012000
Dest buffer:   0x00013000
Size: 4096 bytes

Initializing source data...
Starting DMA transfer (CH0)...
DMA transfer complete!
  CSR: 0x80 (COC )
Verifying data...
Verification PASSED!

=== Demo 2: CPU vs DMA Speed Comparison ===
Transfer size: 4096 bytes

CPU memcpy (10 iterations)...
  Time: 15 (1/100 sec)
DMA transfer (10 iterations)...
  Time: 8 (1/100 sec)

DMA is 187% faster than CPU

=== Demo 3: Multi-Channel Transfer ===
Using all 4 DMA channels
Setting up CH0...
Setting up CH1...
Setting up CH2...
Setting up CH3...
Starting all channels...
  CH0: OK
  CH1: OK
  CH2: OK
  CH3: OK

Verification:
  CH0: PASSED
  CH1: PASSED
  CH2: PASSED
  CH3: PASSED

=== Demo 4: Transfer Size Test ===
Size (bytes)  Result     Verify
------------  ---------  ------
64            OK         PASS
256           OK         PASS
1024          OK         PASS
4096          OK         PASS
16384         OK         PASS

=== Demo 5: DMA Register Dump ===
General Control Register: 0x00

--- Channel 0 ---
  CSR (Status):    0x80
  CER (Error):     0x00
  DCR (Device):    0x00
  OCR (Operation): 0xA0
  SCR (Sequence):  0x05
  CCR (Control):   0x00
  MTC (Count):     0x0000
  MAR (MemAddr):   0x00013000
  DAR (DevAddr):   0x00012000
```

**DMA 채널 용도 (일반적):**
| 채널 | 용도 |
|------|------|
| CH0 | FDC (플로피 디스크) |
| CH1 | SASI/SCSI 하드디스크 |
| CH2 | ADPCM 사운드 |
| CH3 | 범용 (사용자) |

**주의사항:**
- DMA 전송 중 소스/목적지 메모리 접근 주의
- 채널 우선순위: CH0 > CH1 > CH2 > CH3
- 시스템 장치가 사용 중인 채널 피하기
- run68은 DMA를 지원하지 않음

**참고:** run68은 CUI 에뮬레이터로 DMA를 지원하지 않습니다.
실제 테스트는 XM6, XEiJ, px68k 같은 GUI 에뮬레이터가 필요합니다.

### 5.34 RTC 시계 예제 (`src/hello_rtc.c`)

RP5C15 실시간 클럭을 사용한 날짜/시간 처리 예제입니다.

**X68000 RTC 사양:**
- RP5C15 실시간 클럭
- 배터리 백업 (전원 꺼져도 유지)
- 년/월/일/요일/시/분/초
- 알람 기능
- 윤년 자동 계산

**RTC 레지스터 맵:**

```c
#define RTC_BASE        0xE8A000

/* Bank 0 - 시간 레지스터 */
#define RTC_SEC1        (*(volatile unsigned char *)(RTC_BASE + 0x01))  /* 초 1의 자리 */
#define RTC_SEC10       (*(volatile unsigned char *)(RTC_BASE + 0x03))  /* 초 10의 자리 */
#define RTC_MIN1        (*(volatile unsigned char *)(RTC_BASE + 0x05))  /* 분 1의 자리 */
#define RTC_MIN10       (*(volatile unsigned char *)(RTC_BASE + 0x07))  /* 분 10의 자리 */
#define RTC_HOUR1       (*(volatile unsigned char *)(RTC_BASE + 0x09))  /* 시 1의 자리 */
#define RTC_HOUR10      (*(volatile unsigned char *)(RTC_BASE + 0x0B))  /* 시 10의 자리 */
#define RTC_WEEK        (*(volatile unsigned char *)(RTC_BASE + 0x0D))  /* 요일 (0=일) */
#define RTC_DAY1        (*(volatile unsigned char *)(RTC_BASE + 0x0F))  /* 일 1의 자리 */
#define RTC_DAY10       (*(volatile unsigned char *)(RTC_BASE + 0x11))  /* 일 10의 자리 */
#define RTC_MON1        (*(volatile unsigned char *)(RTC_BASE + 0x13))  /* 월 1의 자리 */
#define RTC_MON10       (*(volatile unsigned char *)(RTC_BASE + 0x15))  /* 월 10의 자리 */
#define RTC_YEAR1       (*(volatile unsigned char *)(RTC_BASE + 0x17))  /* 년 1의 자리 */
#define RTC_YEAR10      (*(volatile unsigned char *)(RTC_BASE + 0x19))  /* 년 10의 자리 */
#define RTC_MODE        (*(volatile unsigned char *)(RTC_BASE + 0x1B))  /* 모드 */
```

**시간 읽기:**

```c
typedef struct {
    int year, month, day, week;
    int hour, min, sec;
} DateTime;

static void rtc_read(DateTime *dt)
{
    dt->sec = (RTC_SEC10 & 0x07) * 10 + (RTC_SEC1 & 0x0F);
    dt->min = (RTC_MIN10 & 0x07) * 10 + (RTC_MIN1 & 0x0F);
    dt->hour = (RTC_HOUR10 & 0x03) * 10 + (RTC_HOUR1 & 0x0F);
    dt->day = (RTC_DAY10 & 0x03) * 10 + (RTC_DAY1 & 0x0F);
    dt->month = (RTC_MON10 & 0x01) * 10 + (RTC_MON1 & 0x0F);
    dt->year = 1980 + (RTC_YEAR10 & 0x0F) * 10 + (RTC_YEAR1 & 0x0F);
    dt->week = RTC_WEEK & 0x07;
}
```

**시간 쓰기:**

```c
static void rtc_write(const DateTime *dt)
{
    int year = dt->year - 1980;

    RTC_SEC1 = dt->sec % 10;
    RTC_SEC10 = dt->sec / 10;
    RTC_MIN1 = dt->min % 10;
    RTC_MIN10 = dt->min / 10;
    RTC_HOUR1 = dt->hour % 10;
    RTC_HOUR10 = dt->hour / 10;
    RTC_DAY1 = dt->day % 10;
    RTC_DAY10 = dt->day / 10;
    RTC_MON1 = dt->month % 10;
    RTC_MON10 = dt->month / 10;
    RTC_YEAR1 = year % 10;
    RTC_YEAR10 = year / 10;
    RTC_WEEK = dt->week;
}
```

**IOCS를 통한 시간 읽기:**

```c
static void rtc_read_iocs(DateTime *dt)
{
    int date = _iocs_bindateget();
    int time = _iocs_timeget();

    /* BINDATEGET: bit31-16=년, bit15-8=월, bit7-0=일 */
    dt->year = (date >> 16) & 0xFFFF;
    dt->month = (date >> 8) & 0xFF;
    dt->day = date & 0xFF;

    /* TIMEGET: bit31-24=요일, bit23-16=시, bit15-8=분, bit7-0=초 (BCD) */
    dt->week = (time >> 24) & 0x07;
    dt->hour = ((time >> 20) & 0x0F) * 10 + ((time >> 16) & 0x0F);
    dt->min = ((time >> 12) & 0x0F) * 10 + ((time >> 8) & 0x0F);
    dt->sec = ((time >> 4) & 0x0F) * 10 + (time & 0x0F);
}
```

**IOCS 시간 함수:**

| 함수 | 설명 |
|------|------|
| `_iocs_bindateget()` | 날짜 읽기 (바이너리) |
| `_iocs_bindateset(date)` | 날짜 설정 |
| `_iocs_timeget()` | 시간 읽기 (BCD) |
| `_iocs_timeset(time)` | 시간 설정 |
| `_iocs_ontime()` | 시스템 가동 시간 |

**요일 계산 (Zeller's Congruence):**

```c
static int calc_weekday(int year, int month, int day)
{
    if (month < 3) { month += 12; year--; }
    int k = year % 100;
    int j = year / 100;
    int h = (day + (13*(month+1))/5 + k + k/4 + j/4 + 5*j) % 7;
    return (h + 6) % 7;  /* 0=일요일 */
}
```

**데모 기능:**
1. **현재 시간** - 직접/IOCS 두 방식으로 읽기 비교
2. **달력** - 월별 달력 표시, 방향키로 이동
3. **스톱워치** - 시작/정지/랩/리셋 기능
4. **레지스터 덤프** - RTC 레지스터 상태 표시
5. **시간 설정** - 방향키로 날짜/시간 설정
6. **디지털 시계** - 큰 글자 시계 표시

### 5.35 RTC 예제 실행

```bash
# 빌드
make

# 실행
make run-hello_rtc
```

**예상 실행 결과:**
```
========================================
  X68000 RTC (Real-Time Clock) Demo
========================================

RP5C15 RTC Features:
- Battery-backed (keeps time when off)
- Year/Month/Day/Week/Hour/Min/Sec
- Alarm function
- Leap year support

=== Demo 1: Current Time ===
Reading RTC... (Press any key to stop)

Direct RTC:  2026-01-12 (Sun) 18:15:30
Via IOCS:    2026-01-12 (Sun) 18:15:30

=== Demo 2: Calendar ===
Use LEFT/RIGHT to change month, ESC to exit

     2026-01
 Su Mo Tu We Th Fr Sa
              1  2  3
  4  5  6  7  8  9 10
 11 12 13 14 15 16 17
 18 19 20 21 22 23 24
 25 26 27 28 29 30 31

Today: 2026-01-12 (Sun) 18:15:45

=== Demo 3: Stopwatch ===
SPACE: Start/Stop, L: Lap, R: Reset, ESC: Exit

  01:23.45  [RUNNING]

Lap times:
  1: 00:15.23
  2: 00:32.10
  3: 01:05.67

=== Demo 5: Set Time ===
Use LEFT/RIGHT to select field, UP/DOWN to change value

[2026]-01-12  18:15:30

Day of week: Sunday

=== Demo 6: Digital Clock ===
+---------------------+
|                     |
|   18 : 15 : 45   |
|                     |
+---------------------+
2026-01-12  Sun
```

**레지스터 구조:**

| 오프셋 | 이름 | 비트 | 설명 |
|--------|------|------|------|
| 0x01 | SEC1 | 0-3 | 초 1의 자리 (0-9) |
| 0x03 | SEC10 | 0-2 | 초 10의 자리 (0-5) |
| 0x05 | MIN1 | 0-3 | 분 1의 자리 (0-9) |
| 0x07 | MIN10 | 0-2 | 분 10의 자리 (0-5) |
| 0x09 | HOUR1 | 0-3 | 시 1의 자리 (0-9) |
| 0x0B | HOUR10 | 0-1 | 시 10의 자리 (0-2) |
| 0x0D | WEEK | 0-2 | 요일 (0=일, 6=토) |
| 0x1B | MODE | 0 | Bank 선택 (0/1) |

**참고:** run68에서도 RTC 기능을 부분적으로 지원합니다.
완전한 테스트는 GUI 에뮬레이터에서 수행하세요.

### 5.36 시리얼 통신 예제 (`src/hello_serial.c`)

RS-232C 시리얼 포트를 사용한 데이터 통신 예제입니다.

**X68000 시리얼 사양:**
- RS-232C 표준 호환
- 8251 USART 호환 인터페이스
- 75 ~ 38400 baud
- 5/6/7/8 데이터 비트
- 1/1.5/2 스톱 비트
- None/Odd/Even 패리티

**_iocs_set232c() 파라미터 구조:**

```c
/*
 * bit 0-1: Stop bits
 *   0 = 1 stop bit
 *   1 = 1.5 stop bits
 *   2 = 2 stop bits
 *
 * bit 2-3: Parity
 *   0 = None
 *   1 = Odd
 *   3 = Even
 *
 * bit 4-5: Data bits
 *   0 = 5 bits, 1 = 6 bits
 *   2 = 7 bits, 3 = 8 bits
 *
 * bit 8-11: Baud rate
 *   0=75, 1=150, 2=300, 3=600
 *   4=1200, 5=2400, 6=4800, 7=9600
 *   8=17361, 9=31250(MIDI), A=38400
 */
#define SERIAL_9600_8N1  0x0730  /* 9600-8-N-1 */
#define SERIAL_38400_8N1 0x0A30  /* 38400-8-N-1 */
```

**IOCS 시리얼 함수:**

| 함수 | 설명 |
|------|------|
| `_iocs_set232c(config)` | 시리얼 포트 설정 |
| `_iocs_out232c(data)` | 1바이트 송신 |
| `_iocs_inp232c()` | 1바이트 수신 |
| `_iocs_osns232c()` | 송신 버퍼 상태 (0=busy) |
| `_iocs_isns232c()` | 수신 데이터 유무 |
| `_iocs_lof232c()` | 수신 버퍼 길이 |

**기본 송수신 함수:**

```c
/* 시리얼 출력 (1바이트) */
static void serial_putc(int c)
{
    while (_iocs_osns232c() == 0) { }  /* 송신 가능 대기 */
    _iocs_out232c(c);
}

/* 시리얼 문자열 출력 */
static void serial_puts(const char *str)
{
    while (*str) {
        serial_putc(*str++);
    }
}

/* 시리얼 입력 (논블로킹) */
static int serial_getc_nb(void)
{
    if (_iocs_isns232c()) {
        return _iocs_inp232c();
    }
    return -1;
}
```

**보드레이트 코드:**

| 코드 | 보드레이트 | 코드 | 보드레이트 |
|------|-----------|------|-----------|
| 0x00 | 75 | 0x05 | 2400 |
| 0x01 | 150 | 0x06 | 4800 |
| 0x02 | 300 | 0x07 | 9600 |
| 0x03 | 600 | 0x08 | 17361 |
| 0x04 | 1200 | 0x0A | 38400 |

**데모 기능:**
1. **기본 송수신** - 테스트 메시지 송신 및 수신
2. **터미널** - 간이 터미널 에뮬레이터
3. **루프백** - TX-RX 연결 테스트 (256바이트)
4. **보드레이트** - 속도 선택 및 적용
5. **데이터 포맷** - 비트/패리티/스톱 설정
6. **버퍼 모니터** - 송수신 버퍼 상태 표시
7. **대용량 전송** - 1KB 전송 속도 측정

### 5.37 시리얼 예제 실행

```bash
# 빌드
make

# 실행 (run68은 시리얼 미지원)
# GUI 에뮬레이터 또는 실제 하드웨어 필요
```

**예상 실행 결과:**
```
========================================
  X68000 Serial Communication Demo
========================================

RS-232C Features:
- 75 to 38400 baud
- 5/6/7/8 data bits
- None/Odd/Even parity
- 1/1.5/2 stop bits

=== Demo 1: Basic Serial I/O ===
Config: 9600-8N1

Sending test message...
Sent 2 lines.
Waiting for input (5 seconds)...
Received: Hello back!

=== Demo 3: Loopback Test ===
Config: 9600-8N1
Connect TX to RX for loopback test

Sending 256 bytes...
Receiving...

Results:
  TX: 256 bytes
  RX: 256 bytes
  Errors: 0
  Status: PASSED

=== Demo 4: Baud Rate Selection ===
Available baud rates:
   75 baud
   150 baud
   ...
 > 9600 baud <
   ...
   38400 baud

Current: 9600-8N1

=== Demo 6: Buffer Status Monitor ===
Config: 9600-8N1

TX Status: Ready
RX Status: Empty
RX Buffer: 0 bytes

TX Count: 46
RX Count: 12

=== Demo 7: Bulk Transfer Test ===
Config: 38400-8N1
Transfer size: 1024 bytes

Starting transfer...
Transfer complete!

Results:
  Bytes sent: 1024
  Time: 28 (1/100 sec)
  Throughput: 3657 bytes/sec
  Theoretical max: ~3840 bytes/sec at 38400 baud
```

**연결 방법:**
```
X68000 RS-232C 포트 (D-Sub 25핀)
Pin 2: TXD (송신)
Pin 3: RXD (수신)
Pin 7: GND (그라운드)
Pin 4: RTS
Pin 5: CTS
Pin 6: DSR
Pin 20: DTR
```

**루프백 테스트:**
```
간단한 루프백: Pin 2 (TXD) <-> Pin 3 (RXD) 연결
```

**PC 연결 (크로스 케이블):**
```
X68000 Pin 2 (TXD) --- PC Pin 3 (RXD)
X68000 Pin 3 (RXD) --- PC Pin 2 (TXD)
X68000 Pin 7 (GND) --- PC Pin 5 (GND)
```

**참고:**
- run68은 RS-232C를 에뮬레이션하지 않음
- 일부 GUI 에뮬레이터(XM6 Pro 등)에서 시리얼 포트 지원
- 실제 X68000 하드웨어에서 완전한 테스트 가능

### 5.38 FDD 디스크 접근 예제 (`src/hello_fdd.c`)

플로피 디스크 드라이브(FDD) 저수준 접근 예제입니다.

**핵심 개념:**
- uPD72065 FDC (Floppy Disk Controller) 레지스터 접근
- IOCS BIOS 콜을 통한 디스크 읽기
- 섹터 주소 지정 (CHS: Cylinder-Head-Sector)
- 부트 섹터 및 BPB (BIOS Parameter Block) 분석

**하드웨어 정보:**
```c
/* FDC 레지스터 주소 */
#define FDC_BASE        0xE94000
#define FDC_STATUS      (*(volatile unsigned char *)(FDC_BASE + 0x00))
#define FDC_DATA        (*(volatile unsigned char *)(FDC_BASE + 0x02))
#define FDC_DRVCTRL     (*(volatile unsigned char *)(0xE94006))

/* FDC 상태 레지스터 비트 */
#define FDC_RQM         0x80    /* Request for Master */
#define FDC_DIO         0x40    /* Data direction (1=read) */
#define FDC_CB          0x10    /* Controller busy */

/* 미디어 타입 코드 */
#define MEDIA_2HD       0x90    /* 2HD (1.2MB) */
#define MEDIA_2DD       0x70    /* 2DD (640KB) */
```

**IOCS B_READ 파라미터 패킹:**
```c
/* mode = (media_type << 8) | drive_number */
static int make_mode(int drive, int media)
{
    return (media << 8) | drive;
}

/* pos = (cylinder << 16) | (head << 8) | sector */
static int make_pos(int cylinder, int head, int sector)
{
    return (cylinder << 16) | (head << 8) | sector;
}

/* 섹터 읽기 예제 */
int mode = make_mode(0, MEDIA_2HD);  /* 드라이브 0, 2HD */
int pos = make_pos(0, 0, 1);          /* C=0, H=0, S=1 */
int result = _iocs_b_read(mode, pos, 1, buffer);
```

**드라이브 상태 확인:**
```c
/* _iocs_b_drvchk(drive, cmd) */
int status = _iocs_b_drvchk(0, 0);  /* 드라이브 0 상태 확인 */

/* 상태 비트 해석 */
if (status & 0x8000) /* 2HD */
if (status & 0x4000) /* 2DD */
if (status & 0x0040) /* Write Protect */
if (status & 0x0020) /* Ready */
if (status & 0x0010) /* Track 0 */
```

**트랙 시크 및 리캘리브레이션:**
```c
/* 리캘리브레이션 (트랙 0으로 이동) */
_iocs_b_recali(0);

/* 특정 트랙으로 시크 */
_iocs_b_seek(0, 38);  /* 드라이브 0, 트랙 38 */
```

**섹터 ID 읽기:**
```c
unsigned char id_buffer[8];
int mode = make_mode(0, MEDIA_2HD);

/* _iocs_b_readid(mode, head, buffer) */
_iocs_b_readid(mode, 0, id_buffer);

/* ID 필드 구조 */
/* id_buffer[0]: Cylinder (트랙 번호) */
/* id_buffer[1]: Head (헤드 번호) */
/* id_buffer[2]: Record (섹터 번호) */
/* id_buffer[3]: N (섹터 크기 코드) */
/*   N=0: 128B, N=1: 256B, N=2: 512B, N=3: 1024B */
```

**X68000 디스크 포맷:**
| 포맷 | 트랙/면 | 면 수 | 섹터/트랙 | 바이트/섹터 | 용량 |
|------|---------|-------|-----------|-------------|------|
| 2HD | 77 | 2 | 8 | 1024 | 1232KB |
| 2DD | 80 | 2 | 8 | 512 | 640KB |
| 2HC | 80 | 2 | 18 | 512 | 1440KB |

**미디어 디스크립터:**
| 값 | 포맷 |
|-----|------|
| 0xFE | 2HD (X68000 기본) |
| 0xFD | 2DD 8섹터 |
| 0xF9 | 2DD 9섹터 / 2HC |
| 0xF0 | 2HC 18섹터 |

**BPB (BIOS Parameter Block) 구조 (부트 섹터):**
```c
/* 부트 섹터 구조 */
/* Offset 0-2:   점프 명령어 */
/* Offset 3-10:  OEM 이름 */
/* Offset 11-12: 바이트/섹터 */
/* Offset 13:    섹터/클러스터 */
/* Offset 14-15: 예약 섹터 수 */
/* Offset 16:    FAT 개수 */
/* Offset 17-18: 루트 디렉토리 엔트리 수 */
/* Offset 19-20: 총 섹터 수 */
/* Offset 21:    미디어 디스크립터 */
/* Offset 22-23: 섹터/FAT */
/* Offset 24-25: 섹터/트랙 */
/* Offset 26-27: 헤드 수 */
```

**데모 프로그램:**
1. 드라이브 상태 확인 - 드라이브 존재 여부 및 상태 체크
2. 부트 섹터 읽기 - BPB 정보 표시
3. 섹터 헥스 덤프 - 섹터 데이터 16진수 표시
4. 트랙 시크 테스트 - 헤드 이동 테스트
5. 섹터 ID 읽기 - 디스크의 섹터 ID 정보 표시
6. FDC 레지스터 상태 - 컨트롤러 레지스터 값 표시
7. 트랙 섹터 읽기 - 트랙의 모든 섹터 읽기
8. 디스크 포맷 정보 - X68000 디스크 포맷 설명

### 5.39 FDD 예제 실행

**컴파일:**
```bash
make build/hello_fdd.x
```

**실행 (에뮬레이터):**
```bash
./run68 build/hello_fdd.x
```

**예상 출력 (메뉴):**
```
=== X68000 FDD (Floppy Disk) Access Demo ===

This demo shows low-level floppy disk access.
WARNING: Insert a non-important disk for testing!

Demo programs:
  1. Drive Status Check
  2. Read Boot Sector (BPB)
  3. Sector Hex Dump
  4. Track Seek Test
  5. Read Sector IDs
  6. FDC Register Status
  7. Read Track Sectors
  8. Disk Format Info

  0. Exit

Select demo (0-8):
```

**드라이브 상태 확인 출력:**
```
=== Demo 1: Drive Status Check ===

Checking floppy drive status...

Drive 0: Status=0x9020
  [2HD] [Ready]
Drive 1: Not present or error (-1)
Drive 2: Not present or error (-1)
Drive 3: Not present or error (-1)

FDC Main Status Register: 0x80
  RQM=1 DIO=0 NDM=0 CB=0

Press any key...
```

**부트 섹터 읽기 출력:**
```
=== Demo 2: Read Boot Sector ===

Reading boot sector from Drive 0...
(Insert disk if not present)

Boot sector read successfully!

--- Disk Information (BPB) ---
Jump Code:      60 3C 90
OEM Name:       Human68k
Bytes/Sector:   1024
Sectors/Cluster:1
Reserved Sects: 1
Number of FATs: 2
Root Entries:   192
Total Sectors:  1232
Media Desc:     0xFE (2HD)
Sectors/FAT:    2
Sectors/Track:  8
Number of Heads:2

Press any key...
```

**섹터 헥스 덤프 출력:**
```
=== Demo 3: Sector Hex Dump ===

Reading C=0 H=0 S=1 from Drive 0...

First 256 bytes:

     00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
     -----------------------------------------------
00: 60 3C 90 48 75 6D 61 6E 36 38 6B 00 04 01 01 00 |`<.Human68k.....|
10: 02 C0 00 D0 04 FE 02 00 08 00 02 00 00 00 00 00 |................|
20: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |................|
...

Press any key...
```

**디스크 포맷 정보 출력:**
```
=== Demo 8: X68000 Disk Formats ===

2HD Format (1.2MB / 1232KB):
  Tracks/Side:     77
  Sides:           2
  Sectors/Track:   8
  Bytes/Sector:    1024
  Total Sectors:   1232
  Total Capacity:  1232 KB

2DD Format (640KB):
  Tracks/Side:     80
  Sides:           2
  Sectors/Track:   8
  Bytes/Sector:    512
  Total Sectors:   1280
  Total Capacity:  640 KB

2HC Format (1.44MB) [PC compatible]:
  Tracks/Side:     80
  Sides:           2
  Sectors/Track:   18
  Bytes/Sector:    512
  Total Sectors:   2880
  Total Capacity:  1440 KB

Media Descriptor bytes:
  0xFE - 2HD (X68000 native)
  0xFD - 2DD 8-sector
  0xF9 - 2DD 9-sector / 2HC
  0xF0 - 2HC 18-sector

Press any key...
```

**참고:**
- run68은 실제 FDD를 에뮬레이션하지 않으므로 읽기 에러 발생 가능
- XM6 등 GUI 에뮬레이터에서 디스크 이미지(.XDF, .D88) 마운트 후 테스트
- 실제 X68000 하드웨어에서 완전한 기능 테스트 가능
- **주의: 디스크 쓰기 작업은 데이터 손실 위험! 테스트용 디스크 사용 권장**

### 5.40 SCSI 하드디스크 예제 (`src/hello_scsi.c`)

SCSI 하드디스크 저수준 접근 예제입니다.

**핵심 개념:**
- MB89352 (SPC) SCSI 컨트롤러 레지스터 접근
- IOCS SCSI 콜을 통한 장치 제어
- SCSI 프로토콜 (Inquiry, Read Capacity, Read, Test Unit Ready)
- LBA (Logical Block Addressing) 섹터 접근

**하드웨어 정보:**
```c
/* SCSI 컨트롤러 레지스터 주소 */
#define SCSI_BASE       0xEA0000
#define SPC_BDID        (*(volatile unsigned char *)(SCSI_BASE + 0x01))  /* Bus Device ID */
#define SPC_SCTL        (*(volatile unsigned char *)(SCSI_BASE + 0x03))  /* SPC Control */
#define SPC_SCMD        (*(volatile unsigned char *)(SCSI_BASE + 0x05))  /* Command */
#define SPC_INTS        (*(volatile unsigned char *)(SCSI_BASE + 0x09))  /* Interrupt Sense */
#define SPC_PSNS        (*(volatile unsigned char *)(SCSI_BASE + 0x0B))  /* Phase Sense */
#define SPC_SSTS        (*(volatile unsigned char *)(SCSI_BASE + 0x0D))  /* SPC Status */
#define SPC_SERR        (*(volatile unsigned char *)(SCSI_BASE + 0x0F))  /* SPC Error */
```

**SCSI 모드 파라미터:**
```c
/* mode = (LUN << 3) | SCSI_ID */
static int make_scsi_mode(int id, int lun)
{
    return (lun << 3) | (id & 0x07);
}
```

**IOCS SCSI 함수:**
```c
/* 장치 정보 조회 */
struct iocs_inquiry inquiry;
_iocs_s_inquiry(make_scsi_mode(0, 0), 36, &inquiry);

/* 용량 조회 */
struct iocs_readcap capacity;
_iocs_s_readcap(make_scsi_mode(0, 0), &capacity);
/* capacity.block = 마지막 LBA */
/* capacity.size = 블록 크기 (바이트) */

/* 섹터 읽기 */
/* _iocs_s_read(lba_high, lba_low, mode, count, buffer) */
_iocs_s_read(0, 0, make_scsi_mode(0, 0), 1, buffer);

/* 장치 준비 확인 */
int result = _iocs_s_testunit(make_scsi_mode(0, 0));

/* 센스 데이터 요청 */
_iocs_s_request(make_scsi_mode(0, 0), 18, sense_buffer);
```

**SCSI Inquiry 응답 구조:**
| 오프셋 | 필드 | 설명 |
|--------|------|------|
| 0 | Device Type | 0=HDD, 5=CD-ROM |
| 1 | Removable | bit7: 이동식 미디어 |
| 2 | Version | SCSI 버전 |
| 4 | Additional Length | 추가 데이터 길이 |
| 8-15 | Vendor | 제조사 이름 |
| 16-31 | Product | 제품명 |
| 32-35 | Revision | 펌웨어 버전 |

**SCSI ID 배치 (일반적):**
| ID | 장치 |
|----|------|
| 0 | 주 하드디스크 |
| 1 | 보조 하드디스크 |
| 2-5 | 기타 장치 |
| 6 | CD-ROM |
| 7 | 호스트 어댑터 |

**센스 키 (Sense Key):**
| 값 | 의미 |
|----|------|
| 0x0 | No Sense |
| 0x2 | Not Ready |
| 0x3 | Medium Error |
| 0x4 | Hardware Error |
| 0x5 | Illegal Request |
| 0x6 | Unit Attention |

**데모 프로그램:**
1. SCSI 장치 스캔 - 버스의 모든 장치 검색
2. 장치 정보 상세 - Inquiry 데이터 표시
3. 용량 조회 - 디스크 크기 계산
4. 섹터 읽기 - LBA 0 (MBR) 헥스 덤프
5. Test Unit Ready - 장치 준비 상태 확인
6. SCSI 레지스터 - 컨트롤러 레지스터 덤프
7. Request Sense - 센스 데이터 조회
8. SCSI 정보 - X68000 SCSI 시스템 설명

### 5.41 SCSI 예제 실행

**컴파일:**
```bash
make build/hello_scsi.x
```

**실행 (에뮬레이터):**
```bash
./run68 build/hello_scsi.x
```

**예상 출력 (메뉴):**
```
=== X68000 SCSI Hard Disk Access Demo ===

This demo shows SCSI device access.
Read-only operations for safety.

Demo programs:
  1. SCSI Device Scan
  2. Device Inquiry Details
  3. Read Capacity
  4. Read Sector (LBA 0)
  5. Test Unit Ready
  6. SCSI Controller Registers
  7. Request Sense
  8. SCSI Information

  0. Exit

Select demo (0-8):
```

**SCSI 장치 스캔 출력:**
```
=== Demo 1: SCSI Device Scan ===

Scanning SCSI bus for devices...
(Internal SCSI: ID 0-7)

ID  Status      Type        Vendor/Model
--  ------      ----        ------------
 0  Found       Disk        QUANTUM FIREBALL
 1  Not found
 2  Not found
 3  Not found
 4  Not found
 5  Not found
 6  Found       CD-ROM      SONY CDU-561
 7  Not found

Note: ID 7 is usually the host adapter

Press any key...
```

**용량 조회 출력:**
```
=== Demo 3: Read Capacity ===

Reading capacity from SCSI ID 0...

Disk Information:
-----------------

Last LBA:        2056319 (0x001F5FFF)
Total Blocks:    2056320
Block Size:      512 bytes
Total Capacity:  1004 MB
                 0.9 GB

Press any key...
```

**SCSI 정보 출력:**
```
=== Demo 8: X68000 SCSI Information ===

X68000 SCSI System:
-------------------

Internal SCSI Controller:
  Chip:     MB89352 (SPC)
  Address:  0xEA0000
  IRQ:      Level 1 (SCSI)
  DMA:      Channel 0
  Host ID:  7 (default)

Supported Devices:
  - Hard Disks (40MB - 2GB typical)
  - CD-ROM Drives
  - MO Drives (128MB, 230MB, 640MB)
  - Tape Drives
  - Scanners

SCSI Commands via IOCS:
  _iocs_s_inquiry()  - Get device info
  _iocs_s_testunit() - Check if ready
  _iocs_s_read()     - Read sectors
  _iocs_s_write()    - Write sectors
  _iocs_s_readcap()  - Get capacity
  _iocs_s_request()  - Request sense

Common SCSI IDs:
  ID 0: Primary HDD
  ID 1: Secondary HDD
  ID 2-5: Other devices
  ID 6: CD-ROM (common)
  ID 7: Host Adapter

Press any key...
```

**참고:**
- run68은 SCSI를 에뮬레이션하지 않으므로 장치 미발견 상태
- XM6 등 GUI 에뮬레이터에서 HDD 이미지 마운트 후 테스트
- 실제 X68000 하드웨어에서 완전한 기능 테스트 가능
- **주의: 디스크 쓰기 작업은 데이터 손실 위험!**

### 5.42 프린터 출력 예제 (`src/hello_printer.c`)

Centronics 패럴렐 프린터 출력 예제입니다.

**핵심 개념:**
- 프린터 상태 확인 (BUSY, ONLINE, PAPER OUT)
- 바이트 단위 출력
- 제어 코드 (CR, LF, FF, TAB)
- ESC/P 명령어 시퀀스

**하드웨어 정보:**
```c
/* 프린터 포트 레지스터 주소 */
#define PRN_BASE        0xE8C000
#define PRN_DATA        (*(volatile unsigned char *)(PRN_BASE + 0x01))
#define PRN_STROBE      (*(volatile unsigned char *)(PRN_BASE + 0x03))

/* 프린터 상태 비트 */
#define PRN_BUSY        0x01    /* 프린터 바쁨 */
#define PRN_ONLINE      0x02    /* 온라인 */
#define PRN_PAPEROUT    0x04    /* 용지 없음 */
#define PRN_ERROR       0x08    /* 에러 */
```

**제어 코드:**
```c
#define PRN_CR          0x0D    /* Carriage Return */
#define PRN_LF          0x0A    /* Line Feed */
#define PRN_FF          0x0C    /* Form Feed (페이지 배출) */
#define PRN_HT          0x09    /* Horizontal Tab */
#define PRN_BS          0x08    /* Backspace */
#define PRN_ESC         0x1B    /* Escape (명령어용) */
```

**IOCS 프린터 함수:**
```c
/* 프린터 상태 확인 */
int status = _iocs_snsprn();
/* 반환값: 0 = Ready, 양수 = 상태 비트, 음수 = 에러 */

/* 바이트 출력 */
_iocs_outlpt(char_code);  /* void 반환 */
```

**프린터 출력 함수 구현:**
```c
static int printer_putc(int c)
{
    int status;
    int retry = 100;

    /* 프린터 준비 대기 */
    while (retry > 0) {
        status = _iocs_snsprn();
        if (status == 0) {
            _iocs_outlpt(c);
            return 0;
        }
        if (status < 0) return status;
        retry--;
    }

    _iocs_outlpt(c);
    return 0;
}
```

**ESC/P 명령어 (EPSON 호환):**
| 명령 | 설명 |
|------|------|
| ESC @ | 프린터 초기화 |
| ESC E | 볼드 ON |
| ESC F | 볼드 OFF |
| ESC 4 | 이탤릭 ON |
| ESC 5 | 이탤릭 OFF |
| ESC -1 | 밑줄 ON |
| ESC -0 | 밑줄 OFF |
| ESC M | 엘리트 (12 cpi) |
| ESC P | 피카 (10 cpi) |

**Centronics 커넥터 핀아웃 (주요 신호):**
| 핀 | 신호 | 방향 |
|----|------|------|
| 1 | /STROBE | 출력 |
| 2-9 | DATA 0-7 | 출력 |
| 10 | /ACK | 입력 |
| 11 | BUSY | 입력 |
| 12 | PE (Paper End) | 입력 |
| 13 | SELECT | 입력 |
| 32 | /ERROR | 입력 |

**데모 프로그램:**
1. 프린터 상태 확인 - 상태 비트 표시
2. 테스트 문자 출력 - 알파벳, 숫자, 기호
3. 텍스트 메시지 출력 - 테스트 페이지 인쇄
4. 제어 코드 테스트 - CR, LF, TAB 사용
5. ESC/P 명령어 - 볼드, 이탤릭 테스트
6. ASCII 테이블 출력 - 문자 표 인쇄
7. Form Feed - 페이지 배출
8. 프린터 정보 - X68000 프린터 시스템 설명

### 5.43 프린터 예제 실행

**컴파일:**
```bash
make build/hello_printer.x
```

**실행 (에뮬레이터):**
```bash
./run68 build/hello_printer.x
```

**예상 출력 (메뉴):**
```
=== X68000 Printer Output Demo ===

This demo shows printer port access.
Connect a Centronics printer to test.

Demo programs:
  1. Printer Status Check
  2. Print Test Characters
  3. Print Text Message
  4. Printer Control Codes
  5. ESC/P Commands
  6. Print ASCII Table
  7. Form Feed (Page Eject)
  8. Printer Information

  0. Exit

Select demo (0-8):
```

**프린터 상태 확인 출력:**
```
=== Demo 1: Printer Status Check ===

Checking printer status...

IOCS snsprn() result: 0 (0x00)

Status Decode:
--------------
  Printer is READY

Printer port address: 0xE8C000
Interface: Centronics parallel

Press any key...
```

**텍스트 출력 예 (인쇄물):**
```
================================
    X68000 Printer Test Page
================================

Hello from X68000!
This is a test print from the
hello_printer.c example program.

System: SHARP X68000
Printer: Centronics Parallel

0----+----1----+----2----+----3
================================
```

**프린터 정보 출력:**
```
=== Demo 8: X68000 Printer Information ===

X68000 Printer Interface:
-------------------------

Port Type:    Centronics Parallel
I/O Address:  0xE8C000
Connector:    36-pin Amphenol

IOCS Functions:
  _iocs_snsprn()  - Get status
  _iocs_outlpt()  - Output byte

Compatible Printers:
  - EPSON ESC/P series
  - NEC PC-PR series
  - Canon BJ series
  - Most Centronics printers

Press any key...
```

**참고:**
- run68은 프린터를 에뮬레이션하지 않음
- XM6 등 일부 GUI 에뮬레이터에서 파일로 출력 가능
- 실제 X68000 하드웨어에서 완전한 기능 테스트 가능
- ESC/P 명령어는 EPSON 호환 프린터에서만 동작

### 5.44 CRTC 화면 모드 예제 (`src/hello_crtc.c`)

화면 해상도 및 색상 모드 전환 예제입니다.

**핵심 특징:**
- CRTC 레지스터 직접 접근 (0xE80000)
- 다양한 해상도: 256x256, 512x256, 512x512, 768x512
- 색상 모드: 16색, 256색, 65536색
- 하드웨어 스크롤 데모
- GVRAM 직접 접근 (0xC00000)

**주요 코드:**

```c
/* CRTC 레지스터 정의 */
#define CRTC_BASE       0xE80000
#define CRTC_R00        (*(volatile unsigned short *)(CRTC_BASE + 0x00))  /* H Total */
#define CRTC_R12        (*(volatile unsigned short *)(CRTC_BASE + 0x18))  /* GFX0 X Scroll */
#define CRTC_R13        (*(volatile unsigned short *)(CRTC_BASE + 0x1A))  /* GFX0 Y Scroll */
#define GVRAM_BASE      0xC00000

/* 화면 모드 조회/설정 */
int mode = _iocs_crtmod(-1);  /* 현재 모드 조회 */
_iocs_crtmod(8);               /* 512x512 16색 설정 */
_iocs_g_clr_on();              /* 그래픽 화면 클리어 및 활성화 */

/* GVRAM 직접 접근으로 그리기 */
static void draw_hline_16(int y, int x1, int x2, int color)
{
    volatile unsigned short *gvram = (volatile unsigned short *)GVRAM_BASE;
    int x;
    for (x = x1; x <= x2; x++) {
        gvram[y * 512 + x] = color;
    }
}

/* 하드웨어 스크롤 */
CRTC_R12 = scroll_x;  /* X 스크롤 */
CRTC_R13 = scroll_y;  /* Y 스크롤 */
```

**화면 모드 번호:**

| 모드 | 해상도 | 색상 | GFX 페이지 | 비고 |
|------|--------|------|------------|------|
| 0 | 256x256 | 16 | 4 | 저해상도 |
| 1 | 256x256 | 256 | 2 | |
| 3 | 256x256 | 65536 | 1 | 트루컬러 |
| 4 | 512x256 | 16 | 4 | 와이드 |
| 5 | 512x256 | 256 | 2 | |
| 7 | 512x256 | 65536 | 1 | 트루컬러 |
| 8 | 512x512 | 16 | 4 | 표준 |
| 9 | 512x512 | 256 | 2 | |
| 11 | 512x512 | 65536 | 1 | 트루컬러 |
| 12 | 768x512 | 16 | 2 | PRO/EXPERT 전용 |

**데모 프로그램:**

1. **현재 모드 정보** - CRTC 레지스터 값 표시
2. **화면 모드 목록** - 사용 가능한 모드 설명
3. **512x512 16색** - 컬러 밴드 및 도형 그리기
4. **512x512 256색** - 무지개 그라데이션 팔레트
5. **256x256 65536색** - 트루컬러 그라데이션
6. **CRTC 레지스터 덤프** - 모든 레지스터 값
7. **하드웨어 스크롤** - CRTC 스크롤 레지스터 애니메이션
8. **CRTC 정보** - X68000 비디오 시스템 설명

---

### 5.45 CRTC 예제 실행

**컴파일:**
```bash
make build/hello_crtc.x
```

**실행 (에뮬레이터):**
```bash
./run68 build/hello_crtc.x
```

**예상 출력 (메뉴):**
```
=== X68000 CRTC Screen Mode Demo ===

This demo shows screen mode switching.
Some modes may not work on all monitors.

Demo programs:
  1. Current Mode Information
  2. Available Screen Modes
  3. 512x512 16 Colors
  4. 512x512 256 Colors
  5. 256x256 65536 Colors
  6. CRTC Register Dump
  7. Hardware Scroll Test
  8. CRTC Information

  0. Exit

Select demo (0-8):
```

**현재 모드 정보 출력:**
```
=== Demo 1: Current Screen Mode ===

Current CRTMOD: 16
Mode Name: 512x512 16col

CRTC Register Values:
---------------------
R00 (H Total):     91
R01 (H Sync End):  9
R02 (H Disp Start):17
R03 (H Disp End):  81
R04 (V Total):     567
R05 (V Sync End):  5
R06 (V Disp Start):40
R07 (V Disp End):  552

Calculated Display:
  H Pixels: 512
  V Lines:  512

R20 (Control): 0x0000

Press any key...
```

**CRTC 레지스터 덤프 출력:**
```
=== Demo 6: CRTC Register Dump ===

Address   Name            Value
-------   ----            -----
E80000    R00 H Total     0x005B
E80002    R01 H Sync      0x0009
E80004    R02 H DispS     0x0011
E80006    R03 H DispE     0x0051
E80008    R04 V Total     0x0237
E8000A    R05 V Sync      0x0005
E8000C    R06 V DispS     0x0028
E8000E    R07 V DispE     0x0228
E80010    R08 Adjust      0x0000
E80012    R09 Raster      0x0000
E80028    R20 Control     0x0000
E8002E    R23 Mode        0x0000

Video Controller:
E82400    Screen Mode     0x0008
E82402    Priority        0x0000

Press any key...
```

**참고:**
- 화면 모드 전환 시 모니터가 재동기화됨
- run68은 그래픽 모드를 완전히 지원하지 않음
- XM6, XEiJ 등 GUI 에뮬레이터에서 그래픽 데모 확인 가능
- 768x512 모드는 X68000 PRO/EXPERT 또는 31kHz 지원 모니터 필요
- 원래 화면 모드는 프로그램 종료 시 자동 복원됨

---

## 6. 라이브러리 환경 비교

### Newlib (기본값)
- 현대적인 임베디드 시스템용 C 라이브러리
- C++ 지원
- UTF-8 소스 코드 기본
- Shift-JIS 소스: `-finput-charset=cp932` 플래그 필요

### XC 라이브러리 (install-xclib.sh 실행 후)
- SHARP C Compiler PRO-68K v2.1 호환
- X68000 네이티브 라이브러리와 완벽한 호환성
- X-BASIC 컴파일 지원

라이브러리 선택:
```bash
# Newlib 사용 (기본)
m68k-xelf-gcc -o program.x source.c

# XC 라이브러리 사용
m68k-xelf-gcc -specs=xclib -o program.x source.c
```

---

## 7. 추가 컴파일 옵션

### CPU 타겟 지정

```bash
# 68000 (기본, 모든 X68000 호환)
m68k-xelf-gcc -m68000 -o hello.x hello.c

# 68030 (X68030, XVI 이상)
m68k-xelf-gcc -m68030 -o hello.x hello.c

# 68040/68060 (가속기 탑재)
m68k-xelf-gcc -m68040 -o hello.x hello.c
m68k-xelf-gcc -m68060 -o hello.x hello.c
```

### 최적화 옵션

```bash
# 크기 최적화
m68k-xelf-gcc -Os -o hello.x hello.c

# 속도 최적화
m68k-xelf-gcc -O2 -o hello.x hello.c
m68k-xelf-gcc -O3 -o hello.x hello.c
```

### 메모리 설정 (Newlib)

소스 코드에서 스택/힙 크기 지정:
```c
int _stack_size = 128 * 1024;   // 128KB 스택
int _heap_size = 256 * 1024;    // 256KB 힙
```

---

## 8. 참고 자료

### 크로스 컴파일러 / 툴체인
- [elf2x68k GitHub](https://github.com/yunkya2/elf2x68k) - 메인 크로스 컴파일러 저장소
- [xdev68k GitHub](https://github.com/yosshin4004/xdev68k) - 대안 개발 환경
- [Lydux GitHub](https://github.com/Lydux) - X68000 개발 도구 및 라이브러리

### 튜토리얼 / 강좌
- [X68000 Programming Tutorial](https://federicotech.wordpress.com/2021/12/18/x68000-programming-chapter-2-1-setting-up-the-ide-lydux-flavour/) - IDE 설정 가이드 (Lydux 방식)
- [X68KTutorials GitHub](https://github.com/FedericoTech/X68KTutorials) - X68000 프로그래밍 튜토리얼 예제 코드
- 로컬 복사본: `../../Toolchain/x68000/X68KTutorials`
- [NFG Forums - X68000 Development](https://nfggames.com/forum2/index.php?topic=5417.0) - X68000 개발 관련 포럼 스레드

### 에뮬레이터
- [px68k GitHub](https://github.com/hissorii/px68k) - Portable X68000 에뮬레이터
- [px68k-libretro GitHub](https://github.com/libretro/px68k-libretro) - RetroArch/Libretro용 X68000 에뮬레이터 코어
- [run68x GitHub](https://github.com/kg68k/run68x) - Human68k CUI 에뮬레이터

### 기술 문서 / 아카이브
- [X68000 Library](http://retropc.net/x68000/) - X68000 아카이브
- [Inside X68000](http://www.x68.jp/) - X68000 기술 문서

---

## 9. 실제 설치 및 컴파일 기록

### 9.\1 툴체인 설치 (2026-01-12 실행)

```bash
# 1. GitHub API로 최신 릴리스 URL 확인
curl -sL "https://api.github.com/repos/yunkya2/elf2x68k/releases/latest" | grep '"browser_download_url"'
# 결과: elf2x68k-Linux-20251124.tar.bz2

# 2. 프로젝트 루트의 Toolchain/x68000/toolchain 폴더에 다운로드
mkdir -p ../../Toolchain/x68000/toolchain
cd ../../Toolchain/x68000/toolchain
wget "https://github.com/yunkya2/elf2x68k/releases/download/20251124/elf2x68k-Linux-20251124.tar.bz2" -O elf2x68k.tar.bz2

# 3. 압축 해제 (tar.bz2 형식)
tar xjf elf2x68k.tar.bz2

# 4. 설치 확인
ls -la m68k-xelf/bin/
```

**설치된 툴체인 구조:**
```
Toolchain/x68000/toolchain/
├── elf2x68k.tar.bz2          (108MB, 아카이브)
└── m68k-xelf/
    ├── bin/
    │   ├── m68k-xelf-gcc     (GCC 13.4.0)
    │   ├── m68k-xelf-g++
    │   ├── m68k-xelf-as
    │   ├── m68k-xelf-ld
    │   ├── elf2x68k.py
    │   └── ...
    ├── lib/
    ├── include/
    └── m68k-elf/
```

**컴파일러 버전 확인:**
```bash
$ ../../Toolchain/x68000/toolchain/m68k-xelf/bin/m68k-xelf-gcc --version
m68k-xelf-gcc (elf2x68k) 13.4.0
Copyright (C) 2023 Free Software Foundation, Inc.
```

### 9.\1 Hello World 컴파일 (2026-01-12 실행)

```bash
# 1. PATH 설정 (중요: 링커가 PATH에서 다른 도구를 찾음)
export PATH=/mnt/USERS/onion/DATA_ORIGN/Workspace/05_RetroDeveloperEnvironmentProject/Toolchain/x68000/toolchain/m68k-xelf/bin:$PATH

# 2. 빌드 디렉토리 생성
mkdir -p build

# 3. 컴파일
m68k-xelf-gcc -O2 -o build/hello.x src/hello.c

# 4. 결과 확인
ls -la build/
```

**컴파일 결과:**
```
build/
├── hello.x       (42,336 bytes) - X68000 실행 파일
└── hello.x.elf   (437,696 bytes) - ELF 파일 (디버그 정보 포함)
```

**X 포맷 파일 헤더 확인:**
```bash
$ hexdump -C build/hello.x | head -2
00000000  48 55 00 00 00 00 00 00  00 00 00 00 00 00 88 d4  |HU..............|
00000010  00 00 07 1c 00 00 05 9c  00 00 08 1e 00 00 0d 12  |................|
```
- `48 55` = "HU" 매직 넘버 → Human68k X 포맷 실행 파일 확인 ✅

### 9.\1 주의사항

1. **PATH 설정 필수**: 컴파일 시 `Toolchain/x68000/toolchain/m68k-xelf/bin`이 포함되어 있어야 함
   - 링커 스크립트가 `m68k-xelf-ld.bfd` 등을 PATH에서 찾음
   - PATH 없이 절대 경로로 gcc만 호출하면 링크 실패

2. **파일명 규칙**:
   - `-o output.x` 형식으로 지정하면 자동으로 `.x` (X포맷)와 `.x.elf` (ELF) 두 파일 생성

3. **run68 미포함**:
   - elf2x68k 배포판에 run68 에뮬레이터가 포함되어 있지 않음
   - 별도 설치 필요 (아래 8.4 참조)

### 9.\1 run68x 설치 (2026-01-12 실행)

run68x는 Human68k CUI 에뮬레이터로, X68000용 실행 파일(.x)을 Linux에서 직접 실행할 수 있습니다.

**저장소:** [kg68k/run68x](https://github.com/kg68k/run68x)

#### 사전 요구사항

```bash
sudo apt install build-essential cmake git
```

#### 빌드 과정

```bash
# 1. 저장소 클론
git clone https://github.com/kg68k/run68x.git
cd run68x

# 2. CMakeLists.txt 수정 (GCC 12+ 호환성 문제 해결)
# 62번 라인의 -Werror 제거 및 -Wno-maybe-uninitialized 추가
# 변경 전: -funsigned-char -O3 -Wall -Wextra -Werror -Wno-unused-parameter
# 변경 후: -funsigned-char -O3 -Wall -Wextra -Wno-unused-parameter -Wno-maybe-uninitialized

# 3. 빌드
cmake -B build
cmake --build build

# 4. toolchain에 복사
cp build/run68 ../../../Toolchain/x68000/toolchain/m68k-xelf/bin/
```

**빌드 결과:**
```
run68x/build/run68    (236KB) - Human68k CUI 에뮬레이터
```

#### GCC 12+ 빌드 오류 해결

최신 GCC에서 `-Werror`로 인해 빌드 실패 시:

```bash
# CMakeLists.txt 62번 라인 수정
sed -i 's/-Werror/-Wno-maybe-uninitialized/g' CMakeLists.txt
```

### 9.\1 Hello World 실행 테스트 (2026-01-12 실행)

```bash
# PATH 설정
export PATH=/mnt/USERS/onion/DATA_ORIGN/Workspace/05_RetroDeveloperEnvironmentProject/Toolchain/x68000/toolchain/m68k-xelf/bin:$PATH

# 실행
$ run68 build/hello.x
Hello, X68000 World!
```

**실행 성공 ✅**

### 9.\1 최종 프로젝트 구조

```
05_RetroDeveloperEnvironmentProject/
├── Emulator/x68000/
│   ├── README.md                   (본 문서)
│   ├── src/
│   │   └── hello.c                 (소스 코드)
│   ├── build/
│   │   ├── hello.x                 (X68000 실행 파일)
│   │   └── hello.x.elf             (ELF 디버그 파일)
│   └── run68x/                     (run68x 소스, 빌드용)
└── Toolchain/x68000/toolchain/
    └── m68k-xelf/
        └── bin/
            ├── m68k-xelf-gcc       (크로스 컴파일러)
            ├── run68               (Human68k 에뮬레이터)
            └── ...
```

---

## 10. 체크리스트

- [x] m68k-xelf-gcc 설치 완료 (2026-01-12)
- [x] hello.c 소스 코드 작성 (2026-01-12)
- [x] 컴파일 및 hello.x 생성 (2026-01-12)
- [x] run68x 빌드 및 설치 (2026-01-12)
- [x] run68으로 hello.x 실행 확인 (2026-01-12)
- [x] IOCS 콜 예제 작성 및 테스트 (2026-01-12)
- [x] DOS 콜 예제 작성 및 테스트 (2026-01-12)
- [x] 그래픽 출력 예제 작성 (2026-01-12)
- [x] 사운드 출력 예제 작성 (2026-01-12)
- [ ] 환경 변수 영구 설정 (~/.bashrc)

---

## 변경 이력

| 날짜 | 내용 |
|------|------|
| 2026-01-12 | 초기 문서 작성 |
| 2026-01-12 | 실제 설치 및 컴파일 기록 추가 (섹션 9) |
| 2026-01-12 | run68x 설치 및 실행 테스트 기록 추가 (섹션 9.4~9.6) |
| 2026-01-12 | Makefile 자동화 및 사용법 추가 (섹션 3.3~3.5) |
| 2026-01-12 | 참고 자료 확장 - 튜토리얼, 에뮬레이터 링크 추가 (섹션 8) |
| 2026-01-12 | IOCS/DOS 콜 예제 추가 (섹션 5.1~5.3) |
| 2026-01-12 | 그래픽 출력 예제 추가 (섹션 5.4~5.5) |
| 2026-01-12 | 사운드 출력 예제 추가 (섹션 5.6~5.7) |
| 2026-01-12 | FDD 디스크 접근 예제 추가 (섹션 5.38~5.39) |
| 2026-01-12 | SCSI 하드디스크 예제 추가 (섹션 5.40~5.41) |
| 2026-01-12 | 프린터 출력 예제 추가 (섹션 5.42~5.43) |
| 2026-01-12 | CRTC 화면 모드 예제 추가 (섹션 5.44~5.45) |
