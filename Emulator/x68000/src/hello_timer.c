/*
 * X68000 Human68k - Timer Interrupt Example
 * 타이머 인터럽트 처리 예제 (IOCS)
 *
 * X68000 타이머:
 * - Timer-D (MFP 68901): 범용 타이머
 * - VDISP: 수직 동기 인터럽트 (60Hz/55Hz)
 * - CRTC Raster: 래스터 인터럽트
 * - ontime: 시스템 시간 (1/100초)
 */
#include <x68k/iocs.h>
#include <x68k/dos.h>

/* 전역 변수 (인터럽트 핸들러에서 사용) */
static volatile int timer_count = 0;
static volatile int vdisp_count = 0;
static volatile int timer_running = 0;

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

/* 딜레이 */
static void delay(int count)
{
    volatile int i;
    for (i = 0; i < count * 1000; i++)
        ;
}

/* 시스템 시간 얻기 (1/100초 단위) */
static int get_time(void)
{
    struct iocs_time t = _iocs_ontime();
    return t.sec;  /* 1/100초 단위 */
}

/* Timer-D 인터럽트 핸들러 */
static void __attribute__((interrupt)) timer_handler(void)
{
    timer_count++;
}

/* VDISP 인터럽트 핸들러 */
static void __attribute__((interrupt)) vdisp_handler(void)
{
    vdisp_count++;
}

/* 데모 1: 시스템 시간 (ontime) */
static void demo_ontime(void)
{
    int start_time, current_time, elapsed;
    int seconds = 0;

    print("=== Demo 1: System Time (ontime) ===");
    newline();
    print("Showing elapsed time using get_time().");
    newline();
    print("Press ESC to continue.");
    newline();
    newline();

    start_time = get_time();

    while (1) {
        current_time = get_time();

        /* 자정을 넘긴 경우 처리 */
        if (current_time < start_time) {
            start_time = current_time;
        }

        elapsed = current_time - start_time;

        _dos_c_locate(0, 5);
        print("System time: ");
        print_num(current_time);
        print(" (1/100 sec)    ");

        _dos_c_locate(0, 6);
        print("Elapsed: ");
        print_num(elapsed / 100);
        print(".");
        print_num((elapsed % 100) / 10);
        print_num(elapsed % 10);
        print(" seconds    ");

        /* 1초마다 카운터 업데이트 */
        if (elapsed / 100 > seconds) {
            seconds = elapsed / 100;
            _dos_c_locate(0, 7);
            print("Seconds passed: ");
            print_num(seconds);
            print("    ");
        }

        /* ESC 체크 */
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            if (((key >> 8) & 0xFF) == 0x01) {
                break;
            }
        }

        delay(1);
    }
    newline();
}

/* 데모 2: Timer-D 사용 */
static void demo_timer_d(void)
{
    int prev_count;

    _dos_c_cls_al();
    print("=== Demo 2: Timer-D Interrupt ===");
    newline();
    print("Timer-D generates periodic interrupts.");
    newline();
    print("Press ESC to continue.");
    newline();
    newline();

    /* Timer-D 카운터 초기화 */
    timer_count = 0;
    timer_running = 1;

    /*
     * _iocs_timerdst(handler, unit, count)
     * handler: 인터럽트 핸들러 함수
     * unit: 타이머 단위 (1-4)
     *   1 = 1/200초
     *   2 = 1/100초
     *   3 = 1/50초
     *   4 = 1/25초
     * count: 카운트 값 (1-256, 256=0으로 지정)
     *
     * 실제 인터럽트 주기 = 단위시간 * count
     */
    _iocs_timerdst((void *)timer_handler, 4, 4);  /* 약 1/6초마다 */

    prev_count = 0;

    while (1) {
        if (timer_count != prev_count) {
            _dos_c_locate(0, 5);
            print("Timer-D count: ");
            print_num(timer_count);
            print("    ");

            _dos_c_locate(0, 6);
            print("Approx time: ");
            print_num(timer_count / 6);
            print(".");
            print_num((timer_count * 10 / 6) % 10);
            print(" seconds    ");

            prev_count = timer_count;
        }

        /* ESC 체크 */
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            if (((key >> 8) & 0xFF) == 0x01) {
                break;
            }
        }
    }

    /* Timer-D 정지 */
    _iocs_timerdst(0, 0, 0);
    timer_running = 0;

    newline();
}

/* 데모 3: VDISP (수직 동기) */
static void demo_vdisp(void)
{
    int prev_count;
    int start_count;

    _dos_c_cls_al();
    print("=== Demo 3: VDISP Interrupt (60Hz) ===");
    newline();
    print("VDISP fires every vertical blanking period.");
    newline();
    print("Press ESC to continue.");
    newline();
    newline();

    /* VDISP 카운터 초기화 */
    vdisp_count = 0;

    /*
     * _iocs_vdispst(handler, flag, line)
     * handler: 인터럽트 핸들러
     * flag: 0=등록, 1=해제
     * line: 래스터 라인 (보통 0)
     */
    _iocs_vdispst((void *)vdisp_handler, 0, 0);

    prev_count = 0;
    start_count = 0;

    while (1) {
        if (vdisp_count != prev_count) {
            _dos_c_locate(0, 5);
            print("VDISP count: ");
            print_num(vdisp_count);
            print("    ");

            /* 60Hz 기준 초 계산 */
            int elapsed = vdisp_count - start_count;
            _dos_c_locate(0, 6);
            print("Elapsed: ");
            print_num(elapsed / 60);
            print(".");
            print_num((elapsed % 60) * 100 / 60);
            print(" seconds (60Hz)    ");

            /* FPS 표시 */
            _dos_c_locate(0, 7);
            print("Frame rate: ~60 fps    ");

            prev_count = vdisp_count;
        }

        /* ESC 체크 */
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            if (((key >> 8) & 0xFF) == 0x01) {
                break;
            }
        }
    }

    /* VDISP 해제 */
    _iocs_vdispst(0, 1, 0);

    newline();
}

/* 데모 4: 간단한 스톱워치 */
static void demo_stopwatch(void)
{
    int running = 0;
    int start_time = 0;
    int pause_time = 0;
    int elapsed = 0;
    int lap_times[5] = {0};
    int lap_count = 0;
    int i;

    _dos_c_cls_al();
    print("=== Demo 4: Stopwatch ===");
    newline();
    print("SPACE: Start/Stop, L: Lap, R: Reset, ESC: Exit");
    newline();
    newline();

    while (1) {
        /* 시간 계산 */
        if (running) {
            int current = get_time();
            if (current < start_time) {
                start_time = current - elapsed;
            }
            elapsed = current - start_time;
        }

        /* 시간 표시 */
        _dos_c_locate(0, 4);
        print("  ");
        print_num(elapsed / 6000);
        print(":");
        if ((elapsed / 100) % 60 < 10) print_char('0');
        print_num((elapsed / 100) % 60);
        print(".");
        if (elapsed % 100 < 10) print_char('0');
        print_num(elapsed % 100);
        print("    ");

        /* 상태 표시 */
        _dos_c_locate(0, 5);
        if (running) {
            print("Status: RUNNING  ");
        } else if (elapsed > 0) {
            print("Status: PAUSED   ");
        } else {
            print("Status: STOPPED  ");
        }

        /* 랩 타임 표시 */
        _dos_c_locate(0, 7);
        print("Lap times:");
        for (i = 0; i < 5 && i < lap_count; i++) {
            _dos_c_locate(0, 8 + i);
            print("  Lap ");
            print_num(i + 1);
            print(": ");
            print_num(lap_times[i] / 6000);
            print(":");
            if ((lap_times[i] / 100) % 60 < 10) print_char('0');
            print_num((lap_times[i] / 100) % 60);
            print(".");
            if (lap_times[i] % 100 < 10) print_char('0');
            print_num(lap_times[i] % 100);
            print("    ");
        }

        /* 키 입력 처리 */
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            int scancode = (key >> 8) & 0xFF;
            int charcode = key & 0xFF;

            /* ESC: 종료 */
            if (scancode == 0x01) {
                break;
            }
            /* SPACE: 시작/정지 */
            else if (charcode == ' ') {
                if (running) {
                    running = 0;
                    pause_time = elapsed;
                } else {
                    running = 1;
                    start_time = get_time() - pause_time;
                }
            }
            /* L: 랩 타임 */
            else if (charcode == 'l' || charcode == 'L') {
                if (running && lap_count < 5) {
                    lap_times[lap_count++] = elapsed;
                }
            }
            /* R: 리셋 */
            else if (charcode == 'r' || charcode == 'R') {
                running = 0;
                elapsed = 0;
                pause_time = 0;
                lap_count = 0;
                for (i = 0; i < 5; i++) {
                    lap_times[i] = 0;
                }
                /* 랩 표시 클리어 */
                for (i = 0; i < 5; i++) {
                    _dos_c_locate(0, 8 + i);
                    print("                              ");
                }
            }
        }

        delay(1);
    }
}

/* 데모 5: 프레임 레이트 측정 */
static void demo_framerate(void)
{
    int frame = 0;
    int last_time;
    int fps = 0;
    int frame_in_sec = 0;

    _dos_c_cls_al();
    print("=== Demo 5: Frame Rate Counter ===");
    newline();
    print("Measuring actual frame rate.");
    newline();
    print("Press ESC to continue.");
    newline();
    newline();

    last_time = get_time();

    while (1) {
        int current_time = get_time();

        /* 프레임 카운트 */
        frame++;
        frame_in_sec++;

        /* 1초마다 FPS 계산 */
        if (current_time - last_time >= 100) {
            fps = frame_in_sec;
            frame_in_sec = 0;
            last_time = current_time;
        }

        /* 표시 */
        _dos_c_locate(0, 5);
        print("Total frames: ");
        print_num(frame);
        print("    ");

        _dos_c_locate(0, 6);
        print("FPS: ");
        print_num(fps);
        print("    ");

        _dos_c_locate(0, 7);
        print("Time: ");
        print_num(current_time / 100);
        print(".");
        if (current_time % 100 < 10) print_char('0');
        print_num(current_time % 100);
        print(" sec    ");

        /* 간단한 애니메이션 */
        _dos_c_locate(0, 9);
        print("Animation: ");
        print_char("|/-\\"[frame % 4]);
        print("  ");

        /* ESC 체크 */
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            if (((key >> 8) & 0xFF) == 0x01) {
                break;
            }
        }

        /* 약간의 작업 시뮬레이션 */
        delay(1);
    }
}

/* 데모 6: 주기적 이벤트 */
static void demo_periodic_event(void)
{
    int last_event = 0;
    int event_interval = 100;  /* 1초 간격 */
    int event_count = 0;
    int toggle = 0;

    _dos_c_cls_al();
    print("=== Demo 6: Periodic Events ===");
    newline();
    print("Triggering events at 1 second intervals.");
    newline();
    print("UP/DOWN: Change interval, ESC: Exit");
    newline();
    newline();

    last_event = get_time();

    while (1) {
        int current_time = get_time();

        /* 이벤트 시간 체크 */
        if (current_time - last_event >= event_interval) {
            last_event = current_time;
            event_count++;
            toggle = !toggle;

            /* 이벤트 발생 표시 */
            _dos_c_locate(0, 5);
            print("*** EVENT #");
            print_num(event_count);
            print(" ***    ");

            _dos_c_locate(20, 5);
            if (toggle) {
                print("[ON] ");
            } else {
                print("[OFF]");
            }
        }

        /* 정보 표시 */
        _dos_c_locate(0, 7);
        print("Interval: ");
        print_num(event_interval / 100);
        print(".");
        print_num((event_interval % 100) / 10);
        print(" seconds    ");

        _dos_c_locate(0, 8);
        print("Events: ");
        print_num(event_count);
        print("    ");

        _dos_c_locate(0, 9);
        print("Time to next: ");
        int remaining = event_interval - (current_time - last_event);
        if (remaining < 0) remaining = 0;
        print_num(remaining);
        print(" (1/100s)    ");

        /* 키 입력 */
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            int scancode = (key >> 8) & 0xFF;

            if (scancode == 0x01) {  /* ESC */
                break;
            } else if (scancode == 0x3C) {  /* UP */
                if (event_interval < 500) {
                    event_interval += 50;
                }
            } else if (scancode == 0x3E) {  /* DOWN */
                if (event_interval > 50) {
                    event_interval -= 50;
                }
            }
        }

        delay(1);
    }
}

int main(void)
{
    /* 화면 클리어 */
    _dos_c_cls_al();

    print("========================================");
    newline();
    print("  X68000 Timer Interrupt Demo");
    newline();
    print("========================================");
    newline();
    newline();

    print("This demo shows timer/interrupt handling.");
    newline();
    print("Press any key to start...");
    newline();
    _iocs_b_keyinp();

    /* 데모 실행 */
    _dos_c_cls_al();
    demo_ontime();

    demo_timer_d();

    demo_vdisp();

    demo_stopwatch();

    demo_framerate();

    demo_periodic_event();

    /* 종료 */
    _dos_c_cls_al();
    print("Timer demo complete!");
    newline();
    print("Press any key to exit...");
    newline();
    _iocs_b_keyinp();

    return 0;
}
