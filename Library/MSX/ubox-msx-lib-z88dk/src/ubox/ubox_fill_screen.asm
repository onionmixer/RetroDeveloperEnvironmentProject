; ubox_fill_screen — z88dk port
; void ubox_fill_screen(uint8_t tile) __z88dk_fastcall
; L = tile index

    SECTION code_user

    PUBLIC _ubox_fill_screen
    EXTERN msxbios

    defc FILVRM = $0056
    defc BG_TILE_MAP = $1800

_ubox_fill_screen:
    ld a, l
    ld hl, BG_TILE_MAP
    ld bc, 768
    ld ix, FILVRM
    jp msxbios
