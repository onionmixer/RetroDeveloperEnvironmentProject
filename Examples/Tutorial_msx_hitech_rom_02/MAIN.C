extern void rom_prepare_text(void);
extern void rom_print_hello(void);
extern void rom_wait_loop(void);

int main(void)
{
    rom_prepare_text();
    rom_print_hello();
    rom_wait_loop();
    return 0;
}
