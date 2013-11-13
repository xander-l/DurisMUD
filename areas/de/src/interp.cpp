//
//  File: interp.cpp     originally part of durisEdit
//
//  Usage: contains heart of command interpreter
//

/*
 * Copyright (c) 1995-2007, Michael Glosenger
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of Michael Glosenger may not be used to endorse or promote 
 *       products derived from this software without specific prior written 
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MICHAEL GLOSENGER ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
 * EVENT SHALL MICHAEL GLOSENGER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>

#include "graphcon.h"

#include "fh.h"
#include "types.h"

#include "vardef.h"
#include "de.h"
#include "command/alias.h"
#include "command/maincomm.h"

extern variable *g_varHead;
extern command g_mainCommands[];
extern alias *g_aliasHead;
extern uint g_commandsEntered;


//
// takes inputStrn and expands it into outputStrn1 and outputStrn2 as it expands possibly multiple times -
// returns pointer to ultimate string, both output strings must be intMaxLen + 1 for trailing null
//

char *expandUserInputString(char *inputStrn, char *outputStrn1, char *outputStrn2, const size_t intMaxLen)
{
  char *srcptr = inputStrn;
  char *targptr = outputStrn1;
  char *targstrn = outputStrn1;
  size_t len = 0, numbExpansions = 0;
  bool targIsStrn1 = true;
  bool expandedSomething, atStart = true, copySrc;

  
  do
  {
    expandedSomething = false;

    while (*srcptr)
    {
      copySrc = true;

     // check if this is the start of a new command (start of string, semicolon), then check if it is an actual
     // alias or set command, or if it is an alias that needs to be expanded

      if (atStart)
      {
       // skip leading spaces

        while (*srcptr == ' ')
          srcptr++;

        usint matchingCmd = 0;

        checkCommands(srcptr, g_mainCommands, NULL, NULL, NULL, &matchingCmd);

        if ((matchingCmd == COMM_ALIAS) || (matchingCmd == COMM_SETVAR))
        {
          strncpy(targptr, srcptr, intMaxLen - len);

          targstrn[intMaxLen] = '\0';

          return targstrn;
        }

       // not an alias command, but may be an alias

        char *aliasstart = srcptr;

        while (*srcptr && (*srcptr != ' ') && (*srcptr != ';'))
          srcptr++;

        const char srcorig = *srcptr;

        *srcptr = '\0';

        alias *aliasMatch = getAlias(g_aliasHead, aliasstart);

        if (aliasMatch)
        {
         // get args to alias

          *srcptr = srcorig;

          while (*srcptr && (*srcptr == ' '))
            srcptr++;

          char *argstart = srcptr;

          while (*srcptr && (*srcptr != ';'))
            srcptr++;

          const char srcargorig = *srcptr;

          *srcptr = '\0';

         // expand alias

          const size_t targadded = expandAliasArgs(targptr, aliasMatch->commandStrn, argstart, intMaxLen - len);

          len += targadded;
          targptr += targadded;

          if (len > intMaxLen)
          {
            targstrn[intMaxLen] = '\0';
            return targstrn;
          }

          *srcptr = srcargorig;

          expandedSomething = true;

          copySrc = false;
        }
        else
        {
          *srcptr = srcorig;
          srcptr = aliasstart;
        }
      }

      atStart = false;

     // check if this is a variable; if so, get variable name, and if it's valid, replace things

      if (*srcptr == '$')
      {
        srcptr++;
        char *varstart = srcptr;

        while (*srcptr && (*srcptr != ' ') && (*srcptr != ';'))
          srcptr++;

        const char varendorig = *srcptr;
        *srcptr = '\0';

        if (varExists(g_varHead, varstart))
        {
          getVarStrn(g_varHead, varstart, targptr, "", intMaxLen - len);  // might be off by 1

          const size_t varlen = getVarStrnLen(g_varHead, varstart);

          len += varlen;
          targptr += varlen;

          if (len > intMaxLen)
          {
            targstrn[intMaxLen] = '\0';
            return targstrn;
          }

          *srcptr = varendorig;
          srcptr--;  // will get inc'ed below

          expandedSomething = true;
        }

       // var name didn't match

        else
        {
          *srcptr = varendorig;
          *targptr = '$';

          srcptr = varstart - 1;  // incremented below
        }
      }
      else
      {
       // don't copy on alias expansion

        if (copySrc)
        {
          if (*srcptr == ';')
            atStart = true;

          *targptr = *srcptr;
          len++;

          if (len > intMaxLen)
          {
            targstrn[intMaxLen] = '\0';
            return targstrn;
          }
        }
      }

      if (copySrc)
      {
        srcptr++;
        targptr++;
      }
    }

    *targptr = '\0';

   // if an alias or a var has been expanded, switch things for another go 'round

    if (expandedSomething)
    {
      numbExpansions++;

      if (numbExpansions > intMaxLen)
      {
        _outtext("\nProbable infinite alias/variable loop - aborting.\n");

        return targstrn;
      }

      if (targIsStrn1)
      {
        srcptr = outputStrn1;
        targptr = targstrn = outputStrn2;
        targIsStrn1 = false;
      }
      else
      {
        srcptr = outputStrn2;
        targptr = targstrn = outputStrn1;
        targIsStrn1 = true;
      }

      atStart = true;
      len = 0;
    }
  } while (expandedSomething == true);

  return targstrn;
}


//
// interpCommands : Grab a string from the user and interpret it, taking
//                  action based on what the user typed - returns true if
//                  the user wants to quit
//
//   inputStrn : if non-NULL, treats this as user input
//  dispPrompt : if true, displays prompt after all is said and done
//

bool interpCommands(const char *inputStrn, const bool dispPrompt)
{
  char parseStrn1[MAX_EXPANDEDPROMPTINPUT_LEN + 1], parseStrn2[MAX_EXPANDEDPROMPTINPUT_LEN + 1], 
       origstrn[MAX_PROMPTINPUT_LEN + 1];  // origstrn is used to store user input for 'last command' val
  char *srcptr;


 // grab the string

  if (!inputStrn)
  {
    getStrn(origstrn, MAX_PROMPTINPUT_LEN, 0, 7, ' ', "", true, true);
  }
  else
  {
    strncpy(origstrn, inputStrn, MAX_PROMPTINPUT_LEN);
    origstrn[MAX_PROMPTINPUT_LEN] = '\0';
  }

  nolf(origstrn);

 // if user typed '!', repeat last command

  if (!strcmp(origstrn, "!"))
  {
    strncpy(origstrn, getLastCommandStrn(), MAX_PROMPTINPUT_LEN);
    origstrn[MAX_PROMPTINPUT_LEN] = '\0';
  }

 // expand the input string

  srcptr = expandUserInputString(origstrn, parseStrn1, parseStrn2, MAX_EXPANDEDPROMPTINPUT_LEN);

  while (true)
  {
   // skip past whitespace at start

    while (*srcptr == ' ')
      srcptr++;

    char *srcstart = srcptr;

   // check if this is an alias or variable command; if so, everything gets passed to it

    usint matchingCmd = 0;

    checkCommands(srcstart, g_mainCommands, NULL, NULL, NULL, &matchingCmd);

    if ((matchingCmd == COMM_ALIAS) || (matchingCmd == COMM_SETVAR))
    {
      checkCommands(srcstart, g_mainCommands, MAIN_PROMPT_COMM_NOT_FOUND, mainExecCommand, NULL, NULL);

      break;  // done processing
    }

   // find end of this token

    while (*srcptr && (*srcptr != ';'))
      srcptr++;

    char *srcreset = srcptr;
    char srcresetorig = *srcreset;

    *srcreset = '\0';

    if (checkCommands(srcstart, g_mainCommands, MAIN_PROMPT_COMM_NOT_FOUND, mainExecCommand, NULL, NULL))
      return true;

   // check for periodic saving

    if (getSaveEveryXCommandsVal())
    {
      g_commandsEntered++;

      if (g_commandsEntered >= getSaveHowOftenVal())
      {
        g_commandsEntered = 0;

        displayColorString("&nAutosaving...");

        writeFiles();
      }
    }

    *srcreset = srcresetorig;

   // end of string, done

    if (*srcptr == '\0')
      break;

    srcptr++;  // past semicolon
  }

 // set 'last command executed' var

  if (*origstrn && strcmp(origstrn, "!"))
    addVar(&g_varHead, VAR_LASTCOMMAND_NAME, origstrn);

 // display the prompt

  if (dispPrompt)
    displayPrompt();

 // user didn't quit

  return false;
}
