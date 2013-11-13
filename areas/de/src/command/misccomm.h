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


// MISCCOMM.H - command lists for miscellaneous commands

#ifndef _MISCCOMM_H_

#define LOOKUPCMD_ROOM     0
#define LOOKUPCMD_OBJECT   1
#define LOOKUPCMD_MOB      2

#define CREATECMD_ROOM     0
#define CREATECMD_OBJECT   1
#define CREATECMD_OBJHERE  2
#define CREATECMD_MOB      3
#define CREATECMD_MOBHERE  4
#define CREATECMD_EXIT     5
#define CREATECMD_ALIAS    6
#define CREATECMD_VARIABLE 7

#define LOADCMD_OBJECT     0
#define LOADCMD_MOB        1

#define EDITCMD_ROOM       0
#define EDITCMD_OBJECT     1
#define EDITCMD_MOB        2
#define EDITCMD_QUEST      3
#define EDITCMD_SHOP       4
#define EDITCMD_EXIT       5
#define EDITCMD_ZONE       6
#define EDITCMD_DEFROOM    7
#define EDITCMD_DEFOBJ     8
#define EDITCMD_DEFMOB     9
#define EDITCMD_DEFEXIT   10
#define EDITCMD_DESC      11
#define EDITCMD_DEFEDESC  12
#define EDITCMD_DEFQUEST  13
#define EDITCMD_DEFSHOP   14
#define EDITCMD_SECTOR    15

#define LISTCMD_ROOM       0
#define LISTCMD_OBJECT     1
#define LISTCMD_MOB        2
#define LISTCMD_QUEST      3
#define LISTCMD_SHOP       4
#define LISTCMD_OBJHERE    5
#define LISTCMD_MOBHERE    6
#define LISTCMD_LOADED     7
#define LISTCMD_LOOKUP     8
#define LISTCMD_EXIT       9
#define LISTCMD_ALIAS      10
#define LISTCMD_VARIABLE   11
#define LISTCMD_BOOLEAN    12
#define LISTCMD_TEMPLATE   13
#define LISTCMD_COMMANDS   14

#define CRTEDITCMD_ROOM    0
#define CRTEDITCMD_OBJECT  1
#define CRTEDITCMD_MOB     2
#define CRTEDITCMD_EXIT    3

#define DELETECMD_ROOM     0
#define DELETECMD_OBJECT   1
#define DELETECMD_MOB      2
#define DELETECMD_QUEST    3
#define DELETECMD_SHOP     4
#define DELETECMD_EXIT     5
#define DELETECMD_UNUSED   6
#define DELETECMD_DEFROOM  7
#define DELETECMD_DEFOBJECT 8
#define DELETECMD_DEFMOB   9
#define DELETECMD_DEFEXIT  10
#define DELETECMD_OBJHERE  11
#define DELETECMD_MOBHERE  12
#define DELETECMD_ALIAS    13
#define DELETECMD_VARIABLE 14
#define DELETECMD_DEFEDESC 15
#define DELETECMD_DEFQUEST 16
#define DELETECMD_DEFSHOP  17

#define PURGECMD_ALL      0
#define PURGECMD_ALLMOB   1
#define PURGECMD_ALLOBJ   2
#define PURGECMD_INV      3

#define STATCMD_ROOM       0
#define STATCMD_OBJECT     1
#define STATCMD_MOB        2
#define STATCMD_ZONE       3

#define SETLIMCMD_OBJECT   0
#define SETLIMCMD_MOB      1

#define CLONECMD_ROOM       0
#define CLONECMD_OBJECT     1
#define CLONECMD_MOB        2

#define COPYDESCCMD_ROOM       0
#define COPYDESCCMD_MOB        1

#define COPYEXTRACMD_ROOM      0
#define COPYEXTRACMD_OBJECT    1

#define COPYDEFCMD_ROOM      0
#define COPYDEFCMD_EXIT      1
#define COPYDEFCMD_OBJECT    2
#define COPYDEFCMD_MOB       3
#define COPYDEFCMD_EDESC     5
#define COPYDEFCMD_QUEST     6
#define COPYDEFCMD_SHOP      7

#define FLAGNAME_ROOMFLAG    0
#define FLAGNAME_OBJEXTRA    1
#define FLAGNAME_OBJWEAR     2
#define FLAGNAME_OBJAFF1     3
#define FLAGNAME_OBJAFF2     4
#define FLAGNAME_MOBACT      5
#define FLAGNAME_MOBAFF1     6
#define FLAGNAME_MOBAFF2     7

#define COPYCMD_DEFAULT      0
#define COPYCMD_DESC         1

#define _MISCCOMM_H_
#endif
