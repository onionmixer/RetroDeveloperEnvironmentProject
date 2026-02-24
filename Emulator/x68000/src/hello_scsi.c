/*
 * hello_scsi.c - X68000 SCSI Hard Disk Access Example
 *
 * This example demonstrates SCSI operations:
 * - SCSI device detection
 * - Device inquiry (vendor, product info)
 * - Read capacity
 * - Sector reading
 * - SCSI register access
 *
 * Build: m68k-xelf-gcc -O2 -o build/hello_scsi.x src/hello_scsi.c
 *
 * WARNING: Direct disk access can destroy data!
 * This example only performs READ operations for safety.
 */

#include <x68k/iocs.h>
#include <x68k/dos.h>
#include <string.h>

/*
 * X68000 Internal SCSI Controller (MB89352/SPC)
 * Address: 0xEA0000
 */
#define SCSI_BASE       0xEA0000

/* SPC (SCSI Protocol Controller) Registers */
#define SPC_BDID        (*(volatile unsigned char *)(SCSI_BASE + 0x01))  /* Bus Device ID */
#define SPC_SCTL        (*(volatile unsigned char *)(SCSI_BASE + 0x03))  /* SPC Control */
#define SPC_SCMD        (*(volatile unsigned char *)(SCSI_BASE + 0x05))  /* Command */
#define SPC_INTS        (*(volatile unsigned char *)(SCSI_BASE + 0x09))  /* Interrupt Sense */
#define SPC_PSNS        (*(volatile unsigned char *)(SCSI_BASE + 0x0B))  /* Phase Sense */
#define SPC_SSTS        (*(volatile unsigned char *)(SCSI_BASE + 0x0D))  /* SPC Status */
#define SPC_SERR        (*(volatile unsigned char *)(SCSI_BASE + 0x0F))  /* SPC Error */
#define SPC_PCTL        (*(volatile unsigned char *)(SCSI_BASE + 0x11))  /* Phase Control */
#define SPC_MBC         (*(volatile unsigned char *)(SCSI_BASE + 0x13))  /* Modified Byte Counter */
#define SPC_DREG        (*(volatile unsigned char *)(SCSI_BASE + 0x15))  /* Data Register */
#define SPC_TEMP        (*(volatile unsigned char *)(SCSI_BASE + 0x17))  /* Temporary */
#define SPC_TCH         (*(volatile unsigned char *)(SCSI_BASE + 0x19))  /* Transfer Counter High */
#define SPC_TCM         (*(volatile unsigned char *)(SCSI_BASE + 0x1B))  /* Transfer Counter Mid */
#define SPC_TCL         (*(volatile unsigned char *)(SCSI_BASE + 0x1D))  /* Transfer Counter Low */

/* SCSI Bus Phases */
#define PHASE_DATAOUT   0x00
#define PHASE_DATAIN    0x01
#define PHASE_COMMAND   0x02
#define PHASE_STATUS    0x03
#define PHASE_MSGOUT    0x06
#define PHASE_MSGIN     0x07

/* SCSI Commands */
#define SCSI_TEST_UNIT_READY    0x00
#define SCSI_REQUEST_SENSE      0x03
#define SCSI_INQUIRY            0x12
#define SCSI_READ_CAPACITY      0x25
#define SCSI_READ_10            0x28

/* Sector size */
#define SECTOR_SIZE     512

/* Buffers */
static unsigned char sector_buffer[SECTOR_SIZE];
static unsigned char sense_buffer[18];

/* Inquiry buffer with extra space for vendor/product data */
static struct {
    struct iocs_inquiry hdr;
    unsigned char data[96];
} inquiry_data;

/* Capacity data */
static struct iocs_readcap capacity_data;

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

static void print_num_unsigned(unsigned long num)
{
    char buf[12];
    int i = 0;

    if (num == 0) {
        buf[i++] = '0';
    } else {
        while (num > 0) {
            buf[i++] = '0' + (num % 10);
            num /= 10;
        }
    }

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

static void print_hex_word(unsigned short val)
{
    print_hex_byte((val >> 8) & 0xFF);
    print_hex_byte(val & 0xFF);
}

static void print_hex_long(unsigned long val)
{
    print_hex_word((val >> 16) & 0xFFFF);
    print_hex_word(val & 0xFFFF);
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
 * SCSI mode parameter
 * bits 0-2: SCSI ID (0-7)
 * bits 3-4: LUN (usually 0)
 */
static int make_scsi_mode(int id, int lun)
{
    return (lun << 3) | (id & 0x07);
}

/*
 * Demo 1: SCSI Device Scan
 */
static void demo_device_scan(void)
{
    int id;
    int result;
    unsigned char *buf;

    clear_screen();
    print("=== Demo 1: SCSI Device Scan ===\n\n");

    print("Scanning SCSI bus for devices...\n");
    print("(Internal SCSI: ID 0-7)\n\n");

    print("ID  Status      Type        Vendor/Model\n");
    print("--  ------      ----        ------------\n");

    for (id = 0; id < 8; id++) {
        print(" ");
        print_num(id);
        print("  ");

        /* Try to read device inquiry using IOCS */
        memset(&inquiry_data, 0, sizeof(inquiry_data));

        /* S_INQUIRY: SCSI Inquiry command via IOCS */
        result = _iocs_s_inquiry(make_scsi_mode(id, 0), 36, &inquiry_data.hdr);

        if (result >= 0) {
            print("Found       ");

            buf = (unsigned char *)&inquiry_data.hdr;

            /* Device type (byte 0, bits 0-4) */
            switch (buf[0] & 0x1F) {
                case 0x00: print("Disk        "); break;
                case 0x01: print("Tape        "); break;
                case 0x02: print("Printer     "); break;
                case 0x03: print("Processor   "); break;
                case 0x04: print("WORM        "); break;
                case 0x05: print("CD-ROM      "); break;
                case 0x06: print("Scanner     "); break;
                case 0x07: print("Optical     "); break;
                case 0x08: print("Changer     "); break;
                case 0x0C: print("RAID        "); break;
                case 0x1F: print("Unknown     "); break;
                default:   print("Other       "); break;
            }

            /* Vendor (bytes 8-15) and Product (bytes 16-31) */
            {
                int i;
                for (i = 8; i < 32; i++) {
                    if (buf[i] >= 0x20 && buf[i] < 0x7F) {
                        putchar_at(buf[i]);
                    } else if (buf[i] == 0) {
                        break;
                    }
                }
            }
        } else {
            print("Not found");
        }
        print("\n");
    }

    print("\n");
    print("Note: ID 7 is usually the host adapter\n");

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 2: Device Inquiry Details
 */
static void demo_inquiry_details(void)
{
    int id;
    int result;
    int i;
    unsigned char *buf;

    clear_screen();
    print("=== Demo 2: Device Inquiry Details ===\n\n");

    /* Find first available device */
    for (id = 0; id < 7; id++) {
        memset(&inquiry_data, 0, sizeof(inquiry_data));
        result = _iocs_s_inquiry(make_scsi_mode(id, 0), 96, &inquiry_data.hdr);
        if (result >= 0) break;
    }

    if (id >= 7) {
        print("No SCSI device found!\n");
        print("\nPress any key...");
        wait_key();
        return;
    }

    buf = (unsigned char *)&inquiry_data.hdr;

    print("Device at SCSI ID ");
    print_num(id);
    print(":\n\n");

    /* Device type */
    print("Device Type:     ");
    print_hex_byte(buf[0]);
    print(" - ");
    switch (buf[0] & 0x1F) {
        case 0x00: print("Direct Access (HDD)"); break;
        case 0x05: print("CD-ROM/DVD"); break;
        default:   print("Other"); break;
    }
    print("\n");

    /* Removable media */
    print("Removable:       ");
    print((buf[1] & 0x80) ? "Yes" : "No");
    print("\n");

    /* SCSI version */
    print("SCSI Version:    ");
    switch (buf[2] & 0x07) {
        case 0: print("SCSI-1"); break;
        case 1: print("CCS"); break;
        case 2: print("SCSI-2"); break;
        case 3: print("SPC (SCSI-3)"); break;
        default: print("Unknown"); break;
    }
    print("\n");

    /* Response format */
    print("Response Format: ");
    print_num(buf[3] & 0x0F);
    print("\n");

    /* Additional length */
    print("Additional Len:  ");
    print_num(buf[4]);
    print(" bytes\n");

    /* Vendor ID (bytes 8-15) */
    print("Vendor:          ");
    for (i = 8; i < 16; i++) {
        if (buf[i] >= 0x20 && buf[i] < 0x7F) {
            putchar_at(buf[i]);
        }
    }
    print("\n");

    /* Product ID (bytes 16-31) */
    print("Product:         ");
    for (i = 16; i < 32; i++) {
        if (buf[i] >= 0x20 && buf[i] < 0x7F) {
            putchar_at(buf[i]);
        }
    }
    print("\n");

    /* Revision (bytes 32-35) */
    print("Revision:        ");
    for (i = 32; i < 36; i++) {
        if (buf[i] >= 0x20 && buf[i] < 0x7F) {
            putchar_at(buf[i]);
        }
    }
    print("\n");

    /* Raw hex dump of first 36 bytes */
    print("\nRaw Inquiry Data (first 36 bytes):\n");
    for (i = 0; i < 36; i++) {
        if (i > 0 && (i % 16) == 0) print("\n");
        print_hex_byte(buf[i]);
        putchar_at(' ');
    }
    print("\n");

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 3: Read Capacity
 */
static void demo_read_capacity(void)
{
    int id;
    int result;
    unsigned long last_lba;
    unsigned long block_size;
    unsigned long capacity_mb;
    unsigned char *buf;

    clear_screen();
    print("=== Demo 3: Read Capacity ===\n\n");

    /* Find first available disk device */
    for (id = 0; id < 7; id++) {
        memset(&inquiry_data, 0, sizeof(inquiry_data));
        result = _iocs_s_inquiry(make_scsi_mode(id, 0), 36, &inquiry_data.hdr);
        buf = (unsigned char *)&inquiry_data.hdr;
        if (result >= 0 && (buf[0] & 0x1F) == 0x00) break;
    }

    if (id >= 7) {
        print("No SCSI hard disk found!\n");
        print("\nPress any key...");
        wait_key();
        return;
    }

    print("Reading capacity from SCSI ID ");
    print_num(id);
    print("...\n\n");

    /* Read capacity using IOCS */
    memset(&capacity_data, 0, sizeof(capacity_data));
    result = _iocs_s_readcap(make_scsi_mode(id, 0), &capacity_data);

    if (result < 0) {
        print("Error reading capacity: ");
        print_num(result);
        print("\n");
        print("\nPress any key...");
        wait_key();
        return;
    }

    /* Get values from structure */
    last_lba = capacity_data.block;
    block_size = capacity_data.size;

    print("Disk Information:\n");
    print("-----------------\n\n");

    print("Last LBA:        ");
    print_num_unsigned(last_lba);
    print(" (0x");
    print_hex_long(last_lba);
    print(")\n");

    print("Total Blocks:    ");
    print_num_unsigned(last_lba + 1);
    print("\n");

    print("Block Size:      ");
    print_num_unsigned(block_size);
    print(" bytes\n");

    /* Calculate capacity */
    /* Avoid overflow: capacity = (blocks * block_size) / (1024 * 1024) */
    capacity_mb = ((last_lba + 1) / 1024) * (block_size / 1024);
    if (capacity_mb == 0) {
        capacity_mb = ((last_lba + 1) * block_size) / (1024 * 1024);
    }

    print("Total Capacity:  ");
    print_num_unsigned(capacity_mb);
    print(" MB\n");

    /* Show in other units */
    if (capacity_mb >= 1024) {
        print("                 ");
        print_num_unsigned(capacity_mb / 1024);
        print(".");
        print_num_unsigned((capacity_mb % 1024) * 10 / 1024);
        print(" GB\n");
    }

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 4: Read Sector
 */
static void demo_read_sector(void)
{
    int id;
    int result;
    int offset, i;
    unsigned char *buf;

    clear_screen();
    print("=== Demo 4: Read Sector (LBA 0) ===\n\n");

    /* Find first available disk device */
    for (id = 0; id < 7; id++) {
        memset(&inquiry_data, 0, sizeof(inquiry_data));
        result = _iocs_s_inquiry(make_scsi_mode(id, 0), 36, &inquiry_data.hdr);
        buf = (unsigned char *)&inquiry_data.hdr;
        if (result >= 0 && (buf[0] & 0x1F) == 0x00) break;
    }

    if (id >= 7) {
        print("No SCSI hard disk found!\n");
        print("\nPress any key...");
        wait_key();
        return;
    }

    print("Reading sector 0 from SCSI ID ");
    print_num(id);
    print("...\n\n");

    /* Read sector using IOCS S_READ */
    /* _iocs_s_read(lba_high, lba_low, mode, count, buffer) */
    memset(sector_buffer, 0, sizeof(sector_buffer));
    result = _iocs_s_read(0, 0, make_scsi_mode(id, 0), 1, sector_buffer);

    if (result < 0) {
        print("Error reading sector: ");
        print_num(result);
        print("\n");
        print("\nPress any key...");
        wait_key();
        return;
    }

    print("Sector 0 (Master Boot Record / Partition Table):\n\n");

    /* Display first 256 bytes */
    print("     00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
    print("     -----------------------------------------------\n");

    for (offset = 0; offset < 256; offset += 16) {
        print_hex_byte(offset);
        print(": ");

        for (i = 0; i < 16; i++) {
            print_hex_byte(sector_buffer[offset + i]);
            putchar_at(' ');
        }

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

    /* Check for X68000 partition signature */
    print("\nPartition Check:\n");
    if (sector_buffer[0] == 'X' && sector_buffer[1] == '6' &&
        sector_buffer[2] == '8' && sector_buffer[3] == 'K') {
        print("  X68000 SASI/SCSI format detected\n");
    } else if (sector_buffer[510] == 0x55 && sector_buffer[511] == 0xAA) {
        print("  PC MBR signature detected (55 AA)\n");
    } else {
        print("  Unknown format or unpartitioned\n");
    }

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 5: Test Unit Ready
 */
static void demo_test_unit_ready(void)
{
    int id;
    int result;

    clear_screen();
    print("=== Demo 5: Test Unit Ready ===\n\n");

    print("Testing all SCSI IDs...\n\n");

    print("ID  Test Result\n");
    print("--  -----------\n");

    for (id = 0; id < 8; id++) {
        print(" ");
        print_num(id);
        print("  ");

        result = _iocs_s_testunit(make_scsi_mode(id, 0));

        if (result == 0) {
            print("Ready (unit is operational)\n");
        } else if (result > 0) {
            print("Check Condition - sense required\n");
        } else {
            print("Not ready or no device (");
            print_num(result);
            print(")\n");
        }
    }

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 6: SCSI Controller Registers
 */
static void demo_scsi_registers(void)
{
    clear_screen();
    print("=== Demo 6: SCSI Controller Registers ===\n\n");

    print("X68000 Internal SCSI (MB89352/SPC)\n");
    print("Base Address: 0xEA0000\n\n");

    print("Register Dump:\n");
    print("--------------\n\n");

    print("BDID (0x01): 0x");
    print_hex_byte(SPC_BDID);
    print("  - Bus Device ID\n");

    print("SCTL (0x03): 0x");
    print_hex_byte(SPC_SCTL);
    print("  - SPC Control\n");

    print("SCMD (0x05): 0x");
    print_hex_byte(SPC_SCMD);
    print("  - Command\n");

    print("INTS (0x09): 0x");
    print_hex_byte(SPC_INTS);
    print("  - Interrupt Sense\n");

    print("PSNS (0x0B): 0x");
    print_hex_byte(SPC_PSNS);
    print("  - Phase Sense\n");

    print("SSTS (0x0D): 0x");
    print_hex_byte(SPC_SSTS);
    print("  - SPC Status\n");

    print("SERR (0x0F): 0x");
    print_hex_byte(SPC_SERR);
    print("  - SPC Error\n");

    print("PCTL (0x11): 0x");
    print_hex_byte(SPC_PCTL);
    print("  - Phase Control\n");

    print("MBC  (0x13): 0x");
    print_hex_byte(SPC_MBC);
    print("  - Modified Byte Counter\n");

    print("TCH  (0x19): 0x");
    print_hex_byte(SPC_TCH);
    print("  - Transfer Counter High\n");

    print("TCM  (0x1B): 0x");
    print_hex_byte(SPC_TCM);
    print("  - Transfer Counter Mid\n");

    print("TCL  (0x1D): 0x");
    print_hex_byte(SPC_TCL);
    print("  - Transfer Counter Low\n");

    /* Decode PSNS (Phase Sense) */
    {
        unsigned char psns = SPC_PSNS;
        print("\nPhase Sense Decode:\n");
        print("  REQ=");
        print_num((psns >> 7) & 1);
        print(" ACK=");
        print_num((psns >> 6) & 1);
        print(" ATN=");
        print_num((psns >> 5) & 1);
        print(" SEL=");
        print_num((psns >> 4) & 1);
        print(" BSY=");
        print_num((psns >> 3) & 1);
        print("\n");

        print("  Current Phase: ");
        switch (psns & 0x07) {
            case 0: print("Data Out"); break;
            case 1: print("Data In"); break;
            case 2: print("Command"); break;
            case 3: print("Status"); break;
            case 6: print("Message Out"); break;
            case 7: print("Message In"); break;
            default: print("Unknown"); break;
        }
        print("\n");
    }

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 7: Request Sense
 */
static void demo_request_sense(void)
{
    int id;
    int result;

    clear_screen();
    print("=== Demo 7: Request Sense ===\n\n");

    /* Find first available device */
    for (id = 0; id < 7; id++) {
        memset(&inquiry_data, 0, sizeof(inquiry_data));
        result = _iocs_s_inquiry(make_scsi_mode(id, 0), 36, &inquiry_data.hdr);
        if (result >= 0) break;
    }

    if (id >= 7) {
        print("No SCSI device found!\n");
        print("\nPress any key...");
        wait_key();
        return;
    }

    print("Requesting sense data from SCSI ID ");
    print_num(id);
    print("...\n\n");

    memset(sense_buffer, 0, sizeof(sense_buffer));
    result = _iocs_s_request(make_scsi_mode(id, 0), 18, sense_buffer);

    if (result < 0) {
        print("Error: ");
        print_num(result);
        print("\n");
    } else {
        print("Sense Data:\n");
        print("-----------\n\n");

        /* Error code */
        print("Error Code:    0x");
        print_hex_byte(sense_buffer[0] & 0x7F);
        if (sense_buffer[0] & 0x80) print(" (Valid)");
        print("\n");

        /* Sense key */
        print("Sense Key:     0x");
        print_hex_byte(sense_buffer[2] & 0x0F);
        print(" - ");
        switch (sense_buffer[2] & 0x0F) {
            case 0x0: print("No Sense"); break;
            case 0x1: print("Recovered Error"); break;
            case 0x2: print("Not Ready"); break;
            case 0x3: print("Medium Error"); break;
            case 0x4: print("Hardware Error"); break;
            case 0x5: print("Illegal Request"); break;
            case 0x6: print("Unit Attention"); break;
            case 0x7: print("Data Protect"); break;
            case 0xB: print("Aborted Command"); break;
            default:  print("Other"); break;
        }
        print("\n");

        /* Additional sense code */
        print("ASC:           0x");
        print_hex_byte(sense_buffer[12]);
        print("\n");

        /* Additional sense code qualifier */
        print("ASCQ:          0x");
        print_hex_byte(sense_buffer[13]);
        print("\n");

        /* Raw hex dump */
        print("\nRaw Sense Data:\n");
        {
            int i;
            for (i = 0; i < 18; i++) {
                print_hex_byte(sense_buffer[i]);
                putchar_at(' ');
            }
        }
        print("\n");
    }

    print("\nPress any key...");
    wait_key();
}

/*
 * Demo 8: SCSI Information
 */
static void demo_scsi_info(void)
{
    clear_screen();
    print("=== Demo 8: X68000 SCSI Information ===\n\n");

    print("X68000 SCSI System:\n");
    print("-------------------\n\n");

    print("Internal SCSI Controller:\n");
    print("  Chip:     MB89352 (SPC)\n");
    print("  Address:  0xEA0000\n");
    print("  IRQ:      Level 1 (SCSI)\n");
    print("  DMA:      Channel 0\n");
    print("  Host ID:  7 (default)\n\n");

    print("Supported Devices:\n");
    print("  - Hard Disks (40MB - 2GB typical)\n");
    print("  - CD-ROM Drives\n");
    print("  - MO Drives (128MB, 230MB, 640MB)\n");
    print("  - Tape Drives\n");
    print("  - Scanners\n\n");

    print("SCSI Commands via IOCS:\n");
    print("  _iocs_s_inquiry()  - Get device info\n");
    print("  _iocs_s_testunit() - Check if ready\n");
    print("  _iocs_s_read()     - Read sectors\n");
    print("  _iocs_s_write()    - Write sectors\n");
    print("  _iocs_s_readcap()  - Get capacity\n");
    print("  _iocs_s_request()  - Request sense\n\n");

    print("Common SCSI IDs:\n");
    print("  ID 0: Primary HDD\n");
    print("  ID 1: Secondary HDD\n");
    print("  ID 2-5: Other devices\n");
    print("  ID 6: CD-ROM (common)\n");
    print("  ID 7: Host Adapter\n");

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
        print("=== X68000 SCSI Hard Disk Access Demo ===\n\n");

        print("This demo shows SCSI device access.\n");
        print("Read-only operations for safety.\n\n");

        print("Demo programs:\n");
        print("  1. SCSI Device Scan\n");
        print("  2. Device Inquiry Details\n");
        print("  3. Read Capacity\n");
        print("  4. Read Sector (LBA 0)\n");
        print("  5. Test Unit Ready\n");
        print("  6. SCSI Controller Registers\n");
        print("  7. Request Sense\n");
        print("  8. SCSI Information\n\n");

        print("  0. Exit\n\n");

        print("Select demo (0-8): ");

        demo = wait_key() - '0';

        switch (demo) {
            case 1: demo_device_scan(); break;
            case 2: demo_inquiry_details(); break;
            case 3: demo_read_capacity(); break;
            case 4: demo_read_sector(); break;
            case 5: demo_test_unit_ready(); break;
            case 6: demo_scsi_registers(); break;
            case 7: demo_request_sense(); break;
            case 8: demo_scsi_info(); break;
            case 0: running = 0; break;
            default: break;
        }
    }

    clear_screen();
    print("SCSI demo finished.\n");

    return 0;
}
