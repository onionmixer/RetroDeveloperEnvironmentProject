/*
 * hello_crtc.c - X68000 CRTC Screen Mode Example
 *
 * This example demonstrates CRTC operations:
 * - Screen resolution switching
 * - Color mode changes
 * - CRTC register access
 * - Display timing
 * - Video memory configuration
 *
 * Build: m68k-xelf-gcc -O2 -o build/hello_crtc.x src/hello_crtc.c
 *
 * X68000 CRTC: HD63450 compatible
 * Address: 0xE80000
 */

#include <x68k/iocs.h>
#include <x68k/dos.h>
#include <string.h>

/*
 * CRTC Register Addresses
 */
#define CRTC_BASE       0xE80000

/* CRTC Registers */
#define CRTC_R00        (*(volatile unsigned short *)(CRTC_BASE + 0x00))  /* H Total */
#define CRTC_R01        (*(volatile unsigned short *)(CRTC_BASE + 0x02))  /* H Sync End */
#define CRTC_R02        (*(volatile unsigned short *)(CRTC_BASE + 0x04))  /* H Disp Start */
#define CRTC_R03        (*(volatile unsigned short *)(CRTC_BASE + 0x06))  /* H Disp End */
#define CRTC_R04        (*(volatile unsigned short *)(CRTC_BASE + 0x08))  /* V Total */
#define CRTC_R05        (*(volatile unsigned short *)(CRTC_BASE + 0x0A))  /* V Sync End */
#define CRTC_R06        (*(volatile unsigned short *)(CRTC_BASE + 0x0C))  /* V Disp Start */
#define CRTC_R07        (*(volatile unsigned short *)(CRTC_BASE + 0x0E))  /* V Disp End */
#define CRTC_R08        (*(volatile unsigned short *)(CRTC_BASE + 0x10))  /* Adjust */
#define CRTC_R09        (*(volatile unsigned short *)(CRTC_BASE + 0x12))  /* Raster Number */
#define CRTC_R10        (*(volatile unsigned short *)(CRTC_BASE + 0x14))  /* Text X Scroll */
#define CRTC_R11        (*(volatile unsigned short *)(CRTC_BASE + 0x16))  /* Text Y Scroll */
#define CRTC_R12        (*(volatile unsigned short *)(CRTC_BASE + 0x18))  /* GFX0 X Scroll */
#define CRTC_R13        (*(volatile unsigned short *)(CRTC_BASE + 0x1A))  /* GFX0 Y Scroll */
#define CRTC_R20        (*(volatile unsigned short *)(CRTC_BASE + 0x28))  /* Control */
#define CRTC_R23        (*(volatile unsigned short *)(CRTC_BASE + 0x2E))  /* Mode */

/* Video Controller Registers */
#define VIDCON_BASE     0xE82000
#define VIDCON_R0       (*(volatile unsigned short *)(VIDCON_BASE + 0x400))  /* Screen mode */
#define VIDCON_R1       (*(volatile unsigned short *)(VIDCON_BASE + 0x402))  /* Priority */

/* Graphics VRAM Base */
#define GVRAM_BASE      0xC00000

/* Saved original mode */
static int original_crtmod = -1;

/* Text output */
static int cursor_x = 0;
static int cursor_y = 0;

static void putchar_at(char c)
{
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= 31) cursor_y = 0;
        return;
    }
    _iocs_b_putc(c);
    cursor_x++;
    if (cursor_x >= 96) {
        cursor_x = 0;
        cursor_y++;
    }
}

static void print(const char *str)
{
    while (*str) {
        putchar_at(*str++);
    }
}

static void print_num(int num)
{
    char buf[12];
    int i = 0;
    int neg = 0;

    if (num < 0) {
        neg = 1;
        num = -num;
    }

    if (num == 0) {
        buf[i++] = '0';
    } else {
        while (num > 0) {
            buf[i++] = '0' + (num % 10);
            num /= 10;
        }
    }

    if (neg) buf[i++] = '-';

    while (i > 0) {
        putchar_at(buf[--i]);
    }
}

static void print_hex_word(unsigned short val)
{
    const char hex[] = "0123456789ABCDEF";
    putchar_at(hex[(val >> 12) & 0x0F]);
    putchar_at(hex[(val >> 8) & 0x0F]);
    putchar_at(hex[(val >> 4) & 0x0F]);
    putchar_at(hex[val & 0x0F]);
}

static int wait_key(void)
{
    while (_iocs_b_keysns() == 0) { }
    return _iocs_b_keyinp() & 0xFF;
}

static void clear_screen(void)
{
    _iocs_b_clr_st();
    cursor_x = 0;
    cursor_y = 0;
    _iocs_b_locate(0, 0);
}

/* Wait for VSYNC */
static void wait_vsync(void)
{
    while ((_iocs_b_bpeek((void *)0xE88001) & 0x10) == 0) { }
    while ((_iocs_b_bpeek((void *)0xE88001) & 0x10) != 0) { }
}

/* Save current screen mode */
static void save_screen_mode(void)
{
    if (original_crtmod < 0) {
        original_crtmod = _iocs_crtmod(-1);
    }
}

/* Restore original screen mode */
static void restore_screen_mode(void)
{
    if (original_crtmod >= 0) {
        _iocs_crtmod(original_crtmod);
        _iocs_g_clr_on();
        _iocs_b_clr_st();
    }
}

/* Draw horizontal line in GVRAM (16 color mode) */
static void draw_hline_16(int y, int x1, int x2, int color)
{
    volatile unsigned short *gvram = (volatile unsigned short *)GVRAM_BASE;
    int x;
    int addr;

    for (x = x1; x <= x2; x++) {
        addr = y * 512 + x;
        gvram[addr] = color;
    }
}

/* Draw rectangle outline in GVRAM (16 color mode) */
static void draw_rect_16(int x1, int y1, int x2, int y2, int color)
{
    volatile unsigned short *gvram = (volatile unsigned short *)GVRAM_BASE;
    int x, y;

    /* Top and bottom */
    for (x = x1; x <= x2; x++) {
        gvram[y1 * 512 + x] = color;
        gvram[y2 * 512 + x] = color;
    }
    /* Left and right */
    for (y = y1; y <= y2; y++) {
        gvram[y * 512 + x1] = color;
        gvram[y * 512 + x2] = color;
    }
}

/* Fill rectangle in GVRAM */
static void fill_rect_16(int x1, int y1, int x2, int y2, int color)
{
    volatile unsigned short *gvram = (volatile unsigned short *)GVRAM_BASE;
    int x, y;

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            gvram[y * 512 + x] = color;
        }
    }
}

/*
 * Get screen mode description
 */
static const char* get_mode_name(int mode)
{
    switch (mode & 0x0F) {
        case 0: return "256x256 16col";
        case 1: return "256x256 256col";
        case 2: return "256x256 (undef)";
        case 3: return "256x256 65536col";
        case 4: return "512x256 16col";
        case 5: return "512x256 256col";
        case 6: return "512x256 (undef)";
        case 7: return "512x256 65536col";
        case 8: return "512x512 16col";
        case 9: return "512x512 256col";
        case 10: return "512x512 (undef)";
        case 11: return "512x512 65536col";
        case 12: return "768x512 16col (PRO)";
        case 13: return "768x512 16col";
        case 14: return "768x512 16col";
        case 15: return "768x512 16col";
        default: return "Unknown";
    }
}

/*
 * Demo 1: Current Mode Information
 */
static void demo_current_mode(void)
{
    int mode;

    clear_screen();
    print("=== Demo 1: Current Screen Mode ===\n\n");

    mode = _iocs_crtmod(-1);

    print("Current CRTMOD: ");
    print_num(mode);
    print("\n");
    print("Mode Name: ");
    print(get_mode_name(mode));
    print("\n\n");

    /* Read CRTC registers */
    print("CRTC Register Values:\n");
    print("---------------------\n");

    print("R00 (H Total):     ");
    print_num(CRTC_R00);
    print("\n");

    print("R01 (H Sync End):  ");
    print_num(CRTC_R01);
    print("\n");

    print("R02 (H Disp Start):");
    print_num(CRTC_R02);
    print("\n");

    print("R03 (H Disp End):  ");
    print_num(CRTC_R03);
    print("\n");

    print("R04 (V Total):     ");
    print_num(CRTC_R04);
    print("\n");

    print("R05 (V Sync End):  ");
    print_num(CRTC_R05);
    print("\n");

    print("R06 (V Disp Start):");
    print_num(CRTC_R06);
    print("\n");

    print("R07 (V Disp End):  ");
    print_num(CRTC_R07);
    print("\n\n");

    /* Calculate display size */
    print("Calculated Display:\n");
    print("  H Pixels: ");
    print_num((CRTC_R03 - CRTC_R02) * 8);
    print("\n");
    print("  V Lines:  ");
    print_num(CRTC_R07 - CRTC_R06);
    print("\n\n");

    /* Control register */
    print("R20 (Control): 0x");
    print_hex_word(CRTC_R20);
    print("\n");

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 2: Screen Mode List
 */
static void demo_mode_list(void)
{
    clear_screen();
    print("=== Demo 2: Available Screen Modes ===\n\n");

    print("Mode Resolution  Colors  GFX Pages  Notes\n");
    print("---- ----------  ------  ---------  -----\n");

    print("  0  256x256     16      4 pages    Low res\n");
    print("  1  256x256     256     2 pages    \n");
    print("  3  256x256     65536   1 page     True color\n");
    print("  4  512x256     16      4 pages    Wide\n");
    print("  5  512x256     256     2 pages    \n");
    print("  7  512x256     65536   1 page     True color\n");
    print("  8  512x512     16      4 pages    Standard\n");
    print("  9  512x512     256     2 pages    \n");
    print(" 11  512x512     65536   1 page     True color\n");
    print(" 12  768x512     16      2 pages    Hi-res (PRO)\n");

    print("\n");
    print("High byte options (OR with mode):\n");
    print("  +0x10: Interlace ON\n");
    print("  +0x20: 31kHz horizontal sync\n");

    print("\n");
    print("Note: 768x512 requires X68000 PRO/EXPERT\n");
    print("      or compatible monitor\n");

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 3: Switch to 512x512 16 colors
 */
static void demo_mode_512x512_16(void)
{
    int y;

    clear_screen();
    print("=== Demo 3: 512x512 16 Colors ===\n\n");
    print("Switching to mode 8 (512x512 16col)...\n");
    print("Press any key to switch (ESC to skip)");

    if (wait_key() == 0x1B) {
        return;
    }

    save_screen_mode();

    /* Switch to 512x512 16 colors */
    _iocs_crtmod(8);
    _iocs_g_clr_on();

    /* Set palette */
    _iocs_gpalet(0, 0x0000);   /* Black */
    _iocs_gpalet(1, 0xF800);   /* Red */
    _iocs_gpalet(2, 0x07C0);   /* Green */
    _iocs_gpalet(3, 0x001F);   /* Blue */
    _iocs_gpalet(4, 0xFFFF);   /* White */
    _iocs_gpalet(5, 0xF81F);   /* Magenta */
    _iocs_gpalet(6, 0xFFE0);   /* Yellow */
    _iocs_gpalet(7, 0x07FF);   /* Cyan */

    /* Draw colored bands */
    for (y = 0; y < 512; y++) {
        int color = (y / 64) % 8;
        draw_hline_16(y, 0, 511, color);
    }

    /* Draw border rectangles */
    draw_rect_16(0, 0, 511, 511, 4);
    draw_rect_16(10, 10, 501, 501, 4);

    /* Draw center info box */
    fill_rect_16(150, 200, 360, 300, 0);
    draw_rect_16(150, 200, 360, 300, 4);

    wait_key();

    /* Restore mode */
    restore_screen_mode();
}

/*
 * Demo 4: Switch to 512x512 256 colors
 */
static void demo_mode_512x512_256(void)
{
    int x, y;
    volatile unsigned short *gvram = (volatile unsigned short *)GVRAM_BASE;

    clear_screen();
    print("=== Demo 4: 512x512 256 Colors ===\n\n");
    print("Switching to mode 9 (512x512 256col)...\n");
    print("Press any key to switch (ESC to skip)");

    if (wait_key() == 0x1B) {
        return;
    }

    save_screen_mode();

    /* Switch to 512x512 256 colors */
    _iocs_crtmod(9);
    _iocs_g_clr_on();

    /* Set up gradient palette */
    for (x = 0; x < 256; x++) {
        /* Create rainbow gradient */
        int r = 0, g = 0, b = 0;
        if (x < 43) {
            r = 31; g = x * 31 / 43; b = 0;
        } else if (x < 86) {
            r = 31 - (x - 43) * 31 / 43; g = 31; b = 0;
        } else if (x < 128) {
            r = 0; g = 31; b = (x - 86) * 31 / 42;
        } else if (x < 171) {
            r = 0; g = 31 - (x - 128) * 31 / 43; b = 31;
        } else if (x < 214) {
            r = (x - 171) * 31 / 43; g = 0; b = 31;
        } else {
            r = 31; g = 0; b = 31 - (x - 214) * 31 / 42;
        }
        _iocs_gpalet(x, (r << 11) | (g << 5) | b);
    }

    /* Draw 256-color pattern */
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            /* In 256 color mode, pack 2 pixels per word */
            int addr = y * 512 + x;
            gvram[addr] = (unsigned short)((y << 8) | (x & 0xFF));
        }
    }

    wait_key();

    restore_screen_mode();
}

/*
 * Demo 5: Switch to 256x256 65536 colors
 */
static void demo_mode_256x256_true(void)
{
    int x, y;
    volatile unsigned short *gvram = (volatile unsigned short *)GVRAM_BASE;

    clear_screen();
    print("=== Demo 5: 256x256 65536 Colors ===\n\n");
    print("Switching to mode 3 (256x256 65536col)...\n");
    print("This is true color mode!\n");
    print("Press any key to switch (ESC to skip)");

    if (wait_key() == 0x1B) {
        return;
    }

    save_screen_mode();

    /* Switch to 256x256 65536 colors */
    _iocs_crtmod(3);
    _iocs_g_clr_on();

    /* Draw true color gradient directly */
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            /* Calculate RGB565 color */
            int r = x >> 3;        /* 0-31 */
            int g = y >> 2;        /* 0-63 */
            int b = ((x + y) / 2) >> 3;  /* 0-31 */
            unsigned short color = (r << 11) | (g << 5) | b;

            /* In 65536 color mode, write directly to GVRAM */
            gvram[y * 256 + x] = color;
        }
    }

    wait_vsync();
    wait_key();

    restore_screen_mode();
}

/*
 * Demo 6: CRTC Register Dump
 */
static void demo_crtc_registers(void)
{
    clear_screen();
    print("=== Demo 6: CRTC Register Dump ===\n\n");

    print("Address   Name            Value\n");
    print("-------   ----            -----\n");

    print("E80000    R00 H Total     0x");
    print_hex_word(CRTC_R00);
    print("\n");

    print("E80002    R01 H Sync      0x");
    print_hex_word(CRTC_R01);
    print("\n");

    print("E80004    R02 H DispS     0x");
    print_hex_word(CRTC_R02);
    print("\n");

    print("E80006    R03 H DispE     0x");
    print_hex_word(CRTC_R03);
    print("\n");

    print("E80008    R04 V Total     0x");
    print_hex_word(CRTC_R04);
    print("\n");

    print("E8000A    R05 V Sync      0x");
    print_hex_word(CRTC_R05);
    print("\n");

    print("E8000C    R06 V DispS     0x");
    print_hex_word(CRTC_R06);
    print("\n");

    print("E8000E    R07 V DispE     0x");
    print_hex_word(CRTC_R07);
    print("\n");

    print("E80010    R08 Adjust      0x");
    print_hex_word(CRTC_R08);
    print("\n");

    print("E80012    R09 Raster      0x");
    print_hex_word(CRTC_R09);
    print("\n");

    print("E80028    R20 Control     0x");
    print_hex_word(CRTC_R20);
    print("\n");

    print("E8002E    R23 Mode        0x");
    print_hex_word(CRTC_R23);
    print("\n\n");

    /* Video controller */
    print("Video Controller:\n");
    print("E82400    Screen Mode     0x");
    print_hex_word(VIDCON_R0);
    print("\n");

    print("E82402    Priority        0x");
    print_hex_word(VIDCON_R1);
    print("\n");

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 7: Scroll Test
 */
static void demo_scroll_test(void)
{
    int i;
    int scroll_x = 0, scroll_y = 0;
    int dx = 2, dy = 1;
    int x, y;

    clear_screen();
    print("=== Demo 7: Hardware Scroll Test ===\n\n");
    print("Uses CRTC scroll registers.\n");
    print("Press any key to start (ESC to skip)");

    if (wait_key() == 0x1B) {
        return;
    }

    save_screen_mode();

    /* Set up 512x512 mode */
    _iocs_crtmod(8);
    _iocs_g_clr_on();

    /* Set palette */
    _iocs_gpalet(0, 0x0000);
    _iocs_gpalet(1, 0xF800);
    _iocs_gpalet(2, 0x07C0);
    _iocs_gpalet(3, 0x001F);
    _iocs_gpalet(4, 0xFFFF);

    /* Draw a grid pattern */
    for (y = 0; y < 512; y += 32) {
        draw_hline_16(y, 0, 511, 1);
    }
    for (x = 0; x < 512; x += 32) {
        for (y = 0; y < 512; y++) {
            volatile unsigned short *gvram = (volatile unsigned short *)GVRAM_BASE;
            gvram[y * 512 + x] = 2;
        }
    }

    /* Draw corner markers */
    fill_rect_16(0, 0, 40, 40, 3);
    fill_rect_16(471, 0, 511, 40, 3);
    fill_rect_16(0, 471, 40, 511, 3);
    fill_rect_16(471, 471, 511, 511, 3);

    /* Draw center cross */
    draw_hline_16(256, 200, 312, 4);
    for (y = 200; y < 312; y++) {
        volatile unsigned short *gvram = (volatile unsigned short *)GVRAM_BASE;
        gvram[y * 512 + 256] = 4;
    }

    /* Animate scroll */
    for (i = 0; i < 500; i++) {
        wait_vsync();

        /* Update scroll position */
        scroll_x += dx;
        scroll_y += dy;

        if (scroll_x > 150 || scroll_x < 0) dx = -dx;
        if (scroll_y > 150 || scroll_y < 0) dy = -dy;

        /* Set scroll registers */
        CRTC_R12 = scroll_x;  /* GFX0 X scroll */
        CRTC_R13 = scroll_y;  /* GFX0 Y scroll */

        /* Check for key press */
        if (_iocs_b_keysns()) {
            _iocs_b_keyinp();
            break;
        }
    }

    /* Reset scroll */
    CRTC_R12 = 0;
    CRTC_R13 = 0;

    restore_screen_mode();
}

/*
 * Demo 8: CRTC Information
 */
static void demo_crtc_info(void)
{
    clear_screen();
    print("=== Demo 8: X68000 CRTC Information ===\n\n");

    print("X68000 Video System:\n");
    print("--------------------\n\n");

    print("CRTC: Custom (HD63450 compatible)\n");
    print("Base Address: 0xE80000\n\n");

    print("Display Capabilities:\n");
    print("  Resolutions: 256x256 to 768x512\n");
    print("  Colors: 16, 256, or 65536\n");
    print("  GFX Pages: 1-4 (mode dependent)\n");
    print("  Text: 96x32 or 64x32 chars\n\n");

    print("Video Memory:\n");
    print("  GVRAM: 0xC00000 (512KB)\n");
    print("  TVRAM: 0xE00000 (128KB)\n");
    print("  Sprite: 0xEB8000\n\n");

    print("Sync Modes:\n");
    print("  15kHz: Standard TV/monitor\n");
    print("  31kHz: VGA compatible\n\n");

    print("IOCS Functions:\n");
    print("  _iocs_crtmod()   - Set/get mode\n");
    print("  _iocs_g_clr_on() - Clear & enable\n");
    print("  _iocs_gpalet()   - Set palette\n");

    print("\nPress any key...");
    wait_key();
}

/*
 * Main function
 */
int main(int argc, char *argv[])
{
    int demo = 0;
    int running = 1;

    (void)argc;
    (void)argv;

    /* Save initial mode */
    save_screen_mode();

    _iocs_b_clr_st();
    _iocs_os_curon();

    while (running) {
        clear_screen();
        print("=== X68000 CRTC Screen Mode Demo ===\n\n");

        print("This demo shows screen mode switching.\n");
        print("Some modes may not work on all monitors.\n\n");

        print("Demo programs:\n");
        print("  1. Current Mode Information\n");
        print("  2. Available Screen Modes\n");
        print("  3. 512x512 16 Colors\n");
        print("  4. 512x512 256 Colors\n");
        print("  5. 256x256 65536 Colors\n");
        print("  6. CRTC Register Dump\n");
        print("  7. Hardware Scroll Test\n");
        print("  8. CRTC Information\n\n");

        print("  0. Exit\n\n");

        print("Select demo (0-8): ");

        demo = wait_key() - '0';

        switch (demo) {
            case 1: demo_current_mode(); break;
            case 2: demo_mode_list(); break;
            case 3: demo_mode_512x512_16(); break;
            case 4: demo_mode_512x512_256(); break;
            case 5: demo_mode_256x256_true(); break;
            case 6: demo_crtc_registers(); break;
            case 7: demo_scroll_test(); break;
            case 8: demo_crtc_info(); break;
            case 0: running = 0; break;
            default: break;
        }
    }

    /* Ensure we restore original mode */
    restore_screen_mode();
    clear_screen();
    print("CRTC demo finished.\n");

    return 0;
}
