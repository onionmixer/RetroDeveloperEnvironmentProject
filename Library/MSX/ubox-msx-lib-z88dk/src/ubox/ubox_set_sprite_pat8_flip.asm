; ubox_set_sprite_pat8_flip — z88dk port
; void ubox_set_sprite_pat8_flip(const uint8_t *data, uint8_t pattern) __z88dk_callee
; Stack: [ret:2][pattern:2][data:2]

    SECTION code_user

    PUBLIC _ubox_set_sprite_pat8_flip
    EXTERN msxbios

    defc WRTVRM = $004d
    defc SP_PATTERNS = $3800

_ubox_set_sprite_pat8_flip:
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
    ld bc, SP_PATTERNS
    add hl, bc      ; HL = VRAM dst
    pop de          ; DE = data (RAM src)

    ld b, 8
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
