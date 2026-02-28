; ubox_set_mode — z88dk port
; void ubox_set_mode(uint8_t mode) __z88dk_fastcall
; L = mode

    SECTION code_user

    PUBLIC _ubox_set_mode
    EXTERN msxbios

    defc CHGMOD = $005f

_ubox_set_mode:
    ld a, l
    ld ix, CHGMOD
    jp msxbios
