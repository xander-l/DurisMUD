/**
 * @file nanny_swapstats.h
 * @brief Swap-stat handling extracted from nanny.c.
 */

#ifndef NANNY_SWAPSTATS_H
#define NANNY_SWAPSTATS_H

#include "structs.h"

/* Refactored swap-stat handlers return 1 on success, 0 on error. */
int nanny_show_swapstat(P_desc d);
int nanny_select_swapstat(P_desc d, char *arg);
int nanny_swapstat(P_desc d, char *arg);
int nanny_swapstats(P_char ch, int stat1, int stat2);

#endif
