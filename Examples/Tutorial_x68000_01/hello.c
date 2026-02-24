/*
 * Tutorial_x68000_01 - Human68k Hello World
 *
 * This sample is intentionally minimal and run68-friendly.
 */

#include <stdio.h>
#include <x68k/dos.h>

int main(void)
{
    int drv = _dos_curdrv();

    printf("Hello, X68000 Tutorial!\n");
    printf("Current drive: %c:\\\n", 'A' + drv);
    printf("Built with m68k-xelf-gcc (elf2x68k).\n");

    return 0;
}
