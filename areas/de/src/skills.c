/*
***************************************************************************
*  File: skills.c                                          Part of Duris *
*  Usage: core routines for handling spellcasting                         *
*  Copyright  1990, 1991 - see 'license.doc' for complete information.    *
*  Copyright  1994, 1995 - Duris Systems Ltd.                             *
***************************************************************************
*/
#include <string.h>
#include "defines.h"
#include "utils.h"
#ifndef _DE_
#   include "structs.h"
int      numSkills;
#endif

#if defined(_DE_) || defined(_PFILE_)
struct skill_data
{
  char    *name;
#if defined (_PFILE_)
  struct ClassSkillInfo m_class[CLASS_COUNT];     /* info for each class */
#endif
};
typedef struct skill_data Skill;
extern int flag2idx(int);
#else
#   include "prototypes.h"
#   include "skillrec.h"
extern int SortedSkills[];
#endif
#include "spells.h"

Skill    skills[MAX_AFFECT_TYPES];

#if defined(_DE_) || defined(_PFILE_)
#   define SPELL_CREATE(Name, Index, Type, Beats, MinPos, Targets, Harmful, Spell_pointer, Wear_off) \
  skills[Index].name = (Name)

#   define SKILL_CREATE(Name, Index, Type) \
  skills[Index].name = (Name)

#   define SKILL_CREATE_WITH_MESSAGES(Name, Index, Type, wearoff_char, wearoff_room) \
  SKILL_CREATE(Name, Index, Type)
#   define SKILL_CREATE_WITH_CAT(Name, Index, Type, cat) \
  SKILL_CREATE(Name, Index, Type)
#   define SKILL_CREATE_WITH_MESSAGES_WITH_CAT(Name, Index, Type, wearoff_char, wearoff_room, cat) \
  SKILL_CREATE(Name, Index, Type)


#   define SPELL_CREATE2(Name, Index, Type, Beats, MinPos, Targets, Harmful, Spell_pointer, Wear_off, wear_off_room) \
	SPELL_CREATE(Name, Index, Type, Beats, MinPos, Targets, Harmful, Spell_pointer, Wear_off)

#   define SPEC_SPELL_ADD(Class, Level, Spec)

#   define SPELL_ADD(Class, Level)
#   define TAG_CREATE(Name, Index)
#   define TAG_CREATE_WITH_MESSAGES(Name, Index, wear_off, wear_off_room)
#   define POISON_CREATE(Name, Index, Spell_pointer) \
  skills[Index].name = (Name)
#if defined (_PFILE_)
#   define SPEC_SKILL_ADD(Class, Level,  MaxLearn,  Spec) \
    skills[numSkills].m_class[flag2idx(Class)-1].rlevel = Level;\
    skills[numSkills].m_class[flag2idx(Class)-1].maxlearn[Spec] = MaxLearn;

#   define SKILL_ADD(Class, Level, MaxLearn) \
    skills[numSkills].m_class[flag2idx(Class)-1].rlevel = Level;\
    skills[numSkills].m_class[flag2idx(Class)-1].maxlearn[0] = MaxLearn;
#else
#   define SPEC_SKILL_ADD(Class, Level,  MaxLearn,  Spec)
#   define SKILL_ADD(Class, Level, MaxLearn)
#endif

#else
void SPELL_CREATE(const char *Name, int Index, int Type, int Beats,
                  int MinPos, int Targets, int Harmful,
                  void (*Spell_pointer) (int, P_char, char *, int, P_char,
                                         P_obj), const char *Wear_off)
{
  skills[Index].name = (Name);
  skills[Index].wear_off_char[0] = (Wear_off);
  skills[Index].spell_pointer = Spell_pointer;
  skills[Index].min_pos = (byte) MinPos;
  skills[Index].beats = (sh_int) Beats;
  skills[Index].harmful = (byte) Harmful;
  skills[Index].targets = (unsigned int) Targets;
  skills[Index].type = (int) Type;
  SortedSkills[Index] = Index;
  numSkills = Index;
}

void SKILL_CREATE_WITH_CAT(const char *Name, int Index, int Type, int catergory)
{
    skills[Index].name = (Name);
    skills[Index].type = (Type);
    skills[Index].harmful = -1;
    skills[Index].category = (sh_int) catergory;
    SortedSkills[Index] = Index;
    numSkills = Index;
            
}

void SKILL_CREATE(const char *Name, int Index, int Type)
{
  skills[Index].name = (Name);
  skills[Index].type = (Type);
  skills[Index].harmful = -1;
   skills[Index].category = (sh_int) 0;
  SortedSkills[Index] = Index;
  numSkills = Index;
}


#   define TAG_CREATE(Name, Index) SKILL_CREATE(Name, Index, 0)
#   define TAG_CREATE_WITH_MESSAGES(Name, Index, wear_off, wear_off_room)\
   SKILL_CREATE_WITH_MESSAGES(Name, Index, 0, wear_off, wear_off_room)
#   define POISON_CREATE(Name, Index, Spell_pointer) SPELL_CREATE(Name, Index, 0, 0, 0, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1, (spell_func)Spell_pointer, 0)

void SKILL_CREATE_WITH_MESSAGES_WITH_CAT(const char *Name, int Index, int Type,
                                  const char *wearoff_char,
                                  const char *wearoff_room,
                                  int catergory
                                  )
{
    SKILL_CREATE_WITH_CAT(Name, Index, Type, (sh_int) catergory);
      skills[Index].wear_off_char[0] = wearoff_char;
        skills[Index].wear_off_room[0] = wearoff_room;
}

void SKILL_CREATE_WITH_MESSAGES(const char *Name, int Index, int Type,
                                const char *wearoff_char,
                                const char *wearoff_room)
{
  SKILL_CREATE(Name, Index, Type);
  skills[Index].wear_off_char[0] = wearoff_char;
  skills[Index].wear_off_room[0] = wearoff_room;
}


void SPEC_SKILL_ADD(int Class, int Level, int MaxLearn, int Spec)
{
  skills[numSkills].m_class[(int) (flag2idx(Class) - 1)].rlevel =
    (byte) (Level);
  skills[numSkills].m_class[(int) (flag2idx(Class) - 1)].maxlearn[Spec] =
    (ubyte) (MaxLearn);
}



void SKILL_ADD(int Class, int Level, int MaxLearn)
{
  skills[numSkills].m_class[(int) (flag2idx(Class) - 1)].rlevel =
    (byte) (Level);
  skills[numSkills].m_class[(int) (flag2idx(Class) - 1)].maxlearn[0] =
    (ubyte) (MaxLearn);
  
}

void SPELL_CREATE2(const char *Name, int Index, int Type, int Beats,
                   int MinPos, int Targets, int Harmful,
                   void (*Spell_pointer) (int, P_char, char *, int, P_char,
                                          P_obj), const char *Wear_off,
                   const char *wear_off_room)
{
  SPELL_CREATE(Name, Index, Type, Beats, MinPos, Targets, Harmful,
               Spell_pointer, Wear_off);
  skills[Index].wear_off_room[0] = wear_off_room;
}

void SPEC_SPELL_ADD(int Class, int Level, int Spec)
{
  skills[numSkills].m_class[(int) (flag2idx(Class) - 1)].rlevel =
    (byte) (Level);
  skills[numSkills].m_class[(int) (flag2idx(Class) - 1)].maxlearn[Spec] = 100;
}

void SPELL_ADD(int Class, int Level)
{
  skills[numSkills].m_class[(int) (flag2idx(Class) - 1)].rlevel =
    (byte) (Level);
  skills[numSkills].m_class[(int) (flag2idx(Class) - 1)].maxlearn[0] = 100;
}

#endif


void initialize_skills()
{
  int      i;

  memset(skills, 0, sizeof(skills));

  for (i = 0; i < MAX_AFFECT_TYPES; i++)
  {
    skills[i].name = "Undefined";
  }

  //needed by setbit
  skills[MAX_AFFECT_TYPES].name = "\n";

// Alchemist
  // Brawler

// Psionicist
  SKILL_CREATE_WITH_CAT("flame mastery", SKILL_FLAME_MASTERY, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_DEFENSIVE);
  SPEC_SKILL_ADD(CLASS_PSIONICIST, 46, 100, SPEC_PYROKINETIC);
  
  SKILL_CREATE_WITH_CAT("disperse flames", SKILL_DISPERSE_FLAMES, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_DEFENSIVE);
  SPEC_SKILL_ADD(CLASS_PSIONICIST, 41, 100, SPEC_PYROKINETIC);

  // Dreadlord
  SKILL_CREATE_WITH_CAT("fade", SKILL_FADE, SKILL_TYPE_PHYS_DEX , SKILL_CATEGORY_DEFENSIVE);
  SPEC_SKILL_ADD(CLASS_DREADLORD, 51, 100, SPEC_SHADOWLORD);

  SKILL_CREATE_WITH_CAT("shadow movement", SKILL_SHADOW_MOVEMENT, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_DREADLORD, 31, 100, SPEC_SHADOWLORD);

SKILL_CREATE_WITH_CAT("soul trap", SKILL_SOUL_TRAP, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_DREADLORD, 36, 100, SPEC_DEATHLORD);

/* Mercenary Specs */
  // Opportunitist
  SKILL_CREATE_WITH_CAT("disruptive blow", SKILL_DISRUPTIVE_BLOW, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_MERCENARY, 46, 100, SPEC_OPPORTUNIST);

  SKILL_CREATE_WITH_CAT("crippling strike", SKILL_CRIPPLING_STRIKE,
               SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_MERCENARY, 41, 100, SPEC_OPPORTUNIST);

  SKILL_CREATE_WITH_CAT("ambush", SKILL_AMBUSH, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_DEFENSIVE);
  SPEC_SKILL_ADD(CLASS_MERCENARY, 36, 100, SPEC_OPPORTUNIST);

  SKILL_CREATE_WITH_CAT("mug", SKILL_MUG, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_MERCENARY, 51, 100, SPEC_PROFITEER);

/* Assassin Specs */
  SKILL_CREATE_WITH_CAT("spinal tap", SKILL_SPINAL_TAP, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_ASSASSIN, 35, 100, SPEC_ASSMASTER);
  SPEC_SKILL_ADD(CLASS_ROGUE, 35, 100, SPEC_ASSASSIN);

SKILL_CREATE_WITH_CAT("hamstring", SKILL_HAMSTRING, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_THIEF, 51, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 51, 100, SPEC_THIEF);

  SKILL_CREATE_WITH_CAT("dart throwing", SKILL_DARTS, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_DEFENSIVE);
  SPEC_SKILL_ADD(CLASS_ASSASSIN, 31, 100, SPEC_SHARPSHOOTER);

/* Conj Specs */
  SPELL_CREATE("magma burst", SPELL_MAGMA_BURST, SPELLTYPE_FIRE,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_magma_burst, NULL);
  SPEC_SPELL_ADD(CLASS_CONJURER, 8, SPEC_FIRE);
  SPELL_CREATE("solar flare", SPELL_SOLAR_FLARE, SPELLTYPE_FIRE,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               TRUE, spell_solar_flare, NULL);
  //SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_FIRE);
  SPEC_SPELL_ADD(CLASS_CONJURER, 9, SPEC_FIRE);

  SPELL_CREATE("water to life", SPELL_WATER_TO_LIFE, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM, 0,
               spell_water_to_life, NULL);
  SPEC_SPELL_ADD(CLASS_CONJURER, 7, SPEC_WATER);

  SPELL_CREATE("air form", SPELL_AIR_FORM,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST * 4, TRUE, TAR_SELF_ONLY,
               0, spell_air_form, "Your molecules get back to normal.");
  SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_AIR);

  SPELL_CREATE("ethereal grounds", SPELL_ETHEREAL_GROUNDS,
               SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 4, TRUE, TAR_IGNORE, 0,
               spell_ethereal_grounds, NULL);
  SPEC_SPELL_ADD(CLASS_CONJURER, 11, SPEC_AIR);

  SPELL_CREATE2("earthen tomb", SPELL_EARTHEN_TOMB, SPELLTYPE_GENERIC,
                PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE, 0, cast_earthen_tomb,
                "&+yThe ground stops to &+Lrumble &+yand ceases to quake.&n",
                "&+yThe ground stops to &+Lrumble &+yand ceases to quake.&n");
  SPEC_SPELL_ADD(CLASS_CONJURER, 12, SPEC_EARTH);

  SPELL_CREATE("dread wave", SPELL_DREAD_WAVE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_dread_wave, NULL);
  SPEC_SPELL_ADD(CLASS_CONJURER, 8, SPEC_WATER);

/* Cleric Specs */
  SPELL_CREATE("group heal", SPELL_GROUP_HEAL, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 2, TRUE, TAR_IGNORE, 0, spell_group_heal,
               NULL);

  SPELL_CREATE("lesser sanctuary", SPELL_LESSER_SANCTUARY, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 2, TRUE, TAR_SELF_ONLY, 0, spell_sanctuary,
               NULL);
  SPEC_SPELL_ADD(CLASS_CLERIC, 11, SPEC_HOLYMAN);

/* Warrior Specs */
  SKILL_CREATE_WITH_CAT("quadruple attack", SKILL_QUADRUPLE_ATTACK, SKILL_TYPE_NONE, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_DREADLORD, 56, 30);
  SKILL_ADD(CLASS_AVENGER, 56, 30);
  SKILL_ADD(CLASS_WARRIOR, 56, 30);
  
  
  SKILL_CREATE_WITH_CAT("shield punch", SKILL_SHIELDPUNCH, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 46, 100, SPEC_GUARDIAN);

  SKILL_CREATE_WITH_CAT("shieldless bash", SKILL_SHIELDLESS_BASH, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 36, 85, SPEC_SWORDSMAN);

  SKILL_CREATE_WITH_CAT("sweeping thrust", SKILL_SWEEPING_THRUST, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 31, 100, SPEC_SWORDSMAN);

  SKILL_CREATE_WITH_CAT("follow-up riposte", SKILL_FOLLOWUP_RIPOSTE,
               SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 31, 100, SPEC_GUARDIAN);
  SPEC_SKILL_ADD(CLASS_RANGER, 51, 90, SPEC_BLADEMASTER);
	
  SKILL_CREATE_WITH_CAT("critical attack", SKILL_CRITICAL_ATTACK, SKILL_TYPE_NONE, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_WARRIOR, 41, 100, SPEC_SWORDSMAN);


  SKILL_CREATE_WITH_MESSAGES("war cry", SKILL_WAR_CRY,
                             SKILL_TYPE_PHYS_STR, "Your battle frenzy fades.",
                             NULL);
  SKILL_ADD(CLASS_BERSERKER, 26, 100);

/* Anti-paladin Specs */
  SPELL_CREATE("spawn",
               SPELL_SPAWN, SPELLTYPE_SUMMONING,
               PULSE_SPELLCAST * 3, TRUE, TAR_SELF_ONLY, 0,
               spell_spawn, "&+LThe aura of death slowly fades&n.");
  SPEC_SPELL_ADD(CLASS_ANTIPALADIN, 7, SPEC_SPAWN);

  SPELL_CREATE("armageddon", SPELL_ARMAGEDDON, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 3, TRUE, TAR_SELF_ONLY, 0, spell_armageddon,
               "The powers of &+yarmageddon&n leave you.");
  SPEC_SPELL_ADD(CLASS_ANTIPALADIN, 9, SPEC_APOCALYPTIC);

  SKILL_CREATE_WITH_CAT("skewer", SKILL_SKEWER, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_ANTIPALADIN, 41, 100, SPEC_APOCALYPTIC);
  SPEC_SKILL_ADD(CLASS_PALADIN, 41, 100, SPEC_CRUSADER);

  SPELL_CREATE("apocalypse", SPELL_APOCALYPSE, SPELLTYPE_GENERIC,
               3.5 * PULSE_SPELLCAST, TRUE, TAR_AREA | TAR_OFFAREA, 1,
               spell_apocalypse, NULL);
  SPELL_ADD(CLASS_ANTIPALADIN, 11);

  SPELL_CREATE("judgement", SPELL_JUDGEMENT, SPELLTYPE_GENERIC,
               2 * PULSE_SPELLCAST, TRUE, TAR_AREA | TAR_OFFAREA, 1,
               spell_judgement, NULL);
  SPELL_ADD(CLASS_PALADIN, 11);

  SKILL_CREATE_WITH_CAT("lance charge", SKILL_LANCE_CHARGE, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_ANTIPALADIN, 41, 100, SPEC_DEMONIC);
  SPEC_SKILL_ADD(CLASS_PALADIN, 41, 100, SPEC_CAVALIER);

  SKILL_CREATE_WITH_CAT("sidestep", SKILL_SIDESTEP, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_DEFENSIVE);
  SPEC_SKILL_ADD(CLASS_ANTIPALADIN, 31, 100, SPEC_DEMONIC);
  SPEC_SKILL_ADD(CLASS_PALADIN, 31, 100, SPEC_CAVALIER);

  SKILL_CREATE_WITH_CAT("arcane riposte", SKILL_ARCANE_RIPOSTE, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_DEFENSIVE);
  SPEC_SKILL_ADD(CLASS_SORCERER, 51, 100, SPEC_WIZARD);

  SKILL_CREATE_WITH_CAT("arcane block", SKILL_ARCANE_BLOCK, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_DEFENSIVE);
  SPEC_SKILL_ADD(CLASS_SORCERER, 41, 100, SPEC_WIZARD);

  SPELL_CREATE("chaotic ripple", SPELL_CHAOTIC_RIPPLE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_chaotic_ripple, NULL);
  SPEC_SPELL_ADD(CLASS_SORCERER, 12, SPEC_WILDMAGE);

  SPELL_CREATE("blink", SPELL_BLINK, SPELLTYPE_GENERIC, PULSE_SPELLCAST, TRUE,
               TAR_SELF_ONLY, FALSE, spell_blink, NULL);
  SPEC_SPELL_ADD(CLASS_SORCERER, 6, SPEC_SHADOW);

  SKILL_CREATE_WITH_CAT("control flee", SKILL_CONTROL_FLEE, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_DEFENSIVE);
  SPEC_SKILL_ADD(CLASS_ASSASSIN, 45, 100, SPEC_ASSMASTER);
  SPEC_SKILL_ADD(CLASS_ROGUE, 45, 100, SPEC_ASSASSIN);

  SKILL_CREATE_WITH_CAT("quick step", SKILL_QUICK_STEP, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_DEFENSIVE);
  SPEC_SKILL_ADD(CLASS_THIEF, 36, 90, SPEC_CUTPURSE);

  SKILL_CREATE_WITH_CAT("sneaky strike", SKILL_SNEAKY_STRIKE, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_THIEF, 41, 90, SPEC_CUTPURSE);

  
/* Monk Specs */
  SKILL_CREATE_WITH_MESSAGES_WITH_CAT("fist of dragon", SKILL_FIST_OF_DRAGON,
					       SKILL_TYPE_MENTAL_WIS,
					       "The touch of the dragon leaves your soul.","",
                                                SKILL_CATEGORY_OFFENSIVE                                        
                                               );
  SPEC_SKILL_ADD(CLASS_MONK, 46, 100, SPEC_WAYOFDRAGON);
      
  SKILL_CREATE_WITH_CAT("flurry of blows", SKILL_FLURRY_OF_BLOWS, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_MONK, 31, 100, SPEC_WAYOFSNAKE);
    
/* 
  
  SKILL_CREATE_WITH_MESSAGES("lotus", SKILL_LOTUS,
                             SKILL_TYPE_NONE,
                             "You abandon your lotus trance.",
                             "$n breaks our of his lotus trance.");
  SPEC_SKILL_ADD(CLASS_MONK, 56, 100, SPEC_CHIMONK);

  SKILL_CREATE("chi", SKILL_CHI, SKILL_TYPE_MENTAL_WIS);
  SPEC_SKILL_ADD(CLASS_MONK, 56, 100, SPEC_CHIMONK);

  SKILL_CREATE("true strike", SKILL_TRUE_STRIKE, SKILL_TYPE_PHYS_STR);
  SPEC_SKILL_ADD(CLASS_MONK, 56, 100, SPEC_CHIMONK);

  SKILL_CREATE("displacement", SKILL_DISPLACEMENT, SKILL_TYPE_MENTAL_WIS);
  SPEC_SKILL_ADD(CLASS_MONK, 56, 100, SPEC_CHIMONK); 

  SKILL_CREATE("reconstruction", SKILL_RECONSTRUCTION, SKILL_TYPE_MENTAL_WIS);
  SPEC_SKILL_ADD(CLASS_MONK, 56, 100, SPEC_CHIMONK);

  SKILL_CREATE("sight", SKILL_SIGHT, SKILL_TYPE_MENTAL_WIS);
  SPEC_SKILL_ADD(CLASS_MONK, 56, 100, SPEC_CHIMONK);
*/
/* ranger specs */
  SKILL_CREATE_WITH_CAT("double strike", SKILL_DOUBLE_STRIKE, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_RANGER, 36, 100, SPEC_BLADEMASTER);

  SKILL_CREATE_WITH_MESSAGES_WITH_CAT("whirlwind", SKILL_WHIRLWIND,
                             SKILL_TYPE_PHYS_AGI, "You slow down exhausted.",
                             "$n slows down exhausted.", SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_RANGER, 31, 100, SPEC_BLADEMASTER);

  SPELL_CREATE("healing salve", SPELL_HEALING_SALVE, SPELLTYPE_HEALING,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, FALSE,
               spell_healing_salve, NULL);
  SPEC_SPELL_ADD(CLASS_CLERIC, 5, SPEC_HEALER);

  SPELL_CREATE("plague", SPELL_PLAGUE, SPELLTYPE_HEALING, PULSE_SPELLCAST * 2,
               TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, spell_plague,
               "Your body managed to defeat the disease.");
  SPEC_SPELL_ADD(CLASS_CLERIC, 11, SPEC_ZEALOT);

  SPELL_CREATE("banish", SPELL_BANISH, SPELLTYPE_GENERIC, PULSE_SPELLCAST * 2,
               TRUE, TAR_AREA | TAR_OFFAREA, TRUE, spell_banish, 0);
  SPEC_SPELL_ADD(CLASS_CLERIC, 8, SPEC_ZEALOT);

  SKILL_CREATE_WITH_CAT("vicious attack", SKILL_VICIOUS_ATTACK, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_RANGER, 56, 100);


  // Berserker Specs
  SKILL_CREATE_WITH_CAT("mangle", SKILL_MANGLE, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_BERSERKER, 51, 70, SPEC_MAULER);

  SKILL_CREATE_WITH_CAT("boiling blood", SKILL_BOILING_BLOOD, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_DEFENSIVE);
  SPEC_SKILL_ADD(CLASS_BERSERKER, 41, 70, SPEC_MAULER);

  SKILL_CREATE_WITH_CAT("grapple", SKILL_GRAPPLE, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_DEFENSIVE);
  SPEC_SKILL_ADD(CLASS_BERSERKER, 51, 70, SPEC_RAGELORD);

  // Necromancer Specs

  SKILL_CREATE("exhume", SKILL_EXHUME, SKILL_TYPE_PHYS_DEX);
  SPEC_SKILL_ADD(CLASS_NECROMANCER, 31, 100, SPEC_DIABOLIS);

  SPELL_CREATE("create golem", SPELL_CREATE_GOLEM, SPELL_TYPE_SUMMONING,
               PULSE_SPELLCAST * 3, FALSE, TAR_OBJ_ROOM, 0,
               spell_create_golem, NULL);
  SPEC_SPELL_ADD(CLASS_NECROMANCER, 9, SPEC_DIABOLIS);

  SPELL_CREATE("raise shadow", SPELL_RAISE_SHADOW, SPELL_TYPE_SUMMONING,
               PULSE_SPELLCAST * 3, FALSE, TAR_OBJ_ROOM, 0,
               spell_raise_shadow, NULL);
  SPEC_SPELL_ADD(CLASS_NECROMANCER, 9, SPEC_NECROLYTE);

  /* end specs */


  SPELL_CREATE("pleasantry", SPELL_PLEASANTRY, 1, PULSE_SPELLCAST,
               TRUE, TAR_CHAR_ROOM, 1, spell_pleasantry, NULL);

  SPELL_CREATE("corpseform", SPELL_CORPSEFORM, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 7, FALSE, TAR_OBJ_ROOM, 0, spell_corpseform,
               "You revert back to the form of the living.");
  SPELL_ADD(CLASS_NECROMANCER, 11);

  SPELL_CREATE("negative concussion blast", SPELL_NEGATIVE_CONCUSSION_BLAST,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST * 3 / 2, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_negative_concussion_blast, NULL);
  //SPELL_ADD(CLASS_NECROMANCER, 7);

  SPELL_CREATE("create greater dracolich", SPELL_CREATE_GREATER_DRACOLICH,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST * 10, FALSE, TAR_OBJ_ROOM,
               0, spell_create_greater_dracolich, NULL);
  SPELL_ADD(CLASS_NECROMANCER, 12);

  SPELL_CREATE("regeneration", SPELL_REGENERATION, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 3 / 2, FALSE, TAR_CHAR_ROOM, 0,
               spell_regeneration, "You stop regenerating.");
  SPELL_ADD(CLASS_DRUID, 6);
  SPEC_SPELL_ADD(CLASS_CLERIC, 8, SPEC_HEALER);
	
  SPELL_CREATE("grow", SPELL_GROW, SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 4,
               FALSE, TAR_IGNORE, 0, cast_grow, NULL);
  SPEC_SPELL_ADD(CLASS_DRUID, 10, SPEC_WOODLAND);

  SPELL_CREATE("depressed earth", SPELL_DEPRESSED_EARTH, SPELLTYPE_SPIRIT, PULSE_SPELLCAST * 4,
               FALSE, TAR_IGNORE, 0, cast_depressed_earth, NULL);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 10, SPEC_SPIRITUALIST);

  SPELL_CREATE("transmute rock to mud", SPELL_TRANS_ROCK_MUD, SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 4,
               FALSE, TAR_IGNORE, 0, cast_transmute_rock_mud, NULL);
  SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_EARTH);

  SPELL_CREATE("transmute mud to rock", SPELL_TRANS_MUD_ROCK, SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 4,
               FALSE, TAR_IGNORE, 0, cast_transmute_mud_rock, NULL);
  SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_EARTH);

  SPELL_CREATE("transmute mud to water", SPELL_TRANS_MUD_WATER, SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 4,
               FALSE, TAR_IGNORE, 0, cast_transmute_mud_water, NULL);
  SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_WATER);

  SPELL_CREATE("transmute water to mud", SPELL_TRANS_WATER_MUD, SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 4,
               FALSE, TAR_IGNORE, 0, cast_transmute_water_mud, NULL);
  SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_WATER);

  SPELL_CREATE("transmute water to air", SPELL_TRANS_WATER_AIR, SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 4,
               FALSE, TAR_IGNORE, 0, cast_transmute_water_air, NULL);
  SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_AIR);

  SPELL_CREATE("transmute air to water", SPELL_TRANS_AIR_WATER, SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 4,
               FALSE, TAR_IGNORE, 0, cast_transmute_air_water, NULL);
  SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_AIR);

  SPELL_CREATE("transmute rock to lava", SPELL_TRANS_ROCK_LAVA, SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 4,
               FALSE, TAR_IGNORE, 0, cast_transmute_rock_lava, NULL);
  SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_FIRE);

  SPELL_CREATE("transmute lava to rock", SPELL_TRANS_LAVA_ROCK, SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 4,
               FALSE, TAR_IGNORE, 0, cast_transmute_lava_rock, NULL);
  SPEC_SPELL_ADD(CLASS_CONJURER, 10, SPEC_FIRE);
	
  SPELL_CREATE("vines", SPELL_VINES, SPELLTYPE_PROTECTION,
               PULSE_SPELLCAST * 3, TRUE, TAR_SELF_ONLY, 0, cast_vines, NULL);

  SPEC_SPELL_ADD(CLASS_DRUID, 9, SPEC_WOODLAND);

  SPELL_CREATE("awaken forest", SPELL_AWAKEN_FOREST, SPELLTYPE_SUMMONING,
               PULSE_SPELLCAST * 5, TRUE, TAR_AREA, 1, cast_awaken_forest,
               NULL);

  SPEC_SPELL_ADD(CLASS_DRUID, 11, SPEC_WOODLAND);

	
  SPELL_CREATE("spike growth", SPELL_SPIKE_GROWTH, SPELLTYPE_SUMMONING,
               PULSE_SPELLCAST * 5, TRUE, TAR_AREA, 1, cast_spike_growth,
               NULL);
	SPEC_SPELL_ADD(CLASS_DRUID, 11, SPEC_STORM);
	
  SPELL_CREATE("hurricane", SPELL_HURRICANE, SPELLTYPE_SUMMONING,
               PULSE_SPELLCAST * 5, TRUE, TAR_AREA, 1, cast_hurricane, NULL);

  SPEC_SPELL_ADD(CLASS_DRUID, 10, SPEC_STORM);

  SPELL_CREATE("storm shield", SPELL_STORMSHIELD, SPELLTYPE_PROTECTION,
               PULSE_SPELLCAST * 5, TRUE, TAR_SELF_ONLY, 0, cast_storm_shield,
               "Your shield is no longer infused with pure energy.");

  SPEC_SPELL_ADD(CLASS_DRUID, 9, SPEC_STORM);

  SPELL_CREATE("blood to stone", SPELL_BLOODSTONE, SPELLTYPE_SUMMONING,
               PULSE_SPELLCAST * 1, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               cast_bloodstone, "Your blood flows normally again.");

  SPEC_SPELL_ADD(CLASS_DRUID, 11, SPEC_STORM);

  SPELL_CREATE("accelerated healing", SPELL_ACCEL_HEALING, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_CHAR_ROOM, 0,
               spell_accel_healing, "You no longer regenerate.");
  SPELL_ADD(CLASS_CLERIC, 10);
  SPELL_ADD(CLASS_PALADIN, 11);

  SPELL_CREATE("endurance", SPELL_ENDURANCE, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_CHAR_ROOM, 0,
               spell_endurance, "Your endurance fades away.");
  SPELL_ADD(CLASS_DRUID, 6);
	SPEC_SPELL_ADD(CLASS_RANGER, 8, SPEC_WOODSMAN);
  SPEC_SPELL_ADD(CLASS_CLERIC, 7, SPEC_HEALER);
	
  SPELL_CREATE("fortitude", SPELL_FORTITUDE, SPELLTYPE_PROTECTION,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0, spell_fortitude,
               "You feel less determined to go on with life.");
  SPELL_ADD(CLASS_DRUID, 1);
	SPEC_SPELL_ADD(CLASS_RANGER, 7, SPEC_BLADEMASTER);

  SPELL_CREATE("armor", SPELL_ARMOR, SPELLTYPE_PROTECTION, PULSE_SPELLCAST,
               TRUE, TAR_CHAR_ROOM, 0, spell_armor,
               "You feel less &+Wprotected&n.");
  SPELL_ADD(CLASS_CLERIC, 1);
  SPELL_ADD(CLASS_PALADIN, 3);
  SPELL_ADD(CLASS_ANTIPALADIN, 3);


  SPELL_CREATE("virtue", SPELL_VIRTUE, SPELLTYPE_PROTECTION, PULSE_SPELLCAST,
               TRUE, TAR_CHAR_ROOM, 0, spell_virtue,
               "You no longer feel the call of the &+Wgods&n.");
  SPEC_SPELL_ADD(CLASS_CLERIC, 8, SPEC_HOLYMAN );
	SPEC_SPELL_ADD(CLASS_CLERIC, 7, SPEC_ZEALOT);
	
  SPELL_CREATE("group teleport", SPELL_GROUP_TELEPORT,
               SPELLTYPE_TELEPORTATION, PULSE_SPELLCAST, TRUE, TAR_IGNORE, 0,
               spell_group_teleport, NULL);

  SPELL_CREATE("shatter", SPELL_SHATTER, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 1, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_shatter, NULL);

  SPEC_SPELL_ADD(CLASS_BARD, 9, SPEC_DISHARMONIST);
  SPEC_SPELL_ADD(CLASS_SPIPER, 9, SPEC_DISHARMONIST);

  SPELL_CREATE("enrage", SPELL_ENRAGE, SPELLTYPE_PSIONIC,
               1, TRUE, TAR_CHAR_ROOM + TAR_FIGHT_VICT, 1,
               spell_enrage, NULL);
  SPELL_ADD(CLASS_MINDFLAYER, 9);
  SPELL_ADD(CLASS_PSIONICIST, 12);

  SPELL_CREATE("detonate", SPELL_DETONATE, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM + TAR_FIGHT_VICT, 1,
               spell_detonate, NULL);
  SPELL_ADD(CLASS_PSIONICIST, 7);
  SPELL_ADD(CLASS_MINDFLAYER, 6);
  
  SPELL_CREATE("fire aura", SPELL_FIRE_AURA, SPELL_TYPE_GENERIC,
               PULSE_SPELLCAST * 4, FALSE, TAR_SELF_ONLY, 0,
               spell_fire_aura, "&+BYour &+rfiery aura &+Breturns to normal.&N");
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 12, SPEC_PYROKINETIC);  

  SPELL_CREATE("ether warp", SPELL_ETHER_WARP, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3, FALSE, TAR_CHAR_WORLD, 0,
               spell_ether_warp, NULL);
  SPELL_ADD(CLASS_PSIONICIST, 9);
  SPELL_ADD(CLASS_MINDFLAYER, 7);

  SPELL_CREATE("ethereal rift", SPELL_ETHEREAL_RIFT, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, FALSE, TAR_CHAR_WORLD, 0,
               spell_ethereal_rift, NULL);
//  SPELL_ADD(CLASS_MINDFLAYER, 10);

  SPELL_CREATE("wormhole", SPELL_WORMHOLE,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST * 4, FALSE, TAR_CHAR_WORLD,
               0, spell_wormhole, NULL);
  SPELL_ADD(CLASS_PSIONICIST, 10);
  SPELL_ADD(CLASS_MINDFLAYER, 9);

  SPELL_CREATE("thought beacon", SPELL_THOUGHT_BEACON,
      SPELLTYPE_GENERIC, PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE, 0,
      spell_thought_beacon, NULL);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 12, SPEC_PSYCHEPORTER);

  SPELL_CREATE("radial navigation", SPELL_RADIAL_NAV,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST * 2, FALSE, TAR_IGNORE, 0,
               spell_radial_navigation, NULL);

  SPELL_CREATE("molecular control", SPELL_MOLECULAR_CONTROL,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST, FALSE, TAR_SELF_ONLY, 0,
               spell_molecular_control,
               "You lose control of your molecules.");
  SPELL_ADD(CLASS_MINDFLAYER, 4);
  SPELL_ADD(CLASS_PSIONICIST, 8);

  SPELL_CREATE("molecular agitation", SPELL_MOLECULAR_AGITATION,
               SPELLTYPE_PSIONIC, PULSE_SPELLCAST, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1, spell_molecular_agitation,
               NULL);
  SPELL_ADD(CLASS_PSIONICIST, 3);
  SPELL_ADD(CLASS_MINDFLAYER, 3);

  SPELL_CREATE("spinal corruption", SPELL_SPINAL_CORRUPTION,
               SPELLTYPE_PSIONIC, PULSE_SPELLCAST, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1, spell_spinal_corruption,
               NULL);
  SPELL_ADD(CLASS_MINDFLAYER, 12);

  SPELL_CREATE("adrenaline control", SPELL_ADRENALINE_CONTROL,
               SPELLTYPE_PSIONIC, PULSE_SPELLCAST, TRUE, TAR_SELF_ONLY, 0,
               spell_adrenaline_control,
               "&+rYour rush of adrenaline runs dry.&n");
  SPELL_ADD(CLASS_PSIONICIST, 1);
  SPELL_ADD(CLASS_MINDFLAYER, 1);

  SPELL_CREATE("aura sight", SPELL_AURA_SIGHT, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST, TRUE, TAR_SELF_ONLY, 0, spell_aura_sight,
               "&+CThe auras in your vision disappear.&n");
  SPELL_ADD(CLASS_PSIONICIST, 3);
  SPELL_ADD(CLASS_MINDFLAYER, 3);

  SPELL_CREATE("awe", SPELL_AWE, SPELLTYPE_PSIONIC, PULSE_SPELLCAST, TRUE,
               TAR_IGNORE, 1, spell_awe, "You regain your senses.");
  SPELL_ADD(CLASS_MINDFLAYER, 4);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 6, SPEC_ENSLAVER);

  SPELL_CREATE("ballistic attack", SPELL_BALLISTIC_ATTACK, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE, 1,
               spell_ballistic_attack, NULL);
  SPELL_ADD(CLASS_PSIONICIST, 2);
  SPELL_ADD(CLASS_MINDFLAYER, 2);

  SPELL_CREATE("mental anguish", SPELL_MENTAL_ANGUISH, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE, 1,
               spell_mental_anguish, NULL);
  SPELL_ADD(CLASS_MINDFLAYER, 8);

  SPELL_CREATE("memory block", SPELL_MEMORY_BLOCK, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE, 1,
               spell_memory_block, NULL);
  SPELL_ADD(CLASS_MINDFLAYER, 9);

  SPELL_CREATE("biofeedback", SPELL_BIOFEEDBACK, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST, TRUE, TAR_SELF_ONLY, 0, spell_biofeedback,
               "&+GThe green mist around your body fades.&n");
  SPELL_ADD(CLASS_MINDFLAYER, 7);
  SPELL_ADD(CLASS_PSIONICIST, 11);

  SPELL_CREATE("cell adjustment", SPELL_CELL_ADJUSTMENT, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST, TRUE, TAR_SELF_ONLY, 0, spell_cell_adjustment,
               NULL);
  SPELL_ADD(CLASS_MINDFLAYER, 2);
  SPELL_ADD(CLASS_PSIONICIST, 6);

  SPELL_CREATE("combat mind", SPELL_COMBAT_MIND, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST, TRUE, TAR_SELF_ONLY, 0, spell_combat_mind,
               "You forget your battle tactics.");
  SPELL_ADD(CLASS_PSIONICIST, 1);
  SPELL_ADD(CLASS_MINDFLAYER, 1);

/* what the hell is this? -Zod */
/* a fun joke! -Tavril */
  SPELL_CREATE("area headbutt", SPELL_CREATE_EARTHEN_PROJ, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST, TRUE, TAR_SELF_ONLY, 0, spell_ego_blast,
               NULL);
  //SPELL_ADD(CLASS_PSIONICIST, 12);

  SPELL_CREATE("ego blast", SPELL_EGO_BLAST, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_ego_blast, NULL);
  SPELL_ADD(CLASS_PSIONICIST, 4);

  SPELL_CREATE("control flames", SPELL_CONTROL_FLAMES, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_control_flames, NULL);
 


  SPELL_CREATE("create sound", SPELL_CREATE_SOUND, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST, TRUE, TAR_IGNORE, 0, spell_create_sound,
               NULL);
  SPELL_ADD(CLASS_PSIONICIST, 6);
  SPELL_ADD(CLASS_MINDFLAYER, 2);

  SPELL_CREATE("death field", SPELL_DEATH_FIELD, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_AREA | TAR_OFFAREA, 2,
               spell_death_field, NULL);
  SPELL_ADD(CLASS_PSIONICIST, 10);
  SPELL_ADD(CLASS_MINDFLAYER, 10);

  SPELL_CREATE("psionic cloud", SPELL_PSIONIC_CLOUD, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST, TRUE, TAR_OFFAREA | TAR_AREA, 2,
               spell_psionic_cloud, NULL);
  SPELL_ADD(CLASS_MINDFLAYER, 10);
  SPELL_CREATE("ectoplasmic form", SPELL_ECTOPLASMIC_FORM, 1,
               PULSE_SPELLCAST, TRUE, TAR_SELF_ONLY, 1,
               spell_ectoplasmic_form, NULL);
  SPELL_ADD(CLASS_PSIONICIST, 8);
  SPELL_ADD(CLASS_MINDFLAYER, 6);

  SPELL_CREATE("ego whip", SPELL_EGO_WHIP, SPELLTYPE_PSIONIC, PULSE_SPELLCAST,
               TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE, 1,
               spell_ego_whip, NULL);
  SPELL_ADD(CLASS_PSIONICIST, 1);
  SPELL_ADD(CLASS_MINDFLAYER, 1);

  SPELL_CREATE("energy containment", SPELL_ENERGY_CONTAINMENT, 1,
               PULSE_SPELLCAST, TRUE, TAR_SELF_ONLY, 0,
               spell_energy_containment,
               "&+YYou can no longer absorb energy.&n");
  SPELL_ADD(CLASS_PSIONICIST, 4);
  SPELL_ADD(CLASS_MINDFLAYER, 4);

  SPELL_CREATE("enhance armor", SPELL_ENHANCE_ARMOR, 1, PULSE_SPELLCAST,
               TRUE, TAR_SELF_ONLY, 0, spell_enhance_armor,
               "&+BThe psionic armor around you fades.&n");
  SPELL_ADD(CLASS_MINDFLAYER, 6);

  SPELL_CREATE("enhance strength", SPELL_ENHANCED_STR, 1, PULSE_SPELLCAST,
               FALSE, TAR_SELF_ONLY, 0, spell_enhanced_strength,
               "&+CYour muscles return to normal.&n");
  SPELL_ADD(CLASS_PSIONICIST, 2);
  SPELL_ADD(CLASS_MINDFLAYER, 2);

  SPELL_CREATE("enhance dexterity", SPELL_ENHANCED_DEX, 1,
               PULSE_SPELLCAST, FALSE, TAR_SELF_ONLY, 0,
               spell_enhanced_dexterity, "&+yYour coordination decreases.&n");
  SPELL_ADD(CLASS_PSIONICIST, 2);
  SPELL_ADD(CLASS_MINDFLAYER, 2);

  SPELL_CREATE("enhance agility", SPELL_ENHANCED_AGI, 1, PULSE_SPELLCAST,
               FALSE, TAR_SELF_ONLY, 0, spell_enhanced_agility,
               "&+CYou feel a bit more clumsy.&n");
  SPELL_ADD(CLASS_PSIONICIST, 2);
  SPELL_ADD(CLASS_MINDFLAYER, 2);

  SPELL_CREATE("enhance constitution", SPELL_ENHANCED_CON, 1,
               PULSE_SPELLCAST, FALSE, TAR_SELF_ONLY, 0,
               spell_enhanced_constitution,
               "&+YYour vitality drains away.&n");
  SPELL_ADD(CLASS_PSIONICIST, 2);
  SPELL_ADD(CLASS_MINDFLAYER, 2);

  SPELL_CREATE("flesh armor", SPELL_FLESH_ARMOR, 1, PULSE_SPELLCAST,
               TRUE, TAR_SELF_ONLY, 0, spell_flesh_armor,
               "&+rYour flesh softens.&n");
  SPELL_ADD(CLASS_PSIONICIST, 4);
  SPELL_ADD(CLASS_MINDFLAYER, 5);

  SPELL_CREATE("inertial barrier", SPELL_INERTIAL_BARRIER, 1,
               PULSE_SPELLCAST, TRUE, TAR_SELF_ONLY, 0,
               spell_inertial_barrier,
               "Your &+Winertial barrier&n fades away.");
  SPELL_ADD(CLASS_MINDFLAYER, 7);
  SPELL_ADD(CLASS_PSIONICIST, 8);

  SPELL_CREATE("inflict pain", SPELL_INFLICT_PAIN, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_inflict_pain, NULL);
  SPELL_ADD(CLASS_PSIONICIST, 5);
  SPELL_ADD(CLASS_MINDFLAYER, 5);

  SPELL_CREATE("intellect fortress", SPELL_INTELLECT_FORTRESS, 1,
               PULSE_SPELLCAST, TRUE, TAR_SELF_ONLY, 0,
               spell_intellect_fortress,
               "&+yYour virtual fortress crumbles.&n");
  SPELL_ADD(CLASS_PSIONICIST, 6);
  SPELL_ADD(CLASS_MINDFLAYER, 5);

  SPELL_CREATE("lend health", SPELL_LEND_HEALTH, 1, PULSE_SPELLCAST,
               FALSE, TAR_CHAR_ROOM, 0, spell_lend_health, NULL);
  SPELL_ADD(CLASS_PSIONICIST, 3);  
  SPELL_ADD(CLASS_MINDFLAYER, 3);
  

  SPELL_CREATE("flight", SPELL_POWERCAST_FLY, 1, PULSE_SPELLCAST, TRUE,
               TAR_SELF_ONLY, 0, spell_fly,
               "&+CYour feet slowly descend to the ground.&n");
  SPELL_ADD(CLASS_PSIONICIST, 6);
  SPELL_ADD(CLASS_MINDFLAYER, 5);

  SPELL_CREATE("confuse", SPELL_CONFUSE, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_AREA, 1, spell_confuse, NULL);
  SPELL_ADD(CLASS_MINDFLAYER, 9);
  SPEC_SPELL_ADD(CLASS_PSIONICIST, 12, 1);


  SPELL_CREATE("mind blank", SPELL_MIND_BLANK, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST, TRUE, TAR_SELF_ONLY, 0, spell_mind_blank,
               "&+YYour mind is not as concealed anymore!&N");
  SPELL_ADD(CLASS_MINDFLAYER, 12);

  SPELL_CREATE("psychic crush", SPELL_PSYCHIC_CRUSH, SPELLTYPE_PSIONIC,
               (int) (PULSE_SPELLCAST * 1.5), TRUE,
               TAR_CHAR_ROOM + TAR_FIGHT_VICT, 1, spell_psychic_crush, NULL);
  SPELL_ADD(CLASS_PSIONICIST, 9);


  SPELL_CREATE("cannibalize", SPELL_CANIBALIZE, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST, TRUE, TAR_SELF_ONLY, 0, spell_canibalize,
               "&+YYou can no longer drain your victim!&N");
  SPELL_ADD(CLASS_PSIONICIST, 8);
  SPELL_ADD(CLASS_MINDFLAYER, 7);

/* Removed
  SPELL_CREATE("sight link", SPELL_SIGHT_LINK, SPELLTYPE_DIVINATION,
           PULSE_SPELLCAST * 2, FALSE, TAR_CHAR_WORLD | TAR_SELF_NONO, 0,
               spell_sight_link, NULL);
  SPELL_ADD(CLASS_MINDFLAYER, 11);
*/
  SPELL_CREATE("mind travel", SPELL_MIND_TRAVEL, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST * 4, FALSE, TAR_SELF_ONLY, 0,
               spell_mind_travel, NULL);
  SPELL_ADD(CLASS_MINDFLAYER, 11);

  SPELL_CREATE("tower of iron will", SPELL_TOWER_IRON_WILL, SPELLTYPE_PSIONIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_SELF_ONLY, 0,
               spell_tower_iron_will,
               "&+YYou just lost your tower of iron will!&N");
  SPELL_ADD(CLASS_PSIONICIST, 11);
  SPELL_ADD(CLASS_MINDFLAYER, 8);

/***** End of psionics *****/

  SPELL_CREATE("magic missile", SPELL_MAGIC_MISSILE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2 / 3, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_RANGE2,
               1, spell_magic_missile, NULL);
  SPELL_ADD(CLASS_SORCERER, 1);
  SPELL_ADD(CLASS_CONJURER, 1);
  SPELL_ADD(CLASS_RANGER, 3);
  SPELL_ADD(CLASS_NECROMANCER, 1);
  SPELL_ADD(CLASS_BARD, 3);
  SPELL_ADD(CLASS_ILLUSIONIST, 1);
  SPELL_ADD(CLASS_SPIPER, 3);

  SPELL_CREATE("chill touch", SPELL_CHILL_TOUCH, SPELLTYPE_COLD,
               PULSE_SPELLCAST * 3 / 4, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_chill_touch, "You feel less &+Bchilled&n.");
  SPELL_ADD(CLASS_NECROMANCER, 2);
  SPELL_ADD(CLASS_CONJURER, 2);
  SPELL_ADD(CLASS_SORCERER, 2);
  SPELL_ADD(CLASS_RANGER, 5);
  SPELL_ADD(CLASS_REAVER, 5);
  SPELL_ADD(CLASS_ETHERMANCER, 2);

  SPELL_CREATE("burning hands", SPELL_BURNING_HANDS, SPELLTYPE_FIRE,
               PULSE_SPELLCAST - 1, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_burning_hands, NULL);
  SPELL_ADD(CLASS_SORCERER, 2);
  SPELL_ADD(CLASS_CONJURER, 2);
/*  SPELL_ADD(CLASS_NECROMANCER, 2);*/
  SPELL_ADD(CLASS_BARD, 4);
  SPELL_ADD(CLASS_ILLUSIONIST, 2);
  SPELL_ADD(CLASS_SPIPER, 4);
  SPEC_SPELL_ADD(CLASS_CLERIC, 8, SPEC_ZEALOT); 
	
  SPELL_CREATE("shocking grasp", SPELL_SHOCKING_GRASP, SPELLTYPE_ELECTRIC,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_shocking_grasp, NULL);
  SPELL_ADD(CLASS_SORCERER, 3);
  SPELL_ADD(CLASS_CONJURER, 3);

  SPELL_CREATE("frostbite", SPELL_FROSTBITE, SPELLTYPE_COLD,
               PULSE_SPELLCAST + 1, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_RANGE2,
               1, spell_frostbite, NULL);
  SPELL_ADD(CLASS_WARLOCK, 6);
  SPELL_ADD(CLASS_ETHERMANCER, 6);
  SPELL_ADD(CLASS_NECROMANCER, 7);

  SPELL_CREATE("lightning bolt", SPELL_LIGHTNING_BOLT, SPELLTYPE_ELECTRIC,
               PULSE_SPELLCAST + 1, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_RANGE2,
               1, spell_lightning_bolt, NULL);
  SPELL_ADD(CLASS_SORCERER, 4);
  SPELL_ADD(CLASS_CONJURER, 4);
  SPELL_ADD(CLASS_DRUID, 4);
  SPELL_ADD(CLASS_RANGER, 7);
  SPELL_ADD(CLASS_BARD, 6);
  SPELL_ADD(CLASS_REAVER, 7);
  SPELL_ADD(CLASS_SPIPER, 6);

  SPELL_CREATE("cone of cold", SPELL_CONE_OF_COLD, SPELLTYPE_COLD,
               PULSE_SPELLCAST * 4 / 3, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE, 1,
               spell_cone_of_cold, NULL);
  SPELL_ADD(CLASS_SORCERER, 5);
  SPELL_ADD(CLASS_NECROMANCER, 6);
  SPELL_ADD(CLASS_CONJURER, 7);
  SPELL_ADD(CLASS_ETHERMANCER, 4);

  SPELL_CREATE("energy drain", SPELL_ENERGY_DRAIN, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 1, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_energy_drain, NULL);
  SPELL_ADD(CLASS_NECROMANCER, 6);

  SPELL_CREATE("wither", SPELL_WITHER, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_wither, "You feel less &+Lwithered&n.");
  SPELL_ADD(CLASS_ANTIPALADIN, 8);
  SPELL_ADD(CLASS_NECROMANCER, 4);

  SPELL_CREATE("living stone", SPELL_LIVING_STONE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 4 / 3, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_RANGE2,
               1, spell_living_stone, NULL);

  SPELL_CREATE("greater living stone", SPELL_GREATER_LIVING_STONE,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_RANGE2,
               1, spell_greater_living_stone, NULL);

  SPELL_CREATE("fireball", SPELL_FIREBALL, SPELLTYPE_FIRE,
               PULSE_SPELLCAST * 4 / 3, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_RANGE2,
               1, spell_fireball, NULL);
  SPELL_ADD(CLASS_SORCERER, 6);
  SPELL_ADD(CLASS_CONJURER, 7);
  SPELL_ADD(CLASS_BARD, 7);
  SPELL_ADD(CLASS_REAVER, 8);
  SPELL_ADD(CLASS_SPIPER, 7);

  SPELL_CREATE("meteorswarm", SPELL_METEOR_SWARM, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3, TRUE, TAR_AREA + TAR_OFFAREA, 1,
               spell_meteorswarm, NULL);
  SPELL_ADD(CLASS_SORCERER, 10);


  SPELL_CREATE("entropy storm", SPELL_ENTROPY_STORM, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 2, TRUE, TAR_IGNORE, 1, spell_entropy_storm,
               NULL);
  SPELL_ADD(CLASS_WARLOCK, 11);

  SPELL_CREATE("chain lightning", SPELL_CHAIN_LIGHTNING,
               SPELLTYPE_ELECTRIC, PULSE_SPELLCAST * 2, TRUE,
               TAR_AREA + TAR_OFFAREA /*| TAR_CHAR_RANGE */ , 1,
               spell_chain_lightning, NULL);
  SPELL_ADD(CLASS_SORCERER, 10);

  SPELL_CREATE("lightning ring", SPELL_RING_LIGHTNING,
               SPELLTYPE_ELECTRIC, (int) (PULSE_SPELLCAST * 2.5), TRUE,
               TAR_AREA + TAR_OFFAREA /*| TAR_CHAR_RANGE */ , 1,
               spell_ring_lightning, NULL);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 9, SPEC_FROST_MAGUS);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 9, SPEC_WINDTALKER);

  SPELL_CREATE("bigbys clenched fist", SPELL_BIGBYS_CLENCHED_FIST,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST * 3 / 2, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1, spell_bigbys_clenched_fist,
               NULL);
  SPELL_ADD(CLASS_SORCERER, 7);
  SPELL_ADD(CLASS_BARD, 8);
  SPELL_ADD(CLASS_SPIPER, 8);

  SPELL_CREATE("anti-magic ray", SPELL_ANTI_MAGIC_RAY, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_anti_magic_ray, NULL);

/*
  SPELL_CREATE("bigbys grasping fist", SPELL_BIGBYS_GRASPING_FIST,
          SPELLTYPE_GENERIC, PULSE_SPELLCAST * 3 / 2, TRUE, TAR_CHAR_ROOM |
          TAR_FIGHT_VICT, 1, cast_bigbys_grasping_fist, NULL);
  SPELL_ADD(CLASS_SORCERER, 7);
*/

  SPELL_CREATE("bigbys crushing hand", SPELL_BIGBYS_CRUSHING_HAND,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM |
               TAR_FIGHT_VICT, 1, spell_bigbys_crushing_hand, NULL);
  SPELL_ADD(CLASS_SORCERER, 11);

  SPELL_CREATE("dimension door", SPELL_DIMENSION_DOOR,
               SPELLTYPE_TELEPORTATION, PULSE_SPELLCAST * 3 / 2, FALSE,
               TAR_CHAR_WORLD | TAR_SELF_NONO, 0, spell_dimension_door, NULL);
  SPELL_ADD(CLASS_SORCERER, 5);
  SPELL_ADD(CLASS_CONJURER, 7);
  SPELL_ADD(CLASS_NECROMANCER, 9);
  SPELL_ADD(CLASS_BARD, 7);
  SPELL_ADD(CLASS_SPIPER, 7);

  SPELL_CREATE("dark compact", SPELL_DARK_COMPACT, SPELLTYPE_TELEPORTATION,
               PULSE_SPELLCAST, 0, TAR_SELF_ONLY, 0, spell_dark_compact,
               NULL);
  SPELL_ADD(CLASS_NECROMANCER, 9);

  SPELL_CREATE("relocate", SPELL_RELOCATE, SPELLTYPE_TELEPORTATION,
               PULSE_SPELLCAST * 5, FALSE, TAR_CHAR_WORLD | TAR_SELF_NONO, 0,
               spell_relocate, NULL);
  SPELL_ADD(CLASS_SORCERER, 9);
  SPELL_ADD(CLASS_CONJURER, 9);

  SPELL_CREATE("wizard eye", SPELL_WIZARD_EYE, SPELLTYPE_DIVINATION,
               PULSE_SPELLCAST, FALSE, TAR_CHAR_WORLD | TAR_SELF_NONO, 0,
               spell_wizard_eye, "Your magical eyesight dissipates.");
  SPELL_ADD(CLASS_SORCERER, 4);

  SPELL_CREATE("clairvoyance", SPELL_CLAIRVOYANCE, SPELLTYPE_DIVINATION,
               PULSE_SPELLCAST * 2, FALSE, TAR_CHAR_WORLD | TAR_SELF_NONO, 0,
               spell_clairvoyance, "Your clairvoyant vision dissipates.");
  SPELL_ADD(CLASS_SORCERER, 100);
  SPELL_ADD(CLASS_CONJURER, 100);
  SPELL_ADD(CLASS_BARD, 100);
  SPELL_ADD(CLASS_SPIPER, 100);

  SPELL_CREATE("rejuvenate major", SPELL_REJUVENATE_MAJOR,
               SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 3, FALSE,
               TAR_CHAR_ROOM, 0, spell_rejuvenate_major,
               "You feel &+Lolder&n.");
  SPELL_ADD(CLASS_NECROMANCER, 6);

  SPELL_CREATE("age", SPELL_AGE, SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 2,
               FALSE, TAR_CHAR_ROOM, 0, spell_age, NULL);
  SPELL_ADD(CLASS_NECROMANCER, 9);

  SPELL_CREATE("rejuvenate minor", SPELL_REJUVENATE_MINOR,
               SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 2, FALSE,
               TAR_CHAR_ROOM, 0, spell_rejuvenate_minor,
               "you feel &+Lolder&n.");
  SPELL_ADD(CLASS_NECROMANCER, 4);

  SPELL_CREATE("ray of enfeeblement", SPELL_RAY_OF_ENFEEBLEMENT,
               SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1, spell_ray_of_enfeeblement,
               "You feel less feeble.");
//  SPELL_ADD(CLASS_NECROMANCER, 5);
  SPELL_ADD(CLASS_SORCERER, 4);
  SPELL_ADD(CLASS_CONJURER, 4);
  SPELL_ADD(CLASS_BARD, 6);
  SPELL_ADD(CLASS_REAVER, 6);
  SPELL_ADD(CLASS_SPIPER, 6);

  SPELL_CREATE("earthquake", SPELL_EARTHQUAKE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_IGNORE, 1, spell_earthquake,
               NULL);
  SPELL_ADD(CLASS_CLERIC, 3);
  SPELL_ADD(CLASS_DRUID, 5);

  SPELL_CREATE("call woodland beings", SPELL_CALL_WOODLAND,  SPELLTYPE_SUMMONING,
				                PULSE_SPELLCAST * 3, FALSE, TAR_IGNORE, 0, spell_call_woodland_beings,
											               NULL);
  SPEC_SPELL_ADD(CLASS_DRUID, 7, SPEC_WOODLAND);

	
  SPELL_CREATE("earthen maul", SPELL_EARTHEN_MAUL, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_earthen_maul, NULL);
  SPELL_ADD(CLASS_DRUID, 6);

  SPELL_CREATE("earth spike", SPELL_GROW_SPIKES, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_grow_spike, NULL);
  SPEC_SPELL_ADD(CLASS_DRUID, 7, SPEC_STORM);
  
	SPELL_CREATE("dispel evil", SPELL_DISPEL_EVIL, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST + 1, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_dispel_evil, NULL);
  SPELL_ADD(CLASS_CLERIC, 3);
//  SPELL_ADD(CLASS_DRUID, 3);
  SPELL_ADD(CLASS_PALADIN, 4);
  SPELL_ADD(CLASS_RANGER, 5);

  SPELL_CREATE("dispel good", SPELL_DISPEL_GOOD, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST + 1, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_dispel_good, NULL);
  SPELL_ADD(CLASS_CLERIC, 3);
//  SPELL_ADD(CLASS_DRUID, 3);
  SPELL_ADD(CLASS_ANTIPALADIN, 4);
  SPELL_ADD(CLASS_WARLOCK, 4);

  SPELL_CREATE("call lightning", SPELL_CALL_LIGHTNING, SPELLTYPE_ELECTRIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_AREA | TAR_OFFAREA | TAR_FIGHT_VICT , 1,
               cast_call_lightning, NULL);
  SPEC_SPELL_ADD(CLASS_DRUID, 7, SPEC_STORM);
//  SPELL_ADD(CLASS_RANGER, 8);
 
	
  SPELL_CREATE("harm", SPELL_HARM, SPELLTYPE_GENERIC, PULSE_SPELLCAST * 3
               / 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1, spell_harm,
               NULL);
  SPELL_ADD(CLASS_CLERIC, 6);
//  SPELL_ADD(CLASS_DRUID, 5);
  SPELL_ADD(CLASS_ANTIPALADIN, 9);

  SPELL_CREATE("full harm", SPELL_FULL_HARM, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_full_harm, NULL);
  SPELL_ADD(CLASS_CLERIC, 10);

  SPELL_CREATE("destroy undead", SPELL_DESTROY_UNDEAD, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_destroy_undead, NULL);
  SPELL_ADD(CLASS_CLERIC, 5);
  SPELL_ADD(CLASS_PALADIN, 7);
  SPEC_SPELL_ADD(CLASS_NECROMANCER, 9, SPEC_REAPER);

  SPELL_CREATE("firestorm", SPELL_FIRESTORM, SPELLTYPE_FIRE,
               PULSE_SPELLCAST * 2, TRUE,
               TAR_AREA | TAR_OFFAREA | TAR_FIGHT_VICT, 1, spell_firestorm,
               NULL);
//  SPELL_ADD(CLASS_DRUID, 6);
	SPEC_SPELL_ADD(CLASS_DRUID, 9, SPEC_STORM);
  SPELL_ADD(CLASS_RANGER, 10);
  SPELL_ADD(CLASS_REAVER, 10);

  SPELL_CREATE("flame strike", SPELL_FLAMESTRIKE, SPELLTYPE_FIRE,
               PULSE_SPELLCAST + 1, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE, 1,
               spell_flamestrike, NULL);
  SPELL_ADD(CLASS_CLERIC, 6);

  SPELL_CREATE("teleport", SPELL_TELEPORT, SPELLTYPE_TELEPORTATION,
               PULSE_SPELLCAST, 1, TAR_SELF_ONLY, 0, spell_teleport, NULL);
  SPELL_ADD(CLASS_SORCERER, 6);
  SPELL_ADD(CLASS_NECROMANCER, 5);
  SPELL_ADD(CLASS_CONJURER, 6);
  SPELL_ADD(CLASS_MINDFLAYER, 4);
  /*SPELL_ADD(CLASS_REAVER, 9); */

  SPELL_CREATE("bless", SPELL_BLESS, SPELLTYPE_PROTECTION,
               PULSE_SPELLCAST, FALSE,
               TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_CHAR_ROOM, 0, spell_bless,
               "You feel less &+Wrighteous&n.");
  SPELL_ADD(CLASS_CLERIC, 1);
//  SPELL_ADD(CLASS_DRUID, 2);
  SPELL_ADD(CLASS_PALADIN, 2);
  SPELL_ADD(CLASS_RANGER, 3);

  SPELL_CREATE("blindness", SPELL_BLINDNESS, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_blindness,
               "You feel a &+Lcloak &nof &+Lblindness &ndissolve.");
  SPELL_ADD(CLASS_CLERIC, 2);
  SPELL_ADD(CLASS_ANTIPALADIN, 6);
  SPELL_ADD(CLASS_ILLUSIONIST, 3);
  SPELL_ADD(CLASS_WARLOCK, 3);

  SPELL_CREATE("minor globe of invulnerability", SPELL_MINOR_GLOBE,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM,
               0, spell_minor_globe,
               "Your minor globe shimmers and fades into thin air.");
  SPELL_ADD(CLASS_NECROMANCER, 6);
  SPELL_ADD(CLASS_SORCERER, 5);
  SPELL_ADD(CLASS_CONJURER, 6);


  SPELL_CREATE("deflect", SPELL_DEFLECT,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST * 4, TRUE, TAR_SELF_ONLY,
               0, spell_deflect, "Your sense your magical barrier fade.");
  SPELL_ADD(CLASS_SORCERER, 12);

  SPELL_CREATE("charm person", SPELL_CHARM_PERSON, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_SELF_NONO, 1,
               spell_charm_person, "You regain your senses.");
#ifndef NEW_CODE
  SPELL_ADD(CLASS_SORCERER, 5);
  SPELL_ADD(CLASS_BARD, 7);
  SPELL_ADD(CLASS_SPIPER, 7);
#endif

  SPELL_CREATE("infravision", SPELL_INFRAVISION, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 2, FALSE, TAR_CHAR_ROOM, 0,
               spell_infravision, "Your &+rinfravision&n dissipates.");
  SPELL_ADD(CLASS_SORCERER, 4);
  SPELL_ADD(CLASS_CONJURER, 4);
  SPELL_ADD(CLASS_RANGER, 7);
  SPELL_ADD(CLASS_BARD, 6);
  SPELL_ADD(CLASS_ILLUSIONIST, 4);
  SPELL_ADD(CLASS_SPIPER, 6);

  //SPELL_CREATE("creeping doom", SPELL_CREEPING, SPELLTYPE_SUMMONING,
  //        PULSE_SPELLCAST * 2, TRUE, TAR_AREA | TAR_OFFAREA, 1, spell_creeping, NULL);
  //SPELL_ADD(CLASS_DRUID, 10);

  SPELL_CREATE("harmonic resonance", SPELL_HARMONIC_RESONANCE,
               SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 2, TRUE,
               TAR_AREA | TAR_OFFAREA, 1, spell_harmonic_resonance, NULL);
  //SPELL_ADD(CLASS_DRUID, 11);

  SPELL_CREATE("nova", SPELL_NOVA, SPELLTYPE_SUMMONING,
               PULSE_SPELLCAST * 3, TRUE, TAR_IGNORE, 1, spell_nova, NULL);
  SPELL_ADD(CLASS_DRUID, 12);
 
  SPELL_CREATE("spore burst", SPELL_SPORE_BURST, SPELLTYPE_SUMMONING,
               PULSE_SPELLCAST * 3, TRUE, TAR_IGNORE, 1, spell_spore_burst, NULL);
  SPEC_SPELL_ADD(CLASS_DRUID, 11, SPEC_WOODLAND);
	
/*  
     SPELL_CREATE("control weather", SPELL_CONTROL_WEATHER,
     SPELLTYPE_GENERIC, PULSE_SPELLCAST * 3, FALSE, TAR_IGNORE, 0,
     cast_control_weather, NULL);
     SPELL_ADD(CLASS_DRUID, 7);
     SPELL_ADD(CLASS_RANGER, 8);
*/  
  SPELL_CREATE("mage flame", SPELL_MAGE_FLAME, SPELLTYPE_SUMMONING,
               (PULSE_SPELLCAST * 3) / 2, FALSE, TAR_SELF_ONLY, 0,
               spell_mage_flame,
               "Your mage flame slowly fades into nothingness.");
  SPELL_ADD(CLASS_CONJURER, 3);

  SPELL_CREATE("globe of darkness", SPELL_GLOBE_OF_DARKNESS,
               SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 2, FALSE,
               TAR_SELF_ONLY, 0, spell_globe_of_darkness, NULL);
  // SPELL_ADD(CLASS_CONJURER, 7);

  SPELL_CREATE("flame blade", SPELL_FLAME_BLADE,
               SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 2, FALSE, TAR_IGNORE, 0,
               spell_flame_blade, NULL);
  SPELL_ADD(CLASS_DRUID, 3);

  SPELL_CREATE("shield", SPELL_SHIELD,
               SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 2, FALSE, TAR_IGNORE, 0,
               spell_shield, NULL);
  // SPELL_ADD(CLASS_DRUID, 4);

  SPELL_CREATE("serendipity", SPELL_SERENDIPITY,
               SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 2, FALSE,
               TAR_CHAR_ROOM, 0, spell_serendipity, NULL);
  SPELL_ADD(CLASS_DRUID, 4);


  SPELL_CREATE("minor creation", SPELL_MINOR_CREATION,
               SPELLTYPE_SUMMONING, PULSE_SPELLCAST, FALSE, TAR_IGNORE, 0,
               cast_minor_creation, NULL);
  SPELL_ADD(CLASS_CONJURER, 1);
  SPELL_ADD(CLASS_SORCERER, 1);
  SPELL_ADD(CLASS_NECROMANCER, 1);
  SPELL_ADD(CLASS_BARD, 4);
  SPELL_ADD(CLASS_ILLUSIONIST, 1);
  SPELL_ADD(CLASS_SPIPER, 4);

  SPELL_CREATE("pulchritude", SPELL_PULCHRITUDE, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST, FALSE, TAR_CHAR_ROOM, 0, spell_pulchritude,
               "&+yYour features harden.&n");
  SPELL_ADD(CLASS_DRUID, 1);
	SPEC_SPELL_ADD(CLASS_SHAMAN, 7, SPEC_ANIMALIST);

  SPELL_CREATE("create food", SPELL_CREATE_FOOD, SPELLTYPE_SUMMONING,
               PULSE_SPELLCAST, FALSE, TAR_IGNORE, 0, spell_create_food,
               NULL);
//  SPELL_ADD(CLASS_DRUID, 1);
//  SPELL_ADD(CLASS_RANGER, 4);
//  SPELL_ADD(CLASS_CLERIC, 2);

  SPELL_CREATE("create water", SPELL_CREATE_WATER, SPELLTYPE_SUMMONING,
               PULSE_SPELLCAST, FALSE, TAR_OBJ_INV | TAR_OBJ_EQUIP, 0,
               spell_create_water, NULL);
//  SPELL_ADD(CLASS_DRUID, 1);
//  SPELL_ADD(CLASS_CLERIC, 2);
//  SPELL_ADD(CLASS_RANGER, 4);

  SPELL_CREATE("vigorize light", SPELL_VIGORIZE_LIGHT, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM, 0,
               spell_vigorize_light, NULL);
  SPELL_ADD(CLASS_CLERIC, 2);
//  SPELL_ADD(CLASS_DRUID, 2);
  SPELL_ADD(CLASS_PALADIN, 3);
  SPELL_ADD(CLASS_ANTIPALADIN, 3);
  SPELL_ADD(CLASS_RANGER, 3);

  SPELL_CREATE("cure blind", SPELL_CURE_BLIND, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM, 0, spell_cure_blind,
               NULL);
  SPELL_ADD(CLASS_CLERIC, 3);
//  SPELL_ADD(CLASS_DRUID, 3);
  SPELL_ADD(CLASS_PALADIN, 4);
  SPELL_ADD(CLASS_WARLOCK, 6);

  SPELL_CREATE2("wandering woods", SPELL_WANDERING_WOODS,
                SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE,
                0, spell_wandering_woods,
                "The forest falls asleep again as your enchantment fades.",
                "The land around you grows a tad more familiar.");
  SPELL_ADD(CLASS_DRUID, 9);

  SPELL_CREATE("aid", SPELL_AID, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM, 0, spell_aid,
               "&+gThe aid of nature leaves you.&n");
  SPELL_ADD(CLASS_DRUID, 2);

  SPELL_CREATE("devitalize", SPELL_DEVITALIZE,
               SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 2, TRUE,
               TAR_CHAR_ROOM, 0, spell_devitalize, NULL);
  SPELL_ADD(CLASS_WARLOCK, 5);

  SPELL_CREATE("vigorize serious", SPELL_VIGORIZE_SERIOUS,
               SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 2, TRUE,
               TAR_CHAR_ROOM, 0, spell_vigorize_serious, NULL);
  SPELL_ADD(CLASS_CLERIC, 3);
//  SPELL_ADD(CLASS_DRUID, 3);
  SPELL_ADD(CLASS_PALADIN, 5);
  SPELL_ADD(CLASS_ANTIPALADIN, 5);
  SPELL_ADD(CLASS_RANGER, 5);

  SPELL_CREATE("vigorize critic", SPELL_VIGORIZE_CRITIC,
               SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 2, TRUE,
               TAR_CHAR_ROOM, 0, spell_vigorize_critic, NULL);
  SPELL_ADD(CLASS_CLERIC, 4);

  SPELL_CREATE("disease", SPELL_DISEASE, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_disease, "You feel much better.");
  SPELL_ADD(CLASS_DRUID, 2);

  SPELL_CREATE("curse", SPELL_CURSE, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 3 / 2, TRUE,
               TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_FIGHT_VICT | TAR_SELF_NONO,
               1, spell_curse,
               "Your heart feels lighter, as you feel the &+Lcurse&n lift.");
  SPELL_ADD(CLASS_CLERIC, 6);
  SPELL_ADD(CLASS_ANTIPALADIN, 6);

  SPELL_CREATE("dispel lifeforce", SPELL_DISPEL_LIFEFORCE,
               SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 3 / 2, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, 1,
               spell_dispel_lifeforce,
               "&+wYou feel the life surge back into you");
  SPELL_ADD(CLASS_WARLOCK, 8);

  SPELL_CREATE("alter energy polarity", SPELL_ALTER_ENERGY_POLARITY,
               SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 3, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, 1,
               spell_alter_energy_polarity,
               "&+WYou feel yourself shift back into balance.");
  SPELL_ADD(CLASS_WARLOCK, 9);


  SPELL_CREATE("nether touch", SPELL_NETHER_TOUCH, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 3 / 2, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, 1,
               spell_nether_touch, "&+LA darkness lifts from your soul.");
  SPELL_ADD(CLASS_WARLOCK, 6);

  SPELL_CREATE("water breathing", SPELL_WATERBREATH, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 2, FALSE, TAR_CHAR_ROOM, 0,
               spell_waterbreath, "Your gills retract back into your neck.");
  SPELL_ADD(CLASS_CLERIC, 7);
//  SPELL_ADD(CLASS_DRUID, 7);

  SPELL_CREATE("farsee", SPELL_FARSEE, SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST
               * 5 / 2, FALSE, TAR_SELF_ONLY, 0,
               spell_farsee, "You cannot see as far anymore.");
  SPELL_ADD(CLASS_SORCERER, 4);
  SPELL_ADD(CLASS_RANGER, 9);
  SPELL_ADD(CLASS_CONJURER, 10);
  SPELL_ADD(CLASS_ILLUSIONIST, 4);

  SPELL_CREATE("fear", SPELL_FEAR, SPELLTYPE_GENERIC, PULSE_SPELLCAST,
               TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_fear, "You feel less cowardly.");
  SPELL_ADD(CLASS_CLERIC, 6);
  SPELL_ADD(CLASS_ANTIPALADIN, 5);

  SPELL_CREATE("recharger", SPELL_RECHARGER, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM, 0, spell_recharger,
               NULL);
/*  SPELL_ADD(CLASS_PSIONICIST, 2);*/

  SPELL_CREATE("cure light", SPELL_CURE_LIGHT, SPELLTYPE_HEALING,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0, spell_cure_light,
               NULL);
  SPELL_ADD(CLASS_CLERIC, 1);
//  SPELL_ADD(CLASS_DRUID, 1);
  SPELL_ADD(CLASS_PALADIN, 3);
  SPELL_ADD(CLASS_RANGER, 4);

  SPELL_CREATE("cure disease", SPELL_CURE_DISEASE, SPELLTYPE_HEALING,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0, spell_cure_disease,
               NULL);
  SPELL_ADD(CLASS_CLERIC, 4);
  SPELL_ADD(CLASS_PALADIN, 8);

  SPELL_CREATE("cause light", SPELL_CAUSE_LIGHT, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_cause_light, NULL);
  SPELL_ADD(CLASS_CLERIC, 1);
//  SPELL_ADD(CLASS_DRUID, 1);
  SPELL_ADD(CLASS_ANTIPALADIN, 3);

  SPELL_CREATE("cure serious", SPELL_CURE_SERIOUS, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_CHAR_ROOM, 0,
               spell_cure_serious, NULL);
  SPELL_ADD(CLASS_CLERIC, 2);
//  SPELL_ADD(CLASS_DRUID, 2);
  SPELL_ADD(CLASS_PALADIN, 5);

  SPELL_CREATE("unholy wind", SPELL_UNHOLY_WIND, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 5 / 4, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_unholy_wind, NULL);
  SPELL_ADD(CLASS_WARLOCK, 1);

  SPELL_CREATE("invoke negative energy", SPELL_INVOKE_NEG_ENERGY,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST * 4 / 3, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_invoke_negative_energy, NULL);
  SPELL_ADD(CLASS_WARLOCK, 3);

  SPELL_CREATE("purge living", SPELL_PURGE_LIVING, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_purge_living, NULL);
  SPELL_ADD(CLASS_WARLOCK, 5);

  SPELL_CREATE("channel negative energy", SPELL_CHANNEL_NEG_ENERGY,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST * 5 / 2, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_channel_negative_energy, NULL);
  SPELL_ADD(CLASS_WARLOCK, 10);

  SPELL_CREATE("cause serious", SPELL_CAUSE_SERIOUS, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 5 / 4, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_cause_serious, NULL);
  SPELL_ADD(CLASS_CLERIC, 2);
//  SPELL_ADD(CLASS_DRUID, 2);
  SPELL_ADD(CLASS_ANTIPALADIN, 5);

  SPELL_CREATE("cure critic", SPELL_CURE_CRITIC, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM, 0, spell_cure_critic,
               NULL);
  SPELL_ADD(CLASS_CLERIC, 3);
//  SPELL_ADD(CLASS_DRUID, 4);
  SPELL_ADD(CLASS_PALADIN, 6);

  SPELL_CREATE("cause critical", SPELL_CAUSE_CRITICAL, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 4 / 3, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_cause_critical, NULL);
  SPELL_ADD(CLASS_CLERIC, 3);
//  SPELL_ADD(CLASS_DRUID, 4);
  SPELL_ADD(CLASS_ANTIPALADIN, 6);

  SPELL_CREATE("heal", SPELL_HEAL, SPELLTYPE_HEALING, PULSE_SPELLCAST * 3
               / 2, TRUE, TAR_CHAR_ROOM, 0, spell_heal, NULL);
//  SPELL_ADD(CLASS_DRUID, 6);
  SPELL_ADD(CLASS_CLERIC, 5);
  SPELL_ADD(CLASS_PALADIN, 8);

  SPELL_CREATE("natures touch", SPELL_NATURES_TOUCH, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_CHAR_ROOM, 0,
               spell_natures_touch, NULL);
  SPELL_ADD(CLASS_DRUID, 5);

  SPELL_CREATE("sticks to snakes", SPELL_STICKS_TO_SNAKES, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_sticks_to_snakes, NULL);
  SPELL_ADD(CLASS_DRUID, 2);

  SPELL_CREATE("full heal", SPELL_FULL_HEAL, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM, 0, spell_full_heal,
               NULL);
  SPELL_ADD(CLASS_CLERIC, 7);

  SPELL_CREATE("unholy strength", SPELL_UNHOLY_STRENGTH,
               SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 7 / 3, TRUE,
               TAR_CHAR_ROOM, 0, spell_unholy_strength,
               "Your enhanced strength drains away.");
  SPELL_ADD(CLASS_WARLOCK, 5);

  SPELL_CREATE("vitality", SPELL_VITALITY, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 7 / 3, TRUE, TAR_CHAR_ROOM, 0,
               spell_vitality, "You feel less vitalized.");
  SPELL_ADD(CLASS_CLERIC, 5);
//  SPELL_ADD(CLASS_DRUID, 6);

  SPELL_CREATE("miracle", SPELL_MIRACLE, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 7 / 3, TRUE, TAR_IGNORE, 0,
               spell_miracle, "You feel less vitalized.");
  SPELL_ADD(CLASS_CLERIC, 12);

//  SPELL_CREATE("channel", SPELL_CHANNEL, SPELLTYPE_SUMMONING,
//                PULSE_SPELLCAST * 3, TRUE, TAR_IGNORE, 0,
//                cast_channel, NULL);
//  SPELL_ADD(CLASS_CLERIC, 11);

/* Divine fury currently does nothing due to poor coding */
  SPELL_CREATE("divine fury", SPELL_DIVINE_FURY, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 2, TRUE, TAR_SELF_ONLY, 0,
               spell_divine_fury, "You feel less divinely furious.");
/*  SPELL_ADD(CLASS_CLERIC, 9); */

  SPELL_CREATE("detect invisibility", SPELL_DETECT_INVISIBLE,
               SPELLTYPE_DIVINATION, PULSE_SPELLCAST * 3 / 2, FALSE,
               TAR_CHAR_ROOM, 0, spell_detect_invisibility,
               "Unseen things vanish from your sight.");
  SPELL_ADD(CLASS_NECROMANCER, 8);
  SPELL_ADD(CLASS_SORCERER, 9);
  SPELL_ADD(CLASS_CONJURER, 9);
  SPELL_ADD(CLASS_BARD, 10);
  SPELL_ADD(CLASS_SPIPER, 10);

  SPELL_CREATE("detect evil", SPELL_DETECT_EVIL, SPELLTYPE_DIVINATION,
               PULSE_SPELLCAST, FALSE, TAR_CHAR_ROOM, 0,
               spell_detect_evil,
               "You feel the &+rcrimson &nin your vision fade.");
  SPELL_ADD(CLASS_CLERIC, 1);
//  SPELL_ADD(CLASS_DRUID, 2);
  SPELL_ADD(CLASS_RANGER, 3);

  SPELL_CREATE("detect good", SPELL_DETECT_GOOD, SPELLTYPE_DIVINATION,
               PULSE_SPELLCAST, FALSE, TAR_CHAR_ROOM, 0,
               spell_detect_good,
               "You feel the &+Ygold &nin your vision fade.");
  SPELL_ADD(CLASS_CLERIC, 1);
//  SPELL_ADD(CLASS_DRUID, 2);

  SPELL_CREATE("detect magic", SPELL_DETECT_MAGIC, SPELLTYPE_DIVINATION,
               PULSE_SPELLCAST, FALSE, TAR_CHAR_ROOM, 0,
               spell_detect_magic, "Magical auras fade from your sight.");
  SPELL_ADD(CLASS_CLERIC, 1);
  SPELL_ADD(CLASS_SORCERER, 1);
  SPELL_ADD(CLASS_NECROMANCER, 1);
//  SPELL_ADD(CLASS_DRUID, 1);
  SPELL_ADD(CLASS_CONJURER, 1);
  SPELL_ADD(CLASS_RANGER, 4);
  SPELL_ADD(CLASS_ILLUSIONIST, 1);
  SPELL_ADD(CLASS_ETHERMANCER, 2);

  SPELL_CREATE("plane shift", SPELL_PLANE_SHIFT, SPELLTYPE_TELEPORTATION,
               PULSE_SPELLCAST * 2, FALSE, TAR_IGNORE, 0, cast_plane_shift,
               NULL);
  SPELL_ADD(CLASS_CLERIC, 9);
  SPELL_ADD(CLASS_WARLOCK, 9);
  SPELL_ADD(CLASS_ETHERMANCER, 9); 
  //  SPELL_ADD(CLASS_DRUID, 9);

  SPELL_CREATE("gate", SPELL_GATE, SPELLTYPE_TELEPORTATION,
               PULSE_SPELLCAST * 3, FALSE, TAR_IGNORE, 0, cast_gate, NULL);
  SPELL_ADD(CLASS_CONJURER, 9);

  SPELL_CREATE("nether gate", SPELL_NETHER_GATE, SPELLTYPE_TELEPORTATION,
               PULSE_SPELLCAST * 3, FALSE, TAR_IGNORE, 0, cast_nether_gate,
               NULL);
  SPELL_ADD(CLASS_WARLOCK, 10);

  SPELL_CREATE("sense holiness", SPELL_SENSE_HOLINESS, SPELLTYPE_PROTECTION,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_sense_holiness, "&+wYou can no longer sense holiness.");
  SPELL_ADD(CLASS_WARLOCK, 1);

  SPELL_CREATE("negative energy barrier", SPELL_NEG_ENERGY_BARRIER,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST * 2, TRUE, TAR_SELF_ONLY,
               0, spell_negative_energy_barrier,
               "&+LYour energy barrier fades.&n");
  SPELL_ADD(CLASS_WARLOCK, 6);

  SPELL_CREATE("lesser resurrect", SPELL_LESSER_RESURRECT, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 5, FALSE, TAR_OBJ_ROOM, 0, spell_lesser_resurrect,
               NULL);
  SPELL_ADD(CLASS_CLERIC, 7);

  SPELL_CREATE("resurrect", SPELL_RESURRECT, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 10, FALSE, TAR_OBJ_ROOM, 0, spell_resurrect,
               NULL);
  SPELL_ADD(CLASS_CLERIC, 10);

  SPELL_CREATE("hand of death", SPELL_HANDOFDEATH, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 10, FALSE, TAR_OBJ_ROOM, 0, spell_resurrect,
               NULL);
  SPELL_ADD(CLASS_WARLOCK, 10);


  SPELL_CREATE("death blessing", SPELL_DEATH_BLESSING, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 7 / 3, TRUE, TAR_IGNORE, 0,
               spell_death_blessing, "You feel less vitalized.");
  SPELL_ADD(CLASS_WARLOCK, 12);

  SPELL_CREATE("preserve", SPELL_PRESERVE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, FALSE, TAR_OBJ_ROOM, 0, spell_preserve,
               NULL);
  SPELL_ADD(CLASS_CLERIC, 2);
  SPELL_ADD(CLASS_NECROMANCER, 1);
//  SPELL_ADD(CLASS_DRUID, 2);

  SPELL_CREATE("mass preserve", SPELL_MASS_PRESERVE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3, FALSE, TAR_IGNORE, 0, spell_mass_preserve,
               NULL);
  SPELL_ADD(CLASS_CLERIC, 11);

  SPELL_CREATE("mass invisibility", SPELL_MASS_INVIS, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 3, FALSE, TAR_IGNORE, 0,
               spell_mass_invisibility,
               "You slowly fade back into visibility.");

  SPELL_ADD(CLASS_SORCERER, 9);
  SPELL_ADD(CLASS_ILLUSIONIST, 9);


  SPELL_CREATE("enchant weapon", SPELL_ENCHANT_WEAPON, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 8, FALSE, TAR_OBJ_INV | TAR_OBJ_EQUIP, 0,
               spell_enchant_weapon, NULL);
  SPELL_ADD(CLASS_CONJURER, 4);

  SPELL_CREATE("lifelust", SPELL_LIFELUST, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM, 0,
               spell_lifelust,
               "&+LThe &+Wlife&n&+rlust&+L drains out of you.");
  SPELL_ADD(CLASS_WARLOCK, 4);

  SPELL_CREATE("unmaking", SPELL_UNMAKING, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3, FALSE, TAR_OBJ_ROOM, 0,
               spell_unmaking, NULL);
  SPELL_ADD(CLASS_WARLOCK, 4);
  SPEC_SPELL_ADD(CLASS_NECROMANCER, 4, SPEC_REAPER);

  SPELL_CREATE("dispel invisible", SPELL_DISPEL_INVISIBLE,
               SPELLTYPE_DIVINATION, PULSE_SPELLCAST * 2, FALSE,
               TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_EQUIP, 0,
               spell_dispel_invisible, NULL);
  SPELL_ADD(CLASS_NECROMANCER, 8);
  SPELL_ADD(CLASS_SORCERER, 8);
  SPELL_ADD(CLASS_ILLUSIONIST, 7);

  SPELL_CREATE("improved invisibility", SPELL_INVIS_MAJOR,
               SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 4 / 3, FALSE,
               TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_EQUIP, 0,
               spell_improved_invisibility,
               "You slowly fade back into visibility.");
  SPELL_ADD(CLASS_NECROMANCER, 8);
  SPELL_ADD(CLASS_SORCERER, 7);
  SPELL_ADD(CLASS_CONJURER, 8);

  SPELL_CREATE("concealment", SPELL_INVISIBLE, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 4 / 3, FALSE,
               TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_EQUIP, 0,
               spell_invisibility, "You slowly fade back into visibility.");
  SPELL_ADD(CLASS_NECROMANCER, 5);
  SPELL_ADD(CLASS_SORCERER, 3);
  SPELL_ADD(CLASS_CONJURER, 3);
  SPELL_ADD(CLASS_RANGER, 6);
  SPELL_ADD(CLASS_BARD, 5);
  SPELL_ADD(CLASS_ILLUSIONIST, 3);
  SPELL_ADD(CLASS_SPIPER, 5);

  SPELL_CREATE("poison", SPELL_POISON, SPELLTYPE_GENERIC, PULSE_SPELLCAST
               * 2, TRUE,
               TAR_CHAR_ROOM | TAR_SELF_NONO | TAR_OBJ_INV | TAR_OBJ_EQUIP |
               TAR_FIGHT_VICT, 1, spell_poison,
               "You feel the &+gpoison &nin your &+Rblood &ndissipate.");
  SPELL_ADD(CLASS_DRUID, 4);

  SPELL_CREATE("protection from evil", SPELL_PROTECT_FROM_EVIL,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_protection_from_evil,
               "You do not feel as safe from &+revil&n.");
  SPELL_ADD(CLASS_CLERIC, 4);
//  SPELL_ADD(CLASS_DRUID, 4);
  SPELL_ADD(CLASS_PALADIN, 3);
  SPELL_ADD(CLASS_RANGER, 4);

  SPELL_CREATE("protection from good", SPELL_PROTECT_FROM_GOOD,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_protection_from_good,
               "You do not feel as safe from &+Ygood&n.");
  SPELL_ADD(CLASS_CLERIC, 4);
//  SPELL_ADD(CLASS_DRUID, 4);
  SPELL_ADD(CLASS_ANTIPALADIN, 3);
  SPELL_ADD(CLASS_WARLOCK, 3);



  SPELL_CREATE("raise spectre", SPELL_RAISE_SPECTRE, SPELL_TYPE_SUMMONING,
               PULSE_SPELLCAST * 3, FALSE, TAR_OBJ_ROOM, 0,
               spell_raise_spectre, NULL);
  SPELL_ADD(CLASS_NECROMANCER, 5);

  SPELL_CREATE("raise wraith", SPELL_RAISE_WRAITH, SPELL_TYPE_SUMMONING,
               PULSE_SPELLCAST * 3, FALSE, TAR_OBJ_ROOM, 0,
               spell_raise_wraith, NULL);
  SPELL_ADD(CLASS_NECROMANCER, 7);

  SPELL_CREATE("raise vampire", SPELL_RAISE_VAMPIRE, SPELL_TYPE_SUMMONING,
               PULSE_SPELLCAST * 3, FALSE, TAR_OBJ_ROOM, 0,
               spell_raise_vampire, NULL);
  SPELL_ADD(CLASS_NECROMANCER, 9);

  SPELL_CREATE("raise lich", SPELL_RAISE_LICH, SPELL_TYPE_SUMMONING,
               PULSE_SPELLCAST * 3, FALSE, TAR_OBJ_ROOM, 0, spell_raise_lich,
               NULL);
  SPELL_ADD(CLASS_NECROMANCER, 10);

  SPELL_CREATE("animate dead", SPELL_ANIMATE_DEAD, SPELLTYPE_SUMMONING,
               PULSE_SPELLCAST * 3, FALSE, TAR_OBJ_ROOM, 0,
               spell_animate_dead, NULL);
  SPELL_ADD(CLASS_NECROMANCER, 3);

  SPELL_CREATE("cyclone", SPELL_CYCLONE, SPELLTYPE_BLOW,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_cyclone, NULL);
  SPELL_ADD(CLASS_DRUID, 7);
  //SPEC_SPELL_ADD(CLASS_DRUID, 7, SPEC_STORM);
	
 SPEC_SPELL_ADD(CLASS_ETHERMANCER, 7, SPEC_FROST_MAGUS);
 SPEC_SPELL_ADD(CLASS_ETHERMANCER, 7, SPEC_WINDTALKER);

  SPELL_CREATE("sirens song", SPELL_SIREN_SONG, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_CHAR_RANGE, 1,
               spell_siren_song, NULL);
  SPELL_ADD(CLASS_BARD, 12);
  SPELL_ADD(CLASS_SPIPER, 12);

  SPELL_CREATE("negative energy vortex", SPELL_NEG_ENERGY_VORTEX,
               SPELLTYPE_HEALING, PULSE_SPELLCAST * 2, TRUE, TAR_IGNORE, 0,
               spell_negative_energy_vortex, NULL);
  SPELL_ADD(CLASS_WARLOCK, 8);

  SPELL_CREATE("remove curse", SPELL_REMOVE_CURSE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 4, FALSE,
               TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_OBJ_ROOM, 0,
               spell_remove_curse, NULL);
  SPELL_ADD(CLASS_PALADIN, 6);
  SPELL_ADD(CLASS_CLERIC, 4);
//  SPELL_ADD(CLASS_DRUID, 5);
  SPELL_ADD(CLASS_WARLOCK, 5);

  SPELL_CREATE("stone skin", SPELL_STONE_SKIN, SPELLTYPE_PROTECTION,
               PULSE_SPELLCAST * 4 / 3, TRUE, TAR_CHAR_ROOM, 0,
               spell_stone_skin,
               "You feel your skin soften and return to normal.");
  SPELL_ADD(CLASS_CONJURER, 6);
  SPELL_ADD(CLASS_SORCERER, 8);
  SPELL_ADD(CLASS_RANGER, 12);

  SPELL_CREATE("group stone skin", SPELL_GROUP_STONE_SKIN,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST * 5, TRUE, TAR_IGNORE, 0,
               spell_group_stone_skin,
               "You fell your skin soften and return to normal.");
  SPELL_ADD(CLASS_CONJURER, 10);

  SPELL_CREATE("sleep", SPELL_SLEEP, SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST *
               2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_sleep, "You awake from your magical slumber.");
  SPELL_ADD(CLASS_NECROMANCER, 2);
  SPELL_ADD(CLASS_SORCERER, 2);
  SPELL_ADD(CLASS_CONJURER, 2);
  SPELL_ADD(CLASS_WARLOCK, 1);
  SPELL_ADD(CLASS_ILLUSIONIST, 4);

  SPELL_CREATE("dispel magic", SPELL_DISPEL_MAGIC, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OBJ_INV | TAR_OBJ_ROOM, 1,
               spell_dispel_magic, NULL);
  SPELL_ADD(CLASS_CONJURER, 3);
  SPELL_ADD(CLASS_SORCERER, 4);
  SPELL_ADD(CLASS_NECROMANCER, 3);
  SPELL_ADD(CLASS_CLERIC, 8);
  SPELL_ADD(CLASS_BARD, 6);
  SPELL_ADD(CLASS_DRUID, 7);
  SPELL_ADD(CLASS_REAVER, 8);
  SPELL_ADD(CLASS_ILLUSIONIST, 2);
  SPELL_ADD(CLASS_SPIPER, 6);
  SPELL_ADD(CLASS_WARLOCK, 8);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 7, SPEC_FROST_MAGUS);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 7, SPEC_WINDTALKER);

  SPELL_CREATE("tranquility", SPELL_TRANQUILITY, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3, TRUE,
               TAR_IGNORE, 0, spell_tranquility, NULL);
  SPELL_ADD(CLASS_DRUID, 5);

  SPELL_CREATE("strength", SPELL_STRENGTH, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 2, FALSE, TAR_CHAR_ROOM, 0,
               spell_strength, "You feel less &+Cstrong&n.");
  SPELL_ADD(CLASS_NECROMANCER, 4);
  SPELL_ADD(CLASS_SORCERER, 2);
  SPELL_ADD(CLASS_CONJURER, 3);

  SPELL_CREATE("dexterity", SPELL_DEXTERITY, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 2, FALSE, TAR_CHAR_ROOM, 0,
               spell_dexterity, "You feel less &+Ldexterous&n.");
  SPELL_ADD(CLASS_SORCERER, 3);
  SPELL_ADD(CLASS_CONJURER, 3);

  SPELL_CREATE("summon", SPELL_SUMMON, SPELLTYPE_SUMMONING,
               PULSE_SPELLCAST * 2, FALSE, TAR_CHAR_WORLD, 0, spell_summon,
               NULL);
  SPELL_ADD(CLASS_CLERIC, 4);
//  SPELL_ADD(CLASS_DRUID, 4);

  SPELL_CREATE("ventriloquate", SPELL_VENTRILOQUATE, SPELLTYPE_GENERIC,
               0, FALSE, TAR_CHAR_ROOM | TAR_OBJ_ROOM | TAR_SELF_NONO, 0,
               spell_ventriloquate, NULL);
//  SPELL_ADD(CLASS_NECROMANCER, 1);
//  SPELL_ADD(CLASS_SORCERER, 1);
//  SPELL_ADD(CLASS_CONJURER, 1);
//  SPELL_ADD(CLASS_BARD, 3);
//  SPELL_ADD(CLASS_SPIPER, 3);

  SPELL_CREATE("dazzle", SPELL_DAZZLE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_SELF_ONLY, 0,
               spell_dazzle, "You feel the energy seep out of you.");
  SPELL_ADD(CLASS_RANGER, 10);

  SPELL_CREATE("blur", SPELL_BLUR, SPELLTYPE_GENERIC, PULSE_SPELLCAST * 2,
               TRUE, TAR_SELF_ONLY, 0,
               spell_blur, "&+BYour motions slow to normal.");
  SPELL_ADD(CLASS_RANGER, 11);
  SPELL_ADD(CLASS_ETHERMANCER, 11);

  SPELL_CREATE("pass without trace", SPELL_PASS_WITHOUT_TRACE,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST * 2, TRUE, TAR_SELF_ONLY,
               0, spell_pass_without_trace,
               "The forest close in around you.");
  SPELL_ADD(CLASS_DRUID, 8);


  SPELL_CREATE("sanctuary", SPELL_SANCTUARY, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST, TRUE, TAR_SELF_ONLY, 0, spell_sanctuary,
               "&+WYour glowing sanctuary &n&+wfades.");

  SPELL_CREATE("hellfire", SPELL_HELLFIRE, SPELLTYPE_GENERIC, PULSE_SPELLCAST,
               TRUE, TAR_SELF_ONLY, 0, spell_hellfire,
               "&+RYour burning hellfire &n&+rfades.");
  SPELL_ADD(CLASS_ANTIPALADIN, 12);

  SPELL_CREATE("stornogs metamagic shroud", SPELL_STORNOGS_LOWERED_RES,
               SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 3, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_stornogs_lowered_magical_res,
               "&+WYou feel your innate magical resistance return.");
  SPELL_ADD(CLASS_CONJURER, 7);

  SPELL_CREATE2("stornogs shimmering starshell", SPELL_STARSHELL,
                SPELLTYPE_GENERIC, (PULSE_SPELLCAST * 3) / 2, TRUE,
                TAR_IGNORE, 0, spell_starshell,
                "&+WThe &+Yblazing&N&+W shell dissipates.",
                "&+WThe &+Yblazing&N&+W shell dissipates.");
  SPELL_ADD(CLASS_CONJURER, 10);

  SPELL_CREATE("stornogs spheres", SPELL_STORNOGS_SPHERES,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST * 4, TRUE, TAR_SELF_ONLY, 0,
               spell_stornogs_spheres, NULL);
  SPELL_ADD(CLASS_CONJURER, 11);

  SPELL_CREATE("group stornogs spheres", SPELL_STORNOGS_GREATER_SPHERES,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST * 7, TRUE, TAR_SELF_ONLY, 0,
               spell_group_stornog, NULL);
  SPELL_ADD(CLASS_CONJURER, 12);

  SPELL_CREATE("cloak of fear", SPELL_CLOAK_OF_FEAR, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST, TRUE, TAR_IGNORE, 1, spell_cloak_of_fear,
               NULL);
  SPELL_ADD(CLASS_NECROMANCER, 10);

  SPELL_CREATE("acidimmolate", SPELL_ACIDIMMOLATE, SPELLTYPE_ACID,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_acidimmolate, NULL);

  SPELL_CREATE("immolate", SPELL_IMMOLATE, SPELLTYPE_FIRE,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_immolate, NULL);
  SPELL_ADD(CLASS_SORCERER, 8);
  SPELL_ADD(CLASS_REAVER, 10);

  SPELL_CREATE("creeping doom", SPELL_CDOOM, SPELLTYPE_SUMMONING,
               PULSE_SPELLCAST * 3, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_cdoom, NULL);
  SPELL_ADD(CLASS_DRUID, 10);
// SPEC_SPELL_ADD(CLASS_DRUID, 10, SPEC_WOODLAND);
 
  SPELL_CREATE("prismatic cube", SPELL_PRISMATIC_CUBE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE, 0, cast_prismatic_cube,
               NULL);
  SPELL_ADD(CLASS_CONJURER, 11);

  SPELL_CREATE("haste", SPELL_HASTE, SPELLTYPE_GENERIC, (PULSE_SPELLCAST *
                                                         3) / 2, TRUE,
               TAR_CHAR_ROOM, 0, spell_haste,
               "The world speeds up around you.");
  SPELL_ADD(CLASS_SORCERER, 7);
  SPELL_ADD(CLASS_CONJURER, 7);
  SPELL_ADD(CLASS_NECROMANCER, 8);
  SPELL_ADD(CLASS_RANGER, 9);

  SPELL_CREATE("word of recall", SPELL_WORD_OF_RECALL,
               SPELLTYPE_TELEPORTATION, 0, TRUE, TAR_SELF_ONLY, 0,
               spell_word_of_recall, NULL);
  SPELL_ADD(CLASS_CLERIC, 9);
//  SPELL_ADD(CLASS_WARLOCK, 9);
//  SPELL_ADD(CLASS_DRUID, 9);

  SPELL_CREATE("group recall", SPELL_GROUP_RECALL,
               SPELLTYPE_TELEPORTATION, PULSE_SPELLCAST * 6, TRUE, TAR_IGNORE,
               0, spell_group_recall, NULL);
  SPELL_ADD(CLASS_CLERIC, 11);
 // SPELL_ADD(CLASS_WARLOCK, 11);

  SPELL_CREATE("remove poison", SPELL_REMOVE_POISON, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, FALSE,
               TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, 0,
               spell_remove_poison, NULL);
  SPELL_ADD(CLASS_CLERIC, 3);
  SPELL_ADD(CLASS_PALADIN, 4);
  SPELL_ADD(CLASS_WARLOCK, 3);

  SPELL_CREATE("minor paralysis", SPELL_MINOR_PARALYSIS,
               SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1, spell_minor_paralysis,
               "You can move again.");
  SPELL_ADD(CLASS_SORCERER, 4);

  SPELL_CREATE("major paralysis", SPELL_MAJOR_PARALYSIS,
               SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 3, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1, spell_major_paralysis,
               "You can move again.");
#ifndef NEW_CODE
  SPELL_ADD(CLASS_SORCERER, 8);
  SPELL_ADD(CLASS_CONJURER, 10);
#endif

  SPELL_CREATE("slowness", SPELL_SLOW, SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST
               * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_slow, "You feel your induced slowness dissipate.");
/*  SPELL_ADD(CLASS_SORCERER, 5);*/
  SPELL_ADD(CLASS_CONJURER, 5);
  SPELL_ADD(CLASS_BARD, 7);
  SPELL_ADD(CLASS_WARLOCK, 6);
  SPELL_ADD(CLASS_SPIPER, 7);

  SPELL_CREATE("conjure elemental", SPELL_CONJURE_ELEMENTAL,
               SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE, 0,
               spell_conjour_elemental, NULL);
  SPELL_ADD(CLASS_CONJURER, 5);

  SPELL_CREATE("mirror image", SPELL_MIRROR_IMAGE,
               SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 2, FALSE,
               TAR_IGNORE, 0, spell_mirror_image, NULL);
  SPELL_ADD(CLASS_CONJURER, 2);
  SPELL_ADD(CLASS_BARD, 2);
  SPELL_ADD(CLASS_ILLUSIONIST, 3);
  SPELL_ADD(CLASS_SPIPER, 2);

  SPELL_CREATE("conjure greater elemental", SPELL_CONJURE_GREATER_ELEMENTAL,
               SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE, 0,
               spell_conjour_greater_elemental, NULL);
  SPELL_ADD(CLASS_CONJURER, 11);

  SPELL_CREATE("vitalize mana", SPELL_VITALIZE_MANA, 12,
               FALSE, 28, TAR_SELF_ONLY, 0, spell_vitalize_mana, NULL);


  SPELL_CREATE("sense follower", SPELL_SENSE_FOLLOWER,
               SPELLTYPE_DIVINATION, PULSE_SPELLCAST * 2, FALSE,
               TAR_SELF_ONLY, 0, spell_sense_follower,
               "You no longer seem to sense your followers.");
  SPELL_ADD(CLASS_NECROMANCER, 3);
  SPELL_ADD(CLASS_CONJURER, 5);

  SPELL_CREATE("sense life", SPELL_SENSE_LIFE, SPELLTYPE_DIVINATION,
               PULSE_SPELLCAST * 2, FALSE, TAR_SELF_ONLY, 0,
               spell_sense_life,
               "You no longer seem to sense other lifeforms.");
  SPELL_ADD(CLASS_CLERIC, 3);
//  SPELL_ADD(CLASS_DRUID, 3);
  SPELL_ADD(CLASS_RANGER, 6);

  SPELL_CREATE("continual light", SPELL_CONTINUAL_LIGHT,
               SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 3, TRUE,
               TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_AREA, 0,
               spell_continual_light, NULL);
  SPELL_ADD(CLASS_CLERIC, 6);
//  SPELL_ADD(CLASS_DRUID, 6);
  SPELL_ADD(CLASS_PALADIN, 8);
  SPELL_ADD(CLASS_ILLUSIONIST, 2);

  SPELL_CREATE2("consecrate land", SPELL_CONSECRATE_LAND,
                SPELLTYPE_SPIRIT, PULSE_SPELLCAST * 3, TRUE, TAR_IGNORE,
                0, spell_consecrate_land,
                "&+CYour magical runes vanish in a &+Lpuff of smoke.",
                "&+CThe magical runes vanish in a &+Lpuff of smoke.");
  SPELL_ADD(CLASS_DRUID, 9);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 9, SPEC_SPIRITUALIST);

  SPELL_CREATE2("summon insects", SPELL_SUMMON_INSECTS,
                SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 3, TRUE, TAR_IGNORE,
                0, spell_summon_insects,
                "&+mThe insects in the area scurry away.",
                "&+mThe insets in the area scurry away..");
  SPEC_SPELL_ADD(CLASS_DRUID, 7, SPEC_WOODLAND);
	
  SPELL_CREATE("darkness", SPELL_DARKNESS, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3, TRUE,
               TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_AREA, 0, spell_darkness,
               NULL);
//  SPELL_ADD(CLASS_DRUID, 5);
  SPELL_ADD(CLASS_CLERIC, 6);
  SPELL_ADD(CLASS_ANTIPALADIN, 8);
//  SPELL_ADD(CLASS_NECROMANCER, 8);
  SPELL_ADD(CLASS_WARLOCK, 6);
  SPELL_ADD(CLASS_ILLUSIONIST, 2);
  SPEC_SPELL_ADD(CLASS_NECROMANCER, 4, SPEC_NECROLYTE);

  SPELL_CREATE("protection from fire", SPELL_PROTECT_FROM_FIRE,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_protection_from_fire,
               "You no longer feel safe from &+rfire&n.");
  SPELL_ADD(CLASS_CLERIC, 2);
//  SPELL_ADD(CLASS_DRUID, 2);
  SPELL_ADD(CLASS_WARLOCK, 3);

  SPELL_CREATE("protection from cold", SPELL_PROTECT_FROM_COLD,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_protection_from_cold,
               "You no longer feel safe from &+Bcold&n.");
  SPELL_ADD(CLASS_CLERIC, 2);
//  SPELL_ADD(CLASS_DRUID, 2);
  SPELL_ADD(CLASS_NECROMANCER, 2);
  SPELL_ADD(CLASS_WARLOCK, 2);

  SPELL_CREATE("protection from living", SPELL_PROTECT_FROM_LIVING,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_protection_from_living,
               "You no longer feel safe from the &+wliving&n.");
  SPELL_ADD(CLASS_WARLOCK, 2);
  SPELL_ADD(CLASS_NECROMANCER, 2);

  SPELL_CREATE("protection from animals", SPELL_PROTECT_FROM_ANIMAL,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_protection_from_animals,
               "You no longer feel safe from &+yanimals&n.");
  SPELL_ADD(CLASS_DRUID, 3);

  SPELL_CREATE("animal friendship", SPELL_ANIMAL_FRIENDSHIP,
               SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 2, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT, 0, spell_animal_friendship,
               NULL);
  SPELL_ADD(CLASS_DRUID, 1);

  SPELL_CREATE("protection from gas", SPELL_PROTECT_FROM_GAS,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_protection_from_gas,
               "You no longer feel safe from &+gpoison gas&n.");
  SPELL_ADD(CLASS_CLERIC, 4);
//  SPELL_ADD(CLASS_DRUID, 3);

  SPELL_CREATE("protection from acid", SPELL_PROTECT_FROM_ACID,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_protection_from_acid,
               "You no longer feel safe from &+Gacid&n.");
  SPELL_ADD(CLASS_CLERIC, 4);
//  SPELL_ADD(CLASS_DRUID, 3);

  SPELL_CREATE("protection from lightning", SPELL_PROTECT_FROM_LIGHTNING,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_protection_from_lightning,
               "You no longer feel safe from &+Clightning&n.");
  SPELL_ADD(CLASS_CLERIC, 4);
//  SPELL_ADD(CLASS_DRUID, 3);

  SPELL_CREATE("levitate", SPELL_LEVITATE, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST, FALSE, TAR_CHAR_ROOM, 0,
               spell_levitate, "You feel heavier and stop levitating.");
  SPELL_ADD(CLASS_NECROMANCER, 5);
  SPELL_ADD(CLASS_SORCERER, 4);
  SPELL_ADD(CLASS_CONJURER, 4);
  SPELL_ADD(CLASS_ILLUSIONIST, 4);

  SPELL_CREATE("fly", SPELL_FLY, SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 2,
               FALSE, TAR_CHAR_ROOM, 0, spell_fly,
               "You feel heavier and stop levitating.");
  SPELL_ADD(CLASS_SORCERER, 8);
  SPELL_ADD(CLASS_CONJURER, 9);
  SPELL_ADD(CLASS_NECROMANCER, 10);
  SPELL_ADD(CLASS_ILLUSIONIST, 8);


  SPELL_CREATE("reveal true name", SPELL_REVEAL_TRUE_NAME,
               SPELLTYPE_DIVINATION, PULSE_SPELLCAST * 2, FALSE,
               TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_EQUIP, 0,
               spell_reveal_true_name, NULL);
  SPELL_ADD(CLASS_CONJURER, 4);
  SPELL_ADD(CLASS_SORCERER, 7);
  SPELL_ADD(CLASS_NECROMANCER, 7);
  SPELL_ADD(CLASS_RANGER, 9);
  SPELL_ADD(CLASS_ILLUSIONIST, 7);

  SPELL_CREATE("identify", SPELL_IDENTIFY, SPELLTYPE_DIVINATION,
               PULSE_SPELLCAST * 5, FALSE,
               TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_OBJ_ROOM, 0,
               spell_identify, NULL);
  SPELL_ADD(CLASS_CONJURER, 5);

  SPELL_CREATE("item lore", SPELL_LORE, SPELLTYPE_DIVINATION,
               PULSE_SPELLCAST * 5, FALSE,
               TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_OBJ_ROOM, 0,
               spell_lore, NULL);

  SPELL_CREATE("prismatic spray", SPELL_PRISMATIC_SPRAY,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST * 2, TRUE,
               TAR_AREA | TAR_OFFAREA, 1, spell_prismatic_spray, NULL);
  SPELL_ADD(CLASS_CONJURER, 8);
/*  SPELL_ADD(CLASS_SORCERER, 7);*/

  SPELL_CREATE("fireshield", SPELL_FIRESHIELD, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_SELF_ONLY, 0,
               spell_fireshield,
               "The &+Rburning flames &naround your body die down and fade.");
  SPELL_ADD(CLASS_CONJURER, 5);
  SPELL_ADD(CLASS_NECROMANCER, 5);
  SPELL_ADD(CLASS_SORCERER, 6);
  SPEC_SPELL_ADD(CLASS_REAVER, 7, SPEC_FLAME_REAVER);

  SPELL_CREATE("color spray", SPELL_COLOR_SPRAY, SPELLTYPE_GENERIC, PULSE_SPELLCAST * 5 / 3, TRUE, TAR_AREA | TAR_OFFAREA, 1,   //TAR_CHAR_ROOM /*| TAR_FIGHT_VICT*/ | TAR_CHAR_RANGE| TAR_AREA, 1,
               spell_color_spray, NULL);
  SPELL_ADD(CLASS_SORCERER, 6);
  SPELL_ADD(CLASS_CONJURER, 6);

  SPELL_CREATE("incendiary cloud", SPELL_INCENDIARY_CLOUD,
               SPELLTYPE_FIRE, (int) (PULSE_SPELLCAST * 2.5), TRUE,
               TAR_AREA | TAR_OFFAREA, 1, spell_incendiary_cloud, NULL);
  SPELL_ADD(CLASS_SORCERER, 9);

  SPELL_CREATE("ice storm", SPELL_ICE_STORM, SPELLTYPE_COLD,
               PULSE_SPELLCAST, TRUE,
               TAR_AREA | TAR_OFFAREA /*| TAR_CHAR_RANGE */ , 1,
               spell_ice_storm, NULL);
//  SPELL_ADD(CLASS_NECROMANCER, 5);
  SPELL_ADD(CLASS_SORCERER, 6);
  SPELL_ADD(CLASS_CONJURER, 7);
  SPELL_ADD(CLASS_ETHERMANCER, 5);

  SPELL_CREATE("prismatic ray", SPELL_PRISMATIC_RAY, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_prismatic_ray, NULL);
  SPELL_ADD(CLASS_SORCERER, 9);
  SPELL_ADD(CLASS_CONJURER, 8);

  SPELL_CREATE("disintegrate", SPELL_DISINTEGRATE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_disintegrate, NULL);
  SPELL_ADD(CLASS_CONJURER, 9);
  SPELL_ADD(CLASS_SORCERER, 9);
  SPELL_ADD(CLASS_REAVER, 11);

  SPELL_CREATE("acid stream", SPELL_ACID_STREAM, SPELLTYPE_ACID,
               PULSE_SPELLCAST * 5 / 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_acid_stream, NULL);
  //SPELL_ADD(CLASS_DRUID, 8);

  SPELL_CREATE("gaseous cloud", SPELL_GASEOUS_CLOUD, SPELLTYPE_ELEMENTAL,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_gaseous_cloud, NULL);
  SPELL_ADD(CLASS_SHAMAN, 10);

  SPELL_CREATE("acid blast", SPELL_ACID_BLAST, SPELLTYPE_ACID,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_acid_blast, NULL);
  SPELL_ADD(CLASS_SORCERER, 3);

  SPELL_CREATE("faerie fire", SPELL_FAERIE_FIRE, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_faerie_fire,
               "The outline of &+Mpurple flames &naround your body fades.");
  SPELL_ADD(CLASS_DRUID, 3);
  SPELL_ADD(CLASS_ETHERMANCER, 1);

  SPELL_CREATE("faerie fog", SPELL_FAERIE_FOG, SPELLTYPE_DIVINATION,
               PULSE_SPELLCAST * 2, FALSE, TAR_IGNORE, 0, spell_faerie_fog,
               NULL);
  SPELL_ADD(CLASS_DRUID, 3);
  SPELL_ADD(CLASS_RANGER, 6);
  SPELL_ADD(CLASS_ETHERMANCER, 3);

  SPELL_CREATE("power word kill", SPELL_PWORD_KILL, SPELLTYPE_GENERIC, PULSE_SPELLCAST/2,
               TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_INSTACAST, 1, spell_pword_kill,
               NULL);
  SPELL_ADD(CLASS_SORCERER, 8);
  SPELL_ADD(CLASS_NECROMANCER, 11);

  SPELL_CREATE("power word blind", SPELL_PWORD_BLIND, SPELLTYPE_GENERIC, PULSE_SPELLCAST/2, 
               TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_INSTACAST, 1, spell_pword_blind,
               NULL);
  SPELL_ADD(CLASS_SORCERER, 7);

  SPELL_CREATE("power word stun", SPELL_PWORD_STUN, SPELLTYPE_GENERIC, PULSE_SPELLCAST/2,
               TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_INSTACAST, 1, spell_pword_stun,
               "The world stops spinning.");
  SPELL_ADD(CLASS_SORCERER, 7);

  SPELL_CREATE("unholy word", SPELL_UNHOLY_WORD, SPELLTYPE_GENERIC, PULSE_SPELLCAST/2,
               TRUE, TAR_AREA | TAR_INSTACAST, 1, spell_unholy_word, NULL);
  SPELL_ADD(CLASS_CLERIC, 9);
  SPELL_ADD(CLASS_ANTIPALADIN, 9);
  SPELL_ADD(CLASS_NECROMANCER, 11);

  SPELL_CREATE("holy word", SPELL_HOLY_WORD, SPELLTYPE_GENERIC, PULSE_SPELLCAST/2, TRUE,
               TAR_AREA | TAR_OFFAREA | TAR_INSTACAST, 1, spell_holy_word, NULL);
  SPELL_ADD(CLASS_CLERIC, 9);
  SPELL_ADD(CLASS_PALADIN, 9);

  SPELL_CREATE("sunray", SPELL_SUNRAY, SPELLTYPE_FIRE,
               PULSE_SPELLCAST * 2, TRUE, TAR_FIGHT_VICT | TAR_CHAR_ROOM, 1,
               spell_sunray, NULL);
  SPELL_ADD(CLASS_DRUID, 9);

  SPELL_CREATE("feeblemind", SPELL_FEEBLEMIND, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 2, TRUE, TAR_FIGHT_VICT | TAR_CHAR_ROOM, 1,
               spell_feeblemind,
               "You feel your &+Rbludgeoned &nintellect recover.");
  SPELL_ADD(CLASS_NECROMANCER, 5);
  SPELL_ADD(CLASS_SORCERER, 6);
  SPELL_ADD(CLASS_CONJURER, 6);
  SPELL_ADD(CLASS_REAVER, 9);
  SPELL_ADD(CLASS_BARD, 9);

  SPELL_CREATE("healing blade", SPELL_HEALING_BLADE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 4 / 3, FALSE, TAR_SELF_ONLY, 0,
               spell_healing_blade,
               "&+BYour blade's healing aura dims then vanishes.&n");
//   SPELL_ADD(CLASS_RANGER, 11);


  SPELL_CREATE("windstrom blessing", SPELL_WINDSTROM_BLESSING,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST * 4 / 3, FALSE,
               TAR_SELF_ONLY, 0, spell_windstrom_blessing,
               "&+BYour blade's aura dims then vanishes.&n");

  SPELL_CREATE("silence", SPELL_SILENCE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_FIGHT_VICT | TAR_CHAR_ROOM, 1,
               spell_silence, "You are able to speak again.");
  SPELL_ADD(CLASS_CLERIC, 9);
//  SPELL_ADD(CLASS_DRUID, 6);

  SPELL_CREATE("turn undead", SPELL_TURN_UNDEAD, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST, TRUE, TAR_IGNORE, 0, spell_turn_undead, NULL);
  SPELL_ADD(CLASS_CLERIC, 1);
  SPELL_ADD(CLASS_PALADIN, 5);

  SPELL_CREATE("command undead", SPELL_COMMAND_UNDEAD, SPELLTYPE_GENERIC,
               0, TRUE, TAR_FIGHT_VICT | TAR_CHAR_ROOM, 0,
               spell_command_undead, NULL);
  // SPELL_ADD(CLASS_NECROMANCER, 5);
  // SPELL_ADD(CLASS_ANTIPALADIN, 10);

  SPELL_CREATE("web", SPELL_WEB, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE, 0, cast_web, NULL);
  SPELL_ADD(CLASS_CONJURER, 7);

  SPELL_CREATE("wall of flames", SPELL_WALL_OF_FLAMES, SPELLTYPE_FIRE,
               PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE, 0, cast_wall_of_flames,
               NULL);
  SPELL_ADD(CLASS_CONJURER, 7);

  SPELL_CREATE("wall of ice", SPELL_WALL_OF_ICE, SPELLTYPE_COLD,
               PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE, 0, cast_wall_of_ice,
               NULL);
  SPELL_ADD(CLASS_CONJURER, 7);

  SPELL_CREATE("life ward", SPELL_LIFE_WARD, SPELLTYPE_COLD,
               PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE, 0, cast_life_ward,
               NULL);
  SPELL_ADD(CLASS_WARLOCK, 10);
  SPELL_CREATE("wall of stone", SPELL_WALL_OF_STONE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE, 0, cast_wall_of_stone,
               NULL);
  SPELL_ADD(CLASS_CONJURER, 5);

  SPELL_CREATE("wall of iron", SPELL_WALL_OF_IRON, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 5, FALSE, TAR_IGNORE, 0, cast_wall_of_iron,
               NULL);
  SPELL_ADD(CLASS_CONJURER, 6);

  SPELL_CREATE("wall of force", SPELL_WALL_OF_FORCE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 5, FALSE, TAR_IGNORE, 0, cast_wall_of_force,
               NULL);
  SPELL_ADD(CLASS_CONJURER, 8);

  SPELL_CREATE("lightning curtain", SPELL_LIGHTNING_CURTAIN,
               SPELLTYPE_ELECTRIC, PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE, 0,
               cast_lightning_curtain, NULL);
 // SPELL_ADD(CLASS_DRUID, 8);
  SPEC_SPELL_ADD(CLASS_DRUID, 8, SPEC_STORM);
	 
  SPELL_ADD(CLASS_ETHERMANCER, 6);

  SPELL_CREATE("self comprehension", SPELL_SELF_COMPREHENSION,
               SPELLTYPE_DIVINATION, PULSE_SPELLCAST * 2, FALSE,
               TAR_SELF_ONLY, 0, spell_comprehend_languages,
               "You can no longer understand languages.");
//   SPELL_ADD(CLASS_SORCERER, 4);
//   SPELL_ADD(CLASS_CONJURER, 4);
//   SPELL_ADD(CLASS_BARD, 4);
//   SPELL_ADD(CLASS_NECROMANCER, 4);
//   SPELL_ADD(CLASS_ILLUSIONIST, 4);

  SPELL_CREATE("comprehend languages", SPELL_COMPREHEND_LANGUAGES,
               SPELLTYPE_DIVINATION, PULSE_SPELLCAST * 2, FALSE,
               TAR_CHAR_ROOM, 0, spell_comprehend_languages,
               "You can no longer comprehend languages.");
//   SPELL_ADD(CLASS_CLERIC, 4);
//   SPELL_ADD(CLASS_WARLOCK, 4);

#if 0
  SPELL_ADD(CLASS_SORCERER, 5);
  SPELL_ADD(CLASS_CLERIC, 4);
  SPELL_ADD(CLASS_DRUID, 4);
  SPELL_ADD(CLASS_BARD, 6);
#endif

  SPELL_CREATE("slow poison", SPELL_SLOW_POISON, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0, spell_slow_poison,
               "You feel the &+gpoison &nat work again in your &+Rblood &n.");
  SPELL_ADD(CLASS_CLERIC, 2);

  SPELL_CREATE("coldshield", SPELL_COLDSHIELD, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_SELF_ONLY, 0,
               spell_coldshield,
               "The &+Bkilling ice&N around your body melts.");
  SPELL_ADD(CLASS_CONJURER, 5);
  SPELL_ADD(CLASS_SORCERER, 5);
  SPELL_ADD(CLASS_NECROMANCER, 5);
  SPELL_ADD(CLASS_ETHERMANCER, 4);

  SPELL_CREATE("charm animal", SPELL_CHARM_ANIMAL, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_SELF_NONO, 0,
               spell_charm_animal, "You regain your senses.");
#ifndef NEW_CODE
  SPELL_ADD(CLASS_DRUID, 5);
  SPELL_ADD(CLASS_RANGER, 7);
#endif

  SPELL_CREATE("soulshield", SPELL_SOULSHIELD, SPELLTYPE_PROTECTION,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_SELF_ONLY, 0,
               spell_soulshield,
               "&+WThe spiritual aura about your body fades&n.");
  SPELL_ADD(CLASS_CLERIC, 5);
  SPELL_ADD(CLASS_PALADIN, 7);
  SPELL_ADD(CLASS_ANTIPALADIN, 7);

  SPELL_CREATE("holy sacrifice", SPELL_HOLY_SACRIFICE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3, TRUE, TAR_SELF_ONLY, 0,
               spell_holy_sacrifice,
               "You no longer feel as if your blood will aid anyone.");
  SPELL_ADD(CLASS_PALADIN, 10);

  SPELL_CREATE("battle ecstasy",
               SPELL_BATTLE_ECSTASY, SPELLTYPE_PROTECTION,
               PULSE_SPELLCAST * 3, TRUE, TAR_SELF_ONLY, 0,
               spell_battle_ecstasy,
               "&+WYou feel your inner turmoil disappear&n.");
  SPELL_ADD(CLASS_ANTIPALADIN, 10);

  SPELL_CREATE("mass heal", SPELL_MASS_HEAL, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 2, TRUE, TAR_IGNORE, 0,
               spell_mass_heal, NULL);
  SPELL_ADD(CLASS_CLERIC, 8);
  SPELL_ADD(CLASS_PALADIN, 10);

  SPELL_CREATE("true seeing", SPELL_TRUE_SEEING, SPELLTYPE_DIVINATION,
               PULSE_SPELLCAST * 2, FALSE, TAR_SELF_ONLY, 0,
               spell_true_seeing, "&+MYour vision &n&+mdulls.&n");
  SPELL_ADD(CLASS_CLERIC, 11);

  SPELL_CREATE("shadow vision", SPELL_SHADOW_VISION, SPELLTYPE_DIVINATION,
               PULSE_SPELLCAST * 2, FALSE, TAR_SELF_ONLY, 0,
               spell_shadow_vision, "&+MYour vision &n&+mdulls.&n");
  SPELL_ADD(CLASS_WARLOCK, 11);

  SPELL_CREATE("animal vision", SPELL_ANIMAL_VISION, SPELLTYPE_DIVINATION,
               PULSE_SPELLCAST * 2, FALSE, TAR_SELF_ONLY, 0,
               spell_animal_vision, "You vision &+ydulls.&n");
  SPELL_ADD(CLASS_DRUID, 8);

  SPELL_CREATE("scent of the bloodhound", SPELL_BLOODHOUND, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST * 2, FALSE, TAR_SELF_ONLY, 0,
               spell_bloodhound, "Your nose feels &+Gclogged&n.&n");
  SPEC_SPELL_ADD(CLASS_SHAMAN, 8, SPEC_ANIMALIST);

  SPELL_CREATE("tree", SPELL_TREE, SPELLTYPE_DIVINATION,
               PULSE_SPELLCAST * 2, FALSE, TAR_SELF_ONLY, 0,
               spell_tree, "You feel more like yourself.&n");
  //SPEC_SPELL_ADD(CLASS_DRUID, 11, SPEC_WOODLAND);
	SPELL_ADD(CLASS_RANGER, 10);
	
  SPELL_CREATE("animal growth", SPELL_ANIMAL_GROWTH, SPELLTYPE_ANIMAL,
					     PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM, 0, 
							 spell_animal_growth, NULL);
	SPEC_SPELL_ADD(CLASS_DRUID, 8, SPEC_WOODLAND);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 7, SPEC_ANIMALIST);
 	
	SPELL_CREATE("natures blessing", SPELL_NATURES_BLESSING, SPELLTYPE_PROTECTION,
					      PULSE_SPELLCAST * 2, TRUE, TAR_SELF_ONLY, 0, spell_natures_blessing, "You feel cold and alone for a moment.");
	SPELL_ADD(CLASS_DRUID, 10);
	
  SPELL_CREATE("enlarge", SPELL_ENLARGE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM, 0,
               spell_enlarge, "&+MYou return to your normal size.&n");
  SPELL_ADD(CLASS_SORCERER, 11);
  SPELL_ADD(CLASS_BARD, 12);
  SPELL_ADD(CLASS_SPIPER, 12);

  SPELL_CREATE("rope trick", SPELL_ROPE_TRICK, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM, 0,
               spell_rope_trick, "&+WYou climb down the &+yrope&n.");
  SPELL_ADD(CLASS_BARD, 11);
  SPELL_ADD(CLASS_SPIPER, 11);

  SPELL_CREATE("reduce", SPELL_REDUCE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM, 0,
               spell_reduce, "&+MYou return to your normal size.&n");
  SPELL_ADD(CLASS_SORCERER, 11);

  SPELL_CREATE("word of command", SPELL_COMMAND, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_command, NULL);
  SPELL_ADD(CLASS_CLERIC, 2);


  /* shaman spells */

  /* animal spells first */

  SPELL_CREATE("wolfspeed", SPELL_WOLFSPEED, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM, 0,
               spell_wolfspeed,
               "You feel less like a wolf and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 1);

  SPELL_CREATE("pythonsting", SPELL_PYTHONSTING, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_pythonsting, NULL);
  SPELL_ADD(CLASS_SHAMAN, 3);

  SPELL_CREATE("snailspeed", SPELL_SNAILSPEED, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST * 3, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_snailspeed,
               "You feel less like a snail and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 3);

  SPELL_CREATE("molevision", SPELL_MOLEVISION, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_molevision,
               "You feel less like a mole and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 3);

  SPELL_CREATE("pantherspeed", SPELL_PANTHERSPEED, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM, 0,
               spell_pantherspeed,
               "You feel less like a panther and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 4);

  SPELL_CREATE("mousestrength", SPELL_MOUSESTRENGTH, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_mousestrength,
               "You feel less like a mouse and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 4);

  SPELL_CREATE("summon beast", SPELL_SUMMON_BEAST, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST * 2, FALSE, TAR_IGNORE, 0,
               spell_summon_beast, NULL);
  SPELL_ADD(CLASS_SHAMAN, 4);

  SPELL_CREATE("greater summon beast", SPELL_GREATER_SUMMON_BEAST,
               SPELLTYPE_ANIMAL, PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE, 0,
               spell_greater_summon_beast, NULL);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 9, SPEC_ANIMALIST);

  SPELL_CREATE("hawkvision", SPELL_HAWKVISION, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST * 3, TRUE, TAR_CHAR_ROOM, 0,
               spell_hawkvision,
               "You feel less like a hawk and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 4);

  SPELL_CREATE("bearstrength", SPELL_BEARSTRENGTH, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST * 3, TRUE, TAR_CHAR_ROOM, 0,
               spell_bearstrength,
               "You feel less like a bear and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 5);

  SPELL_CREATE("shrewtameness", SPELL_SHREWTAMENESS, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST * 3, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_shrewtameness,
               "You feel less like a shrew and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 5);

  SPELL_CREATE("lionrage", SPELL_LIONRAGE, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST * 3, TRUE, TAR_CHAR_ROOM, 0,
               spell_lionrage,
               "You feel less like a lion and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 6);

  SPELL_CREATE("elephantstrength", SPELL_ELEPHANTSTRENGTH, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST * 4, TRUE, TAR_CHAR_ROOM, 0,
               spell_elephantstrength,
               "You feel less like an elephant and more like your normal self.");
  SPELL_ADD(CLASS_SHAMAN, 7);

  SPELL_CREATE("ravenflight", SPELL_RAVENFLIGHT, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST * 3, FALSE, TAR_CHAR_ROOM, 0,
               spell_ravenflight,
               "Shoulders tingling, you gently drop back to the ground.");
  SPELL_ADD(CLASS_SHAMAN, 7);

  SPELL_CREATE("greater ravenflight", SPELL_GREATER_RAVENFLIGHT,
               SPELLTYPE_ANIMAL, PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE, 0,
               spell_greater_ravenflight, NULL);
  SPELL_ADD(CLASS_SHAMAN, 9);

  SPELL_CREATE("greater pythonsting", SPELL_GREATER_PYTHONSTING,
               SPELLTYPE_ANIMAL, PULSE_SPELLCAST * 4, TRUE, TAR_AREA, 1,
               spell_greater_pythonsting, NULL);
  SPELL_ADD(CLASS_SHAMAN, 8);

  SPELL_CREATE("call of the wild", SPELL_CALL_OF_THE_WILD, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST * 3, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, 1,
               spell_call_of_the_wild, NULL);
//  SPELL_ADD(CLASS_SHAMAN, 12);

  SPELL_CREATE("beastform", SPELL_BEASTFORM, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST * 3, FALSE, TAR_SELF_ONLY, 0,
               spell_beastform, NULL);
//  SPELL_ADD(CLASS_SHAMAN, 5);



  /* elemental spells */


  SPELL_CREATE("elemental affinity", SPELL_ELEM_AFFINITY, SPELLTYPE_ELEMENTAL,
               PULSE_SPELLCAST * 4, FALSE, TAR_SELF_ONLY, 0,
               spell_elemental_affinity,
               "&+BA slight&N &+Wch&N&+Cil&N&+Wl&N &+Bruns through you as you feel&N &+gn&N&+Ga&N&+gt&N&+Gu&N&+gr&N&+Ge's&N &+Yes&N&+Ws&N&+Yen&N&+Wce&N &+Bleave your body.&N");
  SPEC_SPELL_ADD(CLASS_SHAMAN, 7, SPEC_ELEMENTALIST);

  SPELL_CREATE("elemental fury", SPELL_ELEM_FURY, SPELLTYPE_ELEMENTAL,
               PULSE_SPELLCAST * 3, TRUE, TAR_AREA | TAR_OFFAREA, 1,
               spell_elemental_fury, NULL);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 12, SPEC_ELEMENTALIST);

  SPELL_CREATE("ice missile", SPELL_ICE_MISSILE, SPELLTYPE_ELEMENTAL,
               PULSE_SPELLCAST, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE | TAR_RANGE2,
               1, spell_ice_missile, NULL);
  SPELL_ADD(CLASS_SHAMAN, 1);
  SPELL_ADD(CLASS_ETHERMANCER, 1);

  SPELL_CREATE("flameburst", SPELL_FLAMEBURST, SPELLTYPE_ELEMENTAL,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_flameburst, NULL);
  SPELL_ADD(CLASS_SHAMAN, 2);

  SPELL_CREATE("scalding blast", SPELL_SCALDING_BLAST, SPELLTYPE_ELEMENTAL,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_scalding_blast, NULL);
  SPELL_ADD(CLASS_SHAMAN, 3);

  SPELL_CREATE("cold ward", SPELL_COLD_WARD, SPELLTYPE_ELEMENTAL,
               PULSE_SPELLCAST, FALSE, TAR_CHAR_ROOM, 0,
               spell_cold_ward,
               "&+cYou no longer feel protected from the cold.");
  SPELL_ADD(CLASS_SHAMAN, 3);

  SPELL_CREATE("fire ward", SPELL_FIRE_WARD, SPELLTYPE_ELEMENTAL,
               PULSE_SPELLCAST, FALSE, TAR_CHAR_ROOM, 0,
               spell_fire_ward, "&+rYou no longer feel protected from fire.");
  SPELL_ADD(CLASS_SHAMAN, 3);

  SPELL_CREATE("scorching touch", SPELL_SCORCHING_TOUCH, SPELLTYPE_ELEMENTAL,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_scorching_touch, NULL);
  SPELL_ADD(CLASS_SHAMAN, 4);

  SPELL_CREATE("molten spray", SPELL_MOLTEN_SPRAY, SPELLTYPE_ELEMENTAL,
               PULSE_SPELLCAST * (3 / 2), TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGE, 1,
               spell_molten_spray, NULL);
  SPELL_ADD(CLASS_SHAMAN, 5);

  SPELL_CREATE("earthen grasp", SPELL_EARTHEN_GRASP, SPELLTYPE_ELEMENTAL,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_earthen_grasp,
               "&+yThe earthen fist finally releases its grip on you.");
  SPELL_ADD(CLASS_SHAMAN, 6);

  SPELL_CREATE("greater earthen grasp", SPELL_GREATER_EARTHEN_GRASP,
               SPELLTYPE_ELEMENTAL, PULSE_SPELLCAST * 2, TRUE, TAR_AREA, 1,
               spell_greater_earthen_grasp,
               "&+yThe earthen fist finally releases its grip.");
  SPELL_ADD(CLASS_SHAMAN, 11);

  SPELL_CREATE("arieks shattering iceball",
               SPELL_ARIEKS_SHATTERING_ICEBALL, SPELLTYPE_ELEMENTAL,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT
/*| TAR_CHAR_RANGE | TAR_RANGE2 */ , 1,
               spell_arieks_shattering_iceball, NULL);
  SPELL_ADD(CLASS_SHAMAN, 9);
//  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 11, SPEC_FROST_MAGUS);

  SPELL_CREATE("scathing wind", SPELL_SCATHING_WIND, SPELLTYPE_ELEMENTAL,
               PULSE_SPELLCAST * 2, TRUE, TAR_AREA | TAR_OFFAREA, 1,
               spell_scathing_wind, NULL);
  SPELL_ADD(CLASS_SHAMAN, 8);

  SPELL_CREATE("earthen rain", SPELL_EARTHEN_RAIN, SPELLTYPE_ELEMENTAL,
               PULSE_SPELLCAST * 3, TRUE, TAR_AREA | TAR_OFFAREA, 1,
               spell_earthen_rain, NULL);
  SPELL_ADD(CLASS_SHAMAN, 10);


  /* spirit spells */

  SPELL_CREATE("indomitability", SPELL_INDOMITABILITY, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST * 2, TRUE, TAR_SELF_ONLY, 0,
               spell_indomitability,
               "&+BYour spiritual senses return to the land of the living, bringing your fears and vulnerabilities back with them.&N");
  SPEC_SPELL_ADD(CLASS_SHAMAN, 7, SPEC_SPIRITUALIST);

  SPELL_CREATE("spirit armor", SPELL_SPIRIT_ARMOR, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_spirit_armor, "&+mYou feel less protected.");
  SPELL_ADD(CLASS_SHAMAN, 1);

  SPELL_CREATE("transfer wellness", SPELL_TRANSFER_WELLNESS, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_transfer_wellness, NULL);
  SPELL_ADD(CLASS_SHAMAN, 1);

  SPELL_CREATE("sustenance", SPELL_SUSTENANCE, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST, FALSE, TAR_CHAR_ROOM, 0,
               spell_sustenance, NULL);
  //SPELL_ADD(CLASS_SHAMAN, 2);

  SPELL_CREATE("reveal spirit essence", SPELL_REVEAL_SPIRIT_ESSENCE,
               SPELLTYPE_SPIRIT, PULSE_SPELLCAST * 2, FALSE,
               TAR_CHAR_ROOM | TAR_SELF_NONO, 0,
               spell_reveal_spirit_essence, NULL);
  SPELL_ADD(CLASS_SHAMAN, 2);

  SPELL_CREATE("purify spirit", SPELL_PURIFY_SPIRIT, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_purify_spirit, NULL);
  SPELL_ADD(CLASS_SHAMAN, 3);
  SPELL_ADD(CLASS_ETHERMANCER, 2);

  SPELL_CREATE("lesser mending", SPELL_LESSER_MENDING, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_lesser_mending, NULL);
  SPELL_ADD(CLASS_SHAMAN, 2);

  SPELL_CREATE("mending", SPELL_MENDING, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0, spell_mending, NULL);
  SPELL_ADD(CLASS_SHAMAN, 4);

  SPELL_CREATE("soul disturbance", SPELL_SOUL_DISTURBANCE, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST * (3 / 2), TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1, spell_soul_disturbance,
               NULL);
  SPELL_ADD(CLASS_SHAMAN, 4);

  SPELL_CREATE("spirit sight", SPELL_SPIRIT_SIGHT, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST * 2, FALSE, TAR_CHAR_ROOM, 0,
               spell_spirit_sight, "&+LYour spirit sight fades.");
  SPELL_ADD(CLASS_SHAMAN, 6);

  SPELL_CREATE("sense spirit", SPELL_SENSE_SPIRIT, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST, FALSE, TAR_CHAR_ROOM, 0,
               spell_sense_spirit, "&+LYour sense of spirits disappears.");
  SPELL_ADD(CLASS_SHAMAN, 5);

  SPELL_CREATE("malison", SPELL_MALISON, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_malison, "You feel less cursed.");
  SPELL_ADD(CLASS_SHAMAN, 5);

  SPELL_CREATE("wellness", SPELL_WELLNESS, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST, TRUE, TAR_IGNORE, 0, spell_wellness, NULL);
  SPELL_ADD(CLASS_SHAMAN, 6);

  SPELL_CREATE("greater mending", SPELL_GREATER_MENDING, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_greater_mending, NULL);
  SPELL_ADD(CLASS_SHAMAN, 6);

  SPELL_CREATE("spirit anguish", SPELL_SPIRIT_ANGUISH, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_spirit_anguish, NULL);
  SPELL_ADD(CLASS_SHAMAN, 6);

  SPELL_CREATE("greater spirit anguish", SPELL_GREATER_SPIRIT_ANGUISH,
               SPELLTYPE_SPIRIT, PULSE_SPELLCAST * 3 / 2, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_greater_spirit_anguish, NULL);
  SPELL_ADD(CLASS_SHAMAN, 12);

  SPELL_CREATE("greater soul disturbance", SPELL_GREATER_SOUL_DISTURB,
               SPELLTYPE_SPIRIT, PULSE_SPELLCAST, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_greater_soul_disturbance, NULL);
  SPELL_ADD(CLASS_SHAMAN, 7);

  SPELL_CREATE("spirit ward", SPELL_SPIRIT_WARD, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_spirit_ward, "The dim aura around you fades.");
  SPELL_ADD(CLASS_SHAMAN, 7);

  SPELL_CREATE("greater sustenance", SPELL_GREATER_SUST, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST, FALSE, TAR_IGNORE, 0,
               spell_greater_sustenance, NULL);
//  SPELL_ADD(CLASS_SHAMAN, 7);

  SPELL_CREATE("reveal true form", SPELL_REVEAL_TRUE_FORM, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST, FALSE, TAR_IGNORE, 0,
               spell_reveal_true_form, NULL);
  SPELL_ADD(CLASS_SHAMAN, 8);

  SPELL_CREATE("spirit jump", SPELL_SPIRIT_JUMP, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST * 3, FALSE, TAR_CHAR_WORLD | TAR_SELF_NONO,
               0, spell_spirit_jump, NULL);
  SPELL_ADD(CLASS_SHAMAN, 8);

  SPELL_CREATE("greater spirit sight", SPELL_GREATER_SPIRIT_SIGHT,
               SPELLTYPE_SPIRIT, PULSE_SPELLCAST, FALSE, TAR_CHAR_ROOM, 0,
               spell_greater_spirit_sight, "&+LYour spirit sight fades.");
  SPELL_ADD(CLASS_SHAMAN, 11);
  SPELL_CREATE("greater spirit ward", SPELL_GREATER_SPIRIT_WARD,
               SPELLTYPE_SPIRIT, PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_greater_spirit_ward, "The dim aura around you fades.");
  SPELL_ADD(CLASS_SHAMAN, 9);

  SPELL_CREATE("etherportal", SPELL_ETHERPORTAL, SPELLTYPE_ELEMENTAL,
               PULSE_SPELLCAST * 10, FALSE, TAR_CHAR_WORLD, 0,
               spell_etherportal, NULL);
  SPELL_ADD(CLASS_SHAMAN, 10);

  SPELL_CREATE("spirit walk", SPELL_SPIRIT_WALK, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST * 7, FALSE, TAR_CHAR_ROOM | TAR_SELF_NONO,
               0, spell_spirit_walk, NULL);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 10, SPEC_SPIRITUALIST);

  SPELL_CREATE("essence of the wolf", SPELL_ESSENCE_OF_WOLF, SPELLTYPE_ANIMAL,
               PULSE_SPELLCAST * 3, FALSE, TAR_CHAR_ROOM | TAR_SELF_NONO,
               0, spell_essence_of_the_wolf,
               "&+yThe essence of the wolf leaves your being.&n");
  SPEC_SPELL_ADD(CLASS_SHAMAN, 11, SPEC_ANIMALIST);

// Spells not yet implemented

/*
  SPELL_CREATE("create earthen projectile", SPELL_CREATE_EARTHEN_PROJ, SPELLTYPE_ELEMENTAL,
               PULSE_SPELLCAST * 2, FALSE, TAR_IGNORE, 0,
               spell_create_earthen_proj, NULL);
*/
#if 0
  SPELL_ADD(CLASS_SHAMAN, 1);
#endif

  SPELL_CREATE("animate earthen projectile", SPELL_ANIMATE_EARTHEN_PROJ,
               SPELLTYPE_ELEMENTAL, PULSE_SPELLCAST, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1, spell_animate_earthen_proj,
               NULL);
#if 0
  SPELL_ADD(CLASS_SHAMAN, 1);
#endif

  SPELL_CREATE("summon spirit", SPELL_SUMMON_SPIRIT, SPELLTYPE_SPIRIT,
               PULSE_SPELLCAST, FALSE, TAR_IGNORE, 0,
               spell_summon_spirit, NULL);
#if 0
  SPELL_ADD(CLASS_SHAMAN, 7);
#endif

  /* end of shaman spells */

  /* Reaver spells */
  SPELL_CREATE("baladors protection", SPELL_THARKUNS_PROTECTION,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_baladors_protection,
               "You feel Balador's protection leave you.");
  SPELL_ADD(CLASS_REAVER, 5);

  SPELL_CREATE("ferrix precision", SPELL_KELEMONS_PRECISION,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST, TRUE, TAR_SELF_ONLY, 0,
               spell_ferrix_precision, "&+BYou feel less precise&n");
  SPELL_ADD(CLASS_REAVER, 7);

  SPELL_CREATE("eshabalas vitality", SPELL_KVARKS_VITALITY, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST, TRUE, TAR_SELF_ONLY, 0,
               spell_eshabalas_vitality,
               "&+mYou feel a bit worn out now that Eshabala has left you.&n");
  SPELL_ADD(CLASS_REAVER, 9);

  SPELL_CREATE("squerriks sneaky ways", SPELL_FOOS_SNEAKY_WAYS,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST * 4 / 3, FALSE,
               TAR_SELF_ONLY, 0, spell_squerriks_sneaky_ways,
               "&+mYou feel as if all can see you.&n");
  SPELL_ADD(CLASS_REAVER, 11);

  SPELL_CREATE("daragors flaming sword", SPELL_ILIENZES_FLAME_SWORD,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST * 4 / 3, FALSE,
               TAR_SELF_ONLY, 0, spell_daragors_flame_sword,
               "&+rYour fl&+yam&+re slowly burns out.&n");
  SPELL_ADD(CLASS_REAVER, 11);

  SPELL_CREATE("kanchelsis fury", SPELL_DAKTAS_FURY, SPELLTYPE_GENERIC,
               (PULSE_SPELLCAST * 3) / 2, TRUE, TAR_SELF_ONLY, 0,
               spell_kanchelsis_fury, "&+LThe world speeds up around you.&n");
  SPELL_ADD(CLASS_REAVER, 8);

  SPELL_CREATE("blood alliance", SPELL_BLOOD_ALLIANCE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_SELF_NONO, 0,
               spell_blood_alliance, 0);
  SPEC_SPELL_ADD(CLASS_REAVER, 10, SPEC_BLOOD_REAVER);

/* Illusionists */
/* Mossi Modification: 13 Nov 2002 */

  SPELL_CREATE("phantom armor", SPELL_PHANTOM_ARMOR, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST, TRUE, TAR_SELF_ONLY, 0,
               spell_phantom_armor, "&+LYou become less hazy.&N");
  SPELL_ADD(CLASS_ILLUSIONIST, 1);

  SPELL_CREATE("shadow monster", SPELL_SHADOW_MONSTER, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 4 / 3, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_shadow_monster, NULL);
  SPELL_ADD(CLASS_ILLUSIONIST, 3);

  SPELL_CREATE("insects", SPELL_INSECTS, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 4 / 3, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_insects, NULL);
  SPELL_ADD(CLASS_ILLUSIONIST, 4);

  SPELL_CREATE("illusionary wall", SPELL_ILLUSIONARY_WALL, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE, 0,
               spell_illusionary_wall, NULL);
  SPELL_ADD(CLASS_ILLUSIONIST, 5);

  SPELL_CREATE("boulder", SPELL_BOULDER, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 4 / 3, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_boulder, NULL);
  SPELL_ADD(CLASS_ILLUSIONIST, 5);

  SPELL_CREATE("shadow travel", SPELL_SHADOW_TRAVEL, SPELLTYPE_TELEPORTATION,
               PULSE_SPELLCAST * 3 / 2, FALSE, TAR_CHAR_WORLD | TAR_SELF_NONO,
               0, spell_shadow_travel, NULL);
  SPELL_ADD(CLASS_ILLUSIONIST, 5);

  SPELL_CREATE("stunning visions", SPELL_STUNNING_VISIONS, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_stunning_visions, NULL);
  SPELL_ADD(CLASS_ILLUSIONIST, 6);

  SPELL_CREATE("reflection", SPELL_REFLECTION, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_reflection, NULL);
  SPELL_ADD(CLASS_ILLUSIONIST, 6);

  SPELL_CREATE("mask", SPELL_MASK, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3, FALSE, TAR_IGNORE, 0, spell_mask, NULL);
  SPELL_ADD(CLASS_ILLUSIONIST, 8);

  SPELL_CREATE("watching wall", SPELL_WATCHING_WALL, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 5, FALSE, TAR_IGNORE, 0,
               spell_watching_wall, NULL);

  SPELL_CREATE("nightmare", SPELL_NIGHTMARE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_nightmare, "You feel less frightend.");
  SPELL_ADD(CLASS_ILLUSIONIST, 6);

  SPELL_CREATE("shadow shield", SPELL_SHADOW_SHIELD, SPELLTYPE_PROTECTION,
               PULSE_SPELLCAST * 4 / 3, TRUE, TAR_CHAR_ROOM, 0,
               spell_shadow_shield,
               "&+yYou feel the &+Ls&+Ww&+Li&+Wr&+Ll&+Wi&+Ln&+Wg &n&+yshadows around your body dissipate.&n");
  SPELL_ADD(CLASS_ILLUSIONIST, 8);

  SPELL_CREATE("vanish", SPELL_VANISH, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST, FALSE, TAR_SELF_ONLY, 0,
               spell_vanish, "&+WYou snap back into visibility.&n");
  SPELL_ADD(CLASS_ILLUSIONIST, 8);

  SPELL_CREATE("hammer", SPELL_HAMMER, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_hammer, NULL);
  SPELL_ADD(CLASS_ILLUSIONIST, 7);

  SPELL_CREATE("detect illusion", SPELL_DETECT_ILLUSION, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 4 / 3, TRUE, TAR_SELF_ONLY, 0,
               spell_detect_illusion,
               "Unseen things and illusion vanish from your sight.");
  SPELL_ADD(CLASS_ILLUSIONIST, 9);

  SPELL_CREATE("dream travel", SPELL_SHADOW_RIFT, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 4, FALSE, TAR_CHAR_WORLD, 0,
               spell_shadow_rift, NULL);
  SPELL_ADD(CLASS_ILLUSIONIST, 9);

  SPELL_CREATE("clone form", SPELL_CLONE_FORM, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 4, FALSE, TAR_CHAR_ROOM, 0,
               spell_clone_form, NULL);
  SPELL_ADD(CLASS_ILLUSIONIST, 10);

  SPELL_CREATE("imprisonment", SPELL_IMPRISON, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, FALSE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_imprison, NULL);
  SPELL_ADD(CLASS_ILLUSIONIST, 10);

  SPELL_CREATE("nonexistence", SPELL_NONEXISTENCE, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE, 0,
               spell_nonexistence, "You slowly fade back into visibility.");
  SPELL_ADD(CLASS_ILLUSIONIST, 11);

  SPELL_CREATE("dragon", SPELL_DRAGON, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 4 / 3, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_dragon, NULL);
  SPELL_ADD(CLASS_ILLUSIONIST, 12);

  SPELL_CREATE("titan", SPELL_TITAN, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_titan, "You return to your normal size!");
  SPELL_ADD(CLASS_ILLUSIONIST, 11);

  SPELL_CREATE("delirium", SPELL_DELIRIUM, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_delirium, "&+WYou feel less &+Gconfused&n");
  SPELL_ADD(CLASS_ILLUSIONIST, 10);

  SPELL_CREATE("flicker", SPELL_FLICKER, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE, 0,
               spell_flicker, NULL);
  SPELL_ADD(CLASS_ILLUSIONIST, 11);

  SPELL_CREATE("greater flicker", SPELL_GREATER_FLICKER, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 7, FALSE, TAR_CHAR_ROOM, 0,
               spell_greater_flicker, NULL);
  //SPELL_ADD(CLASS_ILLUSIONIST, 12);

// Ethermancer

  SPELL_CREATE("vapor armor", SPELL_VAPOR_ARMOR, SPELLTYPE_PROTECTION,
               PULSE_SPELLCAST * 2, FALSE, TAR_CHAR_ROOM, 0,
               spell_vapor_armor,
               "&+CYou feel the protection of the winds dissapate.");
  SPELL_ADD(CLASS_ETHERMANCER, 1);

  SPELL_CREATE("faerie sight", SPELL_FAERIE_SIGHT, SPELLTYPE_PROTECTION,
               PULSE_SPELLCAST * 2, FALSE, TAR_SELF_ONLY, 0,
               spell_faerie_sight,
               "You feel the &+mtwinkle&n in your eyes fade.");

  SPELL_ADD(CLASS_ETHERMANCER, 4);

  SPELL_CREATE("frost beacon", SPELL_FROST_BEACON, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 4, FALSE, TAR_IGNORE, 0, spell_frost_beacon,
               NULL);
  SPELL_ADD(CLASS_ETHERMANCER, 6);

  SPELL_CREATE("cold snap", SPELL_COLD_SNAP, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_cold_snap, NULL);
  SPELL_ADD(CLASS_ETHERMANCER, 7);

  SPELL_CREATE("vapor strike", SPELL_VAPOR_STRIKE, SPELLTYPE_PROTECTION,
               PULSE_SPELLCAST, FALSE, TAR_CHAR_ROOM, 0, spell_vapor_strike,
               "&+CThe vapors subside leaving you weak.&n");
  SPELL_ADD(CLASS_ETHERMANCER, 5);

  SPELL_CREATE("wind blade", SPELL_WIND_BLADE, SPELLTYPE_SUMMONING,
               PULSE_SPELLCAST * 2, FALSE, TAR_IGNORE, 0, spell_wind_blade,
               NULL);
  SPELL_ADD(CLASS_ETHERMANCER, 3);

  SPELL_CREATE("windwalk", SPELL_WINDWALK, SPELLTYPE_TELEPORTATION,
               PULSE_SPELLCAST * 3 / 2, FALSE, TAR_CHAR_WORLD | TAR_SELF_NONO,
               0, spell_windwalk, NULL);
  SPELL_ADD(CLASS_ETHERMANCER, 7);

  SPELL_CREATE("frost bolt", SPELL_FROST_BOLT, SPELLTYPE_COLD,
               PULSE_SPELLCAST * 3 / 4, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_frost_bolt, "The &+Cice&n wracking your body melts.");
  SPELL_ADD(CLASS_ETHERMANCER, 3);

  SPELL_CREATE("ethereal form", SPELL_ETHEREAL_FORM, SPELL_TYPE_GENERIC,
               PULSE_SPELLCAST * 3, FALSE, TAR_SELF_ONLY, 0,
               spell_ethereal_form,
               "&+LYou feel your body begin to regain its former substance...");
  SPELL_ADD(CLASS_ETHERMANCER, 8);

  SPELL_CREATE("ethereal recharge", SPELL_ETHEREAL_RECHARGE,
               SPELLTYPE_HEALING, PULSE_SPELLCAST * 3 / 2, TRUE,
               TAR_CHAR_ROOM, 0, spell_ethereal_recharge, NULL);
  SPELL_ADD(CLASS_ETHERMANCER, 8);

  SPELL_CREATE("arcane whirlwind", SPELL_ARCANE_WHIRLWIND, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, 1,
               spell_arcane_whirlwind, NULL);
  SPELL_ADD(CLASS_ETHERMANCER, 9);

  SPELL_CREATE("forked lightning", SPELL_FORKED_LIGHTNING, SPELLTYPE_ELECTRIC,
               PULSE_SPELLCAST * 2, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT
               /*| TAR_CHAR_RANGE | TAR_RANGE2 */ ,
               1, spell_forked_lightning, NULL);
  SPELL_ADD(CLASS_ETHERMANCER, 7);

  SPELL_CREATE("purge", SPELL_PURGE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 0,
               spell_purge, NULL);
  SPELL_ADD(CLASS_ETHERMANCER, 10);

  SPELL_CREATE("conjure air", SPELL_CONJURE_AIR, SPELLTYPE_SUMMONING,
               PULSE_SPELLCAST * 3, FALSE, TAR_IGNORE, 0, spell_conjure_air,
               NULL);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 9, SPEC_FROST_MAGUS);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 9, SPEC_WINDTALKER);

  SPELL_CREATE("induce tupor", SPELL_INDUCE_TUPOR, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 4, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 0,
               spell_induce_tupor, "You awake from your magical slumber.");
  SPELL_ADD(CLASS_ETHERMANCER, 10);

  SPELL_CREATE2("tempest terrain", SPELL_TEMPEST_TERRAIN,
                SPELLTYPE_ENCHANTMENT, PULSE_SPELLCAST * 6, FALSE, TAR_IGNORE,
                0, spell_tempest_terrain,
                "The terrain becomes solid once again.",
                "The terrain becomes solid once again.");
  SPELL_ADD(CLASS_ETHERMANCER, 11);

  SPELL_CREATE("storm empathy", SPELL_STORM_EMPATHY,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0,
               spell_storm_empathy,
               "You do not feel as safe from &+Bstorms&n.");
  SPELL_ADD(CLASS_ETHERMANCER, 5);

  SPELL_CREATE("mass fly", SPELL_MASS_FLY,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST * 3, TRUE, TAR_IGNORE, 0,
               spell_mass_fly, NULL);
  SPELL_ADD(CLASS_ETHERMANCER, 11);

  SPELL_CREATE("path of frost", SPELL_PATH_OF_FROST, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 5, TRUE, TAR_SELF_ONLY, 0,
               spell_path_of_frost, "Your tracks return to normal.");
  SPELL_ADD(CLASS_ETHERMANCER, 12);

  // Cosmomancer spec, Ethermancer
  SPELL_CREATE("supernova", SPELL_SUPERNOVA, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3, TRUE, TAR_AREA + TAR_OFFAREA, 1, spell_supernova, NULL);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 11, SPEC_COSMOMANCER);

  SPELL_CREATE("cosmic vacuum", SPELL_COSMIC_VACUUM, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1, spell_cosmic_vacuum, NULL);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 7, SPEC_COSMOMANCER);

  SPELL_CREATE("comet", SPELL_COMET, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST + 1, TRUE, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1, spell_comet, NULL);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 9, SPEC_COSMOMANCER);

  SPELL_CREATE("ethereal discharge", SPELL_ETHEREAL_DISCHARGE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3, TRUE, TAR_AREA + TAR_OFFAREA, 1, spell_ethereal_discharge, NULL);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 8, SPEC_COSMOMANCER);

  SPELL_CREATE("planetary alignment", SPELL_PLANETARY_ALIGNMENT, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 5, TRUE, TAR_SELF_ONLY, 0,
               spell_planetary_alignment, "Your feel weaker.");
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 9, SPEC_COSMOMANCER);

  // Frost Magus Spec
  SPELL_CREATE("squall", SPELL_TEMPEST, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 5, TRUE, TAR_IGNORE, 1, spell_tempest, NULL);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 11, SPEC_FROST_MAGUS);

  SPELL_CREATE("windrage", SPELL_WIND_RAGE, SPELLTYPE_ENCHANTMENT,
               PULSE_SPELLCAST * 4, TRUE, TAR_SELF_ONLY, 0, spell_wind_rage,
               "&+cYour mind begins to slow as the winds subside.");
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 10, SPEC_FROST_MAGUS);

  // Windtalker Spec
  SPELL_CREATE("ethereal alliance", SPELL_ETHEREAL_ALLIANCE,
               SPELLTYPE_GENERIC, PULSE_SPELLCAST * 5, TRUE, TAR_CHAR_ROOM, 0,
               spell_ethereal_alliance, "&+cYour alliance has been severed.");
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 12, SPEC_WINDTALKER);

  SPELL_CREATE("greater ethereal recharge", SPELL_GREATER_ETHEREAL,
               SPELLTYPE_HEALING, PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM, 0,
               spell_greater_ethereal_recharge, 0);

  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 10, SPEC_WINDTALKER);

  SPELL_CREATE("way of the wind", SPELL_WAY_OF_THE_WIND,
               SPELLTYPE_TELEPORTATION, PULSE_SPELLCAST * 5, FALSE,
               TAR_CHAR_WORLD | TAR_SELF_NONO, 0, spell_relocate, NULL);
  SPEC_SPELL_ADD(CLASS_ETHERMANCER, 9, SPEC_WINDTALKER);



  SPELL_CREATE("doom blade", SPELL_DOOM_BLADE,
               SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 8, FALSE, TAR_IGNORE, 0,
               spell_doom_blade, NULL);
  SPEC_SPELL_ADD(CLASS_NECROMANCER, 8, SPEC_REAPER);


#if 1
  SKILL_CREATE("aerial combat", SKILL_AERIAL_COMBAT, SKILL_TYPE_PHYS_DEX);

  SKILL_CREATE("aerial casting", SKILL_AERIAL_CASTING, SKILL_TYPE_MENTAL_INT);
#endif

  SKILL_CREATE_WITH_CAT("dirt toss", SKILL_DIRTTOSS, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_THIEF, 1, 90);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 90, SPEC_THIEF);

  SKILL_CREATE("climb", SKILL_CLIMB, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_ROGUE, 1, 99);
  SKILL_ADD(CLASS_THIEF, 1, 99);
  SKILL_ADD(CLASS_ASSASSIN, 1, 90);
  SKILL_ADD(CLASS_BARD, 1, 90);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 75);
  SKILL_ADD(CLASS_PALADIN, 1, 80);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 75);
  SKILL_ADD(CLASS_RANGER, 1, 80);
  SKILL_ADD(CLASS_PSIONICIST, 1, 40);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 40);
  SKILL_ADD(CLASS_CLERIC, 1, 55);
  SKILL_ADD(CLASS_CONJURER, 1, 40);
  SKILL_ADD(CLASS_SORCERER, 1, 40);
  SKILL_ADD(CLASS_WARLOCK, 1, 40);
  SKILL_ADD(CLASS_NECROMANCER, 1, 40);
  SKILL_ADD(CLASS_SHAMAN, 1, 45);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 45);
  SKILL_ADD(CLASS_DRUID, 1, 45);
  SKILL_ADD(CLASS_MERCENARY, 1, 80);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 40);
  SKILL_ADD(CLASS_BERSERKER, 1, 75);
  SKILL_ADD(CLASS_REAVER, 1, 75);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 40);
  SKILL_ADD(CLASS_DREADLORD, 1, 80);
  SKILL_ADD(CLASS_AVENGER, 1, 80);

  SKILL_CREATE_WITH_CAT("subterfuge", SKILL_SUBTERFUGE, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_THIEF, 20, 99);
  SKILL_ADD(CLASS_ROGUE, 20, 60);

  SKILL_CREATE("disguise", SKILL_DISGUISE, SKILL_TYPE_NONE);
  SKILL_ADD(CLASS_THIEF, 35, 85);
  SKILL_ADD(CLASS_ROGUE, 30, 85);
  SKILL_ADD(CLASS_ASSASSIN, 30, 90);

  SKILL_CREATE_WITH_CAT("trip", SKILL_TRIP, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_THIEF, 16, 100);
  SKILL_ADD(CLASS_REAVER, 26, 80);
  SPEC_SKILL_ADD(CLASS_BARD, 52, 50, SPEC_SCOUNDREL);
  SPEC_SKILL_ADD(CLASS_SPIPER, 52, 50, SPEC_SCOUNDREL);
  SPEC_SKILL_ADD(CLASS_ROGUE, 16, 95, SPEC_THIEF);
  SPEC_SKILL_ADD(CLASS_ROGUE, 16, 85, SPEC_BRAVO);

  SKILL_CREATE_WITH_CAT("feign death", SKILL_FEIGN_DEATH, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_NECROMANCER, 1, 99);
  SKILL_ADD(CLASS_MONK, 1, 99);
  SKILL_ADD(CLASS_THIEF, 25, 99);
  SPEC_SKILL_ADD(CLASS_ROGUE, 16, 99, SPEC_THIEF);

  SKILL_CREATE_WITH_CAT("quivering palm", SKILL_QUIVERING_PALM, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_MONK, 10, 99);

  SKILL_CREATE_WITH_CAT("combination attack", SKILL_COMBINATION, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_MONK, 10, 100);

  SKILL_CREATE_WITH_CAT("regenerate", SKILL_REGENERATE, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_MONK, 20, 99);

  SKILL_CREATE_WITH_CAT("touch of death", SKILL_TOUCH_DEATH, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);

  SKILL_CREATE("first aid", SKILL_FIRST_AID, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 99);
  SKILL_ADD(CLASS_ASSASSIN, 1, 99);
  SKILL_ADD(CLASS_BARD, 1, 99);
  SKILL_ADD(CLASS_PSIONICIST, 1, 99);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 99);
  SKILL_ADD(CLASS_CLERIC, 1, 99);
  SKILL_ADD(CLASS_CONJURER, 1, 99);
  SKILL_ADD(CLASS_DRUID, 1, 99);
  SKILL_ADD(CLASS_MERCENARY, 1, 99);
  SKILL_ADD(CLASS_PALADIN, 1, 99);
  SKILL_ADD(CLASS_RANGER, 1, 99);
  SKILL_ADD(CLASS_SORCERER, 1, 99);
  SKILL_ADD(CLASS_WARLOCK, 1, 99);
  SKILL_ADD(CLASS_NECROMANCER, 1, 99);
  SKILL_ADD(CLASS_ROGUE, 1, 99);
  SKILL_ADD(CLASS_THIEF, 1, 99);
  SKILL_ADD(CLASS_WARRIOR, 1, 99);
  SKILL_ADD(CLASS_SHAMAN, 1, 99);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 99);
  SKILL_ADD(CLASS_MONK, 1, 99);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 99);
  SKILL_ADD(CLASS_BERSERKER, 1, 99);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 99);
  SKILL_ADD(CLASS_REAVER, 1, 99);
  SKILL_ADD(CLASS_SPIPER, 1, 99);
  SKILL_ADD(CLASS_DREADLORD, 1, 99);
  SKILL_ADD(CLASS_AVENGER, 1, 99);

  SKILL_CREATE_WITH_CAT("retreat", SKILL_RETREAT, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 99);
  SKILL_ADD(CLASS_ASSASSIN, 1, 99);
  SKILL_ADD(CLASS_BARD, 1, 99);
  SKILL_ADD(CLASS_PSIONICIST, 1, 99);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 99);
  SKILL_ADD(CLASS_CLERIC, 1, 99);
  SKILL_ADD(CLASS_CONJURER, 1, 99);
  SKILL_ADD(CLASS_DRUID, 1, 99);
  SKILL_ADD(CLASS_MERCENARY, 1, 99);
  SKILL_ADD(CLASS_PALADIN, 1, 99);
  SKILL_ADD(CLASS_RANGER, 1, 99);
  SKILL_ADD(CLASS_SORCERER, 1, 99);
  SKILL_ADD(CLASS_WARLOCK, 1, 99);
  SKILL_ADD(CLASS_NECROMANCER, 1, 99);
  SKILL_ADD(CLASS_ROGUE, 1, 99);
  SKILL_ADD(CLASS_THIEF, 1, 99);
  SKILL_ADD(CLASS_WARRIOR, 1, 99);
  SKILL_ADD(CLASS_SHAMAN, 1, 99);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 99);
  SKILL_ADD(CLASS_MONK, 1, 99);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 99);
  SKILL_ADD(CLASS_BERSERKER, 1, 25);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 99);
  SKILL_ADD(CLASS_REAVER, 1, 99);
  SKILL_ADD(CLASS_SPIPER, 1, 99);
  SKILL_ADD(CLASS_DREADLORD, 1, 99);

  // Deceiver Skills
  SKILL_CREATE_WITH_CAT("expedited retreat", SKILL_EXPEDITED_RETREAT, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_DEFENSIVE);
  SPEC_SKILL_ADD(CLASS_ILLUSIONIST, 41, 60, SPEC_DECEIVER);

  SKILL_CREATE_WITH_CAT("expedited retreat", SKILL_EXPEDITED_RETREAT, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_DEFENSIVE);
  SPEC_SKILL_ADD(CLASS_ILLUSIONIST, 41, 60, SPEC_DECEIVER);


  SKILL_CREATE("craft", SKILL_CRAFT, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_ALCHEMIST, 21, 100);

  SKILL_CREATE("encrust", SKILL_ENCRUST, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_ALCHEMIST, 51, 100);

  
  SKILL_CREATE("spellbind", SKILL_SPELLBIND, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_ALCHEMIST, 31, 100);
      
  SKILL_CREATE("enchant", SKILL_ENCHANT, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_ALCHEMIST, 56, 100);

  SKILL_CREATE("fix", SKILL_FIX, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_ALCHEMIST, 11, 100);

  SKILL_CREATE_WITH_CAT("flank", SKILL_FLANK, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_DREADLORD, 11, 100);
  SKILL_ADD(CLASS_AVENGER, 11, 100);

  SKILL_CREATE_WITH_MESSAGES_WITH_CAT("gaze", SKILL_GAZE, SKILL_TYPE_PHYS_DEX,
                             "Feeling begins to return to your limbs.",
                             "Color seems to return to $n's face.", SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_DREADLORD, 21, 100);
  SKILL_ADD(CLASS_AVENGER, 21, 100);

  SKILL_CREATE_WITH_CAT("tainted blade", SKILL_TAINTED_BLADE, SKILL_TYPE_PHYS_DEX ,SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_DREADLORD, 26, 100);

  SKILL_CREATE_WITH_CAT("holy blade", SKILL_HOLY_BLADE, SKILL_TYPE_PHYS_DEX ,SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_AVENGER, 26, 100);

  SKILL_CREATE_WITH_CAT("battle senses", SKILL_BATTLE_SENSES, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_DREADLORD, 11, 100);
  SKILL_ADD(CLASS_AVENGER, 11, 100);

  SKILL_CREATE("blood scent", SKILL_BLOOD_SCENT, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_DREADLORD, 31, 100);

  SKILL_CREATE("battle orders", SKILL_BATTLE_ORDERS, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_DREADLORD, 51, 70);
  SKILL_ADD(CLASS_AVENGER, 51, 70);

  SKILL_CREATE("call of the grave", SKILL_CALL_GRAVE, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_DREADLORD, 21, 80);

  SKILL_CREATE("mix", SKILL_MIX, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 100);

  SKILL_CREATE("smelt", SKILL_SMELT, SKILL_TYPE_PHYS_DEX);
  SPEC_SKILL_ADD(CLASS_ALCHEMIST, 1, 100, SPEC_BLACKSMITH);

  SKILL_CREATE_WITH_CAT("throwpotion", SKILL_THROW_POTIONS, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 100);

  SKILL_CREATE("remix", SKILL_REMIX, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_ALCHEMIST, 51, 100);


  SKILL_CREATE_WITH_CAT("safe fall", SKILL_SAFE_FALL, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_ROGUE, 1, 99);
  SKILL_ADD(CLASS_THIEF, 1, 99);
  SKILL_ADD(CLASS_ASSASSIN, 1, 99);
  SKILL_ADD(CLASS_MONK, 1, 99);

  SKILL_CREATE_WITH_CAT("switch opponents", SKILL_SWITCH_OPPONENTS,
               SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_WARRIOR, 1, 99);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 85);
  SKILL_ADD(CLASS_PALADIN, 51, 85);
  SKILL_ADD(CLASS_MERCENARY, 51, 50);
  SKILL_ADD(CLASS_ROGUE, 1, 50);
  SKILL_ADD(CLASS_THIEF, 1, 50);
  SKILL_ADD(CLASS_ASSASSIN, 1, 50);
  SKILL_ADD(CLASS_MONK, 1, 75);
  SKILL_ADD(CLASS_RANGER, 1, 85);
  SKILL_ADD(CLASS_REAVER, 1, 85);
  SKILL_ADD(CLASS_DREADLORD, 26, 90);
  SKILL_ADD(CLASS_AVENGER, 26, 90);
  SPEC_SKILL_ADD(CLASS_ALCHEMIST, 31, 100, SPEC_BATTLE_FORGER);
  SPEC_SKILL_ADD(CLASS_ROGUE, 31, 100, SPEC_BRAVO);

  SKILL_CREATE_WITH_CAT("springleap", SKILL_SPRINGLEAP, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_RANGER, 1, 99);
  SKILL_ADD(CLASS_MONK, 1, 90);

  SKILL_CREATE("martial arts", SKILL_MARTIAL_ARTS, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_MONK, 1, 99);

  SKILL_CREATE("unarmed damage", SKILL_UNARMED_DAMAGE, SKILL_TYPE_PHYS_STR);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 70);
  SKILL_ADD(CLASS_PALADIN, 1, 80);
  SKILL_ADD(CLASS_ROGUE, 1, 80);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SKILL_ADD(CLASS_DREADLORD, 1, 80);

  SKILL_CREATE_WITH_CAT("buddha palm", SKILL_BUDDHA_PALM, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_MONK, 15, 99);

  SKILL_CREATE_WITH_MESSAGES("heroism", SKILL_HEROISM,
                             SKILL_TYPE_MENTAL_WIS,
                             "You no longer feel like a hero!", NULL);
  SKILL_ADD(CLASS_MONK, 1, 99);

  SKILL_CREATE("chant", SKILL_CHANT, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_MONK, 1, 99);

  SKILL_CREATE_WITH_CAT("dragon punch", SKILL_DRAGON_PUNCH, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_MONK, 1, 99);

  SKILL_CREATE("calm", SKILL_CALM, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_MONK, 1, 99);


  SKILL_CREATE_WITH_CAT("song of charming", SONG_CHARMING, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_BARD, 56, 99);

  SKILL_CREATE_WITH_CAT("song of discord", SONG_DISCORD, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_BARD, 16, 99);

  SKILL_CREATE("song of harmony", SONG_HARMONY, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_BARD, 16, 99);

  SKILL_CREATE_WITH_CAT("song of storms", SONG_STORMS, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_BARD, 41, 99);

  SKILL_CREATE_WITH_CAT("song of chaos", SONG_CHAOS, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_BARD, 46, 99);

  SKILL_CREATE("song of dragons", SONG_DRAGONS, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_BARD, 31, 99);

  SKILL_CREATE_WITH_CAT("song of sleep", SONG_SLEEP, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_BARD, 6, 99);

  SKILL_CREATE_WITH_CAT("song of calming", SONG_CALMING, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_BARD, 16, 99);

  SKILL_CREATE_WITH_CAT("song of healing", SONG_HEALING, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_BARD, 36, 99);

  //SKILL_CREATE("song of revelation", SONG_REVELATION, SKILL_TYPE_MENTAL_INT);
  SKILL_CREATE_WITH_MESSAGES("song of revelation", SONG_REVELATION,
                             SKILL_TYPE_MENTAL_INT,
                             "Your senses are no longer enhanced.", NULL);
  SKILL_ADD(CLASS_BARD, 46, 99);

  SKILL_CREATE_WITH_CAT("song of harming", SONG_HARMING, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_BARD, 36, 99);

  SKILL_CREATE("song of flight", SONG_FLIGHT, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_BARD, 31, 99);

  SKILL_CREATE_WITH_CAT("song of protection", SONG_PROTECTION, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_BARD, 21, 99);

  SKILL_CREATE_WITH_MESSAGES("song of heroism", SONG_HEROISM,
                             SKILL_TYPE_MENTAL_INT,
                             "You no longer feel like a hero.", NULL);
  SKILL_ADD(CLASS_BARD, 46, 99);

  SKILL_CREATE_WITH_CAT("song of cowardice", SONG_COWARDICE, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_BARD, 21, 99);

  SKILL_CREATE("song of forgetfulness", SONG_FORGETFULNESS,
               SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_BARD, 21, 99);

SKILL_CREATE_WITH_CAT("song of peace", SONG_PEACE, SKILL_TYPE_MENTAL_INT,SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_BARD, 26, 99);

/* Spectral Piper */


  SKILL_CREATE("dirge of rebuke", PIPER_REBUKE, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SPIPER, 16, 99);

  SKILL_CREATE("hymn of dread", PIPER_DREAD, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SPIPER, 16, 99);

  SKILL_CREATE("chant of subversion", PIPER_SUBVERSION,
               SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SPIPER, 56, 99);

  SKILL_CREATE("chime of the vampire", PIPER_CHIME_VAMPIRE,
               SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SPIPER, 41, 99);

  SKILL_CREATE("song of the night", PIPER_NIGHT, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SPIPER, 46, 99);

  SKILL_CREATE("draconic crescendo", PIPER_DRACONIC_CRESCENDO,
               SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SPIPER, 31, 99);

  SKILL_CREATE("elegy of nightmares", PIPER_ELEGY_NIGHTMARES,
               SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SPIPER, 6, 99);

  SKILL_CREATE("funereal hymn", PIPER_FUNEREAL, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SPIPER, 16, 99);

  SKILL_CREATE("song of the unholy", PIPER_UNHOLY, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SPIPER, 36, 99);

  SKILL_CREATE("hymn of morbid visions", PIPER_MORBID_VISIONS,
               SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SPIPER, 46, 99);

  SKILL_CREATE("song of decay", PIPER_DECAY, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SPIPER, 36, 99);

  SKILL_CREATE("dirge of ghosts", PIPER_GHOSTS, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SPIPER, 31, 99);

  SKILL_CREATE("dirge of bones", PIPER_DIRGE_BONES, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SPIPER, 21, 99);

  SKILL_CREATE("stanza of the fury", PIPER_STANZA_FURY,
               SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SPIPER, 46, 99);

  SKILL_CREATE("terrorizing rhythms", PIPER_TERROR_RHYTHMS,
               SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SPIPER, 21, 99);

  SKILL_CREATE("hymn of the zombie", PIPER_HYMN_ZOMBIE,
               SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SPIPER, 21, 99);

  SKILL_CREATE("requiem of the crypts", PIPER_REQUIEM_CRYP,
               SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SPIPER, 26, 99);

  SKILL_CREATE("flute", INSTRUMENT_FLUTE, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_BARD, 5, 100);
  SKILL_ADD(CLASS_SPIPER, 5, 100);
  SKILL_CREATE("lyre", INSTRUMENT_LYRE, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_BARD, 5, 100);
  SKILL_ADD(CLASS_SPIPER, 5, 100);
  SKILL_CREATE("mandolin", INSTRUMENT_MANDOLIN, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_BARD, 5, 100);
  SKILL_ADD(CLASS_SPIPER, 5, 100);
  SKILL_CREATE("harp", INSTRUMENT_HARP, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_BARD, 5, 100);
  SKILL_ADD(CLASS_SPIPER, 5, 100);
  SKILL_CREATE("drums", INSTRUMENT_DRUMS, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_BARD, 5, 100);
  SKILL_ADD(CLASS_SPIPER, 5, 100);
  SKILL_CREATE("horn", INSTRUMENT_HORN, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_BARD, 5, 100);
  SKILL_ADD(CLASS_SPIPER, 5, 100);

  SKILL_CREATE_WITH_CAT("shriek", SKILL_SHRIEK, SKILL_TYPE_NONE, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_BARD, 36, 99, SPEC_DISHARMONIST);
  SPEC_SKILL_ADD(CLASS_SPIPER, 36, 99, SPEC_DISHARMONIST);

  SKILL_CREATE("legend lore", SKILL_LEGEND_LORE, SKILL_TYPE_MENTAL_INT);
  SPEC_SKILL_ADD(CLASS_BARD, 36, 99, SPEC_MINSTREL);
  SPEC_SKILL_ADD(CLASS_SPIPER, 36, 99, SPEC_MINSTREL);

  SKILL_CREATE("song of drifting", SONG_DRIFTING, SKILL_TYPE_MENTAL_INT);
  SPEC_SKILL_ADD(CLASS_BARD, 51, 99, SPEC_MINSTREL);
  SPEC_SKILL_ADD(CLASS_SPIPER, 51, 99, SPEC_MINSTREL);

  SKILL_CREATE_WITH_CAT("circle", SKILL_CIRCLE, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_ROGUE, 1, 80);
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 75);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_THIEF);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_BRAVO);

  SPELL_CREATE("airy water", SPELL_AIRY_WATER, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 4, TRUE, TAR_IGNORE, 0, spell_airy_water,
               NULL);
  SPELL_ADD(CLASS_CONJURER, 7);

  SPELL_CREATE("globe of invulnerability", SPELL_GLOBE,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST * 3, TRUE, TAR_CHAR_ROOM,
               0, spell_globe,
               "Your globe shimmers and fades into thin air.");
  SPELL_ADD(CLASS_CONJURER, 8);
  //SPELL_ADD(CLASS_SORCERER, 12);

  SPELL_CREATE("group globe of invulnerability", SPELL_GROUP_GLOBE,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST * 7, TRUE, TAR_IGNORE, 0,
               spell_group_globe,
               "Your globe shimmers and fades into thin air.");

  SPELL_ADD(CLASS_CONJURER, 11);

  SKILL_CREATE("scribe", SKILL_SCRIBE, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_SORCERER, 1, 99);
  SKILL_ADD(CLASS_NECROMANCER, 1, 99);
  SKILL_ADD(CLASS_CONJURER, 1, 99);
  SKILL_ADD(CLASS_RANGER, 1, 80);
  SKILL_ADD(CLASS_BARD, 1, 50);
  SKILL_ADD(CLASS_REAVER, 1, 50);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 99);
  SKILL_ADD(CLASS_SPIPER, 1, 50);


  SKILL_CREATE("quick chant", SKILL_QUICK_CHANT, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_PALADIN, 1, 70);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 70);
  SKILL_ADD(CLASS_SORCERER, 1, 99);
  SKILL_ADD(CLASS_WARLOCK, 1, 99);
  SKILL_ADD(CLASS_NECROMANCER, 1, 99);
  SKILL_ADD(CLASS_CONJURER, 1, 99);
  SKILL_ADD(CLASS_PSIONICIST, 1, 99);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 99);
  SKILL_ADD(CLASS_SHAMAN, 1, 90);
  SKILL_ADD(CLASS_CLERIC, 1, 99);
//  SKILL_ADD(CLASS_DRUID, 1, 99);
  SKILL_ADD(CLASS_RANGER, 1, 70);
  SKILL_ADD(CLASS_BARD, 1, 70);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 99);
  SKILL_ADD(CLASS_SPIPER, 1, 70);
  SKILL_ADD(CLASS_REAVER, 11, 70);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 99);


  SKILL_CREATE("clerical spell knowledge", SKILL_SPELL_KNOWLEDGE_CLERICAL,
               SKILL_TYPE_MENTAL_WIS);
  SKILL_ADD(CLASS_PALADIN, 10, 80);
  SKILL_ADD(CLASS_ANTIPALADIN, 10, 80);
  SKILL_ADD(CLASS_CLERIC, 1, 99);
  SKILL_ADD(CLASS_DRUID, 1, 99);
  SKILL_ADD(CLASS_RANGER, 10, 80);
  SKILL_ADD(CLASS_WARLOCK, 10, 80);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 80);

  SKILL_CREATE("sorcerous spell knowledge", SKILL_SPELL_KNOWLEDGE_MAGICAL,
               SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_ANTIPALADIN, 10, 80);
  SKILL_ADD(CLASS_SORCERER, 1, 99);
  SKILL_ADD(CLASS_WARLOCK, 1, 99);
  SKILL_ADD(CLASS_NECROMANCER, 1, 99);
  SKILL_ADD(CLASS_CONJURER, 1, 99);
  SKILL_ADD(CLASS_RANGER, 10, 80);
  SKILL_ADD(CLASS_BARD, 10, 50);
  SKILL_ADD(CLASS_REAVER, 10, 80);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 99);
  SKILL_ADD(CLASS_SPIPER, 10, 50);

  SKILL_CREATE("shaman spell knowledge", SKILL_SPELL_KNOWLEDGE_SHAMAN,
               SKILL_TYPE_MENTAL_WIS);
  SKILL_ADD(CLASS_SHAMAN, 1, 99);

  SKILL_CREATE("summon mount", SKILL_SUMMON_MOUNT, SKILL_TYPE_MENTAL_WIS);
  SKILL_ADD(CLASS_PALADIN, 1, 99);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 99);

  SKILL_CREATE("spellcast generic", SKILL_CAST_GENERIC,
               SKILL_TYPE_MENTAL_INT);
#if 0
  SKILL_ADD(CLASS_PALADIN, 10, 70);
  SKILL_ADD(CLASS_ANTIPALADIN, 10, 70);
  SKILL_ADD(CLASS_SORCERER, 1, 90);
  SKILL_ADD(CLASS_NECROMANCER, 1, 90);
  SKILL_ADD(CLASS_CONJURER, 1, 90);
  SKILL_ADD(CLASS_SHAMAN, 1, 80);
  SKILL_ADD(CLASS_CLERIC, 1, 90);
  SKILL_ADD(CLASS_DRUID, 1, 90);
  SKILL_ADD(CLASS_RANGER, 10, 70);
#endif
  SKILL_CREATE("spellcast fire", SKILL_CAST_FIRE, SKILL_TYPE_MENTAL_INT);
#if 0
  SKILL_ADD(CLASS_PALADIN, 10, 70);
  SKILL_ADD(CLASS_ANTIPALADIN, 10, 70);
  SKILL_ADD(CLASS_SORCERER, 1, 90);
  SKILL_ADD(CLASS_NECROMANCER, 1, 90);
  SKILL_ADD(CLASS_CONJURER, 1, 90);
  SKILL_ADD(CLASS_CLERIC, 1, 90);
  SKILL_ADD(CLASS_DRUID, 1, 90);
  SKILL_ADD(CLASS_RANGER, 10, 70);
#endif
  SKILL_CREATE("spellcast cold", SKILL_CAST_COLD, SKILL_TYPE_MENTAL_INT);
#if 0
  SKILL_ADD(CLASS_PALADIN, 10, 70);
  SKILL_ADD(CLASS_ANTIPALADIN, 10, 70);
  SKILL_ADD(CLASS_SORCERER, 1, 90);
  SKILL_ADD(CLASS_NECROMANCER, 1, 90);
  SKILL_ADD(CLASS_CONJURER, 1, 90);
/*  SKILL_ADD (CLASS_SHAMAN, 1, 80); */
  SKILL_ADD(CLASS_CLERIC, 1, 90);
  SKILL_ADD(CLASS_DRUID, 1, 90);
  SKILL_ADD(CLASS_RANGER, 10, 70);
#endif
  SKILL_CREATE("spellcast healing", SKILL_CAST_HEALING,
               SKILL_TYPE_MENTAL_INT);
#if 0
  SKILL_ADD(CLASS_PALADIN, 10, 70);
  SKILL_ADD(CLASS_ANTIPALADIN, 10, 70);
  SKILL_ADD(CLASS_SORCERER, 1, 90);
  SKILL_ADD(CLASS_NECROMANCER, 1, 90);
  SKILL_ADD(CLASS_CONJURER, 1, 90);
/*  SKILL_ADD (CLASS_SHAMAN, 1, 80); */
  SKILL_ADD(CLASS_CLERIC, 1, 90);
  SKILL_ADD(CLASS_DRUID, 1, 90);
  SKILL_ADD(CLASS_RANGER, 10, 70);
#endif
  SKILL_CREATE("spellcast teleportation", SKILL_CAST_TELEPORTATION,
               SKILL_TYPE_MENTAL_INT);
#if 0
  SKILL_ADD(CLASS_PALADIN, 10, 70);
  SKILL_ADD(CLASS_ANTIPALADIN, 10, 70);
  SKILL_ADD(CLASS_SORCERER, 1, 90);
  SKILL_ADD(CLASS_NECROMANCER, 1, 90);
  SKILL_ADD(CLASS_CONJURER, 1, 90);
/*  SKILL_ADD (CLASS_SHAMAN, 1, 80); */
  SKILL_ADD(CLASS_CLERIC, 1, 90);
  SKILL_ADD(CLASS_DRUID, 1, 90);
  SKILL_ADD(CLASS_RANGER, 10, 70);
#endif
  SKILL_CREATE("spellcast summoning", SKILL_CAST_SUMMONING,
               SKILL_TYPE_MENTAL_INT);
#if 0
  SKILL_ADD(CLASS_PALADIN, 10, 70);
  SKILL_ADD(CLASS_ANTIPALADIN, 10, 70);
  SKILL_ADD(CLASS_SORCERER, 1, 90);
  SKILL_ADD(CLASS_NECROMANCER, 1, 90);
  SKILL_ADD(CLASS_CONJURER, 1, 90);
/*  SKILL_ADD (CLASS_SHAMAN, 1, 80); */
  SKILL_ADD(CLASS_CLERIC, 1, 90);
  SKILL_ADD(CLASS_DRUID, 1, 90);
  SKILL_ADD(CLASS_RANGER, 10, 70);
#endif
  SKILL_CREATE("spellcast protection", SKILL_CAST_PROTECTION,
               SKILL_TYPE_MENTAL_INT);
#if 0
  SKILL_ADD(CLASS_PALADIN, 10, 70);
  SKILL_ADD(CLASS_ANTIPALADIN, 10, 70);
  SKILL_ADD(CLASS_SORCERER, 1, 90);
  SKILL_ADD(CLASS_NECROMANCER, 1, 90);
  SKILL_ADD(CLASS_CONJURER, 1, 90);
/*  SKILL_ADD (CLASS_SHAMAN, 1, 80); */
  SKILL_ADD(CLASS_CLERIC, 1, 90);
  SKILL_ADD(CLASS_DRUID, 1, 90);
  SKILL_ADD(CLASS_RANGER, 10, 70);
#endif
  SKILL_CREATE("spellcast divination", SKILL_CAST_DIVINATION,
               SKILL_TYPE_MENTAL_INT);
#if 0
  SKILL_ADD(CLASS_PALADIN, 10, 70);
  SKILL_ADD(CLASS_ANTIPALADIN, 10, 70);
  SKILL_ADD(CLASS_SORCERER, 1, 90);
  SKILL_ADD(CLASS_NECROMANCER, 1, 90);
  SKILL_ADD(CLASS_CONJURER, 1, 90);
  SKILL_ADD(CLASS_CLERIC, 1, 90);
  SKILL_ADD(CLASS_DRUID, 1, 90);
  SKILL_ADD(CLASS_RANGER, 10, 70);
#endif
  SKILL_CREATE("spellcast enchantment", SKILL_CAST_ENCHANT,
               SKILL_TYPE_MENTAL_INT);
#if 0
  SKILL_ADD(CLASS_SORCERER, 1, 90);
  SKILL_ADD(CLASS_NECROMANCER, 1, 90);
  SKILL_ADD(CLASS_CONJURER, 1, 90);
  SKILL_ADD(CLASS_CLERIC, 1, 90);
  SKILL_ADD(CLASS_DRUID, 1, 90);
  SKILL_ADD(CLASS_CLERIC, 20, 90);
#endif

  /* shaman spellcasting skills */

  SKILL_CREATE("spellcast animal", SKILL_CAST_SH_ANIMAL,
               SKILL_TYPE_MENTAL_INT);
#if 0
  SKILL_ADD(CLASS_SHAMAN, 1, 90);
#endif

  SKILL_CREATE("spellcast elemental", SKILL_CAST_SH_ELEMENT,
               SKILL_TYPE_MENTAL_INT);
#if 0
  SKILL_ADD(CLASS_SHAMAN, 1, 90);
#endif

  SKILL_CREATE("spellcast spirit", SKILL_CAST_SH_SPIRIT,
               SKILL_TYPE_MENTAL_INT);
#if 0
  SKILL_ADD(CLASS_SHAMAN, 1, 90);
#endif

#if 1
  SKILL_CREATE_WITH_CAT("1h bludgeon", SKILL_1H_BLUDGEON, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_PSIONICIST, 1, 50);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 50);
  SKILL_ADD(CLASS_CLERIC, 1, 60);
  SKILL_ADD(CLASS_CONJURER, 1, 50);
  SKILL_ADD(CLASS_DRUID, 1, 60);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_SORCERER, 1, 50);
  SKILL_ADD(CLASS_WARLOCK, 1, 50);
  SKILL_ADD(CLASS_NECROMANCER, 1, 50);
  SKILL_ADD(CLASS_ROGUE, 1, 60);
  SKILL_ADD(CLASS_THIEF, 1, 60);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_SHAMAN, 1, 60);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 100);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 50);
  SPEC_SKILL_ADD(CLASS_CLERIC, 1, 90, SPEC_ZEALOT);
  SPEC_SKILL_ADD(CLASS_NECROMANCER, 1, 100, SPEC_NECROLYTE);

#endif
#if 0
  SKILL_CREATE("clubs/staves", SKILL_CLUB, SKILL_TYPE_PHYS_STR);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 60);
  SKILL_ADD(CLASS_BARD, 1, 50);
  SKILL_ADD(CLASS_PSIONICIST, 1, 80);
  SKILL_ADD(CLASS_CLERIC, 1, 100);
  SKILL_ADD(CLASS_CONJURER, 1, 80);
  SKILL_ADD(CLASS_DRUID, 1, 99);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_SORCERER, 1, 80);
  SKILL_ADD(CLASS_WARLOCK, 1, 80);
  SKILL_ADD(CLASS_NECROMANCER, 1, 80);
  SKILL_ADD(CLASS_ROGUE, 1, 60);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_SHAMAN, 1, 100);
  SKILL_ADD(CLASS_MONK, 1, 100);

  SKILL_CREATE("hammers", SKILL_HAMMER, SKILL_TYPE_PHYS_STR);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 60);
  SKILL_ADD(CLASS_BARD, 1, 50);
  SKILL_ADD(CLASS_PSIONICIST, 1, 80);
  SKILL_ADD(CLASS_CLERIC, 1, 100);
  SKILL_ADD(CLASS_CONJURER, 1, 80);
  SKILL_ADD(CLASS_DRUID, 1, 99);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_SORCERER, 1, 80);
  SKILL_ADD(CLASS_NECROMANCER, 1, 80);
  SKILL_ADD(CLASS_ROGUE, 1, 60);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_SHAMAN, 1, 100);

  SKILL_CREATE("maces", SKILL_MACE, SKILL_TYPE_PHYS_STR);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 60);
  SKILL_ADD(CLASS_BARD, 1, 50);
  SKILL_ADD(CLASS_PSIONICIST, 1, 80);
  SKILL_ADD(CLASS_CLERIC, 1, 100);
  SKILL_ADD(CLASS_CONJURER, 1, 80);
  SKILL_ADD(CLASS_DRUID, 1, 99);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_SORCERER, 1, 80);
  SKILL_ADD(CLASS_NECROMANCER, 1, 80);
  SKILL_ADD(CLASS_ROGUE, 1, 60);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_SHAMAN, 1, 100);
#endif
#if 1
  SKILL_CREATE_WITH_CAT("1h slashing", SKILL_1H_SLASHING, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_ASSASSIN, 1, 50);
  SKILL_ADD(CLASS_DRUID, 1, 60);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 60);
  SKILL_ADD(CLASS_MERCENARY, 1, 60);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_ROGUE, 1, 50);
  SKILL_ADD(CLASS_THIEF, 1, 50);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_REAVER, 1, 100);
  SPEC_SKILL_ADD(CLASS_NECROMANCER, 1, 70, SPEC_REAPER);
    
 
  
#endif
#if 0
  SKILL_CREATE("longswords", SKILL_LONGSWORD, SKILL_TYPE_PHYS_STR);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 50);
  SKILL_ADD(CLASS_BARD, 1, 80);
  SKILL_ADD(CLASS_PSIONICIST, 1, 80);
  SKILL_ADD(CLASS_DRUID, 1, 80);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_ROGUE, 1, 50);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);

  SKILL_CREATE("shortswords", SKILL_SHORTSWORD, SKILL_TYPE_PHYS_STR);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 50);
  SKILL_ADD(CLASS_BARD, 1, 80);
  SKILL_ADD(CLASS_PSIONICIST, 1, 80);
  SKILL_ADD(CLASS_DRUID, 1, 80);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_ROGUE, 1, 50);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);

  SKILL_CREATE("sickles", SKILL_SICKLE, SKILL_TYPE_PHYS_STR);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 50);
  SKILL_ADD(CLASS_BARD, 1, 90);
  SKILL_ADD(CLASS_PSIONICIST, 1, 80);
  SKILL_ADD(CLASS_DRUID, 1, 80);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_ROGUE, 1, 50);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
#endif
#if 1
  SKILL_CREATE_WITH_CAT("1h piercing", SKILL_1H_PIERCING, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SKILL_ADD(CLASS_BARD, 1, 70);
  SKILL_ADD(CLASS_PSIONICIST, 1, 50);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 50);
  SKILL_ADD(CLASS_CONJURER, 1, 50);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_NECROMANCER, 1, 50);
  SKILL_ADD(CLASS_RANGER, 1, 70);
  SKILL_ADD(CLASS_SORCERER, 1, 50);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 50);
  SKILL_ADD(CLASS_WARLOCK, 1, 50);
  SKILL_ADD(CLASS_ROGUE, 1, 100);
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 70);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 50);
  SKILL_ADD(CLASS_SPIPER, 1, 70);

#endif
#if 0
  SKILL_CREATE("axes", SKILL_AXE, SKILL_TYPE_PHYS_STR);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);

  SKILL_CREATE("daggers/knives", SKILL_DAGGER, SKILL_TYPE_PHYS_STR);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SKILL_ADD(CLASS_BARD, 1, 50);
  SKILL_ADD(CLASS_PSIONICIST, 1, 85);
  SKILL_ADD(CLASS_CONJURER, 1, 85);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_NECROMANCER, 1, 85);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_SORCERER, 1, 85);
  SKILL_ADD(CLASS_ROGUE, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);

  SKILL_CREATE("horns", SKILL_HORN, SKILL_TYPE_PHYS_STR);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SKILL_ADD(CLASS_BARD, 1, 50);
  SKILL_ADD(CLASS_PSIONICIST, 1, 85);
  SKILL_ADD(CLASS_CONJURER, 1, 85);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_NECROMANCER, 1, 85);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_SORCERER, 1, 85);
  SKILL_ADD(CLASS_ROGUE, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);

  SKILL_CREATE("polearms", SKILL_POLEARM, SKILL_TYPE_PHYS_STR);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_MONK, 1, 100);
#endif
#if 1
  SKILL_CREATE_WITH_CAT("1h flaying", SKILL_1H_FLAYING, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_WARRIOR, 1, 75);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);
#endif
#if 1
  SKILL_CREATE_WITH_CAT("2h bludgeon", SKILL_2H_BLUDGEON, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_CLERIC, 1, 60);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_SHAMAN, 1, 60);
  SKILL_ADD(CLASS_SORCERER, 1, 60);
  SKILL_ADD(CLASS_WARLOCK, 1, 90);
  SKILL_ADD(CLASS_NECROMANCER, 1, 60);
  SKILL_ADD(CLASS_CONJURER, 1, 60);
  SKILL_ADD(CLASS_PSIONICIST, 1, 75);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 75);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 60);
  SKILL_ADD(CLASS_DREADLORD, 1, 100);
  SPEC_SKILL_ADD(CLASS_CLERIC, 1, 90, SPEC_ZEALOT);

  SKILL_CREATE_WITH_CAT("2h slashing", SKILL_2H_SLASHING, SKILL_TYPE_PHYS_STR,SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 60);
  SKILL_ADD(CLASS_DREADLORD, 1, 100);
  SKILL_ADD(CLASS_AVENGER, 1, 100);
#endif
#if 0
  SKILL_CREATE("two-handed swords", SKILL_TWOHANDED_SWORD,
               SKILL_TYPE_PHYS_STR);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 80);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 70);
  SKILL_ADD(CLASS_WARRIOR, 1, 90);
#endif
  SKILL_CREATE_WITH_CAT("2h flaying", SKILL_2H_FLAYING, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_WARRIOR, 1, 75);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);

  SKILL_CREATE_WITH_CAT("ranged weapons", SKILL_ARCHERY, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);  /* TASFALEN */
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_THIEF, 1, 75);
  SKILL_ADD(CLASS_ROGUE, 1, 75);
  SKILL_ADD(CLASS_ASSASSIN, 1, 75);

  SKILL_CREATE_WITH_CAT("reach weapons", SKILL_REACH_WEAPONS, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_WARRIOR, 25, 80);
  SKILL_ADD(CLASS_MERCENARY, 30, 90);

  SKILL_CREATE_WITH_CAT("blindfighting", SKILL_BLINDFIGHTING, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 70);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 70);
  SKILL_ADD(CLASS_RANGER, 1, 85);
  SKILL_ADD(CLASS_ASSASSIN, 1, 99);
  SKILL_ADD(CLASS_MONK, 1, 80);
  SKILL_ADD(CLASS_ROGUE, 1, 90);
  SKILL_ADD(CLASS_THIEF, 1, 90);
  SKILL_ADD(CLASS_BERSERKER, 6, 100);
  SKILL_ADD(CLASS_REAVER, 11, 85);

  SPELL_CREATE("embalm", SPELL_EMBALM, SPELLTYPE_GENERIC, PULSE_SPELLCAST
               * 3, FALSE, TAR_OBJ_ROOM, 0, spell_embalm, NULL);
  SPELL_ADD(CLASS_NECROMANCER, 3);

  SPELL_CREATE("mass embalm", SPELL_MASS_EMBALM, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3, FALSE, TAR_IGNORE, 0, spell_mass_embalm,
               NULL);
  SPELL_ADD(CLASS_NECROMANCER, 10);

  SPELL_CREATE("vampiric trance", SPELL_VAMPIRE, SPELL_TYPE_GENERIC,
               PULSE_SPELLCAST * 2, FALSE, TAR_SELF_ONLY, 0, spell_vampire,
               "Color fades slowly into your face.");
  SPELL_ADD(CLASS_NECROMANCER, 10);

  SPELL_CREATE("divine blessing", SPELL_DIVINE_BLESSING, SPELL_TYPE_GENERIC,
               PULSE_SPELLCAST * 3, TRUE, TAR_SELF_ONLY, 0, spell_divine_blessing,
               "&+WYou feel detached from your weapon.");
  SPELL_ADD(CLASS_PALADIN, 12);

  SPELL_CREATE("elemental form", SPELL_ELEMENTAL_FORM, SPELL_TYPE_GENERIC,
               PULSE_SPELLCAST * 4, FALSE, TAR_SELF_ONLY, 0,
               spell_elemental_form, "&+BYou return to your normal form.&N");

  SPELL_CREATE("elemental aura", SPELL_ELEMENTAL_AURA, SPELL_TYPE_GENERIC,
               PULSE_SPELLCAST * 4, FALSE, TAR_SELF_ONLY, 0,
               spell_elemental_aura, "&+BYour aura returns to normal.&N");
  SPELL_ADD(CLASS_DRUID, 11);
 //SPEC_SPELL_ADD(CLASS_DRUID, 11, SPEC_STORM);
 
  SPELL_CREATE("wind tunnel", SPELL_WIND_TUNNEL, SPELLTYPE_ELEMENTAL, 
						   PULSE_SPELLCAST * 1, FALSE, TAR_IGNORE, 0, spell_wind_tunnel, NULL);
	SPEC_SPELL_ADD(CLASS_DRUID, 8, SPEC_STORM);
	SPEC_SPELL_ADD(CLASS_SHAMAN, 9, SPEC_ELEMENTALIST);

	SPELL_CREATE("binding wind", SPELL_BINDING_WIND, SPELLTYPE_ELEMENTAL,
					     PULSE_SPELLCAST * 1, FALSE, TAR_IGNORE, 0, spell_binding_wind, NULL);
	SPEC_SPELL_ADD(CLASS_DRUID, 10, SPEC_STORM);
  SPEC_SPELL_ADD(CLASS_SHAMAN, 11, SPEC_ELEMENTALIST);

  SPELL_CREATE("vampiric touch", SPELL_VAMPIRIC_TOUCH, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 2, TRUE, TAR_SELF_ONLY, 0,
               spell_vampiric_touch, "Your hands cease to glow &+Rred.&n");
  SPELL_ADD(CLASS_NECROMANCER, 2);

  SPELL_CREATE("protect undead", SPELL_PROT_UNDEAD, SPELLTYPE_PROTECTION,
               PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM, 0, spell_prot_undead,
               NULL);
  SPELL_ADD(CLASS_NECROMANCER, 7);

  SPELL_CREATE("protection from undead", SPELL_PROT_FROM_UNDEAD,
               SPELLTYPE_PROTECTION, PULSE_SPELLCAST * 2, FALSE,
               TAR_CHAR_ROOM, 0, spell_prot_from_undead,
               "You no longer feel safe from &+Lundead&n.");
  SPELL_ADD(CLASS_NECROMANCER, 1);
  SPEC_SPELL_ADD(CLASS_CLERIC, 7, SPEC_ZEALOT);
	SPEC_SPELL_ADD(CLASS_CLERIC, 8, SPEC_HOLYMAN);
	
  SPELL_CREATE("command horde", SPELL_COMMAND_HORDE, SPELLTYPE_GENERIC,
               0, TRUE, TAR_IGNORE, 0, spell_command_horde, NULL);
  // SPELL_ADD(CLASS_NECROMANCER, 9);

  SPELL_CREATE("heal undead", SPELL_HEAL_UNDEAD, SPELLTYPE_HEALING,
               PULSE_SPELLCAST * 3 / 2, TRUE, TAR_CHAR_ROOM, 0,
               spell_heal_undead, NULL);
  SPELL_ADD(CLASS_NECROMANCER, 5);
  SPELL_ADD(CLASS_WARLOCK, 5);

  SPELL_CREATE("greater heal undead", SPELL_GREATER_HEAL_UNDEAD,
               SPELLTYPE_HEALING, PULSE_SPELLCAST * 2, TRUE, TAR_CHAR_ROOM, 0,
               spell_greater_heal_undead, NULL);
  SPELL_ADD(CLASS_WARLOCK, 7);

  SPELL_CREATE("entangle", SPELL_ENTANGLE,
               SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 1, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT, 1,
               spell_entangle, "You are able to move normally again.");
//  SPELL_ADD(CLASS_DRUID, 8);
  SPEC_SPELL_ADD(CLASS_DRUID, 8, SPEC_WOODLAND);
	 
	
  SPELL_CREATE("create spring", SPELL_CREATE_SPRING, SPELLTYPE_SUMMONING,
               PULSE_SPELLCAST * 6, TRUE, TAR_IGNORE, 0, spell_create_spring,
               NULL);
  SPELL_ADD(CLASS_DRUID, 4);

  SPELL_CREATE("barkskin", SPELL_BARKSKIN, SPELLTYPE_PROTECTION,
               PULSE_SPELLCAST, TRUE, TAR_CHAR_ROOM, 0, spell_barkskin,
               "Your skin loses its &+ybarklike &ntexture.");
  SPELL_ADD(CLASS_DRUID, 1);
  SPELL_ADD(CLASS_RANGER, 5);

  SPELL_CREATE("mass barkskin", SPELL_MASS_BARKSKIN, SPELLTYPE_PROTECTION,
               PULSE_SPELLCAST * 3, TRUE, TAR_IGNORE, 0, spell_mass_barkskin,
               "Your skin loses its &+ybarklike &ntexture.");
//  SPELL_ADD(CLASS_DRUID, 10);
  SPELL_ADD(CLASS_RANGER, 11);

  SPELL_CREATE("moonwell", SPELL_MOONWELL, SPELLTYPE_TELEPORTATION,
               PULSE_SPELLCAST * 10, FALSE, TAR_CHAR_WORLD, 0, spell_moonwell,
               NULL);
  SPELL_ADD(CLASS_DRUID, 10);

  SPELL_CREATE("shadow gate", SPELL_SHADOW_GATE, SPELLTYPE_TELEPORTATION,
               PULSE_SPELLCAST * 10, FALSE, TAR_CHAR_WORLD, 0,
               spell_shadow_gate, NULL);
  SPELL_ADD(CLASS_WARLOCK, 10);

  SPELL_CREATE("moonstone", SPELL_MOONSTONE, SPELLTYPE_TELEPORTATION,
               PULSE_SPELLCAST * 6, FALSE, TAR_IGNORE, 0, cast_moonstone,
               NULL);
  SPELL_ADD(CLASS_DRUID, 12);

  SPELL_CREATE("create dracolich", SPELL_CREATE_DRACOLICH,
               SPELLTYPE_SUMMONING, PULSE_SPELLCAST * 9, FALSE, TAR_OBJ_ROOM,
               0, spell_create_dracolich, NULL);
  SPELL_ADD(CLASS_NECROMANCER, 11);

  SPELL_CREATE("napalm", SPELL_NAPALM, SPELLTYPE_FIRE,
               PULSE_SPELLCAST * 4 / 3, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_napalm, NULL);

  SPELL_CREATE("strong acid", SPELL_STRONG_ACID, SPELLTYPE_FIRE,
               PULSE_SPELLCAST * 3/2, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_strong_acid, NULL);

  SPELL_CREATE("glass bomb", SPELL_GLASS_BOMB, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3/2, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_glass_bomb, NULL);

  SPELL_CREATE("grease", SPELL_GREASE, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3/2, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_grease, NULL);

  SPELL_CREATE("nitrogen", SPELL_NITROGEN, SPELLTYPE_GENERIC,
               PULSE_SPELLCAST * 3/2, TRUE,
               TAR_CHAR_ROOM | TAR_FIGHT_VICT,
               1, spell_nitrogen, NULL);

  SKILL_CREATE_WITH_CAT("sneak", SKILL_SNEAK, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_ROGUE, 1, 70);
  SKILL_ADD(CLASS_ASSASSIN, 1, 95);
  SKILL_ADD(CLASS_BARD, 1, 70);
  SKILL_ADD(CLASS_SPIPER, 1, 70);

  SKILL_CREATE_WITH_CAT("hide", SKILL_HIDE, SKILL_TYPE_NONE, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_ROGUE, 1, 95);
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 95);
  SKILL_ADD(CLASS_MERCENARY, 1, 80);
  SKILL_ADD(CLASS_BARD, 1, 70);
  SKILL_ADD(CLASS_SPIPER, 1, 70);
  SPEC_SKILL_ADD(CLASS_RANGER, 41, 70, SPEC_WOODSMAN);

  SKILL_CREATE("steal", SKILL_STEAL, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_THIEF);


  SKILL_CREATE_WITH_CAT("evade", SKILL_EVADE, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_THIEF, 56, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 56, 100, SPEC_THIEF);
  
  SKILL_CREATE_WITH_CAT("backstab", SKILL_BACKSTAB, SKILL_TYPE_PHYS_DEX ,SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_ROGUE, 1, 90);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SKILL_ADD(CLASS_THIEF, 1, 90);
  SKILL_ADD(CLASS_MERCENARY, 1, 85);
  SKILL_ADD(CLASS_BARD, 1, 85);
  SKILL_ADD(CLASS_SPIPER, 1, 85);
  SPEC_SKILL_ADD(CLASS_BARD, 1, 95, SPEC_SCOUNDREL);
  SPEC_SKILL_ADD(CLASS_SPIPER, 1, 95, SPEC_SCOUNDREL);
  SPEC_SKILL_ADD(CLASS_ROGUE, 1, 100, SPEC_ASSASSIN);

  SKILL_CREATE_WITH_CAT("critical stab", SKILL_CRITICAL_STAB, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_ASSASSIN, 51, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 51, 100, SPEC_ASSASSIN);

  SKILL_CREATE("pick lock", SKILL_PICK_LOCK, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_ROGUE, 1, 90);
  SKILL_ADD(CLASS_THIEF, 1, 99);
  SKILL_ADD(CLASS_ASSASSIN, 1, 90);

  SKILL_CREATE_WITH_CAT("kick", SKILL_KICK, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 95);
  SKILL_ADD(CLASS_RANGER, 1, 80);
  SKILL_ADD(CLASS_MERCENARY, 1, 85);
  SKILL_ADD(CLASS_PALADIN, 1, 95);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SKILL_ADD(CLASS_DREADLORD, 1, 90);
  SKILL_ADD(CLASS_AVENGER, 1, 90);

  SKILL_CREATE_WITH_CAT("roundkick", SKILL_ROUNDKICK, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_MONK, 1, 99);

  SKILL_CREATE_WITH_CAT("throat crush", SKILL_THROAT_CRUSH, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_MERCENARY, 51, 100);

  SKILL_CREATE_WITH_CAT("guard", SKILL_GUARD, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 80);
  SKILL_ADD(CLASS_PALADIN, 1, 60);

  SKILL_CREATE_WITH_CAT("defend", SKILL_DEFEND, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_PALADIN, 31, 100);

  SKILL_CREATE_WITH_CAT("bash", SKILL_BASH, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 80);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);

 
  
  SKILL_CREATE_WITH_CAT("mounted combat", SKILL_MOUNTED_COMBAT, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_PALADIN, 10, 99);
  SKILL_ADD(CLASS_ANTIPALADIN, 10, 99);

  SKILL_CREATE_WITH_CAT("rescue", SKILL_RESCUE, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 75);
  SKILL_ADD(CLASS_MERCENARY, 1, 75);
  SKILL_ADD(CLASS_ANTIPALADIN, 31, 100);
  SKILL_ADD(CLASS_BARD, 31, 50);
  SKILL_ADD(CLASS_SPIPER, 31, 50);


  SKILL_CREATE("trap", SKILL_TRAP, SKILL_TYPE_PHYS_DEX);
  SPEC_SKILL_ADD(CLASS_RANGER, 31, 100, SPEC_WOODSMAN);
  SPEC_SKILL_ADD(CLASS_MERCENARY, 51, 100, SPEC_BOUNTY);
  SKILL_ADD(CLASS_THIEF, 51, 100);
  SKILL_ADD(CLASS_ROGUE, 51, 100);

  SKILL_CREATE_WITH_CAT("track", SKILL_TRACK, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_RANGER, 1, 99);
  SKILL_ADD(CLASS_ROGUE, 1, 95);
  SKILL_ADD(CLASS_THIEF, 1, 95);
  SKILL_ADD(CLASS_ASSASSIN, 1, 95);
  SKILL_ADD(CLASS_MERCENARY, 1, 95);

  SKILL_CREATE("listen", SKILL_LISTEN, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_THIEF, 15, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 15, 100, SPEC_THIEF);
  SKILL_ADD(CLASS_BARD, 15, 80);
  SKILL_ADD(CLASS_SPIPER, 15, 80);
  SPEC_SKILL_ADD(CLASS_MERCENARY, 15, 80, SPEC_BOUNTY);


  SKILL_CREATE_WITH_CAT("disarm", SKILL_DISARM, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_ROGUE, 1, 80);
  SKILL_ADD(CLASS_THIEF, 1, 80);
  SKILL_ADD(CLASS_WARRIOR, 1, 90);
  SKILL_ADD(CLASS_ASSASSIN, 1, 90);

  SKILL_CREATE_WITH_CAT("double attack", SKILL_DOUBLE_ATTACK, SKILL_TYPE_NONE, SKILL_CATEGORY_OFFENSIVE);
  SPEC_SKILL_ADD(CLASS_CLERIC, 51, 75, SPEC_ZEALOT);  
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 95);
  SKILL_ADD(CLASS_ROGUE, 1, 95);
  SKILL_ADD(CLASS_THIEF, 1, 85);
  SKILL_ADD(CLASS_ASSASSIN, 1, 95);
  SKILL_ADD(CLASS_BARD, 1, 60);
  SKILL_ADD(CLASS_DRUID, 1, 75);
  //SKILL_ADD(CLASS_CLERIC, 56, 50);
  SKILL_ADD(CLASS_ALCHEMIST, 11, 75);
  SKILL_ADD(CLASS_BERSERKER, 11, 100);
  SKILL_ADD(CLASS_REAVER, 1, 95);
  SKILL_ADD(CLASS_SPIPER, 1, 60);
  SKILL_ADD(CLASS_DREADLORD, 1, 100);
  SKILL_ADD(CLASS_AVENGER, 1, 100);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 90);

  SKILL_CREATE_WITH_CAT("triple attack", SKILL_TRIPLE_ATTACK, SKILL_TYPE_NONE, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_WARRIOR, 51, 100);
  SKILL_ADD(CLASS_RANGER, 46, 100);
  //SKILL_ADD(CLASS_MERCENARY, 56, 60);
  SKILL_ADD(CLASS_BERSERKER, 46, 100);
  //SKILL_ADD(CLASS_PALADIN, 56, 90);
  //SKILL_ADD(CLASS_ANTIPALADIN, 56, 90);
  SKILL_ADD(CLASS_DREADLORD, 51, 100);
  SKILL_ADD(CLASS_AVENGER, 51, 100);
  SPEC_SKILL_ADD(CLASS_ALCHEMIST, 51, 70, SPEC_BATTLE_FORGER);
  SPEC_SKILL_ADD(CLASS_ROGUE, 51, 80, SPEC_BRAVO);
  
  SKILL_CREATE_WITH_CAT("dual wield", SKILL_DUAL_WIELD, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 90);
  SKILL_ADD(CLASS_ROGUE, 1, 90);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_BERSERKER, 11, 100);
  SKILL_ADD(CLASS_REAVER, 1, 100);

  SKILL_CREATE_WITH_CAT("hitall", SKILL_HITALL, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 11, 100);
  SKILL_ADD(CLASS_DREADLORD, 1, 90);
  SKILL_ADD(CLASS_AVENGER, 1, 90);

  SKILL_CREATE_WITH_CAT("tackle", SKILL_TACKLE, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_MERCENARY, 20, 100);

  SKILL_CREATE_WITH_MESSAGES_WITH_CAT("berserk", SKILL_BERSERK, SKILL_TYPE_PHYS_STR,
                             "Your blood cools, and you no longer see targets everywhere.",
                             "$n seems to have overcome $s battle madness.", SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);

  SKILL_CREATE_WITH_MESSAGES_WITH_CAT("rage", SKILL_RAGE, SKILL_TYPE_PHYS_STR,
                             "You feel yourself return to normal as your flurry abates.",
                             "$n returns to normal as $s flurry abates.", SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_BERSERKER, 6, 100);

  SKILL_CREATE_WITH_CAT("enrage", SKILL_ENRAGE, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_BERSERKER, 41, 100);

  SKILL_CREATE_WITH_CAT("maul", SKILL_MAUL, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_BERSERKER, 31, 100);

  SKILL_CREATE_WITH_CAT("vicious strike", SKILL_VICIOUS_STRIKE, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_BERSERKER, 51, 100);

  SKILL_CREATE_WITH_CAT("rush", SKILL_RUSH, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_BERSERKER, 51, 100);

  SKILL_CREATE_WITH_CAT("rampage", SKILL_RAMPAGE, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_BERSERKER, 36, 100);

  SKILL_CREATE_WITH_CAT("shield block", SKILL_SHIELD_BLOCK, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_MERCENARY, 1, 60);
  SKILL_ADD(CLASS_THIEF, 25, 81);
  SKILL_ADD(CLASS_RANGER, 1, 60);
  SKILL_ADD(CLASS_DRUID, 1, 50);
  SKILL_ADD(CLASS_BARD, 1, 60);
  SKILL_ADD(CLASS_SPIPER, 1, 60);
  SKILL_ADD(CLASS_REAVER, 1, 60);
  SPEC_SKILL_ADD(CLASS_CLERIC, 30, 60, SPEC_ZEALOT);
  SPEC_SKILL_ADD(CLASS_THIEF, 25, 81, SPEC_THIEF);

  SKILL_CREATE_WITH_CAT("parry", SKILL_PARRY, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 90);
  SKILL_ADD(CLASS_MERCENARY, 1, 85);
  SKILL_ADD(CLASS_RANGER, 1, 85);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 90);
  SKILL_ADD(CLASS_BARD, 1, 70);
  SKILL_ADD(CLASS_ROGUE, 20, 75);
  SKILL_ADD(CLASS_THIEF, 20, 75);
  SKILL_ADD(CLASS_ASSASSIN, 36, 70);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 70);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SKILL_ADD(CLASS_REAVER, 18, 85);
  SKILL_ADD(CLASS_SPIPER, 1, 70);
  SKILL_ADD(CLASS_DREADLORD, 1, 90);
  SKILL_ADD(CLASS_AVENGER, 1, 100);


  SKILL_CREATE_WITH_CAT("riposte", SKILL_RIPOSTE, SKILL_TYPE_PHYS_DEX, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_PALADIN, 1, 80);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 80);
  SKILL_ADD(CLASS_RANGER, 1, 75);
  SKILL_ADD(CLASS_MERCENARY, 1, 80);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 6, 100);
  SKILL_ADD(CLASS_REAVER, 26, 75);
  SKILL_ADD(CLASS_DREADLORD, 1, 90);
  SKILL_ADD(CLASS_AVENGER, 1, 90);
  SPEC_SKILL_ADD(CLASS_ALCHEMIST, 36, 70, SPEC_BATTLE_FORGER);
  SPEC_SKILL_ADD(CLASS_ROGUE, 36, 70, SPEC_BRAVO);

  SKILL_CREATE("surprise", SKILL_SURPRISE, SKILL_TYPE_NONE);
  SKILL_ADD(CLASS_RANGER, 1, 80);

  SKILL_CREATE_WITH_CAT("headbutt", SKILL_HEADBUTT, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);

  SKILL_CREATE("double headbutt", SKILL_DOUBLE_HEADBUTT, SKILL_TYPE_PHYS_STR);
  SKILL_ADD(CLASS_MERCENARY, 56, 100);
    
  SKILL_CREATE_WITH_CAT("meditate", SKILL_MEDITATE, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 80);
  SKILL_ADD(CLASS_PALADIN, 1, 80);
  SKILL_ADD(CLASS_BARD, 1, 75);
  SKILL_ADD(CLASS_SPIPER, 1, 75);
  SKILL_ADD(CLASS_CLERIC, 1, 100);
  SKILL_ADD(CLASS_CONJURER, 1, 100);
//  SKILL_ADD(CLASS_DRUID, 1, 95);
  SKILL_ADD(CLASS_RANGER, 1, 80);
  SKILL_ADD(CLASS_SORCERER, 1, 100);
  SKILL_ADD(CLASS_NECROMANCER, 1, 100);
  SKILL_ADD(CLASS_SHAMAN, 1, 95);
  SKILL_ADD(CLASS_PSIONICIST, 1, 100);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 100);
  SKILL_ADD(CLASS_REAVER, 21, 80);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 100);
  SKILL_ADD(CLASS_WARLOCK, 1, 100);

  SKILL_CREATE("apply poison", SKILL_APPLY_POISON, SKILL_TYPE_NONE);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SPEC_SKILL_ADD(CLASS_ROGUE, 40, 100, SPEC_ASSASSIN);
  
  SKILL_CREATE_WITH_CAT("dodge", SKILL_DODGE, SKILL_TYPE_PHYS_AGI, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 85);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SKILL_ADD(CLASS_BARD, 1, 70);
  SKILL_ADD(CLASS_PSIONICIST, 1, 70);
  SKILL_ADD(CLASS_CLERIC, 1, 60);
  SKILL_ADD(CLASS_CONJURER, 1, 50);
  SKILL_ADD(CLASS_DRUID, 1, 85);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_NECROMANCER, 1, 50);
  SKILL_ADD(CLASS_PALADIN, 1, 95);
  SKILL_ADD(CLASS_RANGER, 1, 90);
  SKILL_ADD(CLASS_SHAMAN, 1, 50);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 50);
  SKILL_ADD(CLASS_SORCERER, 1, 50);
  SKILL_ADD(CLASS_WARLOCK, 1, 90);
  SKILL_ADD(CLASS_ROGUE, 1, 95);
  SKILL_ADD(CLASS_THIEF, 1, 95);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 75);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SKILL_ADD(CLASS_REAVER, 1, 85);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 50);
  SKILL_ADD(CLASS_SPIPER, 1, 60);
  SKILL_ADD(CLASS_DREADLORD, 1, 95);
  SKILL_ADD(CLASS_AVENGER, 1, 100);
  SPEC_SKILL_ADD(CLASS_BARD, 1, 95, SPEC_SCOUNDREL);
  SPEC_SKILL_ADD(CLASS_SPIPER, 1, 95, SPEC_SCOUNDREL);
  SPEC_SKILL_ADD(CLASS_CLERIC, 1, 75,SPEC_ZEALOT);

  SKILL_CREATE("mount", SKILL_MOUNT, SKILL_TYPE_PHYS_CON);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_RANGER, 1, 90);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 90);
  SKILL_ADD(CLASS_PSIONICIST, 1, 90);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 90);
  SKILL_ADD(CLASS_SORCERER, 1, 90);
  SKILL_ADD(CLASS_WARLOCK, 1, 90);
  SKILL_ADD(CLASS_CLERIC, 1, 90);
  SKILL_ADD(CLASS_DRUID, 1, 90);
  SKILL_ADD(CLASS_CONJURER, 1, 90);
  SKILL_ADD(CLASS_NECROMANCER, 1, 90);
  SKILL_ADD(CLASS_ROGUE, 1, 90);
  SKILL_ADD(CLASS_THIEF, 1, 90);
  SKILL_ADD(CLASS_SHAMAN, 1, 90);
  SKILL_ADD(CLASS_ASSASSIN, 1, 90);
  SKILL_ADD(CLASS_BARD, 1, 90);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 70);
  SKILL_ADD(CLASS_REAVER, 1, 90);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 90);
  SKILL_ADD(CLASS_SPIPER, 1, 90);
  SKILL_ADD(CLASS_DREADLORD, 1, 90);

  SKILL_CREATE("bandage", SKILL_BANDAGE, SKILL_TYPE_MENTAL_WIS);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_RANGER, 1, 90);
  SKILL_ADD(CLASS_PALADIN, 1, 90);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 90);
  SKILL_ADD(CLASS_WARRIOR, 1, 90);
  SKILL_ADD(CLASS_SORCERER, 1, 90);
  SKILL_ADD(CLASS_WARLOCK, 1, 90);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 90);
  SKILL_ADD(CLASS_CLERIC, 1, 99);
  SKILL_ADD(CLASS_DRUID, 1, 90);
  SKILL_ADD(CLASS_CONJURER, 1, 90);
  SKILL_ADD(CLASS_NECROMANCER, 1, 90);
  SKILL_ADD(CLASS_ROGUE, 1, 90);
  SKILL_ADD(CLASS_THIEF, 1, 90);
  SKILL_ADD(CLASS_SHAMAN, 1, 90);
  SKILL_ADD(CLASS_ASSASSIN, 1, 90);
  SKILL_ADD(CLASS_BARD, 1, 90);
  SKILL_ADD(CLASS_PSIONICIST, 1, 90);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 90);
  SKILL_ADD(CLASS_MONK, 1, 90);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 80);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SKILL_ADD(CLASS_REAVER, 1, 75);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 90);
  SKILL_ADD(CLASS_SPIPER, 1, 90);
  SKILL_ADD(CLASS_DREADLORD, 1, 90);
  SKILL_ADD(CLASS_AVENGER, 1, 90);


  SKILL_CREATE("forge", SKILL_FORGE, SKILL_TYPE_MENTAL_WIS);
  SPEC_SKILL_ADD(CLASS_ALCHEMIST, 41, 100, SPEC_BLACKSMITH);

  SKILL_CREATE("mine", SKILL_MINE, SKILL_TYPE_MENTAL_WIS);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_SORCERER, 1, 100);
  SKILL_ADD(CLASS_WARLOCK, 1, 100);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 100);
  SKILL_ADD(CLASS_CLERIC, 1, 100);
  SKILL_ADD(CLASS_DRUID, 1, 100);
  SKILL_ADD(CLASS_CONJURER, 1, 100);
  SKILL_ADD(CLASS_NECROMANCER, 1, 100);
  SKILL_ADD(CLASS_ROGUE, 1, 100);
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SKILL_ADD(CLASS_SHAMAN, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SKILL_ADD(CLASS_BARD, 1, 100);
  SKILL_ADD(CLASS_PSIONICIST, 1, 100);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 100);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SKILL_ADD(CLASS_REAVER, 1, 100);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 100);
  SKILL_ADD(CLASS_SPIPER, 1, 100);
  SKILL_ADD(CLASS_DREADLORD, 1, 100);
  SKILL_ADD(CLASS_AVENGER, 1, 100);


  SKILL_CREATE("fishing", SKILL_FISHING, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_MERCENARY, 1, 100);
  SKILL_ADD(CLASS_RANGER, 1, 100);
  SKILL_ADD(CLASS_PALADIN, 1, 100);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 100);
  SKILL_ADD(CLASS_WARRIOR, 1, 100);
  SKILL_ADD(CLASS_SORCERER, 1, 100);
  SKILL_ADD(CLASS_WARLOCK, 1, 100);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 100);
  SKILL_ADD(CLASS_CLERIC, 1, 100);
  SKILL_ADD(CLASS_DRUID, 1, 100);
  SKILL_ADD(CLASS_CONJURER, 1, 100);
  SKILL_ADD(CLASS_NECROMANCER, 1, 100);
  SKILL_ADD(CLASS_ROGUE, 1, 100);
  SKILL_ADD(CLASS_THIEF, 1, 100);
  SKILL_ADD(CLASS_SHAMAN, 1, 100);
  SKILL_ADD(CLASS_ASSASSIN, 1, 100);
  SKILL_ADD(CLASS_BARD, 1, 100);
  SKILL_ADD(CLASS_PSIONICIST, 1, 100);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 100);
  SKILL_ADD(CLASS_MONK, 1, 100);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 100);
  SKILL_ADD(CLASS_BERSERKER, 1, 100);
  SKILL_ADD(CLASS_REAVER, 1, 100);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 100);
  SKILL_ADD(CLASS_SPIPER, 1, 100);
  SKILL_ADD(CLASS_DREADLORD, 1, 100);
  SKILL_ADD(CLASS_AVENGER, 1, 100);


  SKILL_CREATE("carve", SKILL_CARVE, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_MERCENARY, 1, 99);
  SKILL_ADD(CLASS_RANGER, 1, 80);
  SKILL_ADD(CLASS_PALADIN, 1, 80);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 90);
  SKILL_ADD(CLASS_WARRIOR, 1, 99);
  SKILL_ADD(CLASS_ROGUE, 1, 90);
  SKILL_ADD(CLASS_THIEF, 1, 90);
  SKILL_ADD(CLASS_SHAMAN, 1, 60);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 60);
  SKILL_ADD(CLASS_ASSASSIN, 1, 99);
  SKILL_ADD(CLASS_BARD, 1, 60);
  SKILL_ADD(CLASS_CLERIC, 1, 60);
  SKILL_ADD(CLASS_DRUID, 1, 60);
  SKILL_ADD(CLASS_PSIONICIST, 1, 60);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 60);
  SKILL_ADD(CLASS_CONJURER, 1, 60);
  SKILL_ADD(CLASS_NECROMANCER, 1, 60);
  SKILL_ADD(CLASS_SORCERER, 1, 60);
  SKILL_ADD(CLASS_WARLOCK, 1, 60);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 85);
  SKILL_ADD(CLASS_BERSERKER, 1, 60);
  SKILL_ADD(CLASS_REAVER, 1, 99);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 60);
  SKILL_ADD(CLASS_SPIPER, 1, 60);
  SKILL_ADD(CLASS_DREADLORD, 1, 90);
  SKILL_ADD(CLASS_AVENGER, 1, 90);


  SKILL_CREATE("swim", SKILL_SWIM, SKILL_TYPE_PHYS_DEX);
  SKILL_ADD(CLASS_MERCENARY, 1, 90);
  SKILL_ADD(CLASS_RANGER, 1, 90);
  SKILL_ADD(CLASS_PALADIN, 1, 90);
  SKILL_ADD(CLASS_ANTIPALADIN, 1, 90);
  SKILL_ADD(CLASS_WARRIOR, 1, 90);
  SKILL_ADD(CLASS_SORCERER, 1, 90);
  SKILL_ADD(CLASS_WARLOCK, 1, 90);
  SKILL_ADD(CLASS_CLERIC, 1, 99);
  SKILL_ADD(CLASS_DRUID, 1, 90);
  SKILL_ADD(CLASS_CONJURER, 1, 90);
  SKILL_ADD(CLASS_NECROMANCER, 1, 90);
  SKILL_ADD(CLASS_ROGUE, 1, 90);
  SKILL_ADD(CLASS_THIEF, 1, 90);
  SKILL_ADD(CLASS_SHAMAN, 1, 90);
  SKILL_ADD(CLASS_ETHERMANCER, 1, 90);
  SKILL_ADD(CLASS_ASSASSIN, 1, 90);
  SKILL_ADD(CLASS_BARD, 1, 90);
  SKILL_ADD(CLASS_PSIONICIST, 1, 90);
  SKILL_ADD(CLASS_MINDFLAYER, 1, 90);
  SKILL_ADD(CLASS_MONK, 1, 90);
  SKILL_ADD(CLASS_ILLUSIONIST, 1, 90);
  SKILL_ADD(CLASS_SPIPER, 1, 90);
  SKILL_ADD(CLASS_DREADLORD, 1, 90);
  SKILL_ADD(CLASS_AVENGER, 1, 90);

  SKILL_CREATE_WITH_CAT("awareness", SKILL_AWARENESS, SKILL_TYPE_MENTAL_INT, SKILL_CATEGORY_DEFENSIVE);
  SKILL_ADD(CLASS_RANGER, 1, 80);
  SKILL_ADD(CLASS_ASSASSIN, 1, 95);
  SKILL_ADD(CLASS_MONK, 1, 75);
  SKILL_ADD(CLASS_ROGUE, 1, 90);
  SKILL_ADD(CLASS_THIEF, 1, 90);
  SKILL_ADD(CLASS_BARD, 1, 90);
  SKILL_ADD(CLASS_SPIPER, 1, 90);


  SKILL_CREATE_WITH_CAT("capture", SKILL_CAPTURE, SKILL_TYPE_PHYS_STR, SKILL_CATEGORY_OFFENSIVE);  /* TASFALEN */
  SKILL_ADD(CLASS_MERCENARY, 1, 95);    /* TASFALEN */

  SKILL_CREATE("appraise", SKILL_APPRAISE, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_ROGUE, 10, 75);
  SKILL_ADD(CLASS_THIEF, 10, 75);
  SKILL_ADD(CLASS_MERCENARY, 20, 40);
  SKILL_ADD(CLASS_ASSASSIN, 20, 60);
  SKILL_ADD(CLASS_BARD, 10, 70);

  SKILL_CREATE("lore", SKILL_LORE, SKILL_TYPE_MENTAL_INT);
  SKILL_ADD(CLASS_BARD, 10, 75);
  SKILL_ADD(CLASS_ALCHEMIST, 1, 100);
  SKILL_ADD(CLASS_SPIPER, 10, 75);


  SKILL_CREATE("age corpse", SKILL_AGE_CORPSE, SKILL_TYPE_MENTAL_WIS);
  SKILL_ADD(CLASS_NECROMANCER, 20, 100);
  SKILL_ADD(CLASS_CLERIC, 40, 75);

  SKILL_CREATE("spell mastery", SKILL_SPELL_MASTERY, SKILL_TYPE_MENTAL_WIS);
  SKILL_CREATE("chant mastery", SKILL_CHANT_MASTERY, SKILL_TYPE_MENTAL_WIS);
  SKILL_CREATE("anatomy", SKILL_ANATOMY, SKILL_TYPE_MENTAL_WIS);
  SKILL_CREATE("summon blizzard", SKILL_SUMMON_BLIZZARD, SKILL_TYPE_MENTAL_WIS);
  SKILL_CREATE("summon familiar", SKILL_SUMMON_FAMILIAR, SKILL_TYPE_MENTAL_WIS);
  SKILL_CREATE("advanced meditation", SKILL_ADVANCED_MEDITATION, SKILL_TYPE_MENTAL_WIS);

  /*
     special, was easier to make them skills, but not assigned to any class,
     nor is it checked for, but this keeps the various arrays, indices, etc,
     from barfing
   */
  SKILL_CREATE("establish camp", SKILL_CAMP, SKILL_TYPE_NONE);

  POISON_CREATE("lifeleak", POISON_LIFELEAK, poison_lifeleak);
  POISON_CREATE("weakness", POISON_WEAKNESS, poison_weakness);
  POISON_CREATE("neurotoxin", POISON_NEUROTOXIN, poison_neurotoxin);
  POISON_CREATE("heart toxin", POISON_HEART_TOXIN, poison_heart_toxin);

  TAG_CREATE("decay", TAG_OBJ_DECAY);
  TAG_CREATE("orig", TAG_ALTERED_EXTRA2);
  TAG_CREATE("no misfire", TAG_NOMISFIRE);
  TAG_CREATE("witch spell", TAG_WITCHSPELL);
  TAG_CREATE("recent frag", TAG_RECENT_FRAG);
  TAG_CREATE_WITH_MESSAGES("phantasmal form", TAG_PHANTASMAL_FORM,
                           "&+WYou feel yourself return to normal as you leave your phantasmal form.&n",
                           "&+W$n looks more solid as $s phantasmal form abates.&n");

  TAG_CREATE_WITH_MESSAGES("ethereal form", TAG_ETHEREAL_FORM,
                           "&+LYou feel your body begin to regain its former substance...&n",
                           "&+L$n's body begins to regain its former substance&n");
  TAG_CREATE_WITH_MESSAGES("divine power", TAG_PALADIN_VIT,
                             "Your divine power fades.", "");

  TAG_CREATE_WITH_MESSAGES("vampire bite", TAG_VAMPIRE_BITE,
                             "Nauseous feeling from the bitten neck fades.", "");

  TAG_CREATE("embrace death", TAG_EMBRACE_DEATH);
  TAG_CREATE("innate timer", TAG_INNATE_TIMER);
  TAG_CREATE("master set", TAG_SET_MASTER);
  TAG_CREATE("daragor sword charge", TAG_IMMOLATE);
  TAG_CREATE("monk critical hits", TAG_PRESSURE_POINTS);

  TAG_CREATE_WITH_MESSAGES("decrepify", TAG_DECREPIFY,
  			   "The red aura around you flashes blue and is gone.","");
  TAG_CREATE_WITH_MESSAGES("hatred", TAG_HATRED,
  			   "You feel the hatred in your heart subside.","");
  TAG_CREATE("shapechange creature", TAG_KNOWN_SHAPE);
  TAG_CREATE("used monolith", TAG_EPIC_MONOLITH);
  TAG_CREATE("epic errand", TAG_EPIC_ERRAND);
  TAG_CREATE("epic completed", TAG_EPIC_COMPLETED);
  TAG_CREATE("epic skillpoints", TAG_EPIC_SKILLPOINTS);
  TAG_CREATE("epic points", TAG_EPIC_POINTS);
  TAG_CREATE("skill timer", TAG_SKILL_TIMER);
  TAG_CREATE("wet", TAG_WET);
  TAG_CREATE("used pool", TAG_POOL);
  TAG_CREATE_WITH_MESSAGES("item set bonus", TAG_SETPROC, "You no longer feel any spirit's support.", "");
 
}
