/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#ifndef __LOCKUTILS_H__
#define __LOCKTILS_H__
#include "common.h"

/*由于大多数锁调用是为了对一个文件区域加锁或者解锁（实际中F_GETLK很少使用），
通常使用下列5个宏中的一个来实现这种功能：*/
#define    READLOCK(fd, offset, whence, len) \
            lockReg((fd), F_SETLK, F_RDLCK, (offset), (whence), (len))
#define    READWLOCK(fd, offset, whence, len) \
            lockReg((fd), F_SETLKW, F_RDLCK, (offset), (whence), (len)) /* w means wait */
#define    WRITELOCK(fd, offset, whence, len) \
            lockReg((fd), F_SETLK, F_WRLCK, (offset), (whence), (len))
#define    WRITEWLOCK(fd, offset, whence, len) \
            lockReg((fd), F_SETLKW, F_WRLCK, (offset), (whence), (len))
#define    UNLOCK(fd, offset, whence, len) \
            lockReg((fd), F_SETLK, F_UNLCK, (offset), (whence), (len))

/*如果存在该锁，那么它将阻塞由参数指定的锁请求，返回持有当前锁的进程ID，
否则，返回0，通常如下两个宏使用lock_test函数：*/
#define    IS_READ_LOCKABLE(fd, offset, whence, len) \
            (lock_test((fd), F_RDLCK, (offset), (whence), (len)) == 0)
#define    IS_WRITE_LOCKABLE(fd, offset, whence, len) \
            (lock_test((fd), F_WRLCK, (offset), (whence), (len)) == 0)

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

int32 setFileLock(int32 fd, int32 lock);

#endif
