; mplayer_init — z88dk port (sccz80 calling convention).
; void mplayer_init(uint8_t *song, uint8_t sub_song);
;
; sccz80 caller-cleanup: ALL args zero-extended to 16-bit on stack, pushed
; left-to-right (first arg pushed first → deepest). For uint8_t the low
; byte holds the value, high byte is 0.
;
; Stack at entry: [ret_lo, ret_hi, sub_song_lo, sub_song_hi, song_lo, song_hi]
;   sp+0..1 = ret
;   sp+2..3 = sub_song (last pushed; low byte = value)
;   sp+4..5 = song     (first pushed)
;
; (NOTE: SDCC's z80 convention packs uint8_t as a single byte and pushes
;  right-to-left, so the original wrapper read song at sp+2 and sub_song
;  at sp+4. Copying that layout to z88dk swapped the args; codex caught
;  this. See PLAN_UBOX_EXAMPLES.md for the full diagnosis.)

    SECTION code_user
    PUBLIC _mplayer_init
    EXTERN _PLY_AKM_INIT

_mplayer_init:
    ld ix, 2
    add ix, sp
    ld a, (ix+0)        ; sub_song (low byte at sp+2)
    ld l, (ix+2)        ; song lo
    ld h, (ix+3)        ; song hi
    di
    call _PLY_AKM_INIT
    ei
    ret
