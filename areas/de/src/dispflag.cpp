//
//  File: dispflag.cpp   originally part of durisEdit
//
//  Usage: functions related to displaying a menu of flags with
//         the flags represented by a 32-bit bitvector or, alternately,
//         with the 'defVal' member of flagDef structs specifying
//         the value of the selection (for enumerated lists)
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

#include "graphcon.h"

#include "fh.h"
#include "dispflag.h"
#include "misc/menu.h"
#include "types.h"
#include "flagdef.h"

//
// displayFlagMenu : displays a menu of flags onscreen, with corresponding
//                   keystrokes, based on information passed in flagArr -
//                   fills colXpos, colTopFlag, and numbCols arrays
//
//         val : current value of bitvector being edited
//     flagArr : array of flagDef records, stores flag info for bitvector being
//               displayed
//
//  entityType : type of entity for which flags are being edited
//  entityName : name of specific entity
//  entityNumb : number of entity
//    flagName : name of flag, used but once
//
//     colXpos : variable-sized array, columns start here
//  colTopFlag : variable-sized array, dynamically created - is set
//               to which flag is at the top of each column (first flag = 0)
//
//    numbCols : number of columns to format flags in - if 0, minimum number
//               is chosen
//
//   asBitVect : if false, editing list of values rather than bitvector
//

void displayFlagMenu(const uint val, const flagDef *flagArr, const char entityType, const char *entityName,
                     const uint entityNumb, const char *flagName, sint *colXpos, uint *colTopFlag, 
                     uint *numbCols, const bool asBitVect)
{
  uint numbFlags, i, j, k;
  uint numbFlagsinCol[MAX_FLAG_COLUMNS];
  struct rccoord coord;
  char buff[1024], ch;
  sint lowestLine = 0;
  sint ypos;


  numbFlags = 0;

  while (flagArr[numbFlags].flagShort)
    numbFlags++;
   
  if ((numbFlags == 0) || (numbCols == NULL))
  {
    return;  // DOH!
  }

 // display header

  displayMenuHeader(entityType, entityName, entityNumb, flagName, asBitVect);


 // if numb cols is 0, figure out the best number of columns based on user-defined screen height -
 // 9 is amount required by header and footer

  const uint intRowsPerScr = getScreenHeight() - 9;

  if (*numbCols == 0)
  {
    if (numbFlags <= intRowsPerScr) 
    {
      *numbCols = 1;
    }
    else
    {
      *numbCols = numbFlags / intRowsPerScr;

      if ((numbFlags % intRowsPerScr) != 0) 
        (*numbCols)++;
    }
  }

 // make sure numb columns isn't too big

  if (*numbCols > ((numbFlags / intRowsPerScr) + 1))
    *numbCols = (numbFlags / intRowsPerScr) + 1;

 // make sure numb columns isn't too small

  if ((numbFlags / *numbCols) > intRowsPerScr)
    *numbCols = (numbFlags / intRowsPerScr) + 1;

 // set numbFlagsinCol

  if (*numbCols > MAX_FLAG_COLUMNS)
  {
    _outtext("displayFlagMenu(): too many flags would require too many columns\n\n");
    return;
  }

  for (i = 0; i < *numbCols; i++)
  {
    numbFlagsinCol[i] = intRowsPerScr;
  }

  numbFlagsinCol[(*numbCols) - 1] = numbFlags % intRowsPerScr;

  colTopFlag[0] = 0;

  for (i = 1; i < *numbCols; i++)
  {
    colTopFlag[i] = colTopFlag[i - 1] + numbFlagsinCol[i - 1];
  }

 // let's dump out the buffer, shall we?

  j = (getScreenWidthVal() - 2) / *numbCols;

  for (i = 1; i < *numbCols; i++)
  {
    colXpos[i] = (sint)(j * i) + 1;  // assume 32767 columns are enough
  }

  colXpos[0] = 1;

  for (j = 0; j < *numbCols; j++)
  {
    if (j != (*numbCols - 1)) 
      k = colTopFlag[j + 1];
    else 
      k = numbFlags;

    for (i = colTopFlag[j]; i < k; i++)
    {
      ypos = HEADER_OFFSETY + (sint)(i - colTopFlag[j]);  // assume never so craaaazzzzzzy

      _settextposition(ypos, colXpos[j]);

      if (ypos > lowestLine) 
        lowestLine = ypos;

      ch = 'A' + (char)i;
      if (ch > 'Z') ch -= 42;

      sprintf(buff, "   %s%c&+L. &+w%s",
              flagArr[i].editable ? "&+Y" : "&+L",
              ch, flagArr[i].flagLong);

      displayColorString(buff);
    }
  }

 // bottom of menu stuff

  sprintf(buff,
"   &+Y!&+L. &+wEnter %s %sdirectly\n",
         flagName,
         asBitVect ? "flags " : "");

  _settextposition(lowestLine + 2, 1);

  displayColorString(buff);

  displayMenuFooter();

  coord = _gettextposition();

 // show flag settings

  updateFlags(val, numbFlags, *numbCols, colXpos, colTopFlag, HEADER_OFFSETY,
              false, asBitVect, flagArr);

  _settextposition(coord.row, coord.col);
}


//
// updateSingleFlag : Updates single flag status indicator on-screen
//
//       flagVal : value of bitvector
//      flagNumb : number of flag to update (0 = first flag)
//     numbFlags : number of valid flags in bitvector
//
//      numbCols : number of columns displayed onscreen (> 0)
//       colXpos : x-position of each column (array with numbCols elems)
//    colTopFlag : top flag of each column (0 = first flag, array of numbCols
//                 elems)
//  headerOffset : amount to offset by rows due to header
//
//     asBitVect : if true, interpret list as bitvector, otherwise as
//                 enumerated
//       flagArr : bitvector/enum list info
//

void updateSingleFlag(const uint flagVal, const uchar flagNumb, const uint numbFlags, const uint numbCols,
                      const sint *colXpos, const uint *colTopFlag, const sint headerOffset, 
                      const bool asBitVect, const flagDef *flagArr)
{
  struct rccoord xy;
  sint ypos;
  uint val;
  int j;


 // for non-bitv, flagnumb is actual flag, flagval is value of flag -
 // if flagval doesn't equal flagnumb's defval, it is cleared

  if (flagNumb >= numbFlags) 
    return;

  xy = _gettextposition();

 // figure out which column this flag is in

  for (j = numbCols - 1; j >= 0; j--)
  {
    if (flagNumb >= colTopFlag[j]) 
      break;
  }

  if (j < 0) 
    return;  // error

  if (asBitVect)
    val = flagVal & (1 << flagNumb);
  else
    val = (flagArr[flagNumb].defVal == flagVal);

  ypos = headerOffset + (sint)(flagNumb - colTopFlag[j]);

  _settextposition(ypos, colXpos[j] + 1);

  if (val) 
    displayColorString(FLAG_ACTIVE_STR);
  else 
    _outtext(" ");

  _settextposition(xy.row, xy.col);
}


//
// updateFlags : Updates flag status indicators on-screen
//
//       flagVal : value of flag being displayed
//     numbFlags : number of valid flags in bitvector
//
//      numbCols : number of columns displayed onscreen (> 0)
//       colXpos : x-position of each column (array with numbCols elems)
//    colTopFlag : top flag of each column (0 = first flag, array of numbCols
//                 elems)
//  headerOffset : amount to offset by rows due to header
// displayNonset : if TRUE, writes spaces over non-set flag locations
//
//     asBitVect : if true, interpret list as bitvector, otherwise as
//                 enumerated
//       flagArr : bitvector/enum list info
//

void updateFlags(const uint flagVal, const uint numbFlags, const uint numbCols, const sint *colXpos,
                 const uint *colTopFlag, const sint headerOffset, const uchar displayNonset, 
                 const bool asBitVect, const flagDef *flagArr)
{
  uint i, k, val;
  uint j;
  sint ypos;//, drewone = FALSE;


  for (j = 0; j < numbCols; j++)
  {
    if (j != (numbCols - 1)) 
      k = colTopFlag[j + 1];
    else 
      k = numbFlags;

    for (i = colTopFlag[j]; i < k; i++)
    {
      if (asBitVect)
        val = flagVal & (1 << i);
      else
        val = ((flagArr[i].defVal) == flagVal);

      if (!val && !displayNonset) continue;

      ypos = headerOffset + (sint)(i - colTopFlag[j]);  // sane is sane

      _settextposition(ypos, colXpos[j] + 1);

      if (val) 
        displayColorString(FLAG_ACTIVE_STR);
      else 
        _outtext(" ");
    }
  }
}
