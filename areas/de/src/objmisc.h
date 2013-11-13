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


/* materials stuff..  feel free to add the standard header if ya want */

#ifndef _OBJMISC_H_

 /* obj->material */

#define MAT_UNDEFINED  0  /* lazy builders, no bonuses */
#define MAT_LOWEST     1
#define MAT_NONSUBSTANTIAL 1
#define MAT_FLESH      2
#define MAT_CLOTH      3
#define MAT_BARK       4
#define MAT_SOFTWOOD   5
#define MAT_HARDWOOD   6
#define MAT_SILICON    7
#define MAT_CRYSTAL    8
#define MAT_CERAMIC    9
#define MAT_BONE      10
#define MAT_STONE     11
#define MAT_HIDE      12
#define MAT_LEATHER   13
#define MAT_CURED_LEATHER 14
#define MAT_IRON      15
#define MAT_STEEL     16
#define MAT_BRASS     17
#define MAT_MITHRIL   18
#define MAT_ADAMANTIUM 19
#define MAT_BRONZE    20
#define MAT_COPPER    21
#define MAT_SILVER    22
#define MAT_ELECTRUM  23
#define MAT_GOLD      24
#define MAT_PLATINUM  25
#define MAT_GEM       26
#define MAT_DIAMOND   27
#define MAT_PAPER     28
#define MAT_PARCHMENT 29
#define MAT_LEAVES    30
#define MAT_RUBY      31
#define MAT_EMERALD   32
#define MAT_SAPPHIRE  33
#define MAT_IVORY     34
#define MAT_DRAGONSCALE 35
#define MAT_OBSIDIAN  36
#define MAT_GRANITE   37
#define MAT_MARBLE    38
#define MAT_LIMESTONE 39
#define MAT_LIQUID    40
#define MAT_BAMBOO    41
#define MAT_REEDS     42
#define MAT_HEMP      43
#define MAT_GLASSTEEL 44
#define MAT_EGGSHELL  45
#define MAT_CHITINOUS 46
#define MAT_REPTILESCALE 47
#define MAT_GENERICFOOD 48
#define MAT_RUBBER    49
#define MAT_FEATHER   50
#define MAT_WAX       51
#define MAT_PEARL     52

#define IS_METAL(mat) ( mat == MAT_IRON || mat == MAT_STEEL || \
                        mat == MAT_COPPER || mat == MAT_BRASS || \
                        mat == MAT_ADAMANTIUM || mat == MAT_GOLD || \
                        mat == MAT_SILVER || mat == MAT_PLATINUM || \
                        mat == MAT_MITHRIL || mat == MAT_ELECTRUM )
#define IS_STONE(mat) ( mat == MAT_GRANITE || mat == MAT_GEM || \
                        mat == MAT_EMERALD || mat == MAT_SAPPHIRE || \
                        mat == MAT_OBSIDIAN || mat == MAT_MARBLE || \
                        mat == MAT_LIMESTONE || mat == MAT_RUBY || \
                        mat == MAT_DIAMOND || mat == MAT_CRYSTAL || \
                        mat == MAT_STONE || mat == MAT_CERAMIC )
#define IS_LEATHER(mat) ( mat == MAT_HIDE || mat == MAT_REPTILESCALE || \
                          mat == MAT_CURED_LEATHER || mat == MAT_LEATHER || \
                          mat == MAT_DRAGONSCALE )
#define IS_WOODEN(mat) ( mat == MAT_BAMBOO || mat == MAT_SOFTWOOD || \
                         mat == MAT_HARDWOOD || mat == MAT_BARK )
#define IS_CLOTH(mat) ( mat == MAT_REEDS || mat == MAT_HEMP || \
                        mat == MAT_LEAVES || mat == MAT_PARCHMENT || \
                        mat == MAT_PAPER || mat == MAT_CLOTH || \
                        mat == MAT_FEATHER )
#define IS_PLASTIC(mat) ( mat == MAT_WAX || mat == MAT_RUBBER || \
                          mat == MAT_GENERICFOOD || mat == MAT_FLESH || \
                          mat == MAT_SILICON )
#define IS_RIGID(mat) ( IS_METAL(mat) || IS_STONE(mat) || IS_WOODEN(mat) || \
                        mat == MAT_IVORY || mat == MAT_PEARL || \
                        mat == MAT_CHITINOUS || mat == MAT_BONE )

#define MAT_HIGHEST     52
#define NUMB_MATERIALS (MAT_HIGHEST + 1)

#define OBJCRAFT_LOWEST  0
#define OBJCRAFT_AVERAGE 7
#define OBJCRAFT_HIGHEST 15

/* assorted armor info */

// armor thickness (val3)

// these values aren't really cut-and-dry..  apply as you wish

#define ARMOR_THICKNESS_LOWEST           1
#define ARMOR_THICKNESS_VERY_THIN        1
#define ARMOR_THICKNESS_THIN             2
#define ARMOR_THICKNESS_AVERAGE          3
#define ARMOR_THICKNESS_THICK            4
#define ARMOR_THICKNESS_VERY_THICK       5
#define ARMOR_THICKNESS_HIGHEST          5

/* obj->status */

#define STAT_BRITTLE   1
#define STAT_HEATED    2
#define STAT_ELECTRIFIED 4

/* weapon stuff */

#define WEAPON_LOWEST       0
#define WEAPON_NONE	    0
#define WEAPON_AXE          1  // axes - slashing
#define WEAPON_DAGGER       2  // daggers, knives - piercing, slashing (with -)
#define WEAPON_FLAIL        3  // flails - whip
#define WEAPON_HAMMER       4  // hammers - bludgeon
#define WEAPON_LONGSWORD    5  // long swords - slashing/piercing (with -)
#define WEAPON_MACE         6  // mace - bludgeon
#define WEAPON_SPIKED_MACE  7  // should be removed
#define WEAPON_POLEARM      8  // polearm - halberds, guisarmes, glaives - slashing
#define WEAPON_SHORTSWORD   9  // short swords - slashing/piercing
#define WEAPON_CLUB        10  // clubs - bludgeon
#define WEAPON_SPIKED_CLUB 11  // should be removed
#define WEAPON_STAFF       12  // staff - like club but longer, maybe 2-handed -
                               //   bludgeon
#define WEAPON_2HANDSWORD  13  // should be removed
#define WEAPON_WHIP        14  // whips - whip
#define WEAPON_SPEAR       15  // long piercing weapons - pierce
#define WEAPON_LANCE       16  // lance - special handling probably
#define WEAPON_SICKLE      17  // sickle - slash/pierce?
#define WEAPON_TRIDENT     18  // forks/rakes - slash
#define WEAPON_HORN        19  // should be removed
#define WEAPON_NUMCHUCKS   20  // numchucks - bludgeon
#define WEAPON_HIGHEST     20
#define NUMB_WEAPONS      (WEAPON_HIGHEST - WEAPON_LOWEST)

#define WEAPONTYPE_UNDEFINED  0
#define WEAPONTYPE_BLUDGEON   1
#define WEAPONTYPE_SLASH      2
#define WEAPONTYPE_PIERCE     3
#define WEAPONTYPE_WHIP       4

#define MISSILE_FIRST                   1
#define MISSILE_ARROW                   1
#define MISSILE_LIGHT_CBOW_QUARREL      2
#define MISSILE_HEAVY_CBOW_QUARREL      3
#define MISSILE_HAND_CBOW_QUARREL       4
#define MISSILE_SLING_BULLET            5
#define MISSILE_DART                    6
#define MISSILE_LAST                    6

#define IS_BACKSTABBER(obj) ((obj)->type == ITEM_WEAPON &&\
                             ((obj)->value[0] == WEAPON_DAGGER ||\
                              (obj)->value[0] == WEAPON_SHORTSWORD ||\
                              (obj)->value[0] == WEAPON_TRIDENT ||\
                              (obj)->value[0] == WEAPON_HORN))

#define IS_FLAYING(obj) ((obj)->type == ITEM_WEAPON &&\
                             ((obj)->value[0] == WEAPON_FLAIL ||\
                              (obj)->value[0] == WEAPON_WHIP))

#define IS_SWORD(obj) ((obj)->type == ITEM_WEAPON && \
                       ((obj)->value[0] == WEAPON_LONGSWORD ||\
                        (obj)->value[0] == WEAPON_SHORTSWORD ||\
                        (obj)->value[0] == WEAPON_2HANDSWORD))

#define IS_LANCE(obj) ((obj)->type == ITEM_WEAPON &&\
                       (obj)->value[0] == WEAPON_LANCE)

#define IS_REACH_WEAPON(obj) ((obj)->type == ITEM_WEAPON &&\
                             IS_SET((obj)->extra_flags, ITEM_TWOHANDS) &&\
                             ((obj)->value[0] == WEAPON_POLEARM ||\
                              (obj)->value[0] == WEAPON_SPEAR ||\
                              (obj)->value[0] == WEAPON_TRIDENT))

#define IS_DART(obj) ((obj)->type == ITEM_MISSILE &&\
                      (obj)->value[3] == MISSILE_DART)

#define IS_DAGGER(obj) ((obj)->type == ITEM_WEAPON && \
                       (obj)->value[0] == WEAPON_DAGGER)

#define CAN_HURT(ch, obj, mob) (!has_innate(mob, INNATE_WEAPON_IMMUNITY) ||\
      ((obj) && IS_SET((obj)->extra2_flags, ITEM2_MAGIC)) ||\
      (!(obj) && (GET_LEVEL(ch) >= 51)))
#define IS_SLAYING(obj, mob) ((IS_SET((obj)->extra2_flags, ITEM2_SLAY_GOOD) && IS_GOOD(mob)) ||\
                              (IS_SET((obj)->extra2_flags, ITEM2_SLAY_EVIL) && IS_EVIL(mob)) ||\
                              (IS_SET((obj)->extra2_flags, ITEM2_SLAY_UNDEAD) && IS_UNDEAD(mob)) ||\
                              (IS_SET((obj)->extra2_flags, ITEM2_SLAY_LIVING) && !IS_UNDEAD(mob)))
  
/* shield stuff */

// type (val0)

#define SHIELDTYPE_LOWEST   1
#define SHIELDTYPE_STRAPARM 1  // strapped to the arm - bucklers
#define SHIELDTYPE_HANDHELD 2  // held by hand
#define SHIELDTYPE_HIGHEST  2

// shape (val1)

#define SHIELDSHAPE_LOWEST   1
#define SHIELDSHAPE_CIRCULAR 1  // perfect circle
#define SHIELDSHAPE_SQUARE   2
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


#define _OBJMISC_H_
#endif
