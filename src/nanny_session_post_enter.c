/**
 * @file nanny_session_post_enter.c
 * @brief Post-login view updates for characters entering the game.
 *
 * This module gathers the final display/setup actions (GMCP updates, room
 * look, and summon-book invocation) that occur after a player fully enters.
 */

#include "gmcp.h"
#include "prototypes.h"
#include "structs.h"

/**
 * Run post-login view actions (GMCP updates, look, summon book).
 */
void run_post_enter_view(P_char ch)
{
  /* Send GMCP data for WebSocket clients. */
  gmcp_char_status(ch);
  gmcp_char_vitals(ch);
  gmcp_quest_status(ch);

  /* Present the room to the player once they are fully logged in. */
  do_look(ch, 0, -4);

  /* Summon the class book automatically for eligible characters. */
  if (has_innate(ch, INNATE_SUMMON_BOOK))
  {
    do_summon_book(ch, "", 0);
  }

  /* Auto-summon a shaman totem for characters trained in totemic mastery. */
  if (GET_CHAR_SKILL(ch, SKILL_TOTEMIC_MASTERY))
  {
    do_summon_totem(ch, "", 0);
  }

  /* Auto-load soulbound item if the character has one configured. */
  if (has_soulbind(ch) != 0)
  {
    load_soulbind(ch);
  }
}
