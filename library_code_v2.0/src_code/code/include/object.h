/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#ifndef __OBJECT_H__
#define __OBJECT_H__
#include "common.h"
/**********************mode Type**************/
/******file type********/
#define   MODE_FILE_READONLY      O_RDONLY
#define   MODE_FILE_WRITEONLY     O_WRONLY 
#define   MODE_FILE_READWRITE     O_RDWR
//select option
#define   MODE_FILE_CREAT         O_CREAT
#define   MODE_FILE_NBLOCK        O_NBLOCK
#define   MODE_FILE_SYNC          O_SYNC

/******socket type******/
#define   MODE_SOCKET_RAW     SOCK_RAW  
#define   MODE_SOCKET_TCP     SOCK_STREAM
#define   MODE_SOCKET_UDP     SOCK_DGRAM
/*********************************************/
typedef struct _objectOps objectOps_t;  

typedef struct objectNode
{
  struct list_head  objectNode;
  int32             fd;
  int32             type;
  objectOps_t       *fOps;
  struct list_head  memListhead;
} objectNode_t;

typedef int32 (*objectOpen)(objectNode_t *node, uint8 *dir, int32 mode, int32 priv);
typedef int32 (*objectClose)(objectNode_t *node);
typedef int32 (*objectRead)(objectNode_t *node, int8 *buf, uint32 len);
typedef int32 (*objectWrite)(objectNode_t *node, int8 *buf, uint32 len);

struct _objectOps 
{
  objectOpen    open;
  objectClose   close;
  objectRead    read;
  objectWrite   write;
};

objectNode_t *newObject(int32 type);
int32 deleteObject(objectNode_t *object);
int32 deleteObjFromModules(objectNode_t  *object);
int32 addObjToModules(objectNode_t  *object);

#endif

