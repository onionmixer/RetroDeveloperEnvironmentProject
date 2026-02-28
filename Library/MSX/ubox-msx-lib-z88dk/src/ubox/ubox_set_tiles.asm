; ubox_set_tiles — z88dk port
; void ubox_set_tiles(const uint8_t *tiles) __z88dk_fastcall
; HL = tiles pointer

    SECTION code_user

    PUBLIC _ubox_set_tiles
    EXTERN ubox_ldirvm_direct

    defc BG_TILES = $0000

_ubox_set_tiles:
    ld de, BG_TILES
    ld bc, 256 * 8
    push hl
    call ubox_ldirvm_direct
    pop hl

    ld de, BG_TILES + 256 * 8
    ld bc, 256 * 8
    push hl
    call ubox_ldirvm_direct
    pop hl

    ld de, BG_TILES + 256 * 8 * 2
    ld bc, 256 * 8
    jp ubox_ldirvm_direct
