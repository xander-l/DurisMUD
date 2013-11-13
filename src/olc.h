#ifndef _OLC_H_
#define _OLC_H_

#include "structs.h"

struct olc_data {
  P_desc desc;
  int mode;
  int rnum;
  void *misc;
};


/* publically availably functions in olc.c */
void do_olc(P_char ch, char *arg, int cmd);
void olc_string_add(struct olc_data *, char *);
void olc_prompt(struct olc_data *);
void olc_end(struct olc_data *);
void olc_save_wld(int zone);
void olc_del_exit_menu(struct olc_data *);

#ifdef _OLC_SOURCE_

#define OLC_MODE_ROOM0 0        /* edit room main menu */
#define OLC_MODE_ROOM1 1        /* chaging room name */
#define OLC_MODE_ROOM2 2        /* in editor, editting room desc */

#define OLC_MODE_ROOM_FLAGS 3   /* room flags */
#define OLC_MODE_ROOM_SECT 4    /* sector type */
#define OLC_MODE_XTRA 5         /* extra description keywords.. */
#define OLC_MODE_XTRA_DEL 7     /* DELeting an extra desc */
#define OLC_MODE_XTRA1 8        /* choose to edit the keywords, or
                                   desc */
#define OLC_MODE_XTRA2 9        /* editting the KEYWORDS for a desc */
#define OLC_MODE_XTRA3 10       /* editting the DESC for a desc */

#define OLC_MODE_EXIT 11        /* main exits menu */
#define OLC_MODE_EXIT_DEL 12    /* deleting an exit */
#define OLC_MODE_EXIT1  13      /* */

#define OLC_MODE_LAST_ROOM 13


extern P_room world;
extern const char *sector_types[];
extern const char *room_bits[];
extern const char *dirs[];
extern struct zone_data *zone_table;
extern int top_of_world;
extern int top_of_zone_table;

extern const char *olc_dirs[];

/* generic functions in olc.c */
void olc_show_menu(struct olc_data *);
void olc_build_bitflag_menu32(char *str, ulong value, const
                              char *names[]);
void olc_build_flag_menu8(char *str, ubyte value, const char
                          *names[]);
int olc_check_rnum_access(P_desc desc, int rnum);

/* room specific branches in olc_room.c */
void olc_room_menu(char *buf1, struct olc_data *data);
void olc_room_string_add(struct olc_data *data, char *str);
void olc_room_callback(P_desc, int, char *);


#endif /* _OLC_SOURCE_ */
#endif /* _OLC_H_ */
