/*
 * 09_tetris — MSX-DOS2 variant (auto-derived from main.c).
 *
 * Differences vs ROM main.c:
 *   1. ubox_init_isr() / ubox_set_user_isr() calls are commented out —
 *      under MSX-DOS2 the HTIMI hook is called with BIOS ROM paged in on
 *      page 0, making ISR code at $0100+ (TPA) unreachable.
 *   2. ubox_wait() is overridden below to use halt-based polling.
 *
 * Built with z88dk (+msx -subtype=msxdos2 -DMSXDOS). See ../compile_dos.sh.
 */

#include "ubox.h"
#include "game.h"
#include "util.h"

uint8_t g_gamestate = STATE_TITLE;
uint8_t ctl;

void draw_title()
{
    uint8_t i;

    ubox_disable_screen();

    ubox_fill_screen(WHITESPACE_TILE);

    put_text(13, 9, "TETRIS");
    put_text(8, 11, "PRESS SPACE BAR");

    put_text(8, 18, "\0372021 YUZA SOFT");

    ubox_enable_screen();

    while (1)
    {
        ctl = ubox_select_ctl();
        
        if (ctl != UBOX_MSX_CTL_NONE)
            break;

        ubox_wait();
    }

    g_gamestate = STATE_IN_GAME;
}

void draw_game_over()
{
    ubox_disable_screen();

    put_text(11, 10, "GAME  OVER");

    ubox_enable_screen();

    ubox_wait_for(128);

    ubox_disable_screen();
    ubox_fill_screen(WHITESPACE_TILE);
    ubox_enable_screen();

    g_gamestate = STATE_TITLE;

}       

void main()
{
    InitEnvironment();

    draw_title();

    while (1)
    {
        switch (g_gamestate)
        {
        case STATE_GAME_OVER:
            draw_game_over();
            break;
        case STATE_IN_GAME:
            run_game();
            break;
        case STATE_TITLE:
            draw_title();
            break;
        }

        ubox_wait();
    }
}
/*
 * DOS-compatible replacement for ubox_wait().
 * The library version (ubox_wait.asm) relies on the HTIMI ($FD9F) hook,
 * which fails under MSX-DOS2 because the hook fires with BIOS ROM paged
 * on page 0, hiding TPA. Two halts approximate the library's default
 * "wait 2 ticks" behavior.
 */
void ubox_wait(void)
{
    __asm
    halt
    halt
    __endasm;
}
