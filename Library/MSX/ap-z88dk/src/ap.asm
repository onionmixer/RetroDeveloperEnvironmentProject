; ap.asm — aPLib decompressor, z88dk z80asm port.
;
; Source:    kingsvalley/src/ap/ap.z80 (SDCC asasm).
; Authors:   Original by Dan Weiss (Dwedit), adapted by utopian,
;            optimized by Metalbrain.
;
; Conversion notes (vs SDCC original):
;   * `.globl X` → PUBLIC X (definitions)
;   * `_label::` → `_label:`
;   * `#NN` immediate → `NN` (or `$NN` for hex)
;   * `.ez80` directive (undocumented opcode enable) → dropped
;     (z80asm accepts `ld ixl/iyl/ixh/iyh` and `srl (ix+d)` by default —
;     verified by minimal test 2026-05-10)
;   * SDCC C entry takes args right-to-left at sp+2..3 (dst→DE),
;     sp+4..5 (src→HL). sccz80 pushes left-to-right with 16-bit args
;     unchanged, so the C entry is *swapped*: src is at sp+2..3 and
;     dst at sp+4..5. The register-call entry `ap_uncompress` keeps
;     the original SDCC contract (HL=src, DE=dst) untouched, so the
;     body is byte-identical to the original.
;   * The body uses IXL/IXH/IYL/IYH heavily, so the C wrapper cannot
;     park its frame in IX — we use HL twice with `add hl, sp` instead.
;
; sccz80 stack layout (verified empirically 2026-05-10):
;   sp+0..1 = ret addr
;   sp+2..3 = src   (last pushed, nearer to top)
;   sp+4..5 = dst   (first pushed, deeper)

    SECTION code_user

    PUBLIC _ap_uncompress
    PUBLIC ap_uncompress

; void ap_uncompress(const uint8_t *dst, const uint8_t *src);
;
; sccz80 caller-clean entry. The body of ap_uncompress freely uses
; IXL/IXH (bit-stream cache) and IYL/IYH (offset history). z88dk
; sccz80's ABI does NOT promise IX/IY are caller-saved (and z88dk
; libraries / ISR helpers may rely on IY being preserved across calls),
; so the wrapper saves IX/IY around the body. The body's `ret z`
; (EOF) returns through the inner `call ap_uncompress` instruction,
; so the wrapper's pop ix/pop iy run on completion in either path.
_ap_uncompress:
    ; sp+2..3 = src (last pushed) ; sp+4..5 = dst (first pushed) — sccz80 push order verified empirically.
    ld hl, 4
    add hl, sp
    ld e, (hl)              ; dst lo (sp+4)
    inc hl
    ld d, (hl)              ; dst hi (sp+5) → DE = dst
    ld hl, 2
    add hl, sp
    ld a, (hl)              ; src lo (sp+2)
    inc hl
    ld h, (hl)              ; src hi (sp+3)
    ld l, a                 ;        → HL = src
    push ix
    push iy
    call ap_uncompress      ; body — HL=src, DE=dst
    pop iy
    pop ix
    ret

; Register-call entry: HL = source (compressed), DE = dest (uncompressed).
; Caller responsible for IX/IY preservation when invoking this directly.

; Register-call entry: HL = source (compressed), DE = dest (uncompressed).
ap_uncompress:
    ld ixl, 128

apbranch1:
    ldi
aploop0:
    ld ixh, 1                  ; LWM = 0
aploop:
    call ap_getbit
    jr nc, apbranch1
    call ap_getbit
    jr nc, apbranch2
    ld b, 0
    call ap_getbit
    jr nc, apbranch3
    ld c, 16                   ; get an offset
apget4bits:
    call ap_getbit
    rl c
    jr nc, apget4bits
    jr nz, apbranch4
    ld a, b
apwritebyte:
    ld (de), a                 ; write a 0
    inc de
    jr aploop0
apbranch4:
    and a
    ex de, hl                  ; write a previous byte (1-15 away from dest)
    sbc hl, bc
    ld a, (hl)
    add hl, bc
    ex de, hl
    jr apwritebyte
apbranch3:
    ld c, (hl)                 ; use 7 bit offset, length = 2 or 3
    inc hl
    rr c
    ret z                      ; if a zero is encountered here, it is EOF
    ld a, 2
    adc a, b
    push hl
    ld iyh, b
    ld iyl, c
    ld h, d
    ld l, e
    sbc hl, bc
    ld c, a
    jr ap_finishup2
apbranch2:
    call ap_getgamma           ; use a gamma code * 256 for offset, another gamma for length
    dec c
    ld a, c
    sub ixh
    jr z, ap_r0_gamma          ; if gamma code is 2, use old r0 offset
    dec a
    ; bc = bc*256+(hl), lazy 16-bit way
    ld b, a
    ld c, (hl)
    inc hl
    ld iyh, b
    ld iyl, c

    push bc

    call ap_getgamma

    ex (sp), hl                ; bc = len, hl = offs
    push de
    ex de, hl

    ld a, 4
    cp d
    jr nc, apskip2
    inc bc
    or a
apskip2:
    ld hl, 127
    sbc hl, de
    jr c, apskip3
    inc bc
    inc bc
apskip3:
    pop hl                     ; bc = len, de = offs, hl = junk
    push hl
    or a
ap_finishup:
    sbc hl, de
    pop de                     ; hl = dest-offs, bc = len, de = dest
ap_finishup2:
    ldir
    pop hl
    ld ixh, b
    jr aploop

ap_r0_gamma:
    call ap_getgamma           ; and a new gamma code for length
    push hl
    push de
    ex de, hl
    ld d, iyh
    ld e, iyl
    jr ap_finishup

ap_getbit:
    ld a, ixl
    add a, a
    ld ixl, a
    ret nz
    ld a, (hl)
    inc hl
    rla
    ld ixl, a
    ret

ap_getgamma:
    ld bc, 1
ap_getgammaloop:
    call ap_getbit
    rl c
    rl b
    call ap_getbit
    jr c, ap_getgammaloop
    ret
