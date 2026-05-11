/*
 * 10_breakout — MSX-DOS2 variant.
 *
 * Same content as main.c. The ROM/DOS divergence lives in:
 *   - util.c: #ifndef MSXDOS gates around ubox_init_isr/ubox_set_user_isr
 *   - main_dos.c (this file): provides a halt-based ubox_wait() override
 *
 * Built with z88dk (+msx -subtype=msxdos2 -DMSXDOS). See ../compile_dos.sh.
 */

#include <stdint.h>
#include "ubox.h"
#include "game.h"
#include "util.h"

uint8_t ctl;
uint8_t g_gamestate = STATE_TITLE;

void draw_title()
{
    ubox_disable_screen();
    ubox_fill_screen(WHITESPACE_TILE);
    put_text(11, 9, "BREAK OUT");
    put_text(8, 11, "PRESS SPACE BAR");
    put_text(8, 18, "\0372021 YUZA SOFT");
    ubox_enable_screen();

    while (1) {
        ctl = ubox_select_ctl();
        if (ctl != UBOX_MSX_CTL_NONE)
            break;
        ubox_wait();
    }
    g_gamestate = STATE_IN_GAME;
}

void draw_end_game()
{
    ubox_disable_screen();
    ubox_fill_screen(WHITESPACE_TILE);
    put_text(3, 9, "GAME CLEAR!");
    put_text(3, 12, "(PRESS ESC)");
    ubox_enable_screen();
    while (1) {
        ctl = ubox_select_ctl();
        if (ctl != UBOX_MSX_CTL_NONE)
            break;
        ubox_wait();
    }
    g_gamestate = STATE_TITLE;
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

int main(void)
{
    InitEnvironment();
    draw_title();

    while (1) {
        switch (g_gamestate) {
        case STATE_GAME_OVER:  draw_game_over(); break;
        case STATE_GAME_CLEAR: draw_end_game();  break;
        case STATE_IN_GAME:    run_game();       break;
        case STATE_TITLE:      draw_title();     break;
        }
        ubox_wait();
    }
    return 0;   /* unreachable */
}

/*
 * DOS-compatible replacement for ubox_wait().
 * The library version (ubox_wait.asm) relies on the HTIMI ($FD9F) hook,
 * which fails under MSX-DOS2 because the hook fires with BIOS ROM paged
 * on page 0, hiding TPA. Two halts approximate the library's "wait 2
 * ticks" default.
 */
void ubox_wait(void)
{
    __asm
    halt
    halt
    __endasm;
}
