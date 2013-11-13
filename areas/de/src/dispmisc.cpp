//
//  File: dispmisc.cpp   originally part of durisEdit
//
//  Usage: generally contains functions that display a list of
//         values/items
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "graphcon.h"

#include "types.h"
#include "fh.h"

#include "obj/object.h"

#include "obj/material.h"
#include "spells.h"




//
//   displayList : display list of enumerated values, pausing every screenful
//
//         startVal : first val (inclusive)
//           endVal : last val
//       searchStrn : if non-NULL, only display stringvals that have this substring
//     getStringVal : function that returns string associated with enumerated value
// getIdentifierVal : if non-NULL, display second string associated with enumerated value - shown on the
//                    left side
//       ignoreStrn : if non-NULL, don't display values with stringvals that match this
//         whatList : type of thing being listed
//

void displayList(const int startVal, const int endVal, const char *searchStrn,
                 const char *(*getStringVal)(const int val), const char *(*getIdentifierVal)(const int val),
                 const char *ignoreStrn, const char *whatList)
{
  int i;
  size_t numbLines = 1;
  char strn[1024];
  bool foundOne = false;


  _outtext("\n\n");

  for (i = startVal; i <= endVal; i++)
  {
    bool displayIt = false;
    char itemStrn[512];
    
    strncpy(itemStrn, getStringVal(i), 511);
    itemStrn[511] = '\0';

    if (ignoreStrn && !strcmp(ignoreStrn, itemStrn)) 
      continue;

    if (!searchStrn)
    {
      displayIt = true;
      foundOne = true;
    }
    else
    {
      char noColorItemStrn[512];

      strcpy(noColorItemStrn, itemStrn);

      remColorCodes(noColorItemStrn);

      if (strstrnocase(noColorItemStrn, searchStrn))
      {
        displayIt = true;
        foundOne = true;
      }
    }

    if (displayIt)
    {
      if (getIdentifierVal)
      {
        sprintf(strn, "  %s - %s\n", getIdentifierVal(i), itemStrn);
      }
      else
      {
        sprintf(strn, "  %2u - %s\n", i, itemStrn);
      }

      if (checkPause(strn, numbLines))
        return;
    }
  }

  if (!foundOne)
  {
    sprintf(strn, "No matching %ss found.\n", whatList);

    _outtext(strn);
  }

  _outtext("\n");
}


//
// displayMaterialList
//

void displayMaterialList(const char *searchStrn)
{
  displayList(MAT_LOWEST, MAT_HIGHEST, searchStrn, getMaterialStrn, NULL, NULL, "material");
}


//
// displaySpellList
//

void displaySpellList(const char *searchStrn)
{
  displayList(1, LAST_SPELL - 2, searchStrn, getSpellTypeStrn, NULL, "Undefined", "spell");
}


//
// displaySkillList
//

void displaySkillList(const char *searchStrn)
{
  displayList(FIRST_SKILL, LAST_SKILL - 2, searchStrn, getSkillTypeStrn, NULL, "Undefined", "skill");
}


//
// displayCommands : display list of DE commands
//

void displayCommands(const command *startCmd)
{
  const command *cmd = startCmd;
  char strn[256];
  size_t lines = 1;
  uint numb = 0;


  _outtext("\n\n");

  while (cmd->commandStrn)
  {
   // let's display three per line, shall we?

    sprintf(strn, "%-25s", cmd->commandStrn);
    numb++;

    cmd++;

    if (cmd->commandStrn)
    {
      sprintf(strn + strlen(strn), "%-25s", cmd->commandStrn);
      numb++;

      cmd++;
    }
    else strcat(strn, "\n");

    if (cmd->commandStrn)
    {
      sprintf(strn + strlen(strn), "%-25s\n", cmd->commandStrn);
      numb++;

      cmd++;
    }
    else strcat(strn, "\n");

   // if at end of list, force pause

    if (checkPause(strn, lines))
      return;
  }

  sprintf(strn, "\n\nTotal number of commands: %u\n\n", numb);
  _outtext(strn);
}


//
// checkPause : returns TRUE if user wants to quit out of the list
//
//    strn  : if non-NULL, string in which to count number of new lines
//    lines : ref to lines var
//

bool checkPause(const char *strn, size_t &lines)
{
  size_t linesNeeded; 

  if (strn)
    linesNeeded = numbLinesStringNeeds(strn);
  else
    linesNeeded = 0;

 // if new string would move offscreen and something has already been displayed, pause before display

  if ((lines > 1) && (lines + linesNeeded > (getScreenHeight() - 2)))
  {
    displayColorString("\n&+CPress a key to continue or Q to quit..&n");

    const usint ch = toupper(getkey());

    _outtext("\n\n");

    lines = linesNeeded + 1;

    if (ch == 'Q')
      return true;
  }
  else
  {
    lines += linesNeeded;
  }

  if (strn)
    displayColorString(strn);

  return false;
}


//
// displayAllocError : couldn't alloc something
//
//   whatAlloc : objectType, etc
//  whereAlloc : function name
//

void displayAllocError(const char *whatAlloc, const char *whereAlloc)
{
 // avoid using stack so as to not use memory

  _outtext("\nError: Out of memory - couldn't allocate ");
  _outtext(whatAlloc);
  _outtext(" [");
  _outtext(whereAlloc);
  _outtext("()]\n");
}


//
// displayArgHelp : display list of valid command-line arguments
//

void displayArgHelp(void)
{
  const char *helpstr =
           "\n"
           "Usage: de <zone filename> [options]\n"
           "\n"
           " Options must start with either / or -, i.e. 'de castle -d /ll=10000'\n"
           "\n"
           " Valid options are:\n"
           "\n"
           "  d        read area files from subdirectories, i.e. .wld from wld/\n"
           "  r        read only .wld file, only edit rooms\n"
           "  o        read only .obj file, only edit objects\n"
           "  m        read only .mob file, only edit mobs\n"
           "  ll=X     change initial maximum vnum to X\n"
           "  novc     don't require that vnum input references exist (default)\n"
           "  vc       require that vnum input references exist\n"
           "  noecc    don't check worn equipment anti-class bits (default)\n"
           "  ecc      check worn equipment anti-class bits\n"
           "  nolc     ignore .zon file load limits\n"
           "  lc       respect .zon file load limits (default)\n"
           "  nozfc    don't ensure that zone number in .zon matches .wld\n"
           "  zfc      ensure that zone number in .zon matches .wld (default)\n"
           "  izs      ignore S line in .zon, stopping only at end of file\n"
           "  h        display this help\n";

#ifdef _WIN32
  _outtext(helpstr);
  _outtext("\n");
#else
  puts(helpstr);
#endif
}
