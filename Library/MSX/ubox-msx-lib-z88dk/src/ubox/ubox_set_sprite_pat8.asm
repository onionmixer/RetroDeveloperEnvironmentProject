; ubox_set_sprite_pat8 — z88dk port
; void ubox_set_sprite_pat8(const uint8_t *data, uint8_t pattern) __z88dk_callee
; Stack: [ret:2][pattern:2][data:2]
; pattern * 8 + SP_PATTERNS = VRAM dst, len=8, data=RAM src

    SECTION code_user

    PUBLIC _ubox_set_sprite_pat8
    EXTERN _ubox_write_vm_asm

    defc SP_PATTERNS = $3800

_ubox_set_sprite_pat8:
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

    ld bc, 8        ; len
    pop de          ; DE = data (RAM src)
    jp _ubox_write_vm_asm
