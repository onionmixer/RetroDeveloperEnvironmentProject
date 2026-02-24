/*
 * X68000 Human68k - Serial Communication Example
 * RS-232C 시리얼 통신 예제
 *
 * X68000 시리얼 포트:
 * - RS-232C 표준 호환
 * - 8251 USART 호환 인터페이스
 * - 75 ~ 38400 baud
 * - 5/6/7/8 데이터 비트
 * - 1/1.5/2 스톱 비트
 * - None/Odd/Even 패리티
 */
#include <x68k/iocs.h>
#include <x68k/dos.h>
#include <string.h>

/* RS-232C 설정값 구조
 * _iocs_set232c 파라미터:
 *
 * bit 0-1: Stop bits
 *   0 = 1 stop bit
 *   1 = 1.5 stop bits (5-bit data only)
 *   2 = 2 stop bits
 *
 * bit 2-3: Parity
 *   0 = None
 *   1 = Odd
 *   2 = Even (actually 3)
 *
 * bit 4-5: Data bits
 *   0 = 5 bits
 *   1 = 6 bits
 *   2 = 7 bits
 *   3 = 8 bits
 *
 * bit 8-11: Baud rate
 *   0 = 75
 *   1 = 150
 *   2 = 300
 *   3 = 600
 *   4 = 1200
 *   5 = 2400
 *   6 = 4800
 *   7 = 9600
 *   8 = 17361 (non-standard)
 *   9 = 31250 (MIDI)
 *   A = 38400
 *
 * bit 12-15: XON/XOFF flow control
 */

/* 보드레이트 코드 */
#define BAUD_75         0x0000
#define BAUD_150        0x0100
#define BAUD_300        0x0200
#define BAUD_600        0x0300
#define BAUD_1200       0x0400
#define BAUD_2400       0x0500
#define BAUD_4800       0x0600
#define BAUD_9600       0x0700
#define BAUD_19200      0x0800  /* 17361 실제 */
#define BAUD_38400      0x0A00

/* 데이터 비트 */
#define DATA_5BIT       0x0000
#define DATA_6BIT       0x0010
#define DATA_7BIT       0x0020
#define DATA_8BIT       0x0030

/* 스톱 비트 */
#define STOP_1BIT       0x0000
#define STOP_1_5BIT     0x0001
#define STOP_2BIT       0x0002

/* 패리티 */
#define PARITY_NONE     0x0000
#define PARITY_ODD      0x0004
#define PARITY_EVEN     0x000C

/* XON/XOFF */
#define FLOW_NONE       0x0000
#define FLOW_XONXOFF    0x1000

/* 일반적인 설정 조합 */
#define SERIAL_9600_8N1     (BAUD_9600 | DATA_8BIT | STOP_1BIT | PARITY_NONE)
#define SERIAL_19200_8N1    (BAUD_19200 | DATA_8BIT | STOP_1BIT | PARITY_NONE)
#define SERIAL_38400_8N1    (BAUD_38400 | DATA_8BIT | STOP_1BIT | PARITY_NONE)
#define SERIAL_2400_7E1     (BAUD_2400 | DATA_7BIT | STOP_1BIT | PARITY_EVEN)

/* 보드레이트 테이블 */
static const struct {
    int code;
    int rate;
    const char *name;
} baud_table[] = {
    { BAUD_75,    75,    "75" },
    { BAUD_150,   150,   "150" },
    { BAUD_300,   300,   "300" },
    { BAUD_600,   600,   "600" },
    { BAUD_1200,  1200,  "1200" },
    { BAUD_2400,  2400,  "2400" },
    { BAUD_4800,  4800,  "4800" },
    { BAUD_9600,  9600,  "9600" },
    { BAUD_19200, 17361, "19200*" },
    { BAUD_38400, 38400, "38400" },
};
#define NUM_BAUDS (sizeof(baud_table) / sizeof(baud_table[0]))

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

static void print_hex(int value, int digits)
{
    char hex[] = "0123456789ABCDEF";
    int i;
    for (i = (digits - 1) * 4; i >= 0; i -= 4) {
        print_char(hex[(value >> i) & 0xF]);
    }
}

/* 시리얼 포트 초기화 */
static int serial_init(int config)
{
    return _iocs_set232c(config);
}

/* 시리얼 출력 (1바이트) */
static void serial_putc(int c)
{
    /* 송신 버퍼가 비워질 때까지 대기 */
    while (_iocs_osns232c() == 0) {
        /* busy wait */
    }
    _iocs_out232c(c);
}

/* 시리얼 문자열 출력 */
static void serial_puts(const char *str)
{
    while (*str) {
        serial_putc(*str++);
    }
}

/* 시리얼 입력 가능 확인 */
static int serial_available(void)
{
    return _iocs_isns232c();
}

/* 시리얼 입력 (1바이트, 블로킹) */
static int serial_getc(void)
{
    while (!serial_available()) {
        /* busy wait */
    }
    return _iocs_inp232c();
}

/* 시리얼 입력 (1바이트, 논블로킹) */
static int serial_getc_nb(void)
{
    if (serial_available()) {
        return _iocs_inp232c();
    }
    return -1;
}

/* 수신 버퍼 길이 */
static int serial_rx_length(void)
{
    return _iocs_lof232c();
}

/* 설정값을 문자열로 변환 */
static void config_to_string(int config, char *buf)
{
    int baud_code = (config >> 8) & 0x0F;
    int data_bits = ((config >> 4) & 0x03) + 5;
    int stop_bits = config & 0x03;
    int parity = (config >> 2) & 0x03;
    int i;
    const char *baud_str = "???";

    /* 보드레이트 찾기 */
    for (i = 0; i < (int)NUM_BAUDS; i++) {
        if ((baud_table[i].code >> 8) == baud_code) {
            baud_str = baud_table[i].name;
            break;
        }
    }

    /* 문자열 생성 */
    char *p = buf;

    /* 보드레이트 */
    while (*baud_str) *p++ = *baud_str++;

    *p++ = '-';

    /* 데이터 비트 */
    *p++ = '0' + data_bits;

    /* 패리티 */
    switch (parity) {
    case 0: *p++ = 'N'; break;
    case 1: *p++ = 'O'; break;
    case 3: *p++ = 'E'; break;
    default: *p++ = '?'; break;
    }

    /* 스톱 비트 */
    switch (stop_bits) {
    case 0: *p++ = '1'; break;
    case 1: *p++ = '1'; *p++ = '.'; *p++ = '5'; break;
    case 2: *p++ = '2'; break;
    default: *p++ = '?'; break;
    }

    *p = '\0';
}

/* 데모 1: 기본 송수신 */
static void demo_basic_io(void)
{
    char config_str[32];
    int config = SERIAL_9600_8N1;

    print("=== Demo 1: Basic Serial I/O ===");
    newline();

    config_to_string(config, config_str);
    print("Config: ");
    print(config_str);
    newline();
    newline();

    serial_init(config);

    print("Sending test message...");
    newline();

    serial_puts("Hello from X68000!\r\n");
    serial_puts("Serial port test message.\r\n");

    print("Sent 2 lines.");
    newline();
    newline();

    print("Waiting for input (5 seconds)...");
    newline();
    print("Received: ");

    {
        struct iocs_time start = _iocs_ontime();
        while (1) {
            struct iocs_time now = _iocs_ontime();
            if (now.sec - start.sec >= 500) break;

            int c = serial_getc_nb();
            if (c >= 0) {
                if (c >= 0x20 && c < 0x7F) {
                    print_char(c);
                } else {
                    print("[");
                    print_hex(c, 2);
                    print("]");
                }
            }
        }
    }

    newline();
    newline();
}

/* 데모 2: 터미널 에뮬레이터 */
static void demo_terminal(void)
{
    int config = SERIAL_9600_8N1;
    char config_str[32];
    int key, scan;

    print("=== Demo 2: Terminal Emulator ===");
    newline();

    config_to_string(config, config_str);
    print("Config: ");
    print(config_str);
    newline();
    print("Press ESC to exit");
    newline();
    print("--- Terminal Start ---");
    newline();

    serial_init(config);

    while (1) {
        /* 시리얼 수신 -> 화면 출력 */
        int c = serial_getc_nb();
        if (c >= 0) {
            if (c == '\r') {
                newline();
            } else if (c == '\n') {
                /* CR 다음의 LF는 무시 */
            } else if (c >= 0x20 && c < 0x7F) {
                print_char(c);
            } else if (c == '\t') {
                print("    ");
            } else if (c == 0x08 || c == 0x7F) {
                /* 백스페이스 */
                print("\b \b");
            }
        }

        /* 키보드 입력 -> 시리얼 송신 */
        if (_iocs_b_keysns()) {
            key = _iocs_b_keyinp();
            scan = (key >> 8) & 0xFF;

            if (scan == 0x01) break;  /* ESC */

            c = key & 0xFF;
            if (c) {
                serial_putc(c);
                /* 로컬 에코 */
                if (c == '\r') {
                    newline();
                } else if (c >= 0x20 && c < 0x7F) {
                    print_char(c);
                }
            }
        }
    }

    print("--- Terminal End ---");
    newline();
}

/* 데모 3: 루프백 테스트 */
static void demo_loopback(void)
{
    int config = SERIAL_9600_8N1;
    char config_str[32];
    unsigned char test_data[256];
    int i;
    int tx_count = 0, rx_count = 0, err_count = 0;

    print("=== Demo 3: Loopback Test ===");
    newline();

    config_to_string(config, config_str);
    print("Config: ");
    print(config_str);
    newline();
    print("Connect TX to RX for loopback test");
    newline();
    newline();

    serial_init(config);

    /* 테스트 데이터 생성 */
    for (i = 0; i < 256; i++) {
        test_data[i] = (unsigned char)i;
    }

    print("Sending 256 bytes...");
    newline();

    /* 송신 */
    for (i = 0; i < 256; i++) {
        serial_putc(test_data[i]);
        tx_count++;
    }

    print("Receiving...");
    newline();

    /* 수신 및 검증 (타임아웃 포함) */
    {
        struct iocs_time start = _iocs_ontime();
        i = 0;

        while (i < 256) {
            struct iocs_time now = _iocs_ontime();
            if (now.sec - start.sec >= 300) {
                print("Timeout!");
                newline();
                break;
            }

            int c = serial_getc_nb();
            if (c >= 0) {
                rx_count++;
                if ((unsigned char)c != test_data[i]) {
                    err_count++;
                    if (err_count <= 5) {
                        print("  Error at ");
                        print_num(i);
                        print(": expected 0x");
                        print_hex(test_data[i], 2);
                        print(", got 0x");
                        print_hex(c, 2);
                        newline();
                    }
                }
                i++;
            }
        }
    }

    newline();
    print("Results:");
    newline();
    print("  TX: ");
    print_num(tx_count);
    print(" bytes");
    newline();
    print("  RX: ");
    print_num(rx_count);
    print(" bytes");
    newline();
    print("  Errors: ");
    print_num(err_count);
    newline();

    if (rx_count == 256 && err_count == 0) {
        print("  Status: PASSED");
    } else if (rx_count == 0) {
        print("  Status: NO DATA (check loopback connection)");
    } else {
        print("  Status: FAILED");
    }
    newline();
    newline();
}

/* 데모 4: 보드레이트 설정 */
static void demo_baudrate(void)
{
    int i;
    int current_baud = 7;  /* 9600 */
    int config;
    char config_str[32];
    int key, scan;

    print("=== Demo 4: Baud Rate Selection ===");
    newline();
    print("Use UP/DOWN to select, ENTER to apply, ESC to exit");
    newline();
    newline();

    while (1) {
        _dos_c_locate(0, 4);

        print("Available baud rates:");
        newline();

        for (i = 0; i < (int)NUM_BAUDS; i++) {
            if (i == current_baud) {
                print(" > ");
            } else {
                print("   ");
            }
            print(baud_table[i].name);
            print(" baud");
            if (i == current_baud) {
                print(" <");
            }
            print("            ");
            newline();
        }

        newline();
        config = baud_table[current_baud].code | DATA_8BIT | STOP_1BIT | PARITY_NONE;
        config_to_string(config, config_str);
        print("Current: ");
        print(config_str);
        print("        ");
        newline();

        /* 입력 대기 */
        key = _iocs_b_keyinp();
        scan = (key >> 8) & 0xFF;

        if (scan == 0x01) break;        /* ESC */
        if (scan == 0x3C) {             /* UP */
            if (current_baud > 0) current_baud--;
        }
        if (scan == 0x3E) {             /* DOWN */
            if (current_baud < (int)NUM_BAUDS - 1) current_baud++;
        }
        if (scan == 0x1D) {             /* ENTER */
            serial_init(config);
            print("Applied! Sending test...");
            newline();
            serial_puts("Test at new baud rate\r\n");
            print("Done.");
            newline();

            struct iocs_time start = _iocs_ontime();
            while (1) {
                struct iocs_time now = _iocs_ontime();
                if (now.sec - start.sec >= 100) break;
            }
        }
    }
}

/* 데모 5: 데이터 포맷 설정 */
static void demo_data_format(void)
{
    int data_bits = 3;      /* 8 bits */
    int parity = 0;         /* None */
    int stop_bits = 0;      /* 1 bit */
    int field = 0;
    int config;
    char config_str[32];
    int key, scan;

    const char *data_names[] = { "5", "6", "7", "8" };
    const char *parity_names[] = { "None", "Odd", "Even" };
    const int parity_codes[] = { PARITY_NONE, PARITY_ODD, PARITY_EVEN };
    const char *stop_names[] = { "1", "1.5", "2" };

    print("=== Demo 5: Data Format Settings ===");
    newline();
    print("Use LEFT/RIGHT to select field, UP/DOWN to change");
    newline();
    print("Press ENTER to apply, ESC to exit");
    newline();
    newline();

    while (1) {
        _dos_c_locate(0, 5);

        /* 데이터 비트 */
        print("Data bits: ");
        if (field == 0) print("[");
        else print(" ");
        print(data_names[data_bits]);
        if (field == 0) print("]");
        else print(" ");
        print("       ");
        newline();

        /* 패리티 */
        print("Parity:    ");
        if (field == 1) print("[");
        else print(" ");
        print(parity_names[parity]);
        if (field == 1) print("]");
        else print(" ");
        print("       ");
        newline();

        /* 스톱 비트 */
        print("Stop bits: ");
        if (field == 2) print("[");
        else print(" ");
        print(stop_names[stop_bits]);
        if (field == 2) print("]");
        else print(" ");
        print("       ");
        newline();

        newline();
        config = BAUD_9600 | (data_bits << 4) | parity_codes[parity] | stop_bits;
        config_to_string(config, config_str);
        print("Format: ");
        print(config_str);
        print("        ");
        newline();

        /* 입력 대기 */
        key = _iocs_b_keyinp();
        scan = (key >> 8) & 0xFF;

        if (scan == 0x01) break;        /* ESC */
        if (scan == 0x3B) {             /* LEFT */
            field--;
            if (field < 0) field = 2;
        }
        if (scan == 0x3D) {             /* RIGHT */
            field++;
            if (field > 2) field = 0;
        }
        if (scan == 0x3C) {             /* UP */
            switch (field) {
            case 0: if (data_bits < 3) data_bits++; break;
            case 1: if (parity < 2) parity++; break;
            case 2: if (stop_bits < 2) stop_bits++; break;
            }
        }
        if (scan == 0x3E) {             /* DOWN */
            switch (field) {
            case 0: if (data_bits > 0) data_bits--; break;
            case 1: if (parity > 0) parity--; break;
            case 2: if (stop_bits > 0) stop_bits--; break;
            }
        }
        if (scan == 0x1D) {             /* ENTER */
            serial_init(config);
            print("Applied!");
            newline();

            struct iocs_time start = _iocs_ontime();
            while (1) {
                struct iocs_time now = _iocs_ontime();
                if (now.sec - start.sec >= 100) break;
            }
        }
    }
}

/* 데모 6: 버퍼 상태 모니터 */
static void demo_buffer_status(void)
{
    int config = SERIAL_9600_8N1;
    char config_str[32];
    int rx_count = 0;
    int tx_count = 0;

    print("=== Demo 6: Buffer Status Monitor ===");
    newline();

    config_to_string(config, config_str);
    print("Config: ");
    print(config_str);
    newline();
    print("Press ESC to exit, SPACE to send test data");
    newline();
    newline();

    serial_init(config);

    while (1) {
        _dos_c_locate(0, 5);

        /* 송신 상태 */
        print("TX Status: ");
        if (_iocs_osns232c()) {
            print("Ready   ");
        } else {
            print("Busy    ");
        }
        newline();

        /* 수신 상태 */
        print("RX Status: ");
        if (_iocs_isns232c()) {
            print("Data available");
        } else {
            print("Empty         ");
        }
        newline();

        /* 수신 버퍼 길이 */
        print("RX Buffer: ");
        print_num(serial_rx_length());
        print(" bytes    ");
        newline();

        newline();
        print("TX Count: ");
        print_num(tx_count);
        print("    ");
        newline();
        print("RX Count: ");
        print_num(rx_count);
        print("    ");
        newline();

        /* 수신 처리 */
        while (serial_available()) {
            serial_getc_nb();
            rx_count++;
        }

        /* 키 입력 */
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            int scan = (key >> 8) & 0xFF;

            if (scan == 0x01) break;    /* ESC */
            if (scan == 0x35) {         /* SPACE */
                serial_puts("Test data from X68000\r\n");
                tx_count += 23;
            }
        }

        /* 짧은 대기 */
        {
            volatile int j;
            for (j = 0; j < 10000; j++) { }
        }
    }
}

/* 데모 7: 대용량 전송 테스트 */
static void demo_bulk_transfer(void)
{
    int config = SERIAL_38400_8N1;
    char config_str[32];
    int i;
    int size = 1024;
    int start_time, end_time;
    int elapsed;

    print("=== Demo 7: Bulk Transfer Test ===");
    newline();

    config_to_string(config, config_str);
    print("Config: ");
    print(config_str);
    newline();
    print("Transfer size: ");
    print_num(size);
    print(" bytes");
    newline();
    newline();

    serial_init(config);

    print("Starting transfer...");
    newline();

    /* 시간 측정 시작 */
    {
        struct iocs_time t = _iocs_ontime();
        start_time = t.sec;
    }

    /* 데이터 전송 */
    for (i = 0; i < size; i++) {
        serial_putc((unsigned char)(i & 0xFF));
    }

    /* 시간 측정 종료 */
    {
        struct iocs_time t = _iocs_ontime();
        end_time = t.sec;
    }

    elapsed = end_time - start_time;
    if (elapsed < 0) elapsed += 8640000;

    print("Transfer complete!");
    newline();
    newline();

    print("Results:");
    newline();
    print("  Bytes sent: ");
    print_num(size);
    newline();
    print("  Time: ");
    print_num(elapsed);
    print(" (1/100 sec)");
    newline();

    if (elapsed > 0) {
        /* 전송률 계산 (bytes/sec) */
        long bps = ((long)size * 100) / elapsed;
        print("  Throughput: ");
        print_num((int)bps);
        print(" bytes/sec");
        newline();

        /* 이론적 최대값과 비교 */
        print("  Theoretical max: ~3840 bytes/sec at 38400 baud");
        newline();
    }
    newline();
}

int main(void)
{
    /* 화면 클리어 */
    _dos_c_cls_al();

    print("========================================");
    newline();
    print("  X68000 Serial Communication Demo");
    newline();
    print("========================================");
    newline();
    newline();

    print("RS-232C Features:");
    newline();
    print("- 75 to 38400 baud");
    newline();
    print("- 5/6/7/8 data bits");
    newline();
    print("- None/Odd/Even parity");
    newline();
    print("- 1/1.5/2 stop bits");
    newline();
    print("- Hardware/Software flow control");
    newline();
    newline();

    print("Press any key to start...");
    newline();
    _iocs_b_keyinp();

    /* 데모 실행 */
    _dos_c_cls_al();
    demo_basic_io();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_loopback();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_baudrate();

    _dos_c_cls_al();
    demo_data_format();

    _dos_c_cls_al();
    demo_buffer_status();

    _dos_c_cls_al();
    demo_bulk_transfer();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_terminal();

    /* 기본 설정으로 복원 */
    serial_init(SERIAL_9600_8N1);

    /* 종료 */
    _dos_c_cls_al();
    print("Serial communication demo complete!");
    newline();
    print("Press any key to exit...");
    newline();
    _iocs_b_keyinp();

    return 0;
}
