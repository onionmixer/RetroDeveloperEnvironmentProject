; mplayer_is_sound_effect_on — z88dk port (fastcall)
; uint8_t mplayer_is_sound_effect_on(uint8_t chan) __z88dk_fastcall;

    SECTION code_user
    PUBLIC _mplayer_is_sound_effect_on
    EXTERN _PLY_AKM_ISSOUNDEFFECTON

_mplayer_is_sound_effect_on:
    jp _PLY_AKM_ISSOUNDEFFECTON
