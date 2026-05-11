/*
 * 05_music — MSX-DOS2 variant (auto-derived from main.c).
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
extern void mplayer_engine_load(void);  /* z88dk-port: see main.c */

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
	put_text(11, 11, "MUSIC : ALIENALL");
		
    ubox_enable_screen();

    mplayer_engine_load();   /* z88dk-port: copy AKM player to $A000 RAM */
    mplayer_init(SONG, 0);
  
    /* DOS: ubox_set_user_isr(mplayer_play); — skipped (BIOS context unreachable).
     * Instead we poll mplayer_play() once per VBlank from the main loop.
     * Arkos 2 AKM expects to be ticked every VBlank (50/60 Hz); calling
     * it less often (e.g. once per 2 halts) plays at half tempo. */

    while (1)
    {
        __asm
        halt
        __endasm;
        mplayer_play();        /* AKM tick at 50/60 Hz, matching ROM's ISR rate */
    }
}
