; ubox_random / ubox_randomize — z88dk port (kingsvalley fork extension).
;
; void     ubox_randomize(uint16_t seed) __z88dk_fastcall;   ; HL = seed
; uint16_t ubox_random(void);                                 ; returns HL
;
; Galois-style 16-bit LFSR. Equivalent to kingsvalley/src/ubox/ubox_random.z80
; (SDCC asasm) translated to z80asm syntax. Internal `rand_seed` lives in
; `bss_user` — not exposed (verified Iter 4: no C code references it).

    SECTION code_user
    PUBLIC _ubox_randomize
    PUBLIC _ubox_random

_ubox_randomize:
    ld (rand_seed), hl
    ret

_ubox_random:
    ld hl, (rand_seed)
    ld b, h
    ld a, l
    ; HL = HL >> 1
    srl h
    rr l
    ; L = L xor ((not a) and 0x29)
    cpl
    and $29
    xor l
    ld l, a
    ; H = (H xor ((not b) and 0x15)) or (((not b) << 7) & 0x80)
    ld a, b
    cpl
    ld b, a
    and $15
    xor h
    ld h, a
    ld a, b
    rrca
    and $80
    or h
    ld h, a
    ld (rand_seed), hl
    ret

    SECTION bss_user
rand_seed:
    DEFS 2
