#include <cstring>

#include "prototypes.h"
#include "structs.h"
#include "defines.h"
#include "specialization_data.h"

const char *specdata[][MAX_SPEC] = {
	{									   "",												"",												  "",								 ""}, // None
	{						 "&+BSwordsman&N",								   "&+yGuardian&N",                        "&+bSwa&+Bsh&+Wbuc&+Bkler&n",                                 ""}, // Warrior
	{					   "&+cBlademaster&N",								   "&+gHuntsman&N",                               "&+gMa&+yrsha&+gll&n",                                 ""}, // Ranger
	{				 "&+rPyr&+Rokine&+rtic&N",                             "&+MEn&+mslav&+Mer&N",                              "&+bPsyche&+Lporter&N",                                 ""}, // Psionicist
	{					   "&+wCrusa&+Wder&n",								   "&+WCavalier&N",												  "",                                 ""}, // Paladin
	{					   "&+LDark Knight&N",                        "&+LDem&+ronic Ri&+Lder&N",                            "&+LVi&+ro&+Llat&+ror&n",                                 ""}, // Anti-Paladin
	{							"&+YZealot&n",									 "&+WHealer&n",								   "&+cHoly&+Wman&n",                                 ""}, // Cleric
	{				  "&+rRe&+Rd Dra&+rgon&N",                           "&+gElap&+Ghi&+gdist&N",                                                  "",                                 ""}, // Monk
	{				"&+gFo&+Gre&+gst Druid&n",                             "&+cStorm &+CDruid&n",                                                  "",                                 ""}, // Druid
	{		"&+rEl&+Rem&+Lenta&+Rli&n&+rst&n",                         "&+WSpir&+Citua&+Wlist&n",                            "&+yAni&+Ymal&n&+yist&n",                                 ""}, // Shaman
	{					   "&+MWild&+mmage&n",									 "&+LWizard&n",                                "&+LShadow&+wmage&n",                                 ""}, // Sorcerer
	{					   "&+mDia&+rbolis&n",							"&+mNe&+Lcro&+mlyte&n",                                    "&+LReap&+wer&n",                                 ""}, // Necromancer
	{						 "&+CAir Magus&n",								"&+BWater Magus&n",                                   "&+rFire Magus&n",                 "&+yEarth Magus&n"}, // Conjurer
	{						  "&+rAssassin&n",									  "&+LThief&n",										  "Not Used",   "&+LSh&+wa&+Ldow &+BArc&+bher&n"}, // Rogue
	{									   "",												"",												  "",								 ""}, // Assassin was replaced by CLASS_ROGUE, SPEC_ASSASSIN
	{					 "&+yBr&+Lig&+yand&n",						   "&+yBounty &+LHunter&n",                                                  "",                                 ""}, // Mercenary
	{	   "&+rD&+mis&+gha&+crm&+yon&+bist&n",                                  "&+RScoundrel&n",                             "&+YMin&n&+ystr&+Yel&n",                                 ""}, // Bard
	{									   "",												"",												  "",								 ""}, // Thief was replaced by CLASS_ROGUE SPEC_THIEF
	{									   "",												"",												  "",								 ""}, // Warlock
	{									   "",												"",												  "",								 ""}, // MindFlayer
	{			 "&+CBat&n&+ctle-For&+Cger&n",                           "&+LBla&+ccksm&+Lith&n",                                                  "",                                 ""}, // Alchemist
	{					  "&+rMa&+RUle&+rR&n",						  "&+RRa&+rGe&+Rlo&+rRd&n",                                                  "",                                 ""}, // Berserker
	{"&+CI&+Wc&+Ce &+LR&+Le&+wa&+wv&+Le&+Lr&n", "&+rF&+Rl&+Ya&+Rm&+re &+LR&+Le&+wa&+wv&+Le&+Lr&n",         "&+bSh&+Bo&+Wck &+LR&+Le&+wa&+wv&+Le&+Lr&n", "&+LEa&+yrt&+Lh R&+yea&+Lve&+yr&n"}, // Reaver
	{			  "&+BM&+Yag&+Bic&+Yia&+Bn&n",                            "&+LDark &+mDreamer&n",                                                  "",                                 ""}, // Illusionist
	{			"&+YSt&+yor&+Ym &+LBringer&n",                           "&+GSc&+gou&+Yrg&+Ge&n",                                 "&+LRu&+win&+Ler&n",                                 ""}, // Blighter
	{					  "&+LDeath&+rlord&n",							  "&+LShadow&+rlord&n",												  "",                                 ""}, // Dreadlord
	{		 "&+CTem&+cpe&+Cst Ma&+cgu&+Cs&n",                          "&+WFro&+cst &+CMagus&n",                    "&+YSt&+ya&+Yr &+LMa&+wgu&+Ls&n",                                 ""}, // Ethermancer
	{				   "&+YLight&+Wbringer&n",                           "&+WInq&+wuisi&+Wtor&n",                                                  "",                                 ""}, // Avenger
	{						  "&+wMedium&n&n",						"&+YT&+Re&+rmpl&+Ra&+Yr&n", "&+C&+WT&+ch&+Ca&+Wu&+Cm&+ca&+Ct&+Wu&+Cr&+cg&+Ce&n",                                 ""}, // Theurgist
	{				  "&+cCon&+Ctrol&+Wler&n",                "&+rM&+Re&+Yn&+Wtal&+Yi&+Rs&+rt&n",                             "&+gNat&+Gural&+yist&n",                                 ""}, // Summoner
	{  "&+GDr&+Lag&+Gon&n &+gHu&+Lnt&n&+ger&n",           "&+GDr&+Lag&+Gon&n &+RPr&n&+Lie&+Rst&n",             "&+GDr&+Lag&+Gon&n &+MLa&n&+Lnc&+Mer&n",                                 ""}  // Dragoon
};

static_assert(sizeof(specdata) / sizeof(specdata[0]) == CLASS_COUNT + 1,
              "specdata must contain one row for every class index");

struct allowed_race_spec_struct
{
	int  race;
	uint m_class;
	int  spec;
} allowed_race_specs[] =
{
    /* War Ran Psi Pal APa Cle Mon Dru Sha Sor Nec Con Rog Mer Bar Ber Rea Illu Dre Eth Ave The */
	/* RACE_HUMAN,     RACE_BARBARIAN, RACE_DROW,      RACE_GREY,      RACE_MOUNTAIN, RACE_DUERGAR,
	 * RACE_HALFLING,  RACE_GNOME,     RACE_OGRE,      RACE_TROLL,     RACE_HALFELF,  RACE_ORC,
	 * RACE_THRIKREEN, RACE_CENTAUR,   RACE_GITHYANKI, RACE_MINOTAUR,  RACE_GOBLIN,   RACE_LICH,
	 * RACE_PVAMPIRE,  RACE_GITHZERAI, RACE_DRIDER,    RACE_AGATHINON, RACE_ELADRIN,  RACE_KOBOLD,
	 * RACE_PILLITHID, RACE_KUOTOA,    RACE_WOODELF,   RACE_FIRBOLG */
	/* Start Human Options */
	{RACE_HUMAN, CLASS_WARRIOR,     SPEC_GUARDIAN},
	{RACE_HUMAN, CLASS_WARRIOR,     SPEC_SWORDSMAN},
	{RACE_HUMAN, CLASS_WARRIOR,     SPEC_SWASHBUCKLER},
	{RACE_HUMAN, CLASS_MERCENARY,   SPEC_ALL},
	{RACE_HUMAN, CLASS_PALADIN,     SPEC_ALL},
	{RACE_HUMAN, CLASS_RANGER,      SPEC_ALL},
	{RACE_HUMAN, CLASS_DRUID,       SPEC_ALL},
	{RACE_HUMAN, CLASS_ROGUE,       SPEC_THIEF},
	{RACE_HUMAN, CLASS_ROGUE,       SPEC_ASSASSIN},
	{RACE_HUMAN, CLASS_CLERIC,      SPEC_ALL},
	{RACE_HUMAN, CLASS_SORCERER,    SPEC_ALL},
	{RACE_HUMAN, CLASS_CONJURER,    SPEC_ALL},
	{RACE_HUMAN, CLASS_SUMMONER,    SPEC_ALL},
	{RACE_HUMAN, CLASS_ETHERMANCER, SPEC_ALL},
	{RACE_HUMAN, CLASS_MONK,        SPEC_ALL},
	{RACE_HUMAN, CLASS_NECROMANCER, SPEC_ALL},
	{RACE_HUMAN, CLASS_ILLUSIONIST, SPEC_ALL},
	{RACE_HUMAN, CLASS_BERSERKER,   SPEC_ALL},
	{RACE_HUMAN, CLASS_BARD,        SPEC_ALL},
	{RACE_HUMAN, CLASS_SHAMAN,      SPEC_ALL},
	{RACE_HUMAN, CLASS_SHAMAN,      SPEC_ALL},
	{RACE_HUMAN, CLASS_PSIONICIST,  SPEC_ALL},
	{RACE_HUMAN, CLASS_DRAGOON,  	SPEC_DRAGON_HUNTER},
    {RACE_HUMAN, CLASS_DRAGOON,  	SPEC_DRAGON_LANCER},
	/* End Human Options */

	/* Start Orc Options */
	{RACE_ORC, CLASS_WARRIOR,       SPEC_GUARDIAN},
	{RACE_ORC, CLASS_WARRIOR,       SPEC_SWORDSMAN},
	{RACE_ORC, CLASS_WARRIOR,       SPEC_SWASHBUCKLER},
	{RACE_ORC, CLASS_PSIONICIST,    SPEC_ALL},
	{RACE_ORC, CLASS_MERCENARY,     SPEC_ALL},
	{RACE_ORC, CLASS_ANTIPALADIN,   SPEC_ALL},
	{RACE_ORC, CLASS_REAVER,        SPEC_ALL},
	{RACE_ORC, CLASS_ROGUE,         SPEC_THIEF},
	{RACE_ORC, CLASS_ROGUE,         SPEC_ASSASSIN},
	{RACE_ORC, CLASS_CLERIC,        SPEC_ALL},
	{RACE_ORC, CLASS_SORCERER,      SPEC_ALL},
	{RACE_ORC, CLASS_CONJURER,      SPEC_ALL},
	{RACE_ORC, CLASS_SUMMONER,      SPEC_ALL},
	{RACE_ORC, CLASS_ETHERMANCER,   SPEC_ALL},
	{RACE_ORC, CLASS_MONK, 	        SPEC_ALL},
	{RACE_ORC, CLASS_NECROMANCER,   SPEC_ALL},
	{RACE_ORC, CLASS_ILLUSIONIST,   SPEC_ALL},
	{RACE_ORC, CLASS_BERSERKER,     SPEC_ALL},
	{RACE_ORC, CLASS_BARD,          SPEC_ALL},
	{RACE_ORC, CLASS_SHAMAN,        SPEC_ALL},
	{RACE_ORC, CLASS_BLIGHTER,      SPEC_ALL},
	{RACE_ORC, CLASS_DRAGOON,  		SPEC_DRAGON_HUNTER},
	{RACE_ORC, CLASS_DRAGOON,  		SPEC_DRAGON_LANCER},
	/* End Orc Options */

	/* Start Mountain Dwarf Options */
	{RACE_MOUNTAIN, CLASS_WARRIOR,      SPEC_GUARDIAN},
	{RACE_MOUNTAIN, CLASS_WARRIOR,      SPEC_SWORDSMAN},
	{RACE_MOUNTAIN, CLASS_MERCENARY,    SPEC_ALL},
	{RACE_MOUNTAIN, CLASS_PALADIN,      SPEC_ALL},
	{RACE_MOUNTAIN, CLASS_DRUID,        SPEC_ALL},
	{RACE_MOUNTAIN, CLASS_ROGUE,        SPEC_THIEF},
	{RACE_MOUNTAIN, CLASS_ROGUE,        SPEC_ASSASSIN},
	{RACE_MOUNTAIN, CLASS_CLERIC,       SPEC_ALL},
	{RACE_MOUNTAIN, CLASS_BERSERKER,    SPEC_ALL},
	/* End Mountain Dwarf Options */

	/* Start Duergar Dwarf Options */
	{RACE_DUERGAR, CLASS_WARRIOR,       SPEC_GUARDIAN},
	{RACE_DUERGAR, CLASS_WARRIOR,       SPEC_SWORDSMAN},
	{RACE_DUERGAR, CLASS_MERCENARY,     SPEC_ALL},
	{RACE_DUERGAR, CLASS_ANTIPALADIN,   SPEC_ALL},
	{RACE_DUERGAR, CLASS_REAVER,        SPEC_EARTH_REAVER},
	{RACE_DUERGAR, CLASS_ROGUE,         SPEC_THIEF},
	{RACE_DUERGAR, CLASS_ROGUE,         SPEC_ASSASSIN},
	{RACE_DUERGAR, CLASS_CLERIC,        SPEC_ALL},
	{RACE_DUERGAR, CLASS_BERSERKER,     SPEC_ALL},
	{RACE_DUERGAR, CLASS_BLIGHTER,      SPEC_ALL},
  /* End Duergar Dwarf Options */

	/* Start Centaur Options */
	{RACE_CENTAUR, CLASS_WARRIOR,     SPEC_GUARDIAN},
	{RACE_CENTAUR, CLASS_WARRIOR,     SPEC_SWORDSMAN},
	{RACE_CENTAUR, CLASS_PALADIN,     SPEC_CRUSADER},
	{RACE_CENTAUR, CLASS_RANGER,      SPEC_ALL},
	{RACE_CENTAUR, CLASS_DRUID,       SPEC_ALL},
	{RACE_CENTAUR, CLASS_SHAMAN,      SPEC_ALL},
	/* End Centaur Options */

	/* Start Drider Options */
	{RACE_DRIDER, CLASS_WARRIOR,        SPEC_SWORDSMAN},
	{RACE_DRIDER, CLASS_WARRIOR,        SPEC_GUARDIAN},
	{RACE_DRIDER, CLASS_ANTIPALADIN,    SPEC_DARKKNIGHT},
	{RACE_DRIDER, CLASS_SORCERER,       SPEC_ALL},
	{RACE_DRIDER, CLASS_REAVER,         SPEC_ALL},
	{RACE_DRIDER, CLASS_NECROMANCER,    SPEC_ALL},
	{RACE_DRIDER, CLASS_BLIGHTER,       SPEC_ALL},
	/* End Drider Options */

	/* Start Barbarian Options */
	{RACE_BARBARIAN, CLASS_WARRIOR,     SPEC_GUARDIAN},
	{RACE_BARBARIAN, CLASS_WARRIOR,     SPEC_SWORDSMAN},
	{RACE_BARBARIAN, CLASS_MERCENARY,   SPEC_ALL},
	{RACE_BARBARIAN, CLASS_ROGUE,       SPEC_THIEF},
	{RACE_BARBARIAN, CLASS_ROGUE,       SPEC_ASSASSIN},
	{RACE_BARBARIAN, CLASS_BERSERKER,   SPEC_ALL},
	{RACE_BARBARIAN, CLASS_SHAMAN,      SPEC_ALL},
	/* End Barbarian Options */

	/* Start Troll Options */
	{RACE_TROLL, CLASS_WARRIOR,       SPEC_GUARDIAN},
	{RACE_TROLL, CLASS_WARRIOR,       SPEC_SWORDSMAN},
	{RACE_TROLL, CLASS_MERCENARY,     SPEC_ALL},
	{RACE_TROLL, CLASS_ROGUE,         SPEC_THIEF},
	{RACE_TROLL, CLASS_ROGUE,         SPEC_ASSASSIN},
	{RACE_TROLL, CLASS_BERSERKER,     SPEC_ALL},
	{RACE_TROLL, CLASS_SHAMAN,        SPEC_ALL},
	/* End Troll Options */

	/* Start Grey Elf Options */
	{RACE_GREY, CLASS_WARRIOR,      SPEC_GUARDIAN},
	{RACE_GREY, CLASS_WARRIOR,      SPEC_SWORDSMAN},
	{RACE_GREY, CLASS_WARRIOR,      SPEC_SWASHBUCKLER},
	{RACE_GREY, CLASS_MERCENARY,    SPEC_ALL},
	{RACE_GREY, CLASS_PALADIN,      SPEC_ALL},
	{RACE_GREY, CLASS_ROGUE,        SPEC_THIEF},
	{RACE_GREY, CLASS_ROGUE,        SPEC_ASSASSIN},
	{RACE_GREY, CLASS_DRUID,        SPEC_ALL},
	{RACE_GREY, CLASS_RANGER,       SPEC_ALL},
	{RACE_GREY, CLASS_CLERIC,       SPEC_ALL},
	{RACE_GREY, CLASS_SORCERER,     SPEC_ALL},
	{RACE_GREY, CLASS_CONJURER,     SPEC_ALL},
	{RACE_GREY, CLASS_SUMMONER,     SPEC_ALL},
	{RACE_GREY, CLASS_ETHERMANCER,  SPEC_ALL},
	{RACE_GREY, CLASS_ILLUSIONIST,  SPEC_ALL},
	{RACE_GREY, CLASS_BARD,         SPEC_ALL},
	{RACE_GREY, CLASS_SHAMAN,       SPEC_ALL},
	{RACE_GREY, CLASS_PSIONICIST,   SPEC_ALL},
	{RACE_GREY, CLASS_MONK,         SPEC_ALL},
	{RACE_GREY, CLASS_DRAGOON,  	SPEC_DRAGON_PRIEST},
	{RACE_GREY, CLASS_DRAGOON,  	SPEC_DRAGON_HUNTER},
	/* End Grey Elf Options */

	/* Start Tiefling Options */
	{RACE_TIEFLING, CLASS_WARRIOR,      SPEC_GUARDIAN},
	{RACE_TIEFLING, CLASS_WARRIOR,      SPEC_SWORDSMAN},
	{RACE_TIEFLING, CLASS_WARRIOR,      SPEC_SWASHBUCKLER},
	{RACE_TIEFLING, CLASS_MERCENARY,    SPEC_ALL},
	{RACE_TIEFLING, CLASS_PALADIN,      SPEC_ALL},
	{RACE_TIEFLING, CLASS_ANTIPALADIN,  SPEC_ALL},
	{RACE_TIEFLING, CLASS_REAVER,       SPEC_ALL},
	{RACE_TIEFLING, CLASS_ROGUE,        SPEC_THIEF},
	{RACE_TIEFLING, CLASS_ROGUE,        SPEC_ASSASSIN},
	{RACE_TIEFLING, CLASS_DRUID,        SPEC_ALL},
	{RACE_TIEFLING, CLASS_RANGER,       SPEC_ALL},
	{RACE_TIEFLING, CLASS_CLERIC,       SPEC_ALL},
	{RACE_TIEFLING, CLASS_SORCERER,     SPEC_ALL},
	{RACE_TIEFLING, CLASS_CONJURER,     SPEC_ALL},
	{RACE_TIEFLING, CLASS_SUMMONER,     SPEC_ALL},
	{RACE_TIEFLING, CLASS_ETHERMANCER,  SPEC_ALL},
	{RACE_TIEFLING, CLASS_ILLUSIONIST,  SPEC_ALL},
	{RACE_TIEFLING, CLASS_BARD,         SPEC_ALL},
	{RACE_TIEFLING, CLASS_SHAMAN,       SPEC_ALL},
	{RACE_TIEFLING, CLASS_PSIONICIST,   SPEC_ALL},
	{RACE_TIEFLING, CLASS_MONK,         SPEC_ALL},
	{RACE_TIEFLING, CLASS_DRAGOON,  	SPEC_DRAGON_PRIEST},
	{RACE_TIEFLING, CLASS_DRAGOON,  	SPEC_DRAGON_HUNTER},
	{RACE_TIEFLING, CLASS_DRAGOON,  	SPEC_DRAGON_LANCER},
	/* End Tiefling Options */

	/* Start Drow Elf Options */
	{RACE_DROW, CLASS_WARRIOR,      SPEC_GUARDIAN},
	{RACE_DROW, CLASS_WARRIOR,      SPEC_SWORDSMAN},
	{RACE_DROW, CLASS_WARRIOR,      SPEC_SWASHBUCKLER},
	{RACE_DROW, CLASS_MERCENARY,    SPEC_ALL},
	{RACE_DROW, CLASS_ANTIPALADIN,  SPEC_ALL},
	{RACE_DROW, CLASS_REAVER,       SPEC_ALL},
	{RACE_DROW, CLASS_ROGUE,        SPEC_THIEF},
	{RACE_DROW, CLASS_ROGUE,        SPEC_ASSASSIN},
	{RACE_DROW, CLASS_CLERIC,       SPEC_ALL},
	{RACE_DROW, CLASS_SORCERER,     SPEC_ALL},
	{RACE_DROW, CLASS_CONJURER,     SPEC_ALL},
	{RACE_DROW, CLASS_SUMMONER,     SPEC_ALL},
	{RACE_DROW, CLASS_ETHERMANCER,  SPEC_ALL},
	{RACE_DROW, CLASS_NECROMANCER,  SPEC_ALL},
	{RACE_DROW, CLASS_ILLUSIONIST,  SPEC_ALL},
	{RACE_DROW, CLASS_BARD,         SPEC_ALL},
	{RACE_DROW, CLASS_SHAMAN,       SPEC_ALL},
	{RACE_DROW, CLASS_PSIONICIST,   SPEC_ALL},
	{RACE_DROW, CLASS_MONK,         SPEC_ALL},
	{RACE_DROW, CLASS_BLIGHTER,     SPEC_ALL},
	{RACE_DROW, CLASS_DRAGOON,  	SPEC_DRAGON_PRIEST},
	{RACE_DROW, CLASS_DRAGOON,  	SPEC_DRAGON_HUNTER},
	/* End Drow Elf Options */

	/* Start Gnome Options */
	{RACE_GNOME, CLASS_WARRIOR,     SPEC_GUARDIAN},
	{RACE_GNOME, CLASS_WARRIOR,     SPEC_SWORDSMAN},
	{RACE_GNOME, CLASS_MERCENARY,   SPEC_ALL},
	{RACE_GNOME, CLASS_CLERIC,      SPEC_ALL},
	{RACE_GNOME, CLASS_SORCERER,    SPEC_ALL},
	{RACE_GNOME, CLASS_CONJURER,    SPEC_ALL},
	{RACE_GNOME, CLASS_SUMMONER,    SPEC_ALL},
	{RACE_GNOME, CLASS_ILLUSIONIST, SPEC_ALL},
	{RACE_GNOME, CLASS_BARD,        SPEC_ALL},
	{RACE_GNOME, CLASS_SHAMAN,      SPEC_ALL},
	{RACE_GNOME, CLASS_NECROMANCER, SPEC_ALL},
	/* End Gnome Options */

	/* Start Kobold Options */
	{RACE_KOBOLD, CLASS_WARRIOR,      SPEC_GUARDIAN},
	{RACE_KOBOLD, CLASS_WARRIOR,      SPEC_SWORDSMAN},
	{RACE_KOBOLD, CLASS_MERCENARY,    SPEC_ALL},
	{RACE_KOBOLD, CLASS_CLERIC,       SPEC_ALL},
	{RACE_KOBOLD, CLASS_SORCERER,     SPEC_ALL},
	{RACE_KOBOLD, CLASS_CONJURER,     SPEC_ALL},
	{RACE_KOBOLD, CLASS_SUMMONER,     SPEC_ALL},
	{RACE_KOBOLD, CLASS_ETHERMANCER,  SPEC_ALL},
	{RACE_KOBOLD, CLASS_NECROMANCER,  SPEC_ALL},
	{RACE_KOBOLD, CLASS_ILLUSIONIST,  SPEC_ALL},
	{RACE_KOBOLD, CLASS_BARD,         SPEC_ALL},
	{RACE_KOBOLD, CLASS_SHAMAN,       SPEC_ALL},
	/* End Kobold Options */

	/* Start Halfling Options */
	{RACE_HALFLING, CLASS_WARRIOR,      SPEC_GUARDIAN},
	{RACE_HALFLING, CLASS_WARRIOR,      SPEC_SWORDSMAN},
	{RACE_HALFLING, CLASS_MERCENARY,    SPEC_ALL},
	{RACE_HALFLING, CLASS_RANGER,       SPEC_ALL},
	{RACE_HALFLING, CLASS_DRUID,        SPEC_ALL},
	{RACE_HALFLING, CLASS_ROGUE,        SPEC_THIEF},
	{RACE_HALFLING, CLASS_ROGUE,        SPEC_ASSASSIN},
	{RACE_HALFLING, CLASS_CLERIC,       SPEC_ALL},
	{RACE_HALFLING, CLASS_SORCERER,     SPEC_ALL},
	{RACE_HALFLING, CLASS_CONJURER,     SPEC_ALL},
	{RACE_HALFLING, CLASS_SUMMONER,     SPEC_ALL},
	{RACE_HALFLING, CLASS_ETHERMANCER,  SPEC_ALL},
	{RACE_HALFLING, CLASS_ILLUSIONIST,  SPEC_ALL},
	{RACE_HALFLING, CLASS_BARD,         SPEC_ALL},
	{RACE_HALFLING, CLASS_SHAMAN,       SPEC_ALL},
	/* End Halfling Options */

	/* Start Goblin Options */
	{RACE_GOBLIN, CLASS_WARRIOR,       SPEC_GUARDIAN},
	{RACE_GOBLIN, CLASS_WARRIOR,       SPEC_SWORDSMAN},
	{RACE_GOBLIN, CLASS_MERCENARY,     SPEC_ALL},
	{RACE_GOBLIN, CLASS_ANTIPALADIN,   SPEC_ALL},
	{RACE_GOBLIN, CLASS_REAVER,        SPEC_ALL},
	{RACE_GOBLIN, CLASS_ROGUE,         SPEC_THIEF},
	{RACE_GOBLIN, CLASS_ROGUE,         SPEC_ASSASSIN},
	{RACE_GOBLIN, CLASS_CLERIC,        SPEC_ALL},
	{RACE_GOBLIN, CLASS_SORCERER,      SPEC_ALL},
	{RACE_GOBLIN, CLASS_CONJURER,      SPEC_ALL},
	{RACE_GOBLIN, CLASS_SUMMONER,      SPEC_ALL},
	{RACE_GOBLIN, CLASS_ETHERMANCER,   SPEC_ALL},
	{RACE_GOBLIN, CLASS_NECROMANCER,   SPEC_ALL},
	{RACE_GOBLIN, CLASS_ILLUSIONIST,   SPEC_ALL},
	{RACE_GOBLIN, CLASS_BARD,          SPEC_ALL},
	{RACE_GOBLIN, CLASS_SHAMAN,        SPEC_ALL},
	{RACE_GOBLIN, CLASS_BLIGHTER,      SPEC_ALL},
	/* End Goblin Options */

	/* Start Githzerai Options */
	{RACE_GITHZERAI, CLASS_WARRIOR,     SPEC_GUARDIAN},
	{RACE_GITHZERAI, CLASS_WARRIOR,     SPEC_SWORDSMAN},
	{RACE_GITHZERAI, CLASS_PALADIN,     SPEC_ALL},
	{RACE_GITHZERAI, CLASS_RANGER,      SPEC_ALL},
	{RACE_GITHZERAI, CLASS_DRUID,       SPEC_ALL},
	{RACE_GITHZERAI, CLASS_CLERIC,      SPEC_ALL},
	{RACE_GITHZERAI, CLASS_SORCERER,    SPEC_ALL},
	{RACE_GITHZERAI, CLASS_CONJURER,    SPEC_ALL},
	{RACE_GITHZERAI, CLASS_SUMMONER,    SPEC_ALL},
	{RACE_GITHZERAI, CLASS_ETHERMANCER, SPEC_ALL},
	{RACE_GITHZERAI, CLASS_MONK,        SPEC_ALL},
	{RACE_GITHZERAI, CLASS_NECROMANCER, SPEC_ALL},
	{RACE_GITHZERAI, CLASS_ILLUSIONIST, SPEC_ALL},
	{RACE_GITHZERAI, CLASS_BARD,        SPEC_ALL},
	{RACE_GITHZERAI, CLASS_PSIONICIST,  SPEC_ALL},
	{RACE_GITHZERAI, CLASS_DRAGOON,     SPEC_DRAGON_LANCER},
	{RACE_GITHZERAI, CLASS_DRAGOON,     SPEC_DRAGON_PRIEST},
	/* End Githzerai Options */

	/* Start Githyanki Options */
	{RACE_GITHYANKI, CLASS_WARRIOR,       SPEC_GUARDIAN},
	{RACE_GITHYANKI, CLASS_WARRIOR,       SPEC_SWORDSMAN},
	{RACE_GITHYANKI, CLASS_ANTIPALADIN,   SPEC_ALL},
	{RACE_GITHYANKI, CLASS_REAVER,        SPEC_ALL},
	{RACE_GITHYANKI, CLASS_ROGUE,         SPEC_THIEF},
	{RACE_GITHYANKI, CLASS_ROGUE,         SPEC_ASSASSIN},
	{RACE_GITHYANKI, CLASS_CLERIC,        SPEC_ALL},
	{RACE_GITHYANKI, CLASS_SORCERER,      SPEC_ALL},
	{RACE_GITHYANKI, CLASS_CONJURER,      SPEC_ALL},
	{RACE_GITHYANKI, CLASS_SUMMONER,      SPEC_ALL},
	{RACE_GITHYANKI, CLASS_ETHERMANCER,   SPEC_ALL},
	{RACE_GITHYANKI, CLASS_MONK, 	      SPEC_ALL},
	{RACE_GITHYANKI, CLASS_NECROMANCER,   SPEC_ALL},
	{RACE_GITHYANKI, CLASS_ILLUSIONIST,   SPEC_ALL},
	{RACE_GITHYANKI, CLASS_BARD,          SPEC_ALL},
	{RACE_GITHYANKI, CLASS_PSIONICIST,    SPEC_ALL},
	{RACE_GITHYANKI, CLASS_BLIGHTER,      SPEC_ALL},
	{RACE_GITHYANKI, CLASS_DRAGOON,       SPEC_DRAGON_LANCER},
	{RACE_GITHYANKI, CLASS_DRAGOON,       SPEC_DRAGON_PRIEST},
	/* End Githyanki Options */

	/* Start Firbolg Options */
	{RACE_FIRBOLG, CLASS_WARRIOR,     	SPEC_GUARDIAN},
	{RACE_FIRBOLG, CLASS_WARRIOR,     	SPEC_SWORDSMAN},
	{RACE_FIRBOLG, CLASS_SHAMAN,      	SPEC_ALL},
	/* End Firbolg Options */

	/* Start Ogre Options */
	{RACE_OGRE, CLASS_WARRIOR,   		SPEC_GUARDIAN},
	{RACE_OGRE, CLASS_WARRIOR,   		SPEC_SWORDSMAN},
	{RACE_OGRE, CLASS_SHAMAN,        	SPEC_ALL},
	/* End Ogre Options */

	/* Start Half-Elf Options */
/*	{RACE_HALFELF, CLASS_WARRIOR,     SPEC_GUARDIAN},
	{RACE_HALFELF, CLASS_WARRIOR,     SPEC_SWASHBUCKLER},
	{RACE_HALFELF, CLASS_RANGER,      SPEC_BLADEMASTER},
	{RACE_HALFELF, CLASS_PALADIN,     SPEC_ALL},
	{RACE_HALFELF, CLASS_CLERIC,      SPEC_ZEALOT},
	{RACE_HALFELF, CLASS_DRUID,       SPEC_STORM},
	{RACE_HALFELF, CLASS_SORCERER,    SPEC_ALL},
	{RACE_HALFELF, CLASS_CONJURER,    SPEC_ALL},
	{RACE_HALFELF, CLASS_SUMMONER,    SPEC_ALL},
	{RACE_HALFELF, CLASS_ROGUE,       SPEC_ALL},
	{RACE_HALFELF, CLASS_MERCENARY,   SPEC_OPPORTUNIST},
	{RACE_HALFELF, CLASS_BARD,        SPEC_ALL},
	{RACE_HALFELF, CLASS_ILLUSIONIST, SPEC_ALL},
	{RACE_HALFELF, CLASS_THEURGIST,   SPEC_TEMPLAR},
	{RACE_HALFELF, CLASS_THEURGIST,   SPEC_THAUMATURGE},
*/	/* End Half-Elf Options */

	/* Start Thri-Kreen Options */
	{RACE_THRIKREEN, CLASS_WARRIOR, 		SPEC_SWORDSMAN},
	/* End Thri-Kreen Options */

	/* Start Minotaur Options */
	{RACE_MINOTAUR, CLASS_WARRIOR,   		SPEC_GUARDIAN},
	{RACE_MINOTAUR, CLASS_WARRIOR,   		SPEC_SWORDSMAN},
	{RACE_MINOTAUR, CLASS_MERCENARY,     	SPEC_ALL},
	{RACE_MINOTAUR, CLASS_SHAMAN,        	SPEC_ALL},
	{RACE_MINOTAUR, CLASS_SORCERER,      	SPEC_ALL},
	{RACE_MINOTAUR, CLASS_BERSERKER,     	SPEC_ALL},
	/* End Minotaur Options */

	/* Start Lich Options */
	{RACE_LICH, CLASS_NECROMANCER, SPEC_ALL},
	/* End Lich Options */

	/* Start Vampire Options */
	//{RACE_PVAMPIRE, CLASS_DREADLORD, SPEC_ALL},
	/* End Vampire Options */

	/* Start Agathinon Options */
	//{RACE_AGATHINON, CLASS_AVENGER, SPEC_ALL},
	/* End Agathinon Options */

	/* Start Eladrin Options */
	//{RACE_ELADRIN, CLASS_THEURGIST, SPEC_ALL},
	/* End Eladrin Options */

	/* Start Illithid Options */
	//{RACE_PILLITHID, CLASS_WARRIOR,     SPEC_SWORDSMAN},
/*	{RACE_PILLITHID, CLASS_PSIONICIST,  SPEC_ALL},
	{RACE_PILLITHID, CLASS_SORCERER,    SPEC_SHADOW},
	{RACE_PILLITHID, CLASS_SORCERER,    SPEC_WIZARD},
	{RACE_PILLITHID, CLASS_CONJURER,    SPEC_ALL},
	{RACE_PILLITHID, CLASS_SUMMONER,    SPEC_ALL},
	//{RACE_PILLITHID, CLASS_ILLUSIONIST, SPEC_DARK_DREAMER},
	//{RACE_PILLITHID, CLASS_ILLUSIONIST, SPEC_DECEIVER},
	//{RACE_PILLITHID, CLASS_ETHERMANCER, SPEC_STARMAGUS},
*/	/* End Illithid Options */

	/* Start Kuo Toa Options */
/*	{RACE_KUOTOA, CLASS_WARRIOR, SPEC_GUARDIAN},
	{RACE_KUOTOA, CLASS_WARRIOR, SPEC_SWORDSMAN},
	{RACE_KUOTOA, CLASS_CLERIC,  SPEC_ZEALOT},
	{RACE_KUOTOA, CLASS_SHAMAN,  SPEC_ELEMENTALIST},
	{RACE_KUOTOA, CLASS_ROGUE,   SPEC_ASSASSIN},
*/	/* End Kuo Toa Options */

	/* Start Wood Elf Options */
/*	{RACE_WOODELF, CLASS_WARRIOR, SPEC_GUARDIAN},
	{RACE_WOODELF, CLASS_WARRIOR, SPEC_SWORDSMAN},
	{RACE_WOODELF, CLASS_RANGER,  SPEC_ALL},
	{RACE_WOODELF, CLASS_DRUID,   SPEC_WOODLAND},
	{RACE_WOODELF, CLASS_ROGUE,   SPEC_ASSASSIN},
	{RACE_WOODELF, CLASS_ROGUE,   SPEC_THIEF},
*/	/* End Wood Elf Options */


	{0, 0, 0}
};

const char *specialization_name_by_index(int class_index, int spec_index)
{
    if (class_index < 0 || class_index >= CLASS_COUNT + 1 ||
        spec_index < 0 || spec_index >= MAX_SPEC)
        return "";
    return specdata[class_index][spec_index];
}

const char *specialization_name(uint m_class, int spec_index)
{
    return specialization_name_by_index(flag2idx(m_class), spec_index);
}

bool specialization_exists_by_index(int class_index, int spec_index)
{
    const char *name = specialization_name_by_index(class_index, spec_index);
    return *name && std::strcmp(name, "Not Used") != 0;
}

bool specialization_exists(uint m_class, int spec_index)
{
    return specialization_exists_by_index(flag2idx(m_class), spec_index);
}

bool specialization_is_active(P_char ch)
{
    return ch != nullptr && ch->player.spec != 0;
}

bool specialization_matches(P_char ch, uint m_class, int spec)
{
    return ch != nullptr && (ch->player.m_class & m_class) &&
           ch->player.spec == spec;
}

bool specialization_is_allowed_race_spec(int race, uint m_class, int spec)
{
    for (int i = 0; allowed_race_specs[i].race; ++i)
    {
        if (allowed_race_specs[i].race == race &&
            allowed_race_specs[i].m_class == m_class &&
            (allowed_race_specs[i].spec == SPEC_ALL ||
             allowed_race_specs[i].spec == spec))
            return TRUE;
    }
    return FALSE;
}
