/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */
#include "fileUtils.h"
#include "logprint.h"

static int8 currentWorkPath[BUFSIZE];

int8 initFileDir(void)
{
  int32 rv = 0;
  
  memset(currentWorkPath, 0x0, BUFSIZE);

  if (getcwd(currentWorkPath,BUFSIZE) == FALSE)
  {
    return TASK_ERROR;
  }
  
  return TASK_OK;
}

int8 *getcurrentWorkPath(void)
{
  return currentWorkPath;
}

int32 saveDataToFile(int8 *dir, int8 *data, int32 len)
{
  int32 fileNode = -1;
  int32 size = 0;
  if (access(dir, F_OK) < 0) 
  {
    LOG_DBG("file %s no exist \n", dir);
    fileNode = open(dir, MODE_FILE_READWRITE | MODE_FILE_CREAT, 777);
  }
  else
  {
    fileNode = open(dir, MODE_FILE_READWRITE, 777);
    if (fileNode < 0)
    {
      LOG_ERROR("write file error\n");
      return TASK_ERROR;
    }
  }

  size = write(fileNode, data, len);
  
  fsync(fileNode);
  close(fileNode); 

  return size;
}

void deleteFile(int8 *dir)
{
  if (access(dir, F_OK) != -1) 
  {
    LOG_DBG("delete file %s\n", dir);
    unlink(dir);
  }
  else
  {
    LOG_DBG("file %s no exist \n", dir);
  }
}

int32 readDataFromFile(int8 *dir, int8 *buf, int32 len)
{
  int32 fileNode = -1;
  int32 size;
  
  fileNode = open(dir, MODE_FILE_READONLY);
  if (fileNode < 0)
  {
    LOG_ERROR("read file error\n");
    return TASK_ERROR;
  }

  size = read(fileNode, buf, len);
  
  close(fileNode);

  return size;
}


/*�ƶ��ļ�
������ԭ�͡�int lseek(int handle,long offset,int fromwhere)
�����ܽ��⡿�ƶ��ļ��Ķ�/дָ�룬���ɹ��򷵻ص�ǰ�Ķ�/дλ�ã�
Ҳ���Ǿ����ļ���ͷ�ж��ٸ��ֽڣ����д����򷵻�-1��
������˵����handleΪ�ļ������offsetΪƫ������ÿһ��д��������Ҫ�ƶ��ľ��롣
fromwhereΪ������

����fromwhereΪ�����е�һ����
SEEK_SET������/дλ��ָ���ļ�ͷ��������offset��λ������
SEEK_CUR����Ŀǰ�Ķ�/дλ�õ�λ������������offset��λ������
SEEK_END������/дλ��ָ���ļ�β��������offset��λ������
  ��whence ֵΪSEEK_CUR ��SEEK_END ʱ, ����offet ����ֵ�ĳ���.
  ����˵����Linux ϵͳ������lseek()��tty װ������, ���������lseek()����ESPIPE.
*/
int32 moveToBeginAtFile(int32 fd)
{
  int32 offset;

  offset = lseek(fd, 0, SEEK_SET);
  if (offset < 0)
  {
    LOG_ERROR("lseek SEET_SET error\n");
    return TASK_ERROR;
  }
  
  return offset;
}

int32 moveToEndAtFile(int32 fd)
{
  int32 offset;

  offset = lseek(fd, 0, SEEK_END);
  if (offset < 0)
  {
    LOG_ERROR("lseek SEEK_END error\n");
    return TASK_ERROR;
  }
  
  return offset;
}

int32 moveToRequestAtFile(int32 fd, int32 offset)
{
  int32 len;

  offset = lseek(fd, offset, SEEK_CUR);
  if (offset < 0)
  {
    LOG_ERROR("lseek SEEK_END error\n");
    return TASK_ERROR;
  }
  
  return offset;
}

/*
off_t    currpos;
currpos = lseek(fd, 0, SEEK_CUR);
  �������Ҳ�������ж������Ƿ���Ըı�ĳ���ļ���ƫ������
������� fd���ļ���������ָ������ pipe���ܵ�����FIFO ���� socket��
lseek ���� -1 ������ errno Ϊ ESPIPE��
*/
int32 getFdOffset(int32 fd)
{
  int32 offset;

  offset = lseek(fd, 0, SEEK_CUR);
  if (offset < 0)
  {
    LOG_ERROR("lseek SEEK_END error\n");
    return TASK_ERROR;
  }
  
  return offset;
}

/*
��UNIX�ļ������У��ļ�λ�������Դ����ļ��ĵ�ǰ���ȣ�
����������£��Ը��ļ�����һ��д���ӳ����ļ��������ļ��й���һ���ն���
��һ��������ġ�λ���ļ��е�û��д�����ֽڶ�����Ϊ 0��
��� offset ���ļ��ĵ�ǰ���ȸ�����һ��д�����ͻ���ļ����Ŵ�extend������
�������ν�����ļ��ﴴ�조�ն���hole������û�б�ʵ��д���ļ��������ֽ����ظ��� 0 ��ʾ��
�ն��Ƿ�ռ��Ӳ�̿ռ������ļ�>ϵͳ��file system�������ġ�
�ն��ļ����úܴ�����Ѹ�������ļ�����δ�������ʱ���Ѿ�ռ����ȫ���ļ���С�Ŀռ䣬
��ʱ����ǿն��ļ�������ʱ���û�пն��ļ���
���߳�����ʱ�ļ��Ͷ�ֻ�ܴ�һ���ط�д�룬��Ͳ��Ƕ��߳��ˡ����
���˿ն��ļ������ԴӲ�ͬ�ĵ�ַд�룬������˶��̵߳���������

*/
int32 createHollowFile(int8 *dir, int32 leng)
{
  int32 fd;
  off_t offset;
  
  fd = creat(dir, 0777);   //����һ��Ȩ��Ϊ�ɶ���д��ִ�е��ļ�"tmp"
  if(fd < 0)   //���������-1
  {
    LOG_ERROR("creat file error\n");
    return TASK_ERROR;
  }
  offset = lseek(fd, leng, SEEK_END);  //����ƫ�ƵĴ�С,��ƫ�Ƶ��ļ�β//��
  if (offset < 0)
  {
    LOG_ERROR("lseek SEEK_END error\n");
    return TASK_ERROR;
  }
  
  offset = write(fd, "", 1);  //д�գ�д1���ֽڵ��ļ���������
  close(fd);   //�ر��ļ�������
  return TASK_OK;
}

int32 displayFileInfo(int8 *dir)
{
	struct stat st;

  if (access(dir, F_OK) < 0) 
  {
    LOG_DBG("file %s no exist \n", dir);
    return TASK_ERROR;
  }
    
  if(stat(dir, &st) < 0)
  {
    LOG_ERROR("get file info error\n");
    return TASK_ERROR;
  }
  
  printf("�������ļ����豸ID��%d\n",(int32)st.st_dev);
  printf("���ļ��Ľڵ㣺%d\n",(int32)st.st_ino);
  printf("���ļ��ı���ģʽ��%d\n",(int32)st.st_mode);
  printf("���ļ���Ӳ��������%d\n",(int32)st.st_nlink);
  printf("���ļ���������ID��%d\n",(int32)st.st_uid);
  printf("���ļ��������ߵ���ID��%d\n",(int32)st.st_gid);
  printf("�豸ID��������ļ�Ϊ�����豸����%d\n",(int32)st.st_rdev);
  printf("���ļ��Ĵ�С��%d\n",(int32)st.st_size);
  printf("���ļ��������ļ�ϵͳ���С��%d\n",(int32)st.st_blksize);
  printf("���ļ���ռ�ÿ�������%d\n",(int32)st.st_blocks);
  printf("���ļ���������ʱ�䣺%d\n",(int32)st.st_atime);
  printf("���ļ�������޸�ʱ�䣺%d\n",(int32)st.st_mtime);
  printf("���ļ������״̬�ı�ʱ�䣺%d\n",(int32)st.st_ctime);
  
 	
	return TASK_OK;
}

/*
ͷ�ļ���#include <unistd.h>    #include <sys/mman.h>

���庯����void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offsize);

����˵����mmap()������ĳ���ļ�����ӳ�䵽�ڴ��У��Ը��ڴ�����Ĵ�ȡ����ֱ�ӶԸ��ļ����ݵĶ�д��

����	                   ˵��
start	            ָ������Ӧ���ڴ���ʼ��ַ��ͨ����ΪNULL��������ϵͳ�Զ�ѡ����ַ����Ӧ�ɹ���õ�ַ�᷵�ء�
length	          �����ļ��ж��Ĳ��ֶ�Ӧ���ڴ档
prot	            ����ӳ������ı�����ʽ����������ϣ�
                  PROT_EXEC  ӳ������ɱ�ִ�У�
                  PROT_READ  ӳ������ɱ���ȡ��
                  PROT_WRITE  ӳ������ɱ�д�룻
                  PROT_NONE  ӳ�������ܴ�ȡ��
                  
flags	            ��Ӱ��ӳ������ĸ������ԣ�
                  MAP_FIXED  ������� start ��ָ�ĵ�ַ�޷��ɹ�����ӳ��ʱ��
                    �����ӳ�䣬���Ե�ַ��������ͨ���������ô���ꡣ
                  MAP_SHARED  ��Ӧ�������д�����ݻḴ�ƻ��ļ��ڣ�
                    ������������ӳ����ļ��Ľ��̹���
                  MAP_PRIVATE  ��Ӧ�������д����������һ��ӳ���ļ��ĸ��ƣ�
                    ��˽�˵�"д��ʱ����" (copy on write)�Դ����������κ��޸Ķ�����д��ԭ�����ļ����ݡ�
                  MAP_ANONYMOUS  ��������ӳ�䣬��ʱ����Բ���fd�����漰�ļ���
                    ����ӳ�������޷����������̹���
                  MAP_DENYWRITE  ֻ�����Ӧ�������д�������
                    �������ļ�ֱ��д��Ĳ������ᱻ�ܾ���
                  MAP_LOCKED  ��ӳ����������ס�����ʾ�����򲻻ᱻ�û�(swap)��

                  �ڵ���mmap()ʱ����Ҫָ��MAP_SHARED ��MAP_PRIVATE��
                  
fd	              open()���ص��ļ������ʣ�������ӳ�䵽�ڴ���ļ���

offset	          �ļ�ӳ���ƫ������ͨ������Ϊ0��������ļ���ǰ����ʼ��Ӧ��
                  offset�����Ƿ�ҳ��С����������

����ֵ����ӳ��ɹ��򷵻�ӳ�������ڴ���ʼ��ַ�����򷵻�MAP_FAILED(-1)������ԭ�����errno �С�

������룺
EBADF  ����fd ������Ч���ļ������ʡ�
EACCES  ��ȡȨ�����������MAP_PRIVATE ������ļ�����ɶ���ʹ��MAP_SHARED ��Ҫ��PROT_WRITE �Լ����ļ�Ҫ��д�롣
EINVAL  ����start��length ��offset ��һ�����Ϸ���
EAGAIN  �ļ�����ס��������̫���ڴ汻��ס��
ENOMEM  �ڴ治�㡣

�ܽ�һ�£����ܲ������������£�

����1.�����ļ�ӳ�������������ӵ�ж�Ȩ�ޣ���������SIGSEGV�ź�
����2.���ڴ�����д��ӳ���ļ�ʱ������ȷ����д�ļ���ǰλ�õ��ļ���β�ĳ��Ȳ�С����д���ݳ��ȣ�
    �������SIGBUS�ź�
����3.�ر��ļ������������ܱ�֤�ļ����ݲ����޸�
����4.munmap������ʹӳ�������д�ش���

*/

int32 readFileByMmap(int8 *dir, int8 *buf)
{
  int32 fileNode =  -1;
  void *start;
  struct stat sb;

  if (access(dir, F_OK) < 0) 
  {
    LOG_DBG("file %s no exist \n", dir);
    return TASK_ERROR;
  }
    
  fileNode = open(dir, MODE_FILE_READWRITE, 0777);
  if (fileNode < 0)
  {
    LOG_ERROR("open file error\n");
    return TASK_ERROR;
  }
  
  fstat(fileNode, &sb); /* ȡ���ļ���С */
  
  start = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fileNode, 0);
  if(start == MAP_FAILED) /* �ж��Ƿ�ӳ��ɹ� */
  {
    LOG_ERROR("mmap file error\n");
    return TASK_ERROR;
  }

  memcpy(buf, start, sb.st_size);
  
  munmap(start, sb.st_size); /* ���ӳ�� */
  
  close(fileNode);

  return sb.st_size;
}

int32 saveDataToFileByMmap(int8 *dir, int8 *buf)
{
  int32 fileNode = -1;
  void *start;
  struct stat sb;

  if (access(dir, F_OK) < 0) 
  {
    LOG_DBG("file %s no exist \n", dir);
    fileNode = open(dir, MODE_FILE_READWRITE | MODE_FILE_CREAT, 777);
  }
  else
  {
    fileNode = open(dir, MODE_FILE_READWRITE, 777);
    if (fileNode < 0)
    {
      LOG_ERROR("write file error\n");
      return TASK_ERROR;
    }
  }  
  
  start = mmap(NULL, sizeof(buf), PROT_WRITE, MAP_PRIVATE, fileNode, 0);
  if(start == MAP_FAILED) /* �ж��Ƿ�ӳ��ɹ� */
  {
    LOG_ERROR("mmap file error\n");
    return TASK_ERROR;
  }

  memcpy(start, buf, sizeof(buf));
  
  munmap(start, sizeof(buf)); /* ���ӳ�� */
  
  close(fileNode);

  return sizeof(buf);
}

