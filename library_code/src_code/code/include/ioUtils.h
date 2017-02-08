/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#ifndef __IOUTILS_H__
#define __IOUTILS_H__

/* ------------------------ Include ------------------------- */
#include "common.h"


/* ------------------------ Defines ------------------------- */
#define ACCESSFD_AT_NEWFORK       1
#define NO_ACCESSFD_AT_NEWFORK    0  

#define IO_CTL_OK                 1
#define IO_CTL_NO                 0

typedef enum
{
  IO_NO_CTL = 0,
  IO_CTL_ACCESS,    /*访问方式*/
  IO_CTL_APPEND,    /*读写在尾部*/
  IO_CTL_NOBLOCK,   /*非阻塞*/
  IO_CTL_SYNC,      /*同步，等待写完成 数据和属性*/
  IO_CTL_DSYNC,     /*同步，等待写完成 只数据*/
  IO_CTL_RSYNC,     /*同步读写*/
  IO_CTL_FSYNC,     /*同步完成*/
  IO_CTL_ASYNC      /*异步*/
}ioCtl_t;        

#define   IO_ACCESS_READONLY      O_RDONLY
#define   IO_ACCESS_WRITEONLY     O_WRONLY 
#define   IO_ACCESS_READWRITE     O_RDWR

typedef enum
{
  IO_CTLOPT_CTL = 0,
  IO_CTLOPT_SET,       /*设置*/
  IO_CTLOPT_CLR,       /*清除*/
  IO_CTLOPT_RDONLY, 
  IO_CTLOPT_WRONLY,
  IO_CTLOPT_RDWR
}ioCtlOpt_t;                
/* ------------------------ Types --------------------------- */


/* ------------------------ Macros -------------------------- */


/* -------------- Global Function Prototypes ---------------- */

/*利用fcntl复制fd为一个大于等于targetFd可用描述符*/
int32 dupFdToNew(int32 fd, int32 targetFd);

/*判断该描述符所属的文件在新旧进程是否可以交差访问， ACCESSFD_AT_NEWFORK是可以*/
int32 isAccessFdByNewFork(int32 fd);

/***
fcntl(fd, F_SETFD, FD_CLOEXEC); // 这里设置为FD_CLOEXEC表示
当程序执行exec函数时本fd将被系统自动关闭,表示不传递给exec创建的新进程, 
如果设置为fcntl(fd, F_SETFD, 0);那么本fd将保持打开状态复制到exec创建的新进程中

can access flag : ACCESSFD_AT_NEWFORK
can't access flag : NO_ACCESSFD_AT_NEWFORK
***/
int32 setCloseFdAtNewFork(int32 fd, int32 flag);

/*
获取IO的访问方式

return value :
IO_ACCESS_READONLY      read only
IO_ACCESS_WRITEONLY     write only 
IO_ACCESS_READWRITE     read or write

*/
/*获取IO的状态 select : ioCtl_t type, return value : IO_CTL_OK or IO_CTL_NO*/
int32 getIoStatus(int32 fd, ioCtl_t select);

/*设置IO的状态*/
int32 SetIoCtl(int32 fd, ioCtl_t select, ioCtlOpt_t mode);

/*获取当前在文件描述词 fd上接收到SIGIO 或 SIGURG事件信号的进程或进程组标识 */
int32 getRevIoSigPid(int32 fd);

/*设置将要在文件描述词fd上接收SIGIO 或 SIGURG事件信号的进程或进程组标识*/
int32 setRevIoSigPid(int32 fd, uint32 pid);
#endif

