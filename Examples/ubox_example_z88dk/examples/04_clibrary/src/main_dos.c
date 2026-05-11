/*
 * 04_clibrary — MSX-DOS2 variant (auto-derived from main.c).
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
#include <stdio.h>

#define LOCAL
#include "tiles.h"

#define WHITESPACE_TILE 129

void put_text(uint8_t x, uint8_t y, const uint8_t *text)
{
    while (*text)
        ubox_put_tile(x++, y, *text++ + 128 - 31);
}

uint8_t g_x = 0;
void _putchar(char character)
{
	ubox_put_tile(g_x++, 0, character + 128 - 31);	
}

char buffer[10];

int counter = 0;

int g_count = 0;


void my_isr()
{
  
	 ++counter;
	 
	 if(counter >= 30)
	 {		 		 
		 g_count++;
		 counter = 0;
	 }
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
	put_text(11, 11, "COUNT : ");
	sprintf(buffer, "%d", g_count);
	put_text(11 + 9, 11, buffer);
		
    ubox_enable_screen();
	
	/* DOS: ubox_set_user_isr(my_isr); — skipped */
	
	int temp_counter = 0;

	printf("04 CLIBRARY\n");

    while (1)
    {			
		if(temp_counter != g_count)
		{
			sprintf(buffer, "%d", g_count);
			put_text(11 + 9, 11, buffer);
			temp_counter = g_count;
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
