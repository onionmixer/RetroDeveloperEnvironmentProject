; ubox_update — z88dk port (kingsvalley fork extension).
; uint8_t ubox_update(void);
;
; Cross-platform abstraction. SDL2/Allegro backends pump events here and
; return 0 if the host requested exit. On MSX there is no event loop,
; so this always returns 1 (game continues). Allows the same
; `while (ubox_update()) { ... }` C loop body to compile on both targets.

    SECTION code_user
    PUBLIC _ubox_update

_ubox_update:
    ld l, 1
    ret
