/**
 * @file nanny_account_flow.h
 * @brief Account/login connection handling extracted from nanny.c.
 */

#ifndef NANNY_ACCOUNT_FLOW_H
#define NANNY_ACCOUNT_FLOW_H

#include "structs.h"

/*
 * Handle account/login related connection states.
 * Returns 1 when handled, 0 when caller should fall back to legacy logic.
 */
int nanny_account_flow(P_desc d, char *arg);

#endif
