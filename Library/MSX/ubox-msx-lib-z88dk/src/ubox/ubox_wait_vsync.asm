; ubox_wait_vsync — z88dk port (function, not macro).
; void ubox_wait_vsync(void);
;
; Was a `__asm halt __endasm` macro in the SDCC port, but sccz80's
; preprocessor collapses the macro body onto a single line, which
; breaks the `__asm`/`__endasm` block markers (it expects each on its
; own line). Exposing it as a real function avoids that pitfall, at
; the cost of a `call/ret` (≈27 T-states; negligible vs the ~71000
; T-states between VBlanks at 60 Hz).

    SECTION code_user
    PUBLIC _ubox_wait_vsync

_ubox_wait_vsync:
    halt
    ret
