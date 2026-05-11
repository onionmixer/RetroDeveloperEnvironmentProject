/**
 * Module storing the generated data.
 *
 * Phase C (Konami 64K cart): all const data here is placed into RODATA_1
 * which z88dk maps to BANK_01 of the cart. Konami mapper's reset state
 * already mounts BANK_01 at $8000-$BFFF (RomKonami::reset → block 2,3 for
 * pages 4,5), so main code can reference these arrays without any explicit
 * select_bank() call. Splitting data.c out of the main bank brings main
 * code under 16 KiB — keeping it within page 1 ($4000-$7FFF) only, which
 * avoids the Konami $6000+ mapper-trigger trap that hit the first attempt.
 *
 * SDCC build ignores #pragma bank silently. Phase A/B (ASCII16) keeps
 * data in main bank — for those, the directive does no harm but its
 * placement effect is mapper-specific. Only Phase C's compile_phaseC.sh
 * actually wants this routed to RODATA_1.
 */
#ifdef __Z88DK
#pragma bank 1
#endif

#define LOCAL
#include <tiles.h>
#include <playermove.h>
#include <playerpickax.h>
#include <playerknife.h>
#include <playerattack.h>
#include <playerdigging.h>
#include <enemy_data.h>
#include <knife_data.h>
#include <door.h>
#include <map_summary.h>

