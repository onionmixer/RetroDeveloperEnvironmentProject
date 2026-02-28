; ubox_wait — z88dk port
; void ubox_wait(void)

    SECTION code_user

    PUBLIC _ubox_wait
    EXTERN ubox_isr_wait_ticks
    EXTERN ubox_isr_wait_tick

_ubox_wait:
    ld hl, ubox_isr_wait_tick
    ld a, (ubox_isr_wait_ticks)
    ld c, a

wait_loop:
    ld a, (hl)
    cp c
    jr nc, wait_done
    halt
    jr wait_loop

wait_done:
    xor a
    di
    ld (hl), a
    ei
    ret
