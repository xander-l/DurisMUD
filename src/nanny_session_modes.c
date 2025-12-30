/**
 * @file nanny_session_modes.c
 * @brief Game-mode-specific adjustments during session entry.
 *
 * This module keeps special-case logic for CTF/CHAOS logins isolated so the
 * main session flow remains easier to follow and test.
 */

#include "prototypes.h"
#include "structs.h"
#include "utils.h"

/**
 * Apply level adjustments for CTF/CHAOS game modes.
 */
void apply_mode_level_adjustments(P_char ch)
{
  /* CTF - level them up, and setbit hardcore off them! */
#if defined(CTF_MUD) && (CTF_MUD == 1)
  REMOVE_BIT(ch->specials.act2, PLR2_HARDCORE_CHAR);
  if (GET_LEVEL(ch) == 53)
  {
    ch->player.level = 52;
  }
  while (GET_LEVEL(ch) < 53)
  {
    advance_level(ch);
  }
#endif

  /* chaos - level them up, and setbit hardcore off them! */
#if defined(CHAOS_MUD) && (CHAOS_MUD == 1)
  REMOVE_BIT(ch->specials.act2, PLR2_HARDCORE_CHAR);
  if (GET_LEVEL(ch) == 56)
  {
    ch->player.level = 54;
  }
  while (GET_LEVEL(ch) < 56)
  {
    advance_level(ch);
  }
#endif
}
