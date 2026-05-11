; ubox_isr — z88dk port
; void ubox_init_isr(uint8_t wait_ticks) __z88dk_fastcall
; L = wait_ticks

    SECTION code_user

    PUBLIC _ubox_init_isr
    PUBLIC _ubox_tick
    PUBLIC ubox_isr_wait_ticks
    PUBLIC ubox_isr_wait_tick
    PUBLIC ubox_usr_isr

    defc HTIMI_HOOK = $fd9f
    defc SCNCNT = $f3f6
    defc REPCNT = $f3f7

_ubox_init_isr:
    ld a, l
    di
    ld (ubox_isr_wait_ticks), a
    xor a
    ld (ubox_isr_wait_tick), a
    ld (_ubox_tick), a
    ld (ubox_usr_isr), a
    ld (ubox_usr_isr+1), a
    ld hl, ubox_isr
    ex de, hl
    ld hl, HTIMI_HOOK
    ld a, $c3
    ld (hl), a
    inc hl
    ld (hl), e
    inc hl
    ld (hl), d
    ei
    ret

ubox_isr:
    ; Preserve main register set.
    push af
    push ix
    push iy
    push bc
    push hl
    push de
    ; Preserve alternate register set too — required because user ISR
    ; bodies (notably the Arkos 2 AKM player) use EXX / EX AF,AF'
    ; extensively. Earlier the SDCC version skipped this because the
    ; SDCC examples never used the alternate set in main flow. The
    ; z88dk port saves them unconditionally so ISR-driven AKM stays
    ; safe to combine with any caller code.
    ex af, af'
    push af
    exx
    push bc
    push hl
    push de

    ; stop BIOS reading keyboard buffer
    xor a
    ld (SCNCNT), a
    ld (REPCNT), a

    ld hl, ubox_isr_wait_tick
    inc (hl)
    inc hl
    inc (hl)
    inc hl
    inc hl

    ; check user isr, only MSB
    ld a, (hl)
    or a
    jr z, no_user_isr

    dec hl
    ld l, (hl)
    ld h, a
    ; local replacement for ___sdcc_call_hl
    call ubox_call_hl

no_user_isr:
    ; Restore alternate set
    pop de
    pop hl
    pop bc
    exx
    pop af
    ex af, af'
    ; Restore main set
    pop de
    pop hl
    pop bc
    pop iy
    pop ix
    pop af
    ret

ubox_call_hl:
    jp (hl)

    SECTION bss_user

ubox_isr_wait_ticks: DEFS 1
ubox_isr_wait_tick:  DEFS 1
_ubox_tick:          DEFS 1
ubox_usr_isr:        DEFS 2
