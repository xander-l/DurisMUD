/**
 * @file nanny_account_flow.c
 * @brief Account/login connection handling extracted from nanny.c.
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "account.h"
#include "comm.h"
#include "db.h"
#include "nanny_account_flow.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"

extern int email_in_use(char *, char *);

int nanny_account_flow(P_desc d, char *arg)
{
  char Gbuf1[MAX_STRING_LENGTH];

  if (!d || !arg)
  {
    logit(LOG_DEBUG, "nanny_account_flow: invalid input (d=%p arg=%p)",
          (void *)d, (void *)arg);
    return 0;
  }

  switch (STATE(d))
  {
#ifdef USE_ACCOUNT
    case CON_GET_ACCT_NAME:
      select_accountname(d, arg);
      return 1;
    case CON_GET_ACCT_PASSWD:
      get_account_password(d, arg);
      return 1;
#ifdef REQUIRE_EMAIL_VERIFICATION
    case CON_CONFIRM_ACCT:
      confirm_account(d, arg);
      return 1;
#endif
    case CON_VERIFY_NEW_ACCT_NAME:
      verify_account_name(d, arg);
      return 1;
    case CON_GET_NEW_ACCT_EMAIL:
      get_new_account_email(d, arg);
      return 1;
    case CON_VERIFY_NEW_ACCT_EMAIL:
      verify_new_account_email(d, arg);
      return 1;
    case CON_GET_NEW_ACCT_PASSWD:
      get_new_account_password(d, arg);
      return 1;
    case CON_VERIFY_NEW_ACCT_PASSWD:
      verify_new_account_password(d, arg);
      return 1;
    case CON_VERIFY_NEW_ACCT_INFO:
      verify_new_account_information(d, arg);
      return 1;
    case CON_ACCT_SELECT_CHAR:
      account_select_char(d, arg);
      return 1;
    case CON_ACCT_CONFIRM_CHAR:
      account_confirm_char(d, arg);
      return 1;
    case CON_ACCT_NEW_CHAR:
      account_new_char(d, arg);
      return 1;
    case CON_ACCT_DELETE_CHAR:
      account_delete_char(d, arg);
      return 1;
    case CON_ACCT_DISPLAY_INFO:
      account_display_info(d, arg);
      return 1;
    case CON_ACCT_CHANGE_EMAIL:
      get_new_account_email(d, arg);
      return 1;
    case CON_ACCT_CHANGE_PASSWD:
      get_new_account_password(d, arg);
      return 1;
    case CON_ACCT_DELETE_ACCT:
      delete_account(d, arg);
      return 1;
    case CON_ACCT_VERIFY_DELETE_ACCT:
      verify_delete_account(d, arg);
      return 1;
    case CON_ACCT_NEW_CHAR_NAME:
      account_new_char_name(d, arg);
      return 1;
    case CON_ACCT_RMOTD:
      display_account_menu(d, NULL);
      STATE(d) = CON_DISPLAY_ACCT_MENU;
      return 1;
#endif
#ifndef USE_ACCOUNT
    case CON_ENTER_LOGIN:
      snprintf(d->registered_login, MAX_STRING_LENGTH, "%s", arg);
      SEND_TO_Q("\n\rNow, the hostname part of your email address: ", d);
      STATE(d) = CON_ENTER_HOST;
      return 1;
    case CON_ENTER_HOST:
      snprintf(d->registered_host, MAX_STRING_LENGTH, "%s", arg);
      if (email_in_use(d->registered_login, d->registered_host))
      {
        SEND_TO_Q("That email is in use already.\n\r", d);
        STATE(d) = CON_EXIT;
        SEND_TO_Q("\n\rPRESS RETURN.", d);
        return 1;
      }
      snprintf(Gbuf1, MAX_STRING_LENGTH,
               "Your email is registered as %s@%s, is this correct? ",
               d->registered_login, d->registered_host);
      SEND_TO_Q(Gbuf1, d);
      STATE(d) = CON_CONFIRM_EMAIL;
      return 1;
    case CON_CONFIRM_EMAIL:
      for (; isspace(*arg); arg++)
      {
      }
      if (*arg == 'y' || *arg == 'Y')
      {                           /* continue */
        SEND_TO_Q(racewars, d);
        STATE(d) = CON_SHOW_RACE_TABLE;
      }
      else
      {                           /* wrong email */
        SEND_TO_Q("Okay, resetting...\n\r", d);
        SEND_TO_Q("What is your login or userid portion of your email? ", d);
        STATE(d) = CON_ENTER_LOGIN;
      }
      return 1;
    case CON_PWD_GET:
    case CON_PWD_CONF:
    case CON_PWD_NEW:
    case CON_PWD_GET_NEW:
    case CON_PWD_NO_CONF:
    case CON_PWD_D_CONF:
    case CON_PWD_NORM:
      /* skip whitespaces */
      for (; isspace(*arg); arg++)
      {
      }

      if (STATE(d) == CON_PWD_NEW ||
          STATE(d) == CON_PWD_GET || STATE(d) == CON_PWD_NORM)
      {
        /*
         ** Since we have turned off echoing for telnet client,
         ** if a telnet client is indeed used, we need to skip the
         ** initial 3 bytes ( -1, -3, 1 ) if they are sent back by
         ** client program.
         */

        if (*arg == -1)
        {
          if (arg[1] != '0' && arg[2] != '0')
          {
            if (arg[3] == '0')
            {                     /* Password on next read  */
              return 1;
            }
            else
            {                     /* Password available */
              arg = arg + 3;
            }
          }
          else
            close_socket(d);
        }
      }
      select_pwd(d, arg);
      return 1;
#endif
    default:
      return 0;
  }
}
