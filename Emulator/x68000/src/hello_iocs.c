/*
 * X68000 Human68k - IOCS Call Example
 * IOCS (I/O Control System) 직접 호출 예제
 *
 * IOCS는 X68000의 BIOS로, 하드웨어를 직접 제어합니다.
 * ROM에 내장되어 있어 Human68k 없이도 사용 가능합니다.
 */
#include <x68k/iocs.h>

/* 문자열 출력 (IOCS B_PRINT) */
static void print_string(const char *str)
{
    _iocs_b_print(str);
}

/* 한 문자 출력 (IOCS B_PUTC) */
static void print_char(int c)
{
    _iocs_b_putc(c);
}

/* 개행 출력 */
static void print_newline(void)
{
    print_char('\r');
    print_char('\n');
}

/* 숫자를 문자열로 변환하여 출력 */
static void print_number(int num)
{
    char buf[12];
    int i = 0;
    int neg = 0;

    if (num < 0) {
        neg = 1;
        num = -num;
    }

    if (num == 0) {
        print_char('0');
        return;
    }

    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    if (neg) print_char('-');

    while (i > 0) {
        print_char(buf[--i]);
    }
}

int main(void)
{
    struct iocs_time time;
    int datebin;

    /* 화면 클리어 */
    _iocs_b_clr_al();

    /* IOCS를 사용한 문자열 출력 */
    print_string("=== IOCS Call Example ===");
    print_newline();
    print_newline();

    print_string("Hello, X68000 World! (via IOCS B_PRINT)");
    print_newline();
    print_newline();

    /* 한 문자씩 출력 */
    print_string("Character output: ");
    print_char('I');
    print_char('O');
    print_char('C');
    print_char('S');
    print_newline();
    print_newline();

    /* 현재 시간 가져오기 (IOCS ONTIME) */
    time = _iocs_ontime();
    print_string("System time (1/100 sec from midnight): ");
    print_number(time.sec);
    print_newline();

    /* 시간 가져오기 (IOCS TIMEGET - BCD 형식) */
    datebin = _iocs_timeget();
    print_string("Time (BCD format): 0x");
    /* 16진수로 출력 */
    {
        int i;
        char hex[] = "0123456789ABCDEF";
        for (i = 28; i >= 0; i -= 4) {
            print_char(hex[(datebin >> i) & 0xf]);
        }
    }
    print_newline();
    print_newline();

    print_string("Press any key to exit...");
    print_newline();

    /* 키 입력 대기 (IOCS B_KEYINP) */
    _iocs_b_keyinp();

    return 0;
}
