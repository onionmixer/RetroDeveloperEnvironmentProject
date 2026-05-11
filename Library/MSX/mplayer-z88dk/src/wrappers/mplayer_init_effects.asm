; mplayer_init_effects — z88dk port
; void mplayer_init_effects(uint8_t *effects) __z88dk_fastcall;
; fastcall: single 16-bit arg in HL on entry.

    SECTION code_user
    PUBLIC _mplayer_init_effects
    PUBLIC mplayer_current_efx
    EXTERN _PLY_AKM_INITSOUNDEFFECTS

_mplayer_init_effects:
    xor a
    ld (mplayer_current_efx), a
    jp _PLY_AKM_INITSOUNDEFFECTS

    SECTION bss_user
mplayer_current_efx:
    DEFS 1
