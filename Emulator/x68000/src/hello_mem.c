/*
 * X68000 Human68k - Memory Management Example
 * 메모리 관리 예제 (DOS)
 *
 * Human68k 메모리:
 * - 68000 24비트 주소 공간 (16MB)
 * - 메인 RAM: 1MB~12MB (모델에 따라)
 * - GVRAM, TVRAM, 스프라이트 RAM 등 별도 영역
 * - DOS 메모리 관리자를 통한 할당/해제
 */
#include <x68k/iocs.h>
#include <x68k/dos.h>
#include <string.h>

/* 메모리 블록 추적용 */
#define MAX_BLOCKS 10
static void *blocks[MAX_BLOCKS];
static int block_sizes[MAX_BLOCKS];
static int block_count = 0;

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

static void print_hex(unsigned long value, int digits)
{
    char hex[] = "0123456789ABCDEF";
    int i;
    for (i = (digits - 1) * 4; i >= 0; i -= 4) {
        print_char(hex[(value >> i) & 0xF]);
    }
}

/* 메모리 블록 정보 표시 */
static void show_blocks(void)
{
    int i;

    print("Allocated blocks: ");
    print_num(block_count);
    newline();

    for (i = 0; i < block_count; i++) {
        print("  Block ");
        print_num(i);
        print(": 0x");
        print_hex((unsigned long)blocks[i], 8);
        print(", ");
        print_num(block_sizes[i]);
        print(" bytes");
        newline();
    }
}

/* 데모 1: 기본 메모리 할당 */
static void demo_basic_alloc(void)
{
    void *ptr;
    int size = 1024;

    print("=== Demo 1: Basic Memory Allocation ===");
    newline();
    newline();

    print("Allocating ");
    print_num(size);
    print(" bytes...");
    newline();

    /* 메모리 할당 */
    ptr = _dos_malloc(size);

    if (ptr == NULL || (long)ptr < 0) {
        print("Allocation failed!");
        newline();
        return;
    }

    print("Success! Address: 0x");
    print_hex((unsigned long)ptr, 8);
    newline();

    /* 메모리에 데이터 쓰기 */
    print("Writing test data...");
    newline();
    memset(ptr, 0xAA, size);

    /* 메모리 읽기 확인 */
    unsigned char *p = (unsigned char *)ptr;
    int ok = 1;
    int i;
    for (i = 0; i < size; i++) {
        if (p[i] != 0xAA) {
            ok = 0;
            break;
        }
    }

    if (ok) {
        print("Memory verification: OK");
    } else {
        print("Memory verification: FAILED at offset ");
        print_num(i);
    }
    newline();

    /* 메모리 해제 */
    print("Freeing memory...");
    newline();
    _dos_mfree(ptr);
    print("Done.");
    newline();
    newline();
}

/* 데모 2: 여러 블록 할당 */
static void demo_multiple_alloc(void)
{
    int sizes[] = {256, 512, 1024, 2048, 4096};
    int count = sizeof(sizes) / sizeof(sizes[0]);
    int i;

    print("=== Demo 2: Multiple Allocations ===");
    newline();
    newline();

    block_count = 0;

    for (i = 0; i < count && i < MAX_BLOCKS; i++) {
        print("Allocating ");
        print_num(sizes[i]);
        print(" bytes... ");

        blocks[i] = _dos_malloc(sizes[i]);

        if (blocks[i] == NULL || (long)blocks[i] < 0) {
            print("FAILED");
            newline();
            break;
        }

        block_sizes[i] = sizes[i];
        block_count++;

        print("0x");
        print_hex((unsigned long)blocks[i], 8);
        newline();
    }

    newline();
    show_blocks();
    newline();

    /* 블록 사이의 간격 분석 */
    print("Block spacing analysis:");
    newline();
    for (i = 1; i < block_count; i++) {
        long gap = (long)blocks[i] - (long)blocks[i-1] - block_sizes[i-1];
        print("  Gap ");
        print_num(i - 1);
        print("->");
        print_num(i);
        print(": ");
        print_num((int)gap);
        print(" bytes (overhead)");
        newline();
    }

    newline();

    /* 역순으로 해제 */
    print("Freeing in reverse order...");
    newline();
    for (i = block_count - 1; i >= 0; i--) {
        _dos_mfree(blocks[i]);
        blocks[i] = NULL;
    }
    block_count = 0;
    print("Done.");
    newline();
    newline();
}

/* 데모 3: 큰 메모리 할당 */
static void demo_large_alloc(void)
{
    void *ptr;
    int size;
    int max_size = 0;

    print("=== Demo 3: Large Memory Allocation ===");
    newline();
    print("Finding maximum allocatable size...");
    newline();
    newline();

    /* 최대 할당 가능 크기 찾기 */
    for (size = 1024 * 1024; size >= 1024; size /= 2) {
        ptr = _dos_malloc(size);
        if (ptr != NULL && (long)ptr > 0) {
            max_size = size;
            _dos_mfree(ptr);
            break;
        }
    }

    if (max_size == 0) {
        print("Could not allocate any large block!");
        newline();
        return;
    }

    /* 더 정밀하게 찾기 */
    int low = max_size;
    int high = max_size * 2;

    while (high - low > 1024) {
        int mid = (low + high) / 2;
        ptr = _dos_malloc(mid);
        if (ptr != NULL && (long)ptr > 0) {
            low = mid;
            _dos_mfree(ptr);
        } else {
            high = mid;
        }
    }

    max_size = low;

    print("Maximum allocatable: ");
    print_num(max_size);
    print(" bytes (");
    print_num(max_size / 1024);
    print(" KB)");
    newline();

    /* 실제로 할당해보기 */
    print("Allocating max size...");
    newline();
    ptr = _dos_malloc(max_size);
    if (ptr != NULL && (long)ptr > 0) {
        print("Success at 0x");
        print_hex((unsigned long)ptr, 8);
        newline();

        /* 메모리 테스트 */
        print("Testing memory (first/last 256 bytes)...");
        newline();

        unsigned char *p = (unsigned char *)ptr;
        int i;

        /* 첫 부분 테스트 */
        for (i = 0; i < 256; i++) {
            p[i] = (unsigned char)i;
        }

        /* 끝 부분 테스트 */
        for (i = 0; i < 256; i++) {
            p[max_size - 256 + i] = (unsigned char)i;
        }

        /* 검증 */
        int ok = 1;
        for (i = 0; i < 256; i++) {
            if (p[i] != (unsigned char)i ||
                p[max_size - 256 + i] != (unsigned char)i) {
                ok = 0;
                break;
            }
        }

        print("Verification: ");
        print(ok ? "PASSED" : "FAILED");
        newline();

        _dos_mfree(ptr);
    }
    newline();
}

/* 데모 4: 메모리 블록 리사이즈 */
static void demo_resize(void)
{
    void *ptr;
    int orig_size = 1024;
    int new_size = 2048;
    int result;

    print("=== Demo 4: Memory Block Resize ===");
    newline();
    newline();

    /* 초기 할당 */
    print("Allocating ");
    print_num(orig_size);
    print(" bytes... ");

    ptr = _dos_malloc(orig_size);
    if (ptr == NULL || (long)ptr < 0) {
        print("FAILED");
        newline();
        return;
    }

    print("0x");
    print_hex((unsigned long)ptr, 8);
    newline();

    /* 데이터 쓰기 */
    memset(ptr, 0x55, orig_size);

    /* 리사이즈 시도 */
    print("Resizing to ");
    print_num(new_size);
    print(" bytes... ");

    result = _dos_setblock(ptr, new_size);

    if (result < 0) {
        print("FAILED (error ");
        print_num(result);
        print(")");
        newline();

        /* 실패시 새로 할당하고 복사 */
        print("Trying realloc workaround...");
        newline();

        void *new_ptr = _dos_malloc(new_size);
        if (new_ptr != NULL && (long)new_ptr > 0) {
            memcpy(new_ptr, ptr, orig_size);
            _dos_mfree(ptr);
            ptr = new_ptr;
            print("New address: 0x");
            print_hex((unsigned long)ptr, 8);
            newline();
        }
    } else {
        print("OK");
        newline();
    }

    /* 원본 데이터 검증 */
    unsigned char *p = (unsigned char *)ptr;
    int ok = 1;
    int i;
    for (i = 0; i < orig_size; i++) {
        if (p[i] != 0x55) {
            ok = 0;
            break;
        }
    }

    print("Original data preserved: ");
    print(ok ? "YES" : "NO");
    newline();

    _dos_mfree(ptr);
    newline();
}

/* 데모 5: 메모리 복사 */
static void demo_memcpy(void)
{
    void *src, *dst;
    int size = 4096;
    int i;

    print("=== Demo 5: Memory Copy ===");
    newline();
    newline();

    /* 소스 버퍼 할당 */
    src = _dos_malloc(size);
    if (src == NULL || (long)src < 0) {
        print("Failed to allocate source!");
        newline();
        return;
    }

    /* 대상 버퍼 할당 */
    dst = _dos_malloc(size);
    if (dst == NULL || (long)dst < 0) {
        print("Failed to allocate destination!");
        newline();
        _dos_mfree(src);
        return;
    }

    print("Source: 0x");
    print_hex((unsigned long)src, 8);
    newline();
    print("Dest:   0x");
    print_hex((unsigned long)dst, 8);
    newline();

    /* 소스에 패턴 쓰기 */
    unsigned char *s = (unsigned char *)src;
    for (i = 0; i < size; i++) {
        s[i] = (unsigned char)(i & 0xFF);
    }

    /* 대상 초기화 */
    memset(dst, 0, size);

    /* 복사 (DOS 함수 사용) */
    print("Copying ");
    print_num(size);
    print(" bytes using _dos_memcpy...");
    newline();

    _dos_memcpy(dst, src, size);

    /* 검증 */
    unsigned char *d = (unsigned char *)dst;
    int ok = 1;
    for (i = 0; i < size; i++) {
        if (d[i] != (unsigned char)(i & 0xFF)) {
            ok = 0;
            print("Mismatch at offset ");
            print_num(i);
            print(": expected ");
            print_num(i & 0xFF);
            print(", got ");
            print_num(d[i]);
            newline();
            break;
        }
    }

    print("Verification: ");
    print(ok ? "PASSED" : "FAILED");
    newline();

    _dos_mfree(src);
    _dos_mfree(dst);
    newline();
}

/* 데모 6: 메모리 맵 정보 */
static void demo_memory_map(void)
{
    print("=== Demo 6: Memory Map ===");
    newline();
    newline();

    /* X68000 메모리 맵 표시 */
    print("X68000 Memory Map:");
    newline();
    print("  0x000000-0x0BFFFF: Main RAM (up to 768KB)");
    newline();
    print("  0x0C0000-0x0DFFFF: Graphic VRAM (128KB)");
    newline();
    print("  0x0E0000-0x0E7FFF: Text VRAM (32KB)");
    newline();
    print("  0x0E8000-0x0EBFFF: CRTC/Video (16KB)");
    newline();
    print("  0x0EC0000-0x0ECFFFF: System I/O");
    newline();
    print("  0x0ED0000-0x0ED3FFF: Sprite RAM (16KB)");
    newline();
    print("  0x0F00000-0x0FBFFFF: CGROM (768KB)");
    newline();
    print("  0x0FC0000-0x0FFFFFF: IPLROM (256KB)");
    newline();
    print("  0x100000-0xBFFFFF: Extended RAM (12MB max)");
    newline();
    newline();

    /* 간단한 메모리 테스트 */
    print("Testing memory allocation...");
    newline();

    int test_sizes[] = {1024, 4096, 16384, 65536, 262144};
    int count = sizeof(test_sizes) / sizeof(test_sizes[0]);
    int i;

    for (i = 0; i < count; i++) {
        void *p = _dos_malloc(test_sizes[i]);
        print("  ");
        print_num(test_sizes[i] / 1024);
        print(" KB: ");
        if (p != NULL && (long)p > 0) {
            print("OK (0x");
            print_hex((unsigned long)p, 8);
            print(")");
            _dos_mfree(p);
        } else {
            print("FAILED");
        }
        newline();
    }
    newline();
}

/* 데모 7: 메모리 단편화 테스트 */
static void demo_fragmentation(void)
{
    void *ptrs[20];
    int i;
    int alloc_count = 0;

    print("=== Demo 7: Memory Fragmentation ===");
    newline();
    newline();

    /* 작은 블록 여러 개 할당 */
    print("Allocating 20 x 1KB blocks...");
    newline();

    for (i = 0; i < 20; i++) {
        ptrs[i] = _dos_malloc(1024);
        if (ptrs[i] != NULL && (long)ptrs[i] > 0) {
            alloc_count++;
        } else {
            break;
        }
    }

    print("Allocated: ");
    print_num(alloc_count);
    print(" blocks");
    newline();

    /* 짝수 블록 해제 (구멍 만들기) */
    print("Freeing even blocks (creating holes)...");
    newline();

    int freed = 0;
    for (i = 0; i < alloc_count; i += 2) {
        _dos_mfree(ptrs[i]);
        ptrs[i] = NULL;
        freed++;
    }

    print("Freed ");
    print_num(freed);
    print(" blocks");
    newline();

    /* 큰 블록 할당 시도 */
    print("Trying to allocate 3KB block...");
    newline();

    void *big = _dos_malloc(3072);
    if (big != NULL && (long)big > 0) {
        print("Success at 0x");
        print_hex((unsigned long)big, 8);
        newline();
        _dos_mfree(big);
    } else {
        print("Failed (fragmentation?)");
        newline();
    }

    /* 나머지 해제 */
    print("Cleaning up...");
    newline();
    for (i = 1; i < alloc_count; i += 2) {
        if (ptrs[i] != NULL) {
            _dos_mfree(ptrs[i]);
        }
    }

    print("Done.");
    newline();
    newline();
}

int main(void)
{
    /* 화면 클리어 */
    _dos_c_cls_al();

    print("========================================");
    newline();
    print("  X68000 Memory Management Demo");
    newline();
    print("========================================");
    newline();
    newline();

    print("This demo shows memory management.");
    newline();
    print("Press any key to start...");
    newline();
    _iocs_b_keyinp();

    /* 데모 실행 */
    _dos_c_cls_al();
    demo_basic_alloc();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_multiple_alloc();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_large_alloc();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_resize();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_memcpy();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_memory_map();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_fragmentation();
    print("Press any key...");
    _iocs_b_keyinp();

    /* 종료 */
    _dos_c_cls_al();
    print("Memory management demo complete!");
    newline();
    print("Press any key to exit...");
    newline();
    _iocs_b_keyinp();

    return 0;
}
