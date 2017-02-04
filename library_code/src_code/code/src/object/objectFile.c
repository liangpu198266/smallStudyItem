/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 * file part
 */
#include "object.h"
#include "objectPublic.h"

/***********************************/
int32 openFile(objectNode_t *object,uint8 *path, int32 mode, int32 priv)
{
  int32 fd = 0;

  LOG_DBG("open file : %s mode is %d", path, mode);
  fd = open(path,mode,priv);
  if(fd < 0)
  {
    displayErrorMsg("open file error\n");
    return TASK_ERROR;
  }

  object->fd = fd;
  return fd;
}

int32 closeFile(objectNode_t *object)
{
  close(object->fd);
  return TASK_OK;
}

int32 readFile(objectNode_t *object, int8 *buf, uint32 leng)
{
  int32 rv = 0;
  
  rv = read(object->fd, buf,leng);
  if(rv < 0)
  {
    displayErrorMsg("read file error\n");
    return TASK_ERROR;
  }

  return rv;
}

int32 writeFile(objectNode_t *object, int8 *buf, uint32 leng)
{
  int32 size = 0;

  LOG_DBG("buf is %s len is %d\n", buf, leng);
  size = write(object->fd, buf, leng);
  if(size < 0)  
  {
    displayErrorMsg("write file error\n");
    return TASK_ERROR;
  }

  return size;
}

