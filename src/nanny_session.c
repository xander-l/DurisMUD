/**
 * @file nanny_session.c
 * @brief Session lifecycle helpers extracted from nanny.c.
 *
 * This module centralizes session-related utilities that were previously
 * embedded in nanny.c (telnet echo control, player counts, and login event
 * scheduling). Keeping these in a dedicated file reduces nanny.c size and
 * clarifies ownership of login/session behavior.
 *
 * The implementations below preserve the existing behavior while adding
 * clearer structure and inline commentary to make future refactors safer.
 */

#include <arpa/telnet.h>
#include "comm.h"
#include "events.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"

/* External data that remains owned by the main game systems. */
extern P_desc descriptor_list;

/**
 * Turn on echoing (specific to telnet client).
 * This only works with telnet clients by sending the correct telnet sequence.
 */
void echo_on(P_desc d)
{
  char on_string[] = {
    (char)IAC,
    (char)WONT,
    (char)TELOPT_ECHO,
    (char)TELOPT_NAOFFD,
    (char)TELOPT_NAOCRD,
    (char)0
  };

  /* Send raw TELNET control sequence to restore local echo. */
  SEND_TO_Q(on_string, d);
}

/**
 * Turn off echoing (specific to telnet client).
 */
void echo_off(P_desc d)
{
  char off_string[] = {
    (char)IAC,
    (char)WILL,
    (char)TELOPT_ECHO,
    (char)0,
  };

  /* Send raw TELNET control sequence to suppress local echo. */
  SEND_TO_Q(off_string, d);
}

/**
 * Count the number of players connected (non-immortal descriptors).
 */
int number_of_players(void)
{
  P_desc d;
  int count = 0;

  for (d = descriptor_list; d != NULL; d = d->next)
  {
    if (!(d->character) || (GET_LEVEL(d->character) < MINLVLIMMORTAL))
      count++;
  }

  return count;
}

/**
 * Validate and reroute a hometown for emergency situations.
 * Currently returns the requested room unchanged.
 */
int alt_hometown_check(P_char ch, int room, int count)
{
  /* Placeholder for future attack-routing logic. */
  (void)ch;
  (void)count;
  return room;
}

/**
 * Schedule standard PC events after the character is fully connected.
 */
void schedule_pc_events(P_char ch)
{
  /* Guard against invalid pointers and dead characters. */
  if (!IS_ALIVE(ch))
  {
    debug("schedule_pc_events: DEAD/NONEXISTANT char %s '%s'.",
      (ch == NULL) ? "!" : IS_NPC(ch) ? "NPC" : "PC",
      (ch == NULL) ? "NULL" : J_NAME(ch));
    logit(LOG_DEBUG, "schedule_pc_events: DEAD/NONEXISTANT char %s '%s'.",
      (ch == NULL) ? "!" : IS_NPC(ch) ? "NPC" : "PC",
      (ch == NULL) ? "NULL" : J_NAME(ch));
    return;
  }

  /* Autosave and innate/class-based timers. */
  add_event(event_autosave, 1200, ch, 0, 0, 0, 0, 0);
  if (has_innate(ch, INNATE_HATRED))
    add_event(event_hatred_check, get_property("innate.timer.hatred", WAIT_SEC),
              ch, 0, 0, 0, 0, 0);
  if (GET_CHAR_SKILL(ch, SKILL_SMITE_EVIL))
    add_event(event_smite_evil,
              get_property("skill.timer.secs.smiteEvil", 5) * WAIT_SEC, ch, 0,
              0, 0, 0, 0);
  if (GET_RACE(ch) == RACE_HALFLING)
    add_event(event_halfling_check, 1, ch, 0, 0, 0, 0, 0);

  /* Periodic aura spell checks. */
  if (affected_by_spell(ch, SPELL_RIGHTEOUS_AURA))
    add_event(event_righteous_aura_check, WAIT_SEC, ch, 0, 0, 0, 0, 0);

  if (affected_by_spell(ch, SPELL_BLEAK_FOEMAN))
    add_event(event_bleak_foeman_check, WAIT_SEC, ch, 0, 0, 0, 0, 0);
}
