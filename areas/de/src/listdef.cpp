//
//  File: listdef.cpp    originally part of durisEdit
//
//  Usage: uses same struct as tables in flagdef.cpp, but these tables
//         are used for 'enumerated list' menu editing rather than bitvector
//         editing
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
#include "obj/objsize.h"
#include "obj/traps.h"
#include "obj/instrmnt.h"
#include "obj/objcraft.h"
#include "obj/liquids.h"
#include "obj/armor.h"
#include "obj/weapons.h"
#include "obj/shields.h"
#include "obj/missiles.h"
#include "zone/zone.h"
#include "defines.h"

/////////////////////////////////
// ROOMS
//

//
// sector types
//

flagDef g_roomSectList[] =
{
  { "SECT_INSIDE", "&+YInside", 1, SECT_INSIDE },
  { "SECT_CITY", "&+yCity", 1, SECT_CITY },
  { "SECT_FIELD", "&+yField", 1, SECT_FIELD },
  { "SECT_FOREST", "&+gForest", 1, SECT_FOREST },
  { "SECT_HILLS", "&+yHills", 1, SECT_HILLS },
  { "SECT_MOUNTN", "&+wMountain", 1, SECT_MOUNTAIN },
  { "SECT_WATERS", "&+cWater (swim)", 1, SECT_WATER_SWIM },
  { "SECT_WATERN", "&+cWater (noswim)", 1, SECT_WATER_NOSWIM },
  { "SECT_MIDAIR", "&+LMidair", 1, SECT_NO_GROUND },
  { "SECT_UNDERW", "&+bUnderwater", 1, SECT_UNDERWATER },
  { "SECT_UNDRWG", "&+bUnderwater (ground)", 1, SECT_UNDERWATER_GR },
  { "SECT_PLANEF", "&+rPlane of fire", 1, SECT_PLANE_OF_FIRE },
  { "SECT_OCEAN", "&+cOcean", 1, SECT_OCEAN },
  { "SECT_UDWDWL", "&+gUndrwld wild", 1, SECT_UNDRWLD_WILD },
  { "SECT_UNWDCT", "&+yUndrwld city", 1, SECT_UNDRWLD_CITY },
  { "SECT_UNWDIN", "&+YUndrwld inside", 1, SECT_UNDRWLD_INSIDE },
  { "SECT_UNWDWA", "&+cUndrwld H2O", 1, SECT_UNDRWLD_WATER },
  { "SECT_UNWDWN", "&+cUndrwld H2O (noswim)", 1, SECT_UNDRWLD_NOSWIM },
  { "SECT_UNWDMA", "&+LUndrwld midair", 1, SECT_UNDRWLD_NOGROUND },
  { "SECT_PLANEA", "&+WPlane of air", 1, SECT_PLANE_OF_AIR },
  { "SECT_PLANEW", "&+bPlane of water", 1, SECT_PLANE_OF_WATER },
  { "SECT_PLANEE", "&+yPlane of earth", 1, SECT_PLANE_OF_EARTH },
  { "SECT_ETHERE", "&+cEthereal", 1, SECT_ETHEREAL },
  { "SECT_ASTRAL", "&+LAstral", 1, SECT_ASTRAL },
  { "SECT_DESERT", "&+rDesert", 1, SECT_DESERT },
  { "SECT_ARCTIC", "&+WArctic", 1, SECT_ARCTIC },
  { "SECT_SWAMP", "&+GSwamp", 1, SECT_SWAMP },
  { "SECT_UNWDMT", "&+wUndrwld mountain", 1, SECT_UNDRWLD_MOUNTAIN },
  { "SECT_UNWDSL", "&+gUndrwld slime", 1, SECT_UNDRWLD_SLIME },
  { "SECT_UNWDLC", "&+LUndrwld low ceiling", 1, SECT_UNDRWLD_LOWCEIL },
  { "SECT_UNWDLM", "&+RUndrwld liquid mith", 1, SECT_UNDRWLD_LIQMITH },
  { "SECT_UNWDMS", "&+mUndrwld mushroom", 1, SECT_UNDRWLD_MUSHROOM },
  { "SECT_CSTLWL", "&+CCastle wall", 1, SECT_CASTLE_WALL },
  { "SECT_CSTLGT", "&+yCastle gate", 1, SECT_CASTLE_GATE },
  { "SECT_CASTLE", "&+WCastle", 1, SECT_CASTLE },
  { "SECT_NEGPLN", "&+LNegative plane", 1, SECT_NEG_PLANE },
  { "SECT_AVERNUS", "&+rPlane of Avernus", 1, SECT_PLANE_OF_AVERNUS },
  { "SECT_ROAD", "&+yPatrolled road", 1, SECT_ROAD },
  { "SECT_SNOWY_FOREST", "&+ySnowy forest", 1, SECT_SNOWY_FOREST },
  { 0 }
};


//
// room mana types
//

flagDef g_roomManaList[] =
{
  { "MANA_ALL", "All", 1, APPLY_MANA_ALL },
  { "MANA_GOOD", "Good-only", 1, MANA_GOOD },
  { "MANA_NEUTRAL", "Neutral-only", 1, MANA_NEUTRAL },
  { "MANA_EVIL", "Evil-only", 1, MANA_EVIL },
  { 0 }
};


//
// exit direction types
//

flagDef g_roomExitDirectionList[] =
{
  { "NORTH", "North", 1, NORTH },
  { "EAST", "East", 1, EAST },
  { "SOUTH", "South", 1, SOUTH },
  { "WEST", "West", 1, WEST },
  { "UP", "Up", 1, UP },
  { "DOWN", "Down", 1, DOWN },
  { "NORTHWEST", "Northwest", 1, NORTHWEST },
  { "SOUTHWEST", "Southwest", 1, SOUTHWEST },
  { "NORTHEAST", "Northeast", 1, NORTHEAST },
  { "SOUTHEAST", "Southeast", 1, SOUTHEAST },
  { 0 }
};


/////////////////////////////////
// OBJECTS
//

//
// object type
//

flagDef g_objTypeList[] =
{
  { "ITEM_LIGHT", "Light", 1, ITEM_LIGHT },
  { "ITEM_SCROLL", "Scroll", 1, ITEM_SCROLL },
  { "ITEM_WAND", "Wand", 1, ITEM_WAND },
  { "ITEM_STAFF", "Staff", 1, ITEM_STAFF },
  { "ITEM_WEAPON", "Weapon", 1, ITEM_WEAPON },
  { "ITEM_FIREWEAPON", "Ranged weapon", 1, ITEM_FIREWEAPON },
  { "ITEM_MISSILE", "Missile", 1, ITEM_MISSILE },
  { "ITEM_TREASURE", "Treasure", 1, ITEM_TREASURE },
  { "ITEM_ARMOR", "Armor", 1, ITEM_ARMOR },
  { "ITEM_POTION", "Potion", 1, ITEM_POTION },
  { "ITEM_WORN", "Worn (no AC)", 1, ITEM_WORN },
  { "ITEM_OTHER", "Other", 1, ITEM_OTHER },
  { "ITEM_TRASH", "Trash", 1, ITEM_TRASH },
  { "ITEM_WALL", "Wall", 1, ITEM_WALL },
  { "ITEM_CONTAINER", "Container", 1, ITEM_CONTAINER },
  { "ITEM_NOTE", "Note", 1, ITEM_NOTE },
  { "ITEM_DRINKCON", "Drink container", 1, ITEM_DRINKCON },
  { "ITEM_KEY", "Key", 1, ITEM_KEY },
  { "ITEM_FOOD", "Food", 1, ITEM_FOOD },
  { "ITEM_MONEY", "Money", 1, ITEM_MONEY },
  { "ITEM_PEN", "Pen", 1, ITEM_PEN },
  { "ITEM_BOAT", "Boat", 1, ITEM_BOAT },
  { "ITEM_BOOK", "Book", 1, ITEM_BOOK },
  { "ITEM_CORPSE", "Corpse", 0, ITEM_CORPSE },
  { "ITEM_TELEPORT", "Teleport", 1, ITEM_TELEPORT },
  { "ITEM_TIMER", "Timer", 1, ITEM_TIMER },
  { "ITEM_VEHICLE", "Vehicle", 1, ITEM_VEHICLE },
  { "ITEM_SHIP", "Ship", 1, ITEM_SHIP },
  { "ITEM_SWITCH", "Switch", 1, ITEM_SWITCH },
  { "ITEM_QUIVER", "Quiver", 1, ITEM_QUIVER },
  { "ITEM_PICK", "Lockpick", 1, ITEM_PICK },
  { "ITEM_INSTRUMENT", "Instrument", 1, ITEM_INSTRUMENT },
  { "ITEM_SPELLBOOK", "Spellbook", 1, ITEM_SPELLBOOK },
  { "ITEM_TOTEM", "Totem", 1, ITEM_TOTEM },
  { "ITEM_STORAGE", "Storage", 1, ITEM_STORAGE },
  { "ITEM_SCABBARD", "Scabbard", 1, ITEM_SCABBARD },
  { "ITEM_SHIELD", "Shield", 1, ITEM_SHIELD },
  { "ITEM_TROOP", "Troops", 1, ITEM_TROOP },
  { 0 }
};


//
// object armor thickness types
//

flagDef g_objArmorThicknessList[] =
{
  { "ARMOR_THICKNESS_VERY_THIN", "Very thin", 1, ARMOR_THICKNESS_VERY_THIN },
  { "ARMOR_THICKNESS_THIN", "Thin", 1, ARMOR_THICKNESS_THIN },
  { "ARMOR_THICKNESS_AVERAGE", "Average", 1, ARMOR_THICKNESS_AVERAGE },
  { "ARMOR_THICKNESS_THICK", "Thick", 1, ARMOR_THICKNESS_THICK },
  { "ARMOR_THICKNESS_VERY_THICK", "Very thick", 1, ARMOR_THICKNESS_VERY_THICK },
  { 0 }
};


//
// object craftsmanship
//

flagDef g_objCraftsmanshipList[] =
{
  { "OBJCRAFT_TERRIBLE", "Terribly made", 1, OBJCRAFT_TERRIBLE },
  { "OBJCRAFT_EXTREMELY_POOR", "Extremely poorly made", 1, OBJCRAFT_EXTREMELY_POOR },
  { "OBJCRAFT_VERY_POOR", "Very poorly made", 1, OBJCRAFT_VERY_POOR },
  { "OBJCRAFT_FAIRLY_POOR", "Fairly poorly made", 1, OBJCRAFT_FAIRLY_POOR },
  { "OBJCRAFT_WELL_BELOW", "Well below average", 1, OBJCRAFT_WELL_BELOW },
  { "OBJCRAFT_BELOW_AVG", "Below average", 1, OBJCRAFT_BELOW_AVG },
  { "OBJCRAFT_SLIGHTLY_BELOW", "Slightly below average", 1, OBJCRAFT_SLIGHTLY_BELOW },
  { "OBJCRAFT_AVERAGE", "Average", 1, OBJCRAFT_AVERAGE },
  { "OBJCRAFT_SLIGHTLY_ABOVE", "Slightly above average", 1, OBJCRAFT_SLIGHTLY_ABOVE },
  { "OBJCRAFT_ABOVE_AVG", "Above average", 1, OBJCRAFT_ABOVE_AVG },
  { "OBJCRAFT_WELL_ABOVE", "Well above average", 1, OBJCRAFT_WELL_ABOVE },
  { "OBJCRAFT_EXCELLENT", "Excellently made", 1, OBJCRAFT_EXCELLENT },
  { "OBJCRAFT_GOOD_ARTISAN", "Made by a skilled artisan", 1, OBJCRAFT_GOOD_ARTISAN },
  { "OBJCRAFT_VERY_ARTISAN", "Made by a very skilled artisan", 1, OBJCRAFT_VERY_ARTISAN },
  { "OBJCRAFT_GREAT_ARTISAN", "Made by a master artisan", 1, OBJCRAFT_GREAT_ARTISAN },
  { "OBJCRAFT_SPOOGEALICIOUS", "One-of-a-kind craftsmanship", 1, OBJCRAFT_SPOOGEALICIOUS },
  { 0 }
};


//
// object size types
//

flagDef g_objSizeList[] =
{
  { "OBJSIZE_NONE", "None (very tiny)", 1, OBJSIZE_NONE },
  { "OBJSIZE_TINY", "Tiny", 1, OBJSIZE_TINY },
  { "OBJSIZE_SMALL", "Small", 1, OBJSIZE_SMALL },
  { "OBJSIZE_MEDIUM", "Medium", 1, OBJSIZE_MEDIUM },
  { "OBJSIZE_LARGE", "Large", 1, OBJSIZE_LARGE },
  { "OBJSIZE_HUGE", "Huge", 1, OBJSIZE_HUGE },
  { "OBJSIZE_GIANT", "Giant", 1, OBJSIZE_GIANT },
  { "OBJSIZE_GARGANT", "Gargantuan", 1, OBJSIZE_GARGANT },
  { "OBJSIZE_SMALLMED", "Small-medium", 1, OBJSIZE_SMALLMED },
  { "OBJSIZE_MEDLARGE", "Medium-large", 1, OBJSIZE_MEDLARGE },
  { "OBJSIZE_MAGICAL", "Magical (any size)", 1, OBJSIZE_MAGICAL },
  { 0 }
};

//
// obj trap types
//

flagDef g_objTrapDamList[] =
{
  { "TRAP_DAM_SLEEP", "&+MSleep", 1, TRAP_DAM_SLEEP },
  { "TRAP_DAM_TELEPORT", "&+WTeleport", 1, TRAP_DAM_TELEPORT },
  { "TRAP_DAM_FIRE", "&+rFire", 1, TRAP_DAM_FIRE },
  { "TRAP_DAM_COLD", "&+cCold", 1, TRAP_DAM_COLD },
  { "TRAP_DAM_ACID", "&+gAcid", 1, TRAP_DAM_ACID },
  { "TRAP_DAM_ENERGY", "&+bEnergy", 1, TRAP_DAM_ENERGY },
  { "TRAP_DAM_BLUNT", "&+wBlunt", 1, TRAP_DAM_BLUNT },
  { "TRAP_DAM_PIERCE", "&+LPierce", 1, TRAP_DAM_PIERCE },
  { "TRAP_DAM_SLASH", "&+CSlash", 1, TRAP_DAM_SLASH },
  { "TRAP_DAM_DISPEL", "&+BDispel magic", 1, TRAP_DAM_DISPEL },
  { "TRAP_DAM_GATE", "&+mGate", 1, TRAP_DAM_GATE },
  { "TRAP_DAM_SUMMON", "&+WSummon", 1, TRAP_DAM_SUMMON },
  { "TRAP_DAM_WITHER", "&+LWither", 1, TRAP_DAM_WITHER },
  { "TRAP_DAM_HARM", "&+RHarm", 1, TRAP_DAM_HARM },
  { "TRAP_DAM_POISON", "&+GPoison", 1, TRAP_DAM_POISON },
  { "TRAP_DAM_PARALYSIS", "&+LParalysis", 1, TRAP_DAM_PARALYSIS },
  { "TRAP_DAM_STUN", "&+YStun", 1, TRAP_DAM_STUN },
  { 0 }
};


//
// obj weapon types
//

flagDef g_objWeaponTypeList[] =
{
  { "WEAPON_AXE", "Axe", 1, WEAPON_AXE },
  { "WEAPON_DAGGER", "Dagger", 1, WEAPON_DAGGER },
  { "WEAPON_FLAIL", "Flail", 1, WEAPON_FLAIL },
  { "WEAPON_HAMMER", "Hammer", 1, WEAPON_HAMMER },
  { "WEAPON_LONGSWORD", "Longsword", 1, WEAPON_LONGSWORD },
  { "WEAPON_MACE", "Mace", 1, WEAPON_MACE },
  { "WEAPON_SPIKED_MACE", "Spiked mace", 1, WEAPON_SPIKED_MACE },
  { "WEAPON_POLEARM", "Polearm", 1, WEAPON_POLEARM },
  { "WEAPON_SHORTSWORD", "Shortsword", 1, WEAPON_SHORTSWORD },
  { "WEAPON_CLUB", "Club", 1, WEAPON_CLUB },
  { "WEAPON_SPIKED_CLUB", "Spiked club", 1, WEAPON_SPIKED_CLUB },
  { "WEAPON_STAFF", "Staff", 1, WEAPON_STAFF },
  { "WEAPON_2HANDSWORD", "Two-handed sword", 1, WEAPON_2HANDSWORD },
  { "WEAPON_WHIP", "Whip", 1, WEAPON_WHIP },
  { "WEAPON_SPEAR", "Spear", 1, WEAPON_SPEAR },
  { "WEAPON_LANCE", "Lance", 1, WEAPON_LANCE },
  { "WEAPON_SICKLE", "Sickle", 1, WEAPON_SICKLE },
  { "WEAPON_TRIDENT", "Trident", 1, WEAPON_TRIDENT },
  { "WEAPON_HORN", "Horn", 1, WEAPON_HORN },
  { "WEAPON_NUMCHUCKS", "Numchucks", 1, WEAPON_NUMCHUCKS },
  { 0 }
};


//
// obj missile types
//

flagDef g_objMissileTypeList[] = 
{
  { "MISSILE_ARROW", "Arrow", 1, MISSILE_ARROW },
  { "MISSILE_LIGHT_CBOW_QUARREL", "Light cbow bolt", 1, MISSILE_LIGHT_CBOW_QUARREL },
  { "MISSILE_HEAVY_CBOW_QUARREL", "Heavy cbow bolt", 1, MISSILE_HEAVY_CBOW_QUARREL },
  { "MISSILE_HAND_CBOW_QUARREL", "Hand cbow bolt", 1, MISSILE_HAND_CBOW_QUARREL },
  { "MISSILE_SLING_BULLET", "Sling bullet", 1, MISSILE_SLING_BULLET },
  { "MISSILE_DART", "Dart", 1, MISSILE_DART },
  {0}
};

//
// obj instrument types
//

flagDef g_objInstrTypeList[] =
{
  { "INSTRUMENT_FLUTE", "Flute", 1, INSTRUMENT_FLUTE },
  { "INSTRUMENT_LYRE", "Lyre", 1, INSTRUMENT_LYRE },
  { "INSTRUMENT_MANDOLIN", "Mandolin", 1, INSTRUMENT_MANDOLIN },
  { "INSTRUMENT_HARP", "Harp", 1, INSTRUMENT_HARP },
  { "INSTRUMENT_DRUMS", "Drums", 1, INSTRUMENT_DRUMS },
  { "INSTRUMENT_HORN", "Horn", 1, INSTRUMENT_HORN },
  { 0 }
};



//
// obj liquid types
//

flagDef g_objLiquidTypeList[] =
{
  { "LIQ_WATER", "&+wWater", 1, LIQ_WATER },
  { "LIQ_BEER", "&+yBeer", 1, LIQ_BEER },
  { "LIQ_WINE", "&+rWine", 1, LIQ_WINE },
  { "LIQ_ALE", "&+yAle", 1, LIQ_ALE },
  { "LIQ_DARKALE", "&+LDark ale", 1, LIQ_DARKALE },
  { "LIQ_WHISKY", "&+wWhiskey", 1, LIQ_WHISKY },
  { "LIQ_LEMONADE", "&+YLemonade", 1, LIQ_LEMONADE },
  { "LIQ_FIREBRT", "&+rFirebreather", 1, LIQ_FIREBRT },
  { "LIQ_LOCALSPC", "&+wLocal specialty", 1, LIQ_LOCALSPC },
  { "LIQ_SLIME", "&+gSlime", 1, LIQ_SLIME },
  { "LIQ_MILK", "&+WMilk", 1, LIQ_MILK },
  { "LIQ_TEA", "&+yTea", 1, LIQ_TEA },
  { "LIQ_COFFEE", "&+yCoffee", 1, LIQ_COFFEE },
  { "LIQ_BLOOD", "&+rBlood", 1, LIQ_BLOOD },
  { "LIQ_SALTWATER", "&+wSaltwater", 1, LIQ_SALTWATER },
  { "LIQ_COKE", "&+LCoca-Cola (tm)", 1, LIQ_COKE },
  { "LIQ_FOUNTAIN", "&+wFountain", 1, LIQ_FOUNTAIN },
  { "LIQ_HOLYWATER", "&+WHoly water", 1, LIQ_HOLYWATER },
  { "LIQ_MISO", "&+rMiso", 1, LIQ_MISO },
  { "LIQ_MINESTRONE", "&+yMinestrone", 1, LIQ_MINESTRONE },
  { "LIQ_DUTCH", "&+wDutch", 1, LIQ_DUTCH },
  { "LIQ_SHARK", "&+LShark fin soup", 1, LIQ_SHARK },
  { "LIQ_BIRD", "&+yBird's nest soup", 1, LIQ_BIRD },
  { "LIQ_CHAMPAGNE", "&+wChampagne", 1, LIQ_CHAMPAGNE },
  { "LIQ_PEPSI", "&+LPepsi (tm)", 1, LIQ_PEPSI },
  { "LIQ_LITWATER", "&+wLit water", 1, LIQ_LITWATER },
  { "LIQ_SAKE", "&+wSake", 1, LIQ_SAKE },
  { "LIQ_POISON", "&+RPoison", 1, LIQ_POISON },
  { "LIQ_UNHOLYWAT", "&+LUnholy water", 1, LIQ_UNHOLYWAT },
  { "LIQ_SCHNAPPES", "&+wSchnappes", 1, LIQ_SCHNAPPES },
  { 0 }
};


//
// obj shield types
//

flagDef g_objShieldTypeList[] =
{
  { "SHIELDTYPE_STRAPARM", "Strapped to the arm", 1, SHIELDTYPE_STRAPARM },
  { "SHIELDTYPE_HANDHELD", "Hand-held", 1, SHIELDTYPE_HANDHELD },
  { 0 }
};


//
// obj shield shapes
//

flagDef g_objShieldShapeList[] =
{
  { "SHIELDSHAPE_CIRCULAR", "Circular", 1, SHIELDSHAPE_CIRCULAR },
  { "SHIELDSHAPE_SQUARE", "Square", 1, SHIELDSHAPE_SQUARE },
  { "SHIELDSHAPE_RECTVERT", "Rectangular - aligned vertically", 1, SHIELDSHAPE_RECTVERT },
  { "SHIELDSHAPE_RECTHORZ", "Rectangular - aligned horizontally", 1, SHIELDSHAPE_RECTHORZ },
  { "SHIELDSHAPE_OVALVERT", "Oval - aligned vertically", 1, SHIELDSHAPE_OVALVERT },
  { "SHIELDSHAPE_OVALHORZ", "Oval - aligned horizontally", 1, SHIELDSHAPE_OVALHORZ },
  { "SHIELDSHAPE_TRIBIGUP", "Triangle - wide side on top", 1, SHIELDSHAPE_TRIBIGUP },
  { "SHIELDSHAPE_TRISMLUP", "Triangle - wide side on bottom", 1, SHIELDSHAPE_TRISMLUP },
  { 0 }
};


//
// obj shield sizes
//

flagDef g_objShieldSizeList[] =
{
  { "SHIELDSIZE_TINY", "Tiny   - really small        (less than 1')", 1, SHIELDSIZE_TINY },
  { "SHIELDSIZE_SMALL", "Small  - bucklers, etc       (1-2')", 1, SHIELDSIZE_SMALL },
  { "SHIELDSIZE_MEDIUM", "Medium - normal shields      (2-3')", 1, SHIELDSIZE_MEDIUM },
  { "SHIELDSIZE_LARGE", "Large  - larger than average (3-5')", 1, SHIELDSIZE_LARGE },
  { "SHIELDSIZE_HUGE", "Huge   - really big          (5'+)", 1, SHIELDSIZE_HUGE },
  { 0 }
};





/////////////////////////////////
// MOBS
//


//
// mob sex types
//

flagDef g_mobSexList[] =
{
  { "SEX_NEUTER", "Neuter", 1, SEX_NEUTER },
  { "SEX_MALE", "Male", 1, SEX_MALE },
  { "SEX_FEMALE", "Female", 1, SEX_FEMALE },
  { 0 }
};


//
// mob position types
//

flagDef g_mobPositionList[] =
{
  { "POSITION_DEAD", "Dead", 0, POSITION_DEAD },
  { "POSITION_MORTALLYW", "Mortally wounded", 0, POSITION_MORTALLYW },
  { "POSITION_INCAP", "Incapacitated", 0, POSITION_INCAP },
  { "POSITION_STUNNED", "Stunned", 0, POSITION_STUNNED },
  { "POSITION_SLEEPING", "Sleeping", 1, POSITION_SLEEPING },
  { "POSITION_RESTING", "Resting", 1, POSITION_RESTING },
  { "POSITION_SITTING", "Sitting", 1, POSITION_SITTING },
  { "POSITION_FIGHTING", "Fighting", 0, POSITION_FIGHTING },
  { "POSITION_STANDING", "Standing", 1, POSITION_STANDING },
  { "POSITION_PRONE", "Prone", 1, POSITION_PRONE },
  { "POSITION_LEVITATED", "Levitating", 0, POSITION_LEVITATED },
  { "POSITION_FLYING", "Flying", 0, POSITION_FLYING },
  { "POSITION_SWIMMING", "Swimming", 0, POSITION_SWIMMING },
  { 0 }
};


//
// mob size types
//

flagDef g_mobSizeList[] =
{
  { "MOB_SIZE_DEFAULT", "Default", 1, MOB_SIZE_DEFAULT },
  { "MOB_SIZE_NONE", "None (very tiny)", 1, MOB_SIZE_NONE },
  { "MOB_SIZE_TINY", "Tiny", 1, MOB_SIZE_TINY },
  { "MOB_SIZE_SMALL", "Small", 1, MOB_SIZE_SMALL },
  { "MOB_SIZE_MEDIUM", "Medium", 1, MOB_SIZE_MEDIUM },
  { "MOB_SIZE_LARGE", "Large", 1, MOB_SIZE_LARGE },
  { "MOB_SIZE_HUGE", "Huge", 1, MOB_SIZE_HUGE },
  { "MOB_SIZE_GIANT", "Giant", 1, MOB_SIZE_GIANT },
  { "MOB_SIZE_GARGANTUAN", "Gargantuan", 1, MOB_SIZE_GARGANTUAN },
  { 0 }
};


//
// mob hometown types
//

flagDef g_mobHometownList[] =
{
  { "HOME_NONE", "None", 1, HOME_NONE },
  { "HOME_WINTERHAVEN", "Winterhaven", 1, HOME_WINTERHAVEN },
  { "HOME_IXARKON", "Ixarkon", 1, HOME_IXARKON },
  { "HOME_ARACHDRATHOS", "Arachdrathos", 1, HOME_ARACHDRATHOS },
  { "HOME_SYLVANDAWN", "Sylvandawn", 1, HOME_SYLVANDAWN },
  { "HOME_KIMORDRIL", "Kimordril", 1, HOME_KIMORDRIL },
  { "HOME_KHILDARAK", "Khildarak", 1, HOME_KHILDARAK },
  { "HOME_WOODSEER", "Woodseer", 1, HOME_WOODSEER },
  { "HOME_ASHRUMITE", "Ashrumite", 1, HOME_ASHRUMITE },
  { "HOME_FAANG", "Faang", 1, HOME_FAANG },
  { "HOME_GHORE", "Ghore", 1, HOME_GHORE },
  { "HOME_UGTA", "Ugta", 1, HOME_UGTA },
  { "HOME_BLOODSTONE", "Bloodstone", 1, HOME_BLOODSTONE },
  { "HOME_SHADY", "Shady Grove", 1, HOME_SHADY },
  { "HOME_NAXVARAN", "Naxvaran", 1, HOME_NAXVARAN },
  { "HOME_MARIGOT", "Marigot", 1, HOME_MARIGOT },
  { "HOME_CHARIN", "Charin", 1, HOME_CHARIN },
  { "HOME_CITYRUINS", "City ruins", 1, HOME_CITYRUINS },
  { "HOME_KHHIYTIK", "Khhiytik", 1, HOME_KHHIYTIK },
  { "HOME_GITHFORT", "Githyanki fort", 1, HOME_GITHFORT },
  { "HOME_GOBLIN", "Goblin", 1, HOME_GOBLIN },
  { "HOME_HARPY", "Harpy", 1, HOME_HARPY },
  { "HOME_NEWBIE", "Newbie", 1, HOME_NEWBIE },
  { 0 }
};


//
// quest receive/give types
//

flagDef g_questGiveTypeList[] =
{
  { "QUEST_GITEM_OBJ", "Object vnum", 1, QUEST_GITEM_OBJ },
  { "QUEST_GITEM_COINS", "Coins", 1, QUEST_GITEM_COINS },
  { "QUEST_GITEM_OBJTYPE", "Object type", 1, QUEST_GITEM_OBJTYPE },
  { 0 }
};


flagDef g_questReceiveTypeList[] =
{
  { "QUEST_RITEM_OBJ", "Object vnum", 1, QUEST_RITEM_OBJ },
  { "QUEST_RITEM_COINS", "Coins", 1, QUEST_RITEM_COINS },
  { "QUEST_RITEM_SKILL", "Skill", 1, QUEST_RITEM_SKILL },
  { "QUEST_RITEM_EXP", "Experience", 1, QUEST_RITEM_EXP },
  { 0 }
};


//
// shopkeeper races
//

flagDef g_shopShopkeeperRaceList[] =
{
  { "SHOP_RACE_HUMAN", "&+CHuman", 1, SHOP_RACE_HUMAN },
  { "SHOP_RACE_BARB", "&+BBarbarian", 1, SHOP_RACE_BARB },
  { "SHOP_RACE_DROW", "&+mDrow Elf", 1, SHOP_RACE_DROW },
  { "SHOP_RACE_GREY", "&+cGrey Elf", 1, SHOP_RACE_GREY },
  { "SHOP_RACE_DWARF", "&+YDwarf", 1, SHOP_RACE_DWARF },
  { "SHOP_RACE_DUERGAR", "&+rDuergar", 1, SHOP_RACE_DUERGAR },
  { "SHOP_RACE_HALFLING", "&+yHalfling", 1, SHOP_RACE_HALFLING },
  { "SHOP_RACE_GNOME", "&+RGnome", 1, SHOP_RACE_GNOME },
  { "SHOP_RACE_OGRE", "&+bOgre", 1, SHOP_RACE_OGRE },
  { "SHOP_RACE_TROLL", "&+gTroll", 1, SHOP_RACE_TROLL },
  { "SHOP_RACE_HALFELF", "&+CHalf&+c-Elf", 1, SHOP_RACE_HALFELF },
  { 0 }
};


//
// zone reset modes
//

flagDef g_zoneResetModeList[] =
{
  { "ZONE_NO_RESET", "Never resets", 1, ZONE_NO_RESET },
  { "ZONE_RESET_EMPTY", "Resets when empty", 1, ZONE_RESET_EMPTY },
  { "ZONE_RESET_ALWAYS", "Resets at lifespan", 1, ZONE_RESET_ALWAYS },
  { 0 }
};
