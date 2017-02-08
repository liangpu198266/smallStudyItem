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
/*fcntl 5种功能*/
/*
1.复制一个现有的描述符（cmd=F_DUPFD）.
2.获得／设置文件描述符标记(cmd=F_GETFD或F_SETFD).
3.获得／设置文件状态标记(cmd=F_GETFL或F_SETFL).
4.获得／设置异步I/O所有权(cmd=F_GETOWN或F_SETOWN).
5.获得／设置记录锁(cmd=F_GETLK,F_SETLK或F_SETLKW).
*/

/*
F_DUPFD      :
（1）最小的大于或等于arg的一个可用的描述符                       
（2）与原始操作符一样的某对象的引用
（3）如果对象是文件(file)的话,返回一个新的描述符,
     这个描述符与arg共享相同的偏移量(offset)
（4）相同的访问模式(读,写或读/写)
（5）相同的文件状态标志(如:两个文件描述符共享相同的状态标志)        
（6）与新的文件描述符结合在一起的close-on-exec标志被设置成交叉式访问
     execve(2)的系统调用
*/
#define   IO_CTL_DUPFD        F_DUPFD 
/***
每个文件描述符都有一个close-on-exec标志。
在系统默认情况下，这个标志最后一位被设置为0。
而FD_CLOEXEC则用来设置文件的close-on-exec，
FD_CLOEXEC的值为1，所以在设置close-on-exec位时可以使用FD_CLOEXEC
也可以直接的使用0或1数字对close-on-exec位进行设置。
当将close-on-exec标志置为1时，即开启此标志。
那么当子进程调用exec函数之前，系统就已经让子进程将此文件描述符关闭了。
当close-on-exec标志置为0时，即关闭了此标志。那么当子进程调用exec函数，
子进程将不会关闭该文件描述符。此时，父子进程将共享该文件，
它们具有同一个文件表项，也就有了同一个文件偏移量等。

 关于close-on-exec :
 每个文件描述符都有一个close-on-exec标志。默认情况下，这个标志最后一位被设置为 0。
 这个标志符的具体作用在于当开辟其他进程调用exec（）族函数时，在调用exec函数之前为exec族函数释放对应的文件描述符。

 另一段摘抄:
 Linux下socket也是文件描述符的一种，当B fork进程C的时候，
 C也会继承B的11111端口socket文件描述符，当B挂了的时候，C就会占领监听权。
所以现在的需求就是在fork的时候关闭B已经打开的文件描述符。
系统给出的解决方案是：close_on_exec。当父进程打开文件时，
只需要应用程序设置FD_CLOSEXEC标志位，则当fork后exec其他程序的时候，
内核自动会将其继承的父进程FD关闭。

F_GETFD 取得与文件描述符fd联合的close-on-exec标志，类似FD_CLOEXEC。
如果返回值和FD_CLOEXEC进行与运算结果是0的话，文件保持交叉式访问exec(),
否则如果通过exec运行的话，文件将被关闭。

F_SETFD 设置close-on-exec标志。
fcntl(fd,F_SETFD,FD_CLOEXEC)；
这里设置为FD_CLOEXEC表示当程序执行exec函数时本fd将被系统自动关闭（FD_CLOEXEC为1），
表示不传递给exec创建的新进程.

fcntl(fd,F_SETFD,0)；
表示本fd将保持打开状态复制到exec创建的新进程中。
注意：在修改文件描述符标志或文件状态标志值必须谨慎，一般是先取得现在的标志值，
然后按照希望修改它，最后设置新标志值。不能只是执行F_SETFD或F_SETFL命令，
这样会关闭以前设置的标志位。

***/
#define   IO_CTL_GETFD        F_GETFD
#define   IO_CTL_SETFD        F_SETFD


/*
命令字(cmd)F_GETFL和F_SETFL的标志如下面的描述:

O_NONBLOCK        非阻塞I/O;如果read(2)调用没有可读取的数据,
                  或者如果write(2)操作>将阻塞,
                  read或write调用返回-1和EAGAIN错误               　　　　       　　O_APPEND             强制每次写(write)操作都添加在文件大的末尾,相当于open(2)的O_APPEND标志

O_DIRECT          最小化或去掉reading和writing的缓存影响.
                  系统将企图避免缓存你>的读或写的数据.

                  如果不能够避免缓存,那么它将最小化已经被缓存了的数 据造成的影响.
                  如果这个标志用的不够好,将大大的降低性能

O_ASYNC           当I/O可用的时候,允许SIGIO信号发送到进程组,
                  例如:当有数据可以>读的时候

*/
#define   IO_CTL_GETFL        F_GETFL
/*F_SETFL     设置给arg描述符状态标志,可以更改的几个标志是
：O_APPEND， O_NONBLOCK，O_SYNC和O_ASYNC*/
#define   IO_CTL_SETFL        F_SETFL
/*F_GETOWN 取得当前正在接收SIGIO或者SIGURG信号的进程id或进程组id,
进程组id返回成负值*/
#define   IO_CTL_GETOWN       F_GETOWN
/*F_SETOWN 设置将接收SIGIO和SIGURG信号的进程id或进程组id,
进程组id通过提供负值的arg来说明,否则,arg将被认为是进程id*/
#define   IO_CTL_SETOWN       F_SETOWN

#define   IO_CTL_GETLK        F_GETLK
#define   IO_CTL_SETLK        F_SETLK
#define   IO_CTL_SETLKW       F_SETLKW

/***标志位部分
文件状态标志    标志值（八进制）  可否通过F_GETFL获取   可否通过F_SETFL设置     说明
O_RDONLY        00                是                    否                      只读打开
O_WRONLY        01                是                    否                      只写打开
O_RDWR          02                是                    否                      为读、写打开
O_APPEND        02000             是                    是                      每次读写在尾部追加
O_NONBLOCK      04000             是                    是                      非阻塞模式
O_SYNC          04010000          是                    是                      等待写完成（数据和属性）
O_DSYNC         010000            是                    是                      等待写完成（仅数据）
O_RSYNC         04010000          是                    是                      同步读、写
O_FSYNC         04010000          是                    是                      等待完成
O_ASYNC         020000            是                    是                      异步I/O
***/


/* ------------------------ Types --------------------------- */


/* ------------------------ Macros -------------------------- */
typedef int32 (*ioCtlOptFunc)(int32 fd, int32 flags);


/* -------------- Global Function Prototypes ---------------- */

/*-------------------static func area-------------------------*/
/*设置文件状态标志*/  
static int32 setIoFlags(int32 fd, int32 flags)  
{  
    int32 val;  
  
    /*取原状态（包含着其他方面的状态）*/  
    if ((val = fcntl(fd, F_GETFL, 0)) < 0)
    {  
      LOG_ERROR("fcntl F_GETFL error\n");
      return TASK_ERROR;
    }  
    
    /*置位指定状态位flags*/     
    val |= flags;  
  
    /*设置新状态*/  
    if (fcntl(fd, F_SETFL, val) < 0)
    {  
      LOG_ERROR("fcntl F_SETFL error\n");
      return TASK_ERROR; 
    }  
    
    return TASK_OK; 
}  
  
/*清除文件状态标志*/  
static int32 clrIoFlags(int32 fd, int32 flags)  
{  
    int32 val;  
  
    /*取原状态（包含着其他方面的状态）*/  
    if ((val = fcntl(fd, F_GETFL, 0)) < 0) 
    {  
      LOG_ERROR("fcntl F_SETFL error\n");
      return TASK_ERROR;  
    }  
  
    /*清除指定状态位flags*/       
    val &= ~flags;  
  
    /*设置新状态*/  
    if (fcntl(fd, F_SETFL, val) < 0) 
    {  
      LOG_ERROR("fcntl F_SETFL error\n");
      return TASK_ERROR; 
    }  

    return TASK_OK; 
}  

/*设置文件状态标志*/  
static int32 setIoAccess(int32 fd, int32 flags)  
{  
    int32 val;  
  
    /*取原状态（包含着其他方面的状态）*/  
    if ((val = fcntl(fd, F_GETFL, 0)) < 0)
    {  
      LOG_ERROR("fcntl F_GETFL error\n");
      return TASK_ERROR;
    }  

    /*清除指定状态位flags*/       
    val &= ~O_ACCMODE;  
    
    /*置位指定状态位flags*/     
    val |= flags;  
  
    /*设置新状态*/  
    if (fcntl(fd, F_SETFL, val) < 0)
    {  
      LOG_ERROR("fcntl F_SETFL error\n");
      return TASK_ERROR; 
    }  
    
    return TASK_OK; 
}  
  

/* ---------------- XXX:   Global Variables ----------------- */

/*利用fcntl复制fd为一个大于等于targetFd可用描述符*/
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

/*判断该描述符所属的文件在新旧进程是否可以交差访问， ACCESSFD_AT_NEWFORK是可以*/
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
fcntl(fd, F_SETFD, FD_CLOEXEC); // 这里设置为FD_CLOEXEC表示
当程序执行exec函数时本fd将被系统自动关闭,表示不传递给exec创建的新进程, 
如果设置为fcntl(fd, F_SETFD, 0);那么本fd将保持打开状态复制到exec创建的新进程中
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

/*获取IO的状态*/
int32 getIoStatus(int32 fd, ioCtl_t select)
{
	int32 flags = -1;
	int32 accmode = -1;

  /*获得标准输入的状态的状态*/
  flags = fcntl(fd, IO_CTL_GETFL);
  if( flags < 0 )
  {
    /*错误发生*/
    LOG_ERROR("fcntl error\n");
    return TASK_ERROR;
  }

  LOG_DBG("fcntl IO_CTL_GETFL flags is %d select is %d\n", flags, select);
  
  switch(select)
    {
      case IO_CTL_ACCESS:
        {
          /*获得访问模式*/
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


/*设置IO的状态*/
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


/*获取当前在文件描述词 fd上接收到SIGIO 或 SIGURG事件信号的进程或进程组标识 */
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

/*设置将要在文件描述词fd上接收SIGIO 或 SIGURG事件信号的进程或进程组标识*/
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

