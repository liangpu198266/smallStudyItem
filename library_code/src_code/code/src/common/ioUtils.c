/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */
#include "fileUtils.h"
#include "ioUtils.h"
#include "logprint.h"

/* ------------------------ Include ------------------------- */


/* ------------------------ Defines ------------------------- */
//IO cmd 
/*fcntl 5�ֹ���*/
/*
1.����һ�����е���������cmd=F_DUPFD��.
2.��ã������ļ����������(cmd=F_GETFD��F_SETFD).
3.��ã������ļ�״̬���(cmd=F_GETFL��F_SETFL).
4.��ã������첽I/O����Ȩ(cmd=F_GETOWN��F_SETOWN).
5.��ã����ü�¼��(cmd=F_GETLK,F_SETLK��F_SETLKW).
*/

/*
F_DUPFD      :
��1����С�Ĵ��ڻ����arg��һ�����õ�������                       
��2����ԭʼ������һ����ĳ���������
��3������������ļ�(file)�Ļ�,����һ���µ�������,
     �����������arg������ͬ��ƫ����(offset)
��4����ͬ�ķ���ģʽ(��,д���/д)
��5����ͬ���ļ�״̬��־(��:�����ļ�������������ͬ��״̬��־)        
��6�����µ��ļ������������һ���close-on-exec��־�����óɽ���ʽ����
     execve(2)��ϵͳ����
*/
#define   IO_CTL_DUPFD        F_DUPFD 
/***
ÿ���ļ�����������һ��close-on-exec��־��
��ϵͳĬ������£������־���һλ������Ϊ0��
��FD_CLOEXEC�����������ļ���close-on-exec��
FD_CLOEXEC��ֵΪ1������������close-on-execλʱ����ʹ��FD_CLOEXEC
Ҳ����ֱ�ӵ�ʹ��0��1���ֶ�close-on-execλ�������á�
����close-on-exec��־��Ϊ1ʱ���������˱�־��
��ô���ӽ��̵���exec����֮ǰ��ϵͳ���Ѿ����ӽ��̽����ļ��������ر��ˡ�
��close-on-exec��־��Ϊ0ʱ�����ر��˴˱�־����ô���ӽ��̵���exec������
�ӽ��̽�����رո��ļ�����������ʱ�����ӽ��̽�������ļ���
���Ǿ���ͬһ���ļ����Ҳ������ͬһ���ļ�ƫ�����ȡ�

 ����close-on-exec :
 ÿ���ļ�����������һ��close-on-exec��־��Ĭ������£������־���һλ������Ϊ 0��
 �����־���ľ����������ڵ������������̵���exec�����庯��ʱ���ڵ���exec����֮ǰΪexec�庯���ͷŶ�Ӧ���ļ���������

 ��һ��ժ��:
 Linux��socketҲ���ļ���������һ�֣���B fork����C��ʱ��
 CҲ��̳�B��11111�˿�socket�ļ�����������B���˵�ʱ��C�ͻ�ռ�����Ȩ��
�������ڵ����������fork��ʱ��ر�B�Ѿ��򿪵��ļ���������
ϵͳ�����Ľ�������ǣ�close_on_exec���������̴��ļ�ʱ��
ֻ��ҪӦ�ó�������FD_CLOSEXEC��־λ����fork��exec���������ʱ��
�ں��Զ��Ὣ��̳еĸ�����FD�رա�

F_GETFD ȡ�����ļ�������fd���ϵ�close-on-exec��־������FD_CLOEXEC��
�������ֵ��FD_CLOEXEC��������������0�Ļ����ļ����ֽ���ʽ����exec(),
�������ͨ��exec���еĻ����ļ������رա�

F_SETFD ����close-on-exec��־��
fcntl(fd,F_SETFD,FD_CLOEXEC)��
��������ΪFD_CLOEXEC��ʾ������ִ��exec����ʱ��fd����ϵͳ�Զ��رգ�FD_CLOEXECΪ1����
��ʾ�����ݸ�exec�������½���.

fcntl(fd,F_SETFD,0)��
��ʾ��fd�����ִ�״̬���Ƶ�exec�������½����С�
ע�⣺���޸��ļ���������־���ļ�״̬��־ֵ���������һ������ȡ�����ڵı�־ֵ��
Ȼ����ϣ���޸�������������±�־ֵ������ֻ��ִ��F_SETFD��F_SETFL���
������ر���ǰ���õı�־λ��

***/
#define   IO_CTL_GETFD        F_GETFD
#define   IO_CTL_SETFD        F_SETFD


/*
������(cmd)F_GETFL��F_SETFL�ı�־�����������:

O_NONBLOCK        ������I/O;���read(2)����û�пɶ�ȡ������,
                  �������write(2)����>������,
                  read��write���÷���-1��EAGAIN����               ��������       ����O_APPEND             ǿ��ÿ��д(write)������������ļ����ĩβ,�൱��open(2)��O_APPEND��־

O_DIRECT          ��С����ȥ��reading��writing�Ļ���Ӱ��.
                  ϵͳ����ͼ���⻺����>�Ķ���д������.

                  ������ܹ����⻺��,��ô������С���Ѿ��������˵��� ����ɵ�Ӱ��.
                  ��������־�õĲ�����,�����Ľ�������

O_ASYNC           ��I/O���õ�ʱ��,����SIGIO�źŷ��͵�������,
                  ����:�������ݿ���>����ʱ��

*/
#define   IO_CTL_GETFL        F_GETFL
/*F_SETFL     ���ø�arg������״̬��־,���Ը��ĵļ�����־��
��O_APPEND�� O_NONBLOCK��O_SYNC��O_ASYNC*/
#define   IO_CTL_SETFL        F_SETFL
/*F_GETOWN ȡ�õ�ǰ���ڽ���SIGIO����SIGURG�źŵĽ���id�������id,
������id���سɸ�ֵ*/
#define   IO_CTL_GETOWN       F_GETOWN
/*F_SETOWN ���ý�����SIGIO��SIGURG�źŵĽ���id�������id,
������idͨ���ṩ��ֵ��arg��˵��,����,arg������Ϊ�ǽ���id*/
#define   IO_CTL_SETOWN       F_SETOWN

#define   IO_CTL_GETLK        F_GETLK
#define   IO_CTL_SETLK        F_SETLK
#define   IO_CTL_SETLKW       F_SETLKW

/***��־λ����
�ļ�״̬��־    ��־ֵ���˽��ƣ�  �ɷ�ͨ��F_GETFL��ȡ   �ɷ�ͨ��F_SETFL����     ˵��
O_RDONLY        00                ��                    ��                      ֻ����
O_WRONLY        01                ��                    ��                      ֻд��
O_RDWR          02                ��                    ��                      Ϊ����д��
O_APPEND        02000             ��                    ��                      ÿ�ζ�д��β��׷��
O_NONBLOCK      04000             ��                    ��                      ������ģʽ
O_SYNC          04010000          ��                    ��                      �ȴ�д��ɣ����ݺ����ԣ�
O_DSYNC         010000            ��                    ��                      �ȴ�д��ɣ������ݣ�
O_RSYNC         04010000          ��                    ��                      ͬ������д
O_FSYNC         04010000          ��                    ��                      �ȴ����
O_ASYNC         020000            ��                    ��                      �첽I/O
***/


/* ------------------------ Types --------------------------- */


/* ------------------------ Macros -------------------------- */
typedef int32 (*ioCtlOptFunc)(int32 fd, int32 flags);


/* -------------- Global Function Prototypes ---------------- */

/*-------------------static func area-------------------------*/
/*�����ļ�״̬��־*/  
static int32 setIoFlags(int32 fd, int32 flags)  
{  
    int32 val;  
  
    /*ȡԭ״̬�����������������״̬��*/  
    if ((val = fcntl(fd, F_GETFL, 0)) < 0)
    {  
      LOG_ERROR("fcntl F_GETFL error\n");
      return TASK_ERROR;
    }  
    
    /*��λָ��״̬λflags*/     
    val |= flags;  
  
    /*������״̬*/  
    if (fcntl(fd, F_SETFL, val) < 0)
    {  
      LOG_ERROR("fcntl F_SETFL error\n");
      return TASK_ERROR; 
    }  
    
    return TASK_OK; 
}  
  
/*����ļ�״̬��־*/  
static int32 clrIoFlags(int32 fd, int32 flags)  
{  
    int32 val;  
  
    /*ȡԭ״̬�����������������״̬��*/  
    if ((val = fcntl(fd, F_GETFL, 0)) < 0) 
    {  
      LOG_ERROR("fcntl F_SETFL error\n");
      return TASK_ERROR;  
    }  
  
    /*���ָ��״̬λflags*/       
    val &= ~flags;  
  
    /*������״̬*/  
    if (fcntl(fd, F_SETFL, val) < 0) 
    {  
      LOG_ERROR("fcntl F_SETFL error\n");
      return TASK_ERROR; 
    }  

    return TASK_OK; 
}  

/*�����ļ�״̬��־*/  
static int32 setIoAccess(int32 fd, int32 flags)  
{  
    int32 val;  
  
    /*ȡԭ״̬�����������������״̬��*/  
    if ((val = fcntl(fd, F_GETFL, 0)) < 0)
    {  
      LOG_ERROR("fcntl F_GETFL error\n");
      return TASK_ERROR;
    }  

    /*���ָ��״̬λflags*/       
    val &= ~O_ACCMODE;  
    
    /*��λָ��״̬λflags*/     
    val |= flags;  
  
    /*������״̬*/  
    if (fcntl(fd, F_SETFL, val) < 0)
    {  
      LOG_ERROR("fcntl F_SETFL error\n");
      return TASK_ERROR; 
    }  
    
    return TASK_OK; 
}  
  

/* ---------------- XXX:   Global Variables ----------------- */

/*����fcntl����fdΪһ�����ڵ���targetFd����������*/
int32 dupFdToNew(int32 fd, int32 targetFd)
{
  int32 ret;
  
  ret = fcntl(fd, IO_CTL_DUPFD, targetFd);
  if (ret < 0)
  {
    LOG_ERROR("fcntl error\n");
    return TASK_ERROR;
  }
  
  return ret;
}

/*�жϸ��������������ļ����¾ɽ����Ƿ���Խ�����ʣ� ACCESSFD_AT_NEWFORK�ǿ���*/
int32 isAccessFdByNewFork(int32 fd)
{
  int32 ret = -1;
  
  ret = fcntl(fd, IO_CTL_GETFD);
  if (ret < 0)
  {
    LOG_ERROR("fcntl error\n");
    return TASK_ERROR;
  }

  ret = ret & FD_CLOEXEC;

  if (ret == 0)
  {
    return ACCESSFD_AT_NEWFORK;
  }

  return NO_ACCESSFD_AT_NEWFORK;
}

/***
fcntl(fd, F_SETFD, FD_CLOEXEC); // ��������ΪFD_CLOEXEC��ʾ
������ִ��exec����ʱ��fd����ϵͳ�Զ��ر�,��ʾ�����ݸ�exec�������½���, 
�������Ϊfcntl(fd, F_SETFD, 0);��ô��fd�����ִ�״̬���Ƶ�exec�������½�����
***/
int32 setCloseFdAtNewFork(int32 fd, int32 flag)
{
  int32 ret = 0, value = 0;

  if (flag == ACCESSFD_AT_NEWFORK)
  {
    value = 0;
  }
  else
  {
    value = FD_CLOEXEC;
  }
  
  ret = fcntl(fd, IO_CTL_SETFD, value);
  if (ret < 0)
  {
    LOG_ERROR("fcntl error\n");
    return TASK_ERROR;
  }

  return TASK_OK;
}

/*��ȡIO��״̬*/
int32 getIoStatus(int32 fd, ioCtl_t select)
{
	int32 flags = -1;
	int32 accmode = -1;

  /*��ñ�׼�����״̬��״̬*/
  flags = fcntl(fd, IO_CTL_GETFL);
  if( flags < 0 )
  {
    /*������*/
    LOG_ERROR("fcntl error\n");
    return TASK_ERROR;
  }

  LOG_DBG("fcntl IO_CTL_GETFL flags is %d select is %d\n", flags, select);
  
  switch(select)
    {
      case IO_CTL_ACCESS:
        {
          /*��÷���ģʽ*/
          accmode = flags & O_ACCMODE;  
          return accmode;
        }
        break;

      case IO_CTL_APPEND:
        {
          if (flags & O_APPEND) 
          {
            return IO_CTL_OK;
          }
          else
          {
            return IO_CTL_NO;
          }
        }
        break;

      case IO_CTL_NOBLOCK:
        {
          if (flags & O_NONBLOCK) 
          {
            return IO_CTL_OK;
          }
          else
          {
            return IO_CTL_NO;
          }
        }
        break;  

      case IO_CTL_SYNC:
        {
          if (flags & O_SYNC) 
          {
            return IO_CTL_OK;
          }
          else
          {
            return IO_CTL_NO;
          }          
        }
        break;

      case IO_CTL_DSYNC:
        {
          if (flags & O_DSYNC) 
          {
            return IO_CTL_OK;
          }
          else
          {
            return IO_CTL_NO;
          }          
        }
        break;

      case IO_CTL_RSYNC:
        {
           if (flags & O_RSYNC) 
          {
            return IO_CTL_OK;
          }
          else
          {
            return IO_CTL_NO;
          }         
        }
        break;  

      case IO_CTL_FSYNC:
        {
          if (flags & O_FSYNC) 
          {
            return IO_CTL_OK;
          }
          else
          {
            return IO_CTL_NO;
          }          
        }
        break;  

      case IO_CTL_ASYNC:
        {
          if (flags & O_ASYNC) 
          {
            return IO_CTL_OK;
          }
          else
          {
            return IO_CTL_NO;
          }          
        }
        break;

      default:
        break;       
    }
 

  return TASK_ERROR;
}


/*����IO��״̬*/
int32 SetIoCtl(int32 fd, ioCtl_t select, ioCtlOpt_t mode)
{
  LOG_DBG("fcntl IO_CTL_SETFL select is %d \n",  select);
 
  ioCtlOptFunc func;
  
  if (mode == IO_CTLOPT_SET)
  {
    func = setIoFlags;
  }
  else if (mode == IO_CTLOPT_CLR)
  {
    func = clrIoFlags;
  }
  else if ((mode == IO_CTLOPT_WRONLY) || 
           (mode == IO_CTLOPT_RDONLY) ||
           (mode == IO_CTLOPT_RDWR))
  {
    func = setIoAccess;
  }
  else
  {
    LOG_ERROR("SetIoCtl mode error\n");
    return TASK_ERROR;
  }
  
  switch(select)
    {
      case IO_CTL_ACCESS:
        {
          if (mode == IO_CTLOPT_WRONLY)
          {
            if (func(fd, O_WRONLY) == TASK_ERROR)
            {
              LOG_ERROR("SetIoCtl error\n");
              return TASK_ERROR;
            }
          }
          else if (mode == IO_CTLOPT_RDONLY)
          {
            if (func(fd, O_RDONLY) == TASK_ERROR)
            {
              LOG_ERROR("SetIoCtl error\n");
              return TASK_ERROR;
            }
          }
          else
          {
            if (func(fd, O_RDWR) == TASK_ERROR)
            {
              LOG_ERROR("SetIoCtl error\n");
              return TASK_ERROR;
            }
          }
        }
        break;
        
      case IO_CTL_APPEND:
        {
          if (func(fd, O_APPEND) == TASK_ERROR)
          {
            LOG_ERROR("SetIoCtl error\n");
            return TASK_ERROR;
          }
        }
        break;

      case IO_CTL_NOBLOCK:
        {
          if (func(fd, O_NONBLOCK) == TASK_ERROR)
          {
            LOG_ERROR("SetIoCtl error\n");
            return TASK_ERROR;
          }
        }
        break;  

      case IO_CTL_SYNC:
        {
          if (func(fd, O_SYNC) == TASK_ERROR)
          {
            LOG_ERROR("SetIoCtl error\n");
            return TASK_ERROR;
          }          
        }
        break;

      case IO_CTL_DSYNC:
        {
          if (func(fd, O_DSYNC) == TASK_ERROR)
          {
            LOG_ERROR("SetIoCtl error\n");
            return TASK_ERROR;
          }          
        }
        break;

      case IO_CTL_RSYNC:
        {
          if (func(fd, O_RSYNC) == TASK_ERROR)
          {
            LOG_ERROR("SetIoCtl error\n");
            return TASK_ERROR;
          }          
        }
        break;  

      case IO_CTL_FSYNC:
        {
          if (func(fd, O_FSYNC) == TASK_ERROR)
          {
            LOG_ERROR("SetIoCtl error\n");
            return TASK_ERROR;
          }          
        }
        break;  

      case IO_CTL_ASYNC:
        {
          if (func(fd, O_ASYNC) == TASK_ERROR)
          {
            LOG_ERROR("SetIoCtl error\n");
            return TASK_ERROR;
          }
        }
        break;

      default:
        break;       
    }
 

  return TASK_OK;
}


/*��ȡ��ǰ���ļ������� fd�Ͻ��յ�SIGIO �� SIGURG�¼��źŵĽ��̻�������ʶ */
int32 getRevIoSigPid(int32 fd)
{
  int32 pid;

  pid = fcntl(fd, F_GETOWN);
  if (pid < 0)
  {
    LOG_ERROR("F_GETOWN error\n");
    return TASK_ERROR;
  }

  return pid;
}

/*���ý�Ҫ���ļ�������fd�Ͻ���SIGIO �� SIGURG�¼��źŵĽ��̻�������ʶ*/
int32 setRevIoSigPid(int32 fd, uint32 pid)
{
  int32 ret = -1;

  ret = fcntl(fd, F_SETOWN, pid);
  if (ret < 0)
  {
    LOG_ERROR("F_SETOWN error\n");
    return TASK_ERROR;
  }

  return ret;
}

