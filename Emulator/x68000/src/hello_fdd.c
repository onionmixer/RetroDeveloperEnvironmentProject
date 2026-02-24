/*
 * hello_fdd.c - X68000 FDD (Floppy Disk Drive) Access Example
 *
 * This example demonstrates low-level floppy disk operations:
 * - Drive status checking
 * - Sector reading
 * - Track seeking
 * - Disk information display
 * - Sector ID reading
 * - Direct FDC register access
 *
 * Build: m68k-xelf-gcc -O2 -o build/hello_fdd.x src/hello_fdd.c
 *
 * WARNING: Direct disk access can destroy data!
 * This example only performs READ operations for safety.
 */

#include <x68k/iocs.h>
#include <x68k/dos.h>
#include <string.h>

/* FDC (uPD72065) Register Addresses */
#define FDC_BASE        0xE94000
#define FDC_STATUS      (*(volatile unsigned char *)(FDC_BASE + 0x00))  /* Main status register */
#define FDC_DATA        (*(volatile unsigned char *)(FDC_BASE + 0x02))  /* Data register */
#define FDC_DOR         (*(volatile unsigned char *)(0xE94004))         /* Digital output register */
#define FDC_DRVCTRL     (*(volatile unsigned char *)(0xE94006))         /* Drive control */

/* FDC Status Register Bits */
#define FDC_RQM         0x80    /* Request for Master */
#define FDC_DIO         0x40    /* Data I/O direction (1=read, 0=write) */
#define FDC_NDM         0x20    /* Non-DMA mode */
#define FDC_CB          0x10    /* Controller busy */
#define FDC_D3B         0x08    /* Drive 3 busy */
#define FDC_D2B         0x04    /* Drive 2 busy */
#define FDC_D1B         0x02    /* Drive 1 busy */
#define FDC_D0B         0x01    /* Drive 0 busy */

/* FDC Commands */
#define FDC_CMD_READ_DATA       0x46    /* Read Data */
#define FDC_CMD_READ_ID         0x4A    /* Read ID */
#define FDC_CMD_RECALIBRATE     0x07    /* Recalibrate */
#define FDC_CMD_SEEK            0x0F    /* Seek */
#define FDC_CMD_SENSE_INT       0x08    /* Sense Interrupt Status */
#define FDC_CMD_SENSE_DRV       0x04    /* Sense Drive Status */
#define FDC_CMD_SPECIFY         0x03    /* Specify */

/* Disk parameters */
#define SECTOR_SIZE     1024    /* X68000 standard: 1024 bytes/sector */
#define SECTORS_TRACK   8       /* 8 sectors per track (2HD) */
#define TRACKS_SIDE     77      /* 77 tracks per side (2HD) */
#define SIDES           2       /* Double-sided */

/* Drive/Media type codes for IOCS */
#define MEDIA_2HD       0x90    /* 2HD (1.2MB) - 77 tracks, 8 sectors, 1024 bytes */
#define MEDIA_2DD       0x70    /* 2DD (640KB) - 80 tracks, 8 sectors, 512 bytes */

/* Buffer for sector data */
static unsigned char sector_buffer[SECTOR_SIZE];

/* Text output position */
static int cursor_x = 0;
static int cursor_y = 0;

/* Print character at current position */
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

/* Print string */
static void print(const char *str)
{
    while (*str) {
        putchar_at(*str++);
    }
}

/* Print number */
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

/* Print hex byte */
static void print_hex_byte(unsigned char val)
{
    const char hex[] = "0123456789ABCDEF";
    putchar_at(hex[(val >> 4) & 0x0F]);
    putchar_at(hex[val & 0x0F]);
}

/* Print hex word */
static void print_hex_word(unsigned short val)
{
    print_hex_byte((val >> 8) & 0xFF);
    print_hex_byte(val & 0xFF);
}

/* Wait for key press */
static int wait_key(void)
{
    while (_iocs_b_keysns() == 0) { }
    return _iocs_b_keyinp() & 0xFF;
}

/* Clear screen */
static void clear_screen(void)
{
    _iocs_b_clr_st();
    cursor_x = 0;
    cursor_y = 0;
    _iocs_b_locate(0, 0);
}

/* Delay function */
static void delay_ms(int ms)
{
    int i;
    for (i = 0; i < ms; i++) {
        int j;
        for (j = 0; j < 1000; j++) {
            __asm__ volatile ("nop");
        }
    }
}

/*
 * IOCS B_READ parameter packing
 * mode = (media_type << 8) | drive_number
 * pos = (cylinder << 16) | (head << 8) | sector
 * len = number of sectors
 */
static int make_mode(int drive, int media)
{
    return (media << 8) | drive;
}

static int make_pos(int cylinder, int head, int sector)
{
    return (cylinder << 16) | (head << 8) | sector;
}

/*
 * Demo 1: Drive Status Check
 */
static void demo_drive_status(void)
{
    int drive;
    int status;

    clear_screen();
    print("=== Demo 1: Drive Status Check ===\n\n");

    print("Checking floppy drive status...\n\n");

    for (drive = 0; drive < 4; drive++) {
        print("Drive ");
        print_num(drive);
        print(": ");

        /* Check drive status using IOCS */
        /* _iocs_b_drvchk(drive, cmd) - cmd=0 for status check */
        status = _iocs_b_drvchk(drive, 0);

        if (status < 0) {
            print("Not present or error (");
            print_num(status);
            print(")\n");
        } else {
            print("Status=0x");
            print_hex_word(status);
            print("\n");

            /* Decode status bits */
            print("  ");
            if (status & 0x8000) print("[2HD] ");
            if (status & 0x4000) print("[2DD] ");
            if (status & 0x0040) print("[WriteProtect] ");
            if (status & 0x0020) print("[Ready] ");
            if (status & 0x0010) print("[Track0] ");
            print("\n");
        }
    }

    print("\n");

    /* Read FDC status register directly */
    print("FDC Main Status Register: 0x");
    print_hex_byte(FDC_STATUS);
    print("\n");

    /* Decode FDC status */
    {
        unsigned char fdc_stat = FDC_STATUS;
        print("  RQM=");
        print_num((fdc_stat & FDC_RQM) ? 1 : 0);
        print(" DIO=");
        print_num((fdc_stat & FDC_DIO) ? 1 : 0);
        print(" NDM=");
        print_num((fdc_stat & FDC_NDM) ? 1 : 0);
        print(" CB=");
        print_num((fdc_stat & FDC_CB) ? 1 : 0);
        print("\n");
    }

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 2: Read Disk Information (Boot Sector)
 */
static void demo_read_boot_sector(void)
{
    int drive = 0;
    int result;
    int i;
    int mode, pos;

    clear_screen();
    print("=== Demo 2: Read Boot Sector ===\n\n");

    print("Reading boot sector from Drive 0...\n");
    print("(Insert disk if not present)\n\n");

    /* Wait a moment */
    delay_ms(500);

    /* Read sector using IOCS B_READ */
    /* mode = (media << 8) | drive */
    /* pos = (cylinder << 16) | (head << 8) | sector */
    mode = make_mode(drive, MEDIA_2HD);
    pos = make_pos(0, 0, 1);  /* Cylinder 0, Head 0, Sector 1 */

    result = _iocs_b_read(mode, pos, 1, sector_buffer);

    if (result < 0) {
        print("Error reading sector! Code: ");
        print_num(result);
        print("\n\nPossible causes:\n");
        print("- No disk in drive\n");
        print("- Disk not formatted\n");
        print("- Wrong media type (try 2DD)\n");
        print("- Drive not ready\n");
    } else {
        print("Boot sector read successfully!\n\n");

        /* Display BPB (BIOS Parameter Block) information */
        print("--- Disk Information (BPB) ---\n");

        /* Jump instruction */
        print("Jump Code:      ");
        for (i = 0; i < 3; i++) {
            print_hex_byte(sector_buffer[i]);
            print(" ");
        }
        print("\n");

        /* OEM Name (bytes 3-10) */
        print("OEM Name:       ");
        for (i = 3; i < 11; i++) {
            if (sector_buffer[i] >= 0x20 && sector_buffer[i] < 0x7F) {
                putchar_at(sector_buffer[i]);
            } else {
                putchar_at('.');
            }
        }
        print("\n");

        /* Bytes per sector (bytes 11-12) */
        print("Bytes/Sector:   ");
        print_num(sector_buffer[11] | (sector_buffer[12] << 8));
        print("\n");

        /* Sectors per cluster (byte 13) */
        print("Sectors/Cluster:");
        print_num(sector_buffer[13]);
        print("\n");

        /* Reserved sectors (bytes 14-15) */
        print("Reserved Sects: ");
        print_num(sector_buffer[14] | (sector_buffer[15] << 8));
        print("\n");

        /* Number of FATs (byte 16) */
        print("Number of FATs: ");
        print_num(sector_buffer[16]);
        print("\n");

        /* Root entries (bytes 17-18) */
        print("Root Entries:   ");
        print_num(sector_buffer[17] | (sector_buffer[18] << 8));
        print("\n");

        /* Total sectors (bytes 19-20) */
        print("Total Sectors:  ");
        print_num(sector_buffer[19] | (sector_buffer[20] << 8));
        print("\n");

        /* Media descriptor (byte 21) */
        print("Media Desc:     0x");
        print_hex_byte(sector_buffer[21]);
        if (sector_buffer[21] == 0xFE) print(" (2HD)");
        else if (sector_buffer[21] == 0xFD) print(" (2DD)");
        print("\n");

        /* Sectors per FAT (bytes 22-23) */
        print("Sectors/FAT:    ");
        print_num(sector_buffer[22] | (sector_buffer[23] << 8));
        print("\n");

        /* Sectors per track (bytes 24-25) */
        print("Sectors/Track:  ");
        print_num(sector_buffer[24] | (sector_buffer[25] << 8));
        print("\n");

        /* Number of heads (bytes 26-27) */
        print("Number of Heads:");
        print_num(sector_buffer[26] | (sector_buffer[27] << 8));
        print("\n");
    }

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 3: Hex Dump of Sector
 */
static void demo_hex_dump(void)
{
    int drive = 0;
    int cylinder, head, sector;
    int result;
    int offset;
    int i;
    int mode, pos;

    clear_screen();
    print("=== Demo 3: Sector Hex Dump ===\n\n");

    /* Read from cylinder 0, head 0, sector 1 (boot sector) */
    cylinder = 0;
    head = 0;
    sector = 1;

    print("Reading C=");
    print_num(cylinder);
    print(" H=");
    print_num(head);
    print(" S=");
    print_num(sector);
    print(" from Drive ");
    print_num(drive);
    print("...\n\n");

    mode = make_mode(drive, MEDIA_2HD);
    pos = make_pos(cylinder, head, sector);

    result = _iocs_b_read(mode, pos, 1, sector_buffer);

    if (result < 0) {
        print("Read error: ");
        print_num(result);
        print("\n");
    } else {
        /* Display first 256 bytes */
        print("First 256 bytes:\n\n");
        print("     00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
        print("     -----------------------------------------------\n");

        for (offset = 0; offset < 256; offset += 16) {
            print_hex_byte(offset);
            print(": ");

            /* Hex dump */
            for (i = 0; i < 16; i++) {
                print_hex_byte(sector_buffer[offset + i]);
                putchar_at(' ');
            }

            /* ASCII dump */
            putchar_at('|');
            for (i = 0; i < 16; i++) {
                unsigned char c = sector_buffer[offset + i];
                if (c >= 0x20 && c < 0x7F) {
                    putchar_at(c);
                } else {
                    putchar_at('.');
                }
            }
            putchar_at('|');
            print("\n");
        }
    }

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 4: Track Seek Test
 */
static void demo_seek_test(void)
{
    int drive = 0;
    int track;
    int result;

    clear_screen();
    print("=== Demo 4: Track Seek Test ===\n\n");

    print("This demo seeks to various tracks on Drive 0.\n");
    print("You will hear the drive head moving.\n\n");

    /* First recalibrate */
    print("Recalibrating drive...\n");
    result = _iocs_b_recali(drive);
    if (result < 0) {
        print("Recalibrate failed: ");
        print_num(result);
        print("\n");
        print("\nPress any key...");
        wait_key();
        return;
    }
    print("Drive at track 0\n\n");

    delay_ms(500);

    /* Seek to various tracks */
    print("Seeking to tracks: ");

    for (track = 0; track <= 76; track += 19) {
        print_num(track);
        print(" ");

        result = _iocs_b_seek(drive, track);
        if (result < 0) {
            print("(Error) ");
        }

        delay_ms(300);
    }

    print("\n\nReturning to track 0...\n");
    _iocs_b_recali(drive);

    print("Done!\n");

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 5: Read Sector IDs
 */
static void demo_read_sector_id(void)
{
    int drive = 0;
    int head;
    int result;
    int i;
    unsigned char id_buffer[8];
    int mode;

    clear_screen();
    print("=== Demo 5: Read Sector IDs ===\n\n");

    print("Reading sector IDs from track 0...\n\n");

    /* Recalibrate first */
    _iocs_b_recali(drive);

    for (head = 0; head < 2; head++) {
        print("Head ");
        print_num(head);
        print(":\n");
        print("  Sector  C  H  R  N\n");
        print("  ------ -- -- -- --\n");

        /* B_READID: mode = (media << 8) | drive */
        /* Second param is head number */
        mode = make_mode(drive, MEDIA_2HD);

        /* Read IDs for each sector on the track */
        for (i = 0; i < 8; i++) {
            result = _iocs_b_readid(mode, head, id_buffer);

            if (result >= 0) {
                print("    ");
                print_num(i + 1);
                print("     ");
                print_hex_byte(id_buffer[0]);  /* Cylinder */
                print(" ");
                print_hex_byte(id_buffer[1]);  /* Head */
                print(" ");
                print_hex_byte(id_buffer[2]);  /* Record (sector) */
                print(" ");
                print_hex_byte(id_buffer[3]);  /* N (size code) */
                print("\n");
            } else {
                print("    ");
                print_num(i + 1);
                print("   (Read error: ");
                print_num(result);
                print(")\n");
                break;
            }
        }
        print("\n");
    }

    print("N value: 0=128B, 1=256B, 2=512B, 3=1024B\n");

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 6: FDC Register Dump
 */
static void demo_fdc_registers(void)
{
    unsigned char status;

    clear_screen();
    print("=== Demo 6: FDC Register Status ===\n\n");

    print("uPD72065 FDC (Floppy Disk Controller)\n");
    print("Base Address: 0xE94000\n\n");

    /* Read main status register */
    status = FDC_STATUS;

    print("Main Status Register (0xE94000): 0x");
    print_hex_byte(status);
    print("\n\n");

    print("Bit breakdown:\n");
    print("  Bit 7 (RQM) = ");
    print_num((status >> 7) & 1);
    print("  - Request for Master\n");

    print("  Bit 6 (DIO) = ");
    print_num((status >> 6) & 1);
    print("  - Data direction (1=FDC->CPU)\n");

    print("  Bit 5 (NDM) = ");
    print_num((status >> 5) & 1);
    print("  - Non-DMA mode\n");

    print("  Bit 4 (CB)  = ");
    print_num((status >> 4) & 1);
    print("  - Controller Busy\n");

    print("  Bit 3 (D3B) = ");
    print_num((status >> 3) & 1);
    print("  - Drive 3 Busy\n");

    print("  Bit 2 (D2B) = ");
    print_num((status >> 2) & 1);
    print("  - Drive 2 Busy\n");

    print("  Bit 1 (D1B) = ");
    print_num((status >> 1) & 1);
    print("  - Drive 1 Busy\n");

    print("  Bit 0 (D0B) = ");
    print_num(status & 1);
    print("  - Drive 0 Busy\n");

    print("\n");

    /* Drive control register */
    print("Drive Control (0xE94006): 0x");
    print_hex_byte(FDC_DRVCTRL);
    print("\n");

    print("\nFDC Commands:\n");
    print("  0x03 - Specify\n");
    print("  0x04 - Sense Drive Status\n");
    print("  0x07 - Recalibrate\n");
    print("  0x08 - Sense Interrupt\n");
    print("  0x0F - Seek\n");
    print("  0x46 - Read Data (MFM)\n");
    print("  0x4A - Read ID (MFM)\n");
    print("  0x4D - Format Track (MFM)\n");

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 7: Read Multiple Sectors
 */
static void demo_read_multiple(void)
{
    int drive = 0;
    int cylinder = 0;
    int head = 0;
    int sector;
    int result;
    int total_read = 0;
    int mode, pos;

    clear_screen();
    print("=== Demo 7: Read Track Sectors ===\n\n");

    print("Reading all sectors from track 0, head 0...\n\n");

    print("Sector  Result   First 8 bytes\n");
    print("------  ------   -------------\n");

    mode = make_mode(drive, MEDIA_2HD);

    for (sector = 1; sector <= 8; sector++) {
        print("  ");
        print_num(sector);
        print("     ");

        memset(sector_buffer, 0, SECTOR_SIZE);
        pos = make_pos(cylinder, head, sector);
        result = _iocs_b_read(mode, pos, 1, sector_buffer);

        if (result >= 0) {
            print("OK      ");

            /* Show first 8 bytes */
            {
                int i;
                for (i = 0; i < 8; i++) {
                    print_hex_byte(sector_buffer[i]);
                    putchar_at(' ');
                }
            }
            total_read++;
        } else {
            print("Error ");
            print_num(result);
        }
        print("\n");
    }

    print("\n");
    print_num(total_read);
    print(" sectors read successfully\n");
    print("Total bytes: ");
    print_num(total_read * SECTOR_SIZE);
    print(" (");
    print_num(total_read);
    print(" KB)\n");

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 8: Disk Capacity Calculator
 */
static void demo_disk_capacity(void)
{
    clear_screen();
    print("=== Demo 8: X68000 Disk Formats ===\n\n");

    print("2HD Format (1.2MB / 1232KB):\n");
    print("  Tracks/Side:     77\n");
    print("  Sides:           2\n");
    print("  Sectors/Track:   8\n");
    print("  Bytes/Sector:    1024\n");
    print("  Total Sectors:   ");
    print_num(77 * 2 * 8);
    print("\n");
    print("  Total Capacity:  ");
    print_num(77 * 2 * 8 * 1024 / 1024);
    print(" KB\n\n");

    print("2DD Format (640KB):\n");
    print("  Tracks/Side:     80\n");
    print("  Sides:           2\n");
    print("  Sectors/Track:   8\n");
    print("  Bytes/Sector:    512\n");
    print("  Total Sectors:   ");
    print_num(80 * 2 * 8);
    print("\n");
    print("  Total Capacity:  ");
    print_num(80 * 2 * 8 * 512 / 1024);
    print(" KB\n\n");

    print("2HC Format (1.44MB) [PC compatible]:\n");
    print("  Tracks/Side:     80\n");
    print("  Sides:           2\n");
    print("  Sectors/Track:   18\n");
    print("  Bytes/Sector:    512\n");
    print("  Total Sectors:   ");
    print_num(80 * 2 * 18);
    print("\n");
    print("  Total Capacity:  ");
    print_num(80 * 2 * 18 * 512 / 1024);
    print(" KB\n\n");

    print("Media Descriptor bytes:\n");
    print("  0xFE - 2HD (X68000 native)\n");
    print("  0xFD - 2DD 8-sector\n");
    print("  0xF9 - 2DD 9-sector / 2HC\n");
    print("  0xF0 - 2HC 18-sector\n");

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

    /* Suppress unused parameter warnings */
    (void)argc;
    (void)argv;

    /* Initialize display */
    _iocs_b_clr_st();
    _iocs_os_curon();

    while (running) {
        clear_screen();
        print("=== X68000 FDD (Floppy Disk) Access Demo ===\n\n");

        print("This demo shows low-level floppy disk access.\n");
        print("WARNING: Insert a non-important disk for testing!\n\n");

        print("Demo programs:\n");
        print("  1. Drive Status Check\n");
        print("  2. Read Boot Sector (BPB)\n");
        print("  3. Sector Hex Dump\n");
        print("  4. Track Seek Test\n");
        print("  5. Read Sector IDs\n");
        print("  6. FDC Register Status\n");
        print("  7. Read Track Sectors\n");
        print("  8. Disk Format Info\n\n");

        print("  0. Exit\n\n");

        print("Select demo (0-8): ");

        demo = wait_key() - '0';

        switch (demo) {
            case 1:
                demo_drive_status();
                break;
            case 2:
                demo_read_boot_sector();
                break;
            case 3:
                demo_hex_dump();
                break;
            case 4:
                demo_seek_test();
                break;
            case 5:
                demo_read_sector_id();
                break;
            case 6:
                demo_fdc_registers();
                break;
            case 7:
                demo_read_multiple();
                break;
            case 8:
                demo_disk_capacity();
                break;
            case 0:
                running = 0;
                break;
            default:
                break;
        }
    }

    clear_screen();
    print("FDD demo finished.\n");

    return 0;
}
