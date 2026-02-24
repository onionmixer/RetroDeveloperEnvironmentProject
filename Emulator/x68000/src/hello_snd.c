/*
 * X68000 Human68k - Sound Example
 * OPM (YM2151) FM 음원을 사용한 사운드 출력 예제
 *
 * X68000 사운드 사양:
 * - YM2151 (OPM): 8채널 4오퍼레이터 FM 음원
 * - MSM6258: ADPCM 음성 재생
 * - OKI MSM6295: PCM (일부 모델)
 */
#include <x68k/iocs.h>

/* OPM (YM2151) 레지스터 주소 */
#define OPM_REG_TEST        0x01    /* 테스트 레지스터 */
#define OPM_REG_KEYON       0x08    /* 키 온/오프 */
#define OPM_REG_NOISE       0x0F    /* 노이즈 설정 */
#define OPM_REG_CLKA1       0x10    /* 타이머 A (상위) */
#define OPM_REG_CLKA2       0x11    /* 타이머 A (하위) */
#define OPM_REG_CLKB        0x12    /* 타이머 B */
#define OPM_REG_TIMER       0x14    /* 타이머 제어 */
#define OPM_REG_LFRQ        0x18    /* LFO 주파수 */
#define OPM_REG_PMD_AMD     0x19    /* PMD/AMD */
#define OPM_REG_CT_W        0x1B    /* CT/Waveform */

/* 채널별 레지스터 오프셋 (채널 0-7) */
#define OPM_REG_PAN_FL_CON  0x20    /* PAN/FL/CON */
#define OPM_REG_KC          0x28    /* Key Code (음정) */
#define OPM_REG_KF          0x30    /* Key Fraction */
#define OPM_REG_PMS_AMS     0x38    /* PMS/AMS */

/* 오퍼레이터별 레지스터 (채널 + 오퍼레이터*8) */
#define OPM_REG_DT1_MUL     0x40    /* DT1/MUL */
#define OPM_REG_TL          0x60    /* Total Level (볼륨) */
#define OPM_REG_KS_AR       0x80    /* KS/AR */
#define OPM_REG_AME_D1R     0xA0    /* AME/D1R */
#define OPM_REG_DT2_D2R     0xC0    /* DT2/D2R */
#define OPM_REG_D1L_RR      0xE0    /* D1L/RR */

/* 키 코드 (옥타브 * 16 + 음계) */
#define NOTE_C   0
#define NOTE_CS  1
#define NOTE_D   2
#define NOTE_DS  3
#define NOTE_E   4
#define NOTE_F   5
#define NOTE_FS  6
#define NOTE_G   8
#define NOTE_GS  9
#define NOTE_A   10
#define NOTE_AS  11
#define NOTE_B   12

/* OPM 레지스터 설정 */
static void opm_write(int reg, int data)
{
    /* OPM이 준비될 때까지 대기 */
    while (_iocs_opmsns() & 0x80)
        ;
    _iocs_opmset(reg, data);
}

/* 딜레이 (간단한 루프) */
static void delay(int count)
{
    volatile int i;
    for (i = 0; i < count * 1000; i++)
        ;
}

/* OPM 초기화 */
static void opm_init(void)
{
    int i;

    /* 모든 채널 키 오프 */
    for (i = 0; i < 8; i++) {
        opm_write(OPM_REG_KEYON, i);
    }

    /* LFO 비활성화 */
    opm_write(OPM_REG_LFRQ, 0);
    opm_write(OPM_REG_PMD_AMD, 0);
}

/* 간단한 사각파 악기 설정 (채널 0) */
static void setup_instrument(int ch)
{
    /* 알고리즘 7 (모든 오퍼레이터가 캐리어), 피드백 0, 좌우 출력 */
    opm_write(OPM_REG_PAN_FL_CON + ch, 0xC7);  /* L+R, FL=0, CON=7 */

    /* 오퍼레이터 1 (M1) 설정 - 채널 + 0 */
    opm_write(OPM_REG_DT1_MUL + ch, 0x01);     /* DT1=0, MUL=1 */
    opm_write(OPM_REG_TL + ch, 0x20);          /* Total Level (볼륨) */
    opm_write(OPM_REG_KS_AR + ch, 0x1F);       /* KS=0, AR=31 (빠른 어택) */
    opm_write(OPM_REG_AME_D1R + ch, 0x00);     /* AME=0, D1R=0 */
    opm_write(OPM_REG_DT2_D2R + ch, 0x00);     /* DT2=0, D2R=0 */
    opm_write(OPM_REG_D1L_RR + ch, 0x0F);      /* D1L=0, RR=15 */

    /* 오퍼레이터 2 (M2) - 채널 + 8 */
    opm_write(OPM_REG_TL + ch + 8, 0x7F);      /* 음소거 */

    /* 오퍼레이터 3 (C1) - 채널 + 16 */
    opm_write(OPM_REG_TL + ch + 16, 0x7F);     /* 음소거 */

    /* 오퍼레이터 4 (C2) - 채널 + 24 */
    opm_write(OPM_REG_TL + ch + 24, 0x7F);     /* 음소거 */
}

/* 음 재생 (키 온) */
static void play_note(int ch, int octave, int note)
{
    int kc = (octave << 4) | note;

    opm_write(OPM_REG_KC + ch, kc);    /* 음정 설정 */
    opm_write(OPM_REG_KF + ch, 0);     /* 미세 조정 없음 */

    /* 키 온 (슬롯 마스크 0x78 = 모든 오퍼레이터) */
    opm_write(OPM_REG_KEYON, 0x78 | ch);
}

/* 음 정지 (키 오프) */
static void stop_note(int ch)
{
    opm_write(OPM_REG_KEYON, ch);  /* 슬롯 마스크 0 = 키 오프 */
}

/* 볼륨 설정 */
static void set_volume(int ch, int vol)
{
    /* TL: 0 = 최대 볼륨, 127 = 무음 */
    opm_write(OPM_REG_TL + ch, 127 - vol);
}

/* 문자열 출력 */
static void print(const char *str)
{
    _iocs_b_print(str);
}

static void newline(void)
{
    _iocs_b_putc('\r');
    _iocs_b_putc('\n');
}

int main(void)
{
    int i;

    /* 음계 데이터: 도레미파솔라시도 */
    const int melody[] = {
        NOTE_C, NOTE_D, NOTE_E, NOTE_F,
        NOTE_G, NOTE_A, NOTE_B, NOTE_C
    };
    const int octaves[] = {
        4, 4, 4, 4, 4, 4, 4, 5
    };
    const char *note_names[] = {
        "C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5"
    };

    print("=== X68000 Sound Demo (OPM/YM2151) ===");
    newline();
    newline();

    print("Initializing OPM...");
    newline();

    /* OPM 초기화 */
    opm_init();

    /* 악기 설정 (채널 0) */
    setup_instrument(0);
    set_volume(0, 100);

    print("Playing scale: Do Re Mi Fa Sol La Si Do");
    newline();
    newline();

    /* 음계 연주 */
    for (i = 0; i < 8; i++) {
        print("  Playing: ");
        print(note_names[i]);
        newline();

        play_note(0, octaves[i], melody[i]);
        delay(300);
        stop_note(0);
        delay(100);
    }

    newline();
    print("Playing chord (C major)...");
    newline();

    /* 화음 연주 (C 메이저: 도-미-솔) */
    setup_instrument(0);
    setup_instrument(1);
    setup_instrument(2);
    set_volume(0, 80);
    set_volume(1, 80);
    set_volume(2, 80);

    play_note(0, 4, NOTE_C);   /* 도 */
    play_note(1, 4, NOTE_E);   /* 미 */
    play_note(2, 4, NOTE_G);   /* 솔 */
    delay(1000);
    stop_note(0);
    stop_note(1);
    stop_note(2);

    newline();
    print("Playing arpeggio...");
    newline();

    /* 아르페지오 */
    set_volume(0, 100);
    for (i = 0; i < 3; i++) {
        play_note(0, 4, NOTE_C);
        delay(100);
        stop_note(0);

        play_note(0, 4, NOTE_E);
        delay(100);
        stop_note(0);

        play_note(0, 4, NOTE_G);
        delay(100);
        stop_note(0);

        play_note(0, 5, NOTE_C);
        delay(100);
        stop_note(0);
    }

    newline();
    print("Sound demo complete!");
    newline();
    print("Press any key to exit...");
    newline();

    /* 키 입력 대기 */
    _iocs_b_keyinp();

    /* 모든 채널 키 오프 */
    for (i = 0; i < 8; i++) {
        stop_note(i);
    }

    return 0;
}
