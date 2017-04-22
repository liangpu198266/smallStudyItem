/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#ifndef __MEMPOOL_H__
#define __MEMPOOL_H__

#include "common.h"

typedef enum
{
  MEMPOOL_TYPE_NO = 0,
  MEMPOOL_TYPE_SMALL,
  MEMPOOL_TYPE_MIDDLE,
  MEMPOOL_TYPE_BIG,
  MEMPOOL_TYPE_LARGE
}memPoolType_t;

typedef enum
{
  MEM_NO_USE = 0,
  MEM_USED
}memNodeUseMark_t;


/***one data define***/
typedef struct memNode_ 
{
  struct list_head    memNode; 
  uint32              mark;
  uint32              type;  
  uint8               *memAddr;
  uint32              realLength;
}memNode_t;

int32 mempoolInit(void);
void mempoolFree(void);
int32 mpFree(memNode_t *freeNode);
memNode_t *mpMalloc(uint32 length);

#endif

