; akm_bridge_konami.asm — kingsvalley Phase C 의 Konami 매퍼용 AKM bridge
;
; Phase B 의 akm_bridge_kv.asm (ASCII16, $7000 register) 를 Konami 매퍼
; 의 dual register ($8000 + $A000) 으로 변형. prototype_06 의
; akm_bridge.asm 패턴 그대로.
;
; Layout (kingsvalley Phase C):
;   bank 0+1 = main 16K 미만 (page 1 fixed, $4000-$7FFF)
;   BANK_01  = data.c 의 rodata (~7.5 KiB) + 라이브러리 잔여 RODATA
;              Konami 매퍼 boot default 로 $8000-$BFFF mounted
;   BANK_02  = AKM blob (~3.5 KiB)
;              mplayer_engine_load 호출 시 일시 swap-in + LDIR + 복원
;
; Konami 매퍼 register:
;   $6000 → $6000-$7FFF window bank
;   $8000 → $8000-$9FFF window bank (cart bank 4 = BANK_02 의 첫 8K)
;   $A000 → $A000-$BFFF window bank (cart bank 5 = BANK_02 의 두 번째 8K)
;
; AKM body 의 RAM 위치 ($D500): kingsvalley Phase B 와 동일.

    SECTION code_user

    PUBLIC  _mplayer_engine_load

    DEFC    AKM_BASE       = $D500
    DEFC    AKM_BLOB_VIRT  = $8000     ; BANK_02 mount 시 $8000 = blob 시작
    DEFC    AKM_BLOB_LEN   = 3510      ; akm.bin 정확한 byte 수 (rasm 출력)
    DEFC    RAM_STUB_BASE  = $F100     ; PLY_AKM_ROM_Buffer 위 safe 영역
    DEFC    MAPPER_8000    = $8000
    DEFC    MAPPER_A000    = $A000

_mplayer_engine_load:
    di
    ; RAM stub LDIR (kingsvalley Phase B 와 동일 trick — bank-2 swap 시
    ; 자기 자신이 unmap 되는 것 회피)
    ld   hl, _engine_stub_start
    ld   de, RAM_STUB_BASE
    ld   bc, _engine_stub_end - _engine_stub_start
    ldir
    call RAM_STUB_BASE
    ei
    ret

;
; engine_stub: RAM-resident routine.
;   - $8000 ← bank 4 (BANK_02 첫 8K)
;   - $A000 ← bank 5 (BANK_02 두 번째 8K) → BANK_02 16K mounted at $8000-$BFFF
;   - LDIR ($8000 → $D500, 3510 byte)
;   - $8000 ← bank 2 (BANK_01 첫 8K, default 복원)
;   - $A000 ← bank 3 (BANK_01 두 번째 8K, default 복원)
;   - RET
;
_engine_stub_start:
    ld   a, 4
    ld   (MAPPER_8000), a
    ld   a, 5
    ld   (MAPPER_A000), a

    ld   hl, AKM_BLOB_VIRT
    ld   de, AKM_BASE
    ld   bc, AKM_BLOB_LEN
    ldir

    ld   a, 2
    ld   (MAPPER_8000), a       ; BANK_01 의 8K bank 2 복원
    ld   a, 3
    ld   (MAPPER_A000), a       ; BANK_01 의 8K bank 3 복원
    ret
_engine_stub_end:

;
; AKM blob 의 cart-side placement: BANK_02 의 RODATA_2.
;
    SECTION RODATA_2

    PUBLIC _akm_blob_start
    PUBLIC _akm_blob_end
_akm_blob_start:
    BINARY "akm.bin"
_akm_blob_end:
