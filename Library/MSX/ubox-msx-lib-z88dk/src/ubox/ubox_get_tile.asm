; ubox_get_tile — z88dk port
; uint8_t ubox_get_tile(uint8_t x, uint8_t y) __z88dk_callee
; Stack: [ret:2][y:2][x:2]
; Returns: L = tile index at (x,y)
;
; Bug fix from original: clear B before add hl,bc to avoid
; corruption when y>0 shifts garbage into H via carry.

    SECTION code_user

    PUBLIC _ubox_get_tile
    EXTERN msxbios

    defc RDVRM = $004a
    defc BG_TILE_MAP = $1800

_ubox_get_tile:
    pop hl          ; ret addr
    pop de          ; y (last param, E=y)
    ex (sp), hl     ; x -> HL (L=x), ret addr -> stack
    ; L=x, E=y

    ld c, l         ; C = x
    ld h, 0
    ld l, e         ; HL = y

    add hl, hl      ; y*2
    add hl, hl      ; y*4
    add hl, hl      ; y*8
    add hl, hl      ; y*16
    add hl, hl      ; y*32

    ld b, 0         ; bug fix: clear B before add
    add hl, bc      ; + x

    ld bc, BG_TILE_MAP
    add hl, bc

    ld ix, RDVRM
    call msxbios    ; A = tile
    ld l, a
    ret
