#ifndef _SPECIALIZATIONS_H_
#define _SPECIALIZATIONS_H_

#include "specialization_data.h"

void   do_specialize(P_char ch, char *argument, int cmd);
string single_spec_list(int race, int cls);
void   unspecialize(P_char ch, P_obj obj);
bool   append_valid_specs(char *buf, P_char ch);

#endif
