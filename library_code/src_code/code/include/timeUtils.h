/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#ifndef __TIMEUTILS_H__
#define __TIMEUTILS_H__
#include "common.h"


typedef void (*functionRun)(void);

double gettimeRunByClock(functionRun func);
void getNowTime(time_t *now);
int32 gettimeRunBytimes(functionRun func);
int32 setSystemTime(char *dt);
void displayNowTime(void);
void displayLocalDateTime(void);
void displayLocalTimeToInter(void);
void displayLocalTime(void);

#endif

