; ubox_wvdp — z88dk port
; void ubox_wvdp(uint8_t reg, uint8_t data) __z88dk_callee
; Stack: [ret:2][data:2][reg:2]
; WRTVDP expects: B=data, C=reg

    SECTION code_user

    PUBLIC _ubox_wvdp
    EXTERN msxbios

    defc WRTVDP = $0047

_ubox_wvdp:
    pop hl          ; ret addr
    pop de          ; data (last param, E=data)
    ex (sp), hl     ; reg -> HL (L=reg), ret addr -> stack
    ld c, l         ; C = reg
    ld b, e         ; B = data
    ld ix, WRTVDP
    jp msxbios
