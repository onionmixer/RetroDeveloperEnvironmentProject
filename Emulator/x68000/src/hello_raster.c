/*
 * X68000 Human68k - Raster Interrupt Example
 * 래스터 인터럽트 예제
 *
 * X68000 래스터 인터럽트:
 * - 특정 스캔라인에서 인터럽트 발생
 * - 화면 분할 효과
 * - 스캔라인별 팔레트/스크롤 변경
 * - 수평 효과 (물결, 왜곡 등)
 */
#include <x68k/iocs.h>
#include <x68k/dos.h>

/* 화면 상수 */
#define SCREEN_WIDTH    512
#define SCREEN_HEIGHT   512
#define VISIBLE_HEIGHT  256     /* 실제 표시 영역 (15kHz) */

/* CRTC 레지스터 주소 */
#define CRTC_BASE       0xE80000
#define CRTC_R00        (*(volatile unsigned short *)(CRTC_BASE + 0x00))  /* H total */
#define CRTC_R01        (*(volatile unsigned short *)(CRTC_BASE + 0x02))  /* H sync end */
#define CRTC_R02        (*(volatile unsigned short *)(CRTC_BASE + 0x04))  /* H disp start */
#define CRTC_R03        (*(volatile unsigned short *)(CRTC_BASE + 0x06))  /* H disp end */
#define CRTC_R04        (*(volatile unsigned short *)(CRTC_BASE + 0x08))  /* V total */
#define CRTC_R05        (*(volatile unsigned short *)(CRTC_BASE + 0x0A))  /* V sync end */
#define CRTC_R06        (*(volatile unsigned short *)(CRTC_BASE + 0x0C))  /* V disp start */
#define CRTC_R07        (*(volatile unsigned short *)(CRTC_BASE + 0x0E))  /* V disp end */
#define CRTC_R09        (*(volatile unsigned short *)(CRTC_BASE + 0x12))  /* Raster number */
#define CRTC_R12        (*(volatile unsigned short *)(CRTC_BASE + 0x18))  /* Screen 0 X scroll */
#define CRTC_R13        (*(volatile unsigned short *)(CRTC_BASE + 0x1A))  /* Screen 0 Y scroll */
#define CRTC_R14        (*(volatile unsigned short *)(CRTC_BASE + 0x1C))  /* Screen 1 X scroll */
#define CRTC_R15        (*(volatile unsigned short *)(CRTC_BASE + 0x1E))  /* Screen 1 Y scroll */

/* MFP 레지스터 (래스터 인터럽트용) */
#define MFP_BASE        0xE88000
#define MFP_GPIP        (*(volatile unsigned char *)(MFP_BASE + 0x01))   /* General Purpose I/O */
#define MFP_IERA        (*(volatile unsigned char *)(MFP_BASE + 0x07))   /* Interrupt Enable A */
#define MFP_IPRA        (*(volatile unsigned char *)(MFP_BASE + 0x0B))   /* Interrupt Pending A */
#define MFP_ISRA        (*(volatile unsigned char *)(MFP_BASE + 0x0F))   /* Interrupt In-Service A */
#define MFP_IMRA        (*(volatile unsigned char *)(MFP_BASE + 0x13))   /* Interrupt Mask A */

/* VSync 대기 (MFP GPIP bit4 = VDISP) */
static void wait_vsync(void)
{
    /* VBlank 끝날 때까지 대기 */
    while (!(MFP_GPIP & 0x10)) { }
    /* VBlank 시작될 때까지 대기 */
    while (MFP_GPIP & 0x10) { }
}

/* 그래픽 VRAM */
#define GVRAM_BASE      0xC00000

/* 팔레트 */
#define PALETTE_BASE    0xE82000
#define GPALETTE(n)     (*(volatile unsigned short *)(PALETTE_BASE + (n) * 2))
#define TPALETTE(n)     (*(volatile unsigned short *)(0xE82200 + (n) * 2))

/* 전역 변수 (인터럽트 핸들러용) */
static volatile int raster_line = 0;
static volatile int raster_count = 0;
static volatile int effect_mode = 0;
static volatile int scroll_offset = 0;
static volatile int wave_phase = 0;

/* 래스터별 스크롤 테이블 */
static volatile short scroll_table[VISIBLE_HEIGHT];

/* 래스터별 팔레트 테이블 */
static volatile unsigned short palette_table[VISIBLE_HEIGHT];

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

/* GVRAM에 픽셀 쓰기 (16색 모드) */
static void gvram_pset(int x, int y, int color)
{
    volatile unsigned short *vram = (volatile unsigned short *)(GVRAM_BASE + (y * 1024 + x) * 2);
    *vram = color;
}

/* GVRAM 수평 라인 그리기 */
static void gvram_hline(int x1, int x2, int y, int color)
{
    volatile unsigned short *vram = (volatile unsigned short *)(GVRAM_BASE + (y * 1024 + x1) * 2);
    int i;
    for (i = x1; i <= x2; i++) {
        *vram++ = color;
    }
}

/* GVRAM 사각형 채우기 */
static void gvram_fill(int x1, int y1, int x2, int y2, int color)
{
    int y;
    for (y = y1; y <= y2; y++) {
        gvram_hline(x1, x2, y, color);
    }
}

/* GVRAM 클리어 */
static void gvram_clear(int color)
{
    volatile unsigned short *vram = (volatile unsigned short *)GVRAM_BASE;
    int i;
    for (i = 0; i < 512 * 512; i++) {
        *vram++ = color;
    }
}

/* 사인 테이블 (0-255 -> 0-127 -> -127~127) */
static const signed char sin_table[256] = {
    0, 3, 6, 9, 12, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46,
    49, 51, 54, 57, 60, 63, 65, 68, 71, 73, 76, 78, 81, 83, 85, 88,
    90, 92, 94, 96, 98, 100, 102, 104, 106, 107, 109, 111, 112, 113, 115, 116,
    117, 118, 120, 121, 122, 122, 123, 124, 125, 125, 126, 126, 126, 127, 127, 127,
    127, 127, 127, 127, 126, 126, 126, 125, 125, 124, 123, 122, 122, 121, 120, 118,
    117, 116, 115, 113, 112, 111, 109, 107, 106, 104, 102, 100, 98, 96, 94, 92,
    90, 88, 85, 83, 81, 78, 76, 73, 71, 68, 65, 63, 60, 57, 54, 51,
    49, 46, 43, 40, 37, 34, 31, 28, 25, 22, 19, 16, 12, 9, 6, 3,
    0, -3, -6, -9, -12, -16, -19, -22, -25, -28, -31, -34, -37, -40, -43, -46,
    -49, -51, -54, -57, -60, -63, -65, -68, -71, -73, -76, -78, -81, -83, -85, -88,
    -90, -92, -94, -96, -98, -100, -102, -104, -106, -107, -109, -111, -112, -113, -115, -116,
    -117, -118, -120, -121, -122, -122, -123, -124, -125, -125, -126, -126, -126, -127, -127, -127,
    -127, -127, -127, -127, -126, -126, -126, -125, -125, -124, -123, -122, -122, -121, -120, -118,
    -117, -116, -115, -113, -112, -111, -109, -107, -106, -104, -102, -100, -98, -96, -94, -92,
    -90, -88, -85, -83, -81, -78, -76, -73, -71, -68, -65, -63, -60, -57, -54, -51,
    -49, -46, -43, -40, -37, -34, -31, -28, -25, -22, -19, -16, -12, -9, -6, -3
};

/* 사인 값 얻기 (0-255 -> -127~127) */
static int get_sin(int angle)
{
    return sin_table[angle & 0xFF];
}

/* 래스터 인터럽트 핸들러 */
static void __attribute__((interrupt)) raster_handler(void)
{
    int line;

    /* 현재 스캔라인 얻기 */
    line = raster_line;
    raster_count++;

    /* 효과 모드에 따른 처리 */
    switch (effect_mode) {
    case 1:  /* 수평 스크롤 분할 */
        if (line < 128) {
            CRTC_R12 = scroll_offset;       /* 상단: 왼쪽으로 */
        } else {
            CRTC_R12 = -scroll_offset;      /* 하단: 오른쪽으로 */
        }
        break;

    case 2:  /* 물결 효과 */
        if (line < VISIBLE_HEIGHT) {
            CRTC_R12 = scroll_table[line];
        }
        break;

    case 3:  /* 그라데이션 팔레트 */
        if (line < VISIBLE_HEIGHT) {
            GPALETTE(1) = palette_table[line];
        }
        break;

    case 4:  /* 다중 분할 */
        {
            int zone = line / 64;
            CRTC_R12 = scroll_offset * (zone & 1 ? 1 : -1) * (zone + 1);
        }
        break;
    }

    /* 다음 래스터 라인 설정 */
    raster_line++;
    if (raster_line >= VISIBLE_HEIGHT) {
        raster_line = 0;
    }

    /* 래스터 번호 레지스터 업데이트 */
    CRTC_R09 = raster_line;

    /* MFP 인터럽트 클리어 */
    MFP_ISRA &= ~0x40;  /* GPIP6 (CRTC) 클리어 */
}

/* 래스터 인터럽트 설정 */
static void *old_raster_vector = 0;

static void setup_raster_interrupt(void)
{
    /* 기존 벡터 저장 및 새 핸들러 등록 */
    old_raster_vector = _iocs_b_intvcs(0x46, (void *)raster_handler);  /* CRTC raster */

    /* 초기 래스터 라인 설정 */
    raster_line = 0;
    CRTC_R09 = 0;

    /* MFP 래스터 인터럽트 활성화 */
    MFP_IERA |= 0x40;   /* GPIP6 활성화 */
    MFP_IMRA |= 0x40;   /* GPIP6 마스크 해제 */
}

static void cleanup_raster_interrupt(void)
{
    /* MFP 래스터 인터럽트 비활성화 */
    MFP_IMRA &= ~0x40;
    MFP_IERA &= ~0x40;

    /* 기존 벡터 복원 */
    if (old_raster_vector) {
        _iocs_b_intvcs(0x46, old_raster_vector);
    }

    /* 스크롤 리셋 */
    CRTC_R12 = 0;
    CRTC_R13 = 0;
}

/* 물결 스크롤 테이블 계산 */
static void calc_wave_scroll(int amplitude, int frequency, int phase)
{
    int i;
    for (i = 0; i < VISIBLE_HEIGHT; i++) {
        int angle = (i * frequency + phase) & 0xFF;
        scroll_table[i] = (get_sin(angle) * amplitude) >> 7;
    }
}

/* 그라데이션 팔레트 테이블 계산 */
static void calc_gradient_palette(int r1, int g1, int b1, int r2, int g2, int b2)
{
    int i;
    for (i = 0; i < VISIBLE_HEIGHT; i++) {
        int t = (i * 256) / VISIBLE_HEIGHT;
        int r = r1 + (((r2 - r1) * t) >> 8);
        int g = g1 + (((g2 - g1) * t) >> 8);
        int b = b1 + (((b2 - b1) * t) >> 8);
        /* X68000 RGB: GGGGGRRR RRBBBBBB (각 5비트) */
        palette_table[i] = ((g & 0x1F) << 11) | ((r & 0x1F) << 6) | (b & 0x1F);
    }
}

/* 테스트 패턴 그리기 */
static void draw_test_pattern(void)
{
    int y, x;

    /* 배경 스트라이프 */
    for (y = 0; y < 256; y++) {
        int color = (y / 16) & 1 ? 1 : 2;
        gvram_hline(0, 511, y, color);
    }

    /* 수직 라인 */
    for (x = 0; x < 512; x += 32) {
        for (y = 0; y < 256; y++) {
            gvram_pset(x, y, 3);
        }
    }

    /* 중앙 박스 */
    gvram_fill(192, 96, 320, 160, 4);
}

/* 체크보드 패턴 그리기 */
static void draw_checker_pattern(void)
{
    int y, x;
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 512; x++) {
            int color = ((x / 32) + (y / 32)) & 1 ? 1 : 2;
            gvram_pset(x, y, color);
        }
    }
}

/* 그라데이션 패턴 그리기 */
static void draw_gradient_pattern(void)
{
    int y;
    for (y = 0; y < 256; y++) {
        gvram_hline(0, 511, y, 1);  /* 팔레트 1 사용 */
    }
}

/* 기본 팔레트 설정 */
static void setup_palette(void)
{
    GPALETTE(0) = 0x0000;   /* 검정 */
    GPALETTE(1) = 0x07C0;   /* 빨강 */
    GPALETTE(2) = 0xF800;   /* 초록 */
    GPALETTE(3) = 0x003E;   /* 파랑 */
    GPALETTE(4) = 0xFFFF;   /* 흰색 */
    GPALETTE(5) = 0xFFC0;   /* 노랑 */
    GPALETTE(6) = 0x07FE;   /* 마젠타 */
    GPALETTE(7) = 0xF83E;   /* 시안 */
}

/* 데모 1: 화면 분할 스크롤 */
static void demo_split_scroll(void)
{
    int frame = 0;

    print("=== Demo 1: Split Screen Scroll ===");
    newline();
    print("Upper half scrolls left, lower half scrolls right");
    newline();
    print("Press any key to continue...");
    newline();

    /* 그래픽 모드 설정 */
    _iocs_crtmod(0);        /* 512x512 16색 */
    _iocs_g_clr_on();       /* 그래픽 화면 ON */

    setup_palette();
    draw_test_pattern();

    /* 래스터 인터럽트 설정 */
    effect_mode = 1;
    setup_raster_interrupt();

    while (!_iocs_b_keysns()) {
        scroll_offset = frame;
        frame += 2;

        wait_vsync();
    }
    _iocs_b_keyinp();

    cleanup_raster_interrupt();
}

/* 데모 2: 물결 효과 */
static void demo_wave_effect(void)
{
    int frame = 0;

    print("=== Demo 2: Wave Effect ===");
    newline();
    print("Sine wave horizontal distortion");
    newline();
    print("Press any key to continue...");
    newline();

    _iocs_crtmod(0);
    _iocs_g_clr_on();

    setup_palette();
    draw_checker_pattern();

    effect_mode = 2;
    setup_raster_interrupt();

    while (!_iocs_b_keysns()) {
        wave_phase = frame * 4;
        calc_wave_scroll(16, 8, wave_phase);    /* 진폭 16, 주파수 8 */
        frame++;

        wait_vsync();
    }
    _iocs_b_keyinp();

    cleanup_raster_interrupt();
}

/* 데모 3: 래스터 팔레트 (그라데이션) */
static void demo_raster_palette(void)
{
    int frame = 0;
    int phase = 0;

    print("=== Demo 3: Raster Palette (Gradient) ===");
    newline();
    print("Color changes per scanline");
    newline();
    print("Press any key to continue...");
    newline();

    _iocs_crtmod(0);
    _iocs_g_clr_on();

    setup_palette();
    draw_gradient_pattern();

    effect_mode = 3;
    setup_raster_interrupt();

    while (!_iocs_b_keysns()) {
        /* 애니메이션되는 그라데이션 */
        int r1 = (get_sin(phase) + 127) >> 3;
        int g1 = (get_sin(phase + 85) + 127) >> 3;
        int b1 = (get_sin(phase + 170) + 127) >> 3;
        int r2 = (get_sin(phase + 128) + 127) >> 3;
        int g2 = (get_sin(phase + 213) + 127) >> 3;
        int b2 = (get_sin(phase + 42) + 127) >> 3;

        calc_gradient_palette(r1, g1, b1, r2, g2, b2);
        phase += 2;
        frame++;

        wait_vsync();
    }
    _iocs_b_keyinp();

    cleanup_raster_interrupt();
}

/* 데모 4: 다중 분할 */
static void demo_multi_split(void)
{
    int frame = 0;

    print("=== Demo 4: Multi-Split Scroll ===");
    newline();
    print("4 zones with different scroll speeds");
    newline();
    print("Press any key to continue...");
    newline();

    _iocs_crtmod(0);
    _iocs_g_clr_on();

    setup_palette();
    draw_test_pattern();

    effect_mode = 4;
    setup_raster_interrupt();

    while (!_iocs_b_keysns()) {
        scroll_offset = frame;
        frame++;

        wait_vsync();
    }
    _iocs_b_keyinp();

    cleanup_raster_interrupt();
}

/* 데모 5: 워터 리플 */
static void demo_water_ripple(void)
{
    int frame = 0;

    print("=== Demo 5: Water Ripple Effect ===");
    newline();
    print("Simulating water reflection");
    newline();
    print("Press any key to continue...");
    newline();

    _iocs_crtmod(0);
    _iocs_g_clr_on();

    setup_palette();

    /* 상단: 풍경, 하단: 물 반사 */
    {
        int y;
        /* 하늘 */
        for (y = 0; y < 80; y++) {
            gvram_hline(0, 511, y, 3);  /* 파랑 */
        }
        /* 언덕 */
        for (y = 80; y < 128; y++) {
            gvram_hline(0, 511, y, 2);  /* 초록 */
        }
        /* 물 (반사) */
        for (y = 128; y < 256; y++) {
            int src_y = 255 - y;        /* 뒤집어서 반사 */
            int color;
            if (src_y < 80) color = 3;
            else color = 2;
            gvram_hline(0, 511, y, color);
        }
    }

    effect_mode = 2;
    setup_raster_interrupt();

    while (!_iocs_b_keysns()) {
        int i;
        /* 상단 (0-127)은 스크롤 없음, 하단 (128-255)만 물결 */
        for (i = 0; i < 128; i++) {
            scroll_table[i] = 0;
        }
        for (i = 128; i < VISIBLE_HEIGHT; i++) {
            int dist = i - 128;
            int amp = dist / 8;         /* 멀리 갈수록 진폭 증가 */
            int freq = 16 - dist / 16;  /* 가까울수록 고주파 */
            if (freq < 4) freq = 4;
            int angle = (dist * freq + frame * 8) & 0xFF;
            scroll_table[i] = (get_sin(angle) * amp) >> 7;
        }
        frame++;

        wait_vsync();
    }
    _iocs_b_keyinp();

    cleanup_raster_interrupt();
}

/* 데모 6: 래스터 카운터 모니터 */
static void demo_raster_monitor(void)
{
    int prev_count;

    print("=== Demo 6: Raster Counter Monitor ===");
    newline();
    print("Showing raster interrupt statistics");
    newline();
    newline();

    _iocs_crtmod(0);
    _iocs_g_clr_on();

    setup_palette();
    gvram_clear(0);

    effect_mode = 0;    /* 효과 없음, 카운트만 */
    raster_count = 0;
    setup_raster_interrupt();

    prev_count = raster_count;

    while (!_iocs_b_keysns()) {
        int cur_count = raster_count;
        int interrupts_per_frame = cur_count - prev_count;

        _dos_c_locate(0, 5);
        print("Raster interrupts:");
        newline();
        print("  Total count: ");
        print_num(cur_count);
        print("        ");
        newline();
        print("  Per frame: ");
        print_num(interrupts_per_frame);
        print("    ");
        newline();
        print("  Current line: ");
        print_num(raster_line);
        print("    ");
        newline();

        prev_count = cur_count;

        wait_vsync();
    }
    _iocs_b_keyinp();

    cleanup_raster_interrupt();
}

/* 데모 7: 인터랙티브 물결 */
static void demo_interactive_wave(void)
{
    int amplitude = 16;
    int frequency = 8;
    int speed = 4;
    int frame = 0;
    int key, scan;

    print("=== Demo 7: Interactive Wave ===");
    newline();
    print("Controls:");
    newline();
    print("  UP/DOWN: Amplitude");
    newline();
    print("  LEFT/RIGHT: Frequency");
    newline();
    print("  Z/X: Speed");
    newline();
    print("  ESC: Exit");
    newline();

    _iocs_crtmod(0);
    _iocs_g_clr_on();

    setup_palette();
    draw_checker_pattern();

    effect_mode = 2;
    setup_raster_interrupt();

    while (1) {
        /* 입력 처리 */
        if (_iocs_b_keysns()) {
            key = _iocs_b_keyinp();
            scan = (key >> 8) & 0xFF;

            if (scan == 0x01) break;    /* ESC */
            if (scan == 0x3C && amplitude < 64) amplitude += 4;  /* UP */
            if (scan == 0x3E && amplitude > 0) amplitude -= 4;   /* DOWN */
            if (scan == 0x3D && frequency < 32) frequency += 2;  /* RIGHT */
            if (scan == 0x3B && frequency > 2) frequency -= 2;   /* LEFT */
            if (scan == 0x2A && speed < 16) speed++;             /* Z */
            if (scan == 0x2B && speed > 1) speed--;              /* X */
        }

        /* 물결 계산 */
        wave_phase = frame * speed;
        calc_wave_scroll(amplitude, frequency, wave_phase);
        frame++;

        /* 상태 표시 */
        _dos_c_locate(0, 20);
        print("Amp: ");
        print_num(amplitude);
        print("  Freq: ");
        print_num(frequency);
        print("  Speed: ");
        print_num(speed);
        print("    ");

        wait_vsync();
    }

    cleanup_raster_interrupt();
}

int main(void)
{
    /* 화면 클리어 */
    _dos_c_cls_al();

    print("========================================");
    newline();
    print("  X68000 Raster Interrupt Demo");
    newline();
    print("========================================");
    newline();
    newline();

    print("This demo shows raster interrupt effects:");
    newline();
    print("- Split screen scrolling");
    newline();
    print("- Wave distortion");
    newline();
    print("- Per-line palette changes");
    newline();
    print("- Water reflection");
    newline();
    newline();

    print("Press any key to start...");
    newline();
    _iocs_b_keyinp();

    /* 데모 실행 */
    _dos_c_cls_al();
    demo_split_scroll();

    _dos_c_cls_al();
    demo_wave_effect();

    _dos_c_cls_al();
    demo_raster_palette();

    _dos_c_cls_al();
    demo_multi_split();

    _dos_c_cls_al();
    demo_water_ripple();

    _dos_c_cls_al();
    demo_raster_monitor();

    _dos_c_cls_al();
    demo_interactive_wave();

    /* 종료 */
    _iocs_crtmod(16);   /* 텍스트 모드 복귀 */
    _dos_c_cls_al();
    print("Raster interrupt demo complete!");
    newline();
    print("Press any key to exit...");
    newline();
    _iocs_b_keyinp();

    return 0;
}
