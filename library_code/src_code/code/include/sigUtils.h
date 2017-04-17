/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#ifndef __SIGUTILS_H__
#define __SIGUTILS_H__
#include "common.h"

typedef void (*sigFunction)(void);

int32 sigProcessCrashRegister(void);
int32 newTimer(int32 sig, int32 delayTime, sigFunction function);
int8 blockProcSig(sigset_t *set);
int8 unblockProcSig(sigset_t *set);
#endif
