/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#include "modules.h"
#include "object.h"
#include "objectPublic.h"


typedef struct
{
  struct list_head  memNode;
  int32             memsize;
  void             *memStart;
} memNode_t;

/******ops part******/
static objectOps_t gsObjectOp[MODULES_MAX] = 
{
  {&openFile, &closeFile, &readFile, &writeFile},
  {&openBus, &closeBus, &readBus, &writeBus},
  {&openDev, &closeDev, &readDev, &writeDev},
  {&openSocket, &closeSocket, &readSocket, &writeSocket},
};

/*************************/
static void memFreeFromList(objectNode_t  *memListhead)
{
  memNode_t *node = NULL, *tmp = NULL;

  PARAMETER_NULL_CHECK(memListhead);
  
  list_for_each_entry_safe(node, tmp, &memListhead->memListhead, memNode) 
  {
    if (node->memStart != NULL)
    {
      free(node->memStart);
      node->memStart = NULL;
    }

    node->memsize = 0;
    list_del(&node->memNode);

    free(node);
    node = NULL;
  }
}

objectNode_t *newObject(int32 type)
{
  objectNode_t *object = NULL;
  int32 ret = 0;

  object = malloc(sizeof(objectNode_t));
  if (object == NULL)
  {
    displayErrorMsg("malloc mem error\n");
    return NULL;
  }

  object->type = type;
  
  ret = addObjToModules(object);
  if(ret == TASK_ERROR)
  {
    displayErrorMsg("add object to modules error");
    return NULL;
  }

  object->fOps = &gsObjectOp[type];
  
  return object;
}

int32 deleteObject(objectNode_t *object)
{
  int ret = 0;
  
  if (object == NULL)
  {
    displayErrorMsg("no object exist");
    return TASK_ERROR;
  }

  ret = deleteObjFromModules(object);
  if(ret == TASK_ERROR)
  {
    displayErrorMsg("delete object from modules error");
    return TASK_ERROR;
  }

  memFreeFromList(object);
  
  object->fOps = NULL;
  list_del(&object->objectNode);
  free(object);
  
  return TASK_OK;
}

