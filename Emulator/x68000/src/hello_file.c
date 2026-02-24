/*
 * X68000 Human68k - File I/O Example
 * 파일 입출력 예제 (DOS)
 *
 * Human68k 파일 시스템:
 * - MS-DOS 호환 FAT 파일 시스템
 * - 8.3 파일명 형식
 * - 드라이브 문자 (A:, B:, etc.)
 * - 디렉토리 지원
 */
#include <x68k/iocs.h>
#include <x68k/dos.h>
#include <string.h>

/* 파일 속성 */
#define ATTR_READONLY   0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME     0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20

/* 파일 열기 모드 */
#define MODE_READ       0
#define MODE_WRITE      1
#define MODE_READWRITE  2

/* Seek 기준 */
#define SEEK_SET        0
#define SEEK_CUR        1
#define SEEK_END        2

/* 테스트 파일명 */
#define TEST_FILE       "TEST.TXT"
#define TEST_FILE2      "TEST2.TXT"
#define TEST_DIR        "TESTDIR"

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

/* 에러 메시지 출력 */
static void print_error(int err)
{
    print("Error: ");
    print_num(err);
    print(" (0x");
    print_hex(err & 0xFFFF, 4);
    print(")");
    newline();
}

/* 데모 1: 파일 쓰기 */
static int demo_write_file(void)
{
    int handle;
    int result;
    const char *text = "Hello, X68000 File System!\r\n"
                       "This is a test file.\r\n"
                       "Written by hello_file.x\r\n";
    int len = strlen(text);

    print("=== Demo 1: Write File ===");
    newline();
    print("Creating and writing to ");
    print(TEST_FILE);
    newline();

    /* 파일 생성 */
    handle = _dos_create(TEST_FILE, 0);  /* 0 = 일반 파일 */
    if (handle < 0) {
        print("Failed to create file!");
        newline();
        print_error(handle);
        return -1;
    }

    print("File created. Handle: ");
    print_num(handle);
    newline();

    /* 파일 쓰기 */
    result = _dos_write(handle, text, len);
    if (result < 0) {
        print("Failed to write!");
        newline();
        print_error(result);
        _dos_close(handle);
        return -1;
    }

    print("Written ");
    print_num(result);
    print(" bytes.");
    newline();

    /* 파일 닫기 */
    _dos_close(handle);
    print("File closed.");
    newline();
    newline();

    return 0;
}

/* 데모 2: 파일 읽기 */
static int demo_read_file(void)
{
    int handle;
    int result;
    char buffer[256];
    int total = 0;

    print("=== Demo 2: Read File ===");
    newline();
    print("Reading from ");
    print(TEST_FILE);
    newline();

    /* 파일 열기 */
    handle = _dos_open(TEST_FILE, MODE_READ);
    if (handle < 0) {
        print("Failed to open file!");
        newline();
        print_error(handle);
        return -1;
    }

    print("File opened. Handle: ");
    print_num(handle);
    newline();
    print("Contents:");
    newline();
    print("---");
    newline();

    /* 파일 읽기 */
    while (1) {
        result = _dos_read(handle, buffer, sizeof(buffer) - 1);
        if (result <= 0) {
            break;
        }
        buffer[result] = '\0';
        print(buffer);
        total += result;
    }

    print("---");
    newline();
    print("Read ");
    print_num(total);
    print(" bytes total.");
    newline();

    /* 파일 닫기 */
    _dos_close(handle);
    newline();

    return 0;
}

/* 데모 3: 파일 추가 쓰기 */
static int demo_append_file(void)
{
    int handle;
    int result;
    const char *text = "Appended line!\r\n";
    int len = strlen(text);

    print("=== Demo 3: Append to File ===");
    newline();

    /* 파일 열기 (읽기/쓰기) */
    handle = _dos_open(TEST_FILE, MODE_READWRITE);
    if (handle < 0) {
        print("Failed to open file!");
        newline();
        print_error(handle);
        return -1;
    }

    /* 파일 끝으로 이동 */
    result = _dos_seek(handle, 0, SEEK_END);
    if (result < 0) {
        print("Failed to seek!");
        newline();
        _dos_close(handle);
        return -1;
    }

    print("File size: ");
    print_num(result);
    print(" bytes");
    newline();

    /* 추가 쓰기 */
    result = _dos_write(handle, text, len);
    print("Appended ");
    print_num(result);
    print(" bytes.");
    newline();

    _dos_close(handle);
    newline();

    return 0;
}

/* 데모 4: 파일 정보 */
static int demo_file_info(void)
{
    struct dos_filbuf filbuf;
    int result;

    print("=== Demo 4: File Information ===");
    newline();

    /* 파일 검색 */
    result = _dos_files(&filbuf, TEST_FILE, 0x3F);
    if (result < 0) {
        print("File not found!");
        newline();
        return -1;
    }

    print("File: ");
    print(filbuf.name);
    newline();

    print("Size: ");
    print_num(filbuf.filelen);
    print(" bytes");
    newline();

    print("Attr: 0x");
    print_hex(filbuf.atr, 2);
    print(" (");
    if (filbuf.atr & ATTR_READONLY) print("R");
    if (filbuf.atr & ATTR_HIDDEN) print("H");
    if (filbuf.atr & ATTR_SYSTEM) print("S");
    if (filbuf.atr & ATTR_DIRECTORY) print("D");
    if (filbuf.atr & ATTR_ARCHIVE) print("A");
    print(")");
    newline();

    print("Date: ");
    /* DOS 날짜 형식: YYYYYYYM MMMDDDDD */
    int date = filbuf.date;
    int year = ((date >> 9) & 0x7F) + 1980;
    int month = (date >> 5) & 0x0F;
    int day = date & 0x1F;
    print_num(year);
    print("-");
    if (month < 10) print_char('0');
    print_num(month);
    print("-");
    if (day < 10) print_char('0');
    print_num(day);
    newline();

    print("Time: ");
    /* DOS 시간 형식: HHHHHMMM MMMSSSSS */
    int time = filbuf.time;
    int hour = (time >> 11) & 0x1F;
    int min = (time >> 5) & 0x3F;
    int sec = (time & 0x1F) * 2;
    if (hour < 10) print_char('0');
    print_num(hour);
    print(":");
    if (min < 10) print_char('0');
    print_num(min);
    print(":");
    if (sec < 10) print_char('0');
    print_num(sec);
    newline();
    newline();

    return 0;
}

/* 데모 5: 디렉토리 목록 */
static int demo_directory(void)
{
    struct dos_filbuf filbuf;
    int result;
    int count = 0;
    char path[64];

    print("=== Demo 5: Directory Listing ===");
    newline();

    /* 현재 디렉토리 얻기 */
    result = _dos_curdir(0, path);
    if (result >= 0) {
        print("Current dir: ");
        print(path);
        newline();
    }

    print("Files in current directory:");
    newline();
    print("---");
    newline();

    /* 모든 파일 검색 */
    result = _dos_files(&filbuf, "*.*", 0x37);  /* 디렉토리 제외 */
    while (result >= 0) {
        /* 파일명 출력 */
        print("  ");
        print(filbuf.name);

        /* 크기 출력 (오른쪽 정렬 시뮬레이션) */
        int namelen = strlen(filbuf.name);
        int i;
        for (i = namelen; i < 14; i++) {
            print_char(' ');
        }

        if (filbuf.atr & ATTR_DIRECTORY) {
            print("<DIR>");
        } else {
            print_num(filbuf.filelen);
        }
        newline();

        count++;
        if (count >= 15) {
            print("  ... (more files)");
            newline();
            break;
        }

        /* 다음 파일 */
        result = _dos_nfiles(&filbuf);
    }

    print("---");
    newline();
    print("Total: ");
    print_num(count);
    print(" files shown.");
    newline();
    newline();

    return 0;
}

/* 데모 6: 파일 복사 */
static int demo_copy_file(void)
{
    int src_handle, dst_handle;
    int result;
    char buffer[512];
    int total = 0;

    print("=== Demo 6: Copy File ===");
    newline();
    print("Copying ");
    print(TEST_FILE);
    print(" to ");
    print(TEST_FILE2);
    newline();

    /* 소스 파일 열기 */
    src_handle = _dos_open(TEST_FILE, MODE_READ);
    if (src_handle < 0) {
        print("Failed to open source!");
        newline();
        return -1;
    }

    /* 대상 파일 생성 */
    dst_handle = _dos_create(TEST_FILE2, 0);
    if (dst_handle < 0) {
        print("Failed to create destination!");
        newline();
        _dos_close(src_handle);
        return -1;
    }

    /* 복사 */
    while (1) {
        result = _dos_read(src_handle, buffer, sizeof(buffer));
        if (result <= 0) {
            break;
        }

        result = _dos_write(dst_handle, buffer, result);
        if (result < 0) {
            print("Write error!");
            newline();
            break;
        }
        total += result;
    }

    _dos_close(src_handle);
    _dos_close(dst_handle);

    print("Copied ");
    print_num(total);
    print(" bytes.");
    newline();
    newline();

    return 0;
}

/* 데모 7: 파일 삭제 */
static int demo_delete_file(void)
{
    int result;

    print("=== Demo 7: Delete Files ===");
    newline();

    /* TEST2.TXT 삭제 */
    print("Deleting ");
    print(TEST_FILE2);
    print("... ");

    result = _dos_delete(TEST_FILE2);
    if (result < 0) {
        print("Failed!");
        newline();
        print_error(result);
    } else {
        print("OK");
        newline();
    }

    /* TEST.TXT 삭제 */
    print("Deleting ");
    print(TEST_FILE);
    print("... ");

    result = _dos_delete(TEST_FILE);
    if (result < 0) {
        print("Failed!");
        newline();
        print_error(result);
    } else {
        print("OK");
        newline();
    }

    newline();
    return 0;
}

/* 데모 8: 드라이브 정보 */
static int demo_drive_info(void)
{
    int cur_drive;
    struct dos_freeinf info;
    int result;

    print("=== Demo 8: Drive Information ===");
    newline();

    /* 현재 드라이브 얻기 */
    cur_drive = _dos_curdrv();
    print("Current drive: ");
    print_char('A' + cur_drive);
    print(":");
    newline();

    /* 여유 공간 정보 */
    result = _dos_dskfre(0, &info);
    if (result >= 0) {
        /* dos_freeinf: free, max, sec(sectors/cluster), byte(bytes/sector) */
        long cluster_size = (long)info.sec * info.byte;
        long free_bytes = (long)info.free * cluster_size;
        long total_bytes = (long)info.max * cluster_size;

        print("Cluster size: ");
        print_num((int)cluster_size);
        print(" bytes (");
        print_num(info.sec);
        print(" sectors x ");
        print_num(info.byte);
        print(" bytes)");
        newline();

        print("Free clusters: ");
        print_num(info.free);
        print(" / ");
        print_num(info.max);
        newline();

        print("Free space: ");
        print_num((int)(free_bytes / 1024));
        print(" KB");
        newline();

        print("Total space: ");
        print_num((int)(total_bytes / 1024));
        print(" KB");
        newline();
    }

    newline();
    return 0;
}

int main(void)
{
    /* 화면 클리어 */
    _dos_c_cls_al();

    print("========================================");
    newline();
    print("  X68000 File I/O Demo");
    newline();
    print("========================================");
    newline();
    newline();

    print("This demo shows file operations.");
    newline();
    print("Press any key to start...");
    newline();
    _iocs_b_keyinp();

    /* 데모 실행 */
    _dos_c_cls_al();

    /* 드라이브 정보 먼저 표시 */
    demo_drive_info();
    print("Press any key...");
    _iocs_b_keyinp();
    _dos_c_cls_al();

    /* 디렉토리 목록 */
    demo_directory();
    print("Press any key...");
    _iocs_b_keyinp();
    _dos_c_cls_al();

    /* 파일 쓰기 */
    if (demo_write_file() < 0) {
        print("Write demo failed. Press any key...");
        _iocs_b_keyinp();
    } else {
        print("Press any key...");
        _iocs_b_keyinp();
    }
    _dos_c_cls_al();

    /* 파일 읽기 */
    demo_read_file();
    print("Press any key...");
    _iocs_b_keyinp();
    _dos_c_cls_al();

    /* 파일 추가 쓰기 */
    demo_append_file();
    demo_read_file();
    print("Press any key...");
    _iocs_b_keyinp();
    _dos_c_cls_al();

    /* 파일 정보 */
    demo_file_info();
    print("Press any key...");
    _iocs_b_keyinp();
    _dos_c_cls_al();

    /* 파일 복사 */
    demo_copy_file();
    demo_directory();
    print("Press any key...");
    _iocs_b_keyinp();
    _dos_c_cls_al();

    /* 파일 삭제 */
    demo_delete_file();
    demo_directory();
    print("Press any key...");
    _iocs_b_keyinp();

    /* 종료 */
    _dos_c_cls_al();
    print("File I/O demo complete!");
    newline();
    print("Press any key to exit...");
    newline();
    _iocs_b_keyinp();

    return 0;
}
