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

### 3.4 ubox-msx-lib-z88dk 예제 12종 (10/12 포팅 완료)

`Examples/ubox_example_z88dk/examples/<NN>_<name>/` 안에서 각각 ROM(cartridge)과
DOS(.COM) 양쪽 빌드+실행. 라이브러리는 `Library/MSX/ubox-msx-lib-z88dk/` (z88dk 포트, MSX-DOS2 BIOS fix 포함)와 `Library/MSX/spman-z88dk/` (10_breakout용) 사용.

```bash
cd Examples/ubox_example_z88dk/examples/01_hello
./compile.sh build       # → build/01_hello.rom (16 KiB plain cartridge)
./compile_dos.sh build   # → build_dos/01_hello (.COM)
./run_openmsx.sh         # cartridge 부팅
./run_openmsx_dos.sh     # MSX-DOS2 부팅 후 HELLO 입력
```

상세 매트릭스 (DOS_NAME, 빌드 사이즈, 적용된 우회) + 보류 사항(05/06: Arkos 2 AKM player 포팅 필요)은 `Examples/ubox_example_z88dk/README_Z88DK_PORT.md` 참조. 작업 계획 / Canonical ZCCFLAGS / mplayer-z88dk (Arkos 2 AKM) 빌드 파이프라인 / Phase 7 라이브러리 개선 사항은 `Examples/ubox_example_z88dk/PLAN_UBOX_EXAMPLES.md` 참조.

### 3.5 ROM 카트 매퍼 + 고급 패턴 (z88dk megaROM)

`Tutorial_msx_z88dk_rom_01` 의 단순 16K plain 카트 다음 단계 — 32K+ 카트 / 음악 통합 / multi-bank 작업 시 알아야 할 패턴 정리. 실제 적용 사례는 `Examples/kingsvalley_z88dk` (Phase A/B/C) 와 `Examples/prototype_05_MSX_ROM_MSXDOS` 참조.

#### 3.5.1 매퍼 선택 가이드

| 매퍼 | 크기 | trigger 주소 | openMSX `-romtype` | 사용 사례 |
|------|------|------------|--------------------|-----------|
| **plain** | 16K / 32K | (없음) | (auto-detect) | ubox 12 예제, kingsvalley Phase A |
| **ASCII16** | 32K~512K | `$7000` write | `Ascii16` | 64K cart + 음악 BANK 분리 (kingsvalley Phase B) |
| **Konami** (without SCC) | 32K~256K, 8K bank | `$6000` / `$8000` / `$A000` write | `Konami` | main 16K fit 제약 (kingsvalley Phase C, prototype_05) |

**중요 함정** — openMSX `RomKonami.cc` 의 `writeMem` 가 `[$6000, $C000)` 범위 *모든* memory write 를 bank-switch trigger 로 해석. main code 가 그 영역에 spread 되면 sccz80 emit 의 write 가 의도치 않은 bank-switch 일으켜 **GT hang**. 회피: main code 를 `$4000-$7FFF` (16K) 안에 fit + `#pragma bank N` 으로 module/data 를 BANK_N 분리.

빌드 옵션:
```bash
ZCCFLAGS="+msx -subtype=rom -compiler=sccz80 -SO2 -create-app"
ZCCFLAGS="$ZCCFLAGS -pragma-define:MAPPER_KONAMI=1"     # 또는 MAPPER_ASCII16=1
```

#### 3.5.2 megaROM 의 `#pragma bank N` 분리 패턴

main 16K fit 시 sccz80 의 `#pragma bank N` 으로 module/data 를 BANK_N 으로 routing:

```c
#ifdef __Z88DK
#pragma bank 1   /* 이 .c 파일 전체가 BANK_01 (CODE_1 + RODATA_1) */
#endif
#include "ubox.h"
/* 함수/const 정의 ... */
```

주의:
- sccz80 의 `#pragma bank` 는 **file 단위**. code+rodata 모두 함께 BANK_N 으로. 일부만 분리하려면 별도 .c 파일로 떼내야 함 (예: kingsvalley Phase C 의 `frames_bank.c`).
- `aplib` (`-lap`) 같은 라이브러리는 default `SECTION code_user` (= main bank) 라 main 끝부분 침범 가능. sed-rewrite 후 직접 link 로 BANK_N 라우팅: `sed 's/^\s*SECTION code_user/SECTION CODE_1/' ap.asm > ap_banked.asm`.
- Konami 매퍼는 boot default 로 bank 2,3 (`$8000-$BFFF` = BANK_01) mount 라 BANK_01 의 rodata 는 main code 가 cross-bank call 없이 직접 read 가능. ASCII16 은 `RomAscii16kB::reset` 이 bank 0 을 page1+page2 mirror 하므로 init 전 bank 1 mount 가 필요 — cart 의 dead-code 영역 (`$4044` 등) 에 8-byte shim 주입 필요 (kingsvalley `akm_bridge_kv.asm` 참조).

#### 3.5.3 CALSLT 우회 fix (ROM 모드 한정)

`ubox-msx-lib-z88dk` 의 BIOS wrapper 는 z88dk `msx_crt0` 의 `msxbios` 함수 (`ld iy, ($FCC0); call $001C (CALSLT); ei; ret`) 를 경유 — 호출당 ~220 cy overhead. ROM 모드에선 page 0 = BIOS slot 이라 직접 jp BIOS_addr 가능 (CALSLT 불필요).

해결 (kingsvalley a70b070 commit 패턴):
```bash
# compile.sh 안에서 ubox source 를 build dir 로 sed-rewrite
for f in "$UBOX_DIR/src/ubox"/*.asm; do
    sed -e 's/\bjp msxbios\b/jp msxbios_fast/g' \
        -e 's/\bcall msxbios\b/call msxbios_fast/g' \
        -e 's/\bEXTERN msxbios\b/EXTERN msxbios_fast/g' \
        "$f" > "$BUILD_DIR/ubox_patched/$(basename "$f")"
done
```

`msxbios_fast` 의 2-byte stub:
```asm
    SECTION code_user
    PUBLIC msxbios_fast
msxbios_fast:
    jp (ix)           ; IX = BIOS routine address, ~8 cy (vs CALSLT ~220 cy)
```

`-lubox` 대신 patched source 를 직접 link. **MSX-DOS2 (page 0 = user TPA) 모드는 적용 불가** — CALSLT 경유 필수.

효과: 호출당 ~193 cy 절감. `ubox_put_tile` burst (예: 256-tile redraw) 시 ~50,000 cy 절감 ≈ 1 VBlank. 적용 사례: kingsvalley `compile_phaseA.sh`, prototype_05 `compile.sh` 의 `prepare_ubox_patched`.

#### 3.5.4 AKM/AKG music player 통합 (ISR-driven 패턴)

Arkos Tracker 2 의 AKM/AKG player (MSXgl `engine/src/arkos/`) 는 header 에 **"must be called once each frame"** 명시. 매 VBlank 정확히 1 회 호출 가정 — 내부 frame counter (envelope, tick decreasing, pattern duration) 가 frame-uniform interval 에 의존.

**Anti-pattern (사용 금지)**: main-loop catch-up + shadow buffer 조합.
```c
/* anti-pattern — burst 시 player state 가 N frame 진행하나 ISR 는 최종
   shadow 만 flush → 중간 frame 의 envelope retrigger/note onset coalesce → 
   audible stretch + compress */
while (music_jiffy != JIFFY_LO) { music_play(); music_jiffy++; }
```

**권장 패턴**: VBlank user ISR 안에서 직접 호출 (kingsvalley Phase B / prototype_05 적용):

```c
/* ASM wrapper: 단순 jp, di-ei 불필요 (ubox_isr 가 이미 DI 안에서 호출) */
/* music_isr.asm:
   _music_isr:
       jp PLY_AKG_PLAY   (또는 PLY_AKM_PLAY)
*/

/* main.c */
render_init();
music_init();              /* PLY_AKG/AKM_INIT */
music_install_isr();       /* ubox_set_user_isr(_music_isr) */
while (running) {
    ubox_wait();
    /* 음악은 ISR 가 자동 처리. main loop 의 music_tick/pump 호출 금지. */
    ...
}
```

핵심:
- AKM/AKG player 를 **PSG 포트 직접 write 모드** (buffer-mode OFF) 로 생성 — prototype_05 의 `convert_akg_player.py` 의 `--buffer-mode` 플래그 제거.
- ISR 안 호출이라 wrapper 의 di-ei 불필요. 단순 `jp PLY_AKG/AKM_PLAY`.
- 50/60Hz 처리 필요 여부는 §3.5.5 의 frame rate mismatch 가이드 참조.

#### 3.5.5 50Hz vs 60Hz frame rate 처리 (음악 tempo)

MSX 시스템 frame rate (지역별 차이):

| 지역 / 시스템 | Frame rate | VBlank 주기 |
|--------------|-----------|-----------|
| 일본 / 한국 (NTSC) | **60Hz** | ~16.7 ms |
| 유럽 / 호주 (PAL) | **50Hz** | 20 ms |

AKM/AKG player 는 "must be called once each frame" design — **시스템 frame rate 에 곡 tempo 가 종속**. 곡 author 의 가정 frame rate 와 시스템 frame rate 가 다르면:

| 곡 가정 | 시스템 | 결과 |
|--------|-------|------|
| 60Hz | 60Hz | 정확한 tempo |
| 50Hz | 50Hz | 정확한 tempo |
| 50Hz | 60Hz | **1.2× 빠르게** (60/50 = 1.2) |
| 60Hz | 50Hz | **0.83× 느리게** (50/60 ≈ 0.83) |

**ISR-driven 패턴 의 default**: 매 VBlank 1 play → 시스템 frame rate 그대로. 60Hz 곡 + 60Hz 시스템 또는 50Hz 곡 + 50Hz 시스템이면 별도 처리 불필요.

**Mismatch 시 처리**:

(a) **50Hz 곡 + 60Hz 시스템** — 60Hz 시스템에서 6 VBlank 마다 1 skip 으로 effective 50Hz:

```asm
; src/music_isr.asm (50Hz 곡 + 60Hz 시스템 대응 변형)
    SECTION code_user
    PUBLIC _music_isr
    EXTERN PLY_AKG_PLAY

skip_acc: defs 1

_music_isr:
    ld a, (skip_acc)
    inc a
    cp 6
    jr z, _skip
    ld (skip_acc), a
    jp PLY_AKG_PLAY            ; 5/6 frame: play
_skip:
    xor a
    ld (skip_acc), a           ; 1/6 frame: skip (60 / 6 × 5 = 50 plays/sec)
    ret
```

(b) **60Hz 곡 + 50Hz 시스템** — 50Hz 시스템에서 일부 VBlank 에 2 play 시도하면 AKG/AKM 의 "once-per-frame" design 위반. 권장: **곡을 50Hz 로 재인코딩** (Arkos Tracker 2 에서 PSG frequency 설정 변경). 강제 2 play 는 envelope retrigger 등 audible artifact 위험.

(c) **Runtime detect** — `ubox_get_vsync_freq()` 가 시스템 frame rate 반환 (0=60Hz, 1=50Hz). 동적 skip 토글 가능:

```c
extern uint8_t is_60hz;   /* ubox_get_vsync_freq() == 0 일 때 set */

/* music_init() 안에서 */
is_60hz = (ubox_get_vsync_freq() == 0);
```

ASM ISR 가 `is_60hz` 와 `skip_acc` 를 함께 검사.

**prototype_05 적용 결과**: `letsroll_song` 이 60Hz 기준. `_music_isr` 가 단순 `jp PLY_AKG_PLAY` (skip 없음). 60Hz 시스템 (Panasonic FS-A1GT 등) 에서 정확한 tempo. 50Hz 시스템 (Philips NMS 등) 에서 약 0.83× 느리게 (체감 가능).

**kingsvalley Phase B 적용 결과**: AKM 곡 도 60Hz 기준. `my_isr` 동일 패턴 (skip 없음).

#### 3.5.6 AKM/AKG blob 의 BANK 배치

대용량 cart (64K+) 에서 player + song blob 을 별도 BANK 에 두는 패턴 (kingsvalley `akm_bridge_kv.asm`):

```asm
SECTION RODATA_2    ; BANK_02 안에 blob INCBIN
_akm_blob_start:
    BINARY "akm.bin"
_akm_blob_end:

SECTION code_user
_mplayer_engine_load:
    di
    ; RAM stub 을 $F100 에 LDIR (bank swap 시 자기 unmap 회피)
    ld hl, _engine_stub_start
    ld de, $F100
    ld bc, _engine_stub_end - _engine_stub_start
    ldir
    call $F100      ; RAM stub 실행: BANK_02 mount → LDIR → 원 bank 복원
    ei
    ret
```

`__BSS_END_tail` (.map) 과 AKM/AKG work area ORG (`$CA00` / `$D500` 등) 충돌 여부 확인 필수.

#### 3.5.7 트러블슈팅: binary 비교 절차

z88dk 빌드 ROM 이 reference (SDCC 원본 등) 와 다른 동작 시 5단계 비교:

```bash
# B-1. 사이즈 + 헤더 ('AB' + entry $4010)
xxd -l 16 build/X.rom

# B-2. Startup sequence (entry 후 64 byte 의 함수 sequence 동일성)
xxd -s 0x10 -l 64 build/X.rom

# B-3. 매퍼-trigger pattern scan (Konami 의 경우)
python3 -c "
data = open('build/X.rom', 'rb').read()
for i in range(len(data)-2):
    if data[i] in (0x32, 0x22):  # ld (nn), a / ld (nn), hl
        addr = data[i+1] | (data[i+2] << 8)
        if 0x6000 <= addr < 0xC000:
            print(f'offset 0x{i:04X}: trigger \${addr:04X}')
"

# B-4. 핵심 심볼 위치 (.map)
grep -E "^_main\b|^__BSS_END_tail\b|^PLY_(AKG|AKM)_PLAY\b" build/X.map

# B-5. Konami 매퍼 인프라 부재 확인 (plain cart 가정 시)
grep -E "banked_call|__current_bank|PUT_P2|__far_map_bank" build/X.map
```

상세 분석 사례 (Konami 매퍼 hang 진단 → ASCII16/Konami 변형까지): `Examples/kingsvalley_z88dk/PLAN_MIGRATION_KINGSVALLY.md` §7.

#### 3.5.8 RS-232C + telnet 통신 패턴 (prototype_20)

MSX ROM 에서 RS-232C cartridge (i8251) 로 GTM/openMSX RS232Net 를 거쳐 telnet 서버에 접속하는 경우 발생한 함정들. prototype_20_MSX_ROM_MSXDOS 검증 완료 (2026-05-30, **게이트웨이 → 던전 진입까지 전체 흐름 동작 확인**).

**1) openMSX RS232Net 의 IP232 default ON — 최대 함정.**
`rs232-net-ip232` setting 이 default true 라서 수신 byte 가 `0xFF` 면 modem-control magic 으로 처리되고 다음 byte 까지 응용까지 안 전달됨. telnet IAC negotiation 의 `\xff\xfb\x00` 같은 sequence 가 application 의 IAC parser 까지 도달 못 함 → IAC 응답 못 보냄 → bridge IAC negotiation timeout (2-3s) 으로 connection close. 화면 "CONNECTING..." 영원 멈춤.

해결: openMSX 시작 직후 또는 `plug` 직전에:
```tcl
set rs232-net-ip232 false
```
prototype_20 의 `run_openmsx.sh` 패턴:
```bash
ARGS+=(-command "set rs232-net-ip232 false; set rs232-net-address ...; catch {plug msx-rs232 rs232-net}")
```
진단: 응용에 `iac_seen` (iac_feed 가 `0xFF` 받은 횟수) 카운터를 두면 즉시 확인 — IP232 ON 이면 0, OFF 이면 6+.

**2) i8251 RX backpressure (openMSX RS232Net patch).**
RS232Net::signal 이 i8251 `isRxBusy()` check 안 해서 9600 baud RX burst 가 i8251 single-byte buffer 에 overrun (RS232Host 는 backpressure 있음, oversight). `Emulator/openMSX/src/serial/RS232Net.cc::signal()` 에 한 줄 추가:
```cpp
if (conn->isRxBusy()) return;   // RS232Host 와 동일 패턴
```
upstream patch 가치 있음. 본 repo 의 openMSX submodule 에 적용됨 (commit `f02010e81`).

**3) ubox_isr 의 VDP S#0 read 누락.**
ubox-msx-lib-z88dk 의 `ubox_isr` 가 HTIMI hook 에 install 되는데 VDP S#0 read 가 없어서, EI busy-wait (`serial_putc` 의 TXRDY 폴링 등) 와 결합 시 IRQ flag 가 RETI 후에도 asserted → 무한 재인터럽트 cascade → C 스택 12KB 폭주 → BIOS workarea 침범 → page-3 swap. `push af` 직후 `in a, ($99)` 추가로 fix:
```asm
ubox_isr:
    push af
    in a, ($99)        ; VDP S#0 read → IRQ flag clear (defensive)
    push ix
    ...
```
`Library/MSX/ubox-msx-lib-z88dk/src/ubox/ubox_isr.asm` 에 적용됨.

**4) serial_putc 송신 burst 동안 DI 책임.**
`serial_putc` 가 TXRDY busy-wait 으로 EI 상태에서 폴링하면 위 #3 cascade 트리거. fix:
- `serial_putc` 자체는 DI/EI 안 함 (caller 책임).
- burst 송신 함수 (`conn_send_frame` 등) 가 전체 구간 `__asm__("di")` 로 감싸기.

**5) recv_buf magic resync.**
GTM/bridge 가 telnet connect 직후 ASCII 잔재 (`"\r\ndisconnecting."`, IAC negotiation 잔재) 를 보낼 수 있음. needle 매칭 후 trailing byte 가 recv_buf 의 첫 byte 에 들어가면 frame magic (`47 50`) 매칭 영원히 실패. `recv_buf_try_extract` 가 첫 byte 가 `0x47` 아니면 1 byte 씩 drop 하며 scan 해야 함.

**6) BSS-stack 137 byte 함정.**
megarom 빌드의 stack 시작점은 BIOS HIMEM (`$F380` 부근), BSS top 과의 간격이 정상 호출 깊이 + ISR push 만으로도 침범 가능 → BIOS workarea (RAMAD3 등) 오염 → BIOS ENASLT 가 corrupt segment 로 `out ($FF), a` → page-3 RAM swap → main RAM "사라짐" (dbg read = 0xFF). 큰 BSS (예: `g_frame.body[4096]`) 줄이거나 extram 이동 필요.

**7) DKFS server 의 IAC BINARY negotiation timeout.**
dkfsbridge 의 `iac_negotiation_timeout_ms` 가 짧으면 (default 2s/3s 등) client 가 IAC 응답을 그 안에 못 보낼 때 connection close. PRESS ENTER 같은 사용자 대기 단계는 main loop 진입 전에 두면 안 됨 — gtm_wait_connected 직후 즉시 main loop 진입해 conn_poll 가 매 frame 호출되도록 해야 한다.

**8) DKFS server 의 player session race.**
DKFS server 는 한 player ID 당 단일 connection 만 허용. **이전 session 종료 직후 같은 player ID 로 너무 빨리 (수백 ms~수 초) 재시도하면 server 측 session cleanup 이 완료되기 전이라 즉시 거부**. dkfsbridge log 에는 `connect timeout 8000ms — closing` + `took 0ms` + `shutdown` 이 같은 ms 에 표시되어 timeout 처럼 보이지만 실제는 server race-condition 거부. 처치: 수십 초~분 단위로 시간 두고 재시도, 또는 다른 player ID 사용.

**9) GTM `+++` escape sequence.**
GTM 가 stale telnet mode 일 때 (이전 session 의 server-side close 후 GTM 측 정리 안 됨) 새 `GTM CONN` 이 telnet 데이터로 처리되어 무시. `+++` escape spec 으로 command mode 복귀:
```c
void gtm_force_command_mode(void) {
    for (i=0; i<72; ++i) ubox_wait();  /* (a) 1.2s idle */
    serial_putc('+'); serial_putc('+'); serial_putc('+');
    for (i=0; i<72; ++i) ubox_wait();  /* (c) 1.2s idle */
    while (serial_data_ready()) serial_getc_raw();  /* drain */
}
```
command mode 였던 경우 `+++` 가 line buffer 에 쌓일 수 있어 다음 명령과 prefix 충돌. **host python 검증에서 `+++` 후 `\r\n` flush 가 필요함을 확인** (`+++GTM CONN ...` 로 합쳐져 `ERROR: command is invalid` 응답). MSX 측은 GTM 가 자체 line 처리로 분리해서 안전하다면 생략 가능.

**10) 폰트의 소문자 영역 누락.**
ROM 빌드의 font.h 는 RAM 절약 목적으로 소문자 영역 (`[97]~[122]`) 을 대문자 패턴 복사 placeholder 로 두는 경우 많다. ASCII 그대로 송신하면 메모리/wire 는 소문자 `"player"` 인데 화면만 대문자 `PLAYER` 로 보임. **font 슬롯 자체는 이미 차지하고 있으므로 패턴 데이터만 교체하면 ROM 크기 영향 0** — 26 글자 5x7 left-aligned 패턴 작성으로 진짜 소문자 표시 가능.

**11) CAPS LOCK 처리.**
`ubox_read_keys` 는 PPI 직접 read 라 BIOS 의 CAPS LOCK case 변환을 우회. 따로 처리 없으면 CAPS LOCK 켜져도 무시. 가장 단순한 fix 는 BIOS workarea `CAPSLT = $FCAB` 1 byte read (BIOS 가 키 toggle + LED 자동 관리):
```c
#define CAPSLT (*(volatile uint8_t *)0xFCAB)
...
if (row >= 3 || (row == 2 && b >= 6)) {     /* 알파벳에만 */
    if (CAPSLT) eff_shift ^= 1;             /* shift XOR caps */
}
```

**12) Render skip + cursor blink 패턴 (P05 패턴 차용).**
main loop 가 매 frame `txt_clear()`+전체 redraw 하면 깜박임 + load. P05 처럼:
- main loop 의 update/render 를 **입력/server-event 시에만** 호출 (`if (input == 0) continue` 류)
- 각 screen 의 render 에 `s_dirty` flag (입력/state 변경 시 1, render 후 0)
- editing 모드에서만 cursor blink — main loop 가 매 N frame `g_blink_on` toggle + `need_render=1`, screen render 의 dirty=0 path 에서 cursor cell 하나만 update (전체 redraw 안 함 → 깜박임 0)

### 3.6 SDCC ↔ z88dk(sccz80) 변환 시 주의할 차이점

같은 MSX C source 를 SDCC 와 z88dk(sccz80) 양쪽으로 빌드하거나, SDCC 작성된
프로젝트 (예: ubox SDCC 원본, MSXgl 의 AKM/AKG player 등) 를 z88dk 로 포팅 시
실제 발생한 차이점 정리. kingsvalley, prototype_05, ubox 12 예제 포팅 작업
(2026-04 ~ 05) 에서 확인됨.

#### 3.6.1 빌드 도구

| 항목 | SDCC | z88dk |
|------|------|-------|
| 컴파일러 | `sdcc -mz80` | `zcc +msx -compiler=sccz80` |
| 어셈블러 | `sdasz80` | `z80asm` (z88dk 의) |
| Linker | `sdcc` (자체) | `zcc` (자체) |
| 라이브러리 | `.lib` (sdar 로 생성) | `.lib` (z80asm `-x` 로 생성) |
| 라이브러리 호환 | ❌ **서로 호환 안 됨** — z88dk 빌드는 source 재컴파일 필요 | |

⚠️ 같은 이름의 라이브러리 directory (예: `Examples/ubox_example_z88dk/src/ubox/`)
가 있어도 그 안의 `.asm` 이 `sdasz80` 문법이면 SDCC 빌드. z88dk 빌드용은
`Library/MSX/ubox-msx-lib-z88dk/` (별도 z80asm port).

#### 3.6.2 ABI (Calling convention) — 가장 큰 함정

| 항목 | SDCC z80 | sccz80 |
|------|---------|--------|
| 8-bit 인자 packing | **1 byte** | **2 byte (zero-extended to word)** |
| 16-bit 인자 | 2 byte | 2 byte (동일) |
| Push 순서 | **right-to-left** (last 인자 deepest) | **left-to-right** (first 인자 deepest) |
| Return uint8 | **A** 레지스터 | **L** 레지스터 |
| Return uint16 | HL | HL (동일) |
| __z88dk_fastcall (HL=single arg) | 해석 시 동일 | 동일 |

**증상**: SDCC asm wrapper 를 z88dk 로 그대로 가져오면 stack offset 어긋남 → 인자가 garbage state.

예 (`mplayer_init(uint8_t* song, uint8_t sub_song)` wrapper):

```asm
; SDCC convention: sp+2 = song(2B), sp+4 = sub_song(1B)
; sccz80 convention: sp+2 = sub_song(2B, last pushed), sp+4 = song(2B)

; sccz80 wrapper:
_mplayer_init:
    ld ix, 2
    add ix, sp
    ld a, (ix+0)        ; sub_song (last pushed, low byte at sp+2)
    ld l, (ix+2)        ; song lo (first pushed, deeper)
    ld h, (ix+3)        ; song hi
    ; ...
```

자세한 사례 + 회피 패턴: `Examples/ubox_example_z88dk/PLAN_UBOX_EXAMPLES.md` §6
"mplayer-z88dk".

#### 3.6.3 언어 / 컴파일러 차이

| 항목 | SDCC | sccz80 |
|------|------|--------|
| Mid-block declarations | 허용 (C99-ish) | **거부 (C89 strict)** — declaration 을 block 시작으로 |
| Struct compound init (nested) | 허용 | **거부** — field-by-field 할당으로 |
| Nested function pointer typedef | 허용 | **거부** (예: 04_clibrary 의 `src/printf.c` 거부) |
| `__sfr` / `__at()` keyword | 지원 | **미지원** → inline asm + 직접 port I/O |

예 (struct compound init):
```c
/* SDCC OK */
struct rect r = { .x = 1, .y = 2 };

/* sccz80 호환 */
struct rect r;
r.x = 1; r.y = 2;
```

#### 3.6.4 헤더 / Preprocessor

| 항목 | SDCC | sccz80 |
|------|------|--------|
| `__SDCC` 매크로 | 정의됨 | 정의 안 됨 |
| `#include "x.h"` lookup | source-dir-first | **`-I` path 순서 따름** |
| `__asm ... __endasm` | 줄 단위 보존 | preprocessor 가 **한 줄로 collapse** (매크로 안에 들면 fail) |

⚠️ **`__asm` 매크로 함정** — SDCC 의 `do { __asm halt __endasm; } while(0)` 패턴은
sccz80 preprocessor 가 한 줄로 합쳐서 `Unknown symbol: __asm` 에러. 회피:
별도 ASM source 로 함수화 (`_ubox_wait_vsync: halt; ret`) 후 link.

⚠️ **Header 충돌 사례** (kingsvalley): `game/generated/enemy.h` 와
`game/src/enemy.h` 가 동명. SDCC 는 `"enemy.h"` 가 source dir 우선 → 의도대로
src 쓰임. sccz80 는 `-I generated -I src` 의 순서면 generated/enemy.h 가 먼저.
해결: header rename (`enemy_data.h`, `knife_data.h`).

⚠️ **`__SDCC` 가드 활성화 문제** — game source 가 `#ifdef __SDCC` 가드 안에
SDCC-only 코드 (예: ubox API 의 specific signature). z88dk 빌드 시 그 가드
inactive → 누락된 정의 발생. 해결: `__SDCC` → `MSX_BUILD || __SDCC` 같이
*양쪽 활성* 으로 확장 (kingsvalley 사례).

#### 3.6.5 Section / Bank pragma

| 항목 | SDCC | z80asm (z88dk) |
|------|------|---------------|
| Code section | `_CODE` | `code_user` / `code_compiler` |
| Data section | `_DATA` | `data_user` / `data_compiler` |
| BSS section | `_BSS` | `bss_user` / `bss_compiler` |
| Rodata section | `_CODE` (자동 ROM) | `rodata_user` / `rodata_compiler` |
| Bank section | `_BANK1`, `_BANK2`, ... | `CODE_1`, `CODE_2`, ... / `RODATA_1`, ... |
| `#pragma bank N` | SDCC: bank 단위 ROM placement | sccz80: **file 단위로 `CODE_N` + `RODATA_1` 양쪽 routing** |

z80asm 의 cross-section call 은 자유 (Konami 매퍼는 boot 시 BANK_01 mount 라
`call $8xxx` 직접 가능). §3.5.2 의 `#pragma bank N` 패턴 참조.

SDCC asm 을 z80asm 으로 변환 시 sed 패턴 (`convert_akg_player.py` 와 유사):
```
.area _CODE    →  SECTION code_user
.globl X       →  PUBLIC X
_label::       →  _label:
#NN            →  NN   (즉시값)
$NN            →  $NN  (16진수, 일부 어셈블러)
#0xNN          →  $NN
```

#### 3.6.6 inline asm 의 차이

둘 다 `__asm ... __endasm` 사용하지만 macro/preprocessor 동작이 다름. 또 inline
asm 안의 label scope, 인자 접근 방식도 차이 — 복잡한 inline asm 은 *별도 .asm
파일로 분리해 link* 가 가장 안전.

#### 3.6.7 Codegen size 차이 (정량)

kingsvalley 의 동일 source 를 SDCC 와 z88dk 양쪽으로 빌드 비교 결과 (
`Examples/kingsvalley_z88dk/PLAN_MIGRATION_KINGSVALLY.md` §4 C-3):

| 영역 | SDCC | z88dk(sccz80) | 차이 |
|------|------|--------------|------|
| 평균 game C 코드 size | — | — | **+5-7%** |
| character.c (hot path) | 3,689 B | 4,584 B | **+24%** |
| character_move.c (hot path) | 1,150 B | 1,501 B | **+31%** |
| game.c (mixed) | 4,079 B | 4,426 B | +9% |
| game_util.c | 1,679 B | 1,517 B | -10% |

**원인**:
- sccz80 의 8-bit arg = word zero-extend overhead
- caller-cleanup ABI 의 더 큰 prologue/epilogue
- character.c 처럼 복잡한 state machine + 많은 함수 호출이 누적 영향

**대응**: hot path 의 직접 asm 작성 외 회피 어려움. sccz80 의 codegen 한계로
수용. CALSLT 우회 fix (§3.5.3) 같은 burst 절감이 효과 큼.

#### 3.6.8 빌드 옵션 차이

| 항목 | SDCC | z88dk |
|------|------|--------|
| 최적화 | `-O2`, `--max-allocs-per-node` | **`-SO2`** (NOT `-O2`/`-O3`) |
| DOS 빌드 매크로 | (custom) | **`-DMSXDOS`** (NOT `-DBUILD_DOS`) |
| ROM 빌드 type | `--code-loc 0x4010` 등 | `-subtype=rom -create-app` |
| DOS .COM 빌드 type | `--code-loc 0x100` | `-subtype=msxdos2 -create-app` |
| 매퍼 | `--code-size` 등 수동 | `-pragma-define:MAPPER_KONAMI=1` 등 |
| Heap | linker script | `-pragma-define:CLIB_MALLOC_HEAP_SIZE=4096` |

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
- `Examples/kingsvalley_z88dk/PLAN_MIGRATION_KINGSVALLY.md` (z88dk 마이그레이션 + Konami/ASCII16 매퍼 진단 history, §3.5 의 원본 상세)
- `Examples/ubox_example_z88dk/PLAN_UBOX_EXAMPLES.md` (ubox 12 예제 z88dk 포팅 작업 계획 + Arkos 2 AKM 빌드 파이프라인)

## 7. 외부 참고 프로젝트 (resource/MSX/)

MSX 개발 시 코드/패턴을 참고할 수 있는 외부 오픈소스 프로젝트들이 `resource/MSX/` 아래에 독립 클론으로 보관되어 있습니다(submodule 아님, 워크스페이스 측에선 untracked로 둡니다 — 개인 reference 용).

| 디렉터리 | 출처 | 사용 시점 |
|---------|------|----------|
| `resource/MSX/kingsvalley` | [pdpdds/kingsvalley](https://github.com/pdpdds/kingsvalley) | 왕가의 계곡 1 C 재구현. **게임 로직 / 스테이지 데이터 구조** 참고. SDCC 빌드 사례. |
| `resource/MSX/ubox_example` | [pdpdds/ubox_example](https://github.com/pdpdds/ubox_example) | Juan J. Martinez의 `ubox` 라이브러리 사용 예. 본 워크스페이스의 `Library/MSX/ubox-msx-lib-z88dk` (z88dk 포팅판)와 짝. **VRAM/스프라이트/사운드 호출 패턴** 참고. |
| `resource/MSX/noborunoca` | [h1romas4/noborunoca](https://github.com/h1romas4/noborunoca) | z88dk-msx-template 기반의 완성된 게임. **GitHub Actions 빌드 파이프라인 + 32 KiB ROM 패키징** 사례. |
| `resource/MSX/z88dk-msx-template` | [h1romas4/z88dk-msx-template](https://github.com/h1romas4/z88dk-msx-template) | z88dk(`zcc +msx`) 기반 빌드 템플릿. **CMake 통합 + GDB(z88dk-gdb) 디버깅 흐름** 참고. |

이들 클론은 직접 빌드/링크하지 않습니다. 패턴 학습용 reading material로만 사용하세요. 우리 빌드 흐름은 `Toolchain/MSX/HITECH_TOOLCHAIN/`(Hi-Tech C) + system z88dk(`/opt/z88dk/bin/zcc`) + `Library/MSX/ubox-msx-lib-z88dk/` 조합을 사용합니다.
