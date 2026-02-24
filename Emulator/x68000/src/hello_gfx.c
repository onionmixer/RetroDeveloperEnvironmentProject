/*
 * X68000 Human68k - Graphics Example
 * IOCS 그래픽 함수를 사용한 기본 그래픽 출력 예제
 *
 * X68000 그래픽 사양:
 * - 512x512 또는 256x256 해상도
 * - 16색 / 256색 / 65536색 모드
 * - 4개의 그래픽 페이지
 */
#include <x68k/iocs.h>

/* 색상 정의 (16색 팔레트 인덱스) */
#define COLOR_BLACK     0
#define COLOR_BLUE      1
#define COLOR_RED       2
#define COLOR_MAGENTA   3
#define COLOR_GREEN     4
#define COLOR_CYAN      5
#define COLOR_YELLOW    6
#define COLOR_WHITE     7

/* 화면 해상도 */
#define SCREEN_WIDTH    512
#define SCREEN_HEIGHT   512

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
    l.x1 = x1;
    l.y1 = y1;
    l.x2 = x2;
    l.y2 = y2;
    l.color = color;
    l.linestyle = 0xFFFF;  /* 실선 */
    _iocs_line(&l);
}

/* 사각형 그리기 (테두리만) */
static void draw_box(int x1, int y1, int x2, int y2, int color)
{
    struct iocs_boxptr b;
    b.x1 = x1;
    b.y1 = y1;
    b.x2 = x2;
    b.y2 = y2;
    b.color = color;
    b.linestyle = 0xFFFF;  /* 실선 */
    _iocs_box(&b);
}

/* 사각형 채우기 */
static void fill_box(int x1, int y1, int x2, int y2, int color)
{
    struct iocs_fillptr f;
    f.x1 = x1;
    f.y1 = y1;
    f.x2 = x2;
    f.y2 = y2;
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
    c.start = 0;      /* 시작 각도 (0 = 전체 원) */
    c.end = 0;        /* 끝 각도 (0 = 전체 원) */
    c.ratio = 256;    /* 종횡비 (256 = 1:1) */
    _iocs_circle(&c);
}

/* 타원 그리기 */
static void draw_ellipse(int x, int y, int radius, int ratio, int color)
{
    struct iocs_circleptr c;
    c.x = x;
    c.y = y;
    c.radius = radius;
    c.color = color;
    c.start = 0;
    c.end = 0;
    c.ratio = ratio;  /* 종횡비 (256 = 1:1, 128 = 1:2, 512 = 2:1) */
    _iocs_circle(&c);
}

/* 그래픽 화면에 텍스트 출력 */
static void draw_text(int x, int y, const char *str, int color)
{
    struct iocs_symbolptr s;
    s.x1 = x;
    s.y1 = y;
    s.string_address = (const unsigned char *)str;
    s.mag_x = 1;      /* 가로 배율 */
    s.mag_y = 1;      /* 세로 배율 */
    s.color = color;
    s.font_type = 0;  /* 폰트 타입 (0=8x16) */
    s.angle = 0;      /* 회전 각도 (0=0도) */
    _iocs_symbol(&s);
}

/* 점선으로 선 그리기 */
static void draw_dashed_line(int x1, int y1, int x2, int y2, int color)
{
    struct iocs_lineptr l;
    l.x1 = x1;
    l.y1 = y1;
    l.x2 = x2;
    l.y2 = y2;
    l.color = color;
    l.linestyle = 0xF0F0;  /* 점선 패턴 */
    _iocs_line(&l);
}

int main(void)
{
    int i;

    /* 그래픽 모드 설정 (512x512, 16색) */
    _iocs_crtmod(12);  /* 모드 12: 512x512 16색 */

    /* 그래픽 화면 클리어 */
    _iocs_g_clr_on();

    /* 그래픽 페이지 설정 */
    _iocs_apage(0);    /* 액세스 페이지 0 */
    _iocs_vpage(0);    /* 표시 페이지 0 */

    /* === 데모 1: 텍스트 표시 === */
    draw_text(150, 20, "X68000 Graphics Demo", COLOR_WHITE);
    draw_text(180, 45, "IOCS GVRAM Functions", COLOR_CYAN);

    /* === 데모 2: 선 그리기 === */
    draw_text(20, 80, "Lines:", COLOR_WHITE);
    for (i = 0; i < 8; i++) {
        draw_line(100 + i * 20, 80, 100 + i * 20, 130, i);
    }
    /* 대각선 */
    draw_line(280, 80, 350, 130, COLOR_YELLOW);
    draw_dashed_line(360, 80, 430, 130, COLOR_MAGENTA);

    /* === 데모 3: 사각형 === */
    draw_text(20, 150, "Boxes:", COLOR_WHITE);
    draw_box(100, 150, 160, 200, COLOR_RED);
    draw_box(170, 155, 225, 195, COLOR_GREEN);
    fill_box(240, 150, 300, 200, COLOR_BLUE);
    fill_box(310, 155, 360, 195, COLOR_CYAN);

    /* === 데모 4: 원과 타원 === */
    draw_text(20, 220, "Circles:", COLOR_WHITE);
    draw_circle(130, 270, 40, COLOR_WHITE);
    draw_circle(220, 270, 30, COLOR_RED);
    draw_circle(220, 270, 20, COLOR_YELLOW);
    draw_circle(220, 270, 10, COLOR_GREEN);

    /* 타원 */
    draw_ellipse(320, 270, 40, 128, COLOR_MAGENTA);   /* 세로로 긴 타원 */
    draw_ellipse(420, 270, 40, 384, COLOR_CYAN);      /* 가로로 긴 타원 */

    /* === 데모 5: 점 패턴 === */
    draw_text(20, 330, "Points:", COLOR_WHITE);
    for (i = 0; i < 50; i++) {
        int x = 100 + (i * 7) % 150;
        int y = 340 + (i * 11) % 50;
        draw_point(x, y, i % 8);
    }

    /* === 데모 6: 복합 도형 === */
    draw_text(20, 410, "Composite:", COLOR_WHITE);
    /* 집 모양 */
    fill_box(100, 450, 180, 490, COLOR_RED);       /* 몸체 */
    draw_line(100, 450, 140, 420, COLOR_YELLOW);   /* 지붕 왼쪽 */
    draw_line(140, 420, 180, 450, COLOR_YELLOW);   /* 지붕 오른쪽 */
    fill_box(130, 465, 150, 490, COLOR_BLUE);      /* 문 */

    /* 나무 모양 */
    fill_box(240, 470, 260, 490, COLOR_RED);       /* 줄기 */
    draw_circle(250, 450, 30, COLOR_GREEN);        /* 잎 */

    /* 별 모양 (선으로) */
    draw_line(350, 420, 370, 490, COLOR_YELLOW);
    draw_line(320, 450, 400, 450, COLOR_YELLOW);
    draw_line(330, 420, 390, 480, COLOR_YELLOW);
    draw_line(330, 480, 390, 420, COLOR_YELLOW);

    /* 안내 메시지 */
    draw_text(150, 500, "Press any key to exit...", COLOR_WHITE);

    /* 키 입력 대기 */
    _iocs_b_keyinp();

    /* 텍스트 모드로 복귀 */
    _iocs_crtmod(16);  /* 모드 16: 768x512 텍스트 */

    return 0;
}
