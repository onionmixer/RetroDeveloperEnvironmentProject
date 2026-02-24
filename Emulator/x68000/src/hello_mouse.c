/*
 * X68000 Human68k - Mouse Input Example
 * 마우스 입력 처리 예제 (IOCS)
 *
 * X68000 마우스:
 * - 전용 마우스 포트 (MSCTRL 커넥터)
 * - 2버튼 마우스 (좌/우)
 * - IOCS를 통한 좌표 및 버튼 상태 읽기
 */
#include <x68k/iocs.h>
#include <x68k/dos.h>

/* 마우스 버튼 비트 마스크 */
#define MOUSE_LEFT      0x01    /* 왼쪽 버튼 */
#define MOUSE_RIGHT     0x02    /* 오른쪽 버튼 */

/* 화면 크기 */
#define SCREEN_WIDTH    768
#define SCREEN_HEIGHT   512

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

/* 10진수 출력 */
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

/* 16진수 출력 */
static void print_hex(int value, int digits)
{
    char hex[] = "0123456789ABCDEF";
    int i;
    for (i = (digits - 1) * 4; i >= 0; i -= 4) {
        print_char(hex[(value >> i) & 0xF]);
    }
}

/* 마우스 데이터 구조 파싱 */
/* _iocs_ms_getdt() 반환값:
 * 비트 31-24: 버튼 상태 (bit0=왼쪽, bit1=오른쪽, 누르면 1)
 * 비트 23-16: Y 이동량 (signed 8-bit)
 * 비트 15-8:  X 이동량 (signed 8-bit)
 * 비트 7-0:   상태
 */
static int get_mouse_buttons(int data)
{
    return (data >> 24) & 0xFF;
}

static int get_mouse_dx(int data)
{
    signed char dx = (data >> 8) & 0xFF;
    return dx;
}

static int get_mouse_dy(int data)
{
    signed char dy = (data >> 16) & 0xFF;
    return dy;
}

/* 데모 1: 기본 마우스 정보 표시 */
static void demo_mouse_info(void)
{
    int data;
    int buttons, dx, dy;
    int abs_x = SCREEN_WIDTH / 2;
    int abs_y = SCREEN_HEIGHT / 2;
    int count = 0;

    print("=== Demo 1: Mouse Information ===");
    newline();
    print("Move mouse and click buttons. Press ESC to continue.");
    newline();
    newline();

    while (1) {
        /* 마우스 데이터 읽기 */
        data = _iocs_ms_getdt();
        buttons = get_mouse_buttons(data);
        dx = get_mouse_dx(data);
        dy = get_mouse_dy(data);

        /* 절대 좌표 계산 */
        abs_x += dx;
        abs_y += dy;

        /* 화면 범위 제한 */
        if (abs_x < 0) abs_x = 0;
        if (abs_x >= SCREEN_WIDTH) abs_x = SCREEN_WIDTH - 1;
        if (abs_y < 0) abs_y = 0;
        if (abs_y >= SCREEN_HEIGHT) abs_y = SCREEN_HEIGHT - 1;

        /* 상태 표시 (움직임이 있거나 버튼이 눌렸을 때만) */
        if (dx != 0 || dy != 0 || buttons != 0) {
            _dos_c_locate(0, 4);
            print("Raw: 0x");
            print_hex(data, 8);
            print("        ");

            _dos_c_locate(0, 5);
            print("Position: (");
            print_num(abs_x);
            print(", ");
            print_num(abs_y);
            print(")      ");

            _dos_c_locate(0, 6);
            print("Delta: dx=");
            print_num(dx);
            print(", dy=");
            print_num(dy);
            print("      ");

            _dos_c_locate(0, 7);
            print("Buttons: ");
            if (buttons & MOUSE_LEFT) {
                print("[LEFT] ");
            } else {
                print("       ");
            }
            if (buttons & MOUSE_RIGHT) {
                print("[RIGHT]");
            } else {
                print("       ");
            }

            _dos_c_locate(0, 8);
            print("Count: ");
            print_num(count++);
            print("      ");
        }

        /* ESC 키 체크 */
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            if (((key >> 8) & 0xFF) == 0x01) {  /* ESC */
                break;
            }
        }

        /* 약간의 딜레이 */
        {
            volatile int i;
            for (i = 0; i < 1000; i++)
                ;
        }
    }
    newline();
}

/* 데모 2: 마우스 커서 표시 (텍스트 모드) */
static void demo_text_cursor(void)
{
    int data;
    int buttons, dx, dy;
    int cursor_x = 40, cursor_y = 12;
    int prev_x = cursor_x, prev_y = cursor_y;

    _dos_c_cls_al();

    print("=== Demo 2: Text Mode Cursor ===");
    newline();
    print("Move mouse to control '*'. Click LEFT to place 'O'.");
    newline();
    print("Click RIGHT to erase. Press ESC to continue.");
    newline();

    /* 초기 커서 표시 */
    _dos_c_locate(cursor_x, cursor_y);
    print_char('*');

    while (1) {
        data = _iocs_ms_getdt();
        buttons = get_mouse_buttons(data);
        dx = get_mouse_dx(data);
        dy = get_mouse_dy(data);

        /* 커서 이동 (텍스트 좌표로 변환, 감도 조절) */
        if (dx != 0 || dy != 0) {
            /* 이전 커서 지우기 */
            _dos_c_locate(prev_x, prev_y);
            print_char(' ');

            cursor_x += dx / 4;
            cursor_y += dy / 4;

            /* 범위 제한 (텍스트 화면) */
            if (cursor_x < 0) cursor_x = 0;
            if (cursor_x > 95) cursor_x = 95;
            if (cursor_y < 4) cursor_y = 4;
            if (cursor_y > 30) cursor_y = 30;

            prev_x = cursor_x;
            prev_y = cursor_y;

            /* 새 커서 표시 */
            _dos_c_locate(cursor_x, cursor_y);
            print_char('*');
        }

        /* 왼쪽 버튼: 'O' 찍기 */
        if (buttons & MOUSE_LEFT) {
            _dos_c_locate(cursor_x, cursor_y);
            print_char('O');
            prev_x = -1;  /* 커서 다시 그리기 */
        }

        /* 오른쪽 버튼: 지우기 */
        if (buttons & MOUSE_RIGHT) {
            _dos_c_locate(cursor_x, cursor_y);
            print_char(' ');
        }

        /* ESC 키 체크 */
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            if (((key >> 8) & 0xFF) == 0x01) {
                break;
            }
        }

        /* 딜레이 */
        {
            volatile int i;
            for (i = 0; i < 500; i++)
                ;
        }
    }
}

/* 데모 3: 버튼 이벤트 감지 */
static void demo_button_events(void)
{
    int data;
    int buttons;
    int prev_buttons = 0;
    int left_clicks = 0, right_clicks = 0;
    int left_down = 0, right_down = 0;

    _dos_c_cls_al();

    print("=== Demo 3: Button Events ===");
    newline();
    print("Click mouse buttons. Press ESC to continue.");
    newline();
    newline();

    while (1) {
        data = _iocs_ms_getdt();
        buttons = get_mouse_buttons(data);

        /* 버튼 상태 변화 감지 */
        /* 왼쪽 버튼 눌림 */
        if ((buttons & MOUSE_LEFT) && !(prev_buttons & MOUSE_LEFT)) {
            left_down = 1;
            _dos_c_locate(0, 5);
            print("LEFT button DOWN!   ");
        }
        /* 왼쪽 버튼 뗌 */
        if (!(buttons & MOUSE_LEFT) && (prev_buttons & MOUSE_LEFT)) {
            if (left_down) {
                left_clicks++;
                left_down = 0;
            }
            _dos_c_locate(0, 5);
            print("LEFT button UP!     ");
        }

        /* 오른쪽 버튼 눌림 */
        if ((buttons & MOUSE_RIGHT) && !(prev_buttons & MOUSE_RIGHT)) {
            right_down = 1;
            _dos_c_locate(0, 6);
            print("RIGHT button DOWN!  ");
        }
        /* 오른쪽 버튼 뗌 */
        if (!(buttons & MOUSE_RIGHT) && (prev_buttons & MOUSE_RIGHT)) {
            if (right_down) {
                right_clicks++;
                right_down = 0;
            }
            _dos_c_locate(0, 6);
            print("RIGHT button UP!    ");
        }

        prev_buttons = buttons;

        /* 클릭 카운터 표시 */
        _dos_c_locate(0, 8);
        print("Left clicks: ");
        print_num(left_clicks);
        print("    ");

        _dos_c_locate(0, 9);
        print("Right clicks: ");
        print_num(right_clicks);
        print("    ");

        /* ESC 키 체크 */
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            if (((key >> 8) & 0xFF) == 0x01) {
                break;
            }
        }

        /* 딜레이 */
        {
            volatile int i;
            for (i = 0; i < 500; i++)
                ;
        }
    }
}

/* 데모 4: 드래그 감지 */
static void demo_drag(void)
{
    int data;
    int buttons, dx, dy;
    int dragging = 0;
    int drag_start_x = 0, drag_start_y = 0;
    int current_x = SCREEN_WIDTH / 2;
    int current_y = SCREEN_HEIGHT / 2;
    int total_drag_x = 0, total_drag_y = 0;

    _dos_c_cls_al();

    print("=== Demo 4: Drag Detection ===");
    newline();
    print("Hold LEFT button and move mouse to drag.");
    newline();
    print("Press ESC to exit.");
    newline();
    newline();

    while (1) {
        data = _iocs_ms_getdt();
        buttons = get_mouse_buttons(data);
        dx = get_mouse_dx(data);
        dy = get_mouse_dy(data);

        current_x += dx;
        current_y += dy;

        /* 드래그 시작 */
        if ((buttons & MOUSE_LEFT) && !dragging) {
            dragging = 1;
            drag_start_x = current_x;
            drag_start_y = current_y;
            total_drag_x = 0;
            total_drag_y = 0;

            _dos_c_locate(0, 5);
            print("Drag started at (");
            print_num(drag_start_x);
            print(", ");
            print_num(drag_start_y);
            print(")       ");
        }

        /* 드래그 중 */
        if (dragging && (buttons & MOUSE_LEFT)) {
            total_drag_x += dx;
            total_drag_y += dy;

            _dos_c_locate(0, 6);
            print("Dragging... distance: (");
            print_num(total_drag_x);
            print(", ");
            print_num(total_drag_y);
            print(")       ");

            _dos_c_locate(0, 7);
            print("Current: (");
            print_num(current_x);
            print(", ");
            print_num(current_y);
            print(")       ");
        }

        /* 드래그 종료 */
        if (dragging && !(buttons & MOUSE_LEFT)) {
            dragging = 0;

            _dos_c_locate(0, 8);
            print("Drag ended! Total: (");
            print_num(total_drag_x);
            print(", ");
            print_num(total_drag_y);
            print(")       ");
        }

        /* ESC 키 체크 */
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            if (((key >> 8) & 0xFF) == 0x01) {
                break;
            }
        }

        /* 딜레이 */
        {
            volatile int i;
            for (i = 0; i < 500; i++)
                ;
        }
    }
}

int main(void)
{
    /* 화면 클리어 */
    _dos_c_cls_al();

    print("========================================");
    newline();
    print("  X68000 Mouse Input Demo");
    newline();
    print("========================================");
    newline();
    newline();

    print("This demo shows mouse input handling.");
    newline();
    print("Make sure a mouse is connected.");
    newline();
    newline();
    print("Press any key to start...");
    newline();
    _iocs_b_keyinp();

    /* 데모 실행 */
    _dos_c_cls_al();
    demo_mouse_info();

    demo_text_cursor();

    demo_button_events();

    demo_drag();

    /* 종료 */
    _dos_c_cls_al();
    print("Mouse demo complete!");
    newline();
    print("Press any key to exit...");
    newline();
    _iocs_b_keyinp();

    return 0;
}
