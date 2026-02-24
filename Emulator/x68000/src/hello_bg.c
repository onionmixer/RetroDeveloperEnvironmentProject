/*
 * X68000 Human68k - BG (Background) Tilemap Example
 * BG 타일맵 표시 예제 (IOCS)
 *
 * X68000 BG 사양:
 * - 2개의 BG 레이어 (BG0, BG1)
 * - 64x64 타일 맵 (8x8 픽셀 타일)
 * - 타일당 16색 (16개 팔레트)
 * - 하드웨어 스크롤
 * - 스프라이트와 PCG 패턴 공유
 */
#include <x68k/iocs.h>
#include <x68k/dos.h>

/* BG 관련 상수 */
#define BG0             0
#define BG1             1
#define BG_WIDTH        64      /* 타일 맵 너비 */
#define BG_HEIGHT       64      /* 타일 맵 높이 */
#define TILE_SIZE       8       /* 타일 크기 (8x8) */

/* 화면 크기 */
#define SCREEN_WIDTH    256
#define SCREEN_HEIGHT   256

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

/* 딜레이 */
static void delay(int count)
{
    volatile int i;
    for (i = 0; i < count * 1000; i++)
        ;
}

/* 타일 패턴 정의 (8x8, 4bpp = 32바이트) */
/* 각 바이트는 2픽셀 (상위4비트=왼쪽, 하위4비트=오른쪽) */

/* 패턴 0: 빈 타일 */
static const unsigned char pattern_empty[32] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

/* 패턴 1: 채워진 블록 */
static const unsigned char pattern_solid[32] = {
    0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11,
};

/* 패턴 2: 벽돌 패턴 */
static const unsigned char pattern_brick[32] = {
    0x22, 0x22, 0x22, 0x20,
    0x22, 0x22, 0x22, 0x20,
    0x22, 0x22, 0x22, 0x20,
    0x00, 0x00, 0x00, 0x00,
    0x22, 0x20, 0x22, 0x22,
    0x22, 0x20, 0x22, 0x22,
    0x22, 0x20, 0x22, 0x22,
    0x00, 0x00, 0x00, 0x00,
};

/* 패턴 3: 잔디 */
static const unsigned char pattern_grass[32] = {
    0x03, 0x30, 0x03, 0x30,
    0x33, 0x33, 0x33, 0x33,
    0x33, 0x33, 0x33, 0x33,
    0x33, 0x33, 0x33, 0x33,
    0x33, 0x33, 0x33, 0x33,
    0x33, 0x33, 0x33, 0x33,
    0x33, 0x33, 0x33, 0x33,
    0x33, 0x33, 0x33, 0x33,
};

/* 패턴 4: 물 */
static const unsigned char pattern_water[32] = {
    0x44, 0x44, 0x44, 0x44,
    0x44, 0x44, 0x44, 0x44,
    0x45, 0x44, 0x45, 0x44,
    0x44, 0x54, 0x44, 0x54,
    0x44, 0x44, 0x44, 0x44,
    0x54, 0x44, 0x54, 0x44,
    0x44, 0x45, 0x44, 0x45,
    0x44, 0x44, 0x44, 0x44,
};

/* 패턴 5: 하늘 */
static const unsigned char pattern_sky[32] = {
    0x66, 0x66, 0x66, 0x66,
    0x66, 0x66, 0x66, 0x66,
    0x66, 0x66, 0x66, 0x66,
    0x66, 0x66, 0x66, 0x66,
    0x66, 0x66, 0x66, 0x66,
    0x66, 0x66, 0x66, 0x66,
    0x66, 0x66, 0x66, 0x66,
    0x66, 0x66, 0x66, 0x66,
};

/* 패턴 6: 구름 */
static const unsigned char pattern_cloud[32] = {
    0x66, 0x77, 0x77, 0x66,
    0x67, 0x77, 0x77, 0x76,
    0x77, 0x77, 0x77, 0x77,
    0x77, 0x77, 0x77, 0x77,
    0x77, 0x77, 0x77, 0x77,
    0x67, 0x77, 0x77, 0x76,
    0x66, 0x77, 0x77, 0x66,
    0x66, 0x66, 0x66, 0x66,
};

/* 패턴 7: 체크보드 */
static const unsigned char pattern_checker[32] = {
    0x18, 0x18, 0x18, 0x18,
    0x81, 0x81, 0x81, 0x81,
    0x18, 0x18, 0x18, 0x18,
    0x81, 0x81, 0x81, 0x81,
    0x18, 0x18, 0x18, 0x18,
    0x81, 0x81, 0x81, 0x81,
    0x18, 0x18, 0x18, 0x18,
    0x81, 0x81, 0x81, 0x81,
};

/* 패턴 정의 테이블 */
static const unsigned char *patterns[] = {
    pattern_empty,
    pattern_solid,
    pattern_brick,
    pattern_grass,
    pattern_water,
    pattern_sky,
    pattern_cloud,
    pattern_checker,
};

/* BG 시스템 초기화 */
static void bg_init(void)
{
    /* 스프라이트/BG 시스템 초기화 */
    _iocs_sp_init();

    /* 스프라이트 표시 ON (BG도 함께 활성화) */
    _iocs_sp_on();
}

/* 타일 패턴 정의 */
static void define_tile(int code, const unsigned char *pattern)
{
    /*
     * _iocs_sp_defcg(code, size, addr)
     * code: 패턴 코드 (0-255, BG와 스프라이트 공유)
     * size: 0 = 8x8, 1 = 16x16
     * addr: 패턴 데이터 주소
     */
    _iocs_sp_defcg(code, 0, pattern);
}

/* BG 제어 설정 */
static void bg_control(int page, int on, int txsel, int hrev, int vrev)
{
    /*
     * _iocs_bgctrlst(page, txsel, ctrl)
     * page: BG 페이지 (0 또는 1)
     * txsel: 텍스트 선택 (0 또는 1)
     * ctrl: 제어 값
     *   bit 0: BG 표시 ON/OFF
     *   bit 1: 수평 반전
     *   bit 2: 수직 반전
     */
    int ctrl = (on ? 1 : 0);
    ctrl |= (hrev ? 1 : 0) << 1;
    ctrl |= (vrev ? 1 : 0) << 2;
    _iocs_bgctrlst(page, txsel, ctrl);
}

/* BG 스크롤 설정 */
static void bg_scroll(int page, int x, int y)
{
    /*
     * _iocs_bgscrlst(page, x, y)
     * page: BG 페이지
     * x, y: 스크롤 위치
     */
    _iocs_bgscrlst(page, x, y);
}

/* BG 타일 설정 */
static void bg_set_tile(int page, int x, int y, int code, int palette)
{
    /*
     * _iocs_bgtextst(page, x, y, data)
     * page: BG 페이지
     * x, y: 타일 좌표 (0-63)
     * data: 타일 데이터
     *   bit 0-7: 패턴 코드
     *   bit 8-11: 팔레트 번호
     *   bit 12: 수평 반전
     *   bit 13: 수직 반전
     */
    int data = (code & 0xFF) | ((palette & 0x0F) << 8);
    _iocs_bgtextst(page, x, y, data);
}

/* BG 전체 클리어 */
static void bg_clear(int page, int code)
{
    /*
     * _iocs_bgtextcl(page, code)
     * 지정된 코드로 BG 전체를 채움
     */
    _iocs_bgtextcl(page, code);
}

/* 팔레트 설정 */
static void set_palette(int block, int index, int color)
{
    _iocs_spalet(1, block * 16 + index, color);
}

/* 기본 팔레트 설정 */
static void setup_palettes(void)
{
    /* 팔레트 블록 1: 기본 색상 */
    set_palette(1, 0, 0x0000);   /* 투명 */
    set_palette(1, 1, 0x001F);   /* 파랑 */
    set_palette(1, 2, 0x8410);   /* 회색 */
    set_palette(1, 3, 0x07E0);   /* 초록 */
    set_palette(1, 4, 0x03FF);   /* 하늘색 */
    set_palette(1, 5, 0x07FF);   /* 밝은 하늘 */
    set_palette(1, 6, 0x7C1F);   /* 하늘 */
    set_palette(1, 7, 0xFFFF);   /* 흰색 */
    set_palette(1, 8, 0xF800);   /* 빨강 */

    /* 팔레트 블록 2: 풍경용 */
    set_palette(2, 0, 0x0000);
    set_palette(2, 1, 0x8B00);   /* 갈색 (벽돌) */
    set_palette(2, 2, 0xD520);   /* 밝은 갈색 */
    set_palette(2, 3, 0x07E0);   /* 초록 */
    set_palette(2, 4, 0x001F);   /* 물 */
    set_palette(2, 5, 0x03FF);   /* 밝은 물 */
    set_palette(2, 6, 0x7C1F);   /* 하늘 */
    set_palette(2, 7, 0xFFFF);   /* 구름 */

    /* 팔레트 블록 3: 체크보드 */
    set_palette(3, 0, 0x0000);
    set_palette(3, 1, 0x0000);   /* 검정 */
    set_palette(3, 8, 0xFFFF);   /* 흰색 */
}

/* 데모 1: 기본 타일맵 표시 */
static void demo_basic_tilemap(void)
{
    int x, y;
    int i;

    print("=== Demo 1: Basic Tilemap ===");
    newline();
    print("Showing different tile patterns.");
    newline();
    print("Press any key to continue.");
    newline();

    /* 패턴 정의 */
    for (i = 0; i < 8; i++) {
        define_tile(i, patterns[i]);
    }

    /* 팔레트 설정 */
    setup_palettes();

    /* BG0 클리어 */
    bg_clear(BG0, 0);

    /* BG0 활성화 */
    bg_control(BG0, 1, 0, 0, 0);

    /* 각 패턴을 격자로 표시 */
    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++) {
            int tile = (y * 8 + x) % 8;
            /* 4x4 블록으로 표시 */
            int tx = x * 4;
            int ty = y * 4 + 2;
            int px, py;
            for (py = 0; py < 4; py++) {
                for (px = 0; px < 4; px++) {
                    bg_set_tile(BG0, tx + px, ty + py, tile, 1);
                }
            }
        }
    }

    _iocs_b_keyinp();

    bg_clear(BG0, 0);
}

/* 데모 2: 스크롤 */
static void demo_scroll(void)
{
    int scroll_x = 0, scroll_y = 0;
    int x, y;
    int joy;

    _dos_c_cls_al();
    print("=== Demo 2: BG Scrolling ===");
    newline();
    print("Use joystick to scroll. Press ESC to continue.");
    newline();

    /* 체크보드 패턴으로 전체 채우기 */
    define_tile(7, pattern_checker);

    for (y = 0; y < 64; y++) {
        for (x = 0; x < 64; x++) {
            bg_set_tile(BG0, x, y, 7, 3);
        }
    }

    bg_control(BG0, 1, 0, 0, 0);

    while (1) {
        joy = _iocs_joyget(0);

        /* 스크롤 제어 */
        if (!(joy & 0x01)) scroll_y -= 2;  /* UP */
        if (!(joy & 0x02)) scroll_y += 2;  /* DOWN */
        if (!(joy & 0x04)) scroll_x -= 2;  /* LEFT */
        if (!(joy & 0x08)) scroll_x += 2;  /* RIGHT */

        /* 래핑 (0-511) */
        scroll_x &= 0x1FF;
        scroll_y &= 0x1FF;

        bg_scroll(BG0, scroll_x, scroll_y);

        _dos_c_locate(0, 3);
        print("Scroll: (");
        print_num(scroll_x);
        print(", ");
        print_num(scroll_y);
        print(")    ");

        /* ESC 체크 */
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            if (((key >> 8) & 0xFF) == 0x01) {
                break;
            }
        }

        delay(3);
    }

    bg_clear(BG0, 0);
    bg_scroll(BG0, 0, 0);
}

/* 데모 3: 2개 레이어 */
static void demo_two_layers(void)
{
    int x, y;
    int scroll0 = 0, scroll1 = 0;
    int frame = 0;

    _dos_c_cls_al();
    print("=== Demo 3: Two BG Layers ===");
    newline();
    print("BG0 and BG1 scrolling at different speeds.");
    newline();
    print("Press any key to continue.");
    newline();

    /* 패턴 정의 */
    define_tile(3, pattern_grass);
    define_tile(5, pattern_sky);
    define_tile(6, pattern_cloud);

    /* BG1: 하늘 (뒤) */
    for (y = 0; y < 32; y++) {
        for (x = 0; x < 64; x++) {
            if ((x + y) % 8 == 0) {
                bg_set_tile(BG1, x, y, 6, 2);  /* 구름 */
            } else {
                bg_set_tile(BG1, x, y, 5, 2);  /* 하늘 */
            }
        }
    }

    /* BG0: 잔디 (앞) */
    for (y = 0; y < 64; y++) {
        for (x = 0; x < 64; x++) {
            if (y >= 24) {
                bg_set_tile(BG0, x, y, 3, 2);  /* 잔디 */
            } else {
                bg_set_tile(BG0, x, y, 0, 0);  /* 투명 */
            }
        }
    }

    /* 양쪽 BG 활성화 */
    bg_control(BG0, 1, 0, 0, 0);
    bg_control(BG1, 1, 0, 0, 0);

    while (1) {
        /* 패럴랙스 스크롤 (다른 속도) */
        scroll0 = (frame * 2) & 0x1FF;   /* BG0: 빠르게 */
        scroll1 = (frame / 2) & 0x1FF;   /* BG1: 느리게 */

        bg_scroll(BG0, scroll0, 0);
        bg_scroll(BG1, scroll1, 0);

        frame++;

        _dos_c_locate(0, 4);
        print("BG0: ");
        print_num(scroll0);
        print("  BG1: ");
        print_num(scroll1);
        print("    ");

        if (_iocs_b_keysns()) {
            _iocs_b_keyinp();
            break;
        }

        delay(2);
    }

    bg_clear(BG0, 0);
    bg_clear(BG1, 0);
    bg_scroll(BG0, 0, 0);
    bg_scroll(BG1, 0, 0);
    bg_control(BG1, 0, 0, 0, 0);
}

/* 데모 4: 맵 에디터 스타일 */
static void demo_map_editor(void)
{
    int cursor_x = 16, cursor_y = 16;
    int current_tile = 1;
    int joy;
    int btn_delay = 0;

    _dos_c_cls_al();
    print("=== Demo 4: Map Editor ===");
    newline();
    print("Move: Joystick, A: Place tile, B: Change tile");
    newline();
    print("Press ESC to continue.");
    newline();

    /* 패턴 정의 */
    define_tile(0, pattern_empty);
    define_tile(1, pattern_solid);
    define_tile(2, pattern_brick);
    define_tile(3, pattern_grass);
    define_tile(4, pattern_water);

    /* BG 클리어 */
    bg_clear(BG0, 0);
    bg_control(BG0, 1, 0, 0, 0);

    while (1) {
        joy = _iocs_joyget(0);

        if (btn_delay == 0) {
            /* 커서 이동 */
            if (!(joy & 0x01) && cursor_y > 0) {
                cursor_y--;
                btn_delay = 5;
            }
            if (!(joy & 0x02) && cursor_y < 31) {
                cursor_y++;
                btn_delay = 5;
            }
            if (!(joy & 0x04) && cursor_x > 0) {
                cursor_x--;
                btn_delay = 5;
            }
            if (!(joy & 0x08) && cursor_x < 31) {
                cursor_x++;
                btn_delay = 5;
            }

            /* 버튼 A: 타일 배치 */
            if (!(joy & 0x20)) {
                bg_set_tile(BG0, cursor_x, cursor_y, current_tile, 2);
                btn_delay = 3;
            }

            /* 버튼 B: 타일 변경 */
            if (!(joy & 0x40)) {
                current_tile = (current_tile + 1) % 5;
                btn_delay = 10;
            }
        } else {
            btn_delay--;
        }

        /* 커서 표시 (점멸) */
        static int blink = 0;
        blink = (blink + 1) % 20;
        if (blink < 10) {
            bg_set_tile(BG0, cursor_x, cursor_y, 7, 3);
        }

        /* 정보 표시 */
        _dos_c_locate(0, 4);
        print("Cursor: (");
        print_num(cursor_x);
        print(", ");
        print_num(cursor_y);
        print(") Tile: ");
        print_num(current_tile);
        print("    ");

        /* ESC 체크 */
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            if (((key >> 8) & 0xFF) == 0x01) {
                break;
            }
        }

        delay(2);
    }

    bg_clear(BG0, 0);
}

/* 데모 5: 간단한 플랫폼 맵 */
static void demo_platform_map(void)
{
    int x, y;
    int scroll_x = 0;

    /* 간단한 플랫폼 맵 데이터 */
    static const unsigned char map[8][32] = {
        {5,5,5,5,5,5,6,5,5,5,5,5,5,5,5,5,5,6,5,5,5,5,5,5,5,5,6,5,5,5,5,5},
        {5,5,5,6,5,5,5,5,5,5,6,5,5,5,5,5,5,5,5,5,6,5,5,5,5,5,5,5,5,6,5,5},
        {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5},
        {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5},
        {5,5,5,5,5,2,2,2,5,5,5,5,5,5,2,2,2,2,5,5,5,5,5,2,2,5,5,5,5,5,5,5},
        {5,5,5,5,5,5,5,5,5,5,5,5,2,2,5,5,5,5,5,5,5,2,2,5,5,5,5,5,5,5,5,5},
        {2,2,2,2,5,5,5,5,5,5,2,2,5,5,5,5,5,5,5,2,2,5,5,5,5,5,5,2,2,2,2,2},
        {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3},
    };

    _dos_c_cls_al();
    print("=== Demo 5: Platform Map ===");
    newline();
    print("Auto-scrolling platform map.");
    newline();
    print("Press any key to continue.");
    newline();

    /* 패턴 정의 */
    define_tile(0, pattern_empty);
    define_tile(2, pattern_brick);
    define_tile(3, pattern_grass);
    define_tile(5, pattern_sky);
    define_tile(6, pattern_cloud);

    /* 맵 데이터를 BG에 복사 */
    for (y = 0; y < 8; y++) {
        for (x = 0; x < 32; x++) {
            int tile = map[y][x];
            /* 세로로 확대 (각 타일을 3칸씩) */
            int ty;
            for (ty = 0; ty < 3; ty++) {
                bg_set_tile(BG0, x, y * 3 + ty + 4, tile, 2);
            }
        }
    }

    bg_control(BG0, 1, 0, 0, 0);

    while (1) {
        /* 자동 스크롤 */
        scroll_x = (scroll_x + 1) & 0xFF;
        bg_scroll(BG0, scroll_x, 0);

        _dos_c_locate(0, 4);
        print("Scroll: ");
        print_num(scroll_x);
        print("    ");

        if (_iocs_b_keysns()) {
            _iocs_b_keyinp();
            break;
        }

        delay(3);
    }

    bg_clear(BG0, 0);
    bg_scroll(BG0, 0, 0);
}

int main(void)
{
    /* 화면 모드 설정 (스프라이트/BG용) */
    _iocs_crtmod(10);  /* 256x256 16색 + 스프라이트/BG */

    /* 화면 클리어 */
    _dos_c_cls_al();

    print("========================================");
    newline();
    print("  X68000 BG Tilemap Demo");
    newline();
    print("========================================");
    newline();
    newline();

    print("This demo shows BG (Background) tilemaps.");
    newline();
    print("Press any key to start...");
    newline();
    _iocs_b_keyinp();

    /* BG 초기화 */
    bg_init();

    /* 데모 실행 */
    _dos_c_cls_al();
    demo_basic_tilemap();

    demo_scroll();

    demo_two_layers();

    demo_map_editor();

    demo_platform_map();

    /* 종료 */
    _iocs_sp_off();
    _iocs_crtmod(16);  /* 텍스트 모드 복귀 */

    _dos_c_cls_al();
    print("BG tilemap demo complete!");
    newline();
    print("Press any key to exit...");
    newline();
    _iocs_b_keyinp();

    return 0;
}
