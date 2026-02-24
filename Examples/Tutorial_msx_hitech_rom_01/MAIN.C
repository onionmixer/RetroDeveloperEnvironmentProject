extern void rom_print_hello(void);
extern void rom_debug_loop(void);
extern void rom_wait_loop(void);

int main(void)
{
    /* Handshake marker used by static/runtime checks for main-pure entry. */
    (*(unsigned char *)0xC000) = 0xA5;
#ifdef MAIN_PURE
    /* Keep C readable: assembly-heavy routines live in MAIN_HELPER.AS. */
    rom_print_hello();
#if DEBUG_BORDER
    rom_debug_loop();
#else
    rom_wait_loop();
#endif
#endif
    return 0;
}
