//
//  File: unixgraph.cpp  originally part of durisEdit
//
//  Usage: ncurses-specific duplicates of Watcom text graphics
//         functions - should work under any Unix
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


#ifndef __UNIX__
#  error UNIXGRAPH.CPP should only be used under Unix
#endif

#include <ncurses.h>
#include "graphcon.h"
#include "types.h"
#include "fh.h"

static uchar fg, bg;


void exitUnixCurses(void)
{
  _outtext("\nPress a key to exit...");
  getkey();

  endwin();
}


//
// initColorsBG
//

void initColorsBG(const uchar bg, const uchar cursesBG, const bool initZeroFG)
{
  uchar pairnumb = bg * 8;


 // color pair 0 is hardwired to white on black..  lame.  so, for ease 
 // of implementation swap foreground colors 7 and 0 so that foreground 7 
 // is black and 0 is white (for BG, 0 is black and 7 is white)

  if (initZeroFG)
    init_pair(pairnumb, COLOR_WHITE, cursesBG);

  pairnumb++;

  init_pair(pairnumb, COLOR_BLUE, cursesBG);
  pairnumb++;

  init_pair(pairnumb, COLOR_GREEN, cursesBG);
  pairnumb++;

  init_pair(pairnumb, COLOR_CYAN, cursesBG);
  pairnumb++;

  init_pair(pairnumb, COLOR_RED, cursesBG);
  pairnumb++;

  init_pair(pairnumb, COLOR_MAGENTA, cursesBG);
  pairnumb++;

  init_pair(pairnumb, COLOR_YELLOW, cursesBG);
  pairnumb++;

  init_pair(pairnumb, COLOR_BLACK, cursesBG);
}


//
// initUnixCursesScreen
//

void initUnixCursesScreen(void)
{
  initscr();
  start_color();


 // pair 0 cannot be initialized (thus the third parameter is 'false' for the first group)

  initColorsBG(0, COLOR_BLACK, false);
  initColorsBG(1, COLOR_BLUE, true);
  initColorsBG(2, COLOR_GREEN, true);
  initColorsBG(3, COLOR_CYAN, true);
  initColorsBG(4, COLOR_RED, true);
  initColorsBG(5, COLOR_MAGENTA, true);
  initColorsBG(6, COLOR_YELLOW, true);
  initColorsBG(7, COLOR_WHITE, true);

  _settextcolor(7);
  _setbkcolor(0);

  cbreak();
  noecho();

  nonl();
  intrflush(stdscr, FALSE);
  keypad(stdscr, TRUE);
  scrollok(stdscr, TRUE);

  atexit(exitUnixCurses);
}

// fg, bg, ch ignored

void clrline(const sint line, const uchar fg, const uchar bg, const uchar ch)
{
  if (line < 1) 
    return;

  move(line - 1, 0);
  clrtoeol();
  refresh();
}

void clrline(const sint line, const uchar fg, const uchar bg)
{
  clrline(line, fg, bg, ' ');
}

void clrline(const sint line)
{
  clrline(line, 7, 0, ' ');
}

usint getkey(void)
{
  return getch();
}

void _settextcolor(const uchar col)
{
  uchar newcol = col;
  bool blink = false;

  if (col >= 24)
    return;

 // if col >= 16, then blink bit is set, with bg color of col - 16, fg remains unchanged

  if (col >= 16)
  {
    bg = col - 16;
    newcol = fg;

    blink = true;
  }

 // otherwise fg color is being changed

  else
  {
    fg = col;
  }

  if (newcol >= 8)
    newcol -= 8;

 // white and black foreground are inverted since color pair 0 is hardcoded to white on black

  if (newcol == 0) 
    attrset(COLOR_PAIR((bg * 8) + 7));
  else if (newcol == 7) 
    attrset(COLOR_PAIR(bg * 8));
  else
    attrset(COLOR_PAIR((bg * 8) + newcol));

  if (fg >= 8)
    attron(A_BOLD);
  else
    attroff(A_BOLD);

  if (blink)
    attron(A_BLINK);
  else
    attroff(A_BLINK);
}

void _setbkcolor(const uchar col)
{
  if (col >= 8)
    return;

  bg = col;

  uchar newfg = fg;
  if (newfg >= 8)
    newfg -= 8;

  if (newfg == 0) 
    attrset(COLOR_PAIR((bg * 8) + 7));
  else if (newfg == 7) 
    attrset(COLOR_PAIR(bg * 8));
  else
    attrset(COLOR_PAIR((bg * 8) + newfg));

  if (fg >= 8)
    attron(A_BOLD);
  else 
    attroff(A_BOLD);
}

uchar _gettextcolor(void)
{
  return fg;
}

uchar _getbkcolor(void)
{
  return bg;
}

void _settextposition(const int y, const int x)
{
  if ((y < 1) || (x < 1)) 
    return;

  move(y - 1, x - 1);
  refresh();
}

struct rccoord _gettextposition(void)
{
  struct rccoord pos;


  getyx(stdscr, pos.row, pos.col);

  pos.row++;
  pos.col++;

  return pos;
}

void _outtext(const char *strn)
{
  if (!(strn && strn[0])) 
    return;

  addstr((char *)strn);
  refresh();
}

void clrscr(const uchar fg, const uchar bg)
{
  clear();
  refresh();
}
