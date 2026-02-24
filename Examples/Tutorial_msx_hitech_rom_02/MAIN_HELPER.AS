	psect	text
	global	_rom_prepare_text
	global	_rom_print_hello
	global	_rom_wait_loop

_rom_prepare_text:
	call	06Ch
	call	0C3h
	ret

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

_rom_wait_loop:
	halt
	jr	_rom_wait_loop
