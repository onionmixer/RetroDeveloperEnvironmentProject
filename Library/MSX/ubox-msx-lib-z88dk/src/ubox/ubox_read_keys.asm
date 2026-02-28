; ubox_read_keys — z88dk port (direct PPI I/O)
; uint8_t ubox_read_keys(uint8_t row) __z88dk_fastcall
; L = row
; Returns: L = key bitmap (active high)
;
; Uses direct PPI I/O to avoid CALSLT register clobbering issues.

    SECTION code_user

    PUBLIC _ubox_read_keys

_ubox_read_keys:
    ld a, l         ; A = row number
    ld b, a         ; save row in B
    di
    in a, ($AA)     ; read PPI C register
    and $F0         ; clear row select bits (0-3)
    or b            ; set row number
    out ($AA), a    ; write PPI C register
    in a, ($A9)     ; read key state (active LOW)
    ei
    cpl             ; invert to active HIGH
    ld l, a
    ret
