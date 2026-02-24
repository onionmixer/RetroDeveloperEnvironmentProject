/*
 * hello.c - Apple II ProDOS Hello World
 *
 * Compile: cl65 -t apple2 -o HELLO hello.c
 */

#include <stdio.h>
#include <conio.h>

int main(void)
{
    clrscr();                           /* Clear screen */
    printf("hello world\n");
    printf("\nPress any key to exit...\n");
    cgetc();                            /* Wait for key press */
    return 0;
}
