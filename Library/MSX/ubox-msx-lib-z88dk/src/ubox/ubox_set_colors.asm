; ubox_set_colors — z88dk port
; void ubox_set_colors(uint8_t fg, uint8_t bg, uint8_t border) __z88dk_callee
; Stack: [ret:2][border:2][bg:2][fg:2]

    SECTION code_user

    PUBLIC _ubox_set_colors
    EXTERN msxbios

    defc CHGCLR = $0062
    defc FORCLR = $f3e9
    defc BAKCLR = $f3ea
    defc BDRCLR = $f3eb

_ubox_set_colors:
    pop iy          ; ret addr
    pop bc          ; border (last param, C=border)
    pop de          ; bg (middle param, E=bg)
    pop hl          ; fg (first param, L=fg)
    push iy         ; ret addr back

    ld a, l
    ld (FORCLR), a
    ld a, e
    ld (BAKCLR), a
    ld a, c
    ld (BDRCLR), a

    ld ix, CHGCLR
    jp msxbios
