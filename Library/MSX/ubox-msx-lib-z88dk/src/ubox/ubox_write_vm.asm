; ubox_write_vm — z88dk port
; void ubox_write_vm(uint8_t *dst, uint16_t len, const uint8_t *src) __z88dk_callee
; Stack: [ret:2][src:2][len:2][dst:2]
;
; Also provides internal ASM entry:
;   _ubox_write_vm_asm: HL=VRAM dst, BC=len, DE=RAM src

    SECTION code_user

    PUBLIC _ubox_write_vm
    PUBLIC _ubox_write_vm_asm
    EXTERN ubox_ldirvm_direct

_ubox_write_vm:
    pop iy          ; ret addr
    pop de          ; src (last param)
    pop bc          ; len (middle param)
    pop hl          ; dst (first param)
    push iy         ; ret addr back

    ; HL=dst(VRAM), BC=len, DE=src(RAM)
    ; ldirvm_direct wants: HL=src(RAM), DE=dst(VRAM), BC=len
    ex de, hl       ; HL=src(RAM), DE=dst(VRAM)
    jp ubox_ldirvm_direct

; Internal ASM entry point — called by sprite functions
; HL=VRAM dst, BC=len, DE=RAM src
_ubox_write_vm_asm:
    ex de, hl       ; HL=src(RAM), DE=dst(VRAM)
    jp ubox_ldirvm_direct
