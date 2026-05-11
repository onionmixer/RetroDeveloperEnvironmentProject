; akm_stub.asm — Phase A 에서 mplayer / AKM 심볼을 모두 no-op 으로 처리
;
; Phase A 의 목표: minimal viable boot — 음악/효과음 없이 게임 화면 띄우기.
; 본 stub 는 game/src/main.c 가 호출하는 mplayer_* 심볼 + main.h 가
; extern 으로 선언한 SONG/EFFECTS 데이터를 제공한다.
;
; Phase B 진입 시 본 파일은 buildlist 에서 제외하고 mplayer-z88dk
; library + akm_bridge.asm (INCBIN AKM blob) 으로 교체한다.

    SECTION code_user

    PUBLIC  _mplayer_init
    PUBLIC  _mplayer_init_effects
    PUBLIC  _mplayer_play
    PUBLIC  _mplayer_play_effect
    PUBLIC  _mplayer_play_effect_p
    PUBLIC  _mplayer_engine_load

_mplayer_init:
_mplayer_init_effects:
_mplayer_play:
_mplayer_play_effect:
_mplayer_play_effect_p:
_mplayer_engine_load:
    ret

    SECTION rodata_user

    PUBLIC  _SONG
    PUBLIC  _EFFECTS

_SONG:    DEFB 0,0,0,0,0
_EFFECTS: DEFB 0,0,0,0,0
