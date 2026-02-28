; ubox_disable_screen — z88dk port
; void ubox_disable_screen(void)

    SECTION code_user

    PUBLIC _ubox_disable_screen
    EXTERN msxbios

    defc DISSCR = $0041

_ubox_disable_screen:
    ld ix, DISSCR
    jp msxbios
