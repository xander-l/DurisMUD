#ifndef NANNY_UTILS_H
#define NANNY_UTILS_H

#include "structs.h"

void loadHints(void);
int tossHint(P_char ch);
void Decrypt(char *text, int sizeOfText, const char *key, int sizeOfKey);

#endif
