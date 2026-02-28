; ubox_vdp_direct — direct VDP I/O for RAM<->VRAM transfers
; Replaces BIOS LDIRVM/LDIRMV to avoid page 0 slot conflict under MSX-DOS2.
; When CALSLT pages in BIOS ROM on page 0, RAM data at $0000-$3FFF becomes
; inaccessible. These routines use direct VDP port I/O instead.

    SECTION code_user

    PUBLIC ubox_ldirvm_direct
    PUBLIC ubox_ldirmv_direct

; ubox_ldirvm_direct: Copy RAM -> VRAM
; HL = RAM source, DE = VRAM destination, BC = byte count
; Clobbers: AF, HL, BC
ubox_ldirvm_direct:
    ld a, e
    di
    out ($99), a        ; VRAM address low
    ld a, d
    or $40              ; bit 6 = write mode
    out ($99), a        ; VRAM address high
wr_loop:
    ld a, (hl)
    out ($98), a
    inc hl
    dec bc
    ld a, b
    or c
    jr nz, wr_loop
    ei
    ret

; ubox_ldirmv_direct: Copy VRAM -> RAM
; HL = VRAM source, DE = RAM destination, BC = byte count
; Clobbers: AF, DE, BC
ubox_ldirmv_direct:
    ld a, l
    di
    out ($99), a        ; VRAM address low
    ld a, h
    and $3F             ; bit 6 = 0 (read mode)
    out ($99), a        ; VRAM address high
rd_loop:
    in a, ($98)
    ld (de), a
    inc de
    dec bc
    ld a, b
    or c
    jr nz, rd_loop
    ei
    ret
