; ubox_put_tile — z88dk port
; void ubox_put_tile(uint8_t x, uint8_t y, uint8_t tile) __z88dk_callee
; Stack: [ret:2][tile:2][y:2][x:2]
; WRTVRM: A=data, HL=VRAM addr

    SECTION code_user

    PUBLIC _ubox_put_tile
    EXTERN msxbios

    defc WRTVRM = $004d
    defc BG_TILE_MAP = $1800

_ubox_put_tile:
    pop iy          ; ret addr
    pop bc          ; tile (last param, C=tile)
    pop de          ; y (middle param, E=y)
    pop hl          ; x (first param, L=x)
    push iy         ; ret addr back

    ; compute VRAM address: y*32 + x + BG_TILE_MAP
    ld a, c         ; A = tile (save for WRTVRM)
    ld c, l         ; C = x
    ld h, 0
    ld l, e         ; HL = y
    add hl, hl      ; y*2
    add hl, hl      ; y*4
    add hl, hl      ; y*8
    add hl, hl      ; y*16
    add hl, hl      ; y*32
    ld b, 0
    add hl, bc      ; + x

    ld bc, BG_TILE_MAP
    add hl, bc

    ld ix, WRTVRM
    jp msxbios
