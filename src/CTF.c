
/* CTF.c

   - Property of Duris
     4/2011

*/

#include <stdlib.h>
#include <cstring>
#include <vector>
using namespace std;

#include "prototypes.h"
#include "defines.h"
#include "utils.h"
#include "structs.h"
#include "racewar_stat_mods.h"
#include "sql.h"
#include "epic.h"
#include "buildings.h"
#include "CTF.h"
#include "assocs.h"
#include "specs.prototypes.h"
#include "utility.h"
#include "guildhall.h"
#include "alliances.h"
#include "events.h"

#ifdef __NO_MYSQL__

int init_CTF()
{
    // load nothing
}

void do_CTF(P_char ch, char *arg, int cmd) 
{
    // do nothing
}
#else

extern MYSQL* DB;

int init_CTF()
{
  fprintf(stderr, "--Loading CTF\r\n");
  
  load_CTF();
}

int load_CTF()
{
  // load CTF from DB
  if( !qry("SELECT id, FROM CTF") )
  {
    debug("load_CTF() can't read from db");
    return FALSE;
  }
  
  MYSQL_RES *res = mysql_store_result(DB);
 
  if( mysql_num_rows(res) < 1 )
  {
    mysql_free_result(res);
    return FALSE;
  }

  MYSQL_ROW row;
  while( row = mysql_fetch_row(res) )
  {
  //venthix code goes here
  }
  mysql_free_result(res);

  return TRUE;
}

/*void show_CTF(P_char ch)
{
  char buf[MAX_STRING_LENGTH], Gbuf1[MAX_STRING_LENGTH];
  char title[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
  int i;
  FILE *f;

  send_to_char("&+GCapture the Flag:&n\r\n", ch);
  for (i = 0; i <= ; i++)
  {
    if (!qry("SELECT id, pid, racewar, last_cap, FROM CTF WHERE id = %d", i))
    {
      debug("show_CTF() cant read from db");
      return;
    }

    MYSQL_RES *res = mysql_store_result(DB);

    if (mysql_num_rows(res) < 1)
    {
      mysql_free_result(res);
      return;
    }

    MYSQL_ROW row = mysql_fetch_row(res);

    //venthix code goes here

    mysql_free_result(res);

    sprintf("replace this with useful information");
    f = fopen(Gbuf1, "r");
    if (!f)
    {
      sprintf(title, "Unknown");
    }
    else
    {
      fgets(Gbuf2, MAX_STR_NORMAL, f);
      Gbuf2[strlen(Gbuf2)-1] = 0;
      Gbuf2[MAX_STR_ASC-1] = 0;
      strcpy(title, Gbuf2);
      fclose(f);
    }

    sprintf(buf, "&+WChar: &+c%2d &+WRacewar: &+c%-18s&n\r\n", i+1, pad_ansi(GET_NAME(ch)), 18).c_str(), title);
    send_to_char(buf, ch);
    if (IS_TRUSTED(ch) || ((owner != 0) && (owner == GET_A_NUM(ch))))
    {
      sprintf(buf, "Guild   Points"));
      send_to_char(buf, ch);
	send_to_char("\r\n", ch);
    }
  }
}*/

/*void do_CTF(P_char ch, char *arg, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char buf3[MAX_STRING_LENGTH];

  if( !ch || IS_NPC(ch) )
    return;

  argument_interpreter(arg, buf2, buf3);
  
  if( IS_TRUSTED(ch) && !str_cmp("reset", buf2) )
  {
    if (GET_LEVEL(ch) != OVERLORD)
    {
      send_to_char("Ask an overlord, this command will reset all active CTF.\n", ch);
      return;
    }
    
    if (!str_cmp("all", buf3))
    {
      send_to_char("Resetting all CTF parameters.\n", ch);
      reset_CTF(ch);
    }
    else if (!isdigit(*buf3))
    {
      send_to_char("You must specify a guild or racewar side ID or 'all'.", ch);
      return;
    }
    else
    {
      int id = atoi(buf3);
      building 
      reset_one_guild_CTF(guild);
      sprintf(buf, "You reset guild # %d's CTF stats.", id);
      return;
    }

    return;
  }
  
  if( IS_TRUSTED(ch) )
    //show_CTF_wiz(ch);
    show_CTF(ch);
  else
    show_CTF(ch);
    
}*/
/*
void reset_one_guild_CTF(guild)
{
  int id;

  if (!guild->id)
  {
    debug("error calling reset_one_guild_CTF, no guild ID available");
    return;
  }
  id = guild->id-1;

} */

void reset_CTF(P_char ch)
{
  //Guild here

  /*for (;;)
  {
    guild = GET_A_NUM(ch);
    debug("resetting guild #: %d", guild->id-1);
    reset_one_guild_CTF(guild);
  }
    remove CTF effects from guild?
   */
}

#endif

