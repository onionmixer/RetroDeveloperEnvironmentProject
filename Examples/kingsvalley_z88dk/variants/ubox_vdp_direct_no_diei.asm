; variants/ubox_vdp_direct_no_diei.asm
;
; Diagnostic variant: identical to ubox-msx-lib-z88dk ubox_vdp_direct.asm
; EXCEPT the `di` at entry and `ei` at exit are removed. Tests hypothesis
; that the di-window during sprite/VRAM transfers (32B sprite pattern =
; ~1600 cy of ISR-blocked time, longer for fill_screen) is what masks
; one VBlank ISR per tool-use, manifesting as the "hitch".
;
; SDCC original kingsvalley uses `jp LDIRVM` (BIOS at $005C) directly,
; which does NOT disable interrupts. The original BIOS path is also
; faster because there is no setup overhead and z80 OUT inside the BIOS
; LDIRVM loop is timing-safe under V9938 active-display constraints.
;
; This variant removes only the di/ei. The direct VDP-I/O loop body
; itself is unchanged. If the glitch goes away in this variant the
; root cause is conclusively the interrupt-disable window.
;
; Symbols match the library version verbatim so this object overrides
; the library copy when linked before the lib (achieved by replacing
; the lib's ubox_vdp_direct.o in the variant build).

    SECTION code_user

    PUBLIC ubox_ldirvm_direct
    PUBLIC ubox_ldirmv_direct

ubox_ldirvm_direct:
    ld a, e
    out ($99), a
    ld a, d
    or $40
    out ($99), a
wr_loop:
    ld a, (hl)
    out ($98), a
    inc hl
    dec bc
    ld a, b
    or c
    jr nz, wr_loop
    ret

ubox_ldirmv_direct:
    ld a, l
    out ($99), a
    ld a, h
    and $3F
    out ($99), a
rd_loop:
    in a, ($98)
    ld (de), a
    inc de
    dec bc
    ld a, b
    or c
    jr nz, rd_loop
    ret
