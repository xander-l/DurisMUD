#include <cstring>
#include "specialization_data.h"
#include <stdio.h>
#include <vector>
using namespace std;

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "interp.h"
#include "utils.h"
#include "epic.h"
#include "specializations.h"
#include "spells.h"
#include "sql.h"

extern P_room                   world;
extern const struct class_names class_names_table[];
extern const struct race_names  race_names_table[];


bool append_valid_specs(char *buf, P_char ch)
{
	if (!ch)
		return false;

	bool found_one = false;
	for (int i = 0; i < MAX_SPEC; i++)
	{
		if (*specialization_name(ch->player.m_class, i) && specialization_is_allowed_race_spec(GET_RACE(ch), ch->player.m_class, i + 1))
		{
			strcat(buf, specialization_name(ch->player.m_class, i));
			strcat(buf, "\r\n");
			found_one = true;
		}
	}

	return found_one;
}

string single_spec_list(int race, int cls)
{
	int    spec, comma = 0;
	string return_str;

	for (spec = 0; spec < MAX_SPEC; spec++)
	{
		if (!strcmp(specialization_name_by_index((cls), (spec)), "") || !strcmp(specialization_name_by_index((cls), (spec)), "Not Used"))
		{
			continue;
		}
		if (specialization_is_allowed_race_spec(race, 1 << (cls - 1), spec + 1))
		{
			if (comma)
				return_str += "&n, ";
			else
				comma = 1;

			return_str += string(specialization_name_by_index((cls), (spec)));
		}
	}

	return return_str;
}

void do_spec_list(P_char ch)
{
	char Gbuf[MAX_STRING_LENGTH], list[MAX_STRING_LENGTH];
	int  race, cls, spec, show;
	bool comma;

	send_to_char("&+WCurrent list of specializations available:&n\n\n", ch);
	for (cls = 1; cls <= CLASS_COUNT; cls++)
	{
		// Look for valid spec for the class.
		for (spec = 0; spec < MAX_SPEC; spec++)
		{
			show = 0;
			if (strcmp(specialization_name_by_index((cls), (spec)), "") && strcmp(specialization_name_by_index((cls), (spec)), "Not Used"))
			{
				show = 1;
				break;
			}
		}
		if (show)
		{
			snprintf(Gbuf, MAX_STRING_LENGTH, "&+W*&n %s &+W*&n\n", class_names_table[cls].ansi);
			send_to_char(Gbuf, ch);
		}
		// Walk through each spec.
		for (spec = 0; spec < MAX_SPEC; spec++)
		{
			if (strcmp(specialization_name_by_index((cls), (spec)), "") && strcmp(specialization_name_by_index((cls), (spec)), "Not Used"))
			{
				snprintf(list, MAX_STRING_LENGTH, " %s:", specialization_name_by_index((cls), (spec)));
				comma = FALSE;
				for (race = 1; race <= RACE_PLAYER_MAX; race++)
				{
					// If race has the spec, add it to the list.
					if (specialization_is_allowed_race_spec(race, 1 << (cls - 1), spec + 1))
					{
						if (comma)
						{
							snprintf(list + strlen(list), MAX_STRING_LENGTH - strlen(list), ", %s&n", race_names_table[race].ansi);
						}
						else
						{
							comma = TRUE;
							snprintf(list + strlen(list), MAX_STRING_LENGTH - strlen(list), " %s&n", race_names_table[race].ansi);
						}
					}
				}
				send_to_char(list, ch);
				send_to_char("\n", ch);
			}
		}
		if (show)
		{
			send_to_char("\n", ch);
		}
	}
}

void do_specialize(P_char ch, char *argument, int cmd)
{
	P_char teacher;
	int    i;
	char   buf[MAX_STRING_LENGTH];
	bool   found_one;

	if (!strcmp(argument, "list"))
	{
		do_spec_list(ch);
		return;
	}

	if (IS_MULTICLASS_PC(ch))
	{
		send_to_char("You have already chosen another path!\n", ch);
		return;
	}

	if (!*argument)
	{
		snprintf(buf, MAX_STRING_LENGTH, "You can choose from the following specializations:\n\r");

		found_one = append_valid_specs(buf, ch);

		if (!found_one)
		{
			send_to_char("There are no specializations available to you.\r\n", ch);
			return;
		}

		send_to_char(buf, ch);
		return;
	}

	for (teacher = world[ch->in_room].people; teacher; teacher = teacher->next_in_room)
	{
		if (IS_DRAGOON(ch) && IS_NPC(teacher) && IS_SET(teacher->specials.act, ACT_SPEC_TEACHER))
		{
			if (GET_CLASS(teacher, CLASS_RANGER) || GET_CLASS(teacher, CLASS_MERCENARY) || GET_CLASS(teacher, CLASS_PALADIN) || GET_CLASS(teacher, CLASS_ANTIPALADIN) ||
			    GET_CLASS(teacher, CLASS_SHAMAN) || GET_CLASS(teacher, CLASS_DRUID))
			{
				break;
			}
		}

		if (IS_NPC(teacher) && GET_CLASS(teacher, ch->player.m_class) && IS_SET(teacher->specials.act, ACT_SPEC_TEACHER))
		{
			break;
		}
		// Allowing people to spec from a God if they have consent.
		if (IS_PC(teacher) && IS_TRUSTED(teacher) && is_linked_to(ch, teacher, LNK_CONSENT) && ch != teacher)
		{
			break;
		}
	}

	if (!teacher)
	{
		send_to_char("You need to find a teacher first.\n", ch);
		return;
	}

	if (IS_NPC(ch))
	{
		mobsay(teacher, "Please stop trying to crash the game by ordering pets to specialize.");
		return;
	}

	if (teacher->player.m_class != ch->player.m_class && !IS_DRAGOON(ch) && !(IS_PC(teacher) && IS_TRUSTED(teacher) && is_linked_to(ch, teacher, LNK_CONSENT)))
	{
		mobsay(teacher, "I know nothing of your kind. Be gone.");
		return;
	}

	if (IS_SPECIALIZED(ch))
	{
		mobsay(teacher, "You are already specialized.");
		return;
	}

	while (*argument == ' ')
		argument++;

	if (time(NULL) < ch->only.pc->time_unspecced)
	{
		snprintf(buf, MAX_STRING_LENGTH, "You cannot specialize until %s.\r\n", asctime(localtime(&ch->only.pc->time_unspecced)));
		send_to_char(buf, ch);
		return;
	}

	if (GET_LEVEL(ch) < 30)
	{
		mobsay(teacher, "You are not yet experienced enough to specialize.");
		return;
	}

	for (i = 0; i < MAX_SPEC; i++)
	{
		if (!*specialization_name(ch->player.m_class, i))
			continue;

		if (!specialization_is_allowed_race_spec(GET_RACE(ch), ch->player.m_class, i + 1))
			continue;

		if (is_abbrev(argument, strip_ansi(specialization_name(ch->player.m_class, i)).c_str()))
		{
			if (IS_DRAGOON(ch))
			{
				if (i + 1 == SPEC_DRAGON_HUNTER && !(GET_CLASS(teacher, CLASS_RANGER) || GET_CLASS(teacher, CLASS_MERCENARY)))
				{
					snprintf(buf, MAX_STRING_LENGTH, "I cannot teach you to follow the path of the %s&n!", specialization_name(ch->player.m_class, i));
					mobsay(teacher, buf);
					return;
				}

				if (i + 1 == SPEC_DRAGON_PRIEST && !(GET_CLASS(teacher, CLASS_SHAMAN) || GET_CLASS(teacher, CLASS_DRUID)))
				{
					snprintf(buf, MAX_STRING_LENGTH, "I cannot teach you to follow the path of the %s&n!", specialization_name(ch->player.m_class, i));
					mobsay(teacher, buf);
					return;
				}

				if (i + 1 == SPEC_DRAGON_LANCER && !(GET_CLASS(teacher, CLASS_PALADIN) || GET_CLASS(teacher, CLASS_ANTIPALADIN)))
				{
					snprintf(buf, MAX_STRING_LENGTH, "I cannot teach you to follow the path of the %s&n!", specialization_name(ch->player.m_class, i));
					mobsay(teacher, buf);
					return;
				}
			}

			snprintf(buf, MAX_STRING_LENGTH, "From this day onwards you will follow the path of the %s&n!", specialization_name(ch->player.m_class, i));
			ch->player.spec = i + 1;
			update_skills(ch);
			mobsay(teacher, buf);
			return;
		}
	}

	mobsay(teacher, "I'm sorry, but that specialization isn't available to you.");
}

void unspecialize(P_char ch, P_obj obj)
{
	if (!IS_SPECIALIZED(ch))
	{
		send_to_char("You pray to the &+bWater Goddess&n but you get no response.", ch);
	}
	if (GET_EPIC_POINTS(ch) < 10)
	{
		send_to_char("You need 10 epic points to pay for this.\n", ch);
	}
	else
	{
		act("You kneel in front of $p and pray to the \n"
		    "&+bWater Goddess&n. As you continue your meditation, you begin\n"
		    "to feel your mind is breaking free from the old habits and you\n"
		    "feel ready to learn new ways.\n",
		    FALSE,
		    ch,
		    obj,
		    0,
		    TO_CHAR);
		act("$n kneels before $p and sinks in prayers.\n"
		    "After few moments of silence $e smiles and stands up looking reborn.\n",
		    FALSE,
		    ch,
		    obj,
		    0,
		    TO_ROOM);
		ch->player.spec = 0;
		update_skills(ch);
		// epic_gain_skillpoints(ch, -1);
		ch->only.pc->epics -= 10;
		forget_spells(ch, -1);
	}
}
