/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#ifndef __LOCKUTILS_H__
#define __LOCKTILS_H__
#include "common.h"

/*���ڴ������������Ϊ�˶�һ���ļ�����������߽�����ʵ����F_GETLK����ʹ�ã���
ͨ��ʹ������5�����е�һ����ʵ�����ֹ��ܣ�*/
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

/*������ڸ�������ô���������ɲ���ָ���������󣬷��س��е�ǰ���Ľ���ID��
���򣬷���0��ͨ������������ʹ��lock_test������*/
#define    IS_READ_LOCKABLE(fd, offset, whence, len) \
            (lock_test((fd), F_RDLCK, (offset), (whence), (len)) == 0)
#define    IS_WRITE_LOCKABLE(fd, offset, whence, len) \
            (lock_test((fd), F_WRLCK, (offset), (whence), (len)) == 0)

/*
�ļ���

LOCK_SH����ʾҪ����һ����������������ʱ���ڣ�һ���ļ��Ĺ��������Ա��������ӵ��
LOCK_EX����ʾ����һ����������������ʱ���ڣ�һ���ļ���������ֻ�ܱ�һ������ӵ��
LOCK_UN����ʾɾ���ý��̴�������
LOCK_MAND������Ҫ�����ڹ���ģʽǿ�������������� LOCK_READ 
           ���� LOCK_WRITE ��������ʹ�ã�
           �Ӷ���ʾ�Ƿ��������Ķ��������߲�����д����
           �������� flock() ���ֲ�ҳ��û�н��� LOCK_MAND��
           �����Ķ��ں�Դ����ͻᷢ�֣������ں����Ѿ�ʵ���ˣ�

*/

int32 setFileLock(int32 fd, int32 lock);

#endif
