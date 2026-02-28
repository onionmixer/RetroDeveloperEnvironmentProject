; ubox_enable_screen — z88dk port
; void ubox_enable_screen(void)

    SECTION code_user

    PUBLIC _ubox_enable_screen
    EXTERN msxbios

    defc ENASCR = $0044

_ubox_enable_screen:
    ld ix, ENASCR
    jp msxbios
