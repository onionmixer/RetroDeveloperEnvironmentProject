/*
 * X68000 Human68k - MIDI Example
 * MIDI 출력 예제
 *
 * X68000 MIDI 사양:
 * - 내장 MIDI 인터페이스 (RS-MIDI)
 * - 31.25 kbaud MIDI 표준
 * - 16채널 지원
 * - GM (General MIDI) 호환 음원 연결 가능
 */
#include <x68k/iocs.h>
#include <x68k/dos.h>

/* MIDI 상태 메시지 */
#define MIDI_NOTE_OFF       0x80    /* 노트 오프 */
#define MIDI_NOTE_ON        0x90    /* 노트 온 */
#define MIDI_POLY_PRESSURE  0xA0    /* 폴리포닉 애프터터치 */
#define MIDI_CONTROL_CHANGE 0xB0    /* 컨트롤 체인지 */
#define MIDI_PROGRAM_CHANGE 0xC0    /* 프로그램 체인지 */
#define MIDI_CH_PRESSURE    0xD0    /* 채널 애프터터치 */
#define MIDI_PITCH_BEND     0xE0    /* 피치 벤드 */
#define MIDI_SYSTEM         0xF0    /* 시스템 메시지 */

/* MIDI 컨트롤러 번호 */
#define CC_BANK_MSB         0       /* 뱅크 셀렉트 MSB */
#define CC_MODULATION       1       /* 모듈레이션 */
#define CC_BREATH           2       /* 브레스 컨트롤러 */
#define CC_FOOT             4       /* 풋 컨트롤러 */
#define CC_PORTAMENTO_TIME  5       /* 포르타멘토 타임 */
#define CC_DATA_MSB         6       /* 데이터 엔트리 MSB */
#define CC_VOLUME           7       /* 채널 볼륨 */
#define CC_BALANCE          8       /* 밸런스 */
#define CC_PAN              10      /* 팬 */
#define CC_EXPRESSION       11      /* 익스프레션 */
#define CC_BANK_LSB         32      /* 뱅크 셀렉트 LSB */
#define CC_SUSTAIN          64      /* 서스테인 페달 */
#define CC_PORTAMENTO       65      /* 포르타멘토 */
#define CC_SOSTENUTO        66      /* 소스테누토 */
#define CC_SOFT             67      /* 소프트 페달 */
#define CC_RESONANCE        71      /* 레조넌스 (GS) */
#define CC_RELEASE          72      /* 릴리즈 타임 */
#define CC_ATTACK           73      /* 어택 타임 */
#define CC_CUTOFF           74      /* 컷오프 (GS) */
#define CC_REVERB           91      /* 리버브 */
#define CC_CHORUS           93      /* 코러스 */
#define CC_ALL_SOUND_OFF    120     /* 모든 사운드 오프 */
#define CC_RESET_ALL        121     /* 리셋 올 컨트롤러 */
#define CC_ALL_NOTES_OFF    123     /* 모든 노트 오프 */

/* GM 프로그램 번호 (일부) */
#define GM_PIANO            0       /* 어쿠스틱 그랜드 피아노 */
#define GM_BRIGHT_PIANO     1       /* 브라이트 피아노 */
#define GM_E_PIANO          4       /* 일렉트릭 피아노 */
#define GM_HARPSICHORD      6       /* 하프시코드 */
#define GM_CELESTA          8       /* 첼레스타 */
#define GM_VIBRAPHONE       11      /* 비브라폰 */
#define GM_MARIMBA          12      /* 마림바 */
#define GM_ORGAN            16      /* 드로바 오르간 */
#define GM_GUITAR           24      /* 어쿠스틱 기타 */
#define GM_E_GUITAR         27      /* 일렉트릭 기타 (클린) */
#define GM_BASS             32      /* 어쿠스틱 베이스 */
#define GM_E_BASS           33      /* 일렉트릭 베이스 */
#define GM_SLAP_BASS        36      /* 슬랩 베이스 */
#define GM_VIOLIN           40      /* 바이올린 */
#define GM_STRINGS          48      /* 스트링 앙상블 */
#define GM_CHOIR            52      /* 콰이어 */
#define GM_TRUMPET          56      /* 트럼펫 */
#define GM_TROMBONE         57      /* 트롬본 */
#define GM_SAX              64      /* 소프라노 색소폰 */
#define GM_FLUTE            73      /* 플루트 */
#define GM_SQUARE           80      /* 스퀘어 리드 */
#define GM_SAWTOOTH         81      /* 톱니파 리드 */
#define GM_PAD              88      /* 판타지아 패드 */
#define GM_FX               96      /* FX 사운드 */

/* 음계 정의 (옥타브 4 기준) */
#define NOTE_C4     60
#define NOTE_CS4    61
#define NOTE_D4     62
#define NOTE_DS4    63
#define NOTE_E4     64
#define NOTE_F4     65
#define NOTE_FS4    66
#define NOTE_G4     67
#define NOTE_GS4    68
#define NOTE_A4     69      /* A4 = 440Hz */
#define NOTE_AS4    70
#define NOTE_B4     71
#define NOTE_C5     72

/* 화면 출력 함수들 */
static void print(const char *str)
{
    _iocs_b_print(str);
}

static void print_char(int c)
{
    _iocs_b_putc(c);
}

static void newline(void)
{
    print_char('\r');
    print_char('\n');
}

static void print_num(int num)
{
    char buf[12];
    int i = 0;
    int neg = 0;

    if (num == 0) {
        print_char('0');
        return;
    }
    if (num < 0) {
        neg = 1;
        num = -num;
    }
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    if (neg) {
        print_char('-');
    }
    while (i > 0) {
        print_char(buf[--i]);
    }
}

static void print_hex(int value, int digits)
{
    char hex[] = "0123456789ABCDEF";
    int i;
    for (i = (digits - 1) * 4; i >= 0; i -= 4) {
        print_char(hex[(value >> i) & 0xF]);
    }
}

/* RS-232C/MIDI 설정값 */
/*
 * _iocs_set232c 파라미터:
 * bit 0-1: Stop bits (0=1, 1=1.5, 2=2)
 * bit 2-3: Parity (0=none, 1=odd, 2=even)
 * bit 4-5: Data bits (0=5, 1=6, 2=7, 3=8)
 * bit 8-11: Baud rate
 *   9 = 31250 (MIDI)
 *
 * MIDI: 8bit, 1stop, no parity, 31250baud = 0x0930
 */
#define MIDI_232C_MODE  0x0930

/* MIDI 초기화 */
static void midi_init(void)
{
    _iocs_set232c(MIDI_232C_MODE);
}

/* 딜레이 함수 */
static void delay(int ms)
{
    /* 약 1ms 딜레이 (대략적) */
    volatile int i, j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 1000; j++) {
            /* busy wait */
        }
    }
}

static void delay_long(int centisec)
{
    /* 1/100초 단위 딜레이 */
    struct iocs_time start = _iocs_ontime();
    while (1) {
        struct iocs_time now = _iocs_ontime();
        if (now.sec - start.sec >= centisec) break;
    }
}

/* MIDI 출력 함수들 */
static void midi_out(int data)
{
    /* 송신 버퍼가 비워질 때까지 대기 */
    while (_iocs_osns232c() == 0) {
        /* busy wait */
    }
    _iocs_out232c(data);
}

static void midi_send(int status, int data1, int data2)
{
    midi_out(status);
    midi_out(data1);
    midi_out(data2);
}

static void midi_send2(int status, int data1)
{
    midi_out(status);
    midi_out(data1);
}

/* 노트 온 */
static void note_on(int channel, int note, int velocity)
{
    midi_send(MIDI_NOTE_ON | (channel & 0x0F), note & 0x7F, velocity & 0x7F);
}

/* 노트 오프 */
static void note_off(int channel, int note, int velocity)
{
    midi_send(MIDI_NOTE_OFF | (channel & 0x0F), note & 0x7F, velocity & 0x7F);
}

/* 프로그램 체인지 (음색 변경) */
static void program_change(int channel, int program)
{
    midi_send2(MIDI_PROGRAM_CHANGE | (channel & 0x0F), program & 0x7F);
}

/* 컨트롤 체인지 */
static void control_change(int channel, int controller, int value)
{
    midi_send(MIDI_CONTROL_CHANGE | (channel & 0x0F), controller & 0x7F, value & 0x7F);
}

/* 피치 벤드 */
static void pitch_bend(int channel, int value)
{
    /* value: 0-16383, 8192 = center */
    int lsb = value & 0x7F;
    int msb = (value >> 7) & 0x7F;
    midi_send(MIDI_PITCH_BEND | (channel & 0x0F), lsb, msb);
}

/* 볼륨 설정 */
static void set_volume(int channel, int volume)
{
    control_change(channel, CC_VOLUME, volume);
}

/* 팬 설정 */
static void set_pan(int channel, int pan)
{
    /* pan: 0=L, 64=C, 127=R */
    control_change(channel, CC_PAN, pan);
}

/* 모든 노트 오프 */
static void all_notes_off(int channel)
{
    control_change(channel, CC_ALL_NOTES_OFF, 0);
}

/* 모든 사운드 오프 */
static void all_sound_off(int channel)
{
    control_change(channel, CC_ALL_SOUND_OFF, 0);
}

/* GM 리셋 */
static void gm_reset(void)
{
    /* GM System On */
    midi_out(0xF0);     /* SysEx 시작 */
    midi_out(0x7E);     /* Universal Non-Real Time */
    midi_out(0x7F);     /* Device ID (all) */
    midi_out(0x09);     /* GM Message */
    midi_out(0x01);     /* GM System On */
    midi_out(0xF7);     /* SysEx 종료 */
    delay_long(50);     /* 리셋 대기 (500ms) */
}

/* 데모 1: 기본 노트 출력 */
static void demo_basic_notes(void)
{
    int notes[] = {NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5};
    int i;

    print("=== Demo 1: Basic MIDI Notes ===");
    newline();
    print("Playing C major scale on channel 1");
    newline();

    /* 피아노 음색 설정 */
    program_change(0, GM_PIANO);
    set_volume(0, 100);

    /* 음계 연주 */
    for (i = 0; i < 8; i++) {
        print("  Note: ");
        print_num(notes[i]);
        newline();

        note_on(0, notes[i], 100);
        delay_long(30);     /* 300ms */
        note_off(0, notes[i], 0);
        delay_long(5);      /* 50ms gap */
    }

    newline();
}

/* 데모 2: 음색 변경 */
static void demo_program_change(void)
{
    int programs[] = {GM_PIANO, GM_E_PIANO, GM_ORGAN, GM_GUITAR, GM_STRINGS, GM_TRUMPET, GM_FLUTE, GM_SAWTOOTH};
    const char *names[] = {"Piano", "E.Piano", "Organ", "Guitar", "Strings", "Trumpet", "Flute", "Synth Lead"};
    int i;

    print("=== Demo 2: Program Change (Instruments) ===");
    newline();
    print("Same note with different instruments");
    newline();

    for (i = 0; i < 8; i++) {
        print("  ");
        print(names[i]);
        print(" (");
        print_num(programs[i]);
        print(")");
        newline();

        program_change(0, programs[i]);
        delay(10);

        note_on(0, NOTE_C4, 100);
        delay_long(40);
        note_off(0, NOTE_C4, 0);
        delay_long(10);
    }

    newline();
}

/* 데모 3: 화음 연주 */
static void demo_chords(void)
{
    print("=== Demo 3: Chords ===");
    newline();

    program_change(0, GM_PIANO);
    set_volume(0, 90);

    /* C major chord */
    print("  C major (C-E-G)");
    newline();
    note_on(0, NOTE_C4, 80);
    note_on(0, NOTE_E4, 80);
    note_on(0, NOTE_G4, 80);
    delay_long(60);
    note_off(0, NOTE_C4, 0);
    note_off(0, NOTE_E4, 0);
    note_off(0, NOTE_G4, 0);
    delay_long(10);

    /* F major chord */
    print("  F major (F-A-C)");
    newline();
    note_on(0, NOTE_F4, 80);
    note_on(0, NOTE_A4, 80);
    note_on(0, NOTE_C5, 80);
    delay_long(60);
    note_off(0, NOTE_F4, 0);
    note_off(0, NOTE_A4, 0);
    note_off(0, NOTE_C5, 0);
    delay_long(10);

    /* G major chord */
    print("  G major (G-B-D)");
    newline();
    note_on(0, NOTE_G4, 80);
    note_on(0, NOTE_B4, 80);
    note_on(0, NOTE_D4 + 12, 80);    /* D5 */
    delay_long(60);
    note_off(0, NOTE_G4, 0);
    note_off(0, NOTE_B4, 0);
    note_off(0, NOTE_D4 + 12, 0);
    delay_long(10);

    /* C major chord (resolution) */
    print("  C major (resolution)");
    newline();
    note_on(0, NOTE_C4, 90);
    note_on(0, NOTE_E4, 90);
    note_on(0, NOTE_G4, 90);
    note_on(0, NOTE_C5, 90);
    delay_long(100);
    note_off(0, NOTE_C4, 0);
    note_off(0, NOTE_E4, 0);
    note_off(0, NOTE_G4, 0);
    note_off(0, NOTE_C5, 0);

    newline();
}

/* 데모 4: 컨트롤 체인지 (볼륨, 팬) */
static void demo_control_change(void)
{
    int i;

    print("=== Demo 4: Control Change ===");
    newline();

    program_change(0, GM_STRINGS);

    /* 볼륨 페이드 인 */
    print("  Volume fade in...");
    newline();
    note_on(0, NOTE_C4, 100);
    for (i = 0; i <= 127; i += 8) {
        set_volume(0, i);
        delay_long(3);
    }
    note_off(0, NOTE_C4, 0);
    delay_long(10);

    /* 팬 좌우 이동 */
    print("  Pan L -> C -> R...");
    newline();
    set_volume(0, 100);
    note_on(0, NOTE_E4, 100);
    for (i = 0; i <= 127; i += 4) {
        set_pan(0, i);
        delay_long(2);
    }
    note_off(0, NOTE_E4, 0);
    set_pan(0, 64);     /* 센터로 복귀 */
    delay_long(10);

    /* 볼륨 페이드 아웃 */
    print("  Volume fade out...");
    newline();
    note_on(0, NOTE_G4, 100);
    for (i = 127; i >= 0; i -= 8) {
        set_volume(0, i);
        delay_long(3);
    }
    note_off(0, NOTE_G4, 0);
    set_volume(0, 100);

    newline();
}

/* 데모 5: 피치 벤드 */
static void demo_pitch_bend(void)
{
    int i;
    int center = 8192;

    print("=== Demo 5: Pitch Bend ===");
    newline();

    program_change(0, GM_E_GUITAR);
    set_volume(0, 100);

    /* 피치 벤드 업 */
    print("  Pitch bend up...");
    newline();
    note_on(0, NOTE_E4, 100);
    for (i = center; i <= 16383; i += 256) {
        pitch_bend(0, i);
        delay_long(1);
    }
    delay_long(10);
    for (i = 16383; i >= center; i -= 256) {
        pitch_bend(0, i);
        delay_long(1);
    }
    note_off(0, NOTE_E4, 0);
    delay_long(10);

    /* 피치 벤드 다운 */
    print("  Pitch bend down...");
    newline();
    note_on(0, NOTE_A4, 100);
    for (i = center; i >= 0; i -= 256) {
        pitch_bend(0, i);
        delay_long(1);
    }
    delay_long(10);
    for (i = 0; i <= center; i += 256) {
        pitch_bend(0, i);
        delay_long(1);
    }
    note_off(0, NOTE_A4, 0);

    /* 피치 벤드 리셋 */
    pitch_bend(0, center);

    newline();
}

/* 데모 6: 멀티채널 연주 */
static void demo_multichannel(void)
{
    int i;

    print("=== Demo 6: Multi-channel Playback ===");
    newline();
    print("3 instruments playing together");
    newline();

    /* 채널별 음색 설정 */
    program_change(0, GM_PIANO);        /* Ch1: 피아노 (멜로디) */
    program_change(1, GM_E_BASS);       /* Ch2: 베이스 */
    program_change(2, GM_STRINGS);      /* Ch3: 스트링 (패드) */

    /* 볼륨/팬 설정 */
    set_volume(0, 100);
    set_volume(1, 90);
    set_volume(2, 70);
    set_pan(0, 64);     /* 센터 */
    set_pan(1, 64);     /* 센터 */
    set_pan(2, 64);     /* 센터 */

    /* 4마디 연주 */
    for (i = 0; i < 4; i++) {
        print("  Bar ");
        print_num(i + 1);
        newline();

        /* 스트링 패드 (C, F, G, C) */
        int pad_notes[] = {NOTE_C4 - 12, NOTE_F4 - 12, NOTE_G4 - 12, NOTE_C4 - 12};
        note_on(2, pad_notes[i], 60);
        note_on(2, pad_notes[i] + 4, 60);   /* +3rd */
        note_on(2, pad_notes[i] + 7, 60);   /* +5th */

        /* 베이스 (루트) */
        note_on(1, pad_notes[i] - 12, 100);

        /* 멜로디 */
        int melody[] = {NOTE_E4, NOTE_G4, NOTE_A4, NOTE_G4};
        note_on(0, melody[i], 90);
        delay_long(25);
        note_off(0, melody[i], 0);

        note_on(0, melody[(i + 1) % 4], 85);
        delay_long(25);
        note_off(0, melody[(i + 1) % 4], 0);

        /* 노트 오프 */
        note_off(1, pad_notes[i] - 12, 0);
        note_off(2, pad_notes[i], 0);
        note_off(2, pad_notes[i] + 4, 0);
        note_off(2, pad_notes[i] + 7, 0);

        delay_long(10);
    }

    newline();
}

/* 데모 7: 드럼 채널 */
static void demo_drums(void)
{
    int i;

    /* GM 드럼 노트 (채널 10) */
    #define DRUM_BASS       36      /* 베이스 드럼 */
    #define DRUM_SNARE      38      /* 스네어 */
    #define DRUM_HIHAT_C    42      /* 하이햇 (클로즈) */
    #define DRUM_HIHAT_O    46      /* 하이햇 (오픈) */
    #define DRUM_CRASH      49      /* 크래시 심벌 */
    #define DRUM_RIDE       51      /* 라이드 심벌 */
    #define DRUM_TOM_H      50      /* 하이 톰 */
    #define DRUM_TOM_M      47      /* 미드 톰 */
    #define DRUM_TOM_L      45      /* 로우 톰 */

    print("=== Demo 7: Drum Channel (Ch.10) ===");
    newline();
    print("Basic drum pattern");
    newline();

    set_volume(9, 100);     /* 채널 10 (인덱스 9) */

    /* 8비트 패턴 2마디 */
    for (i = 0; i < 16; i++) {
        /* 하이햇 - 매 비트 */
        note_on(9, DRUM_HIHAT_C, 80);

        /* 베이스 드럼 - 1, 5, 9, 13 비트 */
        if (i % 4 == 0) {
            note_on(9, DRUM_BASS, 110);
        }

        /* 스네어 - 3, 7, 11, 15 비트 */
        if (i % 4 == 2) {
            note_on(9, DRUM_SNARE, 100);
        }

        delay_long(15);     /* 150ms per beat */

        note_off(9, DRUM_HIHAT_C, 0);
        note_off(9, DRUM_BASS, 0);
        note_off(9, DRUM_SNARE, 0);
    }

    /* 필인 */
    print("  Fill in!");
    newline();
    note_on(9, DRUM_TOM_H, 100);
    delay_long(8);
    note_off(9, DRUM_TOM_H, 0);
    note_on(9, DRUM_TOM_M, 100);
    delay_long(8);
    note_off(9, DRUM_TOM_M, 0);
    note_on(9, DRUM_TOM_L, 100);
    delay_long(8);
    note_off(9, DRUM_TOM_L, 0);
    note_on(9, DRUM_CRASH, 120);
    delay_long(30);
    note_off(9, DRUM_CRASH, 0);

    newline();
}

/* 데모 8: MIDI 입력 모니터 */
static void demo_midi_input(void)
{
    int count = 0;
    int data;
    struct iocs_time start;

    print("=== Demo 8: MIDI Input Monitor ===");
    newline();
    print("Monitoring MIDI input for 10 seconds...");
    newline();
    print("(Connect a MIDI keyboard to see data)");
    newline();
    newline();

    start = _iocs_ontime();

    while (1) {
        struct iocs_time now = _iocs_ontime();
        if (now.sec - start.sec >= 1000) break;  /* 10초 */

        /* MIDI 입력 확인 */
        if (_iocs_isns232c() != 0) {
            data = _iocs_inp232c();
        } else {
            data = -1;
        }
        if (data >= 0) {
            print("  RX: 0x");
            print_hex(data, 2);

            /* 상태 바이트 분석 */
            if (data >= 0x80) {
                int status = data & 0xF0;
                int channel = (data & 0x0F) + 1;
                print(" (Ch.");
                print_num(channel);
                print(" ");
                switch (status) {
                case MIDI_NOTE_OFF:       print("NoteOff"); break;
                case MIDI_NOTE_ON:        print("NoteOn"); break;
                case MIDI_POLY_PRESSURE:  print("PolyAT"); break;
                case MIDI_CONTROL_CHANGE: print("CC"); break;
                case MIDI_PROGRAM_CHANGE: print("PC"); break;
                case MIDI_CH_PRESSURE:    print("ChAT"); break;
                case MIDI_PITCH_BEND:     print("PitchBend"); break;
                case MIDI_SYSTEM:         print("System"); break;
                }
                print(")");
            }
            newline();
            count++;
        }

        /* ESC로 중단 */
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            if ((key >> 8) == 0x01) break;
        }
    }

    print("Received ");
    print_num(count);
    print(" bytes.");
    newline();
    newline();
}

/* MIDI 클린업 */
static void midi_cleanup(void)
{
    int i;
    for (i = 0; i < 16; i++) {
        all_notes_off(i);
        all_sound_off(i);
        pitch_bend(i, 8192);
        set_volume(i, 100);
        set_pan(i, 64);
    }
}

int main(void)
{
    /* 화면 클리어 */
    _dos_c_cls_al();

    print("========================================");
    newline();
    print("  X68000 MIDI Demo");
    newline();
    print("========================================");
    newline();
    newline();

    print("This demo requires a MIDI sound module.");
    newline();
    print("Connect GM/GS compatible device to MIDI OUT.");
    newline();
    newline();
    print("Press any key to start...");
    newline();
    _iocs_b_keyinp();

    /* MIDI 초기화 */
    _dos_c_cls_al();
    print("Initializing MIDI (RS-232C at 31.25kbaud)...");
    newline();
    midi_init();
    print("Done.");
    newline();
    newline();

    /* GM 리셋 */
    print("Sending GM Reset...");
    newline();
    gm_reset();
    print("Done.");
    newline();
    newline();

    print("Press any key for each demo...");
    newline();
    _iocs_b_keyinp();

    /* 데모 실행 */
    _dos_c_cls_al();
    demo_basic_notes();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_program_change();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_chords();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_control_change();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_pitch_bend();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_multichannel();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_drums();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_midi_input();
    print("Press any key...");
    _iocs_b_keyinp();

    /* 클린업 */
    midi_cleanup();

    /* 종료 */
    _dos_c_cls_al();
    print("MIDI demo complete!");
    newline();
    print("Press any key to exit...");
    newline();
    _iocs_b_keyinp();

    return 0;
}
