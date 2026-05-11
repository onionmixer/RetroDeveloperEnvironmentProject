; mplayer_play — z88dk port (trampoline)
; void mplayer_play(void);

    SECTION code_user
    PUBLIC _mplayer_play
    EXTERN _PLY_AKM_PLAY

_mplayer_play:
    jp _PLY_AKM_PLAY
