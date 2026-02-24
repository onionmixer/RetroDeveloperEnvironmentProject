/*
 * X68000 Human68k - Keyboard Input Example
 * 키보드 입력 처리 예제 (IOCS/DOS)
 *
 * X68000 키보드:
 * - 풀 키보드 (109키)
 * - 스캔코드 + ASCII 코드 반환
 * - 특수키 (SHIFT, CTRL, OPT, XF1-5 등)
 */
#include <x68k/iocs.h>
#include <x68k/dos.h>

/* 특수 키 스캔코드 */
#define KEY_ESC         0x01
#define KEY_RETURN      0x1D
#define KEY_SPACE       0x35
#define KEY_UP          0x3C
#define KEY_DOWN        0x3E
#define KEY_LEFT        0x3B
#define KEY_RIGHT       0x3D
#define KEY_F1          0x63
#define KEY_F2          0x64
#define KEY_F3          0x65
#define KEY_F4          0x66
#define KEY_F5          0x67

/* 키 코드 구조 (IOCS B_KEYINP 반환값) */
/* 상위 바이트: 스캔코드, 하위 바이트: ASCII 코드 */
#define SCANCODE(k)     (((k) >> 8) & 0xFF)
#define CHARCODE(k)     ((k) & 0xFF)

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

/* 16진수 출력 */
static void print_hex(int value, int digits)
{
    char hex[] = "0123456789ABCDEF";
    int i;
    for (i = (digits - 1) * 4; i >= 0; i -= 4) {
        print_char(hex[(value >> i) & 0xF]);
    }
}

/* 10진수 출력 */
static void print_num(int num)
{
    char buf[12];
    int i = 0;

    if (num == 0) {
        print_char('0');
        return;
    }
    if (num < 0) {
        print_char('-');
        num = -num;
    }
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    while (i > 0) {
        print_char(buf[--i]);
    }
}

/* 키 이름 반환 */
static const char* get_key_name(int scancode)
{
    switch (scancode) {
        case KEY_ESC:    return "ESC";
        case KEY_RETURN: return "RETURN";
        case KEY_SPACE:  return "SPACE";
        case KEY_UP:     return "UP";
        case KEY_DOWN:   return "DOWN";
        case KEY_LEFT:   return "LEFT";
        case KEY_RIGHT:  return "RIGHT";
        case KEY_F1:     return "F1";
        case KEY_F2:     return "F2";
        case KEY_F3:     return "F3";
        case KEY_F4:     return "F4";
        case KEY_F5:     return "F5";
        default:         return NULL;
    }
}

/* IOCS를 사용한 키 입력 (블로킹) */
static int get_key_iocs(void)
{
    return _iocs_b_keyinp();
}

/* IOCS를 사용한 키 확인 (비블로킹) */
static int check_key_iocs(void)
{
    return _iocs_b_keysns();
}

/* DOS를 사용한 키 입력 (비블로킹) */
static int get_key_dos(void)
{
    return _dos_inkey();
}

/* DOS를 사용한 키 확인 */
static int check_key_dos(void)
{
    return _dos_keysns();
}

/* 데모 1: 기본 키 입력 */
static void demo_basic_input(void)
{
    int key;
    int scancode, charcode;
    const char *keyname;

    print("=== Demo 1: Basic Key Input (IOCS) ===");
    newline();
    print("Press keys to see their codes. Press ESC to continue.");
    newline();
    newline();

    while (1) {
        key = get_key_iocs();
        scancode = SCANCODE(key);
        charcode = CHARCODE(key);

        print("Key: 0x");
        print_hex(key, 4);
        print("  Scan: 0x");
        print_hex(scancode, 2);
        print("  Char: ");

        if (charcode >= 0x20 && charcode < 0x7F) {
            print_char('\'');
            print_char(charcode);
            print_char('\'');
        } else {
            print("0x");
            print_hex(charcode, 2);
        }

        keyname = get_key_name(scancode);
        if (keyname) {
            print("  (");
            print(keyname);
            print(")");
        }
        newline();

        if (scancode == KEY_ESC) {
            break;
        }
    }
    newline();
}

/* 데모 2: 비블로킹 입력 */
static void demo_nonblocking(void)
{
    int key;
    int count = 0;

    print("=== Demo 2: Non-blocking Input (DOS) ===");
    newline();
    print("Press any key (ESC to continue). Counter runs while waiting.");
    newline();
    newline();

    while (1) {
        /* 키 입력 확인 (비블로킹) */
        key = get_key_dos();

        if (key != 0) {
            print("\r\nKey pressed: 0x");
            print_hex(key, 4);
            newline();

            if (SCANCODE(key) == KEY_ESC || key == 0x1B) {
                break;
            }
        }

        /* 카운터 표시 */
        print("\rWaiting... ");
        print_num(count++);
        print("   ");

        /* 약간의 딜레이 */
        {
            volatile int i;
            for (i = 0; i < 10000; i++)
                ;
        }
    }
    newline();
}

/* 데모 3: 방향키 처리 */
static void demo_arrow_keys(void)
{
    int key, scancode;
    int x = 10, y = 5;

    print("=== Demo 3: Arrow Keys ===");
    newline();
    print("Use arrow keys to move '*'. Press ESC to continue.");
    newline();
    newline();

    /* 초기 위치에 * 표시 */
    _dos_c_locate(x, y + 3);
    print_char('*');

    while (1) {
        key = get_key_iocs();
        scancode = SCANCODE(key);

        /* 이전 위치 지우기 */
        _dos_c_locate(x, y + 3);
        print_char(' ');

        /* 방향키에 따라 이동 */
        switch (scancode) {
            case KEY_UP:
                if (y > 0) y--;
                break;
            case KEY_DOWN:
                if (y < 10) y++;
                break;
            case KEY_LEFT:
                if (x > 0) x--;
                break;
            case KEY_RIGHT:
                if (x < 30) x++;
                break;
            case KEY_ESC:
                newline();
                return;
        }

        /* 새 위치에 * 표시 */
        _dos_c_locate(x, y + 3);
        print_char('*');

        /* 좌표 표시 */
        _dos_c_locate(0, 15);
        print("Position: (");
        print_num(x);
        print(", ");
        print_num(y);
        print(")   ");
    }
}

/* 데모 4: 문자열 입력 */
static void demo_string_input(void)
{
    struct dos_inpptr inp;
    int result;

    print("=== Demo 4: String Input (DOS) ===");
    newline();
    print("Enter your name (max 20 chars): ");

    inp.max = 20;  /* 최대 입력 길이 */

    result = _dos_gets(&inp);

    newline();
    print("You entered: ");
    print(inp.buffer);
    newline();
    print("Length: ");
    print_num(inp.length);
    print(" characters");
    newline();
    newline();
}

int main(void)
{
    /* 화면 클리어 */
    _dos_c_cls_al();

    print("========================================");
    newline();
    print("  X68000 Keyboard Input Demo");
    newline();
    print("========================================");
    newline();
    newline();

    /* 데모 실행 */
    demo_basic_input();
    demo_nonblocking();
    demo_arrow_keys();

    /* 화면 클리어 후 문자열 입력 */
    _dos_c_cls_al();
    demo_string_input();

    print("Demo complete! Press any key to exit.");
    newline();
    _iocs_b_keyinp();

    return 0;
}
