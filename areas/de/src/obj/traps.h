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


#ifndef _TRAPS_H_

#define TRAP_DAM_LOWEST         0
#define TRAP_DAM_SLEEP          0
#define TRAP_DAM_TELEPORT       1
#define TRAP_DAM_FIRE           2
#define TRAP_DAM_COLD           3
#define TRAP_DAM_ACID           4
#define TRAP_DAM_ENERGY         5
#define TRAP_DAM_BLUNT          6
#define TRAP_DAM_PIERCE         7
#define TRAP_DAM_SLASH          8
#define TRAP_DAM_DISPEL         9
#define TRAP_DAM_GATE           10
#define TRAP_DAM_SUMMON         11
#define TRAP_DAM_WITHER         12
#define TRAP_DAM_HARM           13
#define TRAP_DAM_POISON         14
#define TRAP_DAM_PARALYSIS      15
#define TRAP_DAM_STUN           16
#define TRAP_DAM_HIGHEST        16

#define NUMB_TRAP_DAM_TYPES     17

#define NUMB_TRAP_EFF_FLAGS  12

#define TRAP_EFF_MOVE         1 /* trigger on movement */
#define TRAP_EFF_OBJECT       2 /* trigger on get or put */
#define TRAP_EFF_ROOM         4 /* affect all in room */
#define TRAP_EFF_NORTH        8 /* movement in this direction */
#define TRAP_EFF_SOUTH       16
#define TRAP_EFF_WEST        32
#define TRAP_EFF_EAST        64
#define TRAP_EFF_UP         128
#define TRAP_EFF_DOWN       256
#define TRAP_EFF_OPEN       512 /* trigger on open */
#define TRAP_EFF_MULTI     1024 
#define TRAP_EFF_GLYPH     2048 

#define _TRAPS_H_
#endif
