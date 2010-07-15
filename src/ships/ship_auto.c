/*****************************************************
* ship_auto.c
*
* Ship autopilot
*****************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "comm.h"
#include "db.h"
#include "graph.h"
#include "interp.h"
#include "objmisc.h"
#include "prototypes.h"
//#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "events.h"
#include "map.h"
#include "ships.h"
#include "ship_auto.h"

void  act_to_all_in_ship(P_ship ship, const char *msg);
int   getmap(P_ship ship);
int   bearing(float x1, float y1, float x2, float y2);
float range(float x1, float y1, float z1, float x2, float y2, float z2);

//Internal variables
struct shipai_data *shipai;

void initialize_shipai()
{
  shipai = NULL;
}

int xbearing(int bearing, int range)
{
  float    rad;

  rad = (float) ((float) bearing * M_PI / 180);
  return (int) (range * sin(rad));
}

int ybearing(int bearing, int range)
{
  float    rad;

  rad = (float) ((float) bearing * M_PI / 180);
  return (int) (range * cos(rad));
}

int assign_shipai(P_ship ship)
{
  struct shipai_data *ai;
  int      i;

  if (!ship)
  {
    return FALSE;
  }
  if (ship->shipai)
  {
    if (IS_SET(ship->shipai->flags, AIB_ENABLED))
    {
      return FALSE;
    }
    else
    {
      ai = ship->shipai;
    }
  }
  else
  {
    CREATE(ai, shipai_data, 1, MEM_TAG_SHIPAI);

    ship->shipai = ai;
  }

  ai->type = 0;
  ai->target = NULL;
  ai->flags = 0;
  ai->t_room = 0;
  ai->ship = ship;
  ai->group = NULL;
  for (i = 0; i < MAXAITIMER; i++)
  {
    ai->timer[i] = 0;
  }
  SET_BIT(ai->flags, AIB_ENABLED);
  return TRUE;
}

int shipgroupremove(struct shipai_data *shipai)
{
  struct shipgroup_data *tmpgroup;
  struct shipgroup_data *tmpgroup2;
  struct shipai_data *leader = NULL;

  if (!shipai)
  {
    return FALSE;
  }
  if (!shipai->group)
  {
    return FALSE;
  }

  if (shipai->group->leader == shipai)
  {
    if (!shipai->group->next)
    {
      shipai->group->leader = NULL;
      shipai->group->ai = NULL;
      FREE(shipai->group);
      shipai->group = NULL;
      return TRUE;
    }
    tmpgroup2 = shipai->group;
    tmpgroup = shipai->group->next;
    tmpgroup2->leader = NULL;
    tmpgroup2->ai = NULL;
    FREE(tmpgroup2);
    shipai->group = NULL;

    while (tmpgroup)
    {
      if (IS_SET(shipai->flags, AIB_DRONE))
      {
        //Destroy drone code
        tmpgroup2 = tmpgroup;
        tmpgroup = tmpgroup->next;
        tmpgroup2->ai->group = NULL;
        tmpgroup2->leader = NULL;
        tmpgroup2->ai = NULL;
        FREE(tmpgroup2);
        continue;
      }
      if (IS_SET(shipai->flags, AIB_MOB))
      {
        if (!leader)
        {
          tmpgroup->leader = shipai;
          leader = shipai;
        }
        else
        {
          tmpgroup->leader = leader;
        }
        if (IS_SET(shipai->flags, AIB_HUNTER))
        {
          shipai->mode = AIM_SEEK;
        }
        else
        {
          shipai->mode = AIM_WAIT;
        }
      }
      tmpgroup = tmpgroup->next;
    }
    return TRUE;
  }
  tmpgroup = shipai->group->leader->group;
  while (tmpgroup)
  {
    if ((tmpgroup = shipai->group))
    {
      tmpgroup2 = tmpgroup;
      tmpgroup = tmpgroup->next;
      tmpgroup2->leader = NULL;
      tmpgroup2->ai = NULL;
      FREE(tmpgroup2);
      shipai->group = NULL;
      continue;
    }
    tmpgroup = tmpgroup->next;
  }
  return TRUE;
}

int shipgroupadd(struct shipai_data *shipai, struct shipgroup_data *group)
{
  struct shipgroup_data *newgroup;
  struct shipgroup_data *tmpgroup;

  if (!shipai)
  {
    return FALSE;
  }
  if (shipai->group)
  {
    return FALSE;
  }
  if (!group)
  {
    CREATE(group, shipgroup_data, 1, MEM_TAG_SHIPGRP);

    group->leader = shipai;
    group->next = NULL;
    group->ai = shipai;
    shipai->group = group;
    return TRUE;
  }
  else
  {
    CREATE(newgroup, shipgroup_data, 1, MEM_TAG_SHIPGRP);

    newgroup->leader = group->leader;
    newgroup->ai = shipai;
    newgroup->next = NULL;
    tmpgroup = group;
    while (tmpgroup->next)
    {
      tmpgroup = tmpgroup->next;
    }
    tmpgroup->next = newgroup;
    shipai->group = newgroup;
    return TRUE;
  }
}

void announceheading(P_ship ship, int heading)
{
  char     tbuf[MAX_STRING_LENGTH];

  sprintf(tbuf, "AI:Heading changed to %d", heading);
  act_to_all_in_ship(ship, tbuf);
}

void announcespeed(P_ship ship, int speed)
{
  char     tbuf[MAX_STRING_LENGTH];

  sprintf(tbuf, "AI:Speed changed to %d", speed);
  act_to_all_in_ship(ship, tbuf);
}

void aishipspeedadjust(P_ship ship, int speed)
{
  if (speed > ship->maxspeed)
  {
    speed = ship->maxspeed;
  }
  if (speed != ship->setspeed)
  {
    ship->setspeed = speed;
    announcespeed(ship, ship->setspeed);
  }
}

void shipai_activity(P_ship ship)
{
  struct shipai_data *ai;
  int      i, j, k, b, x, y;
  float    r;

  if (ship->shipai == NULL)
  {
    return;
  }
  if (!IS_SET(ship->shipai->flags, AIB_ENABLED))
  {
    return;
  }
  ai = ship->shipai;
/* Ship AI starts here */

  switch (ai->type)
  {
  case AI_LINE:
    getmap(ship);
    k = 0;
    for (i = 0; i < 101; i++)
    {
      for (j = 0; j < 101; j++)
      {
        if (tactical_map[i][100 - j].rroom == ai->t_room)
        {
          k = 1;
          x = i;
          y = j;
          b = bearing(50, 50, i, j);
          if (ship->setheading != b && ai->t_room != ship->location)
          {
            ship->setheading = b;
            announceheading(ship, b);
          }
        }
      }
    }
    if (k)
    {
      if (ship->setheading != ship->heading)
      {
        aishipspeedadjust(ai->ship, 0);
      }
      else
      {
        if (ship->location == ai->t_room)
        {
          aishipspeedadjust(ai->ship, 0);
          if (IS_SET(ai->flags, AIB_AUTOPILOT))
          {
            REMOVE_BIT(ai->flags, AIB_AUTOPILOT);
            REMOVE_BIT(ai->flags, AIB_ENABLED);
            act_to_all_in_ship(ship, "Destination has been reached.");
            return;
          }
        }
        else
        {
          r = 0;
          r = range(50, 50, 0, x, y, 0);
          if (IS_SET(ai->flags, AIB_BATTLER))
          {
            aishipspeedadjust(ship, ship->maxspeed);
          }
          else if (r < 2.70)
          {
            aishipspeedadjust(ai->ship, 10);
          }
          else if (r < 5.00)
          {
            aishipspeedadjust(ai->ship, 30);
          }
          else if (r < 10.00)
          {
            aishipspeedadjust(ai->ship, 60);
          }
          else
          {
            aishipspeedadjust(ship, ship->maxspeed);
          }
        }
      }
    }
    else
    {
      act_to_all_in_ship(ai->ship, "Target room not found!\r\n");
    }
    break;
  case AI_STOP:
    break;
  case AI_PATH:
    break;
  default:
    break;
  }
}


