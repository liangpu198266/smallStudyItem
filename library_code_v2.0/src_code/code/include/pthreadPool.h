/*
 * Copyright (C) sunny liang
 * Copyright (C) mini net cmd
 */

#ifndef __PTHREADPOOL_H__
#define __PTHREADPOOL_H__

#include "common.h"

typedef void (*process)(void *arg);

int8 poolAddJob(process function, void *args);
void pthreadPoolsInit(void);
void pthreadPoolsFini(void);


#endif
