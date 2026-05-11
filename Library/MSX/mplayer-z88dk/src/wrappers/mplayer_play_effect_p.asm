; mplayer_play_effect_p — z88dk port (sccz80 calling convention).
; void mplayer_play_effect_p(uint8_t effect_no, uint8_t chan, uint8_t inv_vol);
; Plays effect with priority — checks current channel state, only plays if
; nothing higher priority is active.
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
    ld e, (ix+4)            ; effect_no (kept in E for priority compare)

    ; Save de+bc, query whether channel is currently busy
    push bc
    push de
    ld l, c
    call _mplayer_is_sound_effect_on
    ld a, l
    or a
    pop de
    pop bc
    jr z, play_effect_p_play

    ; Channel busy — only override if new effect has higher priority
    ; (smaller effect_no = higher priority).
    ld a, (mplayer_current_efx)
    dec a
    cp e
    ret nc                  ; current >= new, keep playing current

play_effect_p_play:
    ld a, e
    ld (mplayer_current_efx), a
    jp _PLY_AKM_PLAYSOUNDEFFECT
