; variants/msxbios_no_calslt.asm — Phase B no_calslt 진단 variant
;
; ubox 함수들이 호출하는 `msxbios` 를 별도 심볼 `msxbios_fast` 로
; redirect 한 버전. 원본 z88dk msx_crt0 의 `msxbios` 는 그대로 두고
; (system lib 가 의존), ubox 의 BIOS 호출만 이 stub 사용.
;
; ubox_*.asm 의 sed patch (compile_phaseB_variant.sh 내):
;   EXTERN msxbios   →  EXTERN msxbios_fast
;   jp msxbios       →  jp msxbios_fast
;   call msxbios     →  call msxbios_fast
;
; Original z88dk msxbios:
;     msxbios:
;         ld iy,($FCC0)     ; ~20 cy
;         call $001C        ; CALSLT, ~150 cy + slot save/restore
;         ei                ; 4 cy
;         ret               ; 10 cy
; → ~220 cy per BIOS call.
;
; This stub:
;     msxbios_fast:
;         jp (ix)           ; 8 cy
; → 8 cy. Saves ~212 cy per call.
;
; ROM mode safety: page 0 is BIOS slot at boot, BIOS routines reachable
; by direct `jp` to address (= jp (ix) where IX is loaded with the BIOS
; entry constant). MSX-DOS2 .COM mode would need CALSLT — this stub
; would break under DOS.

    SECTION code_user

    PUBLIC msxbios_fast

msxbios_fast:
    jp (ix)
