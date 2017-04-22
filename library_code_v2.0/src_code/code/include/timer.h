/*
 * Copyright (C) sunny liang
 * Copyright (C) mini net cmd
 */

#ifndef __TIMER_H__
#define __TIMER_H__

#include "common.h"

void gettimeofdayPthreadSafe(struct timeval *tv);
int32 addnewTimerNode(int32 second, int32(*handle)(void *), void *args);
int32 netcmdTimerInit(void);
void netcmdTimerFini(void);
#endif
