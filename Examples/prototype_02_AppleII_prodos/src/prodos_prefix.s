; prodos_prefix.s — Direct ProDOS MLI file operations
;
; Bypasses cc65 C library file I/O entirely.
; Uses MLI calls: ON_LINE, SET_PREFIX, OPEN, READ, CLOSE.
;
; I/O buffer at $0400 (text page 1, unused in HGR mode).
; Builds absolute pathnames: "/VOLNAME/FILENAME"
;
; Exports:
;   _set_prefix_from_boot_device  — detect boot volume, cache volume name
;   _prodos_load_room             — load ROOMnn into FILE_BUFFER ($1CD0)
;   _prodos_load_tileset          — load TILESn into TILE_BUFFER ($1C88)
;   _prodos_get_last_error        — return last MLI error code

        .export _set_prefix_from_boot_device
        .export _prodos_load_room
        .export _prodos_load_tileset
        .export _prodos_get_last_error

; --- ProDOS ---
MLI     = $BF00
DEVNUM  = $BF30

; --- Target buffers (must match C defines) ---
FILE_BUF_ADDR = $1CD0
FILE_BUF_MAX  = 816
TILE_BUF_ADDR = $1C88
TILE_BUF_MAX  = 72

; --- I/O buffer: $0800, page-aligned, 1024 bytes ---
; Same as apple2-iobuf-0800.o. Safe to reuse after STARTUP ($0803) completes.
IO_BUF        = $0800

; ========================================
; BSS
; ========================================
.bss

; ON_LINE parameter list and result
ol_plist:       .res 4
ol_buf:         .res 17

; SET_PREFIX parameter list and path
sp_plist:       .res 3
sp_path:        .res 18

; File pathname (Pascal string for OPEN)
; Layout: [len] [/] [volname] [/] [filename]
; Max: 1 + 15 + 1 + 6 = 23 chars + 1 length byte = 24
fname_buf:      .res 24

; Length of volume prefix in fname_buf (e.g. "/VOLNAME/" = namelen+2)
vol_len:        .res 1

; OPEN ($C8): param_count(1) + pathname(2) + io_buffer(2) + ref_num(1) = 6
open_plist:     .res 6

; READ ($CA): param_count(1) + ref_num(1) + data_buffer(2) + request_count(2) + trans_count(2) = 8
read_plist:     .res 8

; CLOSE ($CC): param_count(1) + ref_num(1) = 2
close_plist:    .res 2

; Last MLI error code (0 = success)
last_error:     .res 1

; ========================================
; CODE
; ========================================
.code

; -----------------------------------------------------------------------
; _set_prefix_from_boot_device
;
; 1. Reads DEVNUM ($BF30) to identify the device that loaded this binary
; 2. Calls ON_LINE ($C5) to get the volume name for that device
; 3. Calls SET_PREFIX ($C6) to set ProDOS prefix (best-effort)
; 4. Caches "/VOLNAME/" in fname_buf for absolute path construction
; -----------------------------------------------------------------------
_set_prefix_from_boot_device:
        lda     #0
        sta     vol_len
        sta     last_error

        ; --- Read DEVNUM ---
        lda     DEVNUM
        bne     dev_ok
        jmp     spbd_done       ; 0 = no device accessed yet
dev_ok: sta     ol_plist+1      ; unit_num

        ; --- ON_LINE ($C5) ---
        lda     #2
        sta     ol_plist        ; param_count = 2
        lda     #<ol_buf
        sta     ol_plist+2
        lda     #>ol_buf
        sta     ol_plist+3
        lda     #0
        sta     ol_buf

        jsr     MLI
        .byte   $C5             ; ON_LINE
        .addr   ol_plist
        bcc     ol_ok
        sta     last_error
        jmp     spbd_done
ol_ok:
        ; Extract name length (low nibble of first byte)
        lda     ol_buf
        and     #$0F
        bne     len_ok1
        jmp     spbd_done
len_ok1:
        cmp     #16
        bcc     len_ok2
        jmp     spbd_done
len_ok2:
        tax                     ; X = namelen

        ; --- Build SET_PREFIX path: Pascal string "/" + volname ---
        txa
        clc
        adc     #1              ; length = namelen + 1 (for '/')
        sta     sp_path         ; sp_path[0] = length

        lda     #'/'
        sta     sp_path+1       ; sp_path[1] = '/'

        ldy     #0
sp_cp:  lda     ol_buf+1,y
        sta     sp_path+2,y
        iny
        dex
        bne     sp_cp

        ; --- SET_PREFIX ($C6) ---
        lda     #1
        sta     sp_plist        ; param_count = 1
        lda     #<sp_path
        sta     sp_plist+1
        lda     #>sp_path
        sta     sp_plist+2

        jsr     MLI
        .byte   $C6             ; SET_PREFIX
        .addr   sp_plist
        bcc     sp_ok
        sta     last_error
        ; Continue — we still cache vol name for absolute paths
sp_ok:
        ; --- Cache "/VOLNAME/" in fname_buf for file loading ---
        ; Reload namelen
        lda     ol_buf
        and     #$0F
        tax                     ; X = namelen

        lda     #'/'
        sta     fname_buf+1     ; fname_buf[1] = '/'

        ldy     #0
fn_cp:  lda     ol_buf+1,y
        sta     fname_buf+2,y   ; fname_buf[2..namelen+1] = volname
        iny
        dex
        bne     fn_cp

        ; fname_buf[namelen+2] = '/'
        ; Y = namelen here
        lda     #'/'
        sta     fname_buf+2,y

        ; vol_len = namelen + 2
        iny                     ; Y = namelen + 1
        iny                     ; Y = namelen + 2
        sty     vol_len

spbd_done:
        rts


; -----------------------------------------------------------------------
; _prodos_load_room  (__fastcall__)
;
; Input:  A = room number (0-99)
; Output: AX = bytes actually read (0 on error)
;
; Builds "/VOLNAME/ROOMnn" and loads into FILE_BUFFER ($1CD0).
; -----------------------------------------------------------------------
_prodos_load_room:
        ; Clear last_error
        ldx     #0
        stx     last_error

        ; Divide A by 10 → X=tens, A=ones
        ldx     #$FF
lr_div: inx
        sec
        sbc     #10
        bcs     lr_div
        adc     #10             ; restore ones digit
        ; X = tens, A = ones

        ; Save digits
        pha                     ; ones on stack
        txa
        pha                     ; tens on stack

        ; Append "ROOM" to fname_buf after vol_prefix
        ldy     vol_len

        lda     #'R'
        sta     fname_buf+1,y
        iny
        lda     #'O'
        sta     fname_buf+1,y
        iny
        lda     #'O'
        sta     fname_buf+1,y
        iny
        lda     #'M'
        sta     fname_buf+1,y
        iny

        ; Append digits
        pla                     ; tens
        clc
        adc     #'0'
        sta     fname_buf+1,y
        iny
        pla                     ; ones
        clc
        adc     #'0'
        sta     fname_buf+1,y
        iny

        ; Set length byte: fname_buf[0] = total path length
        tya                     ; Y = vol_len + 6
        sta     fname_buf

        ; Set up READ target: FILE_BUFFER, max 816 bytes
        lda     #<FILE_BUF_ADDR
        sta     read_plist+2
        lda     #>FILE_BUF_ADDR
        sta     read_plist+3
        lda     #<FILE_BUF_MAX
        sta     read_plist+4
        lda     #>FILE_BUF_MAX
        sta     read_plist+5

        jmp     do_orc


; -----------------------------------------------------------------------
; _prodos_load_tileset  (__fastcall__)
;
; Input:  A = tileset_id (0-9)
; Output: A = 1 on success, 0 on failure
;
; Builds "/VOLNAME/TILESn" and loads into TILE_BUFFER ($1C88).
; -----------------------------------------------------------------------
_prodos_load_tileset:
        ; Clear last_error
        ldx     #0
        stx     last_error

        ; Save tileset_id
        pha

        ; Append "TILES" + digit
        ldy     vol_len

        lda     #'T'
        sta     fname_buf+1,y
        iny
        lda     #'I'
        sta     fname_buf+1,y
        iny
        lda     #'L'
        sta     fname_buf+1,y
        iny
        lda     #'E'
        sta     fname_buf+1,y
        iny
        lda     #'S'
        sta     fname_buf+1,y
        iny

        pla                     ; tileset_id
        clc
        adc     #'0'
        sta     fname_buf+1,y
        iny

        ; Set length byte
        tya
        sta     fname_buf

        ; Set up READ target: TILE_BUFFER, max 72 bytes
        lda     #<TILE_BUF_ADDR
        sta     read_plist+2
        lda     #>TILE_BUF_ADDR
        sta     read_plist+3
        lda     #<TILE_BUF_MAX
        sta     read_plist+4
        lda     #>TILE_BUF_MAX
        sta     read_plist+5

        jsr     do_orc

        ; Convert AX (bytes read) to boolean
        cpx     #0
        bne     lt_ok
        cmp     #0
        beq     lt_fail
lt_ok:  lda     #1
        rts
lt_fail:
        lda     #0
        rts


; -----------------------------------------------------------------------
; _prodos_get_last_error
;
; Output: A = last MLI error code (0 = no error)
; -----------------------------------------------------------------------
_prodos_get_last_error:
        lda     last_error
        ldx     #0
        rts



; -----------------------------------------------------------------------
; do_orc — internal: OPEN / READ / CLOSE
;
; Prerequisites:
;   fname_buf     — ProDOS Pascal string pathname
;   read_plist+2  — data buffer pointer (lo/hi)
;   read_plist+4  — request count (lo/hi)
;
; Output: AX = bytes transferred (0 on error)
; Clobbers: last_error on failure
; -----------------------------------------------------------------------
do_orc:
        ; --- OPEN ($C8) ---
        lda     #3
        sta     open_plist      ; param_count = 3
        lda     #<fname_buf
        sta     open_plist+1    ; pathname lo
        lda     #>fname_buf
        sta     open_plist+2    ; pathname hi
        lda     #<IO_BUF
        sta     open_plist+3    ; io_buffer lo
        lda     #>IO_BUF
        sta     open_plist+4    ; io_buffer hi

        jsr     MLI
        .byte   $C8             ; OPEN
        .addr   open_plist
        bcc     orc_opened
        sta     last_error
        lda     #0
        tax
        rts

orc_opened:
        ; ref_num returned in open_plist+5
        ; --- READ ($CA) ---
        lda     #4
        sta     read_plist      ; param_count = 4
        lda     open_plist+5
        sta     read_plist+1    ; ref_num
        ; data_buffer and request_count already set by caller

        jsr     MLI
        .byte   $CA             ; READ
        .addr   read_plist
        bcc     orc_read_ok
        sta     last_error
        ; Must still close the file
        jmp     orc_close_err

orc_read_ok:
        ; Save trans_count (bytes transferred)
        lda     read_plist+6    ; trans_count lo
        pha
        lda     read_plist+7    ; trans_count hi
        pha

        ; --- CLOSE ($CC) ---
        lda     #1
        sta     close_plist     ; param_count = 1
        lda     open_plist+5
        sta     close_plist+1   ; ref_num

        jsr     MLI
        .byte   $CC             ; CLOSE
        .addr   close_plist
        ; Ignore close error

        ; Return trans_count in AX
        pla                     ; hi
        tax
        pla                     ; lo
        rts

orc_close_err:
        ; Close file (best-effort), return 0
        lda     #1
        sta     close_plist
        lda     open_plist+5
        sta     close_plist+1
        jsr     MLI
        .byte   $CC
        .addr   close_plist
        lda     #0
        tax
        rts
