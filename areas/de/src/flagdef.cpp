//
//  File: flagdef.cpp    originally part of durisEdit
//
//  Usage: defines tables used to give names and positions of
//         flags in bitvectors
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

#include "flagdef.h"

#include "room/room.h"
#include "obj/object.h"
#include "obj/shields.h"
#include "obj/traps.h"
#include "zone/zone.h"

//
// object trap flags
//

flagDef g_objTrapFlagDef[] =
{
  { "MOVE", "Triggers on movement", 1, 0 },
  { "OBJECT", "Triggers on get/put", 1, 0 },
  { "ROOM", "Affects all in room", 1, 0 },
  { "NORTH", "Affects those who leave room north", 1, 0 },
  { "SOUTH", "Affects those who leave room south", 1, 0 },
  { "WEST", "Affects those who leave room west", 1, 0 },
  { "EAST", "Affects those who leave room east", 1, 0 },
  { "UP", "Affects those who leave room up", 1, 0 },
  { "DOWN", "Affects those who leave room down", 1, 0 },
  { "OPEN", "Triggers on being opened", 1, 0 },
  { "MULTI", "Has unlimited charges (same as -1)", 1, 0 },
  { "GLYPH", "Glyph (doesn't prevent movement)", 1, 0 },
  { 0 }
};


//
// misc armor flags
//

flagDef g_armorMiscFlagDef[] =
{
  { "SPIKED", "Armor is spiked", 1, 0 },
  { 0 }
};


//
// object container flags
//

flagDef g_contFlagDef[] =
{
  { "CLOSEABLE", "Closeable", 1, 0 },
  { "HARDPICK", "Hardpick", 1, 0 },
  { "CLOSED", "Closed", 1, 0 },
  { "LOCKED", "Locked", 1, 0 },
  { "PICKPROOF", "Cannot be picked", 1, 0 },
  { 0 }
};


//
// totem sphere flags
//

flagDef g_totemSphereFlagDef[] =
{
  { "LESS_ANIM", "Lesser animal", 1, 0 },
  { "GREA_ANIM", "Greater animal", 1, 0 },
  { "LESS_ELEM", "Lesser elemental", 1, 0 },
  { "GREA_ELEM", "Greater elemental", 1, 0 },
  { "LESS_SPIR", "Lesser spirit", 1, 0 },
  { "GREA_SPIR", "Greater spirit", 1, 0 },
  { 0 }
};


//
// misc shield flags
//

flagDef g_shieldMiscFlagDef[] =
{
  { "SPIKED", "Shield is spiked", 1, 0 },
  { 0 }
};


//
// zone misc flags
//

flagDef g_zoneMiscFlagDef[] =
{
  { "SILENT", "Silent", 1, 0 },
  { "SAFE", "Safe", 1, 0 },
  { "HOMETOWN", "Hometown", 1, 0 },
  { "NEW_RESET", "New reset", 0, 0 },
  { "MAP", "Map", 1, 0 },
  { "CLOSED", "Closed", 1, 0 },
  { 0 }
};
