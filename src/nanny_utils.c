/****************************************************************************
 *  File: nanny_utils.c                                     Part of Duris   *
 *  Usage: utilities shared by the nanny/login flow                        *
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.     *
 *  Copyright 1994 - 2008 - Duris Systems Ltd.                              *
 ****************************************************************************/

#include <stdio.h>

#include "comm.h"
#include "structs.h"
#include "utils.h"
#include "nanny_utils.h"
#include "assocs.h"
#include "prototypes.h"

static char *hint_array[1000];
static int iLOADED = 0;

void loadHints(void)
{
  FILE *f;
  char buf2[MAX_STR_NORMAL * 10];
  int i = 0;

  f = fopen("lib/information/hints.txt", "r");

  if (!f)
    return;

  while (!feof(f))
  {
    if (fgets(buf2, MAX_STR_NORMAL * 10 - 1, f))
    {
      hint_array[i] = str_dup(buf2);
      i++;
    }
  }

  iLOADED = i;
  fclose(f);
}

int tossHint(P_char ch)
{
  char buf2[MAX_STR_NORMAL * 10];

  if (iLOADED < 1)
    return 0;
  snprintf(buf2, MAX_STRING_LENGTH, "&+MHint: &+m%s", hint_array[number(0, iLOADED - 1)]);
  send_to_char(buf2, ch);
  return 0;
}

void Decrypt(char *text, int sizeOfText, const char *key, int sizeOfKey)
{
  int offSet = 0;
  int i = 0;

  for (; i < sizeOfText; ++i, ++offSet)
  {
    if (offSet >= sizeOfKey)
      offSet = 0;

    int value = text[i];
    int keyValue = key[offSet];

    value -= keyValue;

    char decryptedChar = value;

    text[i] = decryptedChar;
  }
}
