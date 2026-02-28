;* * * * *  Small-C/Plus z88dk * * * * *
;  Version: 23854-4d530b6eb7-20251002
;
;	Reconstructed for z80 Module Assembler
;
;	Module compile time: Sat Feb 28 09:30:11 2026


	C_LINE	0,"test_calling_conv.c"

	MODULE	test_calling_conv_c


	INCLUDE "z80_crt0.hdr"


	C_LINE	0,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	0,"/opt/z88dk/bin/..//include/sys/compiler.h"
	C_LINE	0,"/opt/z88dk/bin/..//include/sys/proto.h"
	C_LINE	6,"/opt/z88dk/bin/..//include/sys/compiler.h"
	C_LINE	10,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	0,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	17,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	26,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	38,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	44,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	49,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	54,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	59,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	64,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	69,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	74,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	79,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	84,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	89,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	93,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	94,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	95,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	97,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	98,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	99,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	105,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	110,"/opt/z88dk/bin/..//include/sys/types.h"
	C_LINE	11,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	13,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	14,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	15,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	17,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	18,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	19,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	21,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	22,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	23,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	25,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	26,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	27,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	29,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	30,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	31,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	33,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	34,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	35,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	37,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	38,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	40,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	41,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	43,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	44,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	49,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	52,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	54,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	55,"/opt/z88dk/bin/..//include/stdint.h"
	C_LINE	1,"test_calling_conv.c"
	C_LINE	3,"test_calling_conv.c"
	C_LINE	4,"test_calling_conv.c"
	C_LINE	5,"test_calling_conv.c"
	C_LINE	6,"test_calling_conv.c"
	C_LINE	8,"test_calling_conv.c"
	SECTION	code_compiler

; Function test flags 0x00000200 __smallc 
; void test()
	C_LINE	8,"test_calling_conv.c::test::0::0"
._test
	ld	hl,66	;const
	call	_func_1u8
	ld	hl,4660	;const
	call	_func_1ptr
	ld	hl,17	;const
	push	hl
	ld	l,34
	push	hl
	call	_func_2u8
	ld	hl,17	;const
	push	hl
	ld	l,34
	push	hl
	ld	hl,51	;const
	push	hl
	call	_func_3u8
	ret


	SECTION	bss_compiler
	SECTION	code_compiler
; --- Start of Optimiser additions ---


; --- Start of Static Variables ---

	SECTION	bss_compiler
	SECTION	code_compiler


; --- Start of Scope Defns ---

	GLOBAL	_func_2u8
	GLOBAL	_func_3u8
	GLOBAL	_func_1u8
	GLOBAL	_func_1ptr
	GLOBAL	_test


; --- End of Scope Defns ---


; --- End of Compilation ---
