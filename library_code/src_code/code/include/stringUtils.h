/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#ifndef __STRINGUTILS_H__
#define __STRINGUTILS_H__
#include "common.h"

int32 getFirstLocationAtString(const int8 *string, const int8 *findString);
int32 getLastLocationAtString(const int8 *string, const int8 *findString);
int32 strCmpNoCaseSensitive(const int8 *s1, const int8 *s2);
int32 stringToupper(int8 *string);
int32 stringTolower(int8 *string);
int8 *getStringAtFisrtFind(const int8 *string, const int8 *subStr);

#endif
