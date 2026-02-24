/*
 * hello_printer.c - X68000 Printer Output Example
 *
 * This example demonstrates parallel printer operations:
 * - Printer status checking
 * - Character output
 * - Text printing
 * - Control codes (CR, LF, FF)
 * - ESC/P command sequences
 *
 * Build: m68k-xelf-gcc -O2 -o build/hello_printer.x src/hello_printer.c
 *
 * X68000 Printer Port:
 * - Centronics parallel interface
 * - I/O address: 0xE8C000
 * - Compatible with EPSON ESC/P printers
 */

#include <x68k/iocs.h>
#include <x68k/dos.h>
#include <string.h>

/*
 * Printer Port Registers (directly accessed addresses)
 * X68000 uses memory-mapped I/O for printer
 */
#define PRN_BASE        0xE8C000
#define PRN_DATA        (*(volatile unsigned char *)(PRN_BASE + 0x01))  /* Data register */
#define PRN_STROBE      (*(volatile unsigned char *)(PRN_BASE + 0x03))  /* Strobe control */

/* Printer status bits (from IOCS) */
#define PRN_BUSY        0x01    /* Printer busy */
#define PRN_ONLINE      0x02    /* Printer online */
#define PRN_PAPEROUT    0x04    /* Paper out */
#define PRN_ERROR       0x08    /* General error */

/* Printer control codes */
#define PRN_CR          0x0D    /* Carriage Return */
#define PRN_LF          0x0A    /* Line Feed */
#define PRN_FF          0x0C    /* Form Feed (page eject) */
#define PRN_HT          0x09    /* Horizontal Tab */
#define PRN_BS          0x08    /* Backspace */
#define PRN_ESC         0x1B    /* Escape (for ESC/P commands) */
#define PRN_BEL         0x07    /* Bell */

/* Text output */
static int cursor_x = 0;
static int cursor_y = 0;

static void putchar_at(char c)
{
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= 31) cursor_y = 0;
        return;
    }
    _iocs_b_putc(c);
    cursor_x++;
    if (cursor_x >= 96) {
        cursor_x = 0;
        cursor_y++;
    }
}

static void print(const char *str)
{
    while (*str) {
        putchar_at(*str++);
    }
}

static void print_num(int num)
{
    char buf[12];
    int i = 0;
    int neg = 0;

    if (num < 0) {
        neg = 1;
        num = -num;
    }

    if (num == 0) {
        buf[i++] = '0';
    } else {
        while (num > 0) {
            buf[i++] = '0' + (num % 10);
            num /= 10;
        }
    }

    if (neg) buf[i++] = '-';

    while (i > 0) {
        putchar_at(buf[--i]);
    }
}

static void print_hex_byte(unsigned char val)
{
    const char hex[] = "0123456789ABCDEF";
    putchar_at(hex[(val >> 4) & 0x0F]);
    putchar_at(hex[val & 0x0F]);
}

static int wait_key(void)
{
    while (_iocs_b_keysns() == 0) { }
    return _iocs_b_keyinp() & 0xFF;
}

static void clear_screen(void)
{
    _iocs_b_clr_st();
    cursor_x = 0;
    cursor_y = 0;
    _iocs_b_locate(0, 0);
}

/*
 * Get printer status using IOCS
 * Returns: status bits or negative on error
 */
static int get_printer_status(void)
{
    return _iocs_snsprn();
}

/*
 * Send a single byte to printer using IOCS
 * Returns: 0 on success, negative on error
 */
static int printer_putc(int c)
{
    int status;
    int retry = 100;

    /* Wait for printer to be ready */
    while (retry > 0) {
        status = _iocs_snsprn();
        if (status == 0) {
            /* Printer ready */
            _iocs_outlpt(c);
            return 0;
        }
        if (status < 0) {
            /* Error */
            return status;
        }
        retry--;
    }

    /* Timeout - try anyway */
    _iocs_outlpt(c);
    return 0;
}

/*
 * Send a string to printer
 * Returns: number of bytes sent, or negative on error
 */
static int printer_puts(const char *str)
{
    int count = 0;
    int result;

    while (*str) {
        result = printer_putc(*str++);
        if (result < 0) {
            return (count > 0) ? count : result;
        }
        count++;
    }

    return count;
}

/*
 * Send control code to printer
 */
static int printer_control(int code)
{
    return printer_putc(code);
}

/*
 * Send newline (CR+LF) to printer
 */
static int printer_newline(void)
{
    int result;
    result = printer_putc(PRN_CR);
    if (result < 0) return result;
    return printer_putc(PRN_LF);
}

/*
 * Send form feed (page eject) to printer
 */
static int printer_formfeed(void)
{
    return printer_putc(PRN_FF);
}

/*
 * Send ESC/P command sequence
 */
static int printer_escp(const char *cmd, int len)
{
    int i;
    int result;

    result = printer_putc(PRN_ESC);
    if (result < 0) return result;

    for (i = 0; i < len; i++) {
        result = printer_putc(cmd[i]);
        if (result < 0) return result;
    }

    return 0;
}

/*
 * Demo 1: Printer Status Check
 */
static void demo_status_check(void)
{
    int status;

    clear_screen();
    print("=== Demo 1: Printer Status Check ===\n\n");

    print("Checking printer status...\n\n");

    status = get_printer_status();

    print("IOCS snsprn() result: ");
    print_num(status);
    print(" (0x");
    print_hex_byte(status & 0xFF);
    print(")\n\n");

    if (status < 0) {
        print("Error: Cannot access printer port\n");
    } else {
        print("Status Decode:\n");
        print("--------------\n");

        if (status == 0) {
            print("  Printer is READY\n");
        } else {
            if (status & PRN_BUSY) {
                print("  [BUSY] Printer is busy\n");
            }
            if (!(status & PRN_ONLINE)) {
                print("  [OFFLINE] Printer is offline\n");
            }
            if (status & PRN_PAPEROUT) {
                print("  [PAPER OUT] No paper\n");
            }
            if (status & PRN_ERROR) {
                print("  [ERROR] Printer error\n");
            }
        }
    }

    print("\n");
    print("Printer port address: 0xE8C000\n");
    print("Interface: Centronics parallel\n");

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 2: Print Test Characters
 */
static void demo_print_chars(void)
{
    int result;
    int i;
    int status;

    clear_screen();
    print("=== Demo 2: Print Test Characters ===\n\n");

    /* Check printer status first */
    status = get_printer_status();
    if (status != 0 && status > 0) {
        print("Warning: Printer may not be ready\n");
        print("Status: ");
        print_num(status);
        print("\n\n");
    }

    print("Sending test characters to printer...\n\n");

    /* Print alphabet */
    print("Printing: ABC...XYZ\n");
    for (i = 'A'; i <= 'Z'; i++) {
        result = printer_putc(i);
        if (result < 0) {
            print("Error at '");
            putchar_at(i);
            print("': ");
            print_num(result);
            print("\n");
            break;
        }
    }

    if (result >= 0) {
        printer_newline();
        print("Alphabet sent successfully!\n\n");
    }

    /* Print numbers */
    print("Printing: 0123456789\n");
    for (i = '0'; i <= '9'; i++) {
        result = printer_putc(i);
        if (result < 0) {
            print("Error: ");
            print_num(result);
            print("\n");
            break;
        }
    }

    if (result >= 0) {
        printer_newline();
        print("Numbers sent successfully!\n\n");
    }

    /* Print symbols */
    print("Printing: !@#$%^&*()\n");
    result = printer_puts("!@#$%^&*()");
    if (result > 0) {
        printer_newline();
        print("Symbols sent: ");
        print_num(result);
        print(" bytes\n");
    } else {
        print("Error: ");
        print_num(result);
        print("\n");
    }

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 3: Print Text Message
 */
static void demo_print_text(void)
{
    int result;

    clear_screen();
    print("=== Demo 3: Print Text Message ===\n\n");

    print("Sending text message to printer...\n\n");

    /* Print header */
    result = printer_puts("================================");
    printer_newline();

    result = printer_puts("    X68000 Printer Test Page    ");
    printer_newline();

    result = printer_puts("================================");
    printer_newline();
    printer_newline();

    /* Print body text */
    result = printer_puts("Hello from X68000!");
    printer_newline();

    result = printer_puts("This is a test print from the");
    printer_newline();

    result = printer_puts("hello_printer.c example program.");
    printer_newline();
    printer_newline();

    result = printer_puts("System: SHARP X68000");
    printer_newline();

    result = printer_puts("Printer: Centronics Parallel");
    printer_newline();
    printer_newline();

    /* Print ruler */
    result = printer_puts("0----+----1----+----2----+----3");
    printer_newline();

    /* Print footer */
    result = printer_puts("================================");
    printer_newline();

    if (result >= 0) {
        print("Text message sent to printer!\n");
    } else {
        print("Error sending text: ");
        print_num(result);
        print("\n");
    }

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 4: Printer Control Codes
 */
static void demo_control_codes(void)
{
    int result;

    clear_screen();
    print("=== Demo 4: Printer Control Codes ===\n\n");

    print("Common Printer Control Codes:\n\n");

    print("  0x0D (CR)  - Carriage Return\n");
    print("  0x0A (LF)  - Line Feed\n");
    print("  0x0C (FF)  - Form Feed (page eject)\n");
    print("  0x09 (HT)  - Horizontal Tab\n");
    print("  0x08 (BS)  - Backspace\n");
    print("  0x1B (ESC) - Escape (for commands)\n");
    print("  0x07 (BEL) - Bell\n\n");

    print("Sending test with control codes...\n\n");

    /* Line 1 */
    result = printer_puts("Line 1: Normal text");
    printer_newline();

    /* Line 2 with tab */
    result = printer_puts("Line 2:");
    printer_control(PRN_HT);
    result = printer_puts("After TAB");
    printer_newline();

    /* Line 3 */
    result = printer_puts("Line 3: End of control test");
    printer_newline();
    printer_newline();

    if (result >= 0) {
        print("Control codes sent!\n");
    } else {
        print("Error: ");
        print_num(result);
        print("\n");
    }

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 5: ESC/P Commands
 */
static void demo_escp_commands(void)
{
    int result;

    clear_screen();
    print("=== Demo 5: ESC/P Commands ===\n\n");

    print("Common ESC/P Commands:\n\n");
    print("  ESC @     - Initialize printer\n");
    print("  ESC E     - Bold ON\n");
    print("  ESC F     - Bold OFF\n");
    print("  ESC 4     - Italic ON\n");
    print("  ESC 5     - Italic OFF\n");
    print("  ESC -1    - Underline ON\n");
    print("  ESC -0    - Underline OFF\n");
    print("  ESC M     - Elite (12 cpi)\n");
    print("  ESC P     - Pica (10 cpi)\n\n");

    print("Sending ESC/P test sequence...\n\n");

    /* Initialize printer */
    result = printer_escp("@", 1);
    print("Sent: ESC @ (Initialize)\n");

    /* Normal text */
    result = printer_puts("Normal text, ");

    /* Bold text */
    result = printer_escp("E", 1);  /* Bold ON */
    result = printer_puts("BOLD text, ");
    result = printer_escp("F", 1);  /* Bold OFF */

    /* Italic text */
    result = printer_escp("4", 1);  /* Italic ON */
    result = printer_puts("Italic text, ");
    result = printer_escp("5", 1);  /* Italic OFF */

    /* Normal again */
    result = printer_puts("Normal again.");
    printer_newline();
    printer_newline();

    /* Initialize to reset */
    result = printer_escp("@", 1);

    if (result >= 0) {
        print("ESC/P commands sent!\n");
        print("(Results depend on printer capability)\n");
    } else {
        print("Error: ");
        print_num(result);
        print("\n");
    }

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 6: Print ASCII Table
 */
static void demo_ascii_table(void)
{
    int result;
    int row, col;
    int ch;
    char buf[8];

    clear_screen();
    print("=== Demo 6: Print ASCII Table ===\n\n");

    print("Printing ASCII character table...\n\n");

    /* Header */
    result = printer_puts("ASCII Character Table (0x20-0x7F)");
    printer_newline();
    printer_puts("=================================");
    printer_newline();
    printer_newline();

    /* Column headers */
    printer_puts("     0 1 2 3 4 5 6 7 8 9 A B C D E F");
    printer_newline();
    printer_puts("    --------------------------------");
    printer_newline();

    /* Print table rows */
    for (row = 0x20; row < 0x80; row += 0x10) {
        /* Row header */
        buf[0] = '0';
        buf[1] = 'x';
        buf[2] = "0123456789ABCDEF"[(row >> 4) & 0x0F];
        buf[3] = ' ';
        buf[4] = '\0';
        printer_puts(buf);

        /* Characters in row */
        for (col = 0; col < 16; col++) {
            ch = row + col;
            if (ch < 0x7F) {
                printer_putc(ch);
            } else {
                printer_putc('.');
            }
            printer_putc(' ');
        }

        printer_newline();
    }

    printer_newline();

    if (result >= 0) {
        print("ASCII table sent to printer!\n");
    } else {
        print("Error: ");
        print_num(result);
        print("\n");
    }

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 7: Form Feed Test
 */
static void demo_form_feed(void)
{
    int result;
    char key;

    clear_screen();
    print("=== Demo 7: Form Feed (Page Eject) ===\n\n");

    print("This will eject the current page.\n");
    print("Make sure paper is loaded!\n\n");

    print("Press 'Y' to send Form Feed, any other key to skip: ");

    key = wait_key();
    putchar_at(key);
    print("\n\n");

    if (key == 'Y' || key == 'y') {
        result = printer_formfeed();

        if (result >= 0) {
            print("Form Feed sent - page should eject\n");
        } else {
            print("Error sending Form Feed: ");
            print_num(result);
            print("\n");
        }
    } else {
        print("Form Feed skipped.\n");
    }

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 8: Printer Information
 */
static void demo_printer_info(void)
{
    clear_screen();
    print("=== Demo 8: X68000 Printer Information ===\n\n");

    print("X68000 Printer Interface:\n");
    print("-------------------------\n\n");

    print("Port Type:    Centronics Parallel\n");
    print("I/O Address:  0xE8C000\n");
    print("Connector:    36-pin Amphenol\n\n");

    print("Signal Pinout (main signals):\n");
    print("  Pin 1:   /STROBE (output)\n");
    print("  Pin 2-9: DATA 0-7 (output)\n");
    print("  Pin 10:  /ACK (input)\n");
    print("  Pin 11:  BUSY (input)\n");
    print("  Pin 12:  PE (Paper End, input)\n");
    print("  Pin 13:  SELECT (input)\n");
    print("  Pin 32:  /ERROR (input)\n\n");

    print("IOCS Functions:\n");
    print("  _iocs_snsprn()  - Get status\n");
    print("  _iocs_outlpt()  - Output byte\n\n");

    print("Compatible Printers:\n");
    print("  - EPSON ESC/P series\n");
    print("  - NEC PC-PR series\n");
    print("  - Canon BJ series\n");
    print("  - Most Centronics printers\n\n");

    print("Common Paper Sizes:\n");
    print("  - A4 (210 x 297 mm)\n");
    print("  - B4 (257 x 364 mm)\n");
    print("  - Letter (8.5 x 11 inch)\n");

    print("\nPress any key...");
    wait_key();
}

/*
 * Main function
 */
int main(int argc, char *argv[])
{
    int demo = 0;
    int running = 1;

    (void)argc;
    (void)argv;

    _iocs_b_clr_st();
    _iocs_os_curon();

    while (running) {
        clear_screen();
        print("=== X68000 Printer Output Demo ===\n\n");

        print("This demo shows printer port access.\n");
        print("Connect a Centronics printer to test.\n\n");

        print("Demo programs:\n");
        print("  1. Printer Status Check\n");
        print("  2. Print Test Characters\n");
        print("  3. Print Text Message\n");
        print("  4. Printer Control Codes\n");
        print("  5. ESC/P Commands\n");
        print("  6. Print ASCII Table\n");
        print("  7. Form Feed (Page Eject)\n");
        print("  8. Printer Information\n\n");

        print("  0. Exit\n\n");

        print("Select demo (0-8): ");

        demo = wait_key() - '0';

        switch (demo) {
            case 1: demo_status_check(); break;
            case 2: demo_print_chars(); break;
            case 3: demo_print_text(); break;
            case 4: demo_control_codes(); break;
            case 5: demo_escp_commands(); break;
            case 6: demo_ascii_table(); break;
            case 7: demo_form_feed(); break;
            case 8: demo_printer_info(); break;
            case 0: running = 0; break;
            default: break;
        }
    }

    clear_screen();
    print("Printer demo finished.\n");

    return 0;
}
