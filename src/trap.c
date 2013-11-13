#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"

extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;

#define TRAP_DAM_SLEEP		0
#define TRAP_DAM_TELEPORT 	1
#define TRAP_DAM_FIRE      	2
#define TRAP_DAM_COLD       	3
#define TRAP_DAM_ACID  		4
#define TRAP_DAM_ENERGY   	5
#define TRAP_DAM_BLUNT   	6
#define TRAP_DAM_PIERCE 	7
#define TRAP_DAM_SLASH 		8

#define TRAP_EFF_MOVE       1   /* trigger on movement */
#define TRAP_EFF_OBJECT     2   /* trigger on get or put */
#define TRAP_EFF_ROOM       4   /* affect all in room */
#define TRAP_EFF_NORTH      8   /* movement in this direction */
#define TRAP_EFF_EAST       16
#define TRAP_EFF_SOUTH      32
#define TRAP_EFF_WEST       64
#define TRAP_EFF_UP         128
#define TRAP_EFF_DOWN       256
#define TRAP_EFF_OPEN       512 /* trigger on open */

void do_trapremove(P_char ch, char *argument, int cmd)
{
  P_obj    obj;
  char     arg1[MAX_INPUT_LENGTH];

  argument = one_argument(argument, arg1);

  if (!(obj = get_obj_vis(ch, argument)))
  {
    send_to_char("That is not here!\r\n", ch);
    return;
  }
  obj->trap_charge = 0;
  send_to_char("Trap removed.\r\n", ch);
  return;
}

void do_trapstat(P_char ch, char *argument, int cmd)
{
  char     buf[MAX_STRING_LENGTH];
  P_obj    obj;

  if (!*argument)
    return;

  if (!(obj = get_obj_vis(ch, argument)))
  {
    send_to_char("That is not here!\r\n", ch);
    return;
  }
  if (obj->trap_charge)
  {
    send_to_char("That object is registered as a trap.\r\n", ch);
  }
  else
  {
    send_to_char("That object in not registered as a trap.\r\n", ch);
  }
  switch (obj->trap_dam)
  {
  case TRAP_DAM_SLEEP:
    send_to_char("Damage type is sleep.\r\n", ch);
    break;
  case TRAP_DAM_TELEPORT:
    send_to_char("Damage type is teleport.\r\n", ch);
    break;
  case TRAP_DAM_FIRE:
    send_to_char("Damage type is fire.\r\n", ch);
    break;
  case TRAP_DAM_COLD:
    send_to_char("Damage type is frost.\r\n", ch);
    break;
  case TRAP_DAM_ACID:
    send_to_char("Damage type is acid.\r\n", ch);
    break;
  case TRAP_DAM_ENERGY:
    send_to_char("Damage type is energy.\r\n", ch);
    break;
  case TRAP_DAM_BLUNT:
    send_to_char("Damage type is blunt.\r\n", ch);
    break;
  case TRAP_DAM_PIERCE:
    send_to_char("Damage type is pierce.\r\n", ch);
    break;
  case TRAP_DAM_SLASH:
    send_to_char("Damage type is slash.\r\n", ch);
    break;
  }

  if (obj->trap_eff == 0)
  {
    send_to_char("The trap has no effect.\r\n", ch);
  }
  else
  {
    if (IS_SET(obj->trap_eff, TRAP_EFF_MOVE))
    {
      if (IS_SET(obj->trap_eff, TRAP_EFF_NORTH))
        send_to_char("The trap affects movement to the north.\r\n", ch);
      if (IS_SET(obj->trap_eff, TRAP_EFF_SOUTH))
        send_to_char("The trap affects movement to the south.\r\n", ch);
      if (IS_SET(obj->trap_eff, TRAP_EFF_EAST))
        send_to_char("The trap affects movement to the east.\r\n", ch);
      if (IS_SET(obj->trap_eff, TRAP_EFF_WEST))
        send_to_char("The trap affects movement to the west.\r\n", ch);
      if (IS_SET(obj->trap_eff, TRAP_EFF_UP))
        send_to_char("The trap affects movement up.\r\n", ch);
      if (IS_SET(obj->trap_eff, TRAP_EFF_DOWN))
        send_to_char("The trap affects movement down.\r\n", ch);
    }
    if (IS_SET(obj->trap_eff, TRAP_EFF_OBJECT))
      send_to_char("The trap is set off by get or put.\r\n", ch);
    if (IS_SET(obj->trap_eff, TRAP_EFF_OPEN))
      send_to_char("The trap is set off when opened.\r\n", ch);
    if (IS_SET(obj->trap_eff, TRAP_EFF_ROOM))
      send_to_char("The trap affects the whole room.\r\n", ch);
  }
  sprintf(buf, "Trap Charges left: %d.\r\n", obj->trap_charge);
  send_to_char(buf, ch);
  sprintf(buf, "Trap level: %d\r\n", obj->trap_level);
  send_to_char(buf, ch);
  return;
}

void do_traplist(P_char ch, char *argument, int cmd)
{
  char     buf[MAX_INPUT_LENGTH];
  P_obj    obj;
  bool     found = FALSE;

  for (obj = object_list; obj != NULL; obj = obj->next)
  {
    if (!CAN_SEE_OBJ(ch, obj) || !obj->trap_charge)
      continue;

    found = TRUE;
    sprintf(buf, "%s \r\n", where_obj(obj, FALSE));
    send_to_char(buf, ch);
  }
  if (!found)
    send_to_char("No traps found.\r\n", ch);

  return;
}

void do_trapset(P_char ch, char *argument, int cmd)
{
  P_obj    obj;
  char     arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH],
    arg3[MAX_INPUT_LENGTH];
  int      val = 0;
  P_char   dummy;
  int      bits;

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  argument = one_argument(argument, arg3);
  if (arg1[0] == '\0' || arg2[0] == '\0')
  {
    send_to_char("Syntax: trapset <object> <field> <value>\r\n", ch);
    send_to_char
      ("Field: move, object(get and put), room, open, damage, charge\r\n",
       ch);
    send_to_char
      ("Values: Move> north, south, east, west, up, down, and all.\r\n", ch);
    send_to_char
      ("        Damage> sleep, teleport, fire, cold, acid, energy,\r\n", ch);
    send_to_char("                blunt, pierce, slash.\r\n", ch);
    send_to_char("        Object, open, room> no values\r\n", ch);
    return;
  }
  if (!
      (bits =
       generic_find(arg1, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &dummy, &obj)))
  {
    send_to_char("Nothing like that here!\r\n", ch);
    return;
  }
  if (!obj->trap_eff)
    obj->trap_eff = 1;

  if (!str_cmp(arg2, "move"))
  {
    if (arg3[0] == '\0')
    {
      send_to_char("Syntax: trapset <object> <field> <value>\r\n", ch);
      send_to_char("Field: move, object(get and put), room, open, damage\r\n",
                   ch);
      send_to_char
        ("Values: Move> north, south, east, west, up, down, and all.\r\n",
         ch);
      send_to_char
        ("        Damage> sleep, teleport, fire, cold, acid, energy,\r\n",
         ch);
      send_to_char("                blunt, pierce, slash.\r\n", ch);
      send_to_char("        Object, open, room> no values\r\n", ch);
      return;
    }
    if (!str_cmp(arg3, "north"))
    {
      if (!IS_SET(obj->trap_eff, TRAP_EFF_MOVE))
        SET_BIT(obj->trap_eff, TRAP_EFF_MOVE);
      if (!IS_SET(obj->trap_eff, TRAP_EFF_NORTH))
        SET_BIT(obj->trap_eff, TRAP_EFF_NORTH);
      send_to_char("You set a trap for northward movement!\r\n", ch);
      return;
    }
    else if (!str_cmp(arg3, "south"))
    {
      if (!IS_SET(obj->trap_eff, TRAP_EFF_MOVE))
        SET_BIT(obj->trap_eff, TRAP_EFF_MOVE);
      if (!IS_SET(obj->trap_eff, TRAP_EFF_SOUTH))
        SET_BIT(obj->trap_eff, TRAP_EFF_SOUTH);
      send_to_char("You set a trap for southern movement!\r\n", ch);
      return;
    }
    else if (!str_cmp(arg3, "east"))
    {
      if (!IS_SET(obj->trap_eff, TRAP_EFF_MOVE))
        SET_BIT(obj->trap_eff, TRAP_EFF_MOVE);
      if (!IS_SET(obj->trap_eff, TRAP_EFF_EAST))
        SET_BIT(obj->trap_eff, TRAP_EFF_EAST);
      send_to_char("You set a trap for eastern movement!\r\n", ch);
      return;
    }
    else if (!str_cmp(arg3, "west"))
    {
      if (!IS_SET(obj->trap_eff, TRAP_EFF_MOVE))
        SET_BIT(obj->trap_eff, TRAP_EFF_MOVE);
      if (!IS_SET(obj->trap_eff, TRAP_EFF_WEST))
        SET_BIT(obj->trap_eff, TRAP_EFF_WEST);
      send_to_char("You set a trap for western movement!\r\n", ch);
      return;
    }
    else if (!str_cmp(arg3, "up"))
    {
      if (!IS_SET(obj->trap_eff, TRAP_EFF_MOVE))
        SET_BIT(obj->trap_eff, TRAP_EFF_MOVE);
      if (!IS_SET(obj->trap_eff, TRAP_EFF_UP))
        SET_BIT(obj->trap_eff, TRAP_EFF_UP);
      send_to_char("You set a trap for upward movement!\r\n", ch);
      return;
    }
    else if (!str_cmp(arg3, "down"))
    {
      if (!IS_SET(obj->trap_eff, TRAP_EFF_MOVE))
        SET_BIT(obj->trap_eff, TRAP_EFF_MOVE);
      if (!IS_SET(obj->trap_eff, TRAP_EFF_DOWN))
        SET_BIT(obj->trap_eff, TRAP_EFF_DOWN);
      send_to_char("You set a trap for downward movement!\r\n", ch);
      return;
    }
    else if (!str_cmp(arg3, "all"))
    {
      if (!IS_SET(obj->trap_eff, TRAP_EFF_MOVE))
        SET_BIT(obj->trap_eff, TRAP_EFF_MOVE);
      if (!IS_SET(obj->trap_eff, TRAP_EFF_DOWN))
        SET_BIT(obj->trap_eff, TRAP_EFF_DOWN);
      if (!IS_SET(obj->trap_eff, TRAP_EFF_UP))
        SET_BIT(obj->trap_eff, TRAP_EFF_UP);
      if (!IS_SET(obj->trap_eff, TRAP_EFF_EAST))
        SET_BIT(obj->trap_eff, TRAP_EFF_EAST);
      if (!IS_SET(obj->trap_eff, TRAP_EFF_WEST))
        SET_BIT(obj->trap_eff, TRAP_EFF_WEST);
      if (!IS_SET(obj->trap_eff, TRAP_EFF_NORTH))
        SET_BIT(obj->trap_eff, TRAP_EFF_NORTH);
      if (!IS_SET(obj->trap_eff, TRAP_EFF_SOUTH))
        SET_BIT(obj->trap_eff, TRAP_EFF_SOUTH);
      send_to_char("You set a trap for all movement!\r\n", ch);
      return;
    }
    send_to_char("Value specified was not a valid option for move.\r\n", ch);
    return;
  }
  else if (!str_cmp(arg2, "object"))
  {
    if (!IS_SET(obj->trap_eff, TRAP_EFF_OBJECT))
      SET_BIT(obj->trap_eff, TRAP_EFF_OBJECT);
    send_to_char("You have set an object(get or put) trap!\r\n", ch);
    return;
  }
  else if (!str_cmp(arg2, "room"))
  {
    if (!IS_SET(obj->trap_eff, TRAP_EFF_ROOM))
      SET_BIT(obj->trap_eff, TRAP_EFF_ROOM);
    send_to_char("You have made the trap affect the whole room!\r\n", ch);
    return;
  }
  else if (!str_cmp(arg2, "open"))
  {
    if (!IS_SET(obj->trap_eff, TRAP_EFF_OPEN))
      SET_BIT(obj->trap_eff, TRAP_EFF_OPEN);
    send_to_char("You have made the trap spring when opened!\r\n", ch);
    return;
  }
  else if (!str_cmp(arg2, "damage"))
  {
    if (arg3[0] == '\0')
    {
      send_to_char("You need to specify a value!\r\n", ch);
      return;
    }
    else if (!str_cmp(arg3, "sleep"))
    {
      obj->trap_dam = TRAP_DAM_SLEEP;
      send_to_char("You have made the trap put people to sleep!\r\n", ch);
      return;
    }
    else if (!str_cmp(arg3, "teleport"))
    {
      obj->trap_dam = TRAP_DAM_TELEPORT;
      send_to_char("The trap will now teleport people!\r\n", ch);
      return;
    }
    else if (!str_cmp(arg3, "fire"))
    {
      obj->trap_dam = TRAP_DAM_FIRE;
      send_to_char("The trap will now shoot fire!\r\n", ch);
      return;
    }
    else if (!str_cmp(arg3, "cold"))
    {
      obj->trap_dam = TRAP_DAM_COLD;
      send_to_char("The trap will now freeze its victims!\r\n", ch);
      return;
    }
    else if (!str_cmp(arg3, "acid"))
    {
      obj->trap_dam = TRAP_DAM_ACID;
      send_to_char("You carefully fill the trap with acid!\r\n", ch);
      return;
    }
    else if (!str_cmp(arg3, "energy"))
    {
      obj->trap_dam = TRAP_DAM_ENERGY;
      send_to_char("The trap will now zap people with energy!\r\n", ch);
      return;
    }
    else if (!str_cmp(arg3, "blunt"))
    {
      obj->trap_dam = TRAP_DAM_BLUNT;
      send_to_char("The trap will now bludgeon its victims!\r\n", ch);
      return;
    }
    else if (!str_cmp(arg3, "pierce"))
    {
      obj->trap_dam = TRAP_DAM_PIERCE;
      send_to_char("The trap will now pierce its victims!\r\n", ch);
      return;
    }
    else if (!str_cmp(arg3, "slash"))
    {
      obj->trap_dam = TRAP_DAM_SLASH;
      send_to_char("The trap will now slash its victims!\r\n", ch);
      return;
    }
    send_to_char("That is not an allowed value!\r\n", ch);
    return;
  }
  else if (!str_cmp(arg2, "charge"))
  {
    if (arg3[0] == '\0')
    {
      send_to_char("You need to specify how many charges.\r\n", ch);
      return;
    }
    else if ((val = atoi(arg3)) > 100 || val < -1)
    {
      send_to_char
        ("Allowed range is -1 to 100. (0 disarms, -1 unlimited)\r\n", ch);
      return;
    }
    obj->trap_charge = val;
    send_to_char("Charge value set.\r\n", ch);
    return;
  }
  send_to_char("That is not an allowed option!\r\n", ch);
  return;
}

bool checkmovetrap(P_char ch, int dir)
{
  P_obj    obj, obj_next;
  bool     found;

  if (!world[ch->in_room].contents)
    return FALSE;

  for (obj = world[ch->in_room].contents; obj; obj = obj_next)
  {
    obj_next = obj->next_content;

    if (obj->trap_eff && IS_SET(obj->trap_eff, TRAP_EFF_MOVE) &&
        obj->trap_charge)
      found = TRUE;
    else
      found = FALSE;

    if (found)
    {
      if (IS_SET(obj->trap_eff, TRAP_EFF_NORTH) && dir == 0)
      {
        trapdamage(ch, obj);
        return TRUE;
      }
      else if (IS_SET(obj->trap_eff, TRAP_EFF_EAST) && dir == 1)
      {
        trapdamage(ch, obj);
        return TRUE;
      }
      else if (IS_SET(obj->trap_eff, TRAP_EFF_SOUTH) && dir == 2)
      {
        trapdamage(ch, obj);
        return TRUE;
      }
      else if (IS_SET(obj->trap_eff, TRAP_EFF_WEST) && dir == 3)
      {
        trapdamage(ch, obj);
        return TRUE;
      }
      else if (IS_SET(obj->trap_eff, TRAP_EFF_UP) && dir == 4)
      {
        trapdamage(ch, obj);
        return TRUE;
      }
      else if (IS_SET(obj->trap_eff, TRAP_EFF_DOWN) && dir == 5)
      {
        trapdamage(ch, obj);
        return TRUE;
      }
    }
  }
  return FALSE;
}

bool checkgetput(P_char ch, P_obj obj)
{

  if (!obj->trap_charge)
    return FALSE;

  if (IS_SET(obj->trap_eff, TRAP_EFF_OBJECT))
  {
    trapdamage(ch, obj);
    return TRUE;
  }
  return FALSE;
}

bool checkopen(P_char ch, P_obj obj)
{

  if (!obj->trap_charge)
    return FALSE;

  if (IS_SET(obj->trap_eff, TRAP_EFF_OPEN))
  {
    trapdamage(ch, obj);
    return TRUE;
  }
  return FALSE;
}

void trapdamage(P_char ch, P_obj obj)
{
  struct affected_type af;
  P_char   wch, next_ch;
  int      level, dam;

  if (!obj->trap_charge)
    return;

  if (IS_NPC(ch))
    return;
  
  if (obj->trap_charge > 0)
    obj->trap_charge -= 1;

  if (obj->trap_level)
    level = obj->trap_level;
  else
    level = 25;
  if (level > 100)
    level = 100;

  dam = dice(level, 10);

  switch (obj->trap_dam)
  {
  case TRAP_DAM_SLEEP:
    if (!IS_SET(obj->trap_eff, TRAP_EFF_ROOM))
    {
      if (IS_AFFECTED(ch, AFF_SLEEP) || IS_TRUSTED(ch))
        return;

      bzero(&af, sizeof(af));
      af.type = SPELL_SLEEP;
      af.duration = level / 4;
      af.bitvector = AFF_SLEEP;
      affect_to_char(ch, &af);

      act
        ("&+LA strange gas pours forth from $p, making you feel very sleepy ..... zzzzzz",
         FALSE, ch, obj, 0, TO_CHAR);
      if (ch->specials.fighting)
        stop_fighting(ch);
      if (GET_STAT(ch) > STAT_SLEEPING)
      {
        act("&+LA strange gas pours forth from $p, and $n goes to sleep.",
            TRUE, ch, obj, 0, TO_ROOM);
        SET_POS(ch, GET_POS(ch) + STAT_SLEEPING);
      }
    }
    else
    {
      LOOP_THRU_PEOPLE(wch, ch)
      {
        if (IS_AFFECTED(wch, AFF_SLEEP) || IS_TRUSTED(wch))
          continue;

        bzero(&af, sizeof(af));
        af.type = SPELL_SLEEP;
        af.duration = level / 2;
        af.bitvector = AFF_SLEEP;
        affect_to_char(wch, &af);

        act
          ("&+LA strange gas pours forth from $p, making you feel very sleepy ..... zzzzzz",
           FALSE, wch, obj, 0, TO_CHAR);
        if (wch->specials.fighting)
          stop_fighting(wch);
        if (GET_STAT(wch) > STAT_SLEEPING)
        {
          SET_POS(wch, GET_POS(wch) + STAT_SLEEPING);
        }
      }
    }
    break;
  case TRAP_DAM_TELEPORT:
    if (!IS_SET(obj->trap_eff, TRAP_EFF_ROOM))
    {
      send_to_char("You hear a strange... POP!\r\n", ch);
      spell_teleport(60, ch, 0, 0, ch, 0);
    }
    else
    {
      send_to_room("You hear a strange... POP!\r\n", ch->in_room);
/*      LOOP_THRU_PEOPLE(wch, ch)*/
      for (wch = world[ch->in_room].people; wch; wch = next_ch)
      {
        next_ch = wch->next_in_room;

        spell_teleport(60, wch, 0, 0, wch, 0);
      }
    }
    break;
  case TRAP_DAM_FIRE:
    if (!IS_SET(obj->trap_eff, TRAP_EFF_ROOM))
    {
      act("A fireball shoots out of $p and hits $n!", FALSE, ch, obj, 0,
          TO_ROOM);
      act("A fireball shoots out of $p and hits you!", TRUE, ch, obj, 0,
          TO_CHAR);
      spell_fireball(level, ch, NULL, 0, ch, 0);
    }
    else
    {
      act("A fireball shoots out of $p and hits everyone in the room!", FALSE,
          ch, obj, 0, TO_ROOM);
      act("A fireball shoots out of $p and hits everyone in the room!", FALSE,
          ch, obj, 0, TO_CHAR);
/*      LOOP_THRU_PEOPLE(wch, ch)*/
      for (wch = world[ch->in_room].people; wch; wch = next_ch)
      {
        next_ch = wch->next_in_room;

        spell_fireball(level, ch, NULL, 0, wch, 0);
      }
    }
    break;
  case TRAP_DAM_COLD:
    if (!IS_SET(obj->trap_eff, TRAP_EFF_ROOM))
    {
      act("A blast of frost from $p hits $n!", FALSE, ch, obj, NULL, TO_ROOM);
      act("A blast of frost from $p hits you!", FALSE, ch, obj, NULL,
          TO_CHAR);
      spell_cone_of_cold(level, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
    }
    else
    {
      act("A blast of frost from $p fills the room freezing you!", FALSE, ch,
          obj, NULL, TO_ROOM);
      act("A blast of frost from $p fills the room freezing you!", FALSE, ch,
          obj, NULL, TO_CHAR);
/*      LOOP_THRU_PEOPLE(wch, ch)*/
      for (wch = world[ch->in_room].people; wch; wch = next_ch)
      {
        next_ch = wch->next_in_room;

        spell_cone_of_cold(level, ch, NULL, SPELL_TYPE_SPELL, wch, 0);
      }
    }
    break;
  case TRAP_DAM_ACID:
    if (!IS_SET(obj->trap_eff, TRAP_EFF_ROOM))
    {
      act("A blast of acid erupts from $p, burning your skin!", FALSE, ch,
          obj, NULL, TO_CHAR);
      act("A blast of acid erupts from $p, burning $n's skin!", FALSE, ch,
          obj, NULL, TO_ROOM);
      spell_acid_blast(level, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
    }
    else
    {
      act("A blast of acid erupts from $p, burning your skin!", FALSE, ch,
          obj, NULL, TO_ROOM);
      act("A blast of acid erupts from $p, burning your skin!", FALSE, ch,
          obj, NULL, TO_CHAR);
/*      LOOP_THRU_PEOPLE(wch, ch)*/
      for (wch = world[ch->in_room].people; wch; wch = next_ch)
      {
        next_ch = wch->next_in_room;

        spell_acid_blast(level, ch, NULL, SPELL_TYPE_SPELL, wch, 0);
      }
    }
    break;
  case TRAP_DAM_ENERGY:
    if (!IS_SET(obj->trap_eff, TRAP_EFF_ROOM))
    {
      act("A pulse of energy from $p zaps $n!", FALSE, ch, obj, NULL,
          TO_ROOM);
      act("A pulse of energy from $p zaps you!", FALSE, ch, obj, NULL,
          TO_CHAR);
      spell_energy_drain(level, ch, NULL, 0, ch, 0);
    }
    else
    {
      act("A pulse of energy from $p zaps you!", FALSE, ch, obj, NULL,
          TO_ROOM);
      act("A pulse of energy from $p zaps you!", FALSE, ch, obj, NULL,
          TO_CHAR);
/*      LOOP_THRU_PEOPLE(wch, ch)*/
      for (wch = world[ch->in_room].people; wch; wch = next_ch)
      {
        next_ch = wch->next_in_room;

        spell_energy_drain(level, ch, NULL, 0, wch, 0);
      }
    }
    break;
  case TRAP_DAM_BLUNT:
    if (!IS_SET(obj->trap_eff, TRAP_EFF_ROOM))
    {
      act("$n sets off a trap on $p and a blunt object flies forth!", FALSE,
          ch, obj, NULL, TO_ROOM);
      act("You are hit by a blunt object from $p!", FALSE, ch, obj, NULL,
          TO_CHAR);
      damage(ch, ch, dam, TYPE_TRAP);
    }
    else
    {
      act("$n sets off a trap on $p and you are hit by a flying object!",
          FALSE, ch, obj, NULL, TO_ROOM);
      act("You are hit by a flying blunt object from $p!", FALSE, ch, obj,
          NULL, TO_CHAR);
/*      LOOP_THRU_PEOPLE(wch, ch)*/
      for (wch = world[ch->in_room].people; wch; wch = next_ch)
      {
        next_ch = wch->next_in_room;

        damage(ch, wch, dam, TYPE_TRAP);
      }
    }
    break;
  case TRAP_DAM_PIERCE:
    if (!IS_SET(obj->trap_eff, TRAP_EFF_ROOM))
    {
      act("$n sets off a trap on $p and is pierced in the chest!", FALSE, ch,
          obj, NULL, TO_ROOM);
      act("You set off a trap on $p and are pierced through the chest!",
          FALSE, ch, obj, NULL, TO_CHAR);
      damage(ch, ch, dam, TYPE_TRAP);
    }
    else
    {
      act("$n sets off a trap on $p and you are hit by a piercing object!",
          FALSE, ch, obj, NULL, TO_ROOM);
      act("You set off a trap on $p and are pierced through the chest!",
          FALSE, ch, obj, NULL, TO_CHAR);
/*      LOOP_THRU_PEOPLE(wch, ch)*/
      for (wch = world[ch->in_room].people; wch; wch = next_ch)
      {
        next_ch = wch->next_in_room;

        damage(ch, wch, dam, TYPE_TRAP);
      }
    }
    break;
  case TRAP_DAM_SLASH:
    if (!IS_SET(obj->trap_eff, TRAP_EFF_ROOM))
    {
      act("$n just got slashed by a trap on $p.", FALSE, ch, obj, NULL,
          TO_ROOM);
      act("You just got slashed by a trap on $p!", FALSE, ch, obj, NULL,
          TO_CHAR);
      dam = (dam / 5);
      damage(ch, ch, dam, TYPE_TRAP);
    }
    else
    {
      act("$n set off a trap releasing a blade that slashes you!", FALSE, ch,
          obj, NULL, TO_ROOM);
      act("You set off a trap releasing blades around the room..", FALSE, ch,
          obj, NULL, TO_CHAR);
      act("One of the blades slashes you in the chest!", FALSE, ch, obj, NULL,
          TO_CHAR);
/*      LOOP_THRU_PEOPLE(wch, ch)*/
      for (wch = world[ch->in_room].people; wch; wch = next_ch)
      {
        next_ch = wch->next_in_room;

        damage(ch, wch, dam, TYPE_TRAP);
      }
    }
    break;
  }
}
