; mplayer_stop_effect_channel — z88dk port (fastcall)
; void mplayer_stop_effect_channel(uint8_t chan) __z88dk_fastcall;
; fastcall: 8-bit arg in L on entry.

    SECTION code_user
    PUBLIC _mplayer_stop_effect_channel
    EXTERN _PLY_AKM_STOPSOUNDEFFECTFROMCHANNEL

_mplayer_stop_effect_channel:
    ld a, l
    jp _PLY_AKM_STOPSOUNDEFFECTFROMCHANNEL
