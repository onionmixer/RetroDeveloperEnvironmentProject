; ubox_set_sprite_pat16_flip — z88dk port
; void ubox_set_sprite_pat16_flip(const uint8_t *data, uint8_t pattern) __z88dk_callee
; Stack: [ret:2][pattern:2][data:2]
;
; Horizontal flip of 16x16 sprite: right half (data+16) goes to left VRAM,
; left half (data) goes to right VRAM (VRAM+16).

    SECTION code_user

    PUBLIC _ubox_set_sprite_pat16_flip
    EXTERN msxbios

    defc WRTVRM = $004d
    defc SP_PATTERNS = $3800

_ubox_set_sprite_pat16_flip:
    pop hl          ; ret addr
    pop de          ; pattern (last param, E=pattern)
    ex (sp), hl     ; data -> HL, ret addr -> stack
    ; HL=data(RAM src), E=pattern

    push hl         ; save data
    ld l, e
    ld h, 0         ; HL = pattern
    add hl, hl      ; *2
    add hl, hl      ; *4
    add hl, hl      ; *8
    add hl, hl      ; *16
    add hl, hl      ; *32
    ld bc, SP_PATTERNS
    add hl, bc      ; HL = VRAM dst

    pop de          ; DE = data
    push de         ; save data again

    ; flip right half (data+16) -> VRAM dst
    ld bc, 16
    ex de, hl       ; DE=VRAM dst, HL=data
    add hl, bc      ; HL=data+16
    ex de, hl       ; HL=VRAM dst, DE=data+16
    call flip16     ; after: HL=VRAM+16, DE=data+32

    ; flip left half (data) -> VRAM dst+16
    pop de          ; DE = original data
    call flip16

    ret

flip16:
    ld b, 16
flip0:
    call flip_and_copy
    inc hl
    inc de
    djnz flip0
    ret

flip_and_copy:
    ld a, (de)
    ld c, a
    rlca
    rlca
    xor c
    and $aa
    xor c
    ld c, a
    rlca
    rlca
    rlca
    rrc c
    xor c
    and $66
    xor c

    ld ix, WRTVRM
    jp msxbios
