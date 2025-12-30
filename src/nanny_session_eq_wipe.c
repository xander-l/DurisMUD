/**
 * @file nanny_session_eq_wipe.c
 * @brief EQ wipe support extracted from nanny_session.c.
 *
 * This module isolates equipment wipe behavior, including item removal and
 * locker cleanup, so that session entry logic can remain focused on login flow.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "comm.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"

/**
 * Remove all equipment and inventory for an EQ wipe, plus cleanup artifacts.
 */
void perform_eq_wipe(P_char ch)
{
  static long longestptime = 0;
  struct time_info_data playing_time;
  int i;
  P_obj obj, obj2;
  char Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

  /* Notify the player about the wipe before deleting equipment. */
  send_to_char("&+ROh shit, it seems we have misplaced your items...\n", ch);
  send_to_char("&+RYour boat isnt where you left it.\n", ch);
  send_to_char("&+RThis can mean only one of two things.  Either you've just been\n"
               "&+R robbed or this i the eq-wipe.  Have a nice day.\r\n", ch);

  /* Remove worn items. */
  for (i = 0; i < MAX_WEAR; i++)
  {
    if (ch->equipment[i])
    {
      extract_obj(unequip_char(ch, i), TRUE);
    }
  }

  /* Remove inventory items. */
  for (obj = ch->carrying; obj; obj = obj2)
  {
    obj2 = obj->next_content;
    extract_obj(obj, TRUE);
    obj = NULL;
  }

  /* Delete the player's locker (and backup) to avoid stale wipes. */
  snprintf(Gbuf2, MAX_STRING_LENGTH, "%c%s", LOWER(*ch->player.name),
           ch->player.name + 1);
  snprintf(Gbuf1, MAX_STRING_LENGTH, "%s/%c/%s.locker", SAVE_DIR, *Gbuf2, Gbuf2);
  unlink(Gbuf1);
  strcat(Gbuf1, ".bak");
  unlink(Gbuf1);

  /* Track the longest played time encountered for diagnostics. */
  if (longestptime < ch->player.time.played)
  {
    longestptime = ch->player.time.played;
    playing_time = real_time_passed(
      (long)((time(0) - ch->player.time.logon) + ch->player.time.played), 0);
    snprintf(Gbuf1, MAX_STRING_LENGTH,
             "New Longest Ptime: '%s' %d with %d %dD%dH%dM%dS", J_NAME(ch),
             ch->only.pc->pid, ch->player.time.played, playing_time.day,
             playing_time.hour, playing_time.minute, playing_time.second);
    debug(Gbuf1);
    logit(LOG_STATUS, Gbuf1);
  }
}
