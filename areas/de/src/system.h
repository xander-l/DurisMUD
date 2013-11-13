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


// SYSTEM.H - system-dependent stuff

#ifndef _SYSTEM_H_


// name of "main" filename for default objs etc (no extension)

#define MAIN_DEFAULT_NAME  "de"

#define DEF_WLD_EXT        ".dwd"  // extension of def world
#define DEF_OBJ_EXT        ".dob"  // ext of def obj
#define DEF_MOB_EXT        ".dmb"  // ext of def mob
#define DEF_EXIT_EXT       ".dex"  // ext of def exit
#define DEF_EDESC_EXT      ".ded"  // ext of def extra desc
#define DEF_QUEST_EXT      ".dqs"  // ext of def quest
#define DEF_SHOP_EXT       ".dsp"  // ext of def shop

#define MAIN_SETFILE_NAME  "de.set"


// name of temp files

#define TMPFILE_NAME "detmp.txt"


// screen width in columns

#define DEFAULT_SCREEN_WIDTH   80
#define MINIMUM_SCREEN_WIDTH   40


// screen height in rows

#define DEFAULT_SCREEN_HEIGHT  25
#define MINIMUM_SCREEN_HEIGHT  15


// help file

#define DE_MAINHELP    "dehelp.hlp"
#define DE_SHORTHELP   "deshort.hlp"

#define _SYSTEM_H_
#endif
