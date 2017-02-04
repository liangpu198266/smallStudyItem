/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#ifndef __OBJECTPUBLIC_H__
#define __OBJECTPUBLIC_H__
#include "common.h"
#include "logprint.h"

/*****file part*****/
int32 openFile(objectNode_t *object,uint8 *path, int32 mode, int32 priv);
int32 closeFile(objectNode_t *object);
int32 readFile(objectNode_t *object, int8 *buf, uint32 leng);
int32 writeFile(objectNode_t *object, int8 *buf, uint32 leng);

/*****bus part*****/
int32 openBus(objectNode_t *object,uint8 *path, int32 mode, int32 priv);
int32 closeBus(objectNode_t *object);
int32 readBus(objectNode_t *object, int8 *buf, uint32 leng);
int32 writeBus(objectNode_t *object, int8 *buf, uint32 leng);

/*****dev part*****/
int32 openDev(objectNode_t *object,uint8 *path, int32 mode, int32 priv);
int32 closeDev(objectNode_t *object);
int32 readDev(objectNode_t *object, int8 *buf, uint32 leng);
int32 writeDev(objectNode_t *object, int8 *buf, uint32 leng);

/*****dev part*****/
int32 openSocket(objectNode_t *object,uint8 *path, int32 mode, int32 priv);
int32 closeSocket(objectNode_t *object);
int32 readSocket(objectNode_t *object, int8 *buf, uint32 leng);
int32 writeSocket(objectNode_t *object, int8 *buf, uint32 leng);

#endif

