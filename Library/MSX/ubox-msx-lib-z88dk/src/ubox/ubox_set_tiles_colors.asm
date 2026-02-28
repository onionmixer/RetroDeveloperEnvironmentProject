; ubox_set_tiles_colors — z88dk port
; void ubox_set_tiles_colors(const uint8_t *colors) __z88dk_fastcall
; HL = colors pointer

    SECTION code_user

    PUBLIC _ubox_set_tiles_colors
    EXTERN ubox_ldirvm_direct

    defc BG_COLS = $2000

_ubox_set_tiles_colors:
    ld de, BG_COLS
    ld bc, 256 * 8
    push hl
    call ubox_ldirvm_direct
    pop hl

    ld de, BG_COLS + 256 * 8
    ld bc, 256 * 8
    push hl
    call ubox_ldirvm_direct
    pop hl

    ld de, BG_COLS + 256 * 8 * 2
    ld bc, 256 * 8
    jp ubox_ldirvm_direct
