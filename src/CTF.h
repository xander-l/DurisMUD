#ifndef _CTF_H_
#define _CTF_H_

#include "structs.h"

#define CTF_MODE 1

#if CTF_MODE
#define CTF_GOOD_FLAG 790
#define CTF_EVIL_FLAG 791
#define GOODRACE_FLAGDROP 132573
#define EVILRACE_FLAGDROP 97628
#endif


int init_CTF();
int load_CTF();
void show_CTF(P_char);
void do_CTF(P_char, char*, int);
//void reset_one_guild_CTF(Guild);
void reset_CTF(P_char);
void CTF_upkeep();
#endif
