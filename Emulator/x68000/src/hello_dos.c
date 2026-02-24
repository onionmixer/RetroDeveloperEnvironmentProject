/*
 * X68000 Human68k - DOS Call Example
 * DOS 콜 직접 호출 예제
 *
 * DOS 콜은 Human68k 운영체제가 제공하는 시스템 콜입니다.
 * 파일 I/O, 프로세스 관리, 메모리 관리 등을 담당합니다.
 */
#include <x68k/dos.h>

/* 숫자를 2자리 문자열로 출력 */
static void print_2digit(int num)
{
    _dos_c_putc('0' + (num / 10) % 10);
    _dos_c_putc('0' + num % 10);
}

/* 숫자 출력 */
static void print_number(int num)
{
    char buf[12];
    int i = 0;

    if (num < 0) {
        _dos_c_putc('-');
        num = -num;
    }

    if (num == 0) {
        _dos_c_putc('0');
        return;
    }

    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    while (i > 0) {
        _dos_c_putc(buf[--i]);
    }
}

int main(void)
{
    int date, time;
    int year, month, day, weekday;
    int hour, minute, second;
    int curdrv;
    char cwd[256];
    const char *weekday_str[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };

    /* 화면 클리어 */
    _dos_c_cls_al();

    /* DOS 콜을 사용한 문자열 출력 */
    _dos_c_print("=== DOS Call Example ===\r\n\r\n");

    _dos_c_print("Hello, X68000 World! (via DOS C_PRINT)\r\n\r\n");

    /* 다른 출력 방법: _dos_print (표준 출력) */
    _dos_print("This line uses DOS PRINT function.\r\n\r\n");

    /* 현재 날짜 가져오기 (DOS GETDATE) */
    /* 반환값: bit 0-6: 년(1980+), bit 7-10: 월, bit 11-15: 일, bit 16-18: 요일 */
    date = _dos_getdate();
    year = 1980 + (date & 0x7f);
    month = (date >> 7) & 0x0f;
    day = (date >> 11) & 0x1f;
    weekday = (date >> 16) & 0x07;

    _dos_c_print("Current Date: ");
    print_number(year);
    _dos_c_putc('-');
    print_2digit(month);
    _dos_c_putc('-');
    print_2digit(day);
    _dos_c_print(" (");
    _dos_c_print(weekday_str[weekday]);
    _dos_c_print(")\r\n");

    /* 현재 시간 가져오기 (DOS GETTIME) */
    /* 반환값: bit 0-7: 시, bit 8-15: 분, bit 16-23: 초 */
    time = _dos_gettime();
    hour = time & 0xff;
    minute = (time >> 8) & 0xff;
    second = (time >> 16) & 0xff;

    _dos_c_print("Current Time: ");
    print_2digit(hour);
    _dos_c_putc(':');
    print_2digit(minute);
    _dos_c_putc(':');
    print_2digit(second);
    _dos_c_print("\r\n\r\n");

    /* 현재 드라이브 가져오기 */
    curdrv = _dos_curdrv();
    _dos_c_print("Current Drive: ");
    _dos_c_putc('A' + curdrv);
    _dos_c_print(":\r\n");

    /* 현재 디렉토리 가져오기 */
    if (_dos_curdir(curdrv + 1, cwd) >= 0) {
        _dos_c_print("Current Dir: \\");
        _dos_c_print(cwd);
        _dos_c_print("\r\n");
    }

    /* 메모리 정보 (malloc(-1)은 최대 할당 가능 크기 반환) */
    _dos_c_print("\r\nFree Memory: ");
    print_number((int)(long)_dos_malloc(-1));
    _dos_c_print(" bytes\r\n\r\n");

    _dos_c_print("Press any key to exit...\r\n");

    /* 키 입력 대기 (DOS INKEY) */
    while (_dos_inkey() == 0)
        ;

    return 0;
}
