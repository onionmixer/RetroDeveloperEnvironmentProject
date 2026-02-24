	psect	text
	global	_start
	global	_rom_prepare_text
	global	_rom_print_hello
	global	_rom_wait_loop

_start:
	defs	100h
	defm	'ROM '
	defw	start

start:
	di
	ld	sp,(0FC4Ah)
	call	_rom_prepare_text
	call	_rom_print_hello
	jp	_rom_wait_loop
