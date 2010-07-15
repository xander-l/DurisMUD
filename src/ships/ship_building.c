
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ships.h"
#include "comm.h"
#include "db.h"
#include "graph.h"
#include "interp.h"
#include "objmisc.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "events.h"
#include <math.h>

void newship_build_event()
{
  char     tmp_buf[MAX_STRING_LENGTH];

  if (!current_event || (current_event->type != EVENT_SPECIAL))
  {
    logit(LOG_EXIT, "Ship build event Screwed up");
    raise(SIGSEGV);
  }
  if (sscanf(((const char *) current_event->target.t_arg), "%s", tmp_buf) == 1)
  {
    ShipVisitor svs;
    for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
    {
      if (isname(tmp_buf, svs->ownername))
      {
        REMOVE_BIT(svs->flags, MAINTENANCE);
        return;
      }
    }
  }
}

