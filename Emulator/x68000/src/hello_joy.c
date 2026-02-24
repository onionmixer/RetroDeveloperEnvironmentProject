/*
 * X68000 Human68k - Joystick Input Example
 * 조이스틱 입력 처리 예제 (IOCS)
 *
 * X68000 조이스틱:
 * - 2개의 조이스틱 포트 (ATARI 호환)
 * - 8방향 + 2버튼 (A, B)
 * - 일부 패드는 추가 버튼 지원 (CYBER STICK 등)
 */
#include <x68k/iocs.h>
#include <x68k/dos.h>

/* 조이스틱 포트 번호 */
#define JOY_PORT1       0
#define JOY_PORT2       1

/* 조이스틱 방향 비트 (Active LOW: 0=눌림, 1=안눌림) */
#define JOY_UP          0x01
#define JOY_DOWN        0x02
#define JOY_LEFT        0x04
#define JOY_RIGHT       0x08
#define JOY_BTN_A       0x20    /* 버튼 A (트리거 1) */
#define JOY_BTN_B       0x40    /* 버튼 B (트리거 2) */

/* 조이스틱 상태 체크 매크로 (Active LOW이므로 반전) */
#define JOY_PRESSED(state, btn)  (!((state) & (btn)))

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

/* 조이스틱 상태 읽기 */
static int read_joystick(int port)
{
    return _iocs_joyget(port);
}

/* 방향을 문자로 표시 */
static void print_direction(int state)
{
    int up = JOY_PRESSED(state, JOY_UP);
    int down = JOY_PRESSED(state, JOY_DOWN);
    int left = JOY_PRESSED(state, JOY_LEFT);
    int right = JOY_PRESSED(state, JOY_RIGHT);

    /* 8방향 + 중립 표시 */
    if (up && left) {
        print("NW");
    } else if (up && right) {
        print("NE");
    } else if (down && left) {
        print("SW");
    } else if (down && right) {
        print("SE");
    } else if (up) {
        print("N ");
    } else if (down) {
        print("S ");
    } else if (left) {
        print("W ");
    } else if (right) {
        print("E ");
    } else {
        print("- ");
    }
}

/* ASCII 아트로 조이스틱 상태 표시 */
static void draw_joystick_ascii(int x, int y, int state, int port_num)
{
    int up = JOY_PRESSED(state, JOY_UP);
    int down = JOY_PRESSED(state, JOY_DOWN);
    int left = JOY_PRESSED(state, JOY_LEFT);
    int right = JOY_PRESSED(state, JOY_RIGHT);
    int btn_a = JOY_PRESSED(state, JOY_BTN_A);
    int btn_b = JOY_PRESSED(state, JOY_BTN_B);

    /* 포트 번호 */
    _dos_c_locate(x, y);
    print("JOY ");
    print_num(port_num + 1);

    /* 방향 표시 (3x3 그리드) */
    _dos_c_locate(x + 1, y + 1);
    print_char(up && left ? '*' : ' ');
    print_char(up ? '^' : ' ');
    print_char(up && right ? '*' : ' ');

    _dos_c_locate(x + 1, y + 2);
    print_char(left ? '<' : ' ');
    print_char('o');
    print_char(right ? '>' : ' ');

    _dos_c_locate(x + 1, y + 3);
    print_char(down && left ? '*' : ' ');
    print_char(down ? 'v' : ' ');
    print_char(down && right ? '*' : ' ');

    /* 버튼 표시 */
    _dos_c_locate(x + 6, y + 1);
    print("A:");
    print_char(btn_a ? '@' : 'O');

    _dos_c_locate(x + 6, y + 2);
    print("B:");
    print_char(btn_b ? '@' : 'O');

    /* Raw 값 */
    _dos_c_locate(x, y + 4);
    print("Raw:0x");
    print_hex(state, 2);
}

/* 데모 1: 기본 조이스틱 상태 표시 */
static void demo_joystick_info(void)
{
    int joy1, joy2;
    int prev_joy1 = -1, prev_joy2 = -1;

    print("=== Demo 1: Joystick Status ===");
    newline();
    print("Move joysticks and press buttons. Press ESC to continue.");
    newline();
    newline();

    while (1) {
        joy1 = read_joystick(JOY_PORT1);
        joy2 = read_joystick(JOY_PORT2);

        /* 상태가 변경되었을 때만 업데이트 */
        if (joy1 != prev_joy1 || joy2 != prev_joy2) {
            draw_joystick_ascii(5, 5, joy1, 0);
            draw_joystick_ascii(25, 5, joy2, 1);

            prev_joy1 = joy1;
            prev_joy2 = joy2;
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

/* 데모 2: 캐릭터 이동 */
static void demo_character_move(void)
{
    int joy;
    int x = 20, y = 10;
    int prev_x = x, prev_y = y;

    _dos_c_cls_al();

    print("=== Demo 2: Character Movement (JOY1) ===");
    newline();
    print("Use joystick to move '@'. Press BTN-A to place '*'.");
    newline();
    print("Press ESC to continue.");
    newline();

    /* 초기 위치에 캐릭터 표시 */
    _dos_c_locate(x, y);
    print_char('@');

    while (1) {
        joy = read_joystick(JOY_PORT1);

        /* 방향 이동 */
        if (JOY_PRESSED(joy, JOY_UP) && y > 4) {
            y--;
        }
        if (JOY_PRESSED(joy, JOY_DOWN) && y < 28) {
            y++;
        }
        if (JOY_PRESSED(joy, JOY_LEFT) && x > 0) {
            x--;
        }
        if (JOY_PRESSED(joy, JOY_RIGHT) && x < 78) {
            x++;
        }

        /* 위치가 변경되면 업데이트 */
        if (x != prev_x || y != prev_y) {
            /* 이전 위치 지우기 */
            _dos_c_locate(prev_x, prev_y);
            print_char(' ');

            /* 새 위치에 캐릭터 */
            _dos_c_locate(x, y);
            print_char('@');

            prev_x = x;
            prev_y = y;

            /* 좌표 표시 */
            _dos_c_locate(50, 1);
            print("Pos: (");
            print_num(x);
            print(", ");
            print_num(y);
            print(")   ");
        }

        /* 버튼 A: '*' 찍기 */
        if (JOY_PRESSED(joy, JOY_BTN_A)) {
            _dos_c_locate(x, y);
            print_char('*');
            prev_x = -1;  /* 캐릭터 다시 그리기 */
        }

        /* 버튼 B: 지우기 */
        if (JOY_PRESSED(joy, JOY_BTN_B)) {
            _dos_c_locate(x, y);
            print_char(' ');
        }

        /* ESC 키 체크 */
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            if (((key >> 8) & 0xFF) == 0x01) {
                break;
            }
        }

        /* 딜레이 (이동 속도 조절) */
        {
            volatile int i;
            for (i = 0; i < 5000; i++)
                ;
        }
    }
}

/* 데모 3: 버튼 콤보 감지 */
static void demo_button_combo(void)
{
    int joy;
    int combo_count = 0;
    int ab_combo = 0;

    _dos_c_cls_al();

    print("=== Demo 3: Button Combo Detection ===");
    newline();
    print("Press A+B together to trigger combo!");
    newline();
    print("Press ESC to continue.");
    newline();
    newline();

    while (1) {
        joy = read_joystick(JOY_PORT1);

        int btn_a = JOY_PRESSED(joy, JOY_BTN_A);
        int btn_b = JOY_PRESSED(joy, JOY_BTN_B);

        /* 버튼 상태 표시 */
        _dos_c_locate(0, 5);
        print("Button A: ");
        print(btn_a ? "[PRESSED]" : "[       ]");

        _dos_c_locate(0, 6);
        print("Button B: ");
        print(btn_b ? "[PRESSED]" : "[       ]");

        /* A+B 동시 입력 감지 */
        if (btn_a && btn_b && !ab_combo) {
            ab_combo = 1;
            combo_count++;

            _dos_c_locate(0, 8);
            print("*** COMBO! A+B ***");

            _dos_c_locate(0, 9);
            print("Combo count: ");
            print_num(combo_count);
            print("   ");
        }

        if (!btn_a && !btn_b) {
            ab_combo = 0;
        }

        /* 방향 + 버튼 콤보 */
        _dos_c_locate(0, 11);
        print("Direction: ");
        print_direction(joy);

        if (JOY_PRESSED(joy, JOY_DOWN) && btn_a) {
            _dos_c_locate(0, 12);
            print("Special: DOWN + A (Crouch Attack!)  ");
        } else if (JOY_PRESSED(joy, JOY_RIGHT) && btn_b) {
            _dos_c_locate(0, 12);
            print("Special: RIGHT + B (Dash!)          ");
        } else {
            _dos_c_locate(0, 12);
            print("                                    ");
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
            for (i = 0; i < 1000; i++)
                ;
        }
    }
}

/* 데모 4: 2인용 대전 */
static void demo_two_player(void)
{
    int joy1, joy2;
    int p1_x = 15, p1_y = 15;
    int p2_x = 60, p2_y = 15;
    int prev_p1_x = p1_x, prev_p1_y = p1_y;
    int prev_p2_x = p2_x, prev_p2_y = p2_y;
    int p1_score = 0, p2_score = 0;

    _dos_c_cls_al();

    print("=== Demo 4: Two Player Mode ===");
    newline();
    print("P1(JOY1): Move with stick, BTN-A to attack");
    newline();
    print("P2(JOY2): Move with stick, BTN-A to attack");
    newline();
    print("Press ESC to exit.");
    newline();

    /* 초기 위치 */
    _dos_c_locate(p1_x, p1_y);
    print_char('1');
    _dos_c_locate(p2_x, p2_y);
    print_char('2');

    while (1) {
        joy1 = read_joystick(JOY_PORT1);
        joy2 = read_joystick(JOY_PORT2);

        /* P1 이동 */
        if (JOY_PRESSED(joy1, JOY_UP) && p1_y > 5) p1_y--;
        if (JOY_PRESSED(joy1, JOY_DOWN) && p1_y < 28) p1_y++;
        if (JOY_PRESSED(joy1, JOY_LEFT) && p1_x > 0) p1_x--;
        if (JOY_PRESSED(joy1, JOY_RIGHT) && p1_x < 78) p1_x++;

        /* P2 이동 */
        if (JOY_PRESSED(joy2, JOY_UP) && p2_y > 5) p2_y--;
        if (JOY_PRESSED(joy2, JOY_DOWN) && p2_y < 28) p2_y++;
        if (JOY_PRESSED(joy2, JOY_LEFT) && p2_x > 0) p2_x--;
        if (JOY_PRESSED(joy2, JOY_RIGHT) && p2_x < 78) p2_x++;

        /* P1 화면 업데이트 */
        if (p1_x != prev_p1_x || p1_y != prev_p1_y) {
            _dos_c_locate(prev_p1_x, prev_p1_y);
            print_char(' ');
            _dos_c_locate(p1_x, p1_y);
            print_char('1');
            prev_p1_x = p1_x;
            prev_p1_y = p1_y;
        }

        /* P2 화면 업데이트 */
        if (p2_x != prev_p2_x || p2_y != prev_p2_y) {
            _dos_c_locate(prev_p2_x, prev_p2_y);
            print_char(' ');
            _dos_c_locate(p2_x, p2_y);
            print_char('2');
            prev_p2_x = p2_x;
            prev_p2_y = p2_y;
        }

        /* 충돌 및 공격 감지 */
        if (p1_x == p2_x && p1_y == p2_y) {
            /* 같은 위치에 있을 때 공격 판정 */
            if (JOY_PRESSED(joy1, JOY_BTN_A) && !JOY_PRESSED(joy2, JOY_BTN_A)) {
                p1_score++;
                p2_x = 60;
                p2_y = 15;
            } else if (JOY_PRESSED(joy2, JOY_BTN_A) && !JOY_PRESSED(joy1, JOY_BTN_A)) {
                p2_score++;
                p1_x = 15;
                p1_y = 15;
            }
        }

        /* 스코어 표시 */
        _dos_c_locate(0, 4);
        print("P1 Score: ");
        print_num(p1_score);
        print("   P2 Score: ");
        print_num(p2_score);
        print("   ");

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
            for (i = 0; i < 3000; i++)
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
    print("  X68000 Joystick Input Demo");
    newline();
    print("========================================");
    newline();
    newline();

    print("This demo shows joystick input handling.");
    newline();
    print("Connect joysticks to JOY1/JOY2 ports.");
    newline();
    newline();
    print("Press any key to start...");
    newline();
    _iocs_b_keyinp();

    /* 데모 실행 */
    _dos_c_cls_al();
    demo_joystick_info();

    demo_character_move();

    demo_button_combo();

    demo_two_player();

    /* 종료 */
    _dos_c_cls_al();
    print("Joystick demo complete!");
    newline();
    print("Press any key to exit...");
    newline();
    _iocs_b_keyinp();

    return 0;
}
