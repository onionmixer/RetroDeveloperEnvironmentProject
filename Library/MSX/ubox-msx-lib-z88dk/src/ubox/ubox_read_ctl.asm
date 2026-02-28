; ubox_read_ctl — z88dk port (direct PPI/PSG I/O)
; uint8_t ubox_read_ctl(uint8_t control) __z88dk_fastcall
; L = control (0=cursor, 1=joy1, 2=joy2)
; Returns: L = bitmask (up/down/left/right/fire1/fire2)
;
; Uses direct PPI/PSG I/O to avoid CALSLT register issues under MSX-DOS2.

    SECTION code_user

    PUBLIC _ubox_read_ctl

_ubox_read_ctl:
    ld a, l
    or a
    jr z, is_keyb
    dec a
    jr z, is_joy1
    dec a
    jr z, is_joy2

    ld l, 0
    ret

is_joy2:
    ; select
    ld e, $cf
    jr call_psg

is_joy1:
    ; select
    ld e, $8f

call_psg:
    ; WRTPSG: register $0F, data E
    ld a, $0f
    out ($A0), a        ; select PSG register $0F
    ld a, e
    out ($A1), a        ; write data

    ; RDPSG: register $0E
    ld a, $0e
    di
    out ($A0), a        ; select PSG register $0E
    in a, ($A2)         ; read joystick data
    ei
    cpl
    and $3f
    ld e, a

    ; button 2 (M) — SNSMAT row 4
    ld a, 4
    call snsmat_direct
    rra
    rra
    rra
    ld a, $20
    jr nc, joy_extra_m
    xor a
joy_extra_m:
    or e
    ld l, a
    ret

is_keyb:
    ; button 2 (M) — SNSMAT row 4
    ld a, 4
    call snsmat_direct
    rra
    rra
    rra
    ld e, $20
    jr nc, is_keyb_dir

    ld e, 0

is_keyb_dir:
    ; direction — SNSMAT row 8
    ld a, 8
    call snsmat_direct
    cpl
    rrca
    rrca
    ld c, a

    and 4
    ld b, a
    ; b has left

    ld a, c
    rrca
    rrca
    ld c, a

    and $18
    or b
    ld b, a
    ; added space and right

    ld a, c
    rrca
    and 3
    or b
    ; added down and up

    or e
    ; added 2nd fire (M)

    ld l, a
    ret

; Local SNSMAT replacement using direct PPI I/O
; Input: A = row number (0-10)
; Output: A = key state (active LOW)
; Clobbers: B only
snsmat_direct:
    ld b, a             ; save row in B
    di
    in a, ($AA)         ; read PPI C register
    and $F0             ; clear row select bits (0-3)
    or b                ; set row number
    out ($AA), a        ; write PPI C register
    in a, ($A9)         ; read key state (active LOW)
    ei
    ret
