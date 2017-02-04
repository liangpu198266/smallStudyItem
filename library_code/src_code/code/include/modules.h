/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#ifndef __MODULES_H__
#define __MODULES_H__
#include "common.h"

typedef enum
{
  MODULES_FILE = 0,
  MODULES_BUS,
  MODULES_DEV,
  MODULES_SOCKET,
  MODULES_MAX
}modulesType_t;

typedef struct
{
  struct list_head  listNode;
  uint8             *moduleName;
  int32             moduleType;
  struct list_head  objectList;
  int32             objectNum;
} modulesNode_t;

typedef struct
{
  struct list_head  listHead;
  int32             counter;
} modules_t;

int32 registerModules(void);
void unRegisterModules(void);
#endif

