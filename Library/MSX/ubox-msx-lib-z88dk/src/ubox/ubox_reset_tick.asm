; ubox_reset_tick — z88dk port
; void ubox_reset_tick(void)

    SECTION code_user

    PUBLIC _ubox_reset_tick
    EXTERN _ubox_tick

_ubox_reset_tick:
    xor a
    di
    ld (_ubox_tick), a
    ei
    ret
