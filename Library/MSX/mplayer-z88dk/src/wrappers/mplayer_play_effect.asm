; mplayer_play_effect — z88dk port (sccz80 calling convention).
; void mplayer_play_effect(uint8_t effect_no, uint8_t chan, uint8_t inv_vol);
;
; sccz80 caller-cleanup: each uint8_t is zero-extended to 16-bit on stack,
; pushed left-to-right (first arg deepest). For uint8_t the low byte holds
; the value at the smaller offset.
;
; Stack at entry:
;   sp+0..1 = ret
;   sp+2..3 = inv_vol      (last pushed)
;   sp+4..5 = chan
;   sp+6..7 = effect_no    (first pushed)

    SECTION code_user
    PUBLIC _mplayer_play_effect
    EXTERN _PLY_AKM_PLAYSOUNDEFFECT

_mplayer_play_effect:
    ld ix, 2
    add ix, sp
    ld b, (ix+0)        ; inv_vol
    ld c, (ix+2)        ; chan
    ld a, (ix+4)        ; effect_no
    jp _PLY_AKM_PLAYSOUNDEFFECT
