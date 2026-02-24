/*
 * X68000 Human68k - Sprite Example
 * 스프라이트 표시 예제 (IOCS)
 *
 * X68000 스프라이트 사양:
 * - 최대 128개 스프라이트 (16x16 픽셀)
 * - 스프라이트당 16색 (16개 팔레트)
 * - 하드웨어 우선순위 제어
 * - 수평/수직 반전
 * - BG (Background) 2면
 */
#include <x68k/iocs.h>
#include <x68k/dos.h>

/* 스프라이트 관련 상수 */
#define SP_MAX          128     /* 최대 스프라이트 수 */
#define SP_SIZE         16      /* 스프라이트 크기 (16x16) */
#define SP_PATTERN_SIZE 128     /* 패턴 데이터 크기 (바이트) */

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

/* 스프라이트 패턴 정의 (16x16, 4bpp = 128바이트) */
/* 각 바이트는 2픽셀 (상위4비트=왼쪽, 하위4비트=오른쪽) */

/* 패턴 1: 사각형 */
static const unsigned char pattern_box[128] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* 라인 0 */
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,  /* 라인 1 */
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,  /* 라인 2 */
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,  /* 라인 3 */
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,  /* 라인 4 */
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,  /* 라인 5 */
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,  /* 라인 6 */
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,  /* 라인 7 */
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,  /* 라인 8 */
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,  /* 라인 9 */
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,  /* 라인 10 */
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,  /* 라인 11 */
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,  /* 라인 12 */
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,  /* 라인 13 */
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,  /* 라인 14 */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* 라인 15 */
};

/* 패턴 2: 원형 (대략적인 원) */
static const unsigned char pattern_circle[128] = {
    0x00, 0x00, 0x22, 0x22, 0x22, 0x22, 0x00, 0x00,  /* 라인 0 */
    0x00, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x00,  /* 라인 1 */
    0x22, 0x22, 0x00, 0x00, 0x00, 0x00, 0x22, 0x22,  /* 라인 2 */
    0x22, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x22,  /* 라인 3 */
    0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22,  /* 라인 4 */
    0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22,  /* 라인 5 */
    0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22,  /* 라인 6 */
    0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22,  /* 라인 7 */
    0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22,  /* 라인 8 */
    0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22,  /* 라인 9 */
    0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22,  /* 라인 10 */
    0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22,  /* 라인 11 */
    0x22, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x22,  /* 라인 12 */
    0x22, 0x22, 0x00, 0x00, 0x00, 0x00, 0x22, 0x22,  /* 라인 13 */
    0x00, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x00,  /* 라인 14 */
    0x00, 0x00, 0x22, 0x22, 0x22, 0x22, 0x00, 0x00,  /* 라인 15 */
};

/* 패턴 3: 화살표 (오른쪽 방향) */
static const unsigned char pattern_arrow[128] = {
    0x00, 0x00, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00,  /* 라인 0 */
    0x00, 0x00, 0x00, 0x00, 0x33, 0x30, 0x00, 0x00,  /* 라인 1 */
    0x00, 0x00, 0x00, 0x00, 0x33, 0x33, 0x00, 0x00,  /* 라인 2 */
    0x00, 0x00, 0x00, 0x00, 0x33, 0x33, 0x30, 0x00,  /* 라인 3 */
    0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x00,  /* 라인 4 */
    0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x30,  /* 라인 5 */
    0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,  /* 라인 6 */
    0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,  /* 라인 7 */
    0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,  /* 라인 8 */
    0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,  /* 라인 9 */
    0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x30,  /* 라인 10 */
    0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x00,  /* 라인 11 */
    0x00, 0x00, 0x00, 0x00, 0x33, 0x33, 0x30, 0x00,  /* 라인 12 */
    0x00, 0x00, 0x00, 0x00, 0x33, 0x33, 0x00, 0x00,  /* 라인 13 */
    0x00, 0x00, 0x00, 0x00, 0x33, 0x30, 0x00, 0x00,  /* 라인 14 */
    0x00, 0x00, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00,  /* 라인 15 */
};

/* 패턴 4: 별 모양 */
static const unsigned char pattern_star[128] = {
    0x00, 0x00, 0x00, 0x44, 0x44, 0x00, 0x00, 0x00,  /* 라인 0 */
    0x00, 0x00, 0x00, 0x44, 0x44, 0x00, 0x00, 0x00,  /* 라인 1 */
    0x00, 0x00, 0x00, 0x44, 0x44, 0x00, 0x00, 0x00,  /* 라인 2 */
    0x00, 0x00, 0x44, 0x44, 0x44, 0x44, 0x00, 0x00,  /* 라인 3 */
    0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,  /* 라인 4 */
    0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,  /* 라인 5 */
    0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x00,  /* 라인 6 */
    0x00, 0x00, 0x44, 0x44, 0x44, 0x44, 0x00, 0x00,  /* 라인 7 */
    0x00, 0x00, 0x44, 0x44, 0x44, 0x44, 0x00, 0x00,  /* 라인 8 */
    0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x00,  /* 라인 9 */
    0x00, 0x44, 0x44, 0x00, 0x00, 0x44, 0x44, 0x00,  /* 라인 10 */
    0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00,  /* 라인 11 */
    0x44, 0x44, 0x00, 0x00, 0x00, 0x00, 0x44, 0x44,  /* 라인 12 */
    0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44,  /* 라인 13 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 라인 14 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 라인 15 */
};

/* 패턴 5: 캐릭터 (간단한 사람 모양) */
static const unsigned char pattern_player[128] = {
    0x00, 0x00, 0x05, 0x55, 0x55, 0x50, 0x00, 0x00,  /* 라인 0: 머리 */
    0x00, 0x00, 0x55, 0x55, 0x55, 0x55, 0x00, 0x00,  /* 라인 1 */
    0x00, 0x00, 0x55, 0x05, 0x50, 0x55, 0x00, 0x00,  /* 라인 2: 눈 */
    0x00, 0x00, 0x55, 0x55, 0x55, 0x55, 0x00, 0x00,  /* 라인 3 */
    0x00, 0x00, 0x05, 0x55, 0x55, 0x50, 0x00, 0x00,  /* 라인 4 */
    0x00, 0x00, 0x00, 0x66, 0x66, 0x00, 0x00, 0x00,  /* 라인 5: 목 */
    0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00,  /* 라인 6: 몸통 */
    0x06, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x60,  /* 라인 7 */
    0x66, 0x60, 0x06, 0x66, 0x66, 0x60, 0x06, 0x66,  /* 라인 8: 팔 */
    0x66, 0x00, 0x06, 0x66, 0x66, 0x60, 0x00, 0x66,  /* 라인 9 */
    0x00, 0x00, 0x06, 0x66, 0x66, 0x60, 0x00, 0x00,  /* 라인 10 */
    0x00, 0x00, 0x06, 0x66, 0x66, 0x60, 0x00, 0x00,  /* 라인 11 */
    0x00, 0x00, 0x06, 0x60, 0x06, 0x60, 0x00, 0x00,  /* 라인 12: 다리 */
    0x00, 0x00, 0x06, 0x60, 0x06, 0x60, 0x00, 0x00,  /* 라인 13 */
    0x00, 0x00, 0x66, 0x60, 0x06, 0x66, 0x00, 0x00,  /* 라인 14 */
    0x00, 0x00, 0x66, 0x00, 0x00, 0x66, 0x00, 0x00,  /* 라인 15: 발 */
};

/* 스프라이트 초기화 */
static void sprite_init(void)
{
    /* 스프라이트/BG 화면 초기화 */
    _iocs_sp_init();

    /* 스프라이트 표시 ON */
    _iocs_sp_on();
}

/* 스프라이트 패턴 정의 */
static void define_pattern(int code, const unsigned char *pattern)
{
    /*
     * _iocs_sp_defcg(code, size, addr)
     * code: 패턴 코드 (0-255)
     * size: 1 = 16x16
     * addr: 패턴 데이터 주소
     */
    _iocs_sp_defcg(code, 1, pattern);
}

/* 스프라이트 설정 및 표시 */
static void set_sprite(int num, int x, int y, int code, int palette, int priority)
{
    /*
     * _iocs_sp_regst(num, x, y, code, attr, pri)
     * num: 스프라이트 번호 (0-127)
     * x, y: 좌표
     * code: 패턴 코드 (0-255)
     * attr: 속성 (팔레트 + HV반전)
     * pri: 우선순위 (0-3)
     *
     * attr 구조:
     * bit 0-3: 팔레트 (0-15)
     * bit 4: 수평 반전
     * bit 5: 수직 반전
     */
    int attr = (palette & 0x0F);
    _iocs_sp_regst(num, x, y, code, attr, priority);
}

/* 스프라이트 숨기기 */
static void hide_sprite(int num)
{
    /* 화면 밖으로 이동 */
    _iocs_sp_regst(num, 0, 256, 0, 0, 0);
}

/* 팔레트 설정 */
static void set_palette(int block, int index, int color)
{
    /*
     * _iocs_spalet(mode, block*16+index, color)
     * mode: 0=읽기, 1=쓰기
     * color: 0xGRBI (G=green, R=red, B=blue, I=intensity)
     *        X68000 16색 팔레트 형식
     */
    _iocs_spalet(1, block * 16 + index, color);
}

/* 기본 팔레트 설정 */
static void setup_default_palette(void)
{
    /* 팔레트 블록 1 (흰색 계열) */
    set_palette(1, 0, 0x0000);   /* 투명 */
    set_palette(1, 15, 0xFFFF);  /* 흰색 */

    /* 팔레트 블록 2 (빨강) */
    set_palette(2, 0, 0x0000);
    set_palette(2, 2, 0xF800);   /* 빨강 */

    /* 팔레트 블록 3 (초록) */
    set_palette(3, 0, 0x0000);
    set_palette(3, 3, 0x07E0);   /* 초록 */

    /* 팔레트 블록 4 (노랑) */
    set_palette(4, 0, 0x0000);
    set_palette(4, 4, 0xFFE0);   /* 노랑 */

    /* 팔레트 블록 5 (플레이어 색상) */
    set_palette(5, 0, 0x0000);
    set_palette(5, 5, 0xFFC0);   /* 피부색 */
    set_palette(5, 6, 0x001F);   /* 파랑 (옷) */
}

/* 데모 1: 기본 스프라이트 표시 */
static void demo_basic_sprite(void)
{
    print("=== Demo 1: Basic Sprites ===");
    newline();
    print("Showing different sprite patterns.");
    newline();
    print("Press any key to continue.");
    newline();

    /* 패턴 정의 */
    define_pattern(0, pattern_box);
    define_pattern(1, pattern_circle);
    define_pattern(2, pattern_arrow);
    define_pattern(3, pattern_star);
    define_pattern(4, pattern_player);

    /* 팔레트 설정 */
    setup_default_palette();

    /* 스프라이트 배치 */
    set_sprite(0, 50, 100, 0, 1, 3);    /* 사각형 */
    set_sprite(1, 90, 100, 1, 2, 3);    /* 원 */
    set_sprite(2, 130, 100, 2, 3, 3);   /* 화살표 */
    set_sprite(3, 170, 100, 3, 4, 3);   /* 별 */
    set_sprite(4, 210, 100, 4, 5, 3);   /* 캐릭터 */

    _iocs_b_keyinp();

    /* 스프라이트 숨기기 */
    hide_sprite(0);
    hide_sprite(1);
    hide_sprite(2);
    hide_sprite(3);
    hide_sprite(4);
}

/* 데모 2: 스프라이트 이동 */
static void demo_sprite_move(void)
{
    int x = 128, y = 128;
    int joy;

    _dos_c_cls_al();
    print("=== Demo 2: Sprite Movement ===");
    newline();
    print("Use joystick or arrow keys to move sprite.");
    newline();
    print("Press ESC to continue.");
    newline();

    define_pattern(0, pattern_player);
    set_sprite(0, x, y, 0, 5, 3);

    while (1) {
        /* 조이스틱 입력 */
        joy = _iocs_joyget(0);

        if (!(joy & 0x01) && y > 16) y -= 2;      /* UP */
        if (!(joy & 0x02) && y < 240) y += 2;     /* DOWN */
        if (!(joy & 0x04) && x > 16) x -= 2;      /* LEFT */
        if (!(joy & 0x08) && x < 240) x += 2;     /* RIGHT */

        /* 스프라이트 위치 업데이트 */
        set_sprite(0, x, y, 0, 5, 3);

        /* 좌표 표시 */
        _dos_c_locate(0, 4);
        print("Position: (");
        print_num(x);
        print(", ");
        print_num(y);
        print(")    ");

        /* ESC 체크 */
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            if (((key >> 8) & 0xFF) == 0x01) {
                break;
            }
        }

        delay(5);
    }

    hide_sprite(0);
}

/* 데모 3: 여러 스프라이트 애니메이션 */
static void demo_animation(void)
{
    int i, frame = 0;
    int positions[8][2];  /* 8개 스프라이트 위치 */
    int velocities[8][2]; /* 속도 */

    _dos_c_cls_al();
    print("=== Demo 3: Multiple Sprite Animation ===");
    newline();
    print("Watch sprites bouncing around.");
    newline();
    print("Press any key to continue.");
    newline();

    /* 패턴 정의 */
    define_pattern(0, pattern_circle);
    define_pattern(1, pattern_star);

    /* 초기 위치와 속도 설정 */
    for (i = 0; i < 8; i++) {
        positions[i][0] = 50 + (i * 25);
        positions[i][1] = 80 + (i * 15) % 100;
        velocities[i][0] = (i % 2 == 0) ? 2 : -2;
        velocities[i][1] = (i % 3 == 0) ? 2 : -1;
    }

    while (1) {
        /* 스프라이트 업데이트 */
        for (i = 0; i < 8; i++) {
            /* 위치 업데이트 */
            positions[i][0] += velocities[i][0];
            positions[i][1] += velocities[i][1];

            /* 벽 충돌 체크 */
            if (positions[i][0] <= 16 || positions[i][0] >= 240) {
                velocities[i][0] = -velocities[i][0];
            }
            if (positions[i][1] <= 16 || positions[i][1] >= 240) {
                velocities[i][1] = -velocities[i][1];
            }

            /* 스프라이트 표시 */
            set_sprite(i, positions[i][0], positions[i][1],
                      i % 2, (i % 4) + 1, 3);
        }

        frame++;
        _dos_c_locate(0, 4);
        print("Frame: ");
        print_num(frame);
        print("    ");

        /* 키 입력 체크 */
        if (_iocs_b_keysns()) {
            _iocs_b_keyinp();
            break;
        }

        delay(3);
    }

    /* 스프라이트 숨기기 */
    for (i = 0; i < 8; i++) {
        hide_sprite(i);
    }
}

/* 데모 4: 스프라이트 우선순위 */
static void demo_priority(void)
{
    int i, offset = 0;

    _dos_c_cls_al();
    print("=== Demo 4: Sprite Priority ===");
    newline();
    print("Overlapping sprites with different priorities.");
    newline();
    print("Press any key to continue.");
    newline();

    define_pattern(0, pattern_box);
    define_pattern(1, pattern_circle);

    /* 겹치는 스프라이트 표시 (다른 우선순위) */
    while (1) {
        /* 우선순위별로 스프라이트 배치 */
        for (i = 0; i < 4; i++) {
            int x = 100 + i * 20 + offset;
            int y = 120;
            set_sprite(i, x, y, i % 2, i + 1, 3 - i);  /* 우선순위 3,2,1,0 */
        }

        offset = (offset + 1) % 80;

        _dos_c_locate(0, 4);
        print("Priority order: SP0(3) > SP1(2) > SP2(1) > SP3(0)");

        if (_iocs_b_keysns()) {
            _iocs_b_keyinp();
            break;
        }

        delay(5);
    }

    for (i = 0; i < 4; i++) {
        hide_sprite(i);
    }
}

/* 데모 5: 스프라이트 반전 */
static void demo_flip(void)
{
    int frame = 0;

    _dos_c_cls_al();
    print("=== Demo 5: Sprite Flip ===");
    newline();
    print("Horizontal and vertical flip demo.");
    newline();
    print("Press any key to continue.");
    newline();

    define_pattern(0, pattern_arrow);

    while (1) {
        int flip = (frame / 30) % 4;

        /*
         * attr 구조: bit4=H반전, bit5=V반전, bit0-3=팔레트
         * flip 0: 정상 (0x03)
         * flip 1: H반전 (0x13)
         * flip 2: V반전 (0x23)
         * flip 3: H+V반전 (0x33)
         */
        int base_attr = 3;  /* 팔레트 3 */

        /* 4개 방향 표시 */
        _iocs_sp_regst(0, 80, 120, 0, base_attr | ((flip & 1) ? 0x10 : 0) | ((flip & 2) ? 0x20 : 0), 3);
        _iocs_sp_regst(1, 120, 120, 0, base_attr | 0x10, 3);  /* H반전 */
        _iocs_sp_regst(2, 160, 120, 0, base_attr | 0x20, 3);  /* V반전 */
        _iocs_sp_regst(3, 200, 120, 0, base_attr | 0x30, 3);  /* H+V반전 */

        _dos_c_locate(0, 4);
        print("Flip mode: ");
        switch (flip) {
            case 0: print("Normal      "); break;
            case 1: print("H-Flip      "); break;
            case 2: print("V-Flip      "); break;
            case 3: print("H+V Flip    "); break;
        }

        frame++;

        if (_iocs_b_keysns()) {
            _iocs_b_keyinp();
            break;
        }

        delay(3);
    }

    hide_sprite(0);
    hide_sprite(1);
    hide_sprite(2);
    hide_sprite(3);
}

int main(void)
{
    /* 화면 모드 설정 (스프라이트용) */
    _iocs_crtmod(10);  /* 256x256 16색 + 스프라이트 */

    /* 화면 클리어 */
    _dos_c_cls_al();

    print("========================================");
    newline();
    print("  X68000 Sprite Demo");
    newline();
    print("========================================");
    newline();
    newline();

    print("This demo shows hardware sprites.");
    newline();
    print("Press any key to start...");
    newline();
    _iocs_b_keyinp();

    /* 스프라이트 초기화 */
    sprite_init();

    /* 데모 실행 */
    _dos_c_cls_al();
    demo_basic_sprite();

    demo_sprite_move();

    demo_animation();

    demo_priority();

    demo_flip();

    /* 종료 */
    _iocs_sp_off();
    _iocs_crtmod(16);  /* 텍스트 모드 복귀 */

    _dos_c_cls_al();
    print("Sprite demo complete!");
    newline();
    print("Press any key to exit...");
    newline();
    _iocs_b_keyinp();

    return 0;
}
