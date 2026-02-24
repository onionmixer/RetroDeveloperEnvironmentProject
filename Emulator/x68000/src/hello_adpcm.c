/*
 * X68000 Human68k - ADPCM Sound Example
 * ADPCM (MSM6258) 음성 재생 예제 (IOCS)
 *
 * X68000 ADPCM 사양:
 * - MSM6258 ADPCM 코덱
 * - 4비트 ADPCM 인코딩
 * - 샘플레이트: 3.9kHz, 5.2kHz, 7.8kHz, 10.4kHz, 15.6kHz
 * - 모노 오디오
 */
#include <x68k/iocs.h>
#include <x68k/dos.h>
#include <string.h>

/* ADPCM 샘플레이트 */
#define ADPCM_3_9KHZ    0   /* 3.9kHz */
#define ADPCM_5_2KHZ    1   /* 5.2kHz */
#define ADPCM_7_8KHZ    2   /* 7.8kHz */
#define ADPCM_10_4KHZ   3   /* 10.4kHz */
#define ADPCM_15_6KHZ   4   /* 15.6kHz */

/* ADPCM 모드 */
#define ADPCM_MODE_STOP     0
#define ADPCM_MODE_PLAY     1
#define ADPCM_MODE_REC      2

/* ADPCM 상태 비트 */
#define ADPCM_STAT_PLAY     0x01    /* 재생 중 */
#define ADPCM_STAT_REC      0x02    /* 녹음 중 */

/* 샘플 데이터 버퍼 크기 */
#define SAMPLE_SIZE         4096

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

/* 딜레이 */
static void delay(int count)
{
    volatile int i;
    for (i = 0; i < count * 1000; i++)
        ;
}

/* ADPCM 상태 대기 */
static void wait_adpcm(void)
{
    while (_iocs_adpcmsns() & ADPCM_STAT_PLAY) {
        delay(1);
    }
}

/* ADPCM 정지 */
static void stop_adpcm(void)
{
    _iocs_adpcmmod(ADPCM_MODE_STOP);
}

/*
 * ADPCM 데이터 생성
 * 실제 ADPCM은 차분 인코딩이지만, 간단한 테스트용으로 패턴 생성
 *
 * ADPCM 4비트 구조:
 * - 상위 4비트: 첫 번째 샘플
 * - 하위 4비트: 두 번째 샘플
 * - 값 범위: 0-15 (중앙=8, 무음)
 */

/* 무음 데이터 생성 */
static void generate_silence(unsigned char *buf, int size)
{
    memset(buf, 0x88, size);  /* 중앙값 */
}

/* 톤 생성 (사각파 패턴) */
static void generate_tone(unsigned char *buf, int size, int freq_div)
{
    int i;
    int phase = 0;

    for (i = 0; i < size; i++) {
        int sample1, sample2;

        /* 사각파 생성 */
        sample1 = (phase / freq_div) % 2 ? 0x0F : 0x00;
        phase++;
        sample2 = (phase / freq_div) % 2 ? 0x0F : 0x00;
        phase++;

        buf[i] = (sample1 << 4) | sample2;
    }
}

/* 노이즈 생성 */
static void generate_noise(unsigned char *buf, int size)
{
    int i;
    static unsigned int seed = 12345;

    for (i = 0; i < size; i++) {
        /* 간단한 난수 생성 */
        seed = seed * 1103515245 + 12345;
        int sample1 = (seed >> 16) & 0x0F;
        seed = seed * 1103515245 + 12345;
        int sample2 = (seed >> 16) & 0x0F;

        buf[i] = (sample1 << 4) | sample2;
    }
}

/* 삼각파 생성 */
static void generate_triangle(unsigned char *buf, int size, int period)
{
    int i;

    for (i = 0; i < size; i++) {
        int pos1 = (i * 2) % period;
        int pos2 = (i * 2 + 1) % period;

        int sample1, sample2;

        /* 상승/하강 삼각파 */
        if (pos1 < period / 2) {
            sample1 = pos1 * 16 / (period / 2);
        } else {
            sample1 = 15 - (pos1 - period / 2) * 16 / (period / 2);
        }

        if (pos2 < period / 2) {
            sample2 = pos2 * 16 / (period / 2);
        } else {
            sample2 = 15 - (pos2 - period / 2) * 16 / (period / 2);
        }

        if (sample1 > 15) sample1 = 15;
        if (sample1 < 0) sample1 = 0;
        if (sample2 > 15) sample2 = 15;
        if (sample2 < 0) sample2 = 0;

        buf[i] = (sample1 << 4) | sample2;
    }
}

/* 페이드 아웃 효과 */
static void apply_fadeout(unsigned char *buf, int size)
{
    int i;

    for (i = 0; i < size; i++) {
        int sample1 = (buf[i] >> 4) & 0x0F;
        int sample2 = buf[i] & 0x0F;

        /* 중앙값으로 점점 수렴 */
        int factor = (size - i) * 256 / size;

        sample1 = 8 + ((sample1 - 8) * factor / 256);
        sample2 = 8 + ((sample2 - 8) * factor / 256);

        if (sample1 > 15) sample1 = 15;
        if (sample1 < 0) sample1 = 0;
        if (sample2 > 15) sample2 = 15;
        if (sample2 < 0) sample2 = 0;

        buf[i] = (sample1 << 4) | sample2;
    }
}

/* 데모 1: 기본 ADPCM 재생 */
static void demo_basic_playback(void)
{
    unsigned char *sample;

    print("=== Demo 1: Basic ADPCM Playback ===");
    newline();
    newline();

    /* 메모리 할당 */
    sample = (unsigned char *)_dos_malloc(SAMPLE_SIZE);
    if (sample == NULL || (long)sample < 0) {
        print("Memory allocation failed!");
        newline();
        return;
    }

    /* 톤 생성 */
    print("Generating tone...");
    newline();
    generate_tone(sample, SAMPLE_SIZE, 8);

    /* ADPCM 재생 */
    print("Playing at 15.6kHz...");
    newline();

    _iocs_adpcmout(sample, SAMPLE_SIZE, ADPCM_15_6KHZ);

    /* 재생 완료 대기 */
    wait_adpcm();

    print("Playback complete.");
    newline();

    _dos_mfree(sample);
    newline();
}

/* 데모 2: 다양한 샘플레이트 */
static void demo_sample_rates(void)
{
    unsigned char *sample;
    const char *rate_names[] = {
        "3.9kHz", "5.2kHz", "7.8kHz", "10.4kHz", "15.6kHz"
    };
    int rates[] = {
        ADPCM_3_9KHZ, ADPCM_5_2KHZ, ADPCM_7_8KHZ,
        ADPCM_10_4KHZ, ADPCM_15_6KHZ
    };
    int i;

    print("=== Demo 2: Sample Rates ===");
    newline();
    print("Same tone at different sample rates.");
    newline();
    newline();

    sample = (unsigned char *)_dos_malloc(SAMPLE_SIZE);
    if (sample == NULL || (long)sample < 0) {
        print("Memory allocation failed!");
        newline();
        return;
    }

    generate_tone(sample, SAMPLE_SIZE, 8);

    for (i = 0; i < 5; i++) {
        print("Playing at ");
        print(rate_names[i]);
        print("...");
        newline();

        _iocs_adpcmout(sample, SAMPLE_SIZE, rates[i]);
        wait_adpcm();

        delay(200);
    }

    print("Done.");
    newline();

    _dos_mfree(sample);
    newline();
}

/* 데모 3: 다양한 파형 */
static void demo_waveforms(void)
{
    unsigned char *sample;

    print("=== Demo 3: Waveforms ===");
    newline();
    newline();

    sample = (unsigned char *)_dos_malloc(SAMPLE_SIZE);
    if (sample == NULL || (long)sample < 0) {
        print("Memory allocation failed!");
        newline();
        return;
    }

    /* 사각파 */
    print("Square wave...");
    newline();
    generate_tone(sample, SAMPLE_SIZE, 16);
    _iocs_adpcmout(sample, SAMPLE_SIZE, ADPCM_15_6KHZ);
    wait_adpcm();
    delay(200);

    /* 삼각파 */
    print("Triangle wave...");
    newline();
    generate_triangle(sample, SAMPLE_SIZE, 64);
    _iocs_adpcmout(sample, SAMPLE_SIZE, ADPCM_15_6KHZ);
    wait_adpcm();
    delay(200);

    /* 노이즈 */
    print("Noise...");
    newline();
    generate_noise(sample, SAMPLE_SIZE);
    _iocs_adpcmout(sample, SAMPLE_SIZE, ADPCM_15_6KHZ);
    wait_adpcm();
    delay(200);

    /* 페이드 아웃 톤 */
    print("Fade out tone...");
    newline();
    generate_tone(sample, SAMPLE_SIZE, 8);
    apply_fadeout(sample, SAMPLE_SIZE);
    _iocs_adpcmout(sample, SAMPLE_SIZE, ADPCM_15_6KHZ);
    wait_adpcm();

    print("Done.");
    newline();

    _dos_mfree(sample);
    newline();
}

/* 데모 4: 멜로디 재생 */
static void demo_melody(void)
{
    unsigned char *sample;
    /* 음계 주파수 (분주값으로 표현) */
    int notes[] = {
        32,  /* 도 (낮은) */
        28,  /* 레 */
        25,  /* 미 */
        24,  /* 파 */
        21,  /* 솔 */
        19,  /* 라 */
        17,  /* 시 */
        16,  /* 도 (높은) */
    };
    const char *note_names[] = {
        "C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5"
    };
    int i;
    int note_size = SAMPLE_SIZE / 4;

    print("=== Demo 4: Melody (Do Re Mi...) ===");
    newline();
    newline();

    sample = (unsigned char *)_dos_malloc(note_size);
    if (sample == NULL || (long)sample < 0) {
        print("Memory allocation failed!");
        newline();
        return;
    }

    for (i = 0; i < 8; i++) {
        print("  ");
        print(note_names[i]);
        print("...");
        newline();

        generate_tone(sample, note_size, notes[i]);
        _iocs_adpcmout(sample, note_size, ADPCM_15_6KHZ);
        wait_adpcm();

        delay(50);
    }

    print("Done.");
    newline();

    _dos_mfree(sample);
    newline();
}

/* 데모 5: 효과음 */
static void demo_sound_effects(void)
{
    unsigned char *sample;
    int i;

    print("=== Demo 5: Sound Effects ===");
    newline();
    newline();

    sample = (unsigned char *)_dos_malloc(SAMPLE_SIZE);
    if (sample == NULL || (long)sample < 0) {
        print("Memory allocation failed!");
        newline();
        return;
    }

    /* 상승음 (Sweep Up) */
    print("Sweep up...");
    newline();
    for (i = 0; i < SAMPLE_SIZE; i++) {
        int freq = 32 - (i * 28 / SAMPLE_SIZE);
        if (freq < 4) freq = 4;
        int phase = (i * 2) / freq;
        int sample1 = (phase % 2) ? 0x0F : 0x00;
        int sample2 = ((phase + 1) % 2) ? 0x0F : 0x00;
        sample[i] = (sample1 << 4) | sample2;
    }
    _iocs_adpcmout(sample, SAMPLE_SIZE, ADPCM_15_6KHZ);
    wait_adpcm();
    delay(200);

    /* 하강음 (Sweep Down) */
    print("Sweep down...");
    newline();
    for (i = 0; i < SAMPLE_SIZE; i++) {
        int freq = 4 + (i * 28 / SAMPLE_SIZE);
        int phase = (i * 2) / freq;
        int sample1 = (phase % 2) ? 0x0F : 0x00;
        int sample2 = ((phase + 1) % 2) ? 0x0F : 0x00;
        sample[i] = (sample1 << 4) | sample2;
    }
    _iocs_adpcmout(sample, SAMPLE_SIZE, ADPCM_15_6KHZ);
    wait_adpcm();
    delay(200);

    /* 폭발음 */
    print("Explosion...");
    newline();
    generate_noise(sample, SAMPLE_SIZE);
    apply_fadeout(sample, SAMPLE_SIZE);
    _iocs_adpcmout(sample, SAMPLE_SIZE, ADPCM_7_8KHZ);  /* 낮은 샘플레이트 */
    wait_adpcm();
    delay(200);

    /* 비프음 */
    print("Beep...");
    newline();
    generate_tone(sample, SAMPLE_SIZE / 8, 8);
    _iocs_adpcmout(sample, SAMPLE_SIZE / 8, ADPCM_15_6KHZ);
    wait_adpcm();

    print("Done.");
    newline();

    _dos_mfree(sample);
    newline();
}

/* 데모 6: ADPCM 상태 모니터링 */
static void demo_status(void)
{
    unsigned char *sample;
    int status;
    int frames = 0;

    print("=== Demo 6: ADPCM Status Monitoring ===");
    newline();
    newline();

    sample = (unsigned char *)_dos_malloc(SAMPLE_SIZE * 2);
    if (sample == NULL || (long)sample < 0) {
        print("Memory allocation failed!");
        newline();
        return;
    }

    generate_tone(sample, SAMPLE_SIZE * 2, 8);

    print("Playing long sample...");
    newline();
    print("Monitoring status:");
    newline();

    _iocs_adpcmout(sample, SAMPLE_SIZE * 2, ADPCM_7_8KHZ);

    while (1) {
        status = _iocs_adpcmsns();

        _dos_c_locate(0, 6);
        print("Status: 0x");
        print_hex(status, 2);
        print(" ");

        if (status & ADPCM_STAT_PLAY) {
            print("[PLAYING]");
        } else {
            print("[STOPPED]");
        }

        if (status & ADPCM_STAT_REC) {
            print(" [RECORDING]");
        }

        _dos_c_locate(0, 7);
        print("Frame: ");
        print_num(frames++);
        print("    ");

        if (!(status & ADPCM_STAT_PLAY)) {
            break;
        }

        delay(10);
    }

    newline();
    print("Playback finished after ");
    print_num(frames);
    print(" frames.");
    newline();

    _dos_mfree(sample);
    newline();
}

/* 데모 7: 인터랙티브 사운드보드 */
static void demo_soundboard(void)
{
    unsigned char *samples[4];
    int sample_sizes[4] = {1024, 1024, 2048, 512};
    int i;
    int running = 1;

    print("=== Demo 7: Interactive Soundboard ===");
    newline();
    print("Press 1-4 to play sounds, ESC to exit.");
    newline();
    newline();

    /* 샘플 생성 */
    for (i = 0; i < 4; i++) {
        samples[i] = (unsigned char *)_dos_malloc(sample_sizes[i]);
        if (samples[i] == NULL || (long)samples[i] < 0) {
            print("Memory allocation failed!");
            newline();
            /* 이미 할당된 것 해제 */
            while (--i >= 0) {
                _dos_mfree(samples[i]);
            }
            return;
        }
    }

    /* 다양한 효과음 생성 */
    generate_tone(samples[0], sample_sizes[0], 8);      /* 1: 비프 */
    generate_noise(samples[1], sample_sizes[1]);         /* 2: 노이즈 */
    generate_triangle(samples[2], sample_sizes[2], 32);  /* 3: 부드러운 톤 */
    generate_tone(samples[3], sample_sizes[3], 4);       /* 4: 높은 톤 */

    print("1: Beep");
    newline();
    print("2: Noise");
    newline();
    print("3: Soft tone");
    newline();
    print("4: High tone");
    newline();
    newline();

    while (running) {
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            int charcode = key & 0xFF;
            int scancode = (key >> 8) & 0xFF;

            /* ESC 종료 */
            if (scancode == 0x01) {
                running = 0;
                break;
            }

            /* 재생 중이면 정지 */
            stop_adpcm();

            switch (charcode) {
                case '1':
                    print("Playing: Beep    \r");
                    _iocs_adpcmout(samples[0], sample_sizes[0], ADPCM_15_6KHZ);
                    break;
                case '2':
                    print("Playing: Noise   \r");
                    _iocs_adpcmout(samples[1], sample_sizes[1], ADPCM_10_4KHZ);
                    break;
                case '3':
                    print("Playing: Soft    \r");
                    _iocs_adpcmout(samples[2], sample_sizes[2], ADPCM_15_6KHZ);
                    break;
                case '4':
                    print("Playing: High    \r");
                    _iocs_adpcmout(samples[3], sample_sizes[3], ADPCM_15_6KHZ);
                    break;
            }
        }
    }

    stop_adpcm();

    /* 메모리 해제 */
    for (i = 0; i < 4; i++) {
        _dos_mfree(samples[i]);
    }

    newline();
}

int main(void)
{
    /* 화면 클리어 */
    _dos_c_cls_al();

    print("========================================");
    newline();
    print("  X68000 ADPCM Sound Demo");
    newline();
    print("========================================");
    newline();
    newline();

    print("This demo shows ADPCM (MSM6258) audio.");
    newline();
    print("Make sure speakers are connected.");
    newline();
    newline();
    print("Press any key to start...");
    newline();
    _iocs_b_keyinp();

    /* 데모 실행 */
    _dos_c_cls_al();
    demo_basic_playback();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_sample_rates();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_waveforms();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_melody();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_sound_effects();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_status();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_soundboard();

    /* ADPCM 정지 */
    stop_adpcm();

    /* 종료 */
    _dos_c_cls_al();
    print("ADPCM demo complete!");
    newline();
    print("Press any key to exit...");
    newline();
    _iocs_b_keyinp();

    return 0;
}
