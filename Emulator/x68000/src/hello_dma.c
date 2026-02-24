/*
 * X68000 Human68k - DMA Transfer Example
 * DMA 전송 예제
 *
 * X68000 DMA 컨트롤러:
 * - HD63450 (4채널)
 * - 메모리-메모리, 메모리-I/O 전송
 * - 버스트/사이클스틸 모드
 * - 자동 요청/외부 요청
 */
#include <x68k/iocs.h>
#include <x68k/dos.h>
#include <string.h>

/* DMAC 레지스터 베이스 */
#define DMAC_BASE       0xE84000

/* DMAC 채널 오프셋 (각 채널 0x40 간격) */
#define DMAC_CH(n)      (DMAC_BASE + (n) * 0x40)

/* DMAC 레지스터 오프셋 */
#define DMAC_CSR        0x00    /* Channel Status Register (byte) */
#define DMAC_CER        0x01    /* Channel Error Register (byte) */
#define DMAC_DCR        0x04    /* Device Control Register (byte) */
#define DMAC_OCR        0x05    /* Operation Control Register (byte) */
#define DMAC_SCR        0x06    /* Sequence Control Register (byte) */
#define DMAC_CCR        0x07    /* Channel Control Register (byte) */
#define DMAC_MTC        0x0A    /* Memory Transfer Counter (word) */
#define DMAC_MAR        0x0C    /* Memory Address Register (long) */
#define DMAC_DAR        0x14    /* Device Address Register (long) */
#define DMAC_BTC        0x1A    /* Base Transfer Counter (word) */
#define DMAC_BAR        0x1C    /* Base Address Register (long) */
#define DMAC_NIV        0x25    /* Normal Interrupt Vector (byte) */
#define DMAC_EIV        0x27    /* Error Interrupt Vector (byte) */
#define DMAC_MFC        0x29    /* Memory Function Code (byte) */
#define DMAC_CPR        0x2D    /* Channel Priority Register (byte) */
#define DMAC_DFC        0x31    /* Device Function Code (byte) */
#define DMAC_BFC        0x39    /* Base Function Code (byte) */
#define DMAC_GCR        0xFF    /* General Control Register (byte) */

/* DMAC 레지스터 액세스 매크로 */
#define DMAC_REG_B(ch, reg)  (*(volatile unsigned char *)(DMAC_CH(ch) + (reg)))
#define DMAC_REG_W(ch, reg)  (*(volatile unsigned short *)(DMAC_CH(ch) + (reg)))
#define DMAC_REG_L(ch, reg)  (*(volatile unsigned long *)(DMAC_CH(ch) + (reg)))
#define DMAC_GENERAL         (*(volatile unsigned char *)(DMAC_BASE + DMAC_GCR))

/* CSR (Channel Status Register) 비트 */
#define CSR_COC         0x80    /* Channel Operation Complete */
#define CSR_BLC         0x40    /* Block Transfer Complete */
#define CSR_NDT         0x20    /* Normal Device Termination */
#define CSR_ERR         0x10    /* Error */
#define CSR_ACT         0x08    /* Channel Active */
#define CSR_PCT         0x04    /* PCL Transition */
#define CSR_PCS         0x02    /* PCL State */
#define CSR_BTC         0x01    /* Bus Transfer Complete */

/* CER (Channel Error Register) 비트 */
#define CER_NONE        0x00    /* No error */
#define CER_CONFIG      0x01    /* Configuration error */
#define CER_TIMING      0x02    /* Operation timing error */
#define CER_ADDRESS     0x05    /* Address error */
#define CER_BUS         0x06    /* Bus error */
#define CER_COUNT       0x07    /* Count error */
#define CER_EXTERNAL    0x08    /* External abort */
#define CER_SOFTWARE    0x09    /* Software abort */

/* CCR (Channel Control Register) 비트 */
#define CCR_STR         0x80    /* Start operation */
#define CCR_CNT         0x40    /* Continue operation */
#define CCR_HLT         0x20    /* Halt operation */
#define CCR_SAB         0x10    /* Software abort */
#define CCR_INT         0x08    /* Interrupt enable */

/* OCR (Operation Control Register) 비트 */
#define OCR_DIR_MTD     0x00    /* Memory to Device */
#define OCR_DIR_DTM     0x80    /* Device to Memory */
#define OCR_SIZE_BYTE   0x00    /* Byte transfer */
#define OCR_SIZE_WORD   0x10    /* Word transfer */
#define OCR_SIZE_LONG   0x20    /* Long transfer */
#define OCR_SIZE_PACK   0x30    /* Packed (byte to unpacked) */
#define OCR_CHAIN_NONE  0x00    /* No chaining */
#define OCR_CHAIN_ARRAY 0x08    /* Array chaining */
#define OCR_CHAIN_LINK  0x0C    /* Linked array chaining */
#define OCR_REQG_AUTO   0x00    /* Auto request (max rate) */
#define OCR_REQG_BURST  0x00    /* Auto request - burst */
#define OCR_REQG_STEAL  0x02    /* Cycle steal with hold */
#define OCR_REQG_EXT    0x04    /* External request */

/* SCR (Sequence Control Register) 비트 */
#define SCR_MAC_NONE    0x00    /* Memory address no change */
#define SCR_MAC_INC     0x04    /* Memory address increment */
#define SCR_MAC_DEC     0x08    /* Memory address decrement */
#define SCR_DAC_NONE    0x00    /* Device address no change */
#define SCR_DAC_INC     0x01    /* Device address increment */
#define SCR_DAC_DEC     0x02    /* Device address decrement */

/* DCR (Device Control Register) 비트 */
#define DCR_XRM_BURST   0x00    /* Burst transfer */
#define DCR_XRM_STEAL   0x80    /* Cycle steal */
#define DCR_DTYP_68K    0x00    /* 68000 compatible */
#define DCR_DTYP_ACK    0x40    /* ACK device */
#define DCR_DPS_8       0x00    /* 8-bit device port */
#define DCR_DPS_16      0x20    /* 16-bit device port */
#define DCR_PCL_STATUS  0x00    /* PCL = status input */
#define DCR_PCL_INT     0x08    /* PCL = interrupt input */

/* 테스트 버퍼 크기 */
#define TEST_SIZE       4096
#define PATTERN_SIZE    256

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

/* 시간 측정용 */
static int get_time(void)
{
    struct iocs_time t = _iocs_ontime();
    return t.sec;
}

/* DMA 채널 리셋 */
static void dma_reset(int ch)
{
    /* 소프트웨어 어보트 */
    DMAC_REG_B(ch, DMAC_CCR) = CCR_SAB;

    /* 상태 클리어 대기 */
    while (DMAC_REG_B(ch, DMAC_CSR) & CSR_ACT) {
        /* 채널이 비활성화될 때까지 대기 */
    }

    /* 상태 레지스터 클리어 */
    DMAC_REG_B(ch, DMAC_CSR) = 0xFF;
}

/* DMA 에러 문자열 */
static const char *dma_error_str(int err)
{
    switch (err) {
    case CER_NONE:      return "No error";
    case CER_CONFIG:    return "Configuration error";
    case CER_TIMING:    return "Timing error";
    case CER_ADDRESS:   return "Address error";
    case CER_BUS:       return "Bus error";
    case CER_COUNT:     return "Count error";
    case CER_EXTERNAL:  return "External abort";
    case CER_SOFTWARE:  return "Software abort";
    default:            return "Unknown error";
    }
}

/* 메모리-메모리 DMA 전송 설정 */
static void dma_setup_memcpy(int ch, void *dst, const void *src, int size)
{
    /* 채널 리셋 */
    dma_reset(ch);

    /* Device Control Register: 버스트 모드, 16비트 */
    DMAC_REG_B(ch, DMAC_DCR) = DCR_XRM_BURST | DCR_DPS_16;

    /* Operation Control Register:
     * - Device to Memory (src -> dst로 해석)
     * - Long word 전송
     * - Auto request
     */
    DMAC_REG_B(ch, DMAC_OCR) = OCR_DIR_DTM | OCR_SIZE_LONG | OCR_REQG_AUTO;

    /* Sequence Control Register:
     * - 메모리 주소 증가
     * - 디바이스 주소 증가
     */
    DMAC_REG_B(ch, DMAC_SCR) = SCR_MAC_INC | SCR_DAC_INC;

    /* Function Codes (슈퍼바이저 데이터) */
    DMAC_REG_B(ch, DMAC_MFC) = 0x05;    /* 슈퍼바이저 데이터 */
    DMAC_REG_B(ch, DMAC_DFC) = 0x05;

    /* 전송 카운터 (롱워드 단위이므로 /4) */
    DMAC_REG_W(ch, DMAC_MTC) = size / 4;

    /* 주소 설정 */
    DMAC_REG_L(ch, DMAC_MAR) = (unsigned long)dst;      /* 목적지 */
    DMAC_REG_L(ch, DMAC_DAR) = (unsigned long)src;      /* 소스 */
}

/* DMA 전송 시작 */
static void dma_start(int ch)
{
    DMAC_REG_B(ch, DMAC_CCR) = CCR_STR;
}

/* DMA 완료 대기 */
static int dma_wait(int ch)
{
    unsigned char status;
    int timeout = 100000;

    while (timeout > 0) {
        status = DMAC_REG_B(ch, DMAC_CSR);

        /* 완료 또는 에러 */
        if (status & (CSR_COC | CSR_ERR)) {
            break;
        }
        timeout--;
    }

    if (timeout == 0) {
        return -1;  /* 타임아웃 */
    }

    if (status & CSR_ERR) {
        return DMAC_REG_B(ch, DMAC_CER);  /* 에러 코드 반환 */
    }

    return 0;  /* 성공 */
}

/* DMA 상태 표시 */
static void dma_show_status(int ch)
{
    unsigned char csr = DMAC_REG_B(ch, DMAC_CSR);
    unsigned char cer = DMAC_REG_B(ch, DMAC_CER);

    print("  CSR: 0x");
    print_hex(csr, 2);
    print(" (");
    if (csr & CSR_COC) print("COC ");
    if (csr & CSR_BLC) print("BLC ");
    if (csr & CSR_NDT) print("NDT ");
    if (csr & CSR_ERR) print("ERR ");
    if (csr & CSR_ACT) print("ACT ");
    print(")");
    newline();

    if (csr & CSR_ERR) {
        print("  CER: 0x");
        print_hex(cer, 2);
        print(" (");
        print(dma_error_str(cer));
        print(")");
        newline();
    }
}

/* 데이터 검증 */
static int verify_data(const unsigned char *src, const unsigned char *dst, int size)
{
    int i;
    int errors = 0;

    for (i = 0; i < size; i++) {
        if (src[i] != dst[i]) {
            errors++;
            if (errors <= 5) {
                print("  Mismatch at ");
                print_num(i);
                print(": src=0x");
                print_hex(src[i], 2);
                print(", dst=0x");
                print_hex(dst[i], 2);
                newline();
            }
        }
    }

    return errors;
}

/* 데모 1: 기본 메모리 전송 */
static void demo_basic_transfer(void)
{
    unsigned char *src_buf;
    unsigned char *dst_buf;
    int i, result, errors;

    print("=== Demo 1: Basic Memory Transfer ===");
    newline();

    /* 버퍼 할당 */
    src_buf = (unsigned char *)_dos_malloc(TEST_SIZE);
    dst_buf = (unsigned char *)_dos_malloc(TEST_SIZE);

    if (!src_buf || !dst_buf) {
        print("Memory allocation failed!");
        newline();
        if (src_buf) _dos_mfree(src_buf);
        if (dst_buf) _dos_mfree(dst_buf);
        return;
    }

    print("Source buffer: 0x");
    print_hex((unsigned long)src_buf, 8);
    newline();
    print("Dest buffer:   0x");
    print_hex((unsigned long)dst_buf, 8);
    newline();
    print("Size: ");
    print_num(TEST_SIZE);
    print(" bytes");
    newline();
    newline();

    /* 소스 데이터 초기화 */
    print("Initializing source data...");
    newline();
    for (i = 0; i < TEST_SIZE; i++) {
        src_buf[i] = (unsigned char)(i & 0xFF);
    }

    /* 목적지 클리어 */
    memset(dst_buf, 0, TEST_SIZE);

    /* DMA 전송 */
    print("Starting DMA transfer (CH0)...");
    newline();

    dma_setup_memcpy(0, dst_buf, src_buf, TEST_SIZE);
    dma_start(0);
    result = dma_wait(0);

    if (result < 0) {
        print("DMA transfer timeout!");
        newline();
    } else if (result > 0) {
        print("DMA transfer error: ");
        print(dma_error_str(result));
        newline();
    } else {
        print("DMA transfer complete!");
        newline();
    }

    dma_show_status(0);
    newline();

    /* 검증 */
    print("Verifying data...");
    newline();
    errors = verify_data(src_buf, dst_buf, TEST_SIZE);

    if (errors == 0) {
        print("Verification PASSED!");
    } else {
        print("Verification FAILED: ");
        print_num(errors);
        print(" errors");
    }
    newline();

    /* 버퍼 해제 */
    _dos_mfree(src_buf);
    _dos_mfree(dst_buf);
    newline();
}

/* 데모 2: CPU vs DMA 속도 비교 */
static void demo_speed_comparison(void)
{
    unsigned char *src_buf;
    unsigned char *dst_buf;
    int start_time, end_time;
    int cpu_time, dma_time;
    int i;

    print("=== Demo 2: CPU vs DMA Speed Comparison ===");
    newline();

    /* 버퍼 할당 */
    src_buf = (unsigned char *)_dos_malloc(TEST_SIZE);
    dst_buf = (unsigned char *)_dos_malloc(TEST_SIZE);

    if (!src_buf || !dst_buf) {
        print("Memory allocation failed!");
        newline();
        if (src_buf) _dos_mfree(src_buf);
        if (dst_buf) _dos_mfree(dst_buf);
        return;
    }

    /* 소스 초기화 */
    for (i = 0; i < TEST_SIZE; i++) {
        src_buf[i] = (unsigned char)i;
    }

    print("Transfer size: ");
    print_num(TEST_SIZE);
    print(" bytes");
    newline();
    newline();

    /* CPU 전송 (10회 평균) */
    print("CPU memcpy (10 iterations)...");
    newline();

    start_time = get_time();
    for (i = 0; i < 10; i++) {
        memcpy(dst_buf, src_buf, TEST_SIZE);
    }
    end_time = get_time();
    cpu_time = end_time - start_time;

    print("  Time: ");
    print_num(cpu_time);
    print(" (1/100 sec)");
    newline();

    /* DMA 전송 (10회) */
    print("DMA transfer (10 iterations)...");
    newline();

    start_time = get_time();
    for (i = 0; i < 10; i++) {
        memset(dst_buf, 0, TEST_SIZE);
        dma_setup_memcpy(0, dst_buf, src_buf, TEST_SIZE);
        dma_start(0);
        dma_wait(0);
    }
    end_time = get_time();
    dma_time = end_time - start_time;

    print("  Time: ");
    print_num(dma_time);
    print(" (1/100 sec)");
    newline();
    newline();

    /* 비교 */
    if (dma_time > 0 && cpu_time > 0) {
        if (dma_time < cpu_time) {
            print("DMA is ");
            print_num((cpu_time * 100) / dma_time);
            print("% faster than CPU");
        } else {
            print("CPU is ");
            print_num((dma_time * 100) / cpu_time);
            print("% faster than DMA");
        }
        newline();
    }
    print("(Note: DMA advantage increases with larger transfers)");
    newline();

    _dos_mfree(src_buf);
    _dos_mfree(dst_buf);
    newline();
}

/* 데모 3: 다중 채널 전송 */
static void demo_multi_channel(void)
{
    unsigned char *buf[4];
    int i, result;

    print("=== Demo 3: Multi-Channel Transfer ===");
    newline();
    print("Using all 4 DMA channels");
    newline();
    newline();

    /* 버퍼 할당 (4쌍) */
    for (i = 0; i < 4; i++) {
        buf[i] = (unsigned char *)_dos_malloc(PATTERN_SIZE * 2);
        if (!buf[i]) {
            print("Memory allocation failed for channel ");
            print_num(i);
            newline();
            while (--i >= 0) _dos_mfree(buf[i]);
            return;
        }

        /* 소스 초기화 (각 채널 다른 패턴) */
        memset(buf[i], 0x10 * (i + 1), PATTERN_SIZE);
        memset(buf[i] + PATTERN_SIZE, 0, PATTERN_SIZE);
    }

    /* 모든 채널 설정 */
    for (i = 0; i < 4; i++) {
        print("Setting up CH");
        print_num(i);
        print("...");
        newline();

        dma_setup_memcpy(i, buf[i] + PATTERN_SIZE, buf[i], PATTERN_SIZE);
    }

    /* 모든 채널 동시 시작 */
    print("Starting all channels...");
    newline();
    for (i = 0; i < 4; i++) {
        dma_start(i);
    }

    /* 완료 대기 */
    for (i = 0; i < 4; i++) {
        result = dma_wait(i);
        print("  CH");
        print_num(i);
        print(": ");
        if (result == 0) {
            print("OK");
        } else if (result < 0) {
            print("Timeout");
        } else {
            print("Error (");
            print(dma_error_str(result));
            print(")");
        }
        newline();
    }

    /* 검증 */
    newline();
    print("Verification:");
    newline();
    for (i = 0; i < 4; i++) {
        int errors = verify_data(buf[i], buf[i] + PATTERN_SIZE, PATTERN_SIZE);
        print("  CH");
        print_num(i);
        print(": ");
        if (errors == 0) {
            print("PASSED");
        } else {
            print("FAILED (");
            print_num(errors);
            print(" errors)");
        }
        newline();
    }

    /* 버퍼 해제 */
    for (i = 0; i < 4; i++) {
        _dos_mfree(buf[i]);
    }
    newline();
}

/* 데모 4: 전송 크기별 테스트 */
static void demo_size_test(void)
{
    unsigned char *src_buf;
    unsigned char *dst_buf;
    int sizes[] = {64, 256, 1024, 4096, 16384};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    int i, result, errors;

    print("=== Demo 4: Transfer Size Test ===");
    newline();

    /* 최대 크기로 버퍼 할당 */
    src_buf = (unsigned char *)_dos_malloc(16384);
    dst_buf = (unsigned char *)_dos_malloc(16384);

    if (!src_buf || !dst_buf) {
        print("Memory allocation failed!");
        newline();
        if (src_buf) _dos_mfree(src_buf);
        if (dst_buf) _dos_mfree(dst_buf);
        return;
    }

    /* 소스 초기화 */
    for (i = 0; i < 16384; i++) {
        src_buf[i] = (unsigned char)(i * 7);
    }

    print("Size (bytes)  Result     Verify");
    newline();
    print("------------  ---------  ------");
    newline();

    for (i = 0; i < num_sizes; i++) {
        int size = sizes[i];

        /* 크기 출력 */
        print_num(size);
        if (size < 100) print("   ");
        else if (size < 1000) print("  ");
        else if (size < 10000) print(" ");
        print("         ");

        /* 목적지 클리어 */
        memset(dst_buf, 0, size);

        /* DMA 전송 */
        dma_setup_memcpy(0, dst_buf, src_buf, size);
        dma_start(0);
        result = dma_wait(0);

        if (result == 0) {
            print("OK         ");
        } else if (result < 0) {
            print("Timeout    ");
        } else {
            print("Error      ");
        }

        /* 검증 */
        errors = verify_data(src_buf, dst_buf, size);
        if (errors == 0) {
            print("PASS");
        } else {
            print("FAIL");
        }
        newline();
    }

    _dos_mfree(src_buf);
    _dos_mfree(dst_buf);
    newline();
}

/* 데모 5: DMA 레지스터 덤프 */
static void demo_register_dump(void)
{
    int ch;

    print("=== Demo 5: DMA Register Dump ===");
    newline();

    print("General Control Register: 0x");
    print_hex(DMAC_GENERAL, 2);
    newline();
    newline();

    for (ch = 0; ch < 4; ch++) {
        print("--- Channel ");
        print_num(ch);
        print(" ---");
        newline();

        print("  CSR (Status):    0x");
        print_hex(DMAC_REG_B(ch, DMAC_CSR), 2);
        newline();

        print("  CER (Error):     0x");
        print_hex(DMAC_REG_B(ch, DMAC_CER), 2);
        newline();

        print("  DCR (Device):    0x");
        print_hex(DMAC_REG_B(ch, DMAC_DCR), 2);
        newline();

        print("  OCR (Operation): 0x");
        print_hex(DMAC_REG_B(ch, DMAC_OCR), 2);
        newline();

        print("  SCR (Sequence):  0x");
        print_hex(DMAC_REG_B(ch, DMAC_SCR), 2);
        newline();

        print("  CCR (Control):   0x");
        print_hex(DMAC_REG_B(ch, DMAC_CCR), 2);
        newline();

        print("  MTC (Count):     0x");
        print_hex(DMAC_REG_W(ch, DMAC_MTC), 4);
        newline();

        print("  MAR (MemAddr):   0x");
        print_hex(DMAC_REG_L(ch, DMAC_MAR), 8);
        newline();

        print("  DAR (DevAddr):   0x");
        print_hex(DMAC_REG_L(ch, DMAC_DAR), 8);
        newline();

        newline();
    }
}

/* 데모 6: 블록 전송 (메모리 초기화) */
static void demo_block_fill(void)
{
    unsigned char *buf;
    unsigned long pattern = 0xDEADBEEF;
    int i;

    print("=== Demo 6: Block Fill (Memory Init) ===");
    newline();

    buf = (unsigned char *)_dos_malloc(TEST_SIZE);
    if (!buf) {
        print("Memory allocation failed!");
        newline();
        return;
    }

    print("Buffer: 0x");
    print_hex((unsigned long)buf, 8);
    newline();
    print("Pattern: 0x");
    print_hex(pattern, 8);
    newline();
    print("Size: ");
    print_num(TEST_SIZE);
    print(" bytes");
    newline();
    newline();

    /* 버퍼 클리어 */
    memset(buf, 0, TEST_SIZE);

    /* DMA로 패턴 채우기 (DAR 고정, MAR 증가) */
    dma_reset(0);

    DMAC_REG_B(0, DMAC_DCR) = DCR_XRM_BURST | DCR_DPS_16;
    DMAC_REG_B(0, DMAC_OCR) = OCR_DIR_DTM | OCR_SIZE_LONG | OCR_REQG_AUTO;
    DMAC_REG_B(0, DMAC_SCR) = SCR_MAC_INC | SCR_DAC_NONE;  /* DAR 고정! */
    DMAC_REG_B(0, DMAC_MFC) = 0x05;
    DMAC_REG_B(0, DMAC_DFC) = 0x05;
    DMAC_REG_W(0, DMAC_MTC) = TEST_SIZE / 4;
    DMAC_REG_L(0, DMAC_MAR) = (unsigned long)buf;
    DMAC_REG_L(0, DMAC_DAR) = (unsigned long)&pattern;

    print("Starting block fill...");
    newline();

    dma_start(0);
    dma_wait(0);
    dma_show_status(0);

    /* 검증 */
    print("Verifying...");
    newline();

    int errors = 0;
    unsigned long *lbuf = (unsigned long *)buf;
    for (i = 0; i < TEST_SIZE / 4; i++) {
        if (lbuf[i] != pattern) {
            errors++;
            if (errors <= 3) {
                print("  [");
                print_num(i);
                print("]: 0x");
                print_hex(lbuf[i], 8);
                print(" (expected 0x");
                print_hex(pattern, 8);
                print(")");
                newline();
            }
        }
    }

    if (errors == 0) {
        print("Block fill PASSED!");
    } else {
        print("Block fill FAILED: ");
        print_num(errors);
        print(" errors");
    }
    newline();

    _dos_mfree(buf);
    newline();
}

int main(void)
{
    /* 화면 클리어 */
    _dos_c_cls_al();

    print("========================================");
    newline();
    print("  X68000 DMA Transfer Demo");
    newline();
    print("========================================");
    newline();
    newline();

    print("HD63450 DMA Controller Features:");
    newline();
    print("- 4 independent channels");
    newline();
    print("- Memory-to-memory transfer");
    newline();
    print("- Burst and cycle-steal modes");
    newline();
    print("- Up to 8/16/32-bit transfers");
    newline();
    newline();

    print("Press any key to start...");
    newline();
    _iocs_b_keyinp();

    /* 데모 실행 */
    _dos_c_cls_al();
    demo_basic_transfer();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_speed_comparison();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_multi_channel();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_size_test();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_register_dump();
    print("Press any key...");
    _iocs_b_keyinp();

    _dos_c_cls_al();
    demo_block_fill();
    print("Press any key...");
    _iocs_b_keyinp();

    /* 모든 채널 리셋 */
    {
        int i;
        for (i = 0; i < 4; i++) {
            dma_reset(i);
        }
    }

    /* 종료 */
    _dos_c_cls_al();
    print("DMA demo complete!");
    newline();
    print("Press any key to exit...");
    newline();
    _iocs_b_keyinp();

    return 0;
}
