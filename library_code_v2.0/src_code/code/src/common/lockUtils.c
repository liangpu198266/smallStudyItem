/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */
#include "lockUtils.h"
#include "logprint.h"

/*lock() ֻ��ʵ�ֶ������ļ����м�����������ʵ�ּ�¼���ļ�����
ϵͳ����fcntl() ���� POSIX ��׼���ļ���ʵ�֣���Ҳ�Ƿǳ�ǿ����ļ�����
fcntl() ����ʵ�ֶԼ�¼���м�����*/

///////////////////////////

/*F_GETLK, F_SETLK �� F_SETLKW ����ȡ���ͷŻ���Լ�¼����
ʹ�õ��Ĳ��������½ṹ��ָ�룺

F_SETLK����ָ�����ֽڷ�Χ��ȡ����F_RDLCK, F_WRLCK�������ͷ�����F_UNLCK����
         �������һ�����̵�������������ͻ������ -1����errno����ΪEACCES��EAGAIN�� 
         
F_SETLKW����Ϊ��ͬF_SETLK�����˲��ܻ�ȡ��ʱ��˯�ߵȴ��⡣
          ����ڵȴ��Ĺ����н��յ��źţ����������ز���errno��ΪEINTR�� 

F_GETLK����ȡ�ļ�����Ϣ�� 

F_UNLCK���ͷ��ļ����� 
    
Ϊ�����ö������ļ������Զ��ķ�ʽ�򿪡�Ϊ������д�����ļ�������д�ķ�ʽ�򿪡�
Ϊ�����ö�д�����ļ������Զ�д�ķ�ʽ�򿪡�

lock ����:
      struct flock
      {
        short l_type; // �����ͣ� F_RDLCK, F_WRLCK, F_UNLCK
        short l_whence; //l_start�ֶβ��յ㣺 
                        SEEK_SET(�ļ�ͷ), SEEK_CUR(�ļ���ǰλ��), SEEK_END(�ļ�β)
        off_t l_start; // �����l_whence�ֶε�ƫ���� 
        off_t l_len; // ��Ҫ�����ĳ��� 
        pid_t l_pid; // ��ǰ����ļ����Ľ��̱�ʶ��F_GETLK�����̵�ID(l_pid)���е�����������ǰ����
      };

  �������ڵ�ǰ�ļ�β�˻��߳���β�˿�ʼ�����ǲ������ļ���ʼλ��ǰ��ʼ��

  ���l_lenΪ0�����ʾ���ķ�Χ������չ�������ܵ�ƫ������
  ����ζ�Ų������ļ���׷���˶������ݣ����Ƕ����Դ������ķ�Χ�ڡ�

  �����Ҫ�������ļ���������������l_start��l_whenceָ���ļ�����ʼλ�ã�
����ָ������l_lenΪ0��

 

���ڹ�������Ͷ�ռ��д���������������£�

   ������������һ�������������Ͽ����й��������
   ������һ��������������ֻ����һ������ӵ��һ�Ѷ�ռ��д����

   ��Ҫע����ǣ������ֻ��һ�����̣���ô����ý��̶�һ���ļ������Ѿ�����һ������
   �����ý�������ͼ��ͬһ�ļ������ټ���һ��������ô�������滻��֮ǰ������
   ��ʹ������д�����ֶ��������Ҳ���ö������滻д�����������������мǡ�

*/


/*
ע�⣺
�����滻����:

  ���һ�����̶��ļ������Ѿ������������ý�������ͼ��ͬһ�ļ������ټ�һ��������ô��������
��������

  F_GETLK���ܻ�ȡ��ǰ���̵���״̬������ʹ��F_GETLK����ע��:

  F_GETLK��ֻ�ñ���ע�⣬һ�����̲���ʹ��F_GETLK�����������Լ��Ƿ����ļ���ĳһ���ֳ���һ
������

  F_GETLK�����˵����F_GETLK������Ϣָʾ�Ƿ����ִ������ֹ���ý��������Լ�������(Ҳ��
��˵��F_GETLK�᳢�Լ��������ж��Ƿ��н�����ֹ�Լ����趨���������������У��ͽ�l_type����ΪF_UNLCK����l_pid����Ϊ�������̵�PID)���������ڶ��ڵ�ǰ���̣������滻���������Ե��ý��̾���������>���Լ����е����ϼ���������F_GETLK��������ᱨ����ý����Լ����е�����

*/

/*

���������̳к��ͷ�:

һ��������̺��ļ��Ĺ�ϵ:

    1����һ��������ֹʱ��������������ȫ���ͷš��������˳����ļ���������ϵͳ�رգ�

    2���κ�ʱ��ر�һ���ļ�������ʱ����ý���ͨ������������������õ��ļ��ϵ���������
    �����ͷš���Ϊ��Щ�����ǵ�ǰ�������õġ�(�ر��ļ��������������ļ��ϵ������ͷ�)

��������:

     fd1 = open(pathname,...);

     read_lock(fd1);

     fd2 = dup(fd1);

     close(fd2);                   //��ʱ��pathname�ϵ����е�ǰ���̼ӵ��������ͷ� 



     fd1 = open(pathname,...);
     read_lock(fd1,...);

     fd2 = open(pathname,...);

     close(fd2);                   //��ʱ��pathname�ϵ����е�ǰ���̼ӵ��������ͷ� 

��������fork�Ĺ�ϵ:

    fork�������ӽ��̲��ܼ̳и����̵�������fork�����Ľ��̿��Լ̳и����̵��ļ���������

��������exec�Ĺ�ϵ:

    execִ���³�����³��򽫼̳�exec֮ǰ��ǰ�������õ�������ʱexecִ�е��³����ܼ�>��exec֮ǰ��ǰ���̴򿪵��ļ�������(�����ر���)����������close-on-exec�����Ͳ��ܱ�execִ�е�>�³���̳С�

���ļ�Ӧ��:

1���������ļ��ϼ�д����

         struct flock fl;

         fl.l_type = F_WRLCK;

         fl.l_sart  =  0;

         fl.l_whence = SEEKSET;

         fl.l_len = 0;        //��ζ�ż���һֱ�ӵ�EOF

         fcntl(fd,F_SETLK,&fl);

2���������ļ���д����(������)����������׷�ӵ��ļ���ʱ��׷�ӽ����Ĳ���Ҳ�Ǳ������ġ�

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
ʵ���У�������������F_GETLK�����������º���lockTest�����棬
������ڸ�������ô���������ɲ���ָ���������󣬷��س��е�ǰ���Ľ���ID��
���򣬷���0��ͨ������������ʹ��lock_test������
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
