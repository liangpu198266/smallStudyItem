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
  IO_CTL_ACCESS,    /*���ʷ�ʽ*/
  IO_CTL_APPEND,    /*��д��β��*/
  IO_CTL_NOBLOCK,   /*������*/
  IO_CTL_SYNC,      /*ͬ�����ȴ�д��� ���ݺ�����*/
  IO_CTL_DSYNC,     /*ͬ�����ȴ�д��� ֻ����*/
  IO_CTL_RSYNC,     /*ͬ����д*/
  IO_CTL_FSYNC,     /*ͬ�����*/
  IO_CTL_ASYNC      /*�첽*/
}ioCtl_t;        

#define   IO_ACCESS_READONLY      O_RDONLY
#define   IO_ACCESS_WRITEONLY     O_WRONLY 
#define   IO_ACCESS_READWRITE     O_RDWR

typedef enum
{
  IO_CTLOPT_CTL = 0,
  IO_CTLOPT_SET,       /*����*/
  IO_CTLOPT_CLR,       /*���*/
  IO_CTLOPT_RDONLY, 
  IO_CTLOPT_WRONLY,
  IO_CTLOPT_RDWR
}ioCtlOpt_t;                
/* ------------------------ Types --------------------------- */


/* ------------------------ Macros -------------------------- */


/* -------------- Global Function Prototypes ---------------- */

/*����fcntl����fdΪһ�����ڵ���targetFd����������*/
int32 dupFdToNew(int32 fd, int32 targetFd);

/*�жϸ��������������ļ����¾ɽ����Ƿ���Խ�����ʣ� ACCESSFD_AT_NEWFORK�ǿ���*/
int32 isAccessFdByNewFork(int32 fd);

/***
fcntl(fd, F_SETFD, FD_CLOEXEC); // ��������ΪFD_CLOEXEC��ʾ
������ִ��exec����ʱ��fd����ϵͳ�Զ��ر�,��ʾ�����ݸ�exec�������½���, 
�������Ϊfcntl(fd, F_SETFD, 0);��ô��fd�����ִ�״̬���Ƶ�exec�������½�����

can access flag : ACCESSFD_AT_NEWFORK
can't access flag : NO_ACCESSFD_AT_NEWFORK
***/
int32 setCloseFdAtNewFork(int32 fd, int32 flag);

/*
��ȡIO�ķ��ʷ�ʽ

return value :
IO_ACCESS_READONLY      read only
IO_ACCESS_WRITEONLY     write only 
IO_ACCESS_READWRITE     read or write

*/
/*��ȡIO��״̬ select : ioCtl_t type, return value : IO_CTL_OK or IO_CTL_NO*/
int32 getIoStatus(int32 fd, ioCtl_t select);

/*����IO��״̬*/
int32 SetIoCtl(int32 fd, ioCtl_t select, ioCtlOpt_t mode);

/*��ȡ��ǰ���ļ������� fd�Ͻ��յ�SIGIO �� SIGURG�¼��źŵĽ��̻�������ʶ */
int32 getRevIoSigPid(int32 fd);

/*���ý�Ҫ���ļ�������fd�Ͻ���SIGIO �� SIGURG�¼��źŵĽ��̻�������ʶ*/
int32 setRevIoSigPid(int32 fd, uint32 pid);
#endif

