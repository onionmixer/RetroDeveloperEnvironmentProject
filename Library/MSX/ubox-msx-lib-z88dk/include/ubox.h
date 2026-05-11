/*
 * ubox MSX lib — z88dk port
 *
 * Original: Copyright (C) 2020-2024 by Juan J. Martinez <jjm@usebox.net>
 * z88dk port: 2026
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _UBOX_MSX_H
#define _UBOX_MSX_H

#include <stdint.h>

// Screen and VDP functions

void ubox_set_mode(uint8_t mode) __z88dk_fastcall;
void ubox_enable_screen(void);
void ubox_disable_screen(void);
void ubox_set_colors(uint8_t fg, uint8_t bg, uint8_t border) __z88dk_callee;
void ubox_write_vm(uint8_t *dst, uint16_t len, const uint8_t *src) __z88dk_callee;
void ubox_read_vm(uint8_t *dst, uint16_t len, const uint8_t *src) __z88dk_callee;
void ubox_wvdp(uint8_t reg, uint8_t data) __z88dk_callee;
uint8_t ubox_get_vsync_freq(void);

/* Wait one VBlank (single `halt`).
 * Was a macro in the SDCC port (`do { __asm halt __endasm; } while(0)`),
 * but sccz80's preprocessor collapses the body onto a single line and
 * fails with "Unknown symbol: __asm". Exported as a real function in
 * the z88dk port (src/ubox/ubox_wait_vsync.asm). */
void ubox_wait_vsync(void);

// Tile functions

void ubox_set_tiles(const uint8_t *tiles) __z88dk_fastcall;
void ubox_set_tiles_colors(const uint8_t *colors) __z88dk_fastcall;
void ubox_put_tile(uint8_t x, uint8_t y, uint8_t tile) __z88dk_callee;
uint8_t ubox_get_tile(uint8_t x, uint8_t y) __z88dk_callee;
void ubox_fill_screen(uint8_t tile) __z88dk_fastcall;

// Interrupt and clock functions

/* Install the HTIMI ($FD9F) hook that drives ubox's ISR machinery
 * (ubox_set_user_isr, ubox_wait, ubox_tick) and clear ubox_usr_isr.
 *
 * `wait_ticks` is the interrupts-per-tick threshold for `ubox_wait()`
 * ONLY. It does NOT throttle the user ISR: any function registered
 * via `ubox_set_user_isr()` fires every VBlank (50/60 Hz) regardless
 * of `wait_ticks`. Common usage: pass 2 to make `ubox_wait()` pace
 * the main loop at half the VBlank rate while the user ISR (e.g.
 * Arkos 2 `mplayer_play`) still ticks every frame.
 *
 * ROM only — under MSX-DOS2 the HTIMI hook fires with BIOS ROM paged
 * on page 0, hiding TPA, so any user ISR at $0100+ is unreachable.
 * DOS builds must NOT call this and must define their own `ubox_wait`
 * (see `ubox_wait` below). */
void ubox_init_isr(uint8_t wait_ticks) __z88dk_fastcall;
void ubox_set_user_isr(void (*fn)(void)) __z88dk_fastcall;

/* Wait `wait_ticks` interrupts (set by ubox_init_isr) then return.
 *
 * The library implementation polls ubox_isr_wait_tick (driven by the
 * HTIMI hook) and is therefore ROM-only. For MSX-DOS2 builds, define
 * your own `ubox_wait` in your code — the z88dk linker picks the
 * user-defined symbol over the library version. The canonical halt
 * pattern:
 *
 *     void ubox_wait(void) {
 *         __asm
 *         halt
 *         __endasm;
 *     }
 *
 * Use a single `halt` for one VBlank (matches user-ISR rate). Use
 * two halts for the library's default 2-tick pacing. */
void ubox_wait(void);
void ubox_wait_for(uint8_t frames) __z88dk_fastcall;

/* Cross-platform stub — always returns 1 on MSX (no host event loop).
 * Lets `while (ubox_update()) { ... }` compile identically on SDL2/Allegro
 * desktop ports and the MSX target. (kingsvalley fork extension.) */
uint8_t ubox_update(void);

/* 16-bit Galois LFSR PRNG. Internal seed lives in bss_user.
 * (kingsvalley fork extension; ubox-msx-lib-1.2.0 upstream does not have it.) */
void ubox_randomize(uint16_t seed) __z88dk_fastcall;
uint16_t ubox_random(void);
extern uint8_t ubox_tick;
void ubox_reset_tick(void);

// Sprite functions

struct sprite_attr {
    uint8_t y;
    uint8_t x;
    uint8_t pattern;
    uint8_t attr;
};

void ubox_set_sprite_pat8(const uint8_t *data, uint8_t pattern) __z88dk_callee;
void ubox_set_sprite_pat8_flip(const uint8_t *data, uint8_t pattern) __z88dk_callee;
void ubox_set_sprite_attr(const struct sprite_attr *attr, uint8_t sprite) __z88dk_callee;
void ubox_set_sprite_pat16(const uint8_t *data, uint8_t pattern) __z88dk_callee;
void ubox_set_sprite_pat16_flip(const uint8_t *data, uint8_t pattern) __z88dk_callee;

// Control functions

uint8_t ubox_select_ctl(void);
uint8_t ubox_read_ctl(uint8_t control) __z88dk_fastcall;
uint8_t ubox_read_keys(uint8_t row) __z88dk_fastcall;

#define UBOX_MSX_CTL_CURSOR  0
#define UBOX_MSX_CTL_PORT1   1
#define UBOX_MSX_CTL_PORT2   2
#define UBOX_MSX_CTL_NONE    0xff

#define UBOX_MSX_CTL_UP      1
#define UBOX_MSX_CTL_DOWN    2
#define UBOX_MSX_CTL_LEFT    4
#define UBOX_MSX_CTL_RIGHT   8
#define UBOX_MSX_CTL_FIRE1   16
#define UBOX_MSX_CTL_FIRE2   32

// row 0
#define UBOX_MSX_KEY_7           0x80
#define UBOX_MSX_KEY_6           0x40
#define UBOX_MSX_KEY_5           0x20
#define UBOX_MSX_KEY_4           0x10
#define UBOX_MSX_KEY_3           0x08
#define UBOX_MSX_KEY_2           0x04
#define UBOX_MSX_KEY_1           0x02
#define UBOX_MSX_KEY_0           0x01

// row 1
#define UBOX_MSX_KEY_SEMI        0x80
#define UBOX_MSX_KEY_CSBRACKET   0x40
#define UBOX_MSX_KEY_OSBRACKET   0x20
#define UBOX_MSX_KEY_BSLASH      0x10
#define UBOX_MSX_KEY_EQUAL       0x08
#define UBOX_MSX_KEY_MINUS       0x04
#define UBOX_MSX_KEY_9           0x02
#define UBOX_MSX_KEY_8           0x01

// row 2
#define UBOX_MSX_KEY_B           0x80
#define UBOX_MSX_KEY_A           0x40
#define UBOX_MSX_KEY_FSLASH      0x10
#define UBOX_MSX_KEY_DOT         0x08
#define UBOX_MSX_KEY_COMMA       0x04
#define UBOX_MSX_KEY_QUOTE       0x02
#define UBOX_MSX_KEY_TICK        0x01

// row 3
#define UBOX_MSX_KEY_J           0x80
#define UBOX_MSX_KEY_I           0x40
#define UBOX_MSX_KEY_H           0x20
#define UBOX_MSX_KEY_G           0x10
#define UBOX_MSX_KEY_F           0x08
#define UBOX_MSX_KEY_E           0x04
#define UBOX_MSX_KEY_D           0x02
#define UBOX_MSX_KEY_C           0x01

// row 4
#define UBOX_MSX_KEY_R           0x80
#define UBOX_MSX_KEY_Q           0x40
#define UBOX_MSX_KEY_P           0x20
#define UBOX_MSX_KEY_O           0x10
#define UBOX_MSX_KEY_N           0x08
#define UBOX_MSX_KEY_M           0x04
#define UBOX_MSX_KEY_L           0x02
#define UBOX_MSX_KEY_K           0x01

// row 5
#define UBOX_MSX_KEY_Z           0x80
#define UBOX_MSX_KEY_Y           0x40
#define UBOX_MSX_KEY_X           0x20
#define UBOX_MSX_KEY_W           0x10
#define UBOX_MSX_KEY_V           0x08
#define UBOX_MSX_KEY_U           0x04
#define UBOX_MSX_KEY_T           0x02
#define UBOX_MSX_KEY_S           0x01

// row 6
#define UBOX_MSX_KEY_F3          0x80
#define UBOX_MSX_KEY_F2          0x40
#define UBOX_MSX_KEY_F1          0x20
#define UBOX_MSX_KEY_CODE        0x10
#define UBOX_MSX_KEY_CAP         0x08
#define UBOX_MSX_KEY_GRAPH       0x04
#define UBOX_MSX_KEY_CTRL        0x02
#define UBOX_MSX_KEY_SHIFT       0x01

// row 7
#define UBOX_MSX_KEY_RET         0x80
#define UBOX_MSX_KEY_SEL         0x40
#define UBOX_MSX_KEY_BS          0x20
#define UBOX_MSX_KEY_STOP        0x10
#define UBOX_MSX_KEY_TAB         0x08
#define UBOX_MSX_KEY_ESC         0x04
#define UBOX_MSX_KEY_F5          0x02
#define UBOX_MSX_KEY_F4          0x01

// row 8
#define UBOX_MSX_KEY_RIGHT       0x80
#define UBOX_MSX_KEY_DOWN        0x40
#define UBOX_MSX_KEY_UP          0x20
#define UBOX_MSX_KEY_LEFT        0x10
#define UBOX_MSX_KEY_DEL         0x08
#define UBOX_MSX_KEY_INS         0x04
#define UBOX_MSX_KEY_HOME        0x02
#define UBOX_MSX_KEY_SPACE       0x01

#endif // _UBOX_MSX_H
