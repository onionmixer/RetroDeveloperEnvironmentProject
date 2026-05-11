/*
 * mplayer-z88dk-stub
 * ------------------
 * No-op replacement for Juan J. Martinez's mplayer / Arkos 2 AKM player.
 * Lets ubox_example_z88dk's 05_music / 06_sound (and any other AKM-using
 * code) link & boot — but produces NO sound. The real Arkos 2 player
 * (rasm-macro z80asm, ~2000 lines) needs separate porting; see
 * `Library/MSX/mplayer-z88dk-stub/README.md` and PLAN_UBOX_EXAMPLES.md
 * Phase 3-C for the deferred effort.
 *
 * Symbol coverage (matches mplayer.h API surface):
 *   - mplayer_init, mplayer_init_effects
 *   - mplayer_play, mplayer_stop
 *   - mplayer_play_effect, mplayer_play_effect_p
 *   - mplayer_stop_effect_channel
 *   - mplayer_is_sound_effect_on
 *
 * Data symbol coverage (defined in song_data_stub.c):
 *   - SONG[]    (empty, 1 byte)
 *   - EFFECTS[] (empty, 1 byte)
 */

#include <stdint.h>
#include "mplayer.h"

void mplayer_init(uint8_t *song, uint8_t sub_song)
{
    (void)song;
    (void)sub_song;
}

void mplayer_init_effects(uint8_t *effects) __z88dk_fastcall
{
    (void)effects;
}

void mplayer_play(void)
{
    /* no-op: real impl would tick the AKM player */
}

void mplayer_stop(void)
{
    /* no-op */
}

void mplayer_play_effect(uint8_t effect_no, uint8_t chan, uint8_t inv_vol)
{
    (void)effect_no;
    (void)chan;
    (void)inv_vol;
}

void mplayer_play_effect_p(uint8_t effect_no, uint8_t chan, uint8_t inv_vol)
{
    (void)effect_no;
    (void)chan;
    (void)inv_vol;
}

void mplayer_stop_effect_channel(uint8_t chan) __z88dk_fastcall
{
    (void)chan;
}

uint8_t mplayer_is_sound_effect_on(uint8_t chan) __z88dk_fastcall
{
    (void)chan;
    return 0;   /* always "off" */
}
