/*
 * X68000 Human68k - Simple Game Example
 * 간단한 게임 예제
 *
 * "Star Catcher" - 별을 모으고 폭탄을 피하는 게임
 * - 스프라이트 사용
 * - 조이스틱/키보드 입력
 * - OPM 사운드 효과
 * - 점수 시스템
 */
#include <x68k/iocs.h>
#include <x68k/dos.h>

/* 게임 상수 */
#define SCREEN_WIDTH    256
#define SCREEN_HEIGHT   256
#define PLAYER_Y        220
#define MAX_OBJECTS     16
#define PLAYER_SPEED    4
#define STAR_SCORE      10
#define BOMB_DAMAGE     1

/* 오브젝트 타입 */
#define OBJ_NONE        0
#define OBJ_STAR        1
#define OBJ_BOMB        2

/* 게임 상태 */
#define STATE_TITLE     0
#define STATE_PLAYING   1
#define STATE_GAMEOVER  2
#define STATE_PAUSED    3

/* 조이스틱 비트 */
#define JOY_LEFT        0x04
#define JOY_RIGHT       0x08
#define JOY_BTN_A       0x20

/* 게임 데이터 */
static int player_x;
static int score;
static int lives;
static int level;
static int game_state;
static int high_score = 0;
static int frame_count;

/* 오브젝트 데이터 */
static struct {
    int type;
    int x;
    int y;
    int speed;
} objects[MAX_OBJECTS];

/* 스프라이트 패턴 */

/* 플레이어 (우주선 모양) */
static const unsigned char pattern_player[128] = {
    0x00, 0x00, 0x00, 0x11, 0x11, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x11, 0x11, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x11, 0x11, 0x11, 0x11, 0x00, 0x00,
    0x00, 0x01, 0x11, 0x11, 0x11, 0x11, 0x10, 0x00,
    0x00, 0x11, 0x11, 0x22, 0x22, 0x11, 0x11, 0x00,
    0x01, 0x11, 0x11, 0x22, 0x22, 0x11, 0x11, 0x10,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x01, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x10,
    0x00, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x00,
    0x00, 0x01, 0x11, 0x33, 0x33, 0x11, 0x10, 0x00,
    0x00, 0x00, 0x11, 0x33, 0x33, 0x11, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x33, 0x33, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x33, 0x33, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x03, 0x30, 0x00, 0x00, 0x00,
};

/* 별 */
static const unsigned char pattern_star[128] = {
    0x00, 0x00, 0x00, 0x44, 0x44, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x44, 0x44, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x44, 0x44, 0x40, 0x00, 0x00,
    0x00, 0x04, 0x44, 0x44, 0x44, 0x44, 0x40, 0x00,
    0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
    0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
    0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x00,
    0x00, 0x04, 0x44, 0x44, 0x44, 0x44, 0x40, 0x00,
    0x00, 0x04, 0x44, 0x44, 0x44, 0x44, 0x40, 0x00,
    0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x00,
    0x00, 0x44, 0x44, 0x00, 0x00, 0x44, 0x44, 0x00,
    0x04, 0x44, 0x40, 0x00, 0x00, 0x04, 0x44, 0x40,
    0x44, 0x44, 0x00, 0x00, 0x00, 0x00, 0x44, 0x44,
    0x44, 0x40, 0x00, 0x00, 0x00, 0x00, 0x04, 0x44,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* 폭탄 */
static const unsigned char pattern_bomb[128] = {
    0x00, 0x00, 0x00, 0x55, 0x55, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x05, 0x55, 0x55, 0x50, 0x00, 0x00,
    0x00, 0x05, 0x55, 0x55, 0x55, 0x55, 0x50, 0x00,
    0x00, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x00,
    0x05, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x50,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x05, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x50,
    0x00, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x00,
    0x00, 0x05, 0x55, 0x55, 0x55, 0x55, 0x50, 0x00,
    0x00, 0x00, 0x55, 0x55, 0x55, 0x55, 0x00, 0x00,
    0x00, 0x00, 0x05, 0x55, 0x55, 0x50, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x55, 0x55, 0x00, 0x00, 0x00,
};

/* 하트 (라이프) */
static const unsigned char pattern_heart[128] = {
    0x00, 0x66, 0x66, 0x00, 0x00, 0x66, 0x66, 0x00,
    0x06, 0x66, 0x66, 0x60, 0x06, 0x66, 0x66, 0x60,
    0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
    0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
    0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
    0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
    0x06, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x60,
    0x06, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x60,
    0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00,
    0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00,
    0x00, 0x06, 0x66, 0x66, 0x66, 0x66, 0x60, 0x00,
    0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00,
    0x00, 0x00, 0x06, 0x66, 0x66, 0x60, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x66, 0x66, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* 난수 생성 */
static unsigned int rand_seed = 12345;
static int rand_next(void)
{
    rand_seed = rand_seed * 1103515245 + 12345;
    return (rand_seed >> 16) & 0x7FFF;
}

/* 화면 출력 */
static void print(const char *str)
{
    _iocs_b_print(str);
}

static void print_num(int num)
{
    char buf[12];
    int i = 0;

    if (num == 0) {
        _iocs_b_putc('0');
        return;
    }
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    while (i > 0) {
        _iocs_b_putc(buf[--i]);
    }
}

/* 사운드 효과 */
static void opm_write(int reg, int data)
{
    while (_iocs_opmsns() & 0x80)
        ;
    _iocs_opmset(reg, data);
}

static void sound_init(void)
{
    int i;
    /* 모든 채널 키 오프 */
    for (i = 0; i < 8; i++) {
        opm_write(0x08, i);
    }
}

static void play_beep(int freq, int duration)
{
    /* 간단한 비프음 (채널 7 사용) */
    opm_write(0x20 + 7, 0xC7);  /* PAN/FL/CON */
    opm_write(0x40 + 7, 0x01);  /* DT1/MUL */
    opm_write(0x60 + 7, 0x10);  /* TL */
    opm_write(0x80 + 7, 0x1F);  /* KS/AR */
    opm_write(0xA0 + 7, 0x00);  /* AME/D1R */
    opm_write(0xC0 + 7, 0x00);  /* DT2/D2R */
    opm_write(0xE0 + 7, 0x0F);  /* D1L/RR */

    opm_write(0x28 + 7, freq);  /* KC */
    opm_write(0x30 + 7, 0);     /* KF */
    opm_write(0x08, 0x78 | 7);  /* KEY ON */

    /* 간단한 딜레이 */
    volatile int i;
    for (i = 0; i < duration * 500; i++)
        ;

    opm_write(0x08, 7);  /* KEY OFF */
}

static void sound_star(void)
{
    play_beep(0x4A, 20);  /* 높은 음 */
}

static void sound_bomb(void)
{
    play_beep(0x1A, 50);  /* 낮은 음 */
}

static void sound_gameover(void)
{
    play_beep(0x2A, 100);
    play_beep(0x1A, 100);
    play_beep(0x0A, 200);
}

/* 스프라이트 설정 */
static void set_sprite(int num, int x, int y, int code, int palette, int pri)
{
    int attr = (palette & 0x0F);
    _iocs_sp_regst(num, x, y, code, attr, pri);
}

static void hide_sprite(int num)
{
    _iocs_sp_regst(num, 0, 256, 0, 0, 0);
}

/* 팔레트 설정 */
static void setup_palettes(void)
{
    /* 팔레트 1: 플레이어 (흰색 계열) */
    _iocs_spalet(1, 1*16 + 0, 0x0000);   /* 투명 */
    _iocs_spalet(1, 1*16 + 1, 0xFFFF);   /* 흰색 */
    _iocs_spalet(1, 1*16 + 2, 0x03FF);   /* 하늘색 (창문) */
    _iocs_spalet(1, 1*16 + 3, 0xF800);   /* 빨강 (불꽃) */

    /* 팔레트 2: 별 (노랑) */
    _iocs_spalet(1, 2*16 + 0, 0x0000);
    _iocs_spalet(1, 2*16 + 4, 0xFFE0);   /* 노랑 */

    /* 팔레트 3: 폭탄 (빨강) */
    _iocs_spalet(1, 3*16 + 0, 0x0000);
    _iocs_spalet(1, 3*16 + 5, 0xF800);   /* 빨강 */

    /* 팔레트 4: 하트 (분홍) */
    _iocs_spalet(1, 4*16 + 0, 0x0000);
    _iocs_spalet(1, 4*16 + 6, 0xF81F);   /* 분홍 */
}

/* 게임 초기화 */
static void game_init(void)
{
    int i;

    /* 스프라이트 초기화 */
    _iocs_sp_init();
    _iocs_sp_on();

    /* 패턴 정의 */
    _iocs_sp_defcg(0, 1, pattern_player);
    _iocs_sp_defcg(1, 1, pattern_star);
    _iocs_sp_defcg(2, 1, pattern_bomb);
    _iocs_sp_defcg(3, 1, pattern_heart);

    /* 팔레트 설정 */
    setup_palettes();

    /* 오브젝트 초기화 */
    for (i = 0; i < MAX_OBJECTS; i++) {
        objects[i].type = OBJ_NONE;
    }

    /* 게임 변수 초기화 */
    player_x = SCREEN_WIDTH / 2;
    score = 0;
    lives = 3;
    level = 1;
    frame_count = 0;

    sound_init();
}

/* 새 게임 시작 */
static void new_game(void)
{
    int i;

    player_x = SCREEN_WIDTH / 2;
    score = 0;
    lives = 3;
    level = 1;
    frame_count = 0;

    for (i = 0; i < MAX_OBJECTS; i++) {
        objects[i].type = OBJ_NONE;
        hide_sprite(i + 1);  /* 스프라이트 0은 플레이어 */
    }

    game_state = STATE_PLAYING;
}

/* 오브젝트 생성 */
static void spawn_object(void)
{
    int i;

    for (i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].type == OBJ_NONE) {
            objects[i].x = rand_next() % (SCREEN_WIDTH - 16);
            objects[i].y = 0;
            objects[i].speed = 1 + (level / 2) + (rand_next() % 2);

            /* 별 70%, 폭탄 30% */
            if (rand_next() % 10 < 7) {
                objects[i].type = OBJ_STAR;
            } else {
                objects[i].type = OBJ_BOMB;
            }
            break;
        }
    }
}

/* 입력 처리 */
static void handle_input(void)
{
    int joy = _iocs_joyget(0);

    /* 키보드 입력도 체크 */
    if (_iocs_b_keysns()) {
        int key = _iocs_b_keyinp();
        int scancode = (key >> 8) & 0xFF;

        if (scancode == 0x3B) joy &= ~JOY_LEFT;   /* 왼쪽 화살표 */
        if (scancode == 0x3D) joy &= ~JOY_RIGHT;  /* 오른쪽 화살표 */
        if (scancode == 0x01) {  /* ESC */
            if (game_state == STATE_PLAYING) {
                game_state = STATE_PAUSED;
            } else if (game_state == STATE_PAUSED) {
                game_state = STATE_PLAYING;
            }
        }
        if (scancode == 0x35) joy &= ~JOY_BTN_A;  /* 스페이스 */
    }

    if (game_state == STATE_PLAYING) {
        /* 플레이어 이동 */
        if (!(joy & JOY_LEFT) && player_x > 8) {
            player_x -= PLAYER_SPEED;
        }
        if (!(joy & JOY_RIGHT) && player_x < SCREEN_WIDTH - 24) {
            player_x += PLAYER_SPEED;
        }
    } else if (game_state == STATE_TITLE || game_state == STATE_GAMEOVER) {
        /* 버튼으로 게임 시작 */
        if (!(joy & JOY_BTN_A)) {
            new_game();
        }
    } else if (game_state == STATE_PAUSED) {
        if (!(joy & JOY_BTN_A)) {
            game_state = STATE_PLAYING;
        }
    }
}

/* 오브젝트 업데이트 */
static void update_objects(void)
{
    int i;

    for (i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].type != OBJ_NONE) {
            /* 이동 */
            objects[i].y += objects[i].speed;

            /* 화면 밖으로 나가면 제거 */
            if (objects[i].y > SCREEN_HEIGHT) {
                objects[i].type = OBJ_NONE;
                hide_sprite(i + 1);
            }
        }
    }
}

/* 충돌 체크 */
static void check_collisions(void)
{
    int i;
    int px1 = player_x;
    int py1 = PLAYER_Y;
    int px2 = player_x + 16;
    int py2 = PLAYER_Y + 16;

    for (i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].type != OBJ_NONE) {
            int ox1 = objects[i].x;
            int oy1 = objects[i].y;
            int ox2 = objects[i].x + 16;
            int oy2 = objects[i].y + 16;

            /* AABB 충돌 체크 */
            if (px1 < ox2 && px2 > ox1 && py1 < oy2 && py2 > oy1) {
                if (objects[i].type == OBJ_STAR) {
                    /* 별 획득 */
                    score += STAR_SCORE * level;
                    sound_star();

                    /* 레벨 업 (100점마다) */
                    if (score / 100 >= level && level < 10) {
                        level++;
                    }
                } else if (objects[i].type == OBJ_BOMB) {
                    /* 폭탄 피해 */
                    lives -= BOMB_DAMAGE;
                    sound_bomb();

                    if (lives <= 0) {
                        game_state = STATE_GAMEOVER;
                        if (score > high_score) {
                            high_score = score;
                        }
                        sound_gameover();
                    }
                }

                /* 오브젝트 제거 */
                objects[i].type = OBJ_NONE;
                hide_sprite(i + 1);
            }
        }
    }
}

/* 화면 렌더링 */
static void render(void)
{
    int i;

    /* 플레이어 스프라이트 */
    set_sprite(0, player_x, PLAYER_Y, 0, 1, 3);

    /* 오브젝트 스프라이트 */
    for (i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].type != OBJ_NONE) {
            int code = (objects[i].type == OBJ_STAR) ? 1 : 2;
            int pal = (objects[i].type == OBJ_STAR) ? 2 : 3;
            set_sprite(i + 1, objects[i].x, objects[i].y, code, pal, 2);
        }
    }

    /* 라이프 표시 (하트 스프라이트) */
    for (i = 0; i < 3; i++) {
        if (i < lives) {
            set_sprite(MAX_OBJECTS + 1 + i, 8 + i * 18, 8, 3, 4, 3);
        } else {
            hide_sprite(MAX_OBJECTS + 1 + i);
        }
    }

    /* 점수 표시 (텍스트) */
    _dos_c_locate(20, 0);
    print("SCORE:");
    print_num(score);
    print("  ");

    _dos_c_locate(35, 0);
    print("LV:");
    print_num(level);

    _dos_c_locate(42, 0);
    print("HI:");
    print_num(high_score);
}

/* 타이틀 화면 */
static void show_title(void)
{
    _dos_c_cls_al();

    _dos_c_locate(8, 8);
    print("*** STAR CATCHER ***");

    _dos_c_locate(6, 12);
    print("Catch stars, avoid bombs!");

    _dos_c_locate(8, 16);
    print("Controls:");
    _dos_c_locate(8, 17);
    print("  LEFT/RIGHT - Move");
    _dos_c_locate(8, 18);
    print("  ESC - Pause");

    _dos_c_locate(6, 22);
    print("Press BUTTON A or SPACE to start");

    /* 플레이어 미리보기 */
    set_sprite(0, 120, 100, 0, 1, 3);
    set_sprite(1, 100, 70, 1, 2, 3);  /* 별 */
    set_sprite(2, 140, 70, 2, 3, 3);  /* 폭탄 */
}

/* 게임오버 화면 */
static void show_gameover(void)
{
    _dos_c_locate(10, 10);
    print("*** GAME OVER ***");

    _dos_c_locate(10, 14);
    print("Final Score: ");
    print_num(score);

    if (score >= high_score && score > 0) {
        _dos_c_locate(10, 16);
        print("** NEW HIGH SCORE! **");
    }

    _dos_c_locate(6, 20);
    print("Press BUTTON A to play again");
}

/* 일시정지 화면 */
static void show_paused(void)
{
    _dos_c_locate(12, 12);
    print("*** PAUSED ***");

    _dos_c_locate(8, 16);
    print("Press BUTTON A to continue");
}

/* 메인 게임 루프 */
static void game_loop(void)
{
    int running = 1;

    game_state = STATE_TITLE;
    show_title();

    while (running) {
        handle_input();

        if (game_state == STATE_PLAYING) {
            /* 게임 업데이트 */
            update_objects();
            check_collisions();

            /* 오브젝트 생성 (레벨에 따라 빈도 증가) */
            if (frame_count % (30 - level * 2) == 0) {
                spawn_object();
            }

            frame_count++;

            /* 렌더링 */
            render();
        } else if (game_state == STATE_GAMEOVER) {
            show_gameover();
        } else if (game_state == STATE_PAUSED) {
            show_paused();
        }

        /* 프레임 딜레이 (약 60fps 목표) */
        {
            volatile int i;
            for (i = 0; i < 2000; i++)
                ;
        }

        /* Q키로 완전 종료 */
        if (_iocs_b_keysns()) {
            int key = _iocs_b_keyinp();
            if ((key & 0xFF) == 'q' || (key & 0xFF) == 'Q') {
                running = 0;
            }
        }
    }
}

int main(void)
{
    /* 화면 모드 설정 */
    _iocs_crtmod(10);  /* 256x256 16색 + 스프라이트 */

    /* 게임 초기화 */
    game_init();

    /* 게임 루프 */
    game_loop();

    /* 종료 처리 */
    _iocs_sp_off();
    sound_init();  /* 사운드 정지 */

    _iocs_crtmod(16);  /* 텍스트 모드 복귀 */

    _dos_c_cls_al();
    print("Thanks for playing!");
    print("\r\n");
    print("Final Score: ");
    print_num(score);
    print("\r\n");
    print("High Score: ");
    print_num(high_score);
    print("\r\n\r\n");
    print("Press any key to exit...");
    print("\r\n");
    _iocs_b_keyinp();

    return 0;
}
