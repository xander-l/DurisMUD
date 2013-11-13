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


// EXIT.H - header for EXIT.CPP

#ifndef _EXIT_H_

#include "misc/strnnode.h"

// character values as read from the world files - only used in room/readwld.cpp,
// probably

#define EXIT_FIRST_CH      '0'
#define EXIT_NORTH_CH      '0'
#define EXIT_EAST_CH       '1'
#define EXIT_SOUTH_CH      '2'
#define EXIT_WEST_CH       '3'
#define EXIT_UP_CH         '4'
#define EXIT_DOWN_CH       '5'
#define EXIT_NORTHWEST_CH  '6'
#define EXIT_SOUTHWEST_CH  '7'
#define EXIT_NORTHEAST_CH  '8'
#define EXIT_SOUTHEAST_CH  '9'
#define EXIT_LAST_CH       '9'

#define NO_EXIT            -1  // returned by various exit-related functions
                               // as general "no exit/no match found" indicator
#define USER_CHOICE        -2  // used to signify that the user should be
                               // be prompted for exit direction

// values of exits as referenced in room exits array and in .wld files

#define EXIT_LOWEST         0
#define NORTH               0
#define EAST                1
#define SOUTH               2
#define WEST                3
#define UP                  4
#define DOWN                5
#define NORTHWEST           6
#define SOUTHWEST           7
#define NORTHEAST           8
#define SOUTHEAST           9
#define EXIT_HIGHEST        9

#define NUMB_EXITS         10


#define MAX_EXITKEY_LEN  (usint)256  // used in room/readwld.cpp

typedef struct _roomExit
{
  stringNode *exitDescHead;
                    // exit description

  stringNode *keywordListHead;
                    // instead of one string that contains all the keywords,
                    // this list contains all the keywords parsed, so that
                    // each node has a keyword

  int worldDoorType;  // exit "type" read from .wld
  int zoneDoorState;  // exit "state" read from .zon

  int keyNumb;     // key object numb required to open door

  int destRoom;   // destination room
} roomExit;

// values used for bitvector returned by getExitAvailFlags() and used to
// determine which exits a room already has occupied

#define EXIT_NORTH_FLAG          1
#define EXIT_SOUTH_FLAG          2
#define EXIT_WEST_FLAG           4
#define EXIT_EAST_FLAG           8
#define EXIT_UP_FLAG            16
#define EXIT_DOWN_FLAG          32
#define EXIT_NORTHWEST_FLAG     64
#define EXIT_SOUTHWEST_FLAG    128
#define EXIT_NORTHEAST_FLAG    256
#define EXIT_SOUTHEAST_FLAG    512

#define EXIT_ALL_EXITS_FLAG (EXIT_NORTH_FLAG | EXIT_SOUTH_FLAG | EXIT_WEST_FLAG | EXIT_EAST_FLAG | EXIT_UP_FLAG | EXIT_DOWN_FLAG | EXIT_NORTHWEST_FLAG | EXIT_SOUTHWEST_FLAG | EXIT_NORTHEAST_FLAG | EXIT_SOUTHEAST_FLAG)


#define _EXIT_H_
#endif
