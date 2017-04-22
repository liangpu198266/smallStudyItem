/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#ifndef __IPCUTILS_H__
#define __IPCUTILS_H__

#include "common.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>

/* ------------------------ Include ------------------------- */



/* ------------------------ Defines ------------------------- */



/* ------------------------ Types --------------------------- */



/* ------------------------ Macros -------------------------- */


/* ---------------- XXX:   Global Variables ----------------- */


/* -------------- Global Function Prototypes ---------------- */

typedef int32 (*shmFunctionRun)(int8 *data, int32 dataLen);

int32 ipcSendbyMsg(int8 key, int8 *data, int32 len, int32 type);
int32 ipcRevbyMsg(int8 key, int8 *data, int32 *len, int32 revType);
int32 ipcSendbyFifo(int8 *dir, int8 *data, int32 len, int32 type);
int32 ipcRevbyFifo(int8 *dir, int8 *data, int32 *len, int32 revType);
int32 ipcMapMem(int8 *dir, int32 len, shmFunctionRun shmFunctionRun);
int32 ipcShmMem(int32 key, int32 len, shmFunctionRun shmFunctionRun);

#endif

