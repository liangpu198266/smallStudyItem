/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#ifndef __FILEUTILS_H__
#define __FILEUTILS_H__
#include "common.h"

/******file type********/
#define   MODE_FILE_READONLY      O_RDONLY
#define   MODE_FILE_WRITEONLY     O_WRONLY 
#define   MODE_FILE_READWRITE     O_RDWR
//select option
#define   MODE_FILE_CREAT         O_CREAT
#define   MODE_FILE_NBLOCK        O_NBLOCK
#define   MODE_FILE_SYNC          O_SYNC

int8 initFileDir(void);
int8 *getcurrentWorkPath(void);

/*读取或者写入文件*/
int32 saveDataToFile(int8 *dir, int8 *data, int32 len);
void deleteFile(int8 *dir);
int32 readDataFromFile(int8 *dir, int8 *buf, int32 len);

/*文件描述符的偏移*/
int32 getFdOffset(int32 fd);
int32 moveToRequestAtFile(int32 fd, int32 offset);
int32 moveToEndAtFile(int32 fd);
int32 moveToBeginAtFile(int32 fd);
int32 displayFileInfo(int8 *dir);

#endif

