	psect	text
	global	_rom_print_hello
	global	_rom_debug_loop
	global	_rom_wait_loop

_rom_print_hello:
	ld	a,'H'
	call	0A2h
	ld	a,'e'
	call	0A2h
	ld	a,'l'
	call	0A2h
	ld	a,'l'
	call	0A2h
	ld	a,'o'
	call	0A2h
	ld	a,' '
	call	0A2h
	ld	a,'W'
	call	0A2h
	ld	a,'o'
	call	0A2h
	ld	a,'r'
	call	0A2h
	ld	a,'l'
	call	0A2h
	ld	a,'d'
	call	0A2h
	ld	a,0Dh
	call	0A2h
	ld	a,0Ah
	call	0A2h
	ret

_rom_debug_loop:
	ld	b,1
dbg_loop:
	ld	a,b
	and	0fh
	or	0f0h
	defb	0d3h,099h
	ld	a,087h
	defb	0d3h,099h
	ld	de,0FFFFh
dbg_wait:
	dec	de
	ld	a,d
	or	e
	jp	nz,dbg_wait
	inc	b
	jp	dbg_loop

_rom_wait_loop:
	halt
	jr	_rom_wait_loop
