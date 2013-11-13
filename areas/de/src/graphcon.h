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


// GRAPHCON.H - text graphics library includes, different for each platform

//
//  About graph.h - system-specific text I/O functions supplied with Watcom
//                  and used in various functions for cursor pos/color
//
//  _outtext(char *strn) - displays text in strn using colors set with
//                         _settextcolor and _setbkcolor
//
//  _settextcolor(char fg) - from 0-15 (standard IBM PC colors), sets color of
//                           all subsequent text output with _outtext.
//
//  _setbkcolor(char bg) - from 0-7 (standard IBM PC colors), sets background
//                         color of all subsequent text output with _outtext.
//
//  _gettextcolor - returns current foreground color being used
//
//  _gettextposition - returns a structure rccoord that contains two members,
//                     row and col, each short ints.  row and col are set to
//                     the current cursor position onscreen.  (upper-left
//                     corner - row = 1, col = 1)
//
//  _settextposition(struct rccoord coords) - uses row and col members in
//                                            coords to set cursor pos.
//                                            subsequent text output with
//                                            _outtext is drawn here.
//

#ifndef _GRAPHCON_H_

#if (!defined(_WIN32) && !defined(__UNIX__))
#  error durisEdit can only be compiled under Win32 or UNIX
#endif

#ifdef _WIN32

typedef short SHORT;  // if MS changes this to something else, feel free to fix it

struct rccoord
{
  SHORT row;
  SHORT col;
};
#endif

#ifdef __UNIX__
struct rccoord
{
  int row;
  int col;
};
#endif

#define _GRAPHCON_H_
#endif
