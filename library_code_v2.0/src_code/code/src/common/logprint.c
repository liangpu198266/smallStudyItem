/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#include "logprint.h"
#include "fileUtils.h"

extern int32 errno;

#if defined DEBUG_TO_SYSLOG

int8 initFileLogSystem(void)
{
  openlog("debugLog", LOG_CONS | LOG_PID, 0);

  return TASK_OK;
}

void finiFileLogSystem(void)
{
  closelog();
}

#elif defined DEBUG_TO_SCREEN 

int8 initFileLogSystem(void)
{
  return TASK_OK;
}

void finiFileLogSystem(void)
{

}

#elif defined DEBUG_TO_FILE 

#include <stdarg.h> 

#define LOG_BUF_SIZE  (1024 * 4) 
#define LOG_BUF_SIZE1 (1024 * 4 - 128)

static int8 logWorkPath[BUFSIZE];

static FILE *log_fp = NULL;

static pthread_mutex_t gsLogMutex = PTHREAD_MUTEX_INITIALIZER;

int8 initFileLogSystem(void)
{
  memset(logWorkPath, 0x0, BUFSIZE);
  
  sprintf(logWorkPath, "%s/log", getcurrentWorkPath());
  
  log_fp = fopen(logWorkPath, "a+");
  if(!log_fp) 
  {
    fprintf(stderr,"[log] failed open log error id %d, content: %s \n",errno, strerror(errno));
    return TASK_ERROR; 
  }
  
  return TASK_OK;
}

void finiFileLogSystem(void)
{

  fclose(log_fp);
  log_fp = NULL;
}


int32 logPrintf(int prio, const char *func, const char *fmt, ...)
{  
  va_list ap;  
  char buf1[LOG_BUF_SIZE1], buf[LOG_BUF_SIZE], tm[256] = {0};   
  time_t timep;
  int32 fd = -1;
  
  memset(buf, 0x0, LOG_BUF_SIZE);
  memset(buf1, 0x0, LOG_BUF_SIZE1);
  memset(tm, 0x0, 256);
  
  va_start(ap, fmt);  
  vsnprintf(buf1, LOG_BUF_SIZE1, fmt, ap);  
  va_end(ap); 
  
  time(&timep);
  strcpy(tm,  asctime(localtime(&timep)));
  tm[strlen(tm)-1] = 0;
  
  switch (prio) 
  {
    case DEBUG_ERROR:
          snprintf(buf, LOG_BUF_SIZE, "[%s][ERROR][%s] Line%d: %s\n", tm, func, __LINE__, buf1);
          break;
    case DEBUG_WARNING:
          snprintf(buf, LOG_BUF_SIZE, "[%s][WARN][%s] Line%d: %s\n", tm, func, __LINE__, buf1);
          break;
    case DEBUG_INFO:
          snprintf(buf, LOG_BUF_SIZE, "[%s][INFO][%s] Line%d: %s\n", tm, func, __LINE__, buf1);
          break;
    default:
          snprintf(buf, LOG_BUF_SIZE, "[%s][%s] Line%d: %s\n", tm, func,__LINE__, buf1);
          break;
  }
  
  if(log_fp)
  {
    pthread_mutex_lock(&gsLogMutex);

    fd = fileno(log_fp);
    if(flock(fd,LOCK_EX) == 0)
    {
      fwrite(buf, 1, strlen(buf), log_fp);
      fflush(log_fp);
      if(flock(fd,LOCK_UN)!=0)
      {
        fprintf(stderr,"[log] unlock failed id %d : %s\n",errno, strerror(errno));
      }
    }
    else
    {
      fprintf(stderr,"[log] lock failed id %d : %s \n",errno, strerror(errno));
    }

    fd = -1;
    
    pthread_mutex_unlock(&gsLogMutex);
  } else 
  {
    printf( "%s", buf);
  }
      
  return 0;
}

#else

int8 initFileLogSystem(void)
{
  return TASK_OK;
}

void finiFileLogSystem(void)
{

}

#endif



