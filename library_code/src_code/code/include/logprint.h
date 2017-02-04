/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#ifndef __LOGPRINT_H__
#define __LOGPRINT_H__

#include "common.h"



/********print debug**********/

#if defined DEBUG_TO_SYSLOG

#define LOG_ASSERT(exp)                  do{ assert(exp); } while(0)
#define LOG_DBG(args...)                 syslog(LOG_INFO, args)
#define LOG_WARN(args...)                syslog(LOG_WARNING, args)
#define LOG_ERROR(args...)               syslog(LOG_ERR, args)

#elif defined DEBUG_TO_SCREEN 

#define LOG_ASSERT(exp)                  do{ assert(exp); } while(0)
#define LOG_DBG(format, arg...)          do{ fprintf(stderr, "[INFO] %s@Line%d : " format "\n", __FUNCTION__, __LINE__,  ## arg); fflush(stderr); } while(0)
#define LOG_WARN(format, arg...)         do{ fprintf(stderr, "[WARN] %s@Line%d : " format "\n", __FUNCTION__, __LINE__,  ## arg); fflush(stderr); } while(0)
#define LOG_ERROR(format, arg...)        do{ fprintf(stderr, "[ERROR] %s@Line%d : " format "\n", __FUNCTION__, __LINE__,  ## arg); fflush(stderr);} while(0)

#elif defined DEBUG_TO_FILE 

#define DEBUG_ERROR   1
#define DEBUG_WARNING 2
#define DEBUG_INFO    3

#define LOG_ASSERT(exp)                  do{ assert(exp); } while(0)
#define LOG_DBG(fmt...)                  logPrintf(DEBUG_INFO, __func__, fmt) 
#define LOG_WARN(fmt...)                 logPrintf(DEBUG_WARNING, __func__, fmt) 
#define LOG_ERROR(fmt...)                logPrintf(DEBUG_ERROR, __func__, fmt) 

#else

#define LOG_ASSERT(exp)                  do{ (void) 0; } while(0)
#define LOG_DBG(format, arg...)          do{ (void) 0; } while(0)
#define LOG_WARN(format, arg...)         do{ (void) 0; } while(0)
#define LOG_ERROR(format, arg...)        do{ fprintf(stderr, "[ERROR]: " format "\n", ## arg); fflush(stderr); } while(0)

#endif

#define PARAMETER_NULL_CHECK(exp)                          \
  do {                                                     \
      LOG_ASSERT(exp != NULL);                              \
     } while (0)


extern int8 initFileLogSystem(void);
extern void finiFileLogSystem(void);
void displayErrorMsg(int8 *msg);

#endif

