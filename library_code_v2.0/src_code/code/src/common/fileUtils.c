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


/*移动文件
【函数原型】int lseek(int handle,long offset,int fromwhere)
【功能讲解】移动文件的读/写指针，若成功则返回当前的读/写位置，
也就是距离文件开头有多少个字节，若有错误则返回-1。
【参数说明】handle为文件句柄，offset为偏移量，每一读写操作所需要移动的距离。
fromwhere为出发点

参数fromwhere为下列中的一个：
SEEK_SET，将读/写位置指向文件头后再增加offset个位移量；
SEEK_CUR，以目前的读/写位置的位置再往后增加offset个位移量；
SEEK_END，将读/写位置指向文件尾后再增加offset个位移量。
  当whence 值为SEEK_CUR 或SEEK_END 时, 参数offet 允许负值的出现.
  附加说明：Linux 系统不允许lseek()对tty 装置作用, 此项动作会令lseek()返回ESPIPE.
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
  这个技巧也可用于判断我们是否可以改变某个文件的偏移量。
如果参数 fd（文件描述符）指定的是 pipe（管道）、FIFO 或者 socket，
lseek 返回 -1 并且置 errno 为 ESPIPE。
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
在UNIX文件操作中，文件位移量可以大于文件的当前长度，
在这种情况下，对该文件的下一次写将延长该文件，并在文件中构成一个空洞，
这一点是允许的。位于文件中但没有写过的字节都被设为 0。
如果 offset 比文件的当前长度更大，下一个写操作就会把文件“撑大（extend）”。
这就是所谓的在文件里创造“空洞（hole）”。没有被实际写入文件的所有字节由重复的 0 表示。
空洞是否占用硬盘空间是由文件>系统（file system）决定的。
空洞文件作用很大，例如迅雷下载文件，在未下载完成时就已经占据了全部文件大小的空间，
这时候就是空洞文件。下载时如果没有空洞文件，
多线程下载时文件就都只能从一个地方写入，这就不是多线程了。如果
有了空洞文件，可以从不同的地址写入，就完成了多线程的优势任务。

*/
int32 createHollowFile(int8 *dir, int32 leng)
{
  int32 fd;
  off_t offset;
  
  fd = creat(dir, 0777);   //创建一个权限为可读可写可执行的文件"tmp"
  if(fd < 0)   //如果出错返回-1
  {
    LOG_ERROR("creat file error\n");
    return TASK_ERROR;
  }
  offset = lseek(fd, leng, SEEK_END);  //设置偏移的大小,并偏移到文件尾//部
  if (offset < 0)
  {
    LOG_ERROR("lseek SEEK_END error\n");
    return TASK_ERROR;
  }
  
  offset = write(fd, "", 1);  //写空，写1个字节到文件描述符里
  close(fd);   //关闭文件描述符
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
  
  printf("包含此文件的设备ID：%d\n",(int32)st.st_dev);
  printf("此文件的节点：%d\n",(int32)st.st_ino);
  printf("此文件的保护模式：%d\n",(int32)st.st_mode);
  printf("此文件的硬链接数：%d\n",(int32)st.st_nlink);
  printf("此文件的所有者ID：%d\n",(int32)st.st_uid);
  printf("此文件的所有者的组ID：%d\n",(int32)st.st_gid);
  printf("设备ID（如果此文件为特殊设备）：%d\n",(int32)st.st_rdev);
  printf("此文件的大小：%d\n",(int32)st.st_size);
  printf("此文件的所在文件系统块大小：%d\n",(int32)st.st_blksize);
  printf("此文件的占用块数量：%d\n",(int32)st.st_blocks);
  printf("此文件的最后访问时间：%d\n",(int32)st.st_atime);
  printf("此文件的最后修改时间：%d\n",(int32)st.st_mtime);
  printf("此文件的最后状态改变时间：%d\n",(int32)st.st_ctime);
  
 	
	return TASK_OK;
}

/*
头文件：#include <unistd.h>    #include <sys/mman.h>

定义函数：void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offsize);

函数说明：mmap()用来将某个文件内容映射到内存中，对该内存区域的存取即是直接对该文件内容的读写。

参数	                   说明
start	            指向欲对应的内存起始地址，通常设为NULL，代表让系统自动选定地址，对应成功后该地址会返回。
length	          代表将文件中多大的部分对应到内存。
prot	            代表映射区域的保护方式，有下列组合：
                  PROT_EXEC  映射区域可被执行；
                  PROT_READ  映射区域可被读取；
                  PROT_WRITE  映射区域可被写入；
                  PROT_NONE  映射区域不能存取。
                  
flags	            会影响映射区域的各种特性：
                  MAP_FIXED  如果参数 start 所指的地址无法成功建立映射时，
                    则放弃映射，不对地址做修正。通常不鼓励用此旗标。
                  MAP_SHARED  对应射区域的写入数据会复制回文件内，
                    而且允许其他映射该文件的进程共享。
                  MAP_PRIVATE  对应射区域的写入操作会产生一个映射文件的复制，
                    即私人的"写入时复制" (copy on write)对此区域作的任何修改都不会写回原来的文件内容。
                  MAP_ANONYMOUS  建立匿名映射，此时会忽略参数fd，不涉及文件，
                    而且映射区域无法和其他进程共享。
                  MAP_DENYWRITE  只允许对应射区域的写入操作，
                    其他对文件直接写入的操作将会被拒绝。
                  MAP_LOCKED  将映射区域锁定住，这表示该区域不会被置换(swap)。

                  在调用mmap()时必须要指定MAP_SHARED 或MAP_PRIVATE。
                  
fd	              open()返回的文件描述词，代表欲映射到内存的文件。

offset	          文件映射的偏移量，通常设置为0，代表从文件最前方开始对应，
                  offset必须是分页大小的整数倍。

返回值：若映射成功则返回映射区的内存起始地址，否则返回MAP_FAILED(-1)，错误原因存于errno 中。

错误代码：
EBADF  参数fd 不是有效的文件描述词。
EACCES  存取权限有误。如果是MAP_PRIVATE 情况下文件必须可读，使用MAP_SHARED 则要有PROT_WRITE 以及该文件要能写入。
EINVAL  参数start、length 或offset 有一个不合法。
EAGAIN  文件被锁住，或是有太多内存被锁住。
ENOMEM  内存不足。

总结一下，可能产生的问题如下：

　　1.进行文件映射的描述符必须拥有读权限，否则会产生SIGSEGV信号
　　2.把内存内容写入映射文件时，必须确保被写文件当前位置到文件结尾的长度不小于所写内容长度，
    否则产生SIGBUS信号
　　3.关闭文件描述符并不能保证文件内容不被修改
　　4.munmap并不能使映射的内容写回磁盘

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
  
  fstat(fileNode, &sb); /* 取得文件大小 */
  
  start = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fileNode, 0);
  if(start == MAP_FAILED) /* 判断是否映射成功 */
  {
    LOG_ERROR("mmap file error\n");
    return TASK_ERROR;
  }

  memcpy(buf, start, sb.st_size);
  
  munmap(start, sb.st_size); /* 解除映射 */
  
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
  if(start == MAP_FAILED) /* 判断是否映射成功 */
  {
    LOG_ERROR("mmap file error\n");
    return TASK_ERROR;
  }

  memcpy(start, buf, sizeof(buf));
  
  munmap(start, sizeof(buf)); /* 解除映射 */
  
  close(fileNode);

  return sizeof(buf);
}

