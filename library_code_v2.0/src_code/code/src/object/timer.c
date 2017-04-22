/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */
#include "logprint.h"
#include "timer.h"

typedef struct netcmd_timer_ 
{
  struct list_head    node; 
  struct timeval      nextTime;          /* time for next running */
  void                *args;
  int32               (*timerHandle)(void *);
} netcmdTimer_t;

static pthread_mutex_t  gsTimerMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t  gsTimerListMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t  gsGetTimeMutex = PTHREAD_MUTEX_INITIALIZER; 

static pthread_t        gsTimerServerPthread;  
static pthread_cond_t   gsTimerServercond;  

static struct list_head gsTimerHead;

static void * timerServer(void * arg);

/***solve gettimeofday pthread safe****/
void gettimeofdayPthreadSafe(struct timeval *tv)    
{  
    pthread_mutex_lock(&gsGetTimeMutex);  
    gettimeofday(tv,NULL);  
    pthread_mutex_unlock(&gsGetTimeMutex);  
}  

int32 addnewTimer(int32 second, int32(*handle)(void *), void *args)
{
  struct timeval now;  
  netcmdTimer_t *node = NULL;
      
  if (second == 0 || handle == NULL)
  {
    LOG_ERROR("second || handle is zero\n");
    return TASK_ERROR;
  }

  gettimeofdayPthreadSafe(&now);  

  node = malloc(sizeof(netcmdTimer_t));
  if (node == NULL) 
  {
    LOG_ERROR("timer node malloc fail");
    return TASK_ERROR;  
  }

  node->timerHandle = handle;
  node->args = args;

  node->nextTime.tv_sec = now.tv_sec + second; 
  node->nextTime.tv_usec = now.tv_usec;

  pthread_mutex_lock(&gsTimerListMutex); 
  list_add(&node->node, &gsTimerHead);
  pthread_mutex_unlock(&gsTimerListMutex);
}

int32 netcmdTimerInit(void)
{
  LOG_DBG("timer thread to init\n");
  
  INIT_LIST_HEAD(&gsTimerHead);
  
  pthread_cond_init(&gsTimerServercond, NULL); 

  if (TASK_OK != pthread_create(&gsTimerServerPthread, NULL, timerServer, NULL)) 
  {  
    LOG_ERROR("create timer pthread fail\n");  
  } 
}

void netcmdTimerFini(void)
{
  netcmdTimer_t *node = NULL, *tmp = NULL;
  
  list_for_each_entry_safe(node, tmp, &gsTimerHead, node) 
  {
    list_del(&node->node);
    free(node);
    node = NULL;
  }
  
  pthread_cond_signal(&gsTimerServercond);  
  pthread_mutex_unlock(&gsTimerMutex);  
  pthread_join(gsTimerServerPthread, NULL); 

  LOG_DBG("timer thread to exit\n");  
}

static void timerServerDetect(struct timeval now)
{
  netcmdTimer_t *node = NULL, *tmp = NULL;
  int ret = 0;
  
  pthread_mutex_lock(&gsTimerListMutex); 
  
  list_for_each_entry_safe(node, tmp, &gsTimerHead, node) 
  {
    if (node->nextTime.tv_sec < now.tv_sec)
    {
      ret = node->timerHandle(node->args);
      if (ret < 0) 
      {
        LOG_ERROR("failed to run timer handle function");
      }

      list_del(&node->node);
      free(node);
      node = NULL;
    }
   
  }
  
  pthread_mutex_unlock(&gsTimerListMutex); 
}

static void * timerServer(void * arg)
{
  struct timeval now;  
  struct timespec outTime; 

  pthread_mutex_lock(&gsTimerMutex); 

  while (1) 
  {      
    gettimeofdayPthreadSafe(&now);  
    
    timerServerDetect(now);
        
    outTime.tv_sec = now.tv_sec + 1;  
    
    pthread_cond_timedwait(&gsTimerServercond, &gsTimerMutex, &outTime);  
  }  
  
  pthread_mutex_unlock(&gsTimerMutex); 
  
  LOG_DBG("timer cond thread exit\n"); 
}


