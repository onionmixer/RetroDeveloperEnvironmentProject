; hello.as - Hello world for MSX-DOS
; Uses BDOS function 9 to print string
; Addresses are hardcoded for 0x100 load address

    psect   text,global,pure

    global  start

start:
    ; msg is at file offset 0x0D, memory 0x10D
    ld      de, 10Dh    ; message address (0x100 + 0x0D)
    ld      c, 9        ; BDOS print string function
    call    5           ; BDOS entry point

    ld      c, 0        ; BDOS program terminate
    jp      5           ; Exit to DOS

msg:
    defm    'hello world'
    defb    13, 10, '$'

    end     start
