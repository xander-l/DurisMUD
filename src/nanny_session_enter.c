/**
 * @file nanny_session_enter.c
 * @brief Character entry flow extracted from nanny_session.c.
 *
 * This module contains the main enter_game routine and delegates specialized
 * tasks (EQ wipe handling, game-mode leveling, and post-login views) to
 * focused helpers in separate translation units.
 */

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "events.h"
#include "guildhall.h"
#include "nanny_session_helpers.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"

extern P_char character_list;
extern P_desc descriptor_list;
extern P_room world;
extern P_nevent ne_schedule[PULSES_IN_TICK];
extern P_nevent ne_schedule_tail[PULSES_IN_TICK];
extern struct time_info_data time_info;
extern int pulse;
extern struct zone_data *zone_table;
extern int top_of_world;

/**
 * Existing or new character entering the game.
 */
void enter_game(P_desc d)
{
  struct zone_data *zone;
  struct affected_type af1, *afp1, *afp2;
  crm_rec *crec = NULL;
  int cost;
  int r_room = NOWHERE;
  long time_gone = 0, hit_g, move_g, heal_time, rest;
  time_t ct = time(NULL);
  int mana_g;
  char Gbuf1[MAX_STRING_LENGTH], timestr[MAX_STRING_LENGTH];
  bool nobonus = FALSE;
  P_char ch = d->character;
  P_desc i;
  P_nevent evp;
  P_Guild guild;

  /* Bring them to life. */
  SET_POS(ch, POS_STANDING + STAT_NORMAL);

  /* Choose the room based on the character's rent type and state. */
  if ((d->rtype == RENT_QUIT && GET_LEVEL(ch) < 2) || d->rtype == RENT_DEATH)
  {
    r_room = real_room(GET_BIRTHPLACE(ch));
  }
  else if (d->rtype == RENT_CRASH)
  {
    r_room = real_room(ch->specials.was_in_room);
  }
  else
  {
    r_room = real_room(ch->specials.was_in_room);

    if (r_room == NOWHERE)
      r_room = ch->in_room;
  }

  /* Respect racewar heaven timers, otherwise override to safe defaults. */
  if (ch->only.pc->pc_timer[PC_TIMER_HEAVEN] > ct)
  {
    if (IS_RACEWAR_GOOD(ch))
      r_room = real_room(GOOD_HEAVEN_ROOM);
    else if (IS_RACEWAR_EVIL(ch))
      r_room = real_room(EVIL_HEAVEN_ROOM);
    else if (IS_RACEWAR_UNDEAD(ch))
      r_room = real_room(UNDEAD_HEAVEN_ROOM);
    else if (IS_ILLITHID(ch))
      r_room = real_room(NEUTRAL_HEAVEN_ROOM);
    else if (IS_RACEWAR_NEUTRAL(ch))
      r_room = real_room(NEUTRAL_HEAVEN_ROOM);
    else
      r_room = real_room(VROOM_CAGE);
  }

  if (r_room == NOWHERE)
  {
    if (GET_HOME(ch))
      r_room = real_room(GET_HOME(ch));
    else
      r_room = real_room(GET_BIRTHPLACE(ch));

    if (r_room == NOWHERE)
      if (IS_TRUSTED(ch))
        r_room = real_room0(1200);
      else
        r_room = GET_ORIG_BIRTHPLACE(ch);

    if (r_room == NOWHERE)
      r_room = real_room0(11);
  }
  else if (IS_SHIP_ROOM(r_room))
  {
    r_room = real_room(GET_BIRTHPLACE(ch));
  }

  if (zone_table[world[r_room].zone].flags & ZONE_CLOSED)
    r_room = real_room(GET_BIRTHPLACE(ch));

  /* Avoid invalid room indices. */
  if (r_room > top_of_world)
    r_room = real_room(11);
  if (r_room < 0)
    r_room = real_room(1197);

  /* Check guildhall access for home/birthplace/spawn. */
  r_room = check_gh_home(ch, r_room);

  ch->in_room = NOWHERE;
  char_to_room(ch, r_room, -2);

  ch->specials.x_cord = 0;
  ch->specials.y_cord = 0;
  ch->specials.z_cord = 0;

  if (GET_LEVEL(ch))
  {
    ch->desc = d;

    reset_char(ch);

    cost = 0;

    if ((d->rtype == RENT_CRASH) || (d->rtype == RENT_CRASH2))
    {
      send_to_char("\r\nRestoring items and pets from crash save info...\r\n",
                   ch);
      cost = restoreItemsOnly(ch, 100);
    }
    else if (d->rtype == RENT_CAMPED)
    {
      send_to_char("\r\nYou break camp and get ready to move on...\r\n", ch);
      cost = restoreItemsOnly(ch, 0);
    }
    else if (d->rtype == RENT_INN)
    {
      send_to_char("\r\nRetrieving rented items from storage...\r\n", ch);
      cost = restoreItemsOnly(ch, 100);
    }
    else if (d->rtype == RENT_LINKDEAD)
    {
      send_to_char("\r\nRetrieving items from linkdead storage...\r\n", ch);
      cost = restoreItemsOnly(ch, 200);
    }
    else if (d->rtype == RENT_POOFARTI)
    {
      send_to_char("\r\nThe gods have taken your artifact...\r\n", ch);
      cost = restoreItemsOnly(ch, 100);
    }
    else if (d->rtype == RENT_FIGHTARTI)
    {
      nobonus = TRUE;
      send_to_char("\r\nYour artifacts argued all night...\r\n", ch);
      cost = restoreItemsOnly(ch, 100);
    }
    else if (d->rtype == RENT_SWAPARTI)
    {
      send_to_char(
        "\r\nThe gods have taken your artifact... and replaced it with another!\r\n",
        ch);
      cost = restoreItemsOnly(ch, 100);
    }
    else if (d->rtype == RENT_DEATH)
    {
      if (ch->only.pc->pc_timer[PC_TIMER_HEAVEN] > ct)
        send_to_char("\r\nYour soul finds its way to the afterlife...\r\n", ch);
      else
        send_to_char("\r\nYou rejoin the land of the living...\r\n", ch);
      restoreItemsOnly(ch, 0);
    }
    else
    {
      send_to_char("\r\nCouldn't find any items in storage for you...\r\n", ch);
    }

    if (cost == -2)
    {
      send_to_char(
        "\r\nSomething is wrong with your saved items information - "
        "please talk to an Implementor.\r\n",
        ch);
    }

    /* Remove camping affect to avoid stale camp state after reconnect. */
    if (IS_AFFECTED(ch, AFF_CAMPING))
      affect_from_char(ch, TAG_CAMP);

    ch->specials.affected_by = 0;
    ch->specials.affected_by2 = 0;
    ch->specials.affected_by3 = 0;
    ch->specials.affected_by4 = 0;
    ch->specials.affected_by5 = 0;

    if (affected_by_spell(ch, AIP_YOUSTRAHDME))
      affect_from_char(ch, AIP_YOUSTRAHDME);

    if (affected_by_spell(ch, AIP_YOUSTRAHDME2))
      affect_from_char(ch, AIP_YOUSTRAHDME2);

    /* Remove any morph flag that might be leftover. */
    REMOVE_BIT(ch->specials.act, PLR_MORPH | PLR_WRITE | PLR_MAIL);

    /* This may fix the disguise not showing on who bug. */
    if (PLR_FLAGGED(ch, PLR_NOWHO))
      PLR_TOG_CHK(ch, PLR_NOWHO);

    /* time_gone is how many ticks (currently real minutes) they have been out of the game. */
    time_gone = (ct - ch->player.time.saved) / SECS_PER_MUD_HOUR;
    /* rest is how many seconds they have been out of the game. */
    rest = ct - ch->player.time.saved;

    ch->player.time.birth -= time_gone;

    SET_POS(ch, POS_STANDING + STAT_NORMAL);
    heal_time = MAX(0, (time_gone - 120));

    if (d->rtype != RENT_DEATH)
    {
      hit_g = BOUNDED(0, hit_regen(ch, FALSE) * heal_time, 3000);
      mana_g = BOUNDED(0, mana_regen(ch, FALSE) * heal_time, 3000);
      move_g = BOUNDED(0, move_regen(ch, FALSE) * heal_time, 3000);
    }
    else
    {
      hit_g = mana_g = move_g = 0;
    }

    GET_HIT(ch) = BOUNDED(1, GET_HIT(ch) + hit_g, GET_MAX_HIT(ch));
    GET_MANA(ch) = BOUNDED(1, GET_MANA(ch) + mana_g, GET_MAX_MANA(ch));
    GET_VITALITY(ch) =
      BOUNDED(1, GET_VITALITY(ch) + move_g, GET_MAX_VITALITY(ch));

    if (GET_HIT(ch) != GET_MAX_HIT(ch))
      StartRegen(ch, EVENT_HIT_REGEN);
    if (GET_MANA(ch) != GET_MAX_MANA(ch))
      StartRegen(ch, EVENT_MANA_REGEN);
    if (GET_VITALITY(ch) != GET_MAX_VITALITY(ch))
      StartRegen(ch, EVENT_MOVE_REGEN);

    set_char_size(ch);

    update_skills(ch);
    // Once racial skills are removed, this will be unnecessary.
    //  Furthermore, it will wipe any formerly-racial now-epic skills learned. - Lohrr
    //    reset_racial_skills( ch );

  }
  /* Don't do any of above for new chars, but do give well-rested bonus. */
  else
  {
    /* Slept for a year. */
    rest = 365 * 24 * 60 * 60;
  }

  send_to_char(WELC_MESSG, ch);
  ch->desc = d;
  ch->next = character_list;
  character_list = ch;

  /* Walk through affects and drop AFFTYPE_OFFLINE timers. */
  for (afp1 = ch->affected; afp1; afp1 = afp2)
  {
    afp2 = afp1->next;

    if (IS_SET(afp1->flags, AFFTYPE_OFFLINE))
    {
      if (IS_SET(afp1->flags, AFFTYPE_SHORT))
      {
        LOOP_EVENTS_CH(evp, ch->nevents)
        {
          if (evp->func == event_short_affect && evp->data != NULL
            && ((struct event_short_affect_data *)(evp->data))->af == afp1)
          {
            long total_pulses =
              (evp->timer - ((evp->element < pulse) ? 0 : 1)) * PULSES_IN_TICK
              + evp->element - pulse;
            if ((total_pulses = total_pulses - rest * WAIT_SEC) < 1)
            {
              /* The event would have fired while offline; expire it now. */
              wear_off_message(ch, afp1);
              affect_remove(ch, afp1);
              break;
            }

            /* Pull evp from ne_schedule[] list. */
            if (evp->next_sched)
            {
              evp->next_sched->prev_sched = evp->prev_sched;
            }
            else if (evp == ne_schedule_tail[evp->element])
            {
              ne_schedule_tail[evp->element] = evp->prev_sched;
            }
            else
            {
              raise(SIGSEGV);
            }

            if (evp->prev_sched)
            {
              evp->prev_sched->next_sched = evp->next_sched;
            }
            else if (evp == ne_schedule[evp->element])
            {
              ne_schedule[evp->element] = evp->next_sched;
            }
            else
            {
              raise(SIGSEGV);
            }

            /* Update timer with remaining pulses. */
            evp->timer = (total_pulses) / PULSES_IN_TICK + 1;

            /* total_pulses + pulse because our starting point is ne_schedule[pulse]. */
            evp->element = (total_pulses + pulse) % PULSES_IN_TICK;

            /* Add evp to the end of the new row. */
            evp->next_sched = NULL;
            evp->prev_sched = ne_schedule_tail[evp->element];
            if (evp->prev_sched != NULL)
            {
              evp->prev_sched->next_sched = evp;
            }
            else
            {
              ne_schedule[evp->element] = evp;
            }
            ne_schedule_tail[evp->element] = evp;
            break;
          }
        }
      }
      else if (time_gone > 0)
      {
        afp1->duration -= time_gone;
        if (afp1->duration < 0)
        {
          affect_remove(ch, afp1);
        }
      }
    }
  }

  affect_total(ch, FALSE);

  if ((guild = GET_ASSOC(ch)) != NULL)
  {
    guild->update_member(ch);
    if (IS_MEMBER(GET_A_BITS(ch)))
    {
      do_gmotd(ch, "", CMD_GMOTD);
    }
  }

  /* check the fraglist .. */
  checkFragList(ch);
  if (!ch->player.short_descr)
    generate_desc(ch);

  if (!ch->player.name)
  {
    wizlog(57,
           "&+WSomething fucked up with character name. Tell a coder immediately!&n");
    SEND_TO_Q(
      "Serious screw-up with your player file. Log on another char and talk to gods.",
      d);
    STATE(d) = CON_FLUSH;
  }

  if (!d->host)
  {
    wizlog(57, "%s had null host.", GET_NAME(ch));
    snprintf(d->host, MAX_STRING_LENGTH, "UNKNOWN");
  }

  ch->only.pc->last_ip = ip2ul(d->host);

  if (!d->login)
  {
    wizlog(57, "%s had null login.", GET_NAME(ch));
    snprintf(d->login, MAX_STRING_LENGTH, "UNKNOWN");
  }

  if (IS_TRUSTED(ch))
  {
    ch->only.pc->wiz_invis = 56;
    do_vis(ch, 0, -4);
  }
  if (d->rtype == RENT_DEATH)
  {
    act("$n has returned from the dead.", TRUE, ch, 0, 0, TO_ROOM);
    GET_COND(ch, FULL) = -1;
    GET_COND(ch, THIRST) = -1;
    GET_COND(ch, DRUNK) = 0;
  }
  else
    act("$n has entered the game.", TRUE, ch, 0, 0, TO_ROOM);

  /* Inform gods that a newbie has entered the game. */
  if (IS_NEWBIE(ch))
  {
    statuslog(ch->player.level,
      "&+GNEWBIE %s HAS ENTERED THE GAME! Help him out :) ", GET_NAME(ch));
    snprintf(Gbuf1, MAX_STRING_LENGTH,
      "&+GNEWBIE %s HAS ENTERED THE GAME! Help him out :)\n", GET_NAME(ch));
    for (i = descriptor_list; i; i = i->next)
    {
      if (i->connected)
        continue;

      if (opposite_racewar(ch, i->character))
        continue;
      if (!IS_SET(i->character->specials.act2, PLR2_NCHAT))
        continue;
      if (!IS_SET(PLR2_FLAGS(i->character), PLR2_NEWBIE_GUIDE))
        continue;
      if (IS_DISGUISE_PC(i->character)
        || IS_DISGUISE_ILLUSION(i->character)
        || IS_DISGUISE_SHAPE(i->character))
        continue;

      send_to_char(Gbuf1, i->character, LOG_PRIVATE);
    }
  }

  if (!GET_LEVEL(ch))
  {
    do_start(ch, 0);
    load_obj_to_newbies(ch);
  }
  else if (IS_SET(ch->specials.act2, PLR2_NEWBIEEQ) && !ch->carrying)
    load_obj_to_newbies(ch);

  /* Hack to handle improperly set highest_level. */
  if (ch->only.pc->highest_level > MAXLVL)
  {
    ch->only.pc->highest_level = GET_LEVEL(ch);
  }

  /* Add well-rested or rested bonus, if applicable. */
  if (nobonus)
  {
  }
  else if (rest / 3600 >= 20)
  {
    affect_from_char(ch, TAG_WELLRESTED);
    affect_from_char(ch, TAG_RESTED);

    memset(&af1, 0, sizeof(struct affected_type));
    af1.type = TAG_WELLRESTED;
    af1.modifier = 0;
    af1.duration = 150;
    af1.location = 0;
    af1.flags = AFFTYPE_PERM | AFFTYPE_NODISPEL | AFFTYPE_OFFLINE;
    affect_to_char(ch, &af1);

    debug("'%s' getting well-rested bonus!", J_NAME(ch));
  }
  else if (rest / 3600 >= 9)
  {
    affect_from_char(ch, TAG_WELLRESTED);
    affect_from_char(ch, TAG_RESTED);

    memset(&af1, 0, sizeof(struct affected_type));
    af1.type = TAG_RESTED;
    af1.modifier = 0;
    af1.duration = 150;
    af1.location = 0;
    af1.flags = AFFTYPE_PERM | AFFTYPE_NODISPEL | AFFTYPE_OFFLINE;
    affect_to_char(ch, &af1);

    debug("'%s' getting rested bonus!", J_NAME(ch));
  }

  GetMIA(ch->player.name, Gbuf1);
  ct -= 4 * 60 * 60;
  snprintf(timestr, MAX_STRING_LENGTH, "%s", asctime(localtime(&ct)));
  *(timestr + strlen(timestr) - 1) = '\0';
  strcat(timestr, " EST");
  ct += 4 * 60 * 60;

  loginlog(GET_LEVEL(ch), "%s [%s] enters game @ %s.%s [%d]", GET_NAME(ch),
           d->host, timestr, Gbuf1, world[ch->in_room].number);
  sql_log(ch, CONNECTLOG, "Entered Game");

  if (GET_LEVEL(ch) >= MINLVLIMMORTAL)
    loginlog(GET_LEVEL(ch), "&+GIMMORTAL&n: (%s) [%s] has logged on.%s",
             GET_NAME(ch), d->host, Gbuf1);

  /* Apply CTF/CHAOS adjustments in their own module. */
  apply_mode_level_adjustments(ch);

  writeCharacter(ch, 1, NOWHERE);
  sql_save_player_core(ch);
  sql_connectIP(ch);
  displayShutdownMsg(ch);

  if (IS_SET(ch->specials.act, PLR_SMARTPROMPT))
    REMOVE_BIT(ch->specials.act, PLR_SMARTPROMPT);

  schedule_pc_events(ch);

  if (EVIL_RACE(ch) && PLR_FLAGGED(ch, PLR_NOWHO))
  {
    PLR_TOG_CHK(ch, PLR_NOWHO);
  }

  if (IS_AFFECTED5(ch, AFF5_HOLY_DHARMA))
  {
    affect_from_char(ch, SPELL_HOLY_DHARMA);
    send_to_char("&+cThe &+Cdivine &+cinspiration withdraws from your soul.&n\r\n",
                 ch);
  }

  if (IS_SET(ch->specials.act, PLR_ANONYMOUS))
  {
    REMOVE_BIT(ch->specials.act, PLR_ANONYMOUS);
  }

  affect_from_char(ch, TAG_SKILL_TIMER);

  memset(&af1, 0, sizeof(af1));
  af1.type = TAG_SKILL_TIMER;
  af1.flags = AFFTYPE_STORE | AFFTYPE_SHORT;
  af1.duration = 5 * WAIT_MIN;

  af1.modifier = TAG_MENTAL_SKILL_NOTCH;
  affect_to_char(ch, &af1);

  af1.modifier = TAG_PHYS_SKILL_NOTCH;
  affect_to_char(ch, &af1);

  initialize_logs(ch, true);

  send_offline_messages(ch);

  if (GET_LEVEL(ch) < MINLVLIMMORTAL)
    update_ingame_racewar(GET_RACEWAR(ch));

  /* Make sure existing chars have base stats 80 - Drannak */
  if (d->character->base_stats.Str < 80)
  {
    d->character->base_stats.Str = 80;
  }
  if (d->character->base_stats.Agi < 80)
  {
    d->character->base_stats.Agi = 80;
  }
  if (d->character->base_stats.Dex < 80)
  {
    d->character->base_stats.Dex = 80;
  }
  if (d->character->base_stats.Con < 80)
  {
    d->character->base_stats.Con = 80;
  }
  if (d->character->base_stats.Luk < 80)
  {
    d->character->base_stats.Luk = 80;
  }
  if (d->character->base_stats.Pow < 80)
  {
    d->character->base_stats.Pow = 80;
  }
  if (d->character->base_stats.Int < 80)
  {
    d->character->base_stats.Int = 80;
  }
  if (d->character->base_stats.Wis < 80)
  {
    d->character->base_stats.Wis = 80;
  }

  if (d->character->base_stats.Cha < 80)
  {
    d->character->base_stats.Cha = 80;
  }

  /* Goodie AP fix. */
  if (GET_CLASS(ch, CLASS_ANTIPALADIN) && GET_ALIGNMENT(ch) > -10)
    GET_ALIGNMENT(ch) = -1000;

  /* Goodie mino necro fix. */
  if (GET_CLASS(ch, CLASS_NECROMANCER) && GET_ALIGNMENT(ch) > -10)
    GET_ALIGNMENT(ch) = -1000;

#ifdef EQ_WIPE
  if (d->character->player.time.played < EQ_WIPE)
  {
    if (!IS_TRUSTED(d->character))
    {
      perform_eq_wipe(ch);
    }
    ch->player.time.played += EQ_WIPE;
  }
#endif

  if (affected_by_spell(ch, SPELL_CURSE_OF_YZAR))
  {
    if (!affected_by_spell(ch, TAG_RACE_CHANGE))
    {
      /* First race change in 5 sec. */
      add_event(event_change_yzar_race, 5 * WAIT_SEC, ch, ch, NULL, 0, NULL,
                sizeof(NULL));
    }
    else
    {
      int time_to_witching_hour =
        (time_info.hour >= 3) ? (27 - time_info.hour) : (3 - time_info.hour);
      time_to_witching_hour = time_to_witching_hour * PULSES_IN_TICK;
      time_to_witching_hour -=
        (300 - ne_event_time(get_scheduled(event_another_hour)));

      add_event(event_change_yzar_race, time_to_witching_hour, ch, ch, NULL, 0,
                NULL, sizeof(NULL));
    }
  }

  /* Run view updates as the final step of the entry flow. */
  run_post_enter_view(ch);
}
