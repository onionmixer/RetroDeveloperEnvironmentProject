# X68000 Human68k 예제 프로그램 요약

이 문서는 X68000 개발 환경에서 제공하는 예제 프로그램들을 요약한 것입니다.

## 전체 예제 목록

| # | 파일명 | 카테고리 | 설명 | 사용 API | run68 지원 |
|---|--------|----------|------|----------|------------|
| 1 | `hello.c` | 기본 | 기본 Hello World (printf) | 표준 C | ✅ |
| 2 | `hello_iocs.c` | 시스템 | IOCS 콜 (BIOS 레벨 하드웨어 제어) | `<x68k/iocs.h>` | ⚠️ 부분 |
| 3 | `hello_dos.c` | 시스템 | DOS 콜 (Human68k OS 시스템 콜) | `<x68k/dos.h>` | ✅ |
| 4 | `hello_gfx.c` | 그래픽 | 그래픽 출력 (점, 선, 원, 사각형) | IOCS 그래픽 | ❌ GUI 필요 |
| 5 | `hello_snd.c` | 사운드 | OPM (YM2151) FM 음원 사용 | IOCS OPM | ❌ GUI 필요 |
| 6 | `hello_key.c` | 입력 | 키보드 입력 처리 (스캔코드/ASCII) | IOCS/DOS | ⚠️ 제한적 |
| 7 | `hello_mouse.c` | 입력 | 마우스 입력 (좌표, 버튼, 드래그) | IOCS ms_* | ❌ GUI 필요 |
| 8 | `hello_joy.c` | 입력 | 조이스틱 입력 (2포트, 8방향) | IOCS joyget | ❌ GUI 필요 |
| 9 | `hello_sprite.c` | 그래픽 | 하드웨어 스프라이트 (128개, 16색) | IOCS sp_* | ❌ GUI 필요 |
| 10 | `hello_bg.c` | 그래픽 | BG 타일맵 (2레이어, 스크롤) | IOCS bg* | ❌ GUI 필요 |
| 11 | `hello_timer.c` | 시스템 | 타이머 인터럽트 (Timer-D, VDISP) | IOCS timer* | ⚠️ 부분 |
| 12 | `hello_file.c` | 파일 | 파일 입출력 (생성/읽기/쓰기/삭제) | DOS 파일 함수 | ✅ |
| 13 | `hello_mem.c` | 메모리 | 메모리 관리 (malloc/mfree/setblock) | DOS 메모리 함수 | ✅ |
| 14 | `hello_adpcm.c` | 사운드 | ADPCM 음성 재생 (MSM6258) | IOCS adpcm* | ❌ GUI 필요 |
| 15 | `hello_game.c` | 게임 | "Star Catcher" 미니게임 | 스프라이트+입력+OPM | ❌ GUI 필요 |
| 16 | `hello_midi.c` | 사운드 | RS-232C MIDI 출력 (GM 호환) | IOCS 232c* | ❌ MIDI 장치 필요 |
| 17 | `hello_raster.c` | 그래픽 | 래스터 인터럽트 (물결, 분할 스크롤) | CRTC/MFP 레지스터 | ❌ GUI 필요 |
| 18 | `hello_dma.c` | 시스템 | DMA 전송 (HD63450 컨트롤러) | DMAC 레지스터 | ❌ GUI 필요 |
| 19 | `hello_rtc.c` | 시스템 | RTC 시계 (RP5C15, 날짜/시간) | RTC 레지스터/IOCS | ⚠️ 부분 |
| 20 | `hello_serial.c` | 통신 | RS-232C 시리얼 통신 | IOCS 232c* | ❌ 장치 필요 |
| 21 | `hello_fdd.c` | 저장장치 | FDD 저수준 접근 (섹터 읽기) | IOCS B_READ | ⚠️ 디스크 필요 |
| 22 | `hello_scsi.c` | 저장장치 | SCSI 하드디스크 접근 | IOCS S_* | ❌ HDD 필요 |
| 23 | `hello_printer.c` | 출력 | Centronics 프린터 출력 | IOCS prn* | ❌ 프린터 필요 |
| 24 | `hello_crtc.c` | 그래픽 | CRTC 화면 모드 전환 | CRTC 레지스터 | ❌ GUI 필요 |

---

## 범례

| 기호 | 의미 |
|------|------|
| ✅ | run68에서 완전 지원 |
| ⚠️ | run68에서 부분 지원/제한적 동작 |
| ❌ | GUI 에뮬레이터(XM6, XEiJ, px68k) 또는 실제 하드웨어 필요 |

---

## 카테고리별 분류

| 카테고리 | 예제 수 | 예제 목록 |
|----------|---------|-----------|
| **기본/시스템** | 6 | hello, hello_iocs, hello_dos, hello_timer, hello_mem, hello_rtc |
| **그래픽** | 5 | hello_gfx, hello_sprite, hello_bg, hello_raster, hello_crtc |
| **입력** | 3 | hello_key, hello_mouse, hello_joy |
| **사운드** | 3 | hello_snd, hello_adpcm, hello_midi |
| **저장장치/파일** | 3 | hello_file, hello_fdd, hello_scsi |
| **통신/출력** | 2 | hello_serial, hello_printer |
| **하드웨어** | 1 | hello_dma |
| **게임** | 1 | hello_game |

---

## 상세 설명

### 기본/시스템 예제

| 예제 | 주요 기능 | 학습 포인트 |
|------|-----------|-------------|
| `hello.c` | printf 출력 | 기본 컴파일 환경 확인 |
| `hello_iocs.c` | B_PRINT, B_PUTC, ONTIME | IOCS BIOS 콜 사용법 |
| `hello_dos.c` | C_PRINT, GETDATE, CURDRV | Human68k DOS 콜 사용법 |
| `hello_timer.c` | Timer-D, VDISP, 스톱워치 | 인터럽트 핸들러 작성 |
| `hello_mem.c` | malloc, mfree, setblock | 메모리 할당/해제 |
| `hello_rtc.c` | 날짜/시간 읽기, 달력 | RTC 레지스터 접근 |

### 그래픽 예제

| 예제 | 주요 기능 | 학습 포인트 |
|------|-----------|-------------|
| `hello_gfx.c` | 점, 선, 원, 사각형 그리기 | IOCS 그래픽 함수, 화면 모드 |
| `hello_sprite.c` | 스프라이트 정의/표시/이동 | 패턴 정의, 팔레트, 우선순위 |
| `hello_bg.c` | BG 타일맵, 스크롤 | 2레이어 BG, 패럴랙스 |
| `hello_raster.c` | 물결 효과, 화면 분할 | 래스터 인터럽트, CRTC |
| `hello_crtc.c` | 해상도/색상 모드 전환 | CRTC 레지스터, GVRAM |

### 입력 예제

| 예제 | 주요 기능 | 학습 포인트 |
|------|-----------|-------------|
| `hello_key.c` | 키 입력, 문자열 입력 | 스캔코드, ASCII, 블로킹/논블로킹 |
| `hello_mouse.c` | 좌표, 버튼, 드래그 | 마우스 커서, 이벤트 감지 |
| `hello_joy.c` | 8방향, 버튼, 콤보 | 조이스틱 상태, 2인용 |

### 사운드 예제

| 예제 | 주요 기능 | 학습 포인트 |
|------|-----------|-------------|
| `hello_snd.c` | FM 음원 연주 | OPM 레지스터, 키 코드 |
| `hello_adpcm.c` | PCM 재생, 파형 생성 | 샘플레이트, ADPCM 인코딩 |
| `hello_midi.c` | MIDI 출력, GM 음원 | RS-232C 설정, MIDI 프로토콜 |

### 저장장치/파일 예제

| 예제 | 주요 기능 | 학습 포인트 |
|------|-----------|-------------|
| `hello_file.c` | 파일 생성/읽기/쓰기/삭제 | DOS 파일 핸들, 디렉토리 |
| `hello_fdd.c` | 섹터 읽기, BPB 분석 | FDC, CHS 주소 지정 |
| `hello_scsi.c` | SCSI 장치 스캔, 용량 조회 | SPC 레지스터, SCSI 명령 |

### 통신/출력 예제

| 예제 | 주요 기능 | 학습 포인트 |
|------|-----------|-------------|
| `hello_serial.c` | 시리얼 송수신, 터미널 | 보드레이트, 데이터 포맷 |
| `hello_printer.c` | 프린터 출력, ESC/P | Centronics, 제어 코드 |

### 하드웨어 예제

| 예제 | 주요 기능 | 학습 포인트 |
|------|-----------|-------------|
| `hello_dma.c` | 고속 메모리 전송 | HD63450, 채널 설정 |

### 게임 예제

| 예제 | 주요 기능 | 학습 포인트 |
|------|-----------|-------------|
| `hello_game.c` | Star Catcher 미니게임 | 게임 루프, 충돌 감지, 상태 관리 |

---

## 빌드 및 실행

### 전체 빌드
```bash
make
```

### 개별 빌드
```bash
make build/hello_xxx.x
```

### run68으로 실행
```bash
make run-hello_xxx
```

### GUI 에뮬레이터 테스트
- **XM6 TypeG** (Windows): 그래픽, 사운드, 입력 완전 지원
- **XEiJ** (Java): 크로스플랫폼, 대부분 기능 지원
- **px68k** (Linux/RetroArch): 게임 중심 에뮬레이션

---

## X68000 하드웨어 요약

| 구성요소 | 칩셋 | 주요 기능 |
|----------|------|-----------|
| CPU | MC68000 (10MHz) | 16/32비트 프로세서 |
| FM 음원 | YM2151 (OPM) | 8채널, 4오퍼레이터 FM |
| ADPCM | MSM6258 | 음성/효과음 재생 |
| 그래픽 | CRTC + 커스텀 | 최대 65536색, 스프라이트 128개 |
| DMA | HD63450 | 4채널 DMA 컨트롤러 |
| FDC | uPD72065 | 2HD/2DD 플로피 |
| SCSI | MB89352 (SPC) | 하드디스크, CD-ROM |
| RTC | RP5C15 | 배터리 백업 시계 |

---

## 참고 자료

- [elf2x68k](https://github.com/yunkya2/elf2x68k) - GCC 크로스 컴파일러
- [X68000 LIBRARY](http://retropc.net/x68000/) - X68000 기술 자료
- [XM6 TypeG](http://www.intj.net/xm6/xm6g/) - Windows 에뮬레이터
- [XEiJ](https://stdkmd.net/xeij/) - Java 에뮬레이터
