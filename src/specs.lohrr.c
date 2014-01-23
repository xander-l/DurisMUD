#include "comm.h"
#include "damage.h"
#include "db.h"
#include "graph.h"
#include "interp.h"
#include "justice.h"
#include "objmisc.h"
#include "prototypes.h"
#include "ships.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include <string.h>

extern P_room world;
extern int top_of_objt;
extern bool has_skin_spell(P_char);
extern P_town towns;

int adjacent_room_nesw(P_char ch, int num_rooms );
P_ship leviathan_find_ship( P_char leviathan, int room, int num_rooms );
void explode_ammo( P_char ch, P_obj ammo );
bool is_siege( P_obj weapon );
bool is_loading( P_obj siege );
void damage_siege( P_obj siege, P_obj ammo );
P_obj get_siege_room( P_char ch, char *arg );
void save_towns();
void apply_zone_modifier(P_char ch);
int castlewall( P_obj obj, P_char ch, int cmd, char *arg );

// This is an old proc for Lohrr's eq..
int proc_lohrr( P_obj obj, P_char ch, int cmd, char *argument )
{
  int  locwearing;
  char buf[MAX_STRING_LENGTH];

  if( cmd == CMD_SET_PERIODIC )
    return TRUE;

  // First, verify that it was called properly
  if( !obj )
    return FALSE;

  // Verify object is worn by cy
  if( !OBJ_WORN( obj ) || !obj->loc.wearing )
    return FALSE;

  if( !ch )
    ch = obj->loc.wearing;

  if( ch != obj->loc.wearing )
    return FALSE;

  for( locwearing = 0;locwearing < MAX_WEAR; locwearing++ )
  {
    if( ch->equipment[locwearing] == obj )
      break;
  }
  // obj is not worn !  This must be a bug if true.
  if( locwearing == MAX_WEAR )
    return FALSE;

  switch( locwearing )
  {
    // For his quiver first
    case WEAR_QUIVER:
      // Heal if down more than 10 hps
      if( (GET_HIT(ch) < GET_MAX_HIT(ch) - 10) && (cmd == CMD_PERIODIC) )
      {
        spell_full_heal( 60, ch, 0, 0, ch, 0);
        return TRUE;
      }
    break;
    case WIELD:
    case WIELD2:
    case WIELD3:
    case WIELD4:
      // RAWR!  On a 4, 5 or 6 proc bigbys hand!
      if( (cmd == CMD_MELEE_HIT) && ch->specials.fighting && (dice(1, 6) > 3) )
      {
        spell_bigbys_crushing_hand(60, ch, NULL, SPELL_TYPE_SPELL, ch->specials.fighting, 0);
        return TRUE;
      }
    break;
    case WEAR_WAIST:
      if( (cmd == CMD_PERIODIC) && !has_skin_spell(ch) && (dice(1, 6) > 4) )
        spell_biofeedback(60, ch, 0, 0, ch, 0);
    break;
  }

  return FALSE;
}

/*
// It's a percentage chance to make them attack a few extra times.
// It's size dependdent: < medium = 10, medium/large = 6, > large = 4
void dagger_of_wind( P_obj obj, P_char ch, int cmd, char *argument )
{
   int numhits = 0;
   int i = 0;

   // Verify that obj is dagger of wind and being wielded by ch.
   if( cmd != CMD_MELEE_HIT || !ch || !obj || !OBJ_WORN(obj) || obj->loc.wearing != ch )
      return;
   // Verify that ch is in battle with someone.
   if( !IS_FIGHTING(ch) || !ch->specials.fighting )
      return;

   // 50% chance to proc.
   if( number(1,100) > 50 )
   {
       act("You move with a blur of speed!",
          FALSE, ch, obj, 0, TO_CHAR);
       act("$n moves with a blur of speed!",
          FALSE, ch, obj, 0, TO_ROOM);

      // Calculate number of hits based on size.
      if( GET_SIZE(ch) < SIZE_MEDIUM )
         numhits = 10;
      else if( GET_SIZE(ch) == SIZE_MEDIUM || GET_SIZE(ch) == SIZE_LARGE )
         numhits = 6;
      else
         numhits = 4;

      while( i < numhits )
      {
         // Stop hitting if no one to hit.
         if( !ch->specials.fighting )
            break;
         hit(ch, ch->specials.fighting, obj );
         i++;
      }
   }
}
*/

int sphinx_prefect_crown( P_obj obj, P_char ch, int cmd, char *arg )
{
   int curr_time;
   char first_arg256;

   if( cmd == CMD_SET_PERIODIC )
      return TRUE;
   if( !OBJ_WORN_POS( obj, WEAR_HEAD ) )
      return FALSE;

   curr_time = time(NULL);

   if( arg && (cmd == CMD_SAY) && isname(arg, "sphinx"))
   {

      // Set timer here: 5 min = 60 * 5 sec = 300
      // Note: This will be replaced by get_property("timer.proc.crownXXX", ??? )
      // Note: 300 is a magic number here and below.
      if( curr_time >= obj->timer[0] + 300 )
      {
         act("You say 'sphinx'", FALSE, ch, 0, 0, TO_CHAR);
         act("$n says 'sphinx'", TRUE, ch, obj, NULL, TO_ROOM);
         act("&+LYour crown seems to &+Yp&+yu&+Yl&+ys&+Ye &+Lwith a vibrant &+Ymagic&+L!&n", FALSE, ch, obj, obj, TO_CHAR);
         act("&+LA misty haze of &+yun&+Learthly &+Cknowledge &+Lflows from your crown!&n", FALSE, ch, obj, obj, TO_CHAR);
         act("&+mS&+Mw&+mi&+Mr&+ml&+Mi&+mn&+Mg &+Lthoughts and words of &+Bmagic &+Lseem to &+rsear &+Linto your mind as the wisdom of the &+Ys&+yp&+Yh&+yi&+Yn&+yx &+Cinvigorates &+Lyou with &+Gpower&+L!&n", FALSE, ch, obj, obj, TO_CHAR); 
         act("$n's &+Lcrown glows with a &+Gv&+gi&+Gb&+gr&+Ga&+gn&+Gt &+Ylight&+L!&n", TRUE, ch, obj, NULL, TO_ROOM);
         act("&+LThe image of a wise &+Ys&+yp&+Yh&+yi&+Yn&+yx &+Lseems to &+Cshimmer &+Laround $n's &+Lhead!&n", TRUE, ch, obj, NULL, TO_ROOM);
         act("&+LIn a rush of displaced &+Cair&+L, the image of the &+Ys&+yp&+Yh&+yi&+Yn&+yx &+Levaporates into a misty essence infused with &+mmagic&+L!&n", TRUE, ch, obj, NULL, TO_ROOM);
         act("&+LThe misty essence of the &+Ys&+yp&+Yh&+yi&+Yn&+yx &+Lenvelopes $n!", TRUE, ch, obj, NULL, TO_ROOM);
         act("&+LA wicked &+rgrin &+Lpasses across $n's &+Lface as they are &+me&+Mm&+mp&+Mo&+mw&+Me&+mr&+Me&+md &+Lwith &+rancient &+gwisdom &+Land &+Cknowledge&+L!&n", TRUE, ch, obj, NULL, TO_ROOM);
         spell_mordenkainens_lucubration(60, ch, 0, 0, ch, NULL);
         obj->timer[0] = curr_time;
         obj->timer[1] = 0;														      return FALSE;
         return TRUE;
      }
   }

   // Send a message (once) to the char when the crown becomes usable again.
   if(cmd == CMD_PERIODIC && obj->timer[0] + 300 <= curr_time && obj->timer[1] == 0 )
   {
      act("&+LYour legendary &+ycrown &+Lof the &+Ys&+yp&+Yh&+yi&+Yn&+yx &+Gp&+gr&+Ge&+gf&+Ge&+gc&+Gt&+gs &+Cglows &+Lwith an unearthly &+Wlight &+Land starts to &+mpulse &+Lgently in time to your &+rheartbeat&+L.&n",
         TRUE, obj->loc.wearing, obj, 0, TO_CHAR);
      obj->timer[1] = 1;														      return FALSE;
   }
}

int adjacent_room_nesw(P_char ch, int num_rooms )
{
   int dir = number( 0, 3 );
   int i, j;
   room_direction_data *exit;

   if( !ch )
      return -1;

   for( i = 0; i < 3; i++ )
   {
      exit = EXIT( ch, (dir+i)%4 );
      if( exit && exit->to_room )
         // Move num_rooms away (if possible).
         for ( j = 1; j < num_rooms;j++ )
            if( world[exit->to_room].dir_option[(dir+i)%4] )
               exit = world[exit->to_room].dir_option[(dir+i)%4];
      if( exit && exit->to_room )
         return exit->to_room;
   }

   // If no exit found, return -1;
   return -1;
}

// This is a proc for the Leviathan mob.
int leviathan( P_char ch, P_char pl, int cmd, char *arg )
{
   P_char tch;
   int to_room;
   P_ship ship;
   int ram_damage, heading;

   if( cmd == CMD_SET_PERIODIC )
      return TRUE;

   if( !ch )
      return TRUE;

   // Attack nearby ships (within 3 rooms ) if not fighting
   if( cmd == CMD_PERIODIC && !number( 0, 1 ) && !IS_FIGHTING(ch) )
      if( ship = leviathan_find_ship( ch, ch->in_room, 3 ) )
      {
          act_to_all_in_ship( ship,"$N speeds into your ship!", ch );
          act( "$N speeds into a ship!", FALSE, ch, NULL, ch, TO_ROOM );
          ram_damage = number( 10, 15 );
          heading = number( 0, 3 );
          ch_damage_hull( ch, ship, ram_damage, heading, number( 0, 99 ) );
          update_ship_status( ship );
          return TRUE;
      }

   if( cmd == CMD_PERIODIC && !number( 0, 1 ) )
   {
      switch( number( 1, 2 ) )
      {
      case 1:
         act( "$N lifts up out of the water, then splashes back down, causing a massive wave!", FALSE, ch, NULL, ch, TO_ROOM );

         // To each char in room, chance of knockdown.
         for( tch = world[ch->in_room].people;tch;tch = tch->next_in_room )
         {
            if( tch != ch )
            {
               if( number( 0, 1 ) )
                  SET_POS( tch, POS_SITTING + GET_STAT(tch));
               // if not knocked down, chance to get moved 1 room away.
               else if( number( 0, 1 ) && (to_room = adjacent_room_nesw(ch, 1)) )
               {
                  // Move char 1 room
                  char_from_room(tch);
                  char_to_room(tch, to_room, -1);
               }
            }
         }
      break;
      case 2:
         tch = ch->specials.fighting;
         if( tch )
         {
            act( "$N lashes out with a tentacle, wrapping it around you, lifts and quickly slams you upon the water surface!", FALSE, tch, NULL, ch, TO_CHAR );
            act( "$N lashes out grabbing $n with a tentacle, thrashing $m into the water!", FALSE, tch, NULL, ch, TO_ROOM );
            // Move victim 1-3 rooms away
            if( to_room = adjacent_room_nesw(ch, number( 1, 3 )) )
            {
               char_from_room(tch);
               char_to_room(tch, to_room, -1);
            }
            stop_fighting( tch );
            // Stun for 3-5 sec
            CharWait( tch, number( 3, 5 ) );
         }
      default:
      break;
      }
   }

   return FALSE;
}

// Returns a ship if it's near Leviathan
P_ship leviathan_find_ship( P_char leviathan, int room, int num_rooms )
{
   int i;
   P_obj obj;
   P_ship ship;
   room_direction_data *exit;
   char msg[100];

   if( !leviathan )
      return NULL;

   // Look through contents
   for(obj = world[room].contents;obj;obj = obj->next_content )
      // If found a ship && percent >= 50
      if( obj && (GET_ITEM_TYPE(obj) == ITEM_SHIP) && (obj->value[6] == 1) && number( 0, 1) )
         return shipObjHash.find(obj);

   // This is a bit repetative, but that's ok, it's for a small number.
   if( num_rooms > 0 )
   {
      // We only check exits N, S, E and W.
      for(i = 0; i < 4; i++ )
      {
         exit = world[room].dir_option[i];
         if( exit && exit->to_room )
         {
            ship = leviathan_find_ship( leviathan, exit->to_room, num_rooms - 1 );
            if( ship )
               return ship;
         }
      }
   }
   return NULL;
}

// Dependent on ch's str and weight.  Pretty simple atm.
int siege_move_wait( P_char ch )
{
  // Base: 100 str, 100 weight, 2 secs
  int retval = 100*100*2*WAIT_SEC;

  retval /= GET_C_STR( ch );
  retval /= ch->player.weight;

  return retval;
}

// Based on chars str and agi right now.
int siege_load_wait( P_char ch, P_obj engine )
{
  // Base: 100 str, 100 agi, 2 secs
  int retval = 100*100*10*WAIT_SEC;

  retval /= GET_C_STR( ch );
  retval /= GET_C_AGI( ch );

  return retval;
}

void event_load_engine(P_char ch, P_char victim, P_obj obj, void *data)
{
  act( "You finish loading $p.", FALSE, ch, obj, NULL, TO_CHAR );
  act( "$n finishes loading $p.", TRUE, ch, obj, 0, TO_ROOM);
  obj->value[3]--;
  obj->value[2] = 1;
}

// Having to write this is a pain! Ok.. so not so bad.
// Char can move if exit is unblocked and ch not fighting.
bool can_move( P_char ch, int dir )
{
  if( IS_FIGHTING(ch) )
    return FALSE;

  if( !EXIT( ch, dir ) )
    return FALSE;
  if( !can_enter_room(ch, EXIT(ch, dir)->to_room, FALSE) )
    return FALSE;

  if( IS_CLOSED( ch->in_room, dir ) )
    return FALSE;
  if( IS_HIDDEN( ch->in_room, dir ) )
    return FALSE;
  if( IS_WALLED( ch->in_room, dir ) )
    return FALSE;

  return TRUE;
}

// This proc is for a ballista(OBJ # 461).
int ballista( P_obj obj, P_char ch, int cmd, char *arg )
{
  char   arg1[MAX_STRING_LENGTH];
  char   arg2[MAX_STRING_LENGTH];
  char   arg3[MAX_STRING_LENGTH];
  char   buf[MAX_STRING_LENGTH];
  int    num_rooms;
  int    in_room, ch_room;
  P_obj  ammo;
  int    dir;
  P_char vict;
  P_obj  target;

  if( cmd == CMD_SET_PERIODIC )
    return TRUE;
  if( cmd == CMD_PUSH )
  {
    // Parse argument.
    argument_interpreter(arg, arg1, arg2);
    if( get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents) == obj )
    {
      dir = -1;
      // Figure out what direction.
      if( is_abbrev(arg2, "north") )
        dir = NORTH;
      else if( is_abbrev(arg2, "south") )
        dir = SOUTH;
      else if( is_abbrev(arg2, "east") )
        dir = EAST;
      else if( is_abbrev(arg2, "west") )
        dir = WEST;
      if( dir == -1 || *arg2 == '\0' )
      {
        send_to_char( "Move the ballista what direction?\n", ch );
        return TRUE;
      }
      else
      {
        // If there is no exit in that direction, or it's blocked.
        if( !can_move( ch, dir ) )
        {
          send_to_char( "You can't leave that way right now.\n", ch );
          return TRUE;
        }
        // Move it that direction.
        sprintf( buf, "You begin to push $p %sward.", dirs[dir] );
        act( buf, FALSE, ch, obj, NULL, TO_CHAR );
        sprintf( buf, "$n starts pushing $p %sward.", dirs[dir] );
        act( buf, TRUE, ch, obj, 0, TO_ROOM);
        // Yes, this lags you to stop you from spamming.
        CharWait(ch, siege_move_wait(ch) );
        add_event(event_move_engine, siege_move_wait(ch), ch, NULL, 
          obj, 0, &dir, sizeof(int) );
      }
      return TRUE;
    }
  }
  else if( cmd == CMD_RELOAD )
  {
    argument_interpreter(arg, arg1, arg2);
    if( get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents) != obj ) 
      return FALSE;
    if( obj->value[2] > 0 )
      act( "$p is already loaded.", FALSE, ch, obj, NULL, TO_CHAR );
    else if( obj->value[3] <= 0 )
      act( "$p has no more ammo.", FALSE, ch, obj, NULL, TO_CHAR );
    else if( is_loading( obj ) )
      act( "$p is already being reloaded.", FALSE, ch, obj, NULL, TO_CHAR );
    else
    {
      act( "You begin loading $p.", FALSE, ch, obj, NULL, TO_CHAR );
      act( "$n begins loading $p.", FALSE, ch, obj, NULL, TO_ROOM );
      add_event(event_load_engine, siege_load_wait(ch, obj), ch, NULL, 
        obj, 0, NULL, 0 );
      // Yes, this lags you while loading.
      CharWait(ch, siege_move_wait(ch) );
    }
    return TRUE;
  }
  else if( cmd == CMD_FIRE )
  {
    // Parse argument.
    half_chop(arg, arg1, arg3);
    half_chop(arg3, arg2, arg3);

    if( get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents) == obj )
    {
      dir = -1;

      if( obj->value[2] == 0 )
      {
        act( "$p has no ammo.  Try to reload it first.", FALSE, ch, obj, NULL, TO_CHAR );
        return TRUE;
      }

      // Figure out what direction.
      if( is_abbrev(arg3, "north") )
        dir = NORTH;
      else if( is_abbrev(arg3, "south") )
        dir = SOUTH;
      else if( is_abbrev(arg3, "east") )
        dir = EAST;
      else if( is_abbrev(arg3, "west") )
        dir = WEST;
      if( dir == -1 || *arg3 == '\0' )
      {
        send_to_char( "Fire the ballista at who what direction?\n", ch );
        return TRUE;
      }
      else
      {
        sprintf( buf, "You fire $p %sward.", dirs[dir] );
        act( buf, FALSE, ch, obj, NULL, TO_CHAR );
        sprintf( buf, "$n fires $p %sward.", dirs[dir] );
        act( buf, TRUE, ch, obj, 0, TO_ROOM);

        if( obj->loc_p != LOC_ROOM )
        {
          logit(LOG_DEBUG, "ballista: firing siege weapon not in a room.");
          return FALSE;
        }
        in_room = obj->loc.room;
        if( !in_room )
        {
          logit(LOG_DEBUG, "ballista: firing siege weapon in room 0.");
          return FALSE;
        }
        // Load exploded ammo into the room.
        ammo = read_object(real_object(obj->value[0]), REAL);
        if( !ammo )
        {
          logit(LOG_DEBUG, "ballista: couldn't load ammo.");
          return FALSE;
        }
        obj->value[2] = 0;
        obj_to_room( ammo, in_room );
        vict = NULL;
        target = NULL;
        // Fire the weapon 3x spaces to the dir. Hits walls.
        for( num_rooms = 0;num_rooms<3;num_rooms++)
        {
          ch_room = ch->in_room;
          ch->in_room = in_room;
          // Impale the target!
          if( num_rooms > 0 && !vict && (vict = get_char_room_vis(ch, arg2)) )
          {
            act( "$p impales $n!", TRUE, vict, ammo, 0, TO_ROOM );
            act( "$p impales YOU!", TRUE, vict, ammo, 0, TO_CHAR );
            char_from_room( vict );
          }
          // If we hit target siege object.
          if( num_rooms > 0 && !vict && (target = get_siege_room( ch, arg2)) )
          {
            sprintf( buf, "%s slams into $p", ammo->short_description );
            act( buf, TRUE, NULL, target, 0, TO_ROOM );
            act( buf, TRUE, ch, target, 0, TO_CHAR );
            damage_siege( target, ammo );
            ch->in_room = ch_room;
            return TRUE;
          }
          ch->in_room = ch_room;
          // If we hit a wall.
          if(  !VIRTUAL_EXIT(in_room, dir)
            || !VIRTUAL_EXIT(in_room, dir)->to_room )
          {
            sprintf( buf, "$p hits the %s wall.", dirs[dir] );
            act( buf, TRUE, NULL, ammo, 0, TO_ROOM );
            break;
          }
          else
          {
            in_room = VIRTUAL_EXIT(in_room, dir)->to_room;
            obj_from_room(ammo);
            obj_to_room(ammo, in_room);
            act( "$p flies through the room..", TRUE, NULL, ammo, 0, TO_ROOM );
          }
        }
        ch_room = ch->in_room;
        ch->in_room = ammo->loc.room;
        // Missile go SMACK!
        // If victim was impaled...
        if( vict )
        {
          char_to_room( vict, ammo->loc.room, -1 );
          act( "$p slams into the ground with $n impaled upon it.", TRUE, vict, ammo, NULL, TO_ROOM );
          act( "You hit the ground and bounce off of $p.", TRUE, vict, ammo, NULL, TO_CHAR );
        }
        else if( target = get_siege_room( ch, arg2) )
        {
          sprintf( buf, "%s slams into $p", ammo->short_description );
          act( buf, TRUE, NULL, target, 0, TO_ROOM );
          act( buf, TRUE, ch, target, 0, TO_CHAR );
          damage_siege( target, ammo );
        }
        // If victim in final room...
        else if( (vict = get_char_room_vis(ch, arg2)) != NULL )
        {
          act( "$p slams into $n before hitting the dirt.", TRUE, vict, ammo, NULL, TO_ROOM );
          act( "$p slams into you before hitting the dirt.", TRUE, vict, ammo, NULL, TO_CHAR );
        }
        // Miss!
        else
          act( "$p slams into the ground.", TRUE, NULL, ammo, NULL, TO_ROOM );
        if( vict )
          damage(ch, vict, dice( ammo->value[0], ammo->value[1] * 3 ), TYPE_UNDEFINED );
        ch->in_room = ch_room;
        return TRUE;
      }
    }
  }
  return FALSE;
}

// This proc is for a battering ram(OBJ # 462).
int battering_ram( P_obj obj, P_char ch, int cmd, char *arg )
{
  char  arg1[MAX_STRING_LENGTH];
  char  arg2[MAX_STRING_LENGTH];
  char  buf[MAX_STRING_LENGTH];
  int   dir;
  P_obj target;

  if( cmd == CMD_SET_PERIODIC )
    return TRUE;
  if( cmd == CMD_PUSH )
  {
    // Parse argument.
    argument_interpreter(arg, arg1, arg2);
    if( get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents) == obj )
    {
      dir = -1;
      // Figure out what direction.
      if( is_abbrev(arg2, "north") )
        dir = NORTH;
      else if( is_abbrev(arg2, "south") )
        dir = SOUTH;
      else if( is_abbrev(arg2, "east") )
        dir = EAST;
      else if( is_abbrev(arg2, "west") )
        dir = WEST;
      if( dir == -1 || *arg2 == '\0' )
      {
        send_to_char( "Move the battering ram what direction?\n", ch );
        return TRUE;
      }
      else
      {
        // If there is no exit in that direction, or it's blocked.
        if( !can_move( ch, dir ) )
        {
          send_to_char( "You can't leave that way right now.\n", ch );
          return TRUE;
        }
        // Move it that direction.
        sprintf( buf, "You begin to push $p %sward.", dirs[dir] );
        act( buf, FALSE, ch, obj, NULL, TO_CHAR );
        sprintf( buf, "$n starts pushing $p %sward.", dirs[dir] );
        act( buf, TRUE, ch, obj, 0, TO_ROOM);
        // Yes, this lags you to stop you from spamming.
        CharWait(ch, siege_move_wait(ch) );
        add_event(event_move_engine, siege_move_wait(ch), ch, NULL, 
          obj, 0, &dir, sizeof(int) );
      }
      return TRUE;
    }
  }
  else if( cmd == CMD_THRUST )
  {
    argument_interpreter(arg, arg1, arg2);
    if( get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents) == obj )
    {
      if( (target = get_siege_room( ch, arg2)) != NULL )
      {
        if( obj == target )
        {
          act( "You push $p around in circles.", FALSE, ch, target, NULL, TO_CHAR );
          act( "$n pushes $p around in circles.", FALSE, ch, target, NULL, TO_ROOM );
          return TRUE;
        }

        // Yes, this lags you to stop you from spamming.
        CharWait(ch, siege_move_wait(ch) / 2 );

        sprintf( buf, "You thrust %s at $p!", obj->short_description );
        act( buf, FALSE, ch, target, NULL, TO_CHAR );
        sprintf( buf, "$n thrusts %s at $p!", obj->short_description );
        act( buf, FALSE, ch, target, NULL, TO_ROOM );

        damage_siege( target, obj );
      }
      else
        act( "Thrust $p at what?", FALSE, ch, obj, NULL, TO_CHAR );
      return TRUE;
    }
  }
  return FALSE;
}

// This proc is for a catapult(OBJ # 463).
int catapult( P_obj obj, P_char ch, int cmd, char *arg )
{
  char  arg1[MAX_STRING_LENGTH];
  char  arg2[MAX_STRING_LENGTH];
  char  buf[MAX_STRING_LENGTH];
  int   dir;
  int   num_rooms;
  int   in_room;
  P_obj ammo;

  if( cmd == CMD_SET_PERIODIC )
    return TRUE;
  if( cmd == CMD_PUSH )
  {
    // Parse argument.
    argument_interpreter(arg, arg1, arg2);
    if( get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents) == obj )
    {
      dir = -1;
      // Figure out what direction.
      if( is_abbrev(arg2, "north") )
        dir = NORTH;
      else if( is_abbrev(arg2, "south") )
        dir = SOUTH;
      else if( is_abbrev(arg2, "east") )
        dir = EAST;
      else if( is_abbrev(arg2, "west") )
        dir = WEST;
      if( dir == -1 || *arg2 == '\0' )
      {
        send_to_char( "Move the catapult what direction?\n", ch );
        return TRUE;
      }
      else
      {
        // If there is no exit in that direction, or it's blocked.
        if( !can_move( ch, dir ) )
        {
          send_to_char( "You can't leave that way right now.\n", ch );
          return TRUE;
        }
        // Move it that direction.
        sprintf( buf, "You begin to push $p %sward.", dirs[dir] );
        act( buf, FALSE, ch, obj, NULL, TO_CHAR );
        sprintf( buf, "$n starts pushing $p %sward.", dirs[dir] );
        act( buf, TRUE, ch, obj, 0, TO_ROOM);
        // Yes, this lags you to stop you from spamming.
        CharWait(ch, siege_move_wait(ch) );
        add_event(event_move_engine, siege_move_wait(ch), ch, NULL, 
          obj, 0, &dir, sizeof(int) );
      }
      return TRUE;
    }
  }
  else if( cmd == CMD_RELOAD )
  {
    argument_interpreter(arg, arg1, arg2);
    if( get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents) != obj )
      return FALSE;
    if( obj->value[2] > 0 )
      act( "$p is already loaded.", FALSE, ch, obj, NULL, TO_CHAR );
    else if( obj->value[3] <= 0 )
      act( "$p has no more ammo.", FALSE, ch, obj, NULL, TO_CHAR );
    else if( is_loading( obj ) )
      act( "$p is already being reloaded.", FALSE, ch, obj, NULL, TO_CHAR );
    else
    {
      act( "You begin loading $p.", FALSE, ch, obj, NULL, TO_CHAR );
      act( "$n begins loading $p.", FALSE, ch, obj, NULL, TO_ROOM );
      add_event(event_load_engine, siege_load_wait(ch, obj), ch, NULL, 
        obj, 0, NULL, 0 );
      // Yes, this lags you while loading.
      CharWait(ch, siege_move_wait(ch) );
    }
    return TRUE;
  }
  else if( cmd == CMD_FIRE )
  {
    // Parse argument.
    argument_interpreter(arg, arg1, arg2);
    if( get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents) == obj )
    {
      dir = -1;

      if( obj->value[2] == 0 )
      {
        act( "$p has no ammo.  Try to reload it first.", FALSE, ch, obj, NULL, TO_CHAR );
        return TRUE;
      }

      // Figure out what direction.
      if( is_abbrev(arg2, "north") )
        dir = NORTH;
      else if( is_abbrev(arg2, "south") )
        dir = SOUTH;
      else if( is_abbrev(arg2, "east") )
        dir = EAST;
      else if( is_abbrev(arg2, "west") )
        dir = WEST;
      if( dir == -1 || *arg2 == '\0' )
      {
        send_to_char( "Fire the catapult what direction?\n", ch );
        return TRUE;
      }
      else
      {
        sprintf( buf, "You fire $p %sward.", dirs[dir] );
        act( buf, FALSE, ch, obj, NULL, TO_CHAR );
        sprintf( buf, "$n fires $p %sward.", dirs[dir] );
        act( buf, TRUE, ch, obj, 0, TO_ROOM);

        if( obj->loc_p != LOC_ROOM )
        {
          logit(LOG_DEBUG, "catapult: firing siege weapon not in a room.");
          return FALSE;
        }
        in_room = obj->loc.room;
        if( !in_room )
        {
          logit(LOG_DEBUG, "catapult: firing siege weapon in room 0.");
          return FALSE;
        }
        // Load exploded ammo into the room.
        ammo = read_object(real_object(obj->value[0]), REAL);
        obj->value[2] = 0;
        obj_to_room( ammo, in_room );
        // Fire the weapon 4x spaces to the dir. Hits walls.
        for( num_rooms = 0;num_rooms<4;num_rooms++)
        {
          // If we hit a wall.
          if(  !VIRTUAL_EXIT(in_room, dir)
            || !VIRTUAL_EXIT(in_room, dir)->to_room )
          {
            sprintf( buf, "$p slams into the %s wall.", dirs[dir] );
            act( buf, TRUE, NULL, ammo, 0, TO_ROOM );
            return TRUE;
          }
          else
          {
            in_room = VIRTUAL_EXIT(in_room, dir)->to_room;
            obj_from_room(ammo);
            obj_to_room(ammo, in_room);
            act( "$p flies overhead..", TRUE, NULL, ammo, 0, TO_ROOM );
          }
        }
        // Ammo lands in to_room.. BOOM, SPLAT!
        act( "$p explodes overhead!", TRUE, NULL, ammo, 0, TO_ROOM );
        explode_ammo( ch, ammo );
        return TRUE;
      }
    }
  }
  return FALSE;
}

// Attempt to move the siege engine a direction.
void event_move_engine(P_char ch, P_char victim, P_obj obj, void *data)
{
  char buf[MAX_STRING_LENGTH];
  int dir = *((int *)data);
  int to_room;

  // If there's an exit and it's unblocked (and ch isn't fighting).
  if( can_move( ch, dir ) )
  {
    sprintf( buf, "You push $p %sward.", dirs[dir] );
    act( buf, FALSE, ch, obj, NULL, TO_CHAR );
    sprintf( buf, "$n pushes $p %sward.", dirs[dir] );
    act( buf, TRUE, ch, obj, 0, TO_ROOM);

    to_room = EXIT(ch, dir)->to_room;
    obj_from_room(obj);
    obj_to_room(obj, to_room);

    char_from_room( ch );
    char_to_room( ch, to_room, dir );

    sprintf( buf, "$n pushes $p in from the %s.", dirs[rev_dir[dir]] );
    act( buf, TRUE, ch, obj, 0, TO_ROOM);
  }
  else
    send_to_char( "You can't leave that way right now.\n", ch );

}

// This will probably want an update with ammo types.
void explode_ammo( P_char ch, P_obj ammo )
{
  P_char vict, next_vict;
  P_obj obj, next_obj;

  // Splat ppl!
  for( vict = world[ammo->loc.room].people;vict;vict = next_vict )
  {
    next_vict = vict->next_in_room;
    damage(ch, vict, dice( ammo->value[0], ammo->value[1] * 3 ), TYPE_UNDEFINED );
  }

  // Splat other siege weapons.
  for( obj = world[ammo->loc.room].contents;obj;obj = next_obj )
  {
    next_obj = obj->next_content;
    if( is_siege( obj ) )
      damage_siege( obj, ammo );
  }
}

// Returns TRUE iff ch is blocked by gates.
bool check_gates( P_char ch, int room )
{
  P_obj wall;
  int dir;

  for( dir = 0; dir < NUM_EXITS; dir++ )
  {
    if( world[ch->in_room].dir_option[dir]
     && world[ch->in_room].dir_option[dir]->to_room == room )
      break;
  }
  // If no exit to room, no gate can block it.
  if( dir == NUM_EXITS )
    return FALSE;

  wall = world[ch->in_room].contents;
  while( wall )
  {
    if( obj_index[wall->R_num].func.obj == castlewall )
      break;
    wall = wall->next_content;
  }
  if( !wall )
    return FALSE;

  // The direction the wall blocks is the dir ch wants to go.
  if( wall->value[0] == dir )
    return TRUE;
  return FALSE;
}

// This is the proc for castle walls.  Just a placeholder atm.
int castlewall( P_obj obj, P_char ch, int cmd, char *arg )
{
  if( cmd == CMD_SET_PERIODIC )
    return FALSE;

  if( !ch || !ch->in_room )
    return FALSE;
  if( !obj )
  {
    logit(LOG_DEBUG, "castlewall: proc called with no wall!");
    return FALSE;
  }

  // Is ch an invader of the town?
  if( !IS_INVADER( ch ) )
    return FALSE;

  // Value0 of a castlewall is the direction it blocks.
  if( (obj->value[0] == EAST  && cmd == CMD_EAST) 
   || (obj->value[0] == WEST  && cmd == CMD_WEST) 
   || (obj->value[0] == NORTH && cmd == CMD_NORTH) 
   || (obj->value[0] == SOUTH && cmd == CMD_SOUTH) )
  {
    if( isname( "gates", obj->name ) )
      act( "$p block your path.", FALSE, ch, obj, NULL, TO_CHAR );
    else
      act( "$p blocks your path.", FALSE, ch, obj, NULL, TO_CHAR );
    return TRUE;
  }

  return FALSE;
}

// Returns TRUE iff weapon is a siege object.
bool is_siege( P_obj object )
{
  if( !object )
    return FALSE;

  // If the object proc is ballista/battering_ram/catapult..
  if(  obj_index[object->R_num].func.obj == ballista
    || obj_index[object->R_num].func.obj == battering_ram
    || obj_index[object->R_num].func.obj == catapult
    || obj_index[object->R_num].func.obj == castlewall )
    return TRUE;

  return FALSE;
}

void damage_siege( P_obj siege, P_obj ammo )
{
  char  buf[MAX_STRING_LENGTH];
  bool  destroy = FALSE;
  int   damage;
  P_obj scraps;

  damage = dice( ammo->value[0], ammo->value[1] );

  switch( siege->material )
  {
  default:
  break;
  case MAT_NONSUBSTANTIAL:
  case MAT_LIQUID:
  case MAT_WAX:
    damage *= 10;
  break;
  case MAT_PAPER:
  case MAT_PARCHMENT:
  case MAT_LEAVES:
  case MAT_FLESH:
  case MAT_RUBBER:
  case MAT_FEATHER:
    damage *= 5;
  break;
  case MAT_CLOTH:
    damage = ( damage * 10 ) / 3;
  break;
  case MAT_BAMBOO:
  case MAT_REEDS:
  case MAT_HEMP:
  case MAT_EGGSHELL:
  case MAT_BARK:
  case MAT_GENERICFOOD:
    damage = ( damage * 5 ) / 2;
  break;
  case MAT_SOFTWOOD:
  case MAT_HARDWOOD:
  case MAT_PEARL:
    damage = ( damage * 9 ) / 4;
  break;
  case MAT_SILICON:
  case MAT_CERAMIC:
    damage = ( damage * 5 ) / 3;
  break;
  case MAT_HIDE:
  case MAT_LEATHER:
  case MAT_CURED_LEATHER:
    damage = ( damage * 7 ) / 3;
  break;
  case MAT_CRYSTAL:
  case MAT_BONE:
    damage = ( damage * 7 ) / 3;
  break;
  case MAT_GEM:
  case MAT_STONE:
  case MAT_GRANITE:
  case MAT_MARBLE:
  case MAT_LIMESTONE:
    damage = ( damage * 85 ) / 100;
  break;
  case MAT_IRON:
  case MAT_STEEL:
    damage = ( damage * 75 ) / 100;
  break;
  case MAT_BRASS:
  case MAT_MITHRIL:
    damage = ( damage * 70 ) / 100;
  break;
  case MAT_GLASSTEEL:
  case MAT_ADAMANTIUM:
    damage = ( damage * 65 ) / 100;
  break;
  case MAT_BRONZE:
  case MAT_COPPER:
  case MAT_SILVER:
  case MAT_ELECTRUM:
  case MAT_GOLD:
  case MAT_PLATINUM:
  case MAT_RUBY:
  case MAT_EMERALD:
  case MAT_SAPPHIRE:
  case MAT_IVORY:
    damage = ( damage * 89 ) / 100;
  break;
  case MAT_DRAGONSCALE:
  case MAT_DIAMOND:
    damage = ( damage * 67 ) / 100;
  break;
  case MAT_OBSIDIAN:
    damage = ( damage * 63 ) / 100;
  break;
  case MAT_CHITINOUS:
  case MAT_REPTILESCALE:
    damage = ( damage * 95 ) / 100;
  break;
  }

  // This should never happen, but just to make sure we don't heal target...
  if( damage < 1 )
    damage = 1;

  siege->condition -= damage;
  if( siege->condition <= 0 )
    destroy = TRUE;

  sprintf(buf, "$q %s", destroy ? "is completely destroyed!" :
                                  "is damaged from the blow!" );
  act(buf, TRUE, NULL, siege, 0, TO_ROOM);

  if( destroy )
  {
    act("$p collapses into scraps.", TRUE, NULL, siege, 0, TO_ROOM);
    scraps = read_object(9, VIRTUAL);
    if( !scraps )
    {
      extract_obj( siege, TRUE );
      return;
    }
    sprintf(buf, "Scraps from %s&n lie in a pile here.",
      siege->short_description);
    scraps->description = str_dup(buf);
    sprintf(buf, "a pile of scraps from %s", siege->short_description);
    scraps->short_description = str_dup(buf);
    sprintf(buf, "%s scraps pile", siege->name);
    scraps->name = str_dup(buf);
    scraps->str_mask = STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_KEYS;
    set_obj_affected(scraps, 400, TAG_OBJ_DECAY, 0);
    obj_to_room(scraps, siege->loc.room);

    extract_obj( siege, TRUE );
  }
}

// Find a siege object in ch's room.
P_obj get_siege_room( P_char ch, char *arg )
{
  P_obj obj;
  char *temp = arg;
  int   howmany;
  int   count = 0;

  // Need to handle 2.siege.
  howmany = get_number( &temp );

  for( obj = world[ch->in_room].contents; obj; obj = obj->next_content )
  {
    // If it is a siege weapon, and matches name, and matches amount..
    if( is_siege( obj ) && isname( temp, obj->name ) && howmany == ++count )
        return obj;
  }

  return NULL;
}

// See if someone already started a reload event.
bool is_loading( P_obj siege )
{
  for( P_nevent e = siege->nevents; e; e = e->next )
    if( e->func == event_load_engine )
      return TRUE;
  return FALSE;
}

// Returns the corresponding town to the zone (or null if none).
P_town gettown( P_char ch )
{
  P_town town = towns;
  while( town && town->zone != &(zone_table[world[ch->in_room].zone]) )
    town = town->next_town;
  return town;
}

// This is a proc for the warmaster mobs in towns that build
//   walls etc.
int warmaster( P_char ch, P_char pl, int cmd, char *arg )
{
  P_town town;
  char buf[MAX_STRING_LENGTH], arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  char *rest;
  P_obj donation, siege;
  int ztop, count, i, j, numgate;

  if( cmd == CMD_SET_PERIODIC )
    return FALSE;

  if( cmd == CMD_PERIODIC )
    return FALSE;

  if( !ch )
  {
    logit(LOG_DEBUG, "warmaster: called with no warmaster.");
    return FALSE;
  }

  if( !pl )
  {
    logit(LOG_DEBUG, "warmaster: called with no player.");
    return FALSE;
  }
  // In case someone tries to have a pet do warmaster stuff.
  if( IS_NPC( pl ) )
    return FALSE;

  if(cmd == CMD_LIST)
  {
    town = gettown( ch );
    if( !town )
    {
      logit(LOG_DEBUG, "warmaster: called outside of town.");
      return FALSE;
    }

    // List town's name, offense, defense, and resrources..
    sprintf( buf, "'%s'\nOffense:     %7d\nDefense:     %7d\nResources:   %7d\n"
                  "Town Guards:  %s\nTown Cavalry: %s\nTown Portals: %s\n\n",
                  town->zone->name, town->offense, town->defense, town->resources, 
                  town->deploy_guard ? "  Deployed" : "Not Deployed",
                  town->deploy_cavalry ? "  Deployed" : "Not Deployed",
                  town->deploy_portals ? "  Deployed" : "Not Deployed" );
    send_to_char( buf, pl );

    if( IS_TRUSTED( pl ) )
    {
      sprintf( buf, "Guards: %d deployed of %d max, vnum %d, load in room %d.\n", 
        mob_index[real_mobile(town->guard_vnum)].number, town->guard_max, town->guard_vnum, 
        town->guard_load_room );
      send_to_char( buf, pl );
      sprintf( buf, "Cavalry: %d deployed of %d max, vnum %d, load in room %d.\n\n", 
        mob_index[real_mobile(town->cavalry_vnum)].number, town->cavalry_max, town->cavalry_vnum, 
        town->cavalry_load_room );
      send_to_char( buf, pl );
    }

    if( IS_SET( pl->specials.act3, PLR3_NOSUR ) )
    {
      do_say(ch, "Get out of here whelp! Earn a title before you come to me", CMD_SAY);
      return TRUE;
    }

    send_to_char( "You can donate an item to increase resources.\n", pl );
    if( IS_SET( pl->specials.act3, PLR3_SURSERF ) )
      return TRUE;
    if( IS_SET( pl->specials.act3, PLR3_SURCOMMONER ) )
      return TRUE;
    send_to_char( "You can buy a catapult, ballista, or battering ram.\n", pl );
    send_to_char( "You can buy some town gates.\n", pl );
    if( IS_SET( pl->specials.act3, PLR3_SURKNIGHT ) )
      return TRUE;
    if( IS_SET( pl->specials.act3, PLR3_SURNOBLE ) )
      return TRUE;
    send_to_char( "You can deploy guards/stop deployment with the deploy command.\n", pl );
    send_to_char( "You can deploy town portals/stop deployment with the deploy command.\n", pl );
    if( IS_SET( pl->specials.act3, PLR3_SURLORD ) )
      return TRUE;
    send_to_char( "You can deploy cavalry/stop deployment with the deploy command.\n", pl );
    if( IS_SET( pl->specials.act3, PLR3_SURKING ) )
      return TRUE;

    return TRUE;
  }
  if( cmd == CMD_DONATE )
  {
    town = gettown( ch );
    if( !town )
    {
      logit(LOG_DEBUG, "warmaster: called outside of town.");
      return FALSE;
    }

    donation = get_obj_in_list_vis(pl, arg, pl->carrying);
    if( !donation )
    {
      send_to_char( "Donate what?!?\n", pl );
      return TRUE;
    }
    sprintf( buf, "You donate %s to %s.\n", donation->short_description, 
      town->zone->name );
    send_to_char( buf, pl );
    obj_from_char( donation, TRUE );
    town->resources += itemvalue( ch, donation );
    extract_obj( donation, TRUE );
    save_towns();
    return TRUE;
  }
  if( cmd == CMD_DEPLOY )
  {
    // These guys can't deploy guards.. 
    if( IS_SET( pl->specials.act3, PLR3_SURSERF ) )
      return FALSE;
    if( IS_SET( pl->specials.act3, PLR3_SURCOMMONER ) )
      return FALSE;
    if( IS_SET( pl->specials.act3, PLR3_SURKNIGHT ) )
      return FALSE;
    if( IS_SET( pl->specials.act3, PLR3_SURNOBLE ) )
      return FALSE;
    town = gettown( ch );
    if( !town )
    {
      logit(LOG_DEBUG, "warmaster: called outside of town.");
      return FALSE;
    }

    one_argument( arg, arg1 );
    if( *arg1 == '\0' )
    {
      send_to_char( "You can deploy: guards, calvary, or portals.\n", pl );
      return TRUE;
    }
    if( is_abbrev( arg1, "guards" ) )
    {
      // Toggle deploy guard and save
      if( town->deploy_guard )
      {
        send_to_char( "Town guards will now not be deployed.\n", pl );
        town->deploy_guard = FALSE;
      }
      else
      {
        send_to_char( "Town guards will now be deployed.\n", pl );
        town->deploy_guard = TRUE;
      }
      save_towns();
      return TRUE;
    }
    else if( is_abbrev( arg1, "cavalry" ) )
    {
      if( IS_SET( pl->specials.act3, PLR3_SURLORD ) )
        return FALSE;
      // Toggle deploy guard and save
      if( town->deploy_cavalry )
      {
        send_to_char( "Town cavalry will now not be deployed.\n", pl );
        town->deploy_cavalry = FALSE;
      }
      else
      {
        send_to_char( "Town calvary will now be deployed.\n", pl );
        town->deploy_cavalry = TRUE;
      }
      save_towns();
      return TRUE;
    }
    else if( is_abbrev( arg1, "portals" ) )
    {
      // Toggle deploy guard and save
      if( town->deploy_portals )
      {
        send_to_char( "Town portals will now not be deployed.\n", pl );
        town->deploy_portals = FALSE;
      }
      else
      {
        send_to_char( "Town portals will now be deployed.\n", pl );
        town->deploy_portals = TRUE;
      }
      save_towns();
      return TRUE;
    }
  }
  if( cmd == CMD_BUY )
  {
    // People that can't buy anything here.
    if( IS_SET( pl->specials.act3, PLR3_NOSUR ) 
     || IS_SET( pl->specials.act3, PLR3_SURSERF ) 
     || IS_SET( pl->specials.act3, PLR3_SURCOMMONER ) )
      return FALSE;

    rest = one_argument( arg, arg1 );
    rest = one_argument( rest, arg2 );
    if( *arg1 == '\0' )
    {
      send_to_char( "You can buy a catapult, ballista, or battering ram.\n", pl );
      send_to_char( "You can buy some town gates.\n", pl );
      return TRUE;
    }
    if( is_abbrev( arg1, "ballista" ) )
    {
      if( 5000 * 1000 > GET_MONEY(pl) )
      {
        send_to_char( "A ballista costs 5000 platinum.\n", pl );
        return TRUE;
      }
      else
      {
        // Load them a ballista and take the cash.
        siege = read_object(real_object(461), REAL);
        if( !siege )
        {
          logit(LOG_DEBUG, "warmaster: couldn't load ballista.");
          return FALSE;
        }
        SUB_MONEY( pl, 5000 * 1000, 0 );
        obj_to_room( siege, pl->in_room );
        act( "You buy $p.", FALSE, pl, siege, NULL, TO_CHAR );
        act( "$n buys $p.", FALSE, pl, siege, NULL, TO_ROOM );
        return TRUE;
      }
    }
    if( is_abbrev( arg1, "battering" ) 
     || is_abbrev( arg1, "ram" ) )
    {
      if( 5000 * 1000 > GET_MONEY(pl) )
      {
        send_to_char( "A battering ram costs 5000 platinum.\n", pl );
        return TRUE;
      }
      else
      {
        // Load them a battering ram and take the cash.
        siege = read_object(real_object(462), REAL);
        if( !siege )
        {
          logit(LOG_DEBUG, "warmaster: couldn't load battering ram.");
          return FALSE;
        }
        SUB_MONEY( pl, 5000 * 1000, 0 );
        obj_to_room( siege, pl->in_room );
        act( "You buy $p.", FALSE, pl, siege, NULL, TO_CHAR );
        act( "$n buys $p.", FALSE, pl, siege, NULL, TO_ROOM );
        return TRUE;
      }
    }
    if( is_abbrev( arg1, "catapult" ) )
    {
      if( 5000 * 1000 > GET_MONEY(pl) )
      {
        send_to_char( "A catapult costs 5000 platinum.\n", pl );
        return TRUE;
      }
      else
      {
        // Load them a catapult and take the cash.
        siege = read_object(real_object(463), REAL);
        if( !siege )
        {
          logit(LOG_DEBUG, "warmaster: couldn't load catapult.");
          return FALSE;
        }
        SUB_MONEY( pl, 5000 * 1000, 0 );
        obj_to_room( siege, pl->in_room );
        act( "You buy $p.", FALSE, pl, siege, NULL, TO_CHAR );
        act( "$n buys $p.", FALSE, pl, siege, NULL, TO_ROOM );
        return TRUE;
      }
    }
    if( is_abbrev( arg1, "town" ) 
     || is_abbrev( arg1, "gates" ) )
    {
      town = gettown( ch );
      if( !town )
      {
        logit(LOG_DEBUG, "warmaster: 'buy gates' called outside of town.");
        return FALSE;
      }

      ztop = town->zone->real_top;
      count = 0;
      if( *arg2 == '\0' )
      {
        send_to_char( "You can buy: \n", ch );
        for( i = town->zone->real_bottom; i <= ztop; i++ )
        {
          for( j = 0; j < NUM_EXITS; j++ )
          {
            // Skip non-existant exits and exits to nowhere.
            if( !world[i].dir_option[j] || world[i].dir_option[j]->to_room == NOWHERE )
              continue;
            if( world[world[i].dir_option[j]->to_room].zone != world[i].zone )
            {
              sprintf( buf, "%2d) %-5s %s\n", ++count, dirs[j],
                world[world[i].dir_option[j]->to_room].name );
              send_to_char( buf, pl );
            }
          }
        }
        if( count == 0 )
          send_to_char( "No exits from town found!\n", pl );
        return TRUE;
      }
      else
      {
        if( (numgate = atoi( arg2 )) < 1 )
        {
          send_to_char( "Invalid gate number (too small).\n", pl );
          return TRUE;
        }
        for( i = town->zone->real_bottom; i <= ztop; i++ )
        {
          for( j = 0; j < NUM_EXITS; j++ )
          {
            // Skip non-existant exits and exits to nowhere.
            if( !world[i].dir_option[j] || world[i].dir_option[j]->to_room == NOWHERE )
              continue;
            if( world[world[i].dir_option[j]->to_room].zone != world[i].zone )
            {
              if( ++count == numgate )
              {
                // Put a gate there.
                siege = read_object(real_object(464), REAL);
                if( !siege )
                {
                  logit(LOG_DEBUG, "warmaster: couldn't load town gates.");
                  return FALSE;
                }
                // Block the correct direction.
                siege->value[0] = rev_dir[j];
                obj_to_room( siege, i );//world[i].dir_option[j]->to_room );
                act( "You buy $p.", FALSE, pl, siege, NULL, TO_CHAR );
                act( "$n buys $p.", FALSE, pl, siege, NULL, TO_ROOM );
                return TRUE;
              }
            }
          }
        }
        send_to_char( "Invalid gate number (too big).\n", pl );
        return TRUE;
      }
    }
  }

  return FALSE;
}

// Checks to see if it's time to deploy guards/cavalry/portals.
void check_deploy( struct zone_data *zone )
{
  P_town town = towns;
  P_char mob;
  int i, vnum, rnum;
  char buf[MAX_STRING_LENGTH];

  // Search for town data corresponding to zone.
  while( town && town->zone != zone )
    town = town->next_town;
  // If no town data, or not set to deploy.
  if( !town )
    return;

  if( town->deploy_guard && town->guard_vnum )
  {
    rnum = real_mobile( town->guard_vnum );

    sprintf( buf, "Mob: %d '%s': Current load: %d, Max load: %d in room: %d.",
      town->guard_vnum, mob_index[rnum].desc2, mob_index[rnum].number,
      mob_index[rnum].limit, town->guard_load_room );
    wizlog( 60, buf );

    // Deploy guards if necessary..
    for( i = mob_index[rnum].number; i < mob_index[rnum].limit; i++ )
    {
      mob = read_mobile(rnum, REAL);
      apply_zone_modifier(mob);
      char_to_room(mob, real_room(town->guard_load_room), -1);
    }
  }
  if( town->deploy_cavalry && town->cavalry_vnum )
  {
    rnum = real_mobile( town->cavalry_vnum );

    sprintf( buf, "Mob: %d '%s': Current load: %d, Max load: %d in room: %d.",
      town->cavalry_vnum, mob_index[rnum].desc2, mob_index[rnum].number,
      mob_index[rnum].limit, town->cavalry_load_room );
    wizlog( 60, buf );
    
    // Deploy cavalry if necessary..
    for( i = mob_index[rnum].number; i < mob_index[rnum].limit; i++ )
    {
      mob = read_mobile(rnum, REAL);
      apply_zone_modifier(mob);
      char_to_room(mob, real_room(town->cavalry_load_room), -1);
    }
  }
// Need to add portals

}

