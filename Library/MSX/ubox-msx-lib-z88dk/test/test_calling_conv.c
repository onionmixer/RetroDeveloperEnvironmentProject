#include <stdint.h>

extern void func_2u8(uint8_t a, uint8_t b) __z88dk_callee;
extern void func_3u8(uint8_t a, uint8_t b, uint8_t c) __z88dk_callee;
extern uint8_t func_1u8(uint8_t a) __z88dk_fastcall;
extern void func_1ptr(const uint8_t *p) __z88dk_fastcall;

void test(void) {
    func_1u8(0x42);
    func_1ptr((const uint8_t *)0x1234);
    func_2u8(0x11, 0x22);
    func_3u8(0x11, 0x22, 0x33);
}
