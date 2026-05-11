; mplayer_stop — z88dk port (trampoline)
; void mplayer_stop(void);

    SECTION code_user
    PUBLIC _mplayer_stop
    EXTERN _PLY_AKM_STOP

_mplayer_stop:
    jp _PLY_AKM_STOP
