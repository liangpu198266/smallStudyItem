/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */
#include "lockUtils.h"
#include "logprint.h"

/*lock() 只能实现对整个文件进行加锁，而不能实现记录级的加锁。
系统调用fcntl() 符合 POSIX 标准的文件锁实现，它也是非常强大的文件锁，
fcntl() 可以实现对纪录进行加锁。*/

///////////////////////////

/*F_GETLK, F_SETLK 和 F_SETLKW ：获取，释放或测试记录锁，
使用到的参数是以下结构体指针：

F_SETLK：在指定的字节范围获取锁（F_RDLCK, F_WRLCK）或者释放锁（F_UNLCK）。
         如果与另一个进程的锁操作发生冲突，返回 -1并将errno设置为EACCES或EAGAIN。 
         
F_SETLKW：行为如同F_SETLK，除了不能获取锁时会睡眠等待外。
          如果在等待的过程中接收到信号，会立即返回并将errno置为EINTR。 

F_GETLK：获取文件锁信息。 

F_UNLCK：释放文件锁。 
    
为了设置读锁，文件必须以读的方式打开。为了设置写锁，文件必须以写的方式打开。
为了设置读写锁，文件必须以读写的方式打开。

lock 定义:
      struct flock
      {
        short l_type; // 锁类型： F_RDLCK, F_WRLCK, F_UNLCK
        short l_whence; //l_start字段参照点： 
                        SEEK_SET(文件头), SEEK_CUR(文件当前位置), SEEK_END(文件尾)
        off_t l_start; // 相对于l_whence字段的偏移量 
        off_t l_len; // 需要锁定的长度 
        pid_t l_pid; // 当前获得文件锁的进程标识（F_GETLK）进程的ID(l_pid)持有的锁能阻塞当前进程
      };

  锁可以在当前文件尾端或者超过尾端开始，但是不能在文件起始位置前开始。

  如果l_len为0，则表示锁的范围可以扩展到最大可能的偏移量。
  这意味着不管向文件中追加了多少数据，他们都可以处于锁的范围内。

  如果想要对整个文件加锁，我们设置l_start和l_whence指向文件的起始位置，
并且指定长度l_len为0。

 

关于共享读锁和独占性写锁，基本规则如下：

   任意多个进程在一个给定的区域上可以有共享读锁，
   但是在一个给定的区域上只能有一个进程拥有一把独占性写锁。

   需要注意的是，如果是只有一个进程，那么如果该进程对一个文件区域已经有了一把锁，
   后来该进程又企图在同一文件区域再加另一把锁，那么新锁将替换掉之前的锁，
   即使是先有写锁后又读锁，最后也是用读锁来替换写锁，并不会阻塞，切记。

*/


/*
注意：
新锁替换老锁:

  如果一个进程对文件区间已经加锁，后来该进程又企图在同一文件区域再加一把锁，那么新锁将替
换老锁。

  F_GETLK不能获取当前进程的锁状态，所以使用F_GETLK必须注意:

  F_GETLK的只用必须注意，一个进程不能使用F_GETLK函数来测试自己是否在文件的某一部分持有一
把锁。

  F_GETLK命令定义说明，F_GETLK返回信息指示是否有现存的锁阻止调用进程设置自己的锁。(也就
是说，F_GETLK会尝试加锁，以判断是否有进程阻止自己在设定的区域加锁，如果有，就将l_type设置为F_UNLCK，将l_pid设置为加锁进程的PID)。但是由于对于当前进程，新锁替换老锁，所以调用进程决不会阻塞>在自己持有的锁上加锁。所以F_GETLK命令决不会报告调用进程自己持有的锁。

*/

/*

锁的隐含继承和释放:

一、锁与进程和文件的关系:

    1、当一个进程终止时，它所建立的锁全部释放。（进程退出，文件描述符由系统关闭）

    2、任何时候关闭一个文件描述符时，则该进程通过这个描述符可以引用的文件上的所有锁，
    都被释放。因为这些锁都是当前进程设置的。(关闭文件描述符，加载文件上的锁被释放)

含义如下:

     fd1 = open(pathname,...);

     read_lock(fd1);

     fd2 = dup(fd1);

     close(fd2);                   //此时在pathname上的所有当前进程加的锁都被释放 



     fd1 = open(pathname,...);
     read_lock(fd1,...);

     fd2 = open(pathname,...);

     close(fd2);                   //此时在pathname上的所有当前进程加的锁都被释放 

二、锁与fork的关系:

    fork出来的子进程不能继承父进程的锁。当fork出来的进程可以继承父进程的文件描述符。

三、锁与exec的关系:

    exec执行新程序后，新程序将继承exec之前当前进程设置的锁。当时exec执行的新程序不能继>承exec之前当前进程打开的文件描述符(都被关闭了)。当设置了close-on-exec后，锁就不能被exec执行的>新程序继承。

锁的简单应用:

1、对整个文件上加写锁。

         struct flock fl;

         fl.l_type = F_WRLCK;

         fl.l_sart  =  0;

         fl.l_whence = SEEKSET;

         fl.l_len = 0;        //意味着加锁一直加到EOF

         fcntl(fd,F_SETLK,&fl);

2、对整体文件加写锁后(如上面)，当有数据追加到文件中时，追加进来的部分也是被加锁的。

*/

int32 lockReg(int32 fd, int32 cmd, int32 type, off_t offset, int32 whence, off_t len)
{
    struct flock lock;

    lock.l_type = type;        /* F_RDLCK, F_WRLCK, F_UNLCK */
    lock.l_start = offset;    /* byte offset, relative to l_whence */
    lock.l_whence = whence;    /* SEEK_SET, SEEK_CUR, SEEK_END */
    lock.l_len = len;        /* #bytes (0 means to EOF) */

    return(fcntl(fd, cmd, &lock));
}

/*
实际中，测试锁很少用F_GETLK，而是用如下函数lockTest来代替，
如果存在该锁，那么它将阻塞由参数指定的锁请求，返回持有当前锁的进程ID，
否则，返回0，通常如下两个宏使用lock_test函数：
*/
pid_t lockTest(int32 fd, int32 type, off_t offset, int32 whence, off_t len)
{
    struct flock    lock;

    lock.l_type = type;        /* F_RDLCK or F_WRLCK */
    lock.l_start = offset;    /* byte offset, relative to l_whence */
    lock.l_whence = whence;    /* SEEK_SET, SEEK_CUR, SEEK_END */
    lock.l_len = len;        /* #bytes (0 means to EOF) */

    if (fcntl(fd, F_GETLK, &lock) < 0)
        printf("fcntl error\n");

    if (lock.l_type == F_UNLCK)
        return (0);        /* false, region isn't locked by another proc */
    return (lock.l_pid);    /* true, return pid of lock owner */
}


/*
文件锁

LOCK_SH：表示要创建一个共享锁，在任意时间内，一个文件的共享锁可以被多个进程拥有
LOCK_EX：表示创建一个排他锁，在任意时间内，一个文件的排他锁只能被一个进程拥有
LOCK_UN：表示删除该进程创建的锁
LOCK_MAND：它主要是用于共享模式强制锁，它可以与 LOCK_READ 
           或者 LOCK_WRITE 联合起来使用，
           从而表示是否允许并发的读操作或者并发的写操作
           （尽管在 flock() 的手册页中没有介绍 LOCK_MAND，
           但是阅读内核源代码就会发现，这在内核中已经实现了）

*/

int32 setFileLock(int32 fd, int32 lock)
{
  int32 ret = -1;

  ret = flock(fd,lock);
  if (ret < 0)
  {
    displayErrorMsg("flock set lock error\n");
    return TASK_ERROR;
  }

  return TASK_OK;
}
