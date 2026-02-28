; ubox_set_user_isr — z88dk port
; void ubox_set_user_isr(void (*fn)(void)) __z88dk_fastcall
; HL = function pointer

    SECTION code_user

    PUBLIC _ubox_set_user_isr
    EXTERN ubox_usr_isr

_ubox_set_user_isr:
    di
    ld (ubox_usr_isr), hl
    ei
    ret
