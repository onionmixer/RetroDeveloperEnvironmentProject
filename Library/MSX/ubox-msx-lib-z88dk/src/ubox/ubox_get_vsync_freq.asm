; ubox_get_vsync_freq — z88dk port
; uint8_t ubox_get_vsync_freq(void)
; Returns: L = 0 (60Hz) or 1 (50Hz)

    SECTION code_user

    PUBLIC _ubox_get_vsync_freq

_ubox_get_vsync_freq:
    ld a, ($002b)
    and 128
    jr z, is_60hz
    ld l, 1
    ret
is_60hz:
    ld l, 0
    ret
