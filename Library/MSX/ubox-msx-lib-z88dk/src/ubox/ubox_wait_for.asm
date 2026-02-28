; ubox_wait_for — z88dk port
; void ubox_wait_for(uint8_t frames) __z88dk_fastcall
; L = frames

    SECTION code_user

    PUBLIC _ubox_wait_for
    EXTERN _ubox_wait

_ubox_wait_for:
    ld b, l
wait_for_loop:
    push bc
    call _ubox_wait
    pop bc
    djnz wait_for_loop
    ret
