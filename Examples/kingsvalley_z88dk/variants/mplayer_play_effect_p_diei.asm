; variants/mplayer_play_effect_p_diei.asm
;
; Diagnostic variant: mplayer_play_effect_p with `di .. ei` wrap around
; the AKM-side state mutation. Used to test hypothesis C from the
; spacebar-glitch analysis: whether ISR (mplayer_play) preempting in
; the middle of PLY_AKM_PLAYSOUNDEFFECT's effect-channel state update
; causes a torn-state recovery that costs a frame.
;
; Difference from upstream wrapper: the full path that mutates AKM
; state — `mplayer_current_efx` write + the `jp _PLY_AKM_PLAYSOUNDEFFECT`
; tail — runs with interrupts disabled, then `ei; ret` returns to caller.
; Keeps the priority check outside the di/ei window since it doesn't
; mutate state (just reads `mplayer_current_efx`).
;
; Symbols match the library version verbatim so this object overrides
; the library copy when linked before `-lubox` (mplayer wrappers are
; linked individually, not via .lib, but the override-by-link-order
; rule still applies for raw .asm sources).
;
; sccz80 stack at entry:
;   sp+0..1 = ret
;   sp+2..3 = inv_vol      (last pushed)
;   sp+4..5 = chan
;   sp+6..7 = effect_no    (first pushed)

    SECTION code_user
    PUBLIC _mplayer_play_effect_p
    EXTERN _mplayer_is_sound_effect_on
    EXTERN mplayer_current_efx
    EXTERN _PLY_AKM_PLAYSOUNDEFFECT

_mplayer_play_effect_p:
    ld ix, 2
    add ix, sp
    ld b, (ix+0)            ; inv_vol
    ld c, (ix+2)            ; chan
    ld e, (ix+4)            ; effect_no

    push bc
    push de
    ld l, c
    call _mplayer_is_sound_effect_on
    ld a, l
    or a
    pop de
    pop bc
    jr z, play_effect_p_play_diei

    ld a, (mplayer_current_efx)
    dec a
    cp e
    ret nc                  ; current >= new, keep playing current

play_effect_p_play_diei:
    di
    ld a, e
    ld (mplayer_current_efx), a
    call _PLY_AKM_PLAYSOUNDEFFECT
    ei
    ret
