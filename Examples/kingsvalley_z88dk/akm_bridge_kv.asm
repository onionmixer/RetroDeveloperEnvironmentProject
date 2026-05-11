; akm_bridge_kv.asm — kingsvalley Phase B AKM bridge (ASCII16 multi-bank)
;
; Why kingsvalley-local (vs. mplayer-z88dk's stock akm_bridge.asm):
;   - 32K main code 가 cart bank 0+1 ($4000-$BFFF) 를 모두 차지하므로
;     AKM blob (3.5 KiB) 을 main 안에 INCBIN 할 공간 없음 (Phase A 검증
;     기준 free=1.9 KiB).
;   - 해결: ASCII16 64K cart (or 48K) 의 BANK_02 (RODATA_2 섹션) 에 AKM blob
;     을 두고, runtime 에 mplayer_engine_load() 이 bank 2 를 $8000-$BFFF
;     window 에 mount → LDIR → 원래 bank 1 복구.
;
; Layout 가정 (Phase B):
;   bank 0     = $4000-$7FFF (always mounted; main code 의 페이지 1)
;   bank 1     = $8000-$BFFF (default at startup; main code 의 페이지 2)
;   bank 2     = AKM blob 보관 (mplayer_engine_load 에서만 mount)
;   $C000-$D426  BSS
;   $D500       AKM body LDIR target (ORG=#D500 in akm.z80)
;   $F000       PLY_AKM_ROM_Buffer (akm_ubox.asm)
;   $F380↓     stack
;
; ASCII16 trigger 주소:
;   write to $7000 → $8000-$BFFF window 의 bank 선택
;   (z88dk classic/rom.asm: MAPPER_ADDRESS_8000 = $7000 with MAPPER_ASCII16=1)

    SECTION code_user

    PUBLIC  _mplayer_engine_load

    DEFC    AKM_BASE = $D500
    DEFC    AKM_BLOB_VIRT = $8000     ; bank 2 mount 위치
    DEFC    AKM_BLOB_LEN  = 3510      ; akm.bin 정확한 byte 수 (rasm 출력)
    DEFC    RAM_STUB_BASE = $F100     ; PLY_AKM_ROM_Buffer($F000) 위 안전 영역

;
; mplayer_engine_load 자체는 cart 의 어느 bank 에 있든 (현재 layout: bank 1
; @ $97xx) 실행 시작 가능. 그러나 bank 2 로 swap 하면 자기 자신이 unmap
; 되어 다음 instruction fetch 가 bank 2 content (AKM body) 로 바뀌어 충돌.
;
; 해결: 작은 stub 을 RAM($F100, page 3 — 매퍼 무관) 에 LDIR 복사 후
; 점프해 거기서 bank-switch + LDIR + 복구 수행. RET 으로 caller 에 복귀.
; (caller 는 보통 main() = $43xx 에 있어 bank 0 항상 mount, 안전.)
;
_mplayer_engine_load:
    di
    ; stub 을 RAM 으로 복사
    ld   hl, _engine_stub_start
    ld   de, RAM_STUB_BASE
    ld   bc, _engine_stub_end - _engine_stub_start
    ldir
    ; RAM stub 으로 jump (CALL 후 stub 의 RET 가 우리에게 복귀)
    call RAM_STUB_BASE
    ei
    ret

;
; engine_stub: 21-byte RAM resident routine.
;   - bank 2 → $8000 window 로 swap
;   - bank 2 의 $8000+ 에서 AKM body($D500) 로 LDIR
;   - bank 1 복구
;   - RET
;
; ASCII16 매퍼: write to $7000 selects $8000 window bank.
; 가정: 호출 시점 bank 1 이 mount 되어 있음 (z88dk crt0 init 직후 default).
;
_engine_stub_start:
    ld   a, 2
    ld   ($7000), a               ; bank 2 → $8000
    ld   hl, AKM_BLOB_VIRT        ; $8000
    ld   de, AKM_BASE             ; $D500
    ld   bc, AKM_BLOB_LEN         ; 3510
    ldir
    ld   a, 1
    ld   ($7000), a               ; bank 1 복구
    ret
_engine_stub_end:

; AKM blob 자체는 BANK_02 (RODATA_2 섹션) 에 둠.
; rasm 가 ORG=#D500 로 assemble 했으므로 blob 내부 jp/call 들은
; $D500+ 를 가리킨다. LDIR 후 $D500 RAM 에 복사되면 정상 동작.
    SECTION rodata_user

    DEFC _akm_blob_size = _akm_blob_end - _akm_blob_start
    PUBLIC _akm_blob_size_pub
_akm_blob_size_pub: defw _akm_blob_size

    SECTION RODATA_2

    PUBLIC _akm_blob_start
    PUBLIC _akm_blob_end
_akm_blob_start:
    BINARY "akm.bin"
_akm_blob_end:
