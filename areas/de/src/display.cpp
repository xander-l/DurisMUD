//
//  File: display.cpp    originally part of durisEdit
//
//  Usage: functions that display text onscreen and are not easily
//         classified into another file
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
#include <ctype.h>


#include "fh.h"
#include "types.h"
#include "boolean.h"
#include "graphcon.h"
#include "zone/zone.h"
#include "obj/object.h"
#include "keys.h"

#include "display.h"



extern char g_promptString[];


//
// displayColorString : Displays a string in color, changing colors as per the
//                      Duris color codes - const string version
//
//      *strn : Pointer to string to display
//

void displayColorString(const char *strn)
{
  const bool longStrn = (strlen(strn) >= 16384);
  char outtextstrn[16384];
  char *outstrn;


  if (longStrn)
  {
    outstrn = new char[strlen(strn) + 1];

    if (!outstrn)
    {
      _outtext("\ndisplayColorString(): ERROR - const strn is too long (> 16384 bytes) and alloc failed\n\n");

      return;
    }
  }
  else
  {
    outstrn = outtextstrn;
  }

  strcpy(outstrn, strn);

  displayColorString(outtextstrn);

  if (longStrn)
    delete[] outstrn;
}


//
// displayColorString : Displays a string in color, changing colors as per the
//                      Duris color codes - non-const string version - has no size limit
//
//                      when the function is done, strn is still the same as it was - but
//                      chars are set to null and back during operation, so this algorithm
//                      cannot be used on Data Execute Protection-type memory
//
//      *strn : Pointer to string to display
//

void displayColorString(char *strn)
{
  char *strptr = strn, *resetptr, origch;
  const char *outptr = strn, *strend = strn + strlen(strn);
  uchar color, bgcol;
  bool valid, isFlash, notused;
  const bool showCol = getShowColorVal(), interpCol = getInterpColorVal();


 // run through the string

  while (strptr < strend)
  {
    if (*strptr == '&')
    {
      strptr++;

      if (toupper(*strptr) == 'N')
      {
        if (showCol)
          resetptr = strptr + 1;
        else
          resetptr = strptr - 1;

        origch = *resetptr;

        strptr++;

        *resetptr = '\0';

        _outtext(outptr);

        *resetptr = origch;
        outptr = strptr;

        if (interpCol)
        {
          _settextcolor(7);
          _setbkcolor(0);
        }
      }
      else

     // &+X, set foreground

      if (*strptr == '+')
      {
        strptr++;

        color = getTextColor(*strptr, false, &valid, &isFlash);

        if (valid)
        {
          strptr++;

          if (showCol)
            resetptr = strptr;
          else
            resetptr = strptr - 3;

          origch = *resetptr;

          *resetptr = '\0';

          _outtext(outptr);

          *resetptr = origch;
          outptr = strptr;

          if (interpCol)
          {
            _settextcolor(color);
          }
        }
      }
      else

     // &-X, set background if lowercase or foreground+blink if uppercase

      if (*strptr == '-')
      {
        strptr++;

        color = getTextColor(*strptr, true, &valid, &isFlash);

        if (valid)
        {
          strptr++;

          if (showCol)
            resetptr = strptr;
          else
            resetptr = strptr - 3;

          origch = *resetptr;

          *resetptr = '\0';

          _outtext(outptr);

          *resetptr = origch;
          outptr = strptr;

          if (interpCol)
          {
            if (isFlash)
              _settextcolor(color);
            else
              _setbkcolor(color);
          }
        }
      }
      else

     // &=XY, set background to X and foreground to Y

      if (*strptr == '=')
      {
        strptr++;

       // background letter comes first

        bgcol = getTextColor(*strptr, true, &valid, &isFlash);

        if (valid)
        {
         // then foreground letter - keep BG isFlash value by passing in unused var

          strptr++;

          color = getTextColor(*strptr, false, &valid, &notused);

          if (valid)
          {
            strptr++;

            if (showCol)
              resetptr = strptr;
            else
              resetptr = strptr - 4;

            origch = *resetptr;

            *resetptr = '\0';

            _outtext(outptr);

            *resetptr = origch;
            outptr = strptr;

           // set foreground value first, then background, so that any flash value is kept

            if (interpCol)
            {
              _settextcolor(color);

              if (isFlash)
                _settextcolor(bgcol);
              else
                _setbkcolor(bgcol);
            }
          }
        }
      }
    }
    else
    {
      strptr++;
    }
  }

  _outtext(outptr);
}


//
// getTextColor : called by displayColorString - returns color number if valid
//
//   chCode : color code
//     isBG : if true, set background, if false, foreground (if BG and chCode is uppercase, set
//            text to flashing)
//  isValid : set to true if code is valid
//  isFlash : set to true if isBG is true and color code is uppercase (>= 8)
//

uchar getTextColor(const char chCode, const bool isBG, bool *isValid, bool *isFlash)
{
  uchar color;

  switch (chCode)
  {
    case 'l' : color = 0;  break;  // black
    case 'L' : color = 8;  break;  // dark grey
    case 'b' : color = 1;  break;  // dark blue
    case 'B' : color = 9;  break;  // purplish blue
    case 'g' : color = 2;  break;  // dark green
    case 'G' : color = 10; break;  // bright green
    case 'c' : color = 3;  break;  // light blue (cyan)
    case 'C' : color = 11; break;  // bright blue
    case 'r' : color = 4;  break;  // dark red
    case 'R' : color = 12; break;  // bright red
    case 'm' : color = 5;  break;  // light purple
    case 'M' : color = 13; break;  // bright purple (pink)
    case 'y' : color = 6;  break;  // brown
    case 'Y' : color = 14; break;  // yellow
    case 'w' : color = 7;  break;  // grey
    case 'W' : color = 15; break;  // white

    default :
      *isValid = false;
      return 0;
  }

  *isValid = true;

 // if color >= 8, then char is uppercase which means flash

  if (isBG && (color >= 8))
  {
    *isFlash = true;

    return color + 8;
  }

  *isFlash = false;

  return color;
}


//
// displayStringNodes : outputs a list of stringNodes to the screen, using
//                      displayColorString to display in color
//
//   *rootNode : root node of stringNodes
//

void displayStringNodes(const stringNode *rootNode)
{
  while (rootNode)
  {
    displayColorString(rootNode->string);

    _settextcolor(7);  // as per Duris
    _setbkcolor(0);

    _outtext("\n");

    rootNode = rootNode->Next;
  }
}


//
// displayStringNodesPause : outputs a list of stringNodes to the screen, using
//                           displayColorString to display in color - checkPause()
//                           version
//
//                           returns true if user aborted in checkPause()
//
//   *rootNode : root node of stringNodes
//

bool displayStringNodesPause(const stringNode *rootNode, size_t &numbLines)
{
  while (rootNode)
  {
    if (checkPause(rootNode->string, numbLines))
      return true;

    _settextcolor(7);  // as per Duris
    _setbkcolor(0);

    _outtext("\n");

    rootNode = rootNode->Next;
  }

  return false;
}


//
// displayPrompt : Displays the prompt that will greet you so often in
//                 durisEdit.
//

void displayPrompt(void)
{
  displayColorString(g_promptString);
}


//
// remColorCodes : Removes the Duris color codes from a string, both altering
//                 the original and returning the address.
//
//   *strn : string to alter
//

char *remColorCodes(char *strn)
{
  char *strptr = strn, *writeptr = strn;

 // run through the string

  while (*strptr)
  {
    if (*strptr == '&')
    {
      strptr++;

      if (toupper(*strptr) == 'N') 
      {
        strptr++;
        continue;
      }
      else if (((*strptr == '+') || (*strptr == '-')) && validANSIletter(*(strptr + 1)))
      {
        strptr += 2;
        continue;
      }
      else if ((*strptr == '=') && validANSIletter(*(strptr + 1)) && validANSIletter(*(strptr + 2)))
      {
        strptr += 3;
        continue;
      }
      else
      {
        strptr--;  // grab the & below
      }
    }

    *writeptr = *strptr;

    writeptr++;
    strptr++;
  }

  *writeptr = '\0';

  return strn;
}


//
// createMenuHeader
//

void createMenuHeader(char *strn, const char entityType, const char *entityName, const uint entityNumb, 
                      const char *entitySubName, bool asBitVect)
{
  char subbuff[2048];


 // 'editing XXX for room' vs 'editing room'

  if (entitySubName)
    sprintf(subbuff, "%s %sfor ", entitySubName, asBitVect ? "flags " : "");
  else
    subbuff[0] = '\0';


 // generate and display header

  sprintf(strn, "&=lgEditing %s%s #&+c%u&+w, &+L\"&n%s&+L\"\n\n",
          subbuff, getEntityTypeStrn(entityType), entityNumb, entityName);
}



//
// createShortMenuHeader : no mention of entityName
//

void createShortMenuHeader(char *strn, const char entityType, const uint entityNumb, 
                           const char *entitySubName, bool asBitVect)
{
  char subbuff[2048];


 // 'editing XXX for room' vs 'editing room'

  if (entitySubName)
    sprintf(subbuff, "%s %sfor ", entitySubName, asBitVect ? "flags " : "");
  else
    subbuff[0] = '\0';


 // generate and display header

  sprintf(strn, "&=lgEditing %s%s #&+c%u\n\n",
          subbuff, getEntityTypeStrn(entityType), entityNumb);
}


//
// createMenuHeaderBasic
//

void createMenuHeaderBasic(char *strn, const char *preName, const char *entityName)
{
  sprintf(strn, "&=lgEditing %s &+L\"&n%s&+L\"\n\n", preName, entityName);
}


//
// preDisplayLongMenuHeader : redundant long string massaging - maxLen should be size of newEntityName
//                            including null and must be >= 6
//
//                            returns position in string at which to start deleting characters, 0 if
//                            string is effectively empty
//

size_t preDisplayLongMenuHeader(char *newEntityName, const char *entityName, const size_t maxLen)
{
  strncpy(newEntityName, entityName, maxLen - 6);
  newEntityName[maxLen - 6] = '\0';

  strcat(newEntityName, "&n ..");

  const size_t newlen = strlen(newEntityName);

  if (newlen > 6)
    return newlen - 6;
  else
    return 0;
}


//
// displayMenuHeaderBasic : basic in that you set most of it up yourself
//

void displayMenuHeaderBasic(const char *preName, const char *entityName)
{
  char buff[2048];


  clrscr(7, 0);
  _settextposition(1, 1);

  createMenuHeaderBasic(buff, preName, entityName);

 // adjust header length as necessary for screen width

  if (truestrlen(buff) >= getScreenWidth())
  {
    char newEntityName[512];

    size_t delPos = preDisplayLongMenuHeader(newEntityName, entityName, 512);

    do
    {
     // got rid of entire name, just put in pre and no name and call it good

      if (delPos <= 0)
      {
        sprintf(buff, "&=lgEditing %s ..\n\n", preName);

        break;
      }

      deleteChar(newEntityName, delPos);
      delPos--;

      createMenuHeaderBasic(buff, preName, newEntityName);
    } while (truestrlen(buff) >= getScreenWidth());
  }

  displayColorString(buff);
}


//
// displayMenuHeader
//

void displayMenuHeader(const char entityType, const char *entityName, const uint entityNumb, 
                       const char *entitySubName, bool asBitVect)
{
  char buff[2048];


  clrscr(7, 0);
  _settextposition(1, 1);

  createMenuHeader(buff, entityType, entityName, entityNumb, entitySubName, asBitVect);

 // adjust header length as necessary for screen width

  if (truestrlen(buff) >= getScreenWidth())
  {
    char newEntityName[512];

    size_t delPos = preDisplayLongMenuHeader(newEntityName, entityName, 512);

    do
    {
     // got rid of entire name, just put in # and no name and call it good

      if (delPos <= 0)
      {
        createShortMenuHeader(buff, entityType, entityNumb, entitySubName, asBitVect);

        break;
      }

      deleteChar(newEntityName, delPos);
      delPos--;

      createMenuHeader(buff, entityType, newEntityName, entityNumb, entitySubName, asBitVect);
    } while (truestrlen(buff) >= getScreenWidth());
  }

  displayColorString(buff);
}


//
// displaySimpleMenuHeader
//

void displaySimpleMenuHeader(const char *editWhat)
{
  char buff[2048];


  clrscr(7, 0);
  _settextposition(1, 1);

  sprintf(buff, "&=lgEditing %s\n\n", editWhat);

 // adjust header length as necessary for screen width

  if (truestrlen(buff) >= getScreenWidth())
  {
    char newEditWhat[512];

    size_t delPos = preDisplayLongMenuHeader(newEditWhat, editWhat, 512);

    do
    {
      deleteChar(newEditWhat, delPos);
      delPos--;

      sprintf(buff, "&=lgEditing %s\n\n", newEditWhat);
    } while (truestrlen(buff) >= getScreenWidth());
  }

  displayColorString(buff);
}


//
// displayAnyKeyPrompt : display a prompt on the current line
//

void displayAnyKeyPrompt(const char *prompt)
{
  const struct rccoord coords = _gettextposition();

  clrline(coords.row);
  _settextposition(coords.row, 1);

  displayAnyKeyPromptNoClr(prompt);
}


//
// displayAnyKeyPromptNoClr : display a prompt, don't clear line
//

void displayAnyKeyPromptNoClr(const char *prompt)
{
  displayColorString(prompt);

  getkey();
}


//
// displayYesNoPrompt - '&+' somewhere in prompt means use color in y/n portion
//

promptType displayYesNoPrompt(const char *prompt, const promptType defaultChoice, const bool blnClearLine)
{
  const char *keySelectStrn;
  char allPrompt[1024];
  promptType userChoice;
  const bool blnUseColor = (strstr(prompt, "&+") != NULL);


  switch (defaultChoice)
  {
    case promptNone :
      keySelectStrn = "y/n";
      break;

    case promptYes :
      keySelectStrn = "Y/n";
      break;

    case promptNo :
      keySelectStrn = "y/N";
      break;
  }

  if (blnUseColor)
    sprintf(allPrompt, "%s (&+C%s&+c)? &n", prompt, keySelectStrn);
  else
    sprintf(allPrompt, "%s (%s)? ", prompt, keySelectStrn);

  if (blnClearLine)
  {
    const struct rccoord coords = _gettextposition();

    clrline(coords.row);
    _settextposition(coords.row, 1);
  }

  if (blnUseColor)
    displayColorString(allPrompt);
  else
    _outtext(allPrompt);

  while (true)
  {
    const usint ch = toupper(getkey());

    if (ch == 'Y')
    {
      userChoice = promptYes;
      break;
    }
    else 
    if (ch == 'N')
    {
      userChoice = promptNo;
      break;
    }
    else 
    if ((ch == K_Enter) && (defaultChoice != promptNone))
    {
      userChoice = defaultChoice;
      break;
    }
  }

  switch (userChoice)
  {
    case promptYes :
      _outtext("yes\n\n");
      break;

    case promptNo :
      _outtext("no\n\n");
      break;
  }

  return userChoice;
}
