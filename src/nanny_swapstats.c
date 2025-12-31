/**
 * @file nanny_swapstats.c
 * @brief Swap-stat handling extracted from nanny.c.
 *
 * This module centralizes the stat swap prompts and mutations used during
 * character creation, returning a status code so nanny.c can fall back to the
 * legacy implementation if needed.
 */

#include <ctype.h>

#include "comm.h"
#include "db.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"

/**
 * Show the swap-stat prompt legend.
 *
 * @return 1 on success, 0 on error.
 */
int nanny_show_swapstat(P_desc d)
{
  if (!d)
  {
    logit(LOG_DEBUG, "nanny_show_swapstat: null descriptor");
    return 0;
  }

  SEND_TO_Q("\r\nThe following letters correspond to the stats:\r\n", d);
  SEND_TO_Q("(S)trength            (P)ower\n\r"
            "(D)exterity           (I)ntelligence\n\r"
            "(A)gility             (W)isdom\n\r"
            "(C)onstitution        C(h)arisma\n\r"
            "(L)uck\n\r",
            d);
  SEND_TO_Q("\r\nEnter two letters separated by a space to swap: \r\n", d);
  return 1;
}

/**
 * Handle the yes/no prompt to enter swap-stat selection.
 *
 * @return 1 on success, 0 on error.
 */
int nanny_select_swapstat(P_desc d, char *arg)
{
  if (!d || !arg)
  {
    logit(LOG_DEBUG, "nanny_select_swapstat: invalid input (d=%p arg=%p)",
          (void *)d, (void *)arg);
    return 0;
  }

  /* Skip whitespace to find the first meaningful response. */
  for (; isspace(*arg); arg++)
  {
  }

  switch (*arg)
  {
    case 'N':
    case 'n':
      SEND_TO_Q("\r\n\r\nAccepting these stats.\r\n\r\n", d);
      display_characteristics(d);
      display_stats(d);
      SEND_TO_Q(keepchar, d);
      STATE(d) = CON_KEEPCHAR;
      break;
    case 'Y':
    case 'y':
      nanny_show_swapstat(d);
      STATE(d) = CON_SWAPSTAT;
      break;
    default:
      SEND_TO_Q("\r\nUnrecognized response.\r\n", d);
      display_stats(d);
      SEND_TO_Q("Do you want to swap stats (Y/N): ", d);
      STATE(d) = CON_SWAPSTATYN;
      break;
  }

  return 1;
}

/**
 * Parse a swap-stat request and apply the swap.
 *
 * @return 1 on success, 0 on error.
 */
int nanny_swapstat(P_desc d, char *arg)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int stat1, stat2;

  if (!d || !arg)
  {
    logit(LOG_DEBUG, "nanny_swapstat: invalid input (d=%p arg=%p)",
          (void *)d, (void *)arg);
    return 0;
  }

  if (!d->character)
  {
    logit(LOG_DEBUG, "nanny_swapstat: descriptor missing character");
    return 0;
  }

  arg = one_argument(arg, arg1);
  arg = one_argument(arg, arg2);
  if (!*arg1 || !*arg2)
  {
    SEND_TO_Q("\r\nUnrecognized response.\r\n", d);
    display_stats(d);
    SEND_TO_Q("Do you want to swap stats (Y/N): ", d);
    STATE(d) = CON_SWAPSTATYN;
    return 1;
  }

  switch (LOWER(*arg1))
  {
    case 's':
      stat1 = 1;
      break;
    case 'd':
      stat1 = 2;
      break;
    case 'a':
      stat1 = 3;
      break;
    case 'c':
      stat1 = 4;
      break;
    case 'p':
      stat1 = 5;
      break;
    case 'i':
      stat1 = 6;
      break;
    case 'w':
      stat1 = 7;
      break;
    case 'h':
      stat1 = 8;
      break;
    case 'l':
      stat1 = 9;
      break;
    default:
      stat1 = -1;
  }

  switch (LOWER(*arg2))
  {
    case 's':
      stat2 = 1;
      break;
    case 'd':
      stat2 = 2;
      break;
    case 'a':
      stat2 = 3;
      break;
    case 'c':
      stat2 = 4;
      break;
    case 'p':
      stat2 = 5;
      break;
    case 'i':
      stat2 = 6;
      break;
    case 'w':
      stat2 = 7;
      break;
    case 'h':
      stat2 = 8;
      break;
    case 'l':
      stat2 = 9;
      break;
    default:
      stat2 = -1;
  }

  if (stat1 == -1 || stat2 == -1)
  {
    SEND_TO_Q("\r\nUnrecognized response.\r\n", d);
    display_stats(d);
    SEND_TO_Q("Do you want to swap stats (Y/N): ", d);
    STATE(d) = CON_SWAPSTATYN;
    return 1;
  }

  nanny_swapstats(d->character, stat1, stat2);

  display_stats(d);
  SEND_TO_Q("Do you want to swap more stats (Y/N): ", d);
  STATE(d) = CON_SWAPSTATYN;

  return 1;
}

/**
 * Swap two base stats on the character.
 *
 * @return 1 on success, 0 on error.
 */
int nanny_swapstats(P_char ch, int stat1, int stat2)
{
  int tmp;
  sh_int *pstat1;

  if (!ch)
  {
    logit(LOG_DEBUG, "nanny_swapstats: null character");
    return 0;
  }

  /* Record stat1 value and location. */
  switch (stat1)
  {
    case 1:
      tmp = ch->base_stats.Str;
      pstat1 = &(ch->base_stats.Str);
      break;
    case 2:
      tmp = ch->base_stats.Dex;
      pstat1 = &(ch->base_stats.Dex);
      break;
    case 3:
      tmp = ch->base_stats.Agi;
      pstat1 = &(ch->base_stats.Agi);
      break;
    case 4:
      tmp = ch->base_stats.Con;
      pstat1 = &(ch->base_stats.Con);
      break;
    case 5:
      tmp = ch->base_stats.Pow;
      pstat1 = &(ch->base_stats.Pow);
      break;
    case 6:
      tmp = ch->base_stats.Int;
      pstat1 = &(ch->base_stats.Int);
      break;
    case 7:
      tmp = ch->base_stats.Wis;
      pstat1 = &(ch->base_stats.Wis);
      break;
    case 8:
      tmp = ch->base_stats.Cha;
      pstat1 = &(ch->base_stats.Cha);
      break;
    case 9:
      tmp = ch->base_stats.Luk;
      pstat1 = &(ch->base_stats.Luk);
      break;
    default:
      send_to_char("Error in swapstats Part 1!  Tell a God.\n\r", ch);
      return 1;
  }

  /* Swap: put stat2 value into stat1 and tmp into stat2 value. */
  switch (stat2)
  {
    case 1:
      *pstat1 = ch->base_stats.Str;
      ch->base_stats.Str = tmp;
      break;
    case 2:
      *pstat1 = ch->base_stats.Dex;
      ch->base_stats.Dex = tmp;
      break;
    case 3:
      *pstat1 = ch->base_stats.Agi;
      ch->base_stats.Agi = tmp;
      break;
    case 4:
      *pstat1 = ch->base_stats.Con;
      ch->base_stats.Con = tmp;
      break;
    case 5:
      *pstat1 = ch->base_stats.Pow;
      ch->base_stats.Pow = tmp;
      break;
    case 6:
      *pstat1 = ch->base_stats.Int;
      ch->base_stats.Int = tmp;
      break;
    case 7:
      *pstat1 = ch->base_stats.Wis;
      ch->base_stats.Wis = tmp;
      break;
    case 8:
      *pstat1 = ch->base_stats.Cha;
      ch->base_stats.Cha = tmp;
      break;
    case 9:
      *pstat1 = ch->base_stats.Luk;
      ch->base_stats.Luk = tmp;
      break;
    default:
      send_to_char("Error in swapstats Part 2!  Tell a God.\n\r", ch);
      return 1;
  }

  ch->curr_stats = ch->base_stats;
  return 1;
}
