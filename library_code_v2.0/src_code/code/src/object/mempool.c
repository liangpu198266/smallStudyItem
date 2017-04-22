/*
 * Copyright (C) sunny liang
 * Copyright (C) mini net cmd
 */



/* ------------------------ Include ------------------------- */
#include "mempool.h"
#include "logprint.h"

/* ------------------------ Defines ------------------------- */

#define     MEMEPOOL_SMALL_SIZE     4096
#define     MEMEPOOL_MIDDLE_SIZE    4096*16
#define     MEMEPOOL_BIG_SIZE       512*1024

#define     MEMEPOOL_SMALL_MAX      50
#define     MEMEPOOL_MIDDLE_MAX     10
#define     MEMEPOOL_BIG_MAX        5
#define     MEMEPOOL_LARGE_MAX      1

typedef enum
{
  POOL_NO_USE = 0,
  POOL_USED,
  POOL_OVER
}memPoolUseStatus_t;

/* ------------------------ Types --------------------------- */
typedef struct poolTypeList_ 
{
  uint32              typeId;
  uint32              num; 
  uint32              size; 
} poolTypeList_t;

typedef struct mempoolNode_ 
{
  struct list_head    poolhead; 
  uint32              type;
  uint32              num; 
  uint32              status; 
} mempoolNode_t;

typedef struct memPoolsList_ 
{
  mempoolNode_t       smallPoolsList; 
  mempoolNode_t       middlePoolsList; 
  mempoolNode_t       bigPoolsList; 
  mempoolNode_t       largePoolsList;   
} memPoolsList_t;


/* ---------------- Static Global Variables ----------------- */
static poolTypeList_t poolTypeLists [] = 
{
  {MEMPOOL_TYPE_NO,       0,                        0                         },
  {MEMPOOL_TYPE_SMALL,    MEMEPOOL_SMALL_MAX,       MEMEPOOL_SMALL_SIZE       },
  {MEMEPOOL_MIDDLE_MAX,   MEMEPOOL_MIDDLE_MAX,      MEMEPOOL_MIDDLE_SIZE      },
  {MEMPOOL_TYPE_BIG,      MEMEPOOL_BIG_MAX,         MEMEPOOL_BIG_SIZE         },
  {MEMPOOL_TYPE_LARGE,    MEMEPOOL_LARGE_MAX,       0                         }
};

static pthread_mutex_t  gsMemPoolMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t  gsMemAreaMutex = PTHREAD_MUTEX_INITIALIZER;

static memPoolsList_t memPools;

/* ---------------- Static Global Variables ----------------- */


/* --------------- Local Function Prototypes ---------------- */
static memNode_t *mpNewMemNode(uint32 type)
{
  memNode_t *node =  NULL;

  node = malloc(sizeof(memNode_t));
  if (node == NULL)
  {
    LOG_ERROR("mempool malloc mem node malloc fail");
    return NULL;
  }

  switch (type)
  {
    case MEMPOOL_TYPE_SMALL:
      node->memAddr = malloc(MEMEPOOL_SMALL_SIZE);
      if (node->memAddr == NULL)
      {
        LOG_ERROR("mempool malloc mem small size malloc fail");
        return NULL;
      }
      break;

    case MEMPOOL_TYPE_MIDDLE:
      node->memAddr = malloc(MEMEPOOL_MIDDLE_SIZE);
      if (node->memAddr == NULL)
      {
        LOG_ERROR("mempool malloc mem middle size malloc fail");
        return NULL;
      }      
      break;

    case MEMPOOL_TYPE_BIG:
      node->memAddr = malloc(MEMEPOOL_BIG_SIZE);
      if (node->memAddr == NULL)
      {
        LOG_ERROR("mempool malloc mem big size malloc fail");
        return NULL;
      } 
      break;

    default:
      LOG_ERROR("mempool memaddr no malloc");
      return NULL;

  }
  
  node->mark = MEM_NO_USE;
  node->type = type;

  return node;
}

static int32 mpListInit(uint32 nums, uint32 type, mempoolNode_t *poolList)
{
  int32 i = 0;
  memNode_t *node =  NULL;
  
  poolList->type = type;    
  poolList->num = nums;
  poolList->status = POOL_NO_USE;
  
  for (i = 0; i < nums; i++)
  {
    node = mpNewMemNode(type);

    if (node != NULL)
    {
      node->realLength = 0;
      pthread_mutex_lock(&gsMemPoolMutex); 
      list_add(&node->memNode, &poolList->poolhead);
      pthread_mutex_unlock(&gsMemPoolMutex);
    }
  }
}

static void mpFreeOneList(mempoolNode_t *poolList)
{
  memNode_t *node = NULL, *tmp = NULL;
  
  list_for_each_entry_safe(node, tmp, &poolList->poolhead, memNode) 
  {
    list_del(&node->memNode);
    free(node->memAddr);
    node->memAddr = NULL;
    free(node);
    node = NULL;
  }

  poolList->num = 0;
  poolList->type = 0;
}

static memNode_t *mpNewLargeMemNode(uint32 length)
{
  memNode_t *node =  NULL;

  node = malloc(sizeof(memNode_t));
  if (node == NULL)
  {
    LOG_ERROR("mempool malloc mem node malloc fail");
    return NULL;
  }

  node->memAddr = malloc(MEMEPOOL_SMALL_SIZE);
  if (node->memAddr == NULL)
  {
    LOG_ERROR("mempool malloc mem large size malloc fail");
    return NULL;
  }
  
  memset(node->memAddr, 0x0, poolTypeLists[node->type].size);  
  node->mark = MEM_NO_USE;
  node->type = MEMPOOL_TYPE_LARGE;

  return node;
}

static int32 mpLargeListInit(void)
{
  memPools.largePoolsList.num = MEMEPOOL_LARGE_MAX;
  memPools.largePoolsList.status = POOL_NO_USE;
  memPools.largePoolsList.type = MEMPOOL_TYPE_LARGE;
}

static memNode_t *mpCheckOneNode(mempoolNode_t *poolList)
{
  memNode_t *node = NULL, *tmp = NULL;
  
  list_for_each_entry_safe(node, tmp, &poolList->poolhead, memNode) 
  {
    if (node->mark == MEM_NO_USE)
    {
      return node;
    }
  }

  return NULL;
}

static memNode_t *mpCheckNodeByType(uint32 type)
{
  switch (type)
  {
    case MEMPOOL_TYPE_SMALL:
      return mpCheckOneNode(&memPools.smallPoolsList);

    case MEMPOOL_TYPE_MIDDLE:
      return mpCheckOneNode(&memPools.middlePoolsList);

    case MEMPOOL_TYPE_BIG:
      return mpCheckOneNode(&memPools.bigPoolsList);

    case MEMPOOL_TYPE_LARGE:
      return mpCheckOneNode(&memPools.largePoolsList);

    default:
      LOG_ERROR("mp Check Node fail");
      return NULL;  
  }

  return NULL;
}

static uint32 mpGetPoolTypeBylen(uint32 length)
{
  if (length >= MEMEPOOL_BIG_SIZE)
  {
    return MEMPOOL_TYPE_LARGE;
  }
  else if (length >= MEMEPOOL_MIDDLE_SIZE)
  {
    return MEMPOOL_TYPE_BIG;
  }
  else if (length >= MEMEPOOL_SMALL_SIZE)
  {
    return MEMPOOL_TYPE_MIDDLE;
  }
  else
  {
    return MEMPOOL_TYPE_SMALL;
  }

  return MEMPOOL_TYPE_NO;
}

static memNode_t *mpNewOneNodeByType(uint32 type, uint32 length, mempoolNode_t *poolList)
{
  memNode_t *node = NULL;

  if (type == MEMPOOL_TYPE_LARGE)
  {
    node = malloc(sizeof(memNode_t));
    if (node == NULL)
    {
      LOG_ERROR("mempool malloc mem node malloc fail at large");
      return NULL;
    }

    memset(node, 0x0, sizeof(memNode_t));
    
    node->memAddr = malloc(length);
    if (node->memAddr == NULL)
    {
      LOG_ERROR("mempool malloc mem large size malloc fail");
      return NULL;
    } 

    node->type = type;
  }
  else
  {
    node = mpNewMemNode(type);
    if (node == NULL)
    {
      LOG_ERROR("new malloc Node fail");
    }
  }
  
  node->realLength = length;
  node->mark = MEM_USED;
  pthread_mutex_lock(&gsMemPoolMutex); 
  list_add(&node->memNode, &poolList->poolhead);
  pthread_mutex_unlock(&gsMemPoolMutex);
  return node;
}

static memNode_t *mpNewNodeByType(uint32 type, uint32 length)
{ 
  switch (type)
  {
    case MEMPOOL_TYPE_SMALL:
      return mpNewOneNodeByType(type, length, &memPools.smallPoolsList);          
      
    case MEMPOOL_TYPE_MIDDLE:
      return mpNewOneNodeByType(type, length, &memPools.middlePoolsList);

    case MEMPOOL_TYPE_BIG:
      return mpNewOneNodeByType(type, length, &memPools.bigPoolsList);

    case MEMPOOL_TYPE_LARGE:
      return mpNewOneNodeByType(type, length, &memPools.largePoolsList);

    default:
      LOG_ERROR("mp Check Node fail");
      return NULL;  
  }

  return NULL;
}

static void mpSetPoolMemNums(mempoolNode_t *poolList)
{
  memNode_t *node = NULL, *tmp = NULL;
  uint32 count = 0;
  
  list_for_each_entry_safe(node, tmp, &poolList->poolhead, memNode) 
  {
    if (node->mark == MEM_USED)
    {
      count ++;
    }
  }      

  poolList->num = count;

  pthread_mutex_lock(&gsMemPoolMutex);
  
  if (poolList->num > poolTypeLists[poolList->type].num)
  {
    poolList->status = POOL_OVER;
  }
  else if (poolList->num == 0)
  {
    poolList->status = POOL_NO_USE;
  }
  else
  {
    poolList->status = POOL_USED;
  }
  
  pthread_mutex_unlock(&gsMemPoolMutex);
}

static int32 mpSetPoolStatus(uint32 type)
{
  switch (type)
  {
    case MEMPOOL_TYPE_SMALL:
      mpSetPoolMemNums(&memPools.smallPoolsList);
      return TASK_OK;
      
    case MEMPOOL_TYPE_MIDDLE:
      mpSetPoolMemNums(&memPools.middlePoolsList);
      return TASK_OK;

    case MEMPOOL_TYPE_BIG:
       mpSetPoolMemNums(&memPools.bigPoolsList);
      return TASK_OK;

    case MEMPOOL_TYPE_LARGE:
      mpSetPoolMemNums(&memPools.largePoolsList);
      return TASK_OK;

    default:
      LOG_ERROR("mp Check Node fail");
      return TASK_ERROR;  
  }

  return TASK_ERROR;

}

static void mpFreeOneNode(memNode_t *freeNode,mempoolNode_t *poolList)
{
  memNode_t *node = NULL, *tmp = NULL;
  
  list_for_each_entry_safe(node, tmp, &poolList->poolhead, memNode) 
  {
    if (freeNode == node)
    {
      if (poolList->status == POOL_OVER)
      {
        pthread_mutex_lock(&gsMemPoolMutex);
        list_del(&node->memNode);
        pthread_mutex_unlock(&gsMemPoolMutex);
        
        LOG_DBG("mp free Node over type is %d, length is %d", node->type, node->realLength);
        
        if (node->memAddr)
        {
          free(node->memAddr);
          node->memAddr = NULL;
          free(node);
          node = NULL;

          mpSetPoolMemNums(poolList);
        }
        else
        {
          LOG_ERROR("node->memAddr is empty");
        } 
      }
      else
      {
        LOG_DBG("mp free normal Node type is %d, length is %d", node->type, node->realLength);
        pthread_mutex_lock(&gsMemPoolMutex);
        node->realLength = 0;
        node->mark = MEM_NO_USE;
        memset(node->memAddr, 0x0, poolTypeLists[node->type].size);
        pthread_mutex_unlock(&gsMemPoolMutex);
      }
    }
  }   


}

/* ------------------ Global Functions ---------------------- */
/*
 * malloc mem poolnode
 */
memNode_t *mpMalloc(uint32 length)
{
  uint32 type = 0;
  memNode_t *node = NULL;
  int32 ret = 0;

  type = mpGetPoolTypeBylen(length);

  if ((node = mpCheckNodeByType(type)) == NULL)
  {
    node = mpNewNodeByType(type, length);
    if (node == NULL)
    {
      LOG_ERROR("mp mpNewNodeByType fail");
      return NULL;  
    }

    LOG_DBG("mp new Node type is %d, length is %d", node->type, node->realLength);
  }
  else
  {
    node->realLength = length;
    node->mark = MEM_USED;
    LOG_DBG("mp old Node type is %d, length is %d", node->type, node->realLength);
  }

  ret = mpSetPoolStatus(type);
  if (ret == TASK_ERROR)
  {
    LOG_ERROR("mp mpSetPoolStatus fail");
    return NULL;
  }

  return node;
}

/*
 * free mem poolnode
 */
int32 mpFree(memNode_t *freeNode)
{
  switch (freeNode->type)
  {
    case MEMPOOL_TYPE_SMALL:
      mpFreeOneNode(freeNode, &memPools.smallPoolsList);
      return TASK_OK;
      
    case MEMPOOL_TYPE_MIDDLE:
      mpFreeOneNode(freeNode, &memPools.middlePoolsList);
      return TASK_OK;

    case MEMPOOL_TYPE_BIG:
      mpFreeOneNode(freeNode, &memPools.bigPoolsList);
      return TASK_OK;

    case MEMPOOL_TYPE_LARGE:
      mpFreeOneNode(freeNode, &memPools.largePoolsList);
      return TASK_OK;

    default:
      LOG_ERROR("mp free fail");
      return TASK_ERROR;  
  }

  return TASK_ERROR;
}

/*
 * init mem pool
 */
int32 mempoolInit(void)
{
  LOG_DBG("netcmd mem pool init\n");

  memset(&memPools, 0x0, sizeof(memPoolsList_t));
  
  INIT_LIST_HEAD(&memPools.smallPoolsList.poolhead);
  INIT_LIST_HEAD(&memPools.middlePoolsList.poolhead);
  INIT_LIST_HEAD(&memPools.bigPoolsList.poolhead);
  INIT_LIST_HEAD(&memPools.largePoolsList.poolhead);  

  mpListInit(MEMEPOOL_SMALL_MAX, MEMPOOL_TYPE_SMALL, &memPools.smallPoolsList);
  mpListInit(MEMEPOOL_MIDDLE_MAX, MEMPOOL_TYPE_MIDDLE, &memPools.middlePoolsList);
  mpListInit(MEMEPOOL_BIG_MAX, MEMPOOL_TYPE_BIG, &memPools.bigPoolsList);
  mpLargeListInit();
}

void mempoolFree(void)
{
  mpFreeOneList(&memPools.smallPoolsList);
  mpFreeOneList(&memPools.middlePoolsList);
  mpFreeOneList(&memPools.bigPoolsList);
  mpFreeOneList(&memPools.largePoolsList);
}

