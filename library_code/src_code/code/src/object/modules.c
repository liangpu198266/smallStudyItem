/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#include "modules.h"
#include "object.h"
#include "logprint.h"
typedef struct _modulesName
{
  int8 name[100];
} modulesName_t;

static const modulesName_t modulesName[MODULES_MAX] = 
{
  {"files"   },
  {"buses"   },
  {"devices" },
  {"socket"  }
};


static modules_t modulesList;

static int32 initmodulesList(void)
{
  LOG_DBG("init modules list");
  
  memset(&modulesList, 0x0, sizeof(modules_t));
  
  INIT_LIST_HEAD(&modulesList.listHead);
}

static modulesNode_t  *addNewModulesNode(int32 type)
{
  modulesNode_t  *node = NULL;

  node = malloc(sizeof(modulesNode_t));
  if (node == NULL)
  {
    displayErrorMsg("malloc mem error\n");
    return NULL;
  }

  memset(node, 0x0, sizeof(modulesNode_t));
  node->moduleName = modulesName[type].name;
  node->moduleType = type;

  return node;  
}

int32 registerModules(void)
{
  int32 i;
  modulesNode_t  *node = NULL;
  
  initmodulesList();
  
  for (i = 0; i < MODULES_MAX; i++)
  {
    node = addNewModulesNode(i);
    if (node == NULL)
    {
      displayErrorMsg("add new modules node error\n");
      return TASK_ERROR;
    }

    LOG_DBG("add modules : %s", node->moduleName);
    list_add(&node->listNode, &modulesList.listHead);

    INIT_LIST_HEAD(&node->objectList);
    
    modulesList.counter++;
  }

  return TASK_OK;
}

void unRegisterModules(void)
{
  modulesNode_t *node = NULL, *tmp = NULL;

  list_for_each_entry_safe(node, tmp, &modulesList.listHead, listNode) 
  {
    list_del(&node->listNode);
    node->moduleName = NULL;
    free(node);
    node = NULL;
  }
}

int32 addObjToModules(objectNode_t  *object)
{
  modulesNode_t *node = NULL, *tmp = NULL;

  list_for_each_entry_safe(node, tmp, &modulesList.listHead, listNode) 
  {
    if (node->moduleType == object->type)
    {
      list_add(&object->objectNode, &node->objectList);
      node->objectNum ++;
      LOG_DBG("add new obj to modules: %s, num is %d", node->moduleName, node->objectNum);
      return TASK_OK;
    }
  }

  return TASK_ERROR;
}

int32 deleteObjFromModules(objectNode_t  *object)
{
  modulesNode_t *node = NULL, *tmp = NULL;

  list_for_each_entry_safe(node, tmp, &modulesList.listHead, listNode) 
  {
    if (node->moduleType == object->type)
    {
      node->objectNum--;
      LOG_DBG("delete obj from modules: %s, num is : %d", node->moduleName, node->objectNum);
      return TASK_OK;
    }
  }

  return TASK_ERROR;
}