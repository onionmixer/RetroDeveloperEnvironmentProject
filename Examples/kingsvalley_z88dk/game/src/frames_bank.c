/* frames_bank.c — Phase C 의 main 16K fit 위해 game.c 에서 분리한 const
 * arrays. #pragma bank 1 로 BANK_01 RODATA_1 에 배치된다.
 *
 * Konami 매퍼 boot default 가 bank 2,3 ($8000-$BFFF = BANK_01) mount 이므로
 * BANK_01 의 RODATA 는 추가 swap 없이 main code 에서 직접 읽을 수 있다.
 */
#ifdef __Z88DK
#pragma bank 1
#endif

#include <ubox.h>
#include "game.h"

const uint8_t enemy_reborn_frames[ENEMY_REBORN_CYCLE] = { 4, 5 };
const uint8_t walk_frames[WALK_CYCLE]    = { 0, 1, 0, 2 };
const uint8_t stair_frames[STAIR_CYCLE]  = { 0, 1 };
const uint8_t attack_frames[ATTACK_CYCLE] = { 0, 1 };
const uint8_t knife_frames[KNIFE_CYCLE]  = { 0, 1, 2, 3 };
const uint8_t digging_frames[DIGGING_CYCLE] = { 0, 1 };
