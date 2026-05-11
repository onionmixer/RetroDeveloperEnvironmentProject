/*
 * 06_sound — MSX-DOS2 variant (auto-derived from main.c).
 *
 * Differences vs ROM main.c:
 *   1. ubox_init_isr() / ubox_set_user_isr() calls are commented out —
 *      under MSX-DOS2 the HTIMI hook is called with BIOS ROM paged in on
 *      page 0, making ISR code at $0100+ (TPA) unreachable.
 *   2. ubox_wait() is overridden below to use halt-based polling.
 *
 * Built with z88dk (+msx -subtype=msxdos2 -DMSXDOS). See ../compile_dos.sh.
 */

#include <stdint.h>
#include "ubox.h"
#include "mplayer.h"

#define LOCAL
#include "tiles.h"

extern uint8_t SONG[];
extern uint8_t EFFECTS[];
extern void mplayer_engine_load(void);  /* z88dk-port */

#define EFX_CHAN_NO 2

enum effects
{
    EFX_NONE = 0,
    EFX_START,
    EFX_BATTERY,
    EFX_ELEVATOR,
    EFX_HIT,
    EFX_DEAD,
};

#define WHITESPACE_TILE 129

void put_text(uint8_t x, uint8_t y, const uint8_t *text)
{
    while (*text)
        ubox_put_tile(x++, y, *text++ + 128 - 31);
}

void main()
{
    
    /* DOS: ubox_init_isr(2); — skipped */

    ubox_set_mode(2);
  
    ubox_set_colors(1, 1, 1);

    ubox_disable_screen();

    ubox_set_tiles(tiles);
    ubox_set_tiles_colors(tiles_colors);
  
    ubox_fill_screen(WHITESPACE_TILE);
	put_text(11, 11, "EFFECTS TEST");
		
    ubox_enable_screen();

    mplayer_engine_load();   /* z88dk-port: copy AKM player to $A000 RAM */
    mplayer_init(SONG, 0);
	mplayer_init_effects(EFFECTS);
  
    /* DOS: ubox_set_user_isr(mplayer_play); — skipped (BIOS context unreachable).
     * Instead we poll mplayer_play() at every VBlank (50/60 Hz) so the AKM
     * tempo matches the ROM build's ISR-driven rate. Key/effect handling
     * runs every 2 frames (= ROM's ubox_wait pacing). */

    while (1)
    {
        /* frame 1 — tick player */
        __asm
        halt
        __endasm;
        mplayer_play();

        /* frame 2 — tick + key handling at half-frame rate */
        __asm
        halt
        __endasm;
        mplayer_play();

        if (ubox_read_keys(7) == UBOX_MSX_KEY_ESC)
            break;

        if (ubox_read_keys(0) == UBOX_MSX_KEY_1)
            mplayer_play_effect_p(EFX_START, EFX_CHAN_NO, 0);

        if (ubox_read_keys(0) == UBOX_MSX_KEY_2)
            mplayer_play_effect_p(EFX_BATTERY, EFX_CHAN_NO, 0);

        if (ubox_read_keys(0) == UBOX_MSX_KEY_3)
            mplayer_play_effect_p(EFX_ELEVATOR, EFX_CHAN_NO, 0);

        if (ubox_read_keys(0) == UBOX_MSX_KEY_4)
            mplayer_play_effect_p(EFX_HIT, EFX_CHAN_NO, 0);

        if (ubox_read_keys(0) == UBOX_MSX_KEY_5)
            mplayer_play_effect_p(EFX_DEAD, EFX_CHAN_NO, 0);
    }
}
