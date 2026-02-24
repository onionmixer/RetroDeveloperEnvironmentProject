/*
 * X68000 Human68k - RTC (Real-Time Clock) Example
 * RTC 시계 예제
 *
 * X68000 RTC:
 * - RP5C15 실시간 클럭
 * - 배터리 백업 (전원 꺼져도 유지)
 * - 년/월/일/요일/시/분/초
 * - 알람 기능
 * - 1Hz 출력
 */
#include <x68k/iocs.h>
#include <x68k/dos.h>

/* RTC 레지스터 베이스 */
#define RTC_BASE        0xE8A000

/* RTC 레지스터 (Bank 0) */
#define RTC_SEC1        (*(volatile unsigned char *)(RTC_BASE + 0x01))  /* 초 1의 자리 */
#define RTC_SEC10       (*(volatile unsigned char *)(RTC_BASE + 0x03))  /* 초 10의 자리 */
#define RTC_MIN1        (*(volatile unsigned char *)(RTC_BASE + 0x05))  /* 분 1의 자리 */
#define RTC_MIN10       (*(volatile unsigned char *)(RTC_BASE + 0x07))  /* 분 10의 자리 */
#define RTC_HOUR1       (*(volatile unsigned char *)(RTC_BASE + 0x09))  /* 시 1의 자리 */
#define RTC_HOUR10      (*(volatile unsigned char *)(RTC_BASE + 0x0B))  /* 시 10의 자리 */
#define RTC_WEEK        (*(volatile unsigned char *)(RTC_BASE + 0x0D))  /* 요일 (0=일) */
#define RTC_DAY1        (*(volatile unsigned char *)(RTC_BASE + 0x0F))  /* 일 1의 자리 */
#define RTC_DAY10       (*(volatile unsigned char *)(RTC_BASE + 0x11))  /* 일 10의 자리 */
#define RTC_MON1        (*(volatile unsigned char *)(RTC_BASE + 0x13))  /* 월 1의 자리 */
#define RTC_MON10       (*(volatile unsigned char *)(RTC_BASE + 0x15))  /* 월 10의 자리 */
#define RTC_YEAR1       (*(volatile unsigned char *)(RTC_BASE + 0x17))  /* 년 1의 자리 */
#define RTC_YEAR10      (*(volatile unsigned char *)(RTC_BASE + 0x19))  /* 년 10의 자리 */
#define RTC_MODE        (*(volatile unsigned char *)(RTC_BASE + 0x1B))  /* 모드 레지스터 */
#define RTC_TEST        (*(volatile unsigned char *)(RTC_BASE + 0x1D))  /* 테스트 레지스터 */
#define RTC_RESET       (*(volatile unsigned char *)(RTC_BASE + 0x1F))  /* 리셋 레지스터 */

/* RTC 레지스터 (Bank 1 - 알람) */
#define RTC_CLKOUT      (*(volatile unsigned char *)(RTC_BASE + 0x01))  /* CLKOUT 선택 */
#define RTC_ADJUST      (*(volatile unsigned char *)(RTC_BASE + 0x03))  /* 조정 레지스터 */
#define RTC_ALM_MIN1    (*(volatile unsigned char *)(RTC_BASE + 0x05))  /* 알람 분 1의 자리 */
#define RTC_ALM_MIN10   (*(volatile unsigned char *)(RTC_BASE + 0x07))  /* 알람 분 10의 자리 */
#define RTC_ALM_HOUR1   (*(volatile unsigned char *)(RTC_BASE + 0x09))  /* 알람 시 1의 자리 */
#define RTC_ALM_HOUR10  (*(volatile unsigned char *)(RTC_BASE + 0x0B))  /* 알람 시 10의 자리 */
#define RTC_ALM_WEEK    (*(volatile unsigned char *)(RTC_BASE + 0x0D))  /* 알람 요일 */
#define RTC_ALM_DAY1    (*(volatile unsigned char *)(RTC_BASE + 0x0F))  /* 알람 일 1의 자리 */
#define RTC_ALM_DAY10   (*(volatile unsigned char *)(RTC_BASE + 0x11))  /* 알람 일 10의 자리 */
#define RTC_LEAP        (*(volatile unsigned char *)(RTC_BASE + 0x17))  /* 윤년 카운터 */

/* 모드 레지스터 비트 */
#define MODE_BANK       0x01    /* 뱅크 선택 (0/1) */
#define MODE_ALARM_EN   0x04    /* 알람 인터럽트 활성화 */
#define MODE_TIMER_EN   0x08    /* 타이머 인터럽트 활성화 */

/* 리셋 레지스터 비트 */
#define RESET_ALARM     0x01    /* 알람 리셋 */
#define RESET_1HZ       0x02    /* 1Hz 출력 리셋 */
#define RESET_16HZ      0x04    /* 16Hz 출력 리셋 */

/* 요일 문자열 */
static const char *day_names[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char *day_names_full[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

/* 월별 일수 */
static const int days_in_month[] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

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

static void print_num2(int num)
{
    /* 2자리 숫자 (앞에 0 채움) */
    if (num < 10) print_char('0');
    print_num(num);
}

static void print_num4(int num)
{
    /* 4자리 숫자 (앞에 0 채움) */
    if (num < 1000) print_char('0');
    if (num < 100) print_char('0');
    if (num < 10) print_char('0');
    print_num(num);
}

/* 시간 구조체 */
typedef struct {
    int year;       /* 1980-2079 */
    int month;      /* 1-12 */
    int day;        /* 1-31 */
    int week;       /* 0-6 (일-토) */
    int hour;       /* 0-23 */
    int min;        /* 0-59 */
    int sec;        /* 0-59 */
} DateTime;

/* RTC 뱅크 선택 */
static void rtc_select_bank(int bank)
{
    unsigned char mode = RTC_MODE;
    if (bank) {
        RTC_MODE = mode | MODE_BANK;
    } else {
        RTC_MODE = mode & ~MODE_BANK;
    }
}

/* RTC에서 시간 읽기 */
static void rtc_read(DateTime *dt)
{
    rtc_select_bank(0);

    dt->sec = (RTC_SEC10 & 0x07) * 10 + (RTC_SEC1 & 0x0F);
    dt->min = (RTC_MIN10 & 0x07) * 10 + (RTC_MIN1 & 0x0F);
    dt->hour = (RTC_HOUR10 & 0x03) * 10 + (RTC_HOUR1 & 0x0F);
    dt->day = (RTC_DAY10 & 0x03) * 10 + (RTC_DAY1 & 0x0F);
    dt->month = (RTC_MON10 & 0x01) * 10 + (RTC_MON1 & 0x0F);
    dt->year = 1980 + (RTC_YEAR10 & 0x0F) * 10 + (RTC_YEAR1 & 0x0F);
    dt->week = RTC_WEEK & 0x07;
}

/* RTC에 시간 쓰기 */
static void rtc_write(const DateTime *dt)
{
    int year = dt->year - 1980;

    rtc_select_bank(0);

    RTC_SEC1 = dt->sec % 10;
    RTC_SEC10 = dt->sec / 10;
    RTC_MIN1 = dt->min % 10;
    RTC_MIN10 = dt->min / 10;
    RTC_HOUR1 = dt->hour % 10;
    RTC_HOUR10 = dt->hour / 10;
    RTC_DAY1 = dt->day % 10;
    RTC_DAY10 = dt->day / 10;
    RTC_MON1 = dt->month % 10;
    RTC_MON10 = dt->month / 10;
    RTC_YEAR1 = year % 10;
    RTC_YEAR10 = year / 10;
    RTC_WEEK = dt->week;
}

/* IOCS를 통한 시간 읽기 */
static void rtc_read_iocs(DateTime *dt)
{
    int date = _iocs_bindateget();
    int time = _iocs_timeget();

    /* BINDATEGET: bit31-16=년, bit15-8=월, bit7-0=일 */
    dt->year = (date >> 16) & 0xFFFF;
    dt->month = (date >> 8) & 0xFF;
    dt->day = date & 0xFF;

    /* TIMEGET: bit31-24=요일, bit23-16=시, bit15-8=분, bit7-0=초 (BCD) */
    dt->week = (time >> 24) & 0x07;
    dt->hour = ((time >> 20) & 0x0F) * 10 + ((time >> 16) & 0x0F);
    dt->min = ((time >> 12) & 0x0F) * 10 + ((time >> 8) & 0x0F);
    dt->sec = ((time >> 4) & 0x0F) * 10 + (time & 0x0F);
}

/* 윤년 판정 */
static int is_leap_year(int year)
{
    if (year % 400 == 0) return 1;
    if (year % 100 == 0) return 0;
    if (year % 4 == 0) return 1;
    return 0;
}

/* 요일 계산 (Zeller's congruence) */
static int calc_weekday(int year, int month, int day)
{
    int y, m, k, j, h;

    if (month < 3) {
        month += 12;
        year--;
    }

    y = year;
    m = month;
    k = y % 100;
    j = y / 100;

    h = (day + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;

    /* 0=토, 1=일, 2=월... -> 0=일, 1=월... */
    return (h + 6) % 7;
}

/* 날짜/시간 표시 */
static void print_datetime(const DateTime *dt)
{
    print_num4(dt->year);
    print("-");
    print_num2(dt->month);
    print("-");
    print_num2(dt->day);
    print(" (");
    print(day_names[dt->week]);
    print(") ");
    print_num2(dt->hour);
    print(":");
    print_num2(dt->min);
    print(":");
    print_num2(dt->sec);
}

/* 달력 출력 */
static void print_calendar(int year, int month)
{
    int first_day, num_days;
    int i, col;

    /* 월의 첫날 요일 */
    first_day = calc_weekday(year, month, 1);

    /* 해당 월의 일수 */
    num_days = days_in_month[month - 1];
    if (month == 2 && is_leap_year(year)) {
        num_days = 29;
    }

    print("     ");
    print_num4(year);
    print("-");
    print_num2(month);
    newline();
    print(" Su Mo Tu We Th Fr Sa");
    newline();

    /* 첫 주 앞의 공백 */
    for (i = 0; i < first_day; i++) {
        print("   ");
    }

    /* 일자 출력 */
    col = first_day;
    for (i = 1; i <= num_days; i++) {
        if (i < 10) print(" ");
        print(" ");
        print_num(i);
        col++;
        if (col >= 7) {
            col = 0;
            newline();
        }
    }
    if (col != 0) {
        newline();
    }
}

/* 데모 1: 현재 시간 표시 */
static void demo_current_time(void)
{
    DateTime dt;
    int i;

    print("=== Demo 1: Current Time ===");
    newline();
    print("Reading RTC... (Press any key to stop)");
    newline();
    newline();

    for (i = 0; i < 100; i++) {
        /* 커서 위치 고정 */
        _dos_c_locate(0, 4);

        /* 직접 읽기 */
        print("Direct RTC:  ");
        rtc_read(&dt);
        print_datetime(&dt);
        newline();

        /* IOCS를 통한 읽기 */
        print("Via IOCS:    ");
        rtc_read_iocs(&dt);
        print_datetime(&dt);
        newline();

        /* 1초 대기 */
        {
            struct iocs_time start = _iocs_ontime();
            while (1) {
                struct iocs_time now = _iocs_ontime();
                if (now.sec - start.sec >= 100) break;
                if (_iocs_b_keysns()) {
                    _iocs_b_keyinp();
                    return;
                }
            }
        }
    }
}

/* 데모 2: 달력 표시 */
static void demo_calendar(void)
{
    DateTime dt;
    int year, month;
    int key, scan;

    print("=== Demo 2: Calendar ===");
    newline();
    print("Use LEFT/RIGHT to change month, ESC to exit");
    newline();
    newline();

    rtc_read(&dt);
    year = dt.year;
    month = dt.month;

    while (1) {
        _dos_c_locate(0, 4);

        print_calendar(year, month);

        /* 오늘 표시 */
        newline();
        print("Today: ");
        rtc_read(&dt);
        print_datetime(&dt);
        print("    ");
        newline();

        /* 입력 대기 */
        if (_iocs_b_keysns()) {
            key = _iocs_b_keyinp();
            scan = (key >> 8) & 0xFF;

            if (scan == 0x01) break;        /* ESC */
            if (scan == 0x3B) {             /* LEFT */
                month--;
                if (month < 1) {
                    month = 12;
                    year--;
                }
            }
            if (scan == 0x3D) {             /* RIGHT */
                month++;
                if (month > 12) {
                    month = 1;
                    year++;
                }
            }
            if (scan == 0x3C) {             /* UP - 이전 년 */
                year--;
            }
            if (scan == 0x3E) {             /* DOWN - 다음 년 */
                year++;
            }
        }

        /* 짧은 대기 */
        {
            struct iocs_time start = _iocs_ontime();
            while (1) {
                struct iocs_time now = _iocs_ontime();
                if (now.sec - start.sec >= 10) break;
                if (_iocs_b_keysns()) break;
            }
        }
    }
}

/* 데모 3: 스톱워치 */
static void demo_stopwatch(void)
{
    int running = 0;
    int elapsed = 0;        /* 1/100초 단위 */
    int lap_times[10];
    int lap_count = 0;
    int i;
    struct iocs_time last_time;
    int key, scan;

    print("=== Demo 3: Stopwatch ===");
    newline();
    print("SPACE: Start/Stop, L: Lap, R: Reset, ESC: Exit");
    newline();
    newline();

    last_time = _iocs_ontime();

    while (1) {
        /* 타이머 업데이트 */
        if (running) {
            struct iocs_time now = _iocs_ontime();
            int delta = now.sec - last_time.sec;
            if (delta < 0) delta += 8640000;  /* 자정 넘김 */
            elapsed += delta;
            last_time = now;
        }

        /* 표시 */
        _dos_c_locate(0, 4);

        /* 메인 타이머 */
        {
            int min = (elapsed / 6000) % 60;
            int sec = (elapsed / 100) % 60;
            int cs = elapsed % 100;

            print("  ");
            print_num2(min);
            print(":");
            print_num2(sec);
            print(".");
            print_num2(cs);
            print("  ");

            if (running) {
                print("[RUNNING]");
            } else {
                print("[STOPPED]");
            }
            print("    ");
            newline();
        }

        newline();
        print("Lap times:");
        newline();
        for (i = 0; i < 10; i++) {
            print("  ");
            print_num(i + 1);
            print(": ");
            if (i < lap_count) {
                int t = lap_times[i];
                int min = (t / 6000) % 60;
                int sec = (t / 100) % 60;
                int cs = t % 100;
                print_num2(min);
                print(":");
                print_num2(sec);
                print(".");
                print_num2(cs);
            } else {
                print("--:--.--");
            }
            newline();
        }

        /* 입력 처리 */
        if (_iocs_b_keysns()) {
            key = _iocs_b_keyinp();
            scan = (key >> 8) & 0xFF;

            if (scan == 0x01) break;        /* ESC */
            if (scan == 0x35) {             /* SPACE */
                running = !running;
                if (running) {
                    last_time = _iocs_ontime();
                }
            }
            if (scan == 0x26 && lap_count < 10) {  /* L */
                lap_times[lap_count++] = elapsed;
            }
            if (scan == 0x13) {             /* R */
                running = 0;
                elapsed = 0;
                lap_count = 0;
            }
        }

        /* 짧은 대기 */
        {
            volatile int j;
            for (j = 0; j < 5000; j++) { }
        }
    }
}

/* 데모 4: RTC 레지스터 덤프 */
static void demo_register_dump(void)
{
    print("=== Demo 4: RTC Register Dump ===");
    newline();
    newline();

    print("--- Bank 0 (Time) ---");
    newline();

    rtc_select_bank(0);

    print("  SEC:   ");
    print_num(RTC_SEC10 & 0x07);
    print_num(RTC_SEC1 & 0x0F);
    print(" (raw: ");
    print_num(RTC_SEC10);
    print("/");
    print_num(RTC_SEC1);
    print(")");
    newline();

    print("  MIN:   ");
    print_num(RTC_MIN10 & 0x07);
    print_num(RTC_MIN1 & 0x0F);
    newline();

    print("  HOUR:  ");
    print_num(RTC_HOUR10 & 0x03);
    print_num(RTC_HOUR1 & 0x0F);
    newline();

    print("  WEEK:  ");
    print_num(RTC_WEEK & 0x07);
    print(" (");
    print(day_names[RTC_WEEK & 0x07]);
    print(")");
    newline();

    print("  DAY:   ");
    print_num(RTC_DAY10 & 0x03);
    print_num(RTC_DAY1 & 0x0F);
    newline();

    print("  MON:   ");
    print_num(RTC_MON10 & 0x01);
    print_num(RTC_MON1 & 0x0F);
    newline();

    print("  YEAR:  ");
    print_num(RTC_YEAR10 & 0x0F);
    print_num(RTC_YEAR1 & 0x0F);
    print(" (19");
    print_num(80 + (RTC_YEAR10 & 0x0F) * 10 + (RTC_YEAR1 & 0x0F));
    print(")");
    newline();

    print("  MODE:  0x");
    {
        int mode = RTC_MODE;
        print_num(mode >> 4);
        print_num(mode & 0x0F);
    }
    newline();

    newline();
    print("--- Bank 1 (Alarm/Control) ---");
    newline();

    rtc_select_bank(1);

    print("  CLKOUT: ");
    print_num(RTC_CLKOUT & 0x0F);
    newline();

    print("  ADJUST: ");
    print_num(RTC_ADJUST & 0x01);
    newline();

    print("  LEAP:   ");
    print_num(RTC_LEAP & 0x03);
    newline();

    rtc_select_bank(0);
    newline();
}

/* 데모 5: 시간 설정 */
static void demo_set_time(void)
{
    DateTime dt;
    int field = 0;      /* 0=year, 1=month, 2=day, 3=hour, 4=min, 5=sec */
    int key, scan;
    int *values[6];

    print("=== Demo 5: Set Time ===");
    newline();
    print("Use LEFT/RIGHT to select field, UP/DOWN to change value");
    newline();
    print("Press ENTER to apply, ESC to cancel");
    newline();
    newline();

    /* 현재 시간 읽기 */
    rtc_read(&dt);

    values[0] = &dt.year;
    values[1] = &dt.month;
    values[2] = &dt.day;
    values[3] = &dt.hour;
    values[4] = &dt.min;
    values[5] = &dt.sec;

    while (1) {
        _dos_c_locate(0, 5);

        /* 년 */
        if (field == 0) print("[");
        else print(" ");
        print_num4(dt.year);
        if (field == 0) print("]");
        else print(" ");

        print("-");

        /* 월 */
        if (field == 1) print("[");
        else print(" ");
        print_num2(dt.month);
        if (field == 1) print("]");
        else print(" ");

        print("-");

        /* 일 */
        if (field == 2) print("[");
        else print(" ");
        print_num2(dt.day);
        if (field == 2) print("]");
        else print(" ");

        print("  ");

        /* 시 */
        if (field == 3) print("[");
        else print(" ");
        print_num2(dt.hour);
        if (field == 3) print("]");
        else print(" ");

        print(":");

        /* 분 */
        if (field == 4) print("[");
        else print(" ");
        print_num2(dt.min);
        if (field == 4) print("]");
        else print(" ");

        print(":");

        /* 초 */
        if (field == 5) print("[");
        else print(" ");
        print_num2(dt.sec);
        if (field == 5) print("]");
        else print(" ");

        newline();
        newline();

        /* 요일 표시 (자동 계산) */
        dt.week = calc_weekday(dt.year, dt.month, dt.day);
        print("Day of week: ");
        print(day_names[dt.week]);
        print(" (");
        print(day_names_full[dt.week]);
        print(")   ");
        newline();

        /* 입력 대기 */
        key = _iocs_b_keyinp();
        scan = (key >> 8) & 0xFF;

        if (scan == 0x01) {             /* ESC */
            print("Cancelled.");
            newline();
            return;
        }
        if (scan == 0x1D) {             /* RETURN */
            /* 요일 계산 후 적용 */
            dt.week = calc_weekday(dt.year, dt.month, dt.day);
            rtc_write(&dt);
            print("Time set successfully!");
            newline();
            return;
        }
        if (scan == 0x3B) {             /* LEFT */
            field--;
            if (field < 0) field = 5;
        }
        if (scan == 0x3D) {             /* RIGHT */
            field++;
            if (field > 5) field = 0;
        }
        if (scan == 0x3C) {             /* UP */
            (*values[field])++;
            /* 범위 체크 */
            if (field == 0 && dt.year > 2079) dt.year = 1980;
            if (field == 1 && dt.month > 12) dt.month = 1;
            if (field == 2 && dt.day > 31) dt.day = 1;
            if (field == 3 && dt.hour > 23) dt.hour = 0;
            if (field == 4 && dt.min > 59) dt.min = 0;
            if (field == 5 && dt.sec > 59) dt.sec = 0;
        }
        if (scan == 0x3E) {             /* DOWN */
            (*values[field])--;
            /* 범위 체크 */
            if (field == 0 && dt.year < 1980) dt.year = 2079;
            if (field == 1 && dt.month < 1) dt.month = 12;
            if (field == 2 && dt.day < 1) dt.day = 31;
            if (field == 3 && dt.hour < 0) dt.hour = 23;
            if (field == 4 && dt.min < 0) dt.min = 59;
            if (field == 5 && dt.sec < 0) dt.sec = 59;
        }
    }
}

/* 데모 6: 디지털 시계 */
static void demo_digital_clock(void)
{
    DateTime dt;
    int last_sec = -1;

    print("=== Demo 6: Digital Clock ===");
    newline();
    print("Large digital clock display. Press any key to exit.");
    newline();

    while (1) {
        rtc_read(&dt);

        /* 초가 변했을 때만 업데이트 */
        if (dt.sec != last_sec) {
            last_sec = dt.sec;

            _dos_c_locate(5, 6);

            /* 큰 글자 시계 */
            print("+---------------------+");
            _dos_c_locate(5, 7);
            print("|                     |");
            _dos_c_locate(5, 8);
            print("|   ");
            print_num2(dt.hour);
            print(" : ");
            print_num2(dt.min);
            print(" : ");
            print_num2(dt.sec);
            print("   |");
            _dos_c_locate(5, 9);
            print("|                     |");
            _dos_c_locate(5, 10);
            print("+---------------------+");

            _dos_c_locate(5, 12);
            print_num4(dt.year);
            print("-");
            print_num2(dt.month);
            print("-");
            print_num2(dt.day);
            print("  ");
            print(day_names[dt.week]);
            print("       ");
        }

        if (_iocs_b_keysns()) {
            _iocs_b_keyinp();
            break;
        }

        /* 짧은 대기 */
        {
            volatile int j;
            for (j = 0; j < 10000; j++) { }
        }
    }
}

int main(void)
{
    /* 화면 클리어 */
    _dos_c_cls_al();

    print("========================================");
    newline();
    print("  X68000 RTC (Real-Time Clock) Demo");
    newline();
    print("========================================");
    newline();
    newline();

    print("RP5C15 RTC Features:");
    newline();
    print("- Battery-backed (keeps time when off)");
    newline();
    print("- Year/Month/Day/Week/Hour/Min/Sec");
    newline();
    print("- Alarm function");
    newline();
    print("- Leap year support");
    newline();
    newline();

    print("Press any key to start...");
    newline();
    _iocs_b_keyinp();

    /* 데모 실행 */
    _dos_c_cls_al();
    demo_current_time();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_calendar();

    _dos_c_cls_al();
    demo_stopwatch();

    _dos_c_cls_al();
    demo_register_dump();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_set_time();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_digital_clock();

    /* 종료 */
    _dos_c_cls_al();
    print("RTC demo complete!");
    newline();
    print("Press any key to exit...");
    newline();
    _iocs_b_keyinp();

    return 0;
}
