; ubox_read_vm — z88dk port
; void ubox_read_vm(uint8_t *dst, uint16_t len, const uint8_t *src) __z88dk_callee
; Stack: [ret:2][src:2][len:2][dst:2]
; LDIRMV: HL=VRAM src, DE=RAM dst, BC=len

    SECTION code_user

    PUBLIC _ubox_read_vm
    EXTERN ubox_ldirmv_direct

_ubox_read_vm:
    pop iy          ; ret addr
    pop hl          ; src (last param = VRAM src)
    pop bc          ; len (middle param)
    pop de          ; dst (first param = RAM dst)
    push iy         ; ret addr back

    jp ubox_ldirmv_direct
