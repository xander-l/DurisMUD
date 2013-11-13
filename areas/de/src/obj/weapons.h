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


// WEAPONS.H - constants for weapon types

#ifndef _WEAPONS_H_

// weapon damage types

// obsolete

/*#define WEAPON_DAM_LOWEST     2
#define WEAPON_DAM_WHIP       2
#define WEAPON_DAM_SLASH      3
#define WEAPON_DAM_CRUSH      6
#define WEAPON_DAM_POUND      7
#define WEAPON_DAM_PIERCE    11
#define WEAPON_DAM_HIGHEST   11

#define NUMB_WEAPON_DAM_TYPES  5*/

// weapon types

#define WEAPON_LOWEST       1
#define WEAPON_AXE          1  // axes - slashing
#define WEAPON_DAGGER       2  // daggers, knives - piercing, slashing (with -)
#define WEAPON_FLAIL        3  // flails - whip
#define WEAPON_HAMMER       4  // hammers - bludgeon
#define WEAPON_LONGSWORD    5  // long swords - slashing/piercing (with -)
#define WEAPON_MACE         6  // mace - bludgeon
#define WEAPON_SPIKED_MACE  7  // spiked mace - bludgeon with pierce thrown in
#define WEAPON_POLEARM      8  // polearm - halberds, pikes, etc - piercing
#define WEAPON_SHORTSWORD   9  // short swords - slashing/piercing
#define WEAPON_CLUB        10  // clubs - bludgeon
#define WEAPON_SPIKED_CLUB 11  // spiked clubs - bludgeon and piercing
#define WEAPON_STAFF       12  // staff - like club but longer, maybe 2-handed -
                               //   bludgeon
#define WEAPON_2HANDSWORD  13  // two-handed swords - slash
#define WEAPON_WHIP        14  // whips - whip
#define WEAPON_SPEAR       15  // spear - pierce
#define WEAPON_LANCE       16  // lance - special handling probably
#define WEAPON_SICKLE      17  // sickle - slash/pierce?
#define WEAPON_TRIDENT     18  // trident/forks/rakes - slash
#define WEAPON_HORN        19  // curved piercer - piercing
#define WEAPON_NUMCHUCKS   20  // numchucks - bludgeon
#define WEAPON_HIGHEST     20

#define NUMB_WEAPON_TYPES  20

#define _WEAPONS_H_
#endif
