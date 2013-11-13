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


// SKILLS.H - skills info

#ifndef _SKILLS_H_

#define SKILL_LOWEST                 551
#define SKILL_SNEAK                  551
#define SKILL_HIDE                   552
#define SKILL_STEAL                  553
#define SKILL_BACKSTAB               554
#define SKILL_PICK_LOCK              555
#define SKILL_KICK                   556
#define SKILL_BASH                   557
#define SKILL_RESCUE                 558
#define SKILL_DOUBLE_ATTACK          559
#define SKILL_DUAL_WIELD             560
#define SKILL_DODGE                  561
#define SKILL_HITALL                 562
#define SKILL_TRAP                   563
#define SKILL_BERSERK                564
#define SKILL_APPLY_POISON           565
#define SKILL_TRACK                  566
#define SKILL_LISTEN                 567
#define SKILL_DISARM                 568
#define SKILL_PARRY                  569
#define SKILL_SHADOW                 570
#define SKILL_INSTANT_KILL           571
#define SKILL_HEADBUTT               572
#define SKILL_MEDITATE               573
#define SKILL_SURPRISE               574
#define SKILL_AWARENESS              575
#define SKILL_MOUNT                  576
#define SKILL_FEIGN_DEATH            577
#define SKILL_QUIVERING_PALM         578
#define SKILL_FIRST_AID              579
#define SKILL_SAFE_FALL              580
#define SKILL_SWITCH_OPPONENTS       581
#define SKILL_SPRINGLEAP             582
#define SKILL_MARTIAL_ARTS           583
#define SKILL_UNARMED_DAMAGE         584
#define SKILL_BUDDHA_PALM            585
#define SKILL_HEROISM                586
#define SKILL_CHANT                  587
#define SKILL_DRAGON_PUNCH           588
#define SKILL_THROAT_CRUSH           589
#define SKILL_CALM                   590
#define SKILL_RANGE_SPECIALIST       591
#define SKILL_ARCHERY                592
#define SKILL_RANGE_WEAPONS          593
#define SKILL_CIRCLE                 594
#define SKILL_ATTACK                 595
#define SKILL_RIPOSTE                596
#define SKILL_SUMMON_MOUNT           597
#define SKILL_BANDAGE                598
#define SKILL_SCRIBE                 599
#define SKILL_QUICK_CHANT            600
#define SKILL_SPELL_KNOWLEDGE_CLERICAL 601
#define SKILL_SPELL_KNOWLEDGE_MAGICAL  602

#define SKILL_1H_BLUDGEON            603
#define SKILL_1H_SLASHING            604
#define SKILL_1H_PIERCING            605
#define SKILL_1H_MISC                606
#define SKILL_2H_BLUDGEON            607
#define SKILL_2H_SLASHING            608
#define SKILL_2H_MISC                609

#define SKILL_BLINDFIGHTING          610

#define SKILL_CAST_GENERIC           611
#define SKILL_CAST_FIRE              612
#define SKILL_CAST_COLD              613
#define SKILL_CAST_HEALING           614
#define SKILL_CAST_TELEPORTATION     615
#define SKILL_CAST_SUMMONING         616
#define SKILL_CAST_PROTECTION        617
#define SKILL_CAST_DIVINATION        618
#define SKILL_CAST_ELECTRIC          619
#define SKILL_CAST_ACID              620
#define SKILL_CAST_ENCHANT           621
#define SKILL_SPEC_FIRE              622
#define SKILL_SPEC_COLD              623
#define SKILL_SPEC_HEALING           624
#define SKILL_SPEC_TELEPORTATION     625
#define SKILL_SPEC_SUMMONING         626
#define SKILL_SPEC_PROTECTION        627
#define SKILL_SPEC_DIVINATION        628
#define SKILL_SPEC_ENCHANT           629

#define SKILL_SKIP_LOWEST            630
#define SKILL_SKIP_HIGHEST           647

#define SKILL_BEARHUG                648
#define SKILL_OGRE_ROAR              649
#define SKILL_CARVE                  650
#define SKILL_SUBTERFUGE             651
#define SKILL_TRIP                   652
#define SKILL_DIRTTOSS               653
#define SKILL_DISGUISE               654
#define SKILL_SPELL_KNOWLEDGE_SHAMAN 655
#define SKILL_CAST_SH_ANIMAL         656
#define SKILL_CAST_SH_ELEMENT        657
#define SKILL_CAST_SH_SPIRIT         658
#define SKILL_CAPTURE                659
#define SKILL_TRIPLE_ATTACK          660
#define SKILL_APPRAISE               661
#define SKILL_LORE                   662
#define SKILL_SWIM                   663
#define SKILL_AXE                    664
#define SKILL_DAGGER                 665
#define SKILL_FLAIL                  666
#define SKILL_HAMMER                 667
#define SKILL_LONGSWORD              668
#define SKILL_MACE                   669
#define SKILL_POLEARM                670
#define SKILL_SHORTSWORD             671
#define SKILL_CLUB                   672
#define SKILL_2HANDSWORD             673
#define SKILL_WHIP                   674
#define SKILL_PARRY_AXE              675
#define SKILL_PARRY_DAGGER           676
#define SKILL_PARRY_FLAIL            677
#define SKILL_PARRY_HAMMER           678
#define SKILL_PARRY_LONGSWORD        679
#define SKILL_PARRY_MACE             680
#define SKILL_PARRY_POLEARM          681
#define SKILL_PARRY_SHORTSWORD       682
#define SKILL_PARRY_CLUB             683
#define SKILL_PARRY_2HANDSWORD       684
#define SKILL_PARRY_WHIP             685
#define SKILL_SHIELD                 686
#define SKILL_TWOHANDED_SWORD        687
#define SKILL_STAFF                  688
#define SKILL_PICK                   689
#define SKILL_LANCE          690
#define SKILL_SICKLE             691
#define SKILL_FORK           692
#define SKILL_HORN           693
#define SKILL_NUMCHUCKS          694
#define SKILL_BAREHANDED_FIGHTING    695
#define SKILL_GRAPPLE            696
#define SKILL_MOUNTED_COMBAT         697
#define SKILL_CLIMB          698
#define SKILL_ROUNDKICK          699
#define SKILL_REGENERATE         700
#define SKILL_AERIAL_COMBAT          701
#define SKILL_AERIAL_CASTING         702
#define SKILL_TOUCH_DEATH            703
#define SKILL_HAMSTRING              704
#define SKILL_BODYSLAM               705
#define SKILL_SHIELD_BLOCK	     706
#define SKILL_COMBINATION	     707
#define SKILL_GUARD	             708
#define SKILL_SHIELDLESS_BASH	     709

#define SKILL_HIGHEST                   709


#define _SKILLS_H_
#endif
