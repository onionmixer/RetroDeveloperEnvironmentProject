#include <stdint.h>
#include "ubox.h"
#include "mplayer.h"

#define LOCAL
#include "tiles.h"

extern uint8_t SONG[];

/* z88dk-port: mplayer_engine_load() must be called once before any
 * mplayer_* function. It LDIR-copies the AKM player binary blob from
 * its INCBIN location (anywhere in code) to $A000 RAM, where the
 * binary's internal absolute jumps expect to find themselves.
 * Provided by Library/MSX/mplayer-z88dk/src/akm/akm_bridge.asm.
 */
extern void mplayer_engine_load(void);

#define WHITESPACE_TILE 129

void put_text(uint8_t x, uint8_t y, const uint8_t *text)
{
    while (*text)
        ubox_put_tile(x++, y, *text++ + 128 - 31);
}

void main()
{
    
    ubox_init_isr(2);

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
  
    ubox_set_user_isr(mplayer_play);

	mplayer_play();

    while (1)
    {	
        ubox_wait();
    }
}
