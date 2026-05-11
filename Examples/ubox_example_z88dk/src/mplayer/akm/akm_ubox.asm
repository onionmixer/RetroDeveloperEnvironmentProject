
; these are common
PLY_AKM_HARDWARE_MSX = 1
PLY_AKM_MANAGE_SOUND_EFFECTS = 1

; z88dk port note (2026-05-09): PLY_AKM_Rom mode disabled.
; Original SDCC build used:
;   PLY_AKM_Rom = 1
;   PLY_AKM_ROM_Buffer = #c000
; That places AKM's working buffer at $C000, which is exactly where z88dk's
; CRT_ORG_BSS sits. ubox's ISR vars (ubox_usr_isr, _ubox_tick) collide with
; AKM's ROM-mode buffer — every interrupt corrupts AKM state and vice
; versa, so ROM builds went silent. We don't actually need ROM mode here:
; the player blob is rasm'd at ORG=$A000 and copied to that RAM page by
; mplayer_engine_load() at startup, so AKM can self-modify in place.
; (codex cross-check 2026-05-09 confirmed the $C000 collision as the
; ROM-only failure mode.)

include "PlayerAkm.asm"

; IN: L = channel
; OUT: L = 0 if is not on
PLY_AKM_IsSoundEffectOnDisarkGenerateExternalLabel:
PLY_AKM_IsSoundEffectOn:
	ld a,l
	add a,a
	add a,a
	add a,a
	ld c,a
	ld b,0
        ld hl,PLY_AKM_Channel1_SoundEffectData
	add hl,bc
	ld a,(hl)
	inc hl
	or (hl)
	ld l,a
	ret

