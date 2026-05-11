; variants/ubox_isr_no_altreg.asm
;
; Diagnostic variant: same as ubox-msx-lib-z88dk/src/ubox/ubox_isr.asm
; EXCEPT the alt-register-set save/restore (ex af,af' / push af / exx /
; push bc/hl/de + symmetric pop) is removed. Used to test hypothesis B
; from the spacebar-glitch analysis: whether the ~104-cycle-per-VBlank
; overhead from preserving AF'/BC'/DE'/HL' is what tips the frame
; budget over the edge during PLY_AKM_PLAYSOUNDEFFECT.
;
; This makes the ISR functionally equivalent to the upstream SDCC
; ubox_isr (which never preserved the alt set). It is UNSAFE to ship —
; if any caller in main flow uses EXX or EX AF,AF', the AKM player's
; ISR-side EXX will silently corrupt the alt set. Phase B is OK in
; practice because no kingsvalley game C function touches the alt set.
;
; Symbols match the library version verbatim so this object overrides
; the library copy when linked before `-lubox`.

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
    ; Preserve main register set ONLY (alt set NOT saved — diagnostic).
    push af
    push ix
    push iy
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
    call ubox_call_hl

no_user_isr:
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
