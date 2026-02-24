#include "render.h"
#include "logic.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <x68k/dos.h>

#define STATUS_LINE_MAX 60
#define HELP_MAX_LINES 256
#define HELP_LINE_MAX  96
#define HELP_PAGE_LINES 18
#define HELP_WRAP_WIDTH 76
#define INFO_LINE_MAX 96

#define SCREEN_ROW_MAP_TOP 1
#define SCREEN_ROW_INFO (VIEW_H + 2)
#define SCREEN_ROW_STATUS (VIEW_H + 3)
#define SCREEN_ROW_STATUS_PREV (VIEW_H + 4)
#ifdef DEBUG_MONSTER_PHASE
#define SCREEN_ROW_DEBUG (VIEW_H + 5)
#define SCREEN_ROW_GUIDE (VIEW_H + 6)
#else
#define SCREEN_ROW_GUIDE (VIEW_H + 5)
#endif

typedef struct {
    unsigned char valid;
    unsigned char room;
    unsigned char cam_x;
    unsigned char cam_y;
    unsigned char x;
    unsigned char y;
#ifdef DEBUG_MONSTER_PHASE
    unsigned char monster_phase;
#endif
    char status[128];
    char status_prev[128];
    char view[VIEW_H][VIEW_W];
} RenderCache;

static RenderCache s_render_cache;
static int help_prompt_quit(void);

static void clear_screen(void)
{
    fputs("\033[2J\033[H", stdout);
}

static void move_cursor(unsigned char row, unsigned char col)
{
    printf("\033[%u;%uH", (unsigned)row, (unsigned)col);
}

static void clear_line_and_print(unsigned char row, const char *text)
{
    move_cursor(row, 1);
    fputs("\033[2K", stdout);
    if (text) fputs(text, stdout);
}

static void build_view(const GameState *g, char out[VIEW_H][VIEW_W])
{
    unsigned char row;
    unsigned char col;

    for (row = 0; row < VIEW_H; row++) {
        for (col = 0; col < VIEW_W; col++) {
            unsigned char wx = (unsigned char)(g->cam_x + col);
            unsigned char wy = (unsigned char)(g->cam_y + row);
            char ch = logic_get_tile(g->room, wx, wy);
            if (wx == g->x && wy == g->y) {
                ch = '!';
            } else if (logic_find_monster_at(g->room, wx, wy) >= 0) {
                ch = '$';
            }
            out[row][col] = ch;
        }
    }
}

static void build_info_line(const GameState *g, char out[INFO_LINE_MAX])
{
    snprintf(out, INFO_LINE_MAX, "R:%u X:%u Y:%u Z:%u",
             (unsigned)(g->room + 1),
             (unsigned)g->x,
             (unsigned)g->y,
             (unsigned)g_room_z[g->room]);
}

static void draw_hud_full(const GameState *g)
{
    char info[INFO_LINE_MAX];
    build_info_line(g, info);
    printf("%s\n", info);
    printf("%s\n", g->status);
    if (g->status_prev[0] != '\0') printf("%s\n", g->status_prev);
    else printf("-\n");
#ifdef DEBUG_MONSTER_PHASE
    printf("[debug] monster phase: %u\n", (unsigned)logic_get_monster_phase(g->room));
#endif
    printf("WASD move | H help | M monster | Q\n");
}

static void draw_full_initial(const GameState *g, const char view[VIEW_H][VIEW_W])
{
    unsigned char row;
    unsigned char col;

    clear_screen();
    for (row = 0; row < VIEW_H; row++) {
        for (col = 0; col < VIEW_W; col++) {
            putchar(view[row][col]);
        }
        putchar('\n');
    }
    putchar('\n');
    draw_hud_full(g);
}

static void draw_full_no_clear(const GameState *g, const char view[VIEW_H][VIEW_W])
{
    unsigned char row;
    unsigned char col;
    char line[VIEW_W + 1];
    char info[INFO_LINE_MAX];

    for (row = 0; row < VIEW_H; row++) {
        for (col = 0; col < VIEW_W; col++) {
            line[col] = view[row][col];
        }
        line[VIEW_W] = '\0';
        clear_line_and_print((unsigned char)(SCREEN_ROW_MAP_TOP + row), line);
    }

    build_info_line(g, info);
    clear_line_and_print(SCREEN_ROW_INFO, info);
    clear_line_and_print(SCREEN_ROW_STATUS, g->status);
    if (g->status_prev[0] != '\0') clear_line_and_print(SCREEN_ROW_STATUS_PREV, g->status_prev);
    else clear_line_and_print(SCREEN_ROW_STATUS_PREV, "-");
#ifdef DEBUG_MONSTER_PHASE
    {
        char debug_line[48];
        snprintf(debug_line, sizeof(debug_line), "[debug] monster phase: %u",
                 (unsigned)logic_get_monster_phase(g->room));
        clear_line_and_print(SCREEN_ROW_DEBUG, debug_line);
    }
#endif
    clear_line_and_print(SCREEN_ROW_GUIDE, "WASD move | H help | M monster | Q");
}

static void draw_partial(const GameState *g, const char view[VIEW_H][VIEW_W])
{
    unsigned char row;
    unsigned char col;

    for (row = 0; row < VIEW_H; row++) {
        for (col = 0; col < VIEW_W; col++) {
            if (view[row][col] != s_render_cache.view[row][col]) {
                move_cursor((unsigned char)(SCREEN_ROW_MAP_TOP + row), (unsigned char)(col + 1));
                putchar(view[row][col]);
            }
        }
    }

    if (g->room != s_render_cache.room || g->x != s_render_cache.x || g->y != s_render_cache.y) {
        char info[INFO_LINE_MAX];
        build_info_line(g, info);
        clear_line_and_print(SCREEN_ROW_INFO, info);
    }

    if (strcmp(g->status, s_render_cache.status) != 0) {
        clear_line_and_print(SCREEN_ROW_STATUS, g->status);
    }
    if (strcmp(g->status_prev, s_render_cache.status_prev) != 0) {
        const char *prev = g->status_prev[0] != '\0' ? g->status_prev : "-";
        clear_line_and_print(SCREEN_ROW_STATUS_PREV, prev);
    }
#ifdef DEBUG_MONSTER_PHASE
    {
        unsigned char phase = logic_get_monster_phase(g->room);
        if (phase != s_render_cache.monster_phase) {
            char debug_line[48];
            snprintf(debug_line, sizeof(debug_line), "[debug] monster phase: %u", (unsigned)phase);
            clear_line_and_print(SCREEN_ROW_DEBUG, debug_line);
        }
    }
#endif
    move_cursor(SCREEN_ROW_GUIDE, 1);
}

static void snapshot_render_state(const GameState *g, const char view[VIEW_H][VIEW_W])
{
    unsigned char row;
    unsigned char col;

    s_render_cache.valid = 1;
    s_render_cache.room = g->room;
    s_render_cache.cam_x = g->cam_x;
    s_render_cache.cam_y = g->cam_y;
    s_render_cache.x = g->x;
    s_render_cache.y = g->y;
    strncpy(s_render_cache.status, g->status, sizeof(s_render_cache.status) - 1);
    s_render_cache.status[sizeof(s_render_cache.status) - 1] = '\0';
    strncpy(s_render_cache.status_prev, g->status_prev, sizeof(s_render_cache.status_prev) - 1);
    s_render_cache.status_prev[sizeof(s_render_cache.status_prev) - 1] = '\0';
#ifdef DEBUG_MONSTER_PHASE
    s_render_cache.monster_phase = logic_get_monster_phase(g->room);
#endif
    for (row = 0; row < VIEW_H; row++) {
        for (col = 0; col < VIEW_W; col++) {
            s_render_cache.view[row][col] = view[row][col];
        }
    }
}

void render_set_status(GameState *g, const char *text)
{
    unsigned int i, n;
    size_t len;

    if (!text) {
        g->status[0] = '\0';
        return;
    }

    len = strlen(text);
    n = (unsigned int)len;
    if (n > STATUS_LINE_MAX) n = STATUS_LINE_MAX;

    if (n == STATUS_LINE_MAX && len > STATUS_LINE_MAX) {
        /* Reserve the tail for ellipsis when truncated. */
        for (i = 0; i < STATUS_LINE_MAX - 3; i++) {
            g->status[i] = text[i];
        }
        g->status[STATUS_LINE_MAX - 3] = '.';
        g->status[STATUS_LINE_MAX - 2] = '.';
        g->status[STATUS_LINE_MAX - 1] = '.';
        g->status[STATUS_LINE_MAX] = '\0';
        return;
    }

    for (i = 0; i < n; i++) {
        g->status[i] = text[i];
    }
    g->status[n] = '\0';
}

void render_draw(const GameState *g)
{
    char view[VIEW_H][VIEW_W];
    int need_full;

    build_view(g, view);

    need_full = g->room != s_render_cache.room ||
                g->cam_x != s_render_cache.cam_x ||
                g->cam_y != s_render_cache.cam_y;

    if (!s_render_cache.valid) draw_full_initial(g, view);
    else if (need_full) draw_full_no_clear(g, view);
    else draw_partial(g, view);

    snapshot_render_state(g, view);
    fflush(stdout);
}

int render_read_key(void)
{
    int ch;

    do {
        ch = _dos_getchar();
    } while (ch == '\r' || ch == '\n');

    return tolower(ch & 0xff);
}

int render_prompt_yes_no(GameState *g, const char *text)
{
    char buf[128];
    char status_backup[128];
    int key;

    snprintf(buf, sizeof(buf), "%s 1=yes 0=no", text);

    /* Overlay prompt on the same status line without shifting history. */
    strncpy(status_backup, g->status, sizeof(status_backup) - 1);
    status_backup[sizeof(status_backup) - 1] = '\0';
    strncpy(g->status, buf, sizeof(g->status) - 1);
    g->status[sizeof(g->status) - 1] = '\0';
    render_draw(g);

    for (;;) {
        key = render_read_key();
        if (key == '1') {
            strncpy(g->status, status_backup, sizeof(g->status) - 1);
            g->status[sizeof(g->status) - 1] = '\0';
            return 1;
        }
        if (key == '0') {
            strncpy(g->status, status_backup, sizeof(g->status) - 1);
            g->status[sizeof(g->status) - 1] = '\0';
            return 0;
        }
    }
}

void render_wait_any_key(GameState *g, const char *text)
{
    render_set_status(g, text);
    render_draw(g);
    (void)render_read_key();
}

static int render_show_help_builtin(void)
{
    int key;

    clear_screen();
    puts("Prototype 01 X68000 Help");
    puts("");
    puts("Move      : W A S D");
    puts("Monster   : M toggle move");
    puts("Confirm   : 1=yes, 0=no");
    puts("Quit      : Q then 1");
    puts("Cancel    : Q then 0");
    puts("Symbols   : !=Player  $=Monster");
    puts("            #=Wall");
    puts("            @=Door  <=Down");
    puts("            >=Up    %=Box");
    puts("");
    puts("Press any key for page 2...");
    fflush(stdout);
    (void)render_read_key();

    clear_screen();
    puts("Interaction Flow");
    puts("");
    puts("Door/Stair:");
    puts("  Move into tile -> prompt -> 1/0");
    puts("");
    puts("Box:");
    puts("  Move into tile -> prompt ->");
    puts("  effect message -> item list");
    puts("");
    puts("SPACE/H: back   Q: quit");
    fflush(stdout);
    for (;;) {
        key = render_read_key();
        if (key == ' ' || key == 'h') return 0;
        if (key == 'q') {
            if (help_prompt_quit()) return 1;
            clear_line_and_print(11, "SPACE/H: back   Q: quit");
            fflush(stdout);
        }
    }
}

static int try_open_help_file(FILE **out_fp)
{
    static const char * const k_paths[] = {
        "HELP.TXT",
        "help.txt",
        "./HELP.TXT",
        "./help.txt",
        "A:HELP.TXT",
        "A:help.txt"
    };
    unsigned int i;
    FILE *fp = NULL;

    for (i = 0; i < sizeof(k_paths) / sizeof(k_paths[0]); i++) {
        fp = fopen(k_paths[i], "r");
        if (fp) {
            *out_fp = fp;
            return 1;
        }
    }
    return 0;
}

static int load_help_lines(char lines[HELP_MAX_LINES][HELP_LINE_MAX], int *out_count)
{
    FILE *fp = NULL;
    char buf[HELP_LINE_MAX * 2];
    int count = 0;

    *out_count = 0;
    if (!try_open_help_file(&fp)) return 0;

    while (fgets(buf, sizeof(buf), fp) && count < HELP_MAX_LINES) {
        size_t len = strlen(buf);
        int pos = 0;
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
            buf[len - 1] = '\0';
            len--;
        }
        if (len == 0) {
            lines[count][0] = '\0';
            count++;
            continue;
        }
        while ((size_t)pos < len && count < HELP_MAX_LINES) {
            int remaining = (int)len - pos;
            int cut = remaining;
            if (cut > HELP_WRAP_WIDTH) {
                int i;
                cut = HELP_WRAP_WIDTH;
                for (i = HELP_WRAP_WIDTH; i > 0; i--) {
                    if (buf[pos + i] == ' ') {
                        cut = i;
                        break;
                    }
                }
                if (cut <= 0) cut = HELP_WRAP_WIDTH;
            }
            if (cut >= HELP_LINE_MAX) cut = HELP_LINE_MAX - 1;
            memcpy(lines[count], buf + pos, (size_t)cut);
            lines[count][cut] = '\0';
            count++;
            pos += cut;
            while ((size_t)pos < len && buf[pos] == ' ') pos++;
        }
    }
    fclose(fp);
    *out_count = count;
    return count > 0;
}

static int help_prompt_quit(void)
{
    int key;
    clear_line_and_print(HELP_PAGE_LINES + 3, "Quit? 1=yes 0=no");
    fflush(stdout);
    for (;;) {
        key = render_read_key();
        if (key == '1') return 1;
        if (key == '0') return 0;
    }
}

static int render_show_help_file_scroller(const char lines[HELP_MAX_LINES][HELP_LINE_MAX], int line_count)
{
    int top = 0;

    if (line_count <= 0) return 0;

    for (;;) {
        int i;
        int end = top + HELP_PAGE_LINES;
        if (end > line_count) end = line_count;

        clear_screen();
        printf("Prototype 01 Help (%d-%d/%d)\n\n", top + 1, end, line_count);
        for (i = top; i < end; i++) puts(lines[i]);
        for (; i < top + HELP_PAGE_LINES; i++) putchar('\n');
        puts("W/S scroll  SPACE/H back  Q quit");
        fflush(stdout);

        {
            int key = render_read_key();
            if (key == ' ' || key == 'h') break;
            if (key == 'q') {
                if (help_prompt_quit()) return 1;
                continue;
            }
            if (key == 'w' && top > 0) top--;
            else if (key == 's' && top + HELP_PAGE_LINES < line_count) top++;
        }
    }
    return 0;
}

int render_show_help(void)
{
    char lines[HELP_MAX_LINES][HELP_LINE_MAX];
    int line_count = 0;
    int quit_requested = 0;

    if (load_help_lines(lines, &line_count)) {
        quit_requested = render_show_help_file_scroller(lines, line_count);
        s_render_cache.valid = 0;
        return quit_requested;
    }
    quit_requested = render_show_help_builtin();
    s_render_cache.valid = 0;
    return quit_requested;
}
