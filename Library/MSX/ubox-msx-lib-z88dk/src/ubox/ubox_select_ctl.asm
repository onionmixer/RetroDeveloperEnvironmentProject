; ubox_select_ctl — z88dk port
; uint8_t ubox_select_ctl(void)
; Returns: L = selected control (0=cursor, 1=port1, 2=port2, 0xff=none)

    SECTION code_user

    PUBLIC _ubox_select_ctl

    defc GTTRIG = $00d8

_ubox_select_ctl:
    ld b, 3
loop:
    ld a, b
    dec a
    push bc
    call GTTRIG
    pop bc
    or a
    jr nz, trigger
    djnz loop

    ; 2nd button
    ld b, 4
    ld a, b
    push bc
    call GTTRIG
    pop bc
    or a
    jr nz, trigger_b

    dec b
    ld a, b
    push bc
    call GTTRIG
    pop bc
    or a
    jr nz, trigger_b

    ld l, $ff
    ret
trigger_b:
    dec b
trigger:
    dec b
    ld l, b
    ret
