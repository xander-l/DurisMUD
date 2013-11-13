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


// SHIELDS.H - info for items of type ITEM_SHIELD

#ifndef _SHIELDS_H_

// type (val0)

#define SHIELDTYPE_LOWEST   1
#define SHIELDTYPE_STRAPARM 1  // strapped to the arm - bucklers, others
#define SHIELDTYPE_HANDHELD 2  // held by hand
#define SHIELDTYPE_HIGHEST  2

// shape (val1)

#define SHIELDSHAPE_LOWEST   1
#define SHIELDSHAPE_CIRCULAR 1  // perfect circle
#define SHIELDSHAPE_SQUARE   2  // square..
#define SHIELDSHAPE_RECTVERT 3  // a rectangle aligned vertically
#define SHIELDSHAPE_RECTHORZ 4  // horizontally
#define SHIELDSHAPE_OVALVERT 5  // vertical 'oval'
#define SHIELDSHAPE_OVALHORZ 6  // horizontal 'oval'
#define SHIELDSHAPE_TRIBIGUP 7  // triangle - wide side on top
#define SHIELDSHAPE_TRISMLUP 8  // triangle - narrow point on top

// dunno what they're called, but the type of shield that is square on
// top and rounded on the bottom should be added

#define SHIELDSHAPE_HIGHEST  8

// size (val2) - used to determine how well it can block (and how well it
//               encumbers your weapon use, perhaps)

#define SHIELDSIZE_LOWEST   1
#define SHIELDSIZE_TINY     1  // really small suckers
#define SHIELDSIZE_SMALL    2  // bucklers, small shields
#define SHIELDSIZE_MEDIUM   3  // normal shields
#define SHIELDSIZE_LARGE    4  // big shields
#define SHIELDSIZE_HUGE     5  // huge shields (might not need this)
#define SHIELDSIZE_HIGHEST  5

#define _SHIELDS_H_
#endif
