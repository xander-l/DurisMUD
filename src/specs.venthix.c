// Venthix's procs

#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <list>
#include <vector>
#include <math.h>
using namespace std;

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "spells.h"
#include "specs.prototypes.h"
#include "structs.h"
#include "utils.h"
#include "map.h"
#include "damage.h"

extern P_room world;

int roulette_pistol(P_obj obj, P_char ch, int cmd, char *arg)
{
  int position; //The local of the live round in the pistol, 0 if none exist (fired)
  int dieval = 0;
  
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!obj || !ch)
    return FALSE;

  if (arg && ((cmd == CMD_RELOAD) || (cmd == CMD_FIRE)) && isname(arg, obj->name))
  {
    if (ch->equipment[HOLD] != obj)
    {
      act("You must be holding the pistol to use it.", FALSE, ch, obj, 0, TO_CHAR);
      return TRUE;
    }
  } 

  if (arg && (cmd == CMD_RELOAD) )
  {
    if (isname(arg, obj->name))
    {
    
      // Does the pistol still have a live round?
      if (obj->value[0])
      {
        act("&+yA live round is still loaded.  You spin the chamber and lock it.&n", FALSE, ch, obj, 0, TO_CHAR);
        //  Uhh how do I make $e show as uppercase?
        act("&+y$n opens the chamber and notices a live round already chambered.&L$e spins the chamber and locks it back.&n", FALSE, ch, obj, 0, TO_ROOM);
      }

      // If no live round is found in the pistol...
      if (!obj->value[0])
      {
        act("&+y$n quickly reloads $p &+ywith a live round.", FALSE, ch, obj, 0, TO_ROOM);
        act("&+yYou quickly reload $p &+ywith a live round.", FALSE, ch, obj, 0, TO_CHAR);
      }
      
      // Randomize the live round...  
      obj->value[0] = number(1,6);
  
      return TRUE;
    }
  }
  
  if (arg && (cmd == CMD_FIRE) )
  {
    if (isname(arg, obj->name))
    {
      if (!obj->value[0])
      {
        // Gun is empty, no live round.  Reload before playing...
        act("&+WClick!  &+yThere is no live round in the gun, reload you dummy!", FALSE, ch, obj, 0, TO_CHAR);
        act("&+WClick!  &+y$n&+y tries to play roulette with an empty gun.", FALSE, ch, obj, 0, TO_ROOM);
        return TRUE;
      }
      else
      {
        if ((obj->value[0] > 6) || (obj->value[0] < 0))
        {
          //debug issue with code. Value out of acceptable parameters.
          return TRUE;
        }
        if (obj->value[0] != 1)
        {
          // CLICK, nothing happened, wew!
          act("&+WClick! &+yNothing happened... &+Lweeew!&n", FALSE, ch, obj, 0, TO_CHAR);
          act("&+WClick! &+y$n&+y pulled the trigger, but nothing happened.", FALSE, ch, obj, 0, TO_ROOM);
        }
        else
        {
          // BANG, your dead!
          act("&-RBANG!&n &+WYou're dead!&n", FALSE, ch, obj, 0, TO_CHAR);
          act("&-RBANG!&n &+W$n&+W shot himself... What a loser!", FALSE, ch, obj, 0, TO_ROOM);
          if (!IS_TRUSTED(ch))
          {
            
            //obj_from_char(unequip_char(ch, HOLD), TRUE);
            //obj_to_room(obj, ch->in_room);
            //act("$p&+y drops to the floor.", FALSE, ch, obj, 0, TO_ROOM);
            GET_HIT(ch) = (-100);
          }
          else
          {
            act("&+yThe bullet simply bounces off $n&+y's head.", FALSE, ch, obj, 0, TO_ROOM);
            send_to_char("You can't die!\n", ch);
          }
        }
        obj->value[0]--;

        return TRUE;
      }
    }
  }
  return FALSE;
}

// 67282 &+mt&+Mh&+me &+Mo&+mr&+Mb &+mo&+Mf &+md&+Me&+mc&+Me&+mp&+Mt&+mi&+Mo&+mn
int orb_of_deception(P_obj obj, P_char ch, int cmd, char *arg)
{
  int curr_time = time(NULL);

  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (!obj || !ch)
    return FALSE;

  if (arg && (cmd == CMD_SAY))
  {
    if (isname(arg, "mirage"))
    {
      if (((ch->equipment[WEAR_EARRING_L] == obj) ||
           (ch->equipment[WEAR_EARRING_R] == obj)) &&
          ((obj->timer[0] + 180) <= curr_time))
      {
        act("&+L$n&+L's $p &+Lbegins to vibrate.", FALSE, ch, obj, 0, TO_ROOM);
        act("&+LYour $p &+Lbegins to vibrate.", FALSE, ch, obj, 0, TO_CHAR);
        spell_mirage(51, ch, 0, SPELL_TYPE_SPELL, ch, 0);
        obj->timer[0] = time(NULL);
        return TRUE;
      }
    }
  }
  return FALSE;
}

#define CANNON_PLAYER 0
#define CANNON_AMMO   1
#define CANNON_DIR    2
#define CANNON_FIRING 3

#define COORD_X      0
#define COORD_Y      1

#define M_PI 3.14159265358979323846

struct cannon_data {
  int fuze;  // fuze for countdown
  int ammo;  // gunpowder 1-50
  int dir;   // dir to go
  double x;  // slope data x coordinates east and west
  double y;  // slope data y coordinates north and south
  int doxy[2];   // make the move north south east or west
  int moved; // how many rooms has player moved
};

void get_cannon_heading(void *data)
{
  struct cannon_data *cdata = (struct cannon_data*)data;
  
  if (!cdata)
  {
    debug("Error: get_cannon_heading called with null pointer");
    return;
  }
 
  float rad = (float)cdata->dir * M_PI / 180.000;
  cdata->x += (float) sin(rad);
  cdata->y += (float) cos(rad);

  cdata->doxy[COORD_X] = -1;
  cdata->doxy[COORD_Y] = -1;

  if ((cdata->y >= 51.000) || (cdata->x >= 51.000) || (cdata->y < 50.000) || (cdata->x < 50.000))
  {
    if (cdata->x > 50.999)
    {
      cdata->x -= 1.000;
      cdata->doxy[COORD_X] = EAST;
    }
    else if (cdata->x < 50.000)
    {
      cdata->x += 1.000;
      cdata->doxy[COORD_X] = WEST;
    }
    if (cdata->y > 50.999)
    {
      cdata->y -= 1.000;
      cdata->doxy[COORD_Y] = NORTH;
    }
    else if (cdata->y < 50.000)
    {
      cdata->y += 1.000;
      cdata->doxy[COORD_Y] = SOUTH;
    }
  }
  debug("dir: %d, doxy[x]: %d, doxy[y]: %d, x: %f, y: %f", cdata->dir, cdata->doxy[COORD_X], cdata->doxy[COORD_Y], cdata->x, cdata->y);
  return;
}

void event_super_cannon_fire(P_char ch, P_char vict, P_obj obj, void *data)
{
  struct cannon_data *cdata = (struct cannon_data*)data;
  int troom;
  int dir;

  if (!cdata)
  {
    debug("passed null pointer to event_super_cannon_fire");
    return;
  }

  if (!ch)
  {
    debug("event_super_cannon_fire called with !ch");
    return;
  }

  if (ch)
  {
    //Lets fly through the air!
    cdata->moved++;
    
    if (cdata->moved <= cdata->ammo/2)
      ch->specials.z_cord = MIN((int)get_property("cannon.super.max.z", 15), 
	  (int)((float)cdata->moved/((float)cdata->ammo/2)*(float)cdata->ammo/5));
    else
      ch->specials.z_cord = MIN((int)get_property("cannon.super.max.z", 15),
	  (int)(((float)cdata->ammo-(float)cdata->moved)/((float)cdata->ammo/2)*(float)cdata->ammo/5));

    // handle movement
    get_cannon_heading(cdata);

    for (int n = COORD_X; n <= COORD_Y; n++)
    {
      dir = -1;
      if (cdata->doxy[n] != -1 &&
	  (cdata->doxy[n] == NORTH ||
	   cdata->doxy[n] == EAST ||
	   cdata->doxy[n] == SOUTH ||
	   cdata->doxy[n] == WEST))
	dir = cdata->doxy[n];
      debug("dir: %d", dir); 
      if (dir < -1 || dir > 3)
      {
	debug("Error in event_super_cannon_fire: invalid dir");
	send_to_char("Ack! Something went wrong!\r\n", ch);
	ch->specials.z_cord = 0;
	return;
      }

      if (dir == -1)
        continue;

      if (!EXIT(ch, dir) ||
	  EXIT(ch, dir)->to_room == NOWHERE ||
	  !IS_SURFACE_MAP(EXIT(ch, dir)->to_room))
      {
        send_to_char("&+WSMACK! &+LYou ran into something, that hurt!\r\n", ch);
        ch->specials.z_cord = 0;
        spell_damage(ch, ch, 20, SPLDAM_GENERIC,
                SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, 0);
        SET_POS(ch, GET_STAT(ch) + POS_PRONE);
        return;
      }
      troom = EXIT(ch, dir)->to_room;
      //can we enter next room or did we hit something bad?
      if (IS_FIGHTING(ch))
        stop_fighting(ch);
    
      if (ch->in_room != NOWHERE)
        for (P_char attacker = world[ch->in_room].people; attacker; attacker = attacker->next)
	  if (IS_FIGHTING(attacker) && (attacker->specials.fighting == ch))
	    stop_fighting(attacker);

      if (IS_PC(ch) && IS_RIDING(ch))
        stop_riding(ch);
      if (P_char rider = GET_RIDER(ch))
        stop_riding(rider);
    
      char_from_room(ch);
      char_to_room(ch, troom, -1);
    }
    
    if (cdata->moved >= cdata->ammo)
    {
      //landed!
      ch->specials.z_cord = 0;
      if (world[ch->in_room].sector_type == SECT_WATER_SWIM ||
          world[ch->in_room].sector_type == SECT_WATER_NOSWIM ||
          world[ch->in_room].sector_type == SECT_OCEAN)
      {
        send_to_char("&+LYou &+Bsplash &+Linto the &+Bwater &+Las you land!\r\n", ch);
      }
      else
      {	  
        send_to_char("&+LYou've landed!  You roll as you hit the ground.&n\r\n", ch);
        if (!affected_by_spell(ch, SPELL_FLY) &&
          GET_CHAR_SKILL(ch, SKILL_SAFE_FALL) >= number(0, 100))
        {
          send_to_char("&+WOuch! &+LYou should try a fly spell next time.&n\r\n", ch);
          spell_damage(ch, ch, 20, SPLDAM_GENERIC,
              SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, 0);
        }
        SET_POS(ch, GET_STAT(ch) + POS_PRONE);
      }
      return;
    }

    add_event(event_super_cannon_fire, 1, ch, 0, 0, 0, data, sizeof(struct cannon_data));
  }
}

void event_super_cannon(P_char pl, P_char vict, P_obj obj, void *data)
{
  struct cannon_data *cdata = (struct cannon_data*)data;
  P_char ch;
  double slope, x, y;
  int dir, offsetaim;

  if (!cdata)
  {
    debug("Passed null pointer to event_super_cannon");
    obj->value[CANNON_PLAYER] = 0;
    return;
  }

  // If our projectile still in the room?
  if (obj->loc.room > 0)
  {
    for (P_char tch = world[obj->loc.room].people; tch; tch = tch->next_in_room )
    {
      if (IS_PC(tch) && GET_PID(tch) == obj->value[CANNON_PLAYER])
      {
        ch = tch;
        break;
      }
    }
    if (!ch || !IS_ALIVE(ch))
    {
      obj->value[CANNON_PLAYER] = 0;
      return;
    }
  }

  cdata->fuze--;

  if (cdata->fuze == 2)
  {
    //perform lighting of fuze
    act("&+LA &+Rgnomish &+Ct&+Bi&+Mn&+Rk&+Ge&+Yr &+Lruns in from the s&+bh&+La&+bd&+Lo&+bw&+Ls and &+rl&+Ri&+rgh&+Rt&+rs &+Lthe fuze before everyone is ready!", FALSE, ch, obj, 0, TO_CHAR);
    act("&+LA &+Rgnomish &+Ct&+Bi&+Mn&+Rk&+Ge&+Yr &+Lruns in from the s&+bh&+La&+bd&+Lo&+bw&+Ls and &+rl&+Ri&+rgh&+Rt&+rs &+Lthe fuze before everyone is ready!", FALSE, ch, obj, 0, TO_ROOM);
    add_event(event_super_cannon, WAIT_SEC, 0, 0, obj, 0, data, sizeof(struct cannon_data));
    return;
  }
  else if (cdata->fuze == 1)
  {
    //perform fuze burning msg
    act("&+LThe fuze &+Rsi&+rzz&+Rl&+re&+Rs &+Lquickly...&n", FALSE, ch, obj, 0, TO_CHAR);
    act("&+LThe fuze &+Rsi&+rzz&+Rl&+re&+Rs &+Lquickly...&n", FALSE, ch, obj, 0, TO_ROOM);
    add_event(event_super_cannon, WAIT_SEC, 0, 0, obj, 0, data, sizeof(struct cannon_data));
    return;
  }
  else if (cdata->fuze == 0)
  {
    //perform firing event on player
    act("&-RBOOOOOOOOOOOOOOOOM!", FALSE, ch, obj, 0, TO_CHAR);
    act("&-RBOOOOOOOOOOOOOOOOM!", FALSE, ch, obj, 0, TO_ROOM);
    act("$n has a shocked look on his face as he flies out of $p.", FALSE, ch, obj, 0, TO_ROOM);
    obj->value[CANNON_PLAYER] = 0;
    obj->value[CANNON_FIRING] = FALSE; 
    cdata->x = 50.500;
    cdata->y = 50.500;
    cdata->doxy[COORD_X] = -1;
    cdata->doxy[COORD_Y] = -1;
    cdata->moved = 0; 
    offsetaim = cdata->ammo / 50;
    cdata->dir += number(-offsetaim, offsetaim);  // So the aim isn't perfect
    add_event(event_super_cannon_fire, 0, ch, 0, 0, 0, data, sizeof(struct cannon_data));
    return;
  }
}

int super_cannon(P_obj obj, P_char ch, int cmd, char *arg)
{
  char load[MAX_STRING_LENGTH], dirstr[MAX_STRING_LENGTH];
  char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int i, found = 0;
  
  struct cannon_data cdata;
  cdata.fuze = 3;

  if (cmd == CMD_SET_PERIODIC)
  {
    // Make sure the cannon is reset on object creation
    for (i = 0; i < 6; i++)
    {
      obj->value[i] = 0;
    }
    return FALSE;
  }

  if (cmd == CMD_PERIODIC)
    return FALSE;

  if (!obj || !ch)
    return FALSE;

  if (!IS_PC(ch))
    return FALSE;

  if (arg && (cmd == CMD_ORDER))
  {
    half_chop(arg, arg1, arg2);
    if (!isname(arg1, "cannon"))
      return FALSE;
  
    // If player isn't in room, reset the cannon for another's use.
    for (P_char tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
    {
      if (IS_PC(tch) && GET_PID(tch) == obj->value[CANNON_PLAYER])
        found = TRUE;
    }
    if (!found)
    {
      obj->value[CANNON_PLAYER] = 0;
      obj->value[CANNON_FIRING] = FALSE;
    }
    if (obj->value[CANNON_PLAYER] != GET_PID(ch) &&
	obj->value[CANNON_PLAYER] != 0)
    {
      act("$p is already in use!", FALSE, ch, obj, 0, TO_CHAR);
      return TRUE;
    }

    argument_interpreter(arg2, load, dirstr);
    
    if (!*load || !*dirstr || !isdigit(*load) || !isdigit(*dirstr))
    {
      act("Please specify the load and direction.  Example: Order Cannon 10 45", FALSE, ch, 0, 0, TO_CHAR);
      return TRUE;
    }
    if (atoi(load) <= 0)
    {
      act("Zero ammo wont get you very far.", FALSE, ch, 0, 0, TO_CHAR);
      return TRUE;
    }
    if (atoi(load) > (int)get_property("cannon.super.range", 500))
    {
      sprintf(buf, "Are you trying to blow up the cannon? That's too much gunpowder! Try %d or less.", (int)get_property("cannon.super.range", 500));
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
      return TRUE;
    }
    
    obj->value[CANNON_AMMO] = atoi(load);
    obj->value[CANNON_DIR] = atoi(dirstr);
    obj->value[CANNON_PLAYER] = GET_PID(ch);
    sprintf(buf, "&+L$n &+Lloads &+L$p &+Lwith %d pounds of gunpowder and tweaks it's aim.  Grinning, $e hops in.", atoi(load));
    act(buf, FALSE, ch, obj, 0, TO_ROOM);
    sprintf(buf, "&+LYou load $p &+Lwith %d pounds of gunpowder and aim it.", atoi(load));
    act(buf, FALSE, ch, obj, 0, TO_CHAR);
    return TRUE;
  }

  if (arg && (cmd == CMD_SHOUT) &&
      obj->value[CANNON_FIRING] != TRUE)
  {

    if (isname(arg, "fire"))
    {
      if (obj->value[CANNON_PLAYER] == 0)
      {
        act("$p isn't primed for firing yet.", FALSE, ch, obj, 0, TO_CHAR);
        return TRUE;
      }
    
      if (obj->value[CANNON_PLAYER] != GET_PID(ch))
      {
        act("$p is already in use!", FALSE, ch, obj, 0, TO_CHAR);
        return TRUE;
      }

      if (!IS_SURFACE_MAP(ch->in_room))
      {
	act("Sorry, this cannon only works on the surface.", FALSE, ch, 0, 0, TO_CHAR);
	obj->value[CANNON_PLAYER] = 0;
	return TRUE;
      }
      obj->value[CANNON_FIRING] = TRUE;
      cdata.ammo = obj->value[CANNON_AMMO];
      cdata.dir = obj->value[CANNON_DIR];
      add_event(event_super_cannon, WAIT_SEC, 0, 0, obj, 0, &cdata, sizeof(cdata));
      return FALSE;
    }
  }
  return FALSE;
}

void halloween_mine_proc(P_char ch)
{
  char buff[MAX_STRING_LENGTH];
  sprintf(buff, " %s 86", GET_NAME(ch));
  act("Your dig hits a burried &+ypumpkin&n.&LSuddenly it begins to move and digs itself out of the mine!", TRUE, ch, 0, 0, TO_CHAR);
  act("$n dig hits a burried &+ypumpkin&n.&LSuddenly it begins to move and digs itself out of the mine!", TRUE, ch, 0, 0, TO_ROOM);
  do_givepet(ch, buff, CMD_GIVEPET);
}
