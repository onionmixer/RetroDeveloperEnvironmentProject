; ubox_set_sprite_attr — z88dk port
; void ubox_set_sprite_attr(const struct sprite_attr *attr, uint8_t sprite) __z88dk_callee
; Stack: [ret:2][sprite:2][attr:2]
; sprite * 4 + SP_ATTRS = VRAM dst, len=4, attr=RAM src

    SECTION code_user

    PUBLIC _ubox_set_sprite_attr
    EXTERN _ubox_write_vm_asm

    defc SP_ATTRS = $1b00

_ubox_set_sprite_attr:
    pop hl          ; ret addr
    pop de          ; sprite (last param, E=sprite)
    ex (sp), hl     ; attr -> HL, ret addr -> stack
    ; HL=attr(RAM src), E=sprite

    push hl         ; save attr
    ld l, e
    ld h, 0         ; HL = sprite
    add hl, hl      ; *2
    add hl, hl      ; *4
    ld bc, SP_ATTRS
    add hl, bc      ; HL = VRAM dst

    ld bc, 4        ; len
    pop de          ; DE = attr (RAM src)
    jp _ubox_write_vm_asm
