/*
 * Copyright (c) 1995-2007, Michael Glosenger
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of Michael Glosenger may not be used to endorse or promote 
 *       products derived from this software without specific prior written 
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MICHAEL GLOSENGER ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
 * EVENT SHALL MICHAEL GLOSENGER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _VAR_H_

#include "de.h"

#define MAX_VARNAME_LEN    41
#define MAX_VARVAL_LEN    MAX_PROMPTINPUT_LEN

typedef struct _variable
{
  char varName[MAX_VARNAME_LEN];  // name of variable
  char varValue[MAX_VARVAL_LEN]; // value of variable

  struct _variable *Next;
} variable;

#define VARTYPE_LOWEST       0
#define VARTYPE_DISPVAR      0   // display config var
#define VARTYPE_CONFIGVAR    1   // 'config' config var
#define VARTYPE_CHECKVAR     2   // check config var
#define VARTYPE_USERVAR      3   // user-defined var
#define VARTYPE_OTHER        4   // shrug
#define VARTYPE_HIGHEST      4

#define VAR_ADDED           0
#define VAR_REPLACED        1
#define VAR_BLANK           2
#define VAR_SAMEVAL         3
#define VAR_ERROR           4
#define VAR_DISPLAY         5
#define VAR_CANNOT_BE_ZERO  6
#define VAR_BELOW_MINIMUM   7

#define _VAR_H_
#endif
