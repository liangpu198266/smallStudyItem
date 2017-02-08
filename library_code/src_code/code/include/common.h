/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#ifndef __COMMON_H__
#define __COMMON_H__
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h> 
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/file.h>
#include <errno.h> 
#include <signal.h>
#include <assert.h>
#include <sys/mman.h>/*mmap*/

#include "comm_list.h"


/*****default type define*****/
#ifndef TRUE
#define TRUE            1
#endif 

#ifndef FALSE
#define FALSE           0
#endif 

#ifndef NULL
#define NULL 0
#endif

typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

/********size define**********/
#define     BUFSIZE           2048

#define     TASK_ERROR        -1
#define     TASK_OK           0



/*****************************/
#endif
