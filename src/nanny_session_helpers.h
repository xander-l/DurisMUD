/**
 * @file nanny_session_helpers.h
 * @brief Shared helper declarations for session entry/refactor split.
 *
 * This header exposes focused helpers that were extracted from nanny_session.c
 * to keep distinct concerns (EQ wipe handling, game-mode leveling, and post-
 * login actions) isolated in their own translation units.
 */

#ifndef NANNY_SESSION_HELPERS_H
#define NANNY_SESSION_HELPERS_H

#include "structs.h"

/* Perform an EQ wipe for characters that fall below the wipe threshold. */
void perform_eq_wipe(P_char ch);

/*
 * Apply game-mode leveling tweaks (CTF/CHAOS) during session entry.
 * Activation note: these modes are compile-time flags (CTF_MUD/CHAOS_MUD).
 * See src/Makefile for the CFLAGS toggles:
 *   - CTF:   CFLAGS += -DCTF_MUD=1
 *   - CHAOS: CFLAGS += -DCHAOS_MUD=1
 * No in-game command toggles were found for enabling these modes; they are
 * configured at build time.
 */
void apply_mode_level_adjustments(P_char ch);

/* Run post-login view actions (GMCP, look, summon book). */
void run_post_enter_view(P_char ch);

#endif
