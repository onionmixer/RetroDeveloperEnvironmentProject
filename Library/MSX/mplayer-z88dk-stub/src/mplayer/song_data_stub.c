/*
 * Stub data symbols for mplayer.
 *
 * 05_music declares `extern uint8_t SONG[]` and 06_sound additionally
 * declares `extern uint8_t EFFECTS[]`. Real upstream provides these
 * via Arkos-derived asm (alienall.asm / effects.asm / song.asm).
 * Without them the link fails. The stub gives them as empty arrays so
 * the application links and runs (silently).
 */

#include <stdint.h>

/* uppercase symbol names match the C-side `extern uint8_t SONG[]` / `EFFECTS[]`
   declarations — z88dk's sccz80 prefixes user symbols with `_` like SDCC, so
   from C side they are `SONG` and `EFFECTS`. */
uint8_t SONG[1]    = { 0 };
uint8_t EFFECTS[1] = { 0 };
