/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */
#include "ipcUtils.h"
#include "logprint.h"

/*************message part **********/
#define MAX_TXT  1024

#define IPC_MSG_PROCESS   1
#define IPC_MSG_LAST      2

typedef struct
{
  int64 msgType;
  int8  txt[MAX_TXT];
} ipcMsg_t;// 消息格式，与接收端一致

typedef struct ipcMsgHeadInfo
{ 
  int32   checkNum;
  int32   status;
  int32   len;
}ipcMsgHeadInfo_t;

typedef struct ipcMessageSendInfo
{
  int64               msgType;
  ipcMsgHeadInfo_t    head;
  uint8               info[MAX_TXT];
}ipcMessageSendInfo_t;

typedef struct ipcMessageRevInfo
{
  int64               msgType;
  ipcMsgHeadInfo_t    head;
  uint8               info[0];
}ipcMessageRevInfo_t;


/*
linux下进程间通信的几种主要手段简介：
管道（Pipe）及有名管道（named pipe）：管道可用于具有亲缘关系进程间的通信，有名管道克服了管道没有名字的限制，因此，除具有管道所具有的功能外，
  它还允许无亲缘关系进程间的通信；
  
信号（Signal）：信号是比较复杂的通信方式，用于通知接受进程有某种事件发生，除了用于进程间通信外，进程还可以发送信号给进程本身；
  linux除了支持Unix早期信号语义函数sigal外，还支持语义符合Posix.1标准的信号函数sigaction（实际上，该函数是基于BSD的，BSD为了实现可靠信号机制，
  又能够统一对外接口，用sigaction函数重新实现了signal函数）；
  
报文（Message）队列（消息队列）：消息队列是消息的链接表，包括Posix消息队列system V消息队列。有足够权限的进程可以向队列中添加消息，
  被赋予读权限的进程则可以读走队列中的消息。消息队列克服了信号承载信息量少，管道只能承载无格式字节流以及缓冲区大小受限等缺点。
  
共享内存：使得多个进程可以访问同一块内存空间，是最快的可用IPC形式。是针对其他通信机制运行效率较低而设计的。往往与其它通信机制，
  如信号量结合使用，来达到进程间的同步及互斥。
  
信号量（semaphore）：主要作为进程间以及同一进程不同线程之间的同步手段。

套接口（Socket）：更为一般的进程间通信机制，可用于不同机器之间的进程间通信。起初是由Unix系统的BSD分支开发出来的，
  但现在一般可以移植到其它类Unix系统上：Linux和System V的变种都支持套接字。
*/

/*
linux 消息队列

  消息队列（也叫做报文队列）能够克服早期unix通信机制的一些缺点。作为早期unix通信机制之一的信号能够传送的信息量有限，
后来虽然POSIX 1003.1b在信号的实时性方面作了拓广，使得信号在传递信息量方面有了相当程度的改进，但是信号这种通信方式更像"即时"的通信方式，
它要求接受信号的进程在某个时间范围内对信号做出反应，因此该信号最多在接受信号进程的生命周期内才有意义，
信号所传递的信息是接近于随进程持续的概念（process-persistent），
见 附录 1；管道及有名管道及有名管道则是典型的随进程持续IPC，并且，只能传送无格式的字节流无疑会给应用程序开发带来不便，
另外，它的缓冲区大小也受到限制。
  消息队列就是一个消息的链表。可以把消息看作一个记录，具有特定的格式以及特定的优先级。对消息队列有写权限的进程可以向中按照一定的规则添加新消息；
对消息队列有读权限的进程则可以从消息队列中读走消息。消息队列是随内核持续的（参见 附录 1）。
  目前主要有两种类型的消息队列：POSIX消息队列以及系统V消息队列，系统V消息队列目前被大量使用。考虑到程序的可移植性，
新开发的应用程序应尽量使用POSIX消息队列。

一、消息队列基本概念

  系统V消息队列是随内核持续的，只有在内核重起或者显示删除一个消息队列时，该消息队列才会真正被删除。
因此系统中记录消息队列的数据结构（struct ipc_ids msg_ids）位于内核中，系统中的所有消息队列都可以在结构msg_ids中找到访问入口。

  消息队列就是一个消息的链表。每个消息队列都有一个队列头，用结构struct msg_queue来描述（参见 附录 2）。队列头中包含了该消息队列的大量信息，
包括消息队列键值、用户ID、组ID、消息队列中消息数目等等，甚至记录了最近对消息队列读写进程的ID。读者可以访问这些信息，也可以设置其中的某些信息。
 
  struct ipc_ids msg_ids是内核中记录消息队列的全局数据结构；struct msg_queue是每个消息队列的队列头。
  
二、操作消息队列
对消息队列的操作无非有下面三种类型：

1、 打开或创建消息队列 
消息队列的内核持续性要求每个消息队列都在系统范围内对应唯一的键值，所以，要获得一个消息队列的描述字，只需提供该消息队列的键值即可；
注：消息队列描述字是由在系统范围内唯一的键值生成的，而键值可以看作对应系统内的一条路经。

2、 读写操作
消息读写操作非常简单，对开发人员来说，每个消息都类似如下的数据结构：
  struct msgbuf{
  long mtype;
  char mtext[1];
  };
  
mtype成员代表消息类型，从消息队列中读取消息的一个重要依据就是消息的类型；mtext是消息内容，当然长度不一定为1。
因此，对于发送消息来说，首先预置一个msgbuf缓冲区并写入消息类型和内容，调用相应的发送函数即可；对读取消息来说，
首先分配这样一个msgbuf缓冲区，然后把消息读入该缓冲区即可。

3、 获得或设置消息队列属性：
消息队列的信息基本上都保存在消息队列头中，因此，可以分配一个类似于消息队列头的结构(struct msqid_ds，见 附录 2)，
来返回消息队列的属性；同样可以设置该数据结构。

消息队列API

1、文件名到键值
#include <sys/types.h>
#include <sys/ipc.h>
key_t ftok (char*pathname, char proj)；

它返回与路径pathname相对应的一个键值。该函数不直接对消息队列操作，但在调用ipc(MSGGET,…)或msgget()来获得消息队列描述字前，
往往要调用该函数。典型的调用代码是：
key=ftok(path_ptr, 'a');
    ipc_id=ipc(MSGGET, (int)key, flags,0,NULL,0);
    …
    
2、linux为操作系统V进程间通信的三种方式（消息队列、信号灯、共享内存区）提供了一个统一的用户界面：
int ipc(unsigned int call, int first, int second, int third, void * ptr, long fifth);
第一个参数指明对IPC对象的操作方式，对消息队列而言共有四种操作：MSGSND、MSGRCV、MSGGET以及MSGCTL，分别代表向消息队列发送消息、
从消息队列读取消息、打开或创建消息队列、控制消息队列；first参数代表唯一的IPC对象；下面将介绍四种操作。
int ipc( MSGGET, intfirst, intsecond, intthird, void*ptr, longfifth); 
与该操作对应的系统V调用为：int msgget( (key_t)first，second)。
int ipc( MSGCTL, intfirst, intsecond, intthird, void*ptr, longfifth) 
与该操作对应的系统V调用为：int msgctl( first，second, (struct msqid_ds*) ptr)。
int ipc( MSGSND, intfirst, intsecond, intthird, void*ptr, longfifth); 
与该操作对应的系统V调用为：int msgsnd( first, (struct msgbuf*)ptr, second, third)。
int ipc( MSGRCV, intfirst, intsecond, intthird, void*ptr, longfifth); 
与该操作对应的系统V调用为：int msgrcv( first，(struct msgbuf*)ptr, second, fifth,third)，
注：本人不主张采用系统调用ipc()，而更倾向于采用系统V或者POSIX进程间通信API。原因如下：
虽然该系统调用提供了统一的用户界面，但正是由于这个特性，它的参数几乎不能给出特定的实际意义（如以first、second来命名参数），在一定程度上造成开发不便。
正如ipc手册所说的：ipc()是linux所特有的，编写程序时应注意程序的移植性问题；
该系统调用的实现不过是把系统V IPC函数进行了封装，没有任何效率上的优势；
系统V在IPC方面的API数量不多，形式也较简洁。

3.系统V消息队列API
系统V消息队列API共有四个，使用时需要包括几个头文件：
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

1）int msgget(key_t key, int msgflg)
参数key是一个键值，由ftok获得；msgflg参数是一些标志位。该调用返回与健值key相对应的消息队列描述字。
在以下两种情况下，该调用将创建一个新的消息队列：
如果没有消息队列与健值key相对应，并且msgflg中包含了IPC_CREAT标志位；
key参数为IPC_PRIVATE；
参数msgflg可以为以下：IPC_CREAT、IPC_EXCL、IPC_NOWAIT或三者的或结果。
调用返回：成功返回消息队列描述字，否则返回-1。
注：参数key设置成常数IPC_PRIVATE并不意味着其他进程不能访问该消息队列，只意味着即将创建新的消息队列。

2）int msgrcv(int msqid, struct msgbuf *msgp, int msgsz, long msgtyp, int msgflg);
该系统调用从msgid代表的消息队列中读取一个消息，并把消息存储在msgp指向的msgbuf结构中。
msqid为消息队列描述字；消息返回后存储在msgp指向的地址，
msgsz指定msgbuf的mtext成员的长度（即消息内容的长度），
msgtyp为请求读取的消息类型；读消息标志msgflg可以为以下几个常值的或：
  IPC_NOWAIT 如果没有满足条件的消息，调用立即返回，此时，errno=ENOMSG
  IPC_EXCEPT 与msgtyp>0配合使用，返回队列中第一个类型不为msgtyp的消息
  IPC_NOERROR 如果队列中满足条件的消息内容大于所请求的msgsz字节，则把该消息截断，截断部分将丢失。
msgrcv手册中详细给出了消息类型取不同值时(>0; <0; =0)，调用将返回消息队列中的哪个消息。
msgrcv()解除阻塞的条件有三个：
消息队列中有了满足条件的消息；
msqid代表的消息队列被删除；
调用msgrcv（）的进程被信号中断；
调用返回：成功返回读出消息的实际字节数，否则返回-1。

3）int msgsnd(int msqid, struct msgbuf *msgp, int msgsz, int msgflg);
向msgid代表的消息队列发送一个消息，即将发送的消息存储在msgp指向的msgbuf结构中，消息的大小由msgze指定。
对发送消息来说，有意义的msgflg标志为IPC_NOWAIT，指明在消息队列没有足够空间容纳要发送的消息时，msgsnd是否等待。造成msgsnd()等待的条件有两种：
当前消息的大小与当前消息队列中的字节数之和超过了消息队列的总容量；
当前消息队列的消息数（单位"个"）不小于消息队列的总容量（单位"字节数"），此时，虽然消息队列中的消息数目很多，但基本上都只有一个字节。
msgsnd()解除阻塞的条件有三个：
不满足上述两个条件，即消息队列中有容纳该消息的空间；
msqid代表的消息队列被删除；
调用msgsnd（）的进程被信号中断；
调用返回：成功返回0，否则返回-1。

4）int msgctl(int msqid, int cmd, struct msqid_ds *buf);
该系统调用对由msqid标识的消息队列执行cmd操作，共有三种cmd操作：IPC_STAT、IPC_SET 、IPC_RMID。
IPC_STAT：该命令用来获取消息队列信息，返回的信息存贮在buf指向的msqid结构中；
IPC_SET：该命令用来设置消息队列的属性，要设置的属性存储在buf指向的msqid结构中；可设置属性包括：
msg_perm.uid、msg_perm.gid、msg_perm.mode以及msg_qbytes，同时，也影响msg_ctime成员。
IPC_RMID：删除msqid标识的消息队列；
调用返回：成功返回0，否则返回-1。

三、消息队列的限制
每个消息队列的容量（所能容纳的字节数）都有限制，该值因系统不同而不同。在后面的应用实例中，输出了redhat 8.0的限制，结果参见 附录 3。
另一个限制是每个消息队列所能容纳的最大消息数：在redhad 8.0中，该限制是受消息队列容量制约的：消息个数要小于消息队列的容量（字节数）。
注：上述两个限制是针对每个消息队列而言的，系统对消息队列的限制还有系统范围内的最大消息队列个数，以及整个系统范围内的最大消息数。
一般来说，实际开发过程中不会超过这个限制。

小结：
消息队列与管道以及有名管道相比，具有更大的灵活性，首先，它提供有格式字节流，有利于减少开发人员的工作量；
其次，消息具有类型，在实际应用中，可作为优先级使用。这两点是管道以及有名管道所不能比的。同样，消息队列可以在几个进程间复用，
而不管这几个进程是否具有亲缘关系，这一点与有名管道很相似；但消息队列是随内核持续的，与有名管道（随进程持续）相比，生命力更强，应用空间更大。
*/

/*
消息队列API

创建新消息队列或取得已存在消息队列
#include <sys/msg.h> ------------------------------------
int msgget(key_t key, int msgflg);
参数	解释
key	键值，每个消息对了key值不同，可以使用ftok生成对于的key
msgflg	IPC_CREAT: 如果没有该队列，则创建该队列。如果该队列已经存在，返回该队列ID.IPC_CREAT & IPC_EXCL: 如果该队列不存在创建，如果存在返回失败EEXIST.

向队列读或者写数据
#include <sys/msg.h>
-----------------------------------------
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp,int msgflg);
参数	解释
msqid	消息队列的描述符ID
msgp	执行消息缓冲区的指针，用来存储消息。格式如下：
msgsz	消息的大小
msgflg	IPC_NOWAIT: 如果消息队列中没有数据，则立刻返回不用等待。MSG_NOERROR:如果消息队列长度大于msgsz,截断消息。
struct msgbuf 
 {
      long mtype;       //message type, must be > 0
      char mtext[1];    //message data
 };
 
msgtyp:  信息類型。 取值如下：
  msgtyp = 0 ，不分類型，直接返回消息隊列中的第一項
  msgtyp > 0 ,返回第一項 msgtyp與 msgbuf結構體中的mtype相同的信息
  msgtyp <0 , 返回第一項 mtype小於等於msgtyp絕對值的信息

msgflg:取值如下：
IPC_NOWAIT ,不阻塞
IPC_NOERROR ，若信息長度超過参數msgsz，則截斷信息而不報錯。

设置消息队列属性
#include <sys/msg.h> ---------------------
int msgctl(int msqid, int cmd, struct msqid_ds *buf);
参数	解释
msgid	消息队列id
cmd	队列执行的命令
bug	执行msqid_ds的指针
进一步对cmd名字做详细解释

cmd	解释
IPC_STAT	取得此队列的msqid_ds结构，存在在buf指向的结构中。
IPC_SET	该命令用来设置消息队列的属性，要设置的属性存储在buf中。
IPC_RMID	从内核中删除 msqid 标识的消息队列。
kernel关于IPC参数
名称	含义
auto_msgmni	根据系统memory增加，移除或者namespace创建，移除自动获取msgmni的值
msgmni	该文件指定消息队列标识的最大数目，即系统范围内最大多少个消息队列。
msgmnb	该文件指定一个消息队列的最大长度（bytes）。
msgmax	该文件指定了从一个进程发送到另一个进程的消息的最大长度（bytes）。
以上信息可以根据ipcs -l获得系统的信息

------ Messages: Limits --------
max queues system wide = 2779
max size of message (bytes) = 8192
default max size of queue (bytes) = 16384
根据kernel.txt可以得知，auto_msgmni的值默认是1，当设置为0的时候，就不能自动获取msgmni
*/

/*
 系统建立IPC通讯（如消息队列、共享内存时）必须指定一个ID值。通常情况下，该id值通过ftok函数得到。
ftok原型如下：
key_t ftok( char * fname, int id )

fname就时你指定的文件名(该文件必须是存在而且可以访问的)，id是子序号，虽然为int，但是只有8个比特被使用(0-255)。

当成功执行的时候，一个key_t值将会被返回，否则 -1 被返回。

   在一般的UNIX实现中，是将文件的索引节点号取出，前面加上子序号得到key_t的返回值。如指定文件的索引节点号为65538，换算成16进制为 0x010002，
   而你指定的ID值为38，换算成16进制为0x26，则最后的key_t返回值为0x26010002。
   
查询文件索引节点号的方法是： ls -i

ftok的陷阱
  根据pathname指定的文件（或目录）名称，以及proj_id参数指定的数字，ftok函数为IPC对象生成一个唯一性的键值。
在实际应用中，很容易产生的一个理解是，在proj_id相同的情况下，只要文件（或目录）名称不变，就可以确保ftok返回始终一致的键值。
然而，这个理解并非完全正确，有可能给应用开发埋下很隐晦的陷阱。因为ftok的实现存在这样的风险，
即在访问同一共享内存的多个进程先后调用ftok函数的时间段中，如果pathname指定的文件（或目录）被删除且重新创建，
则文件系统会赋予这个同名文件（或目录）新的i节点信息，于是这些进程所调用的ftok虽然都能正常返回，但得到的键值却并不能保证相同。
由此可能造成的后果是，原本这些进程意图访问一个相同的共享内存对象，然而由于它们各自得到的键值不同，实际上进程指向的共享内存不再一致；
如果这些共享内存都得到创建，则在整个应用运行的过程中表面上不会报出任何错误，然而通过一个共享内存对象进行数据传输的目的将无法实现。
*/

static int32 getCheckSum(const int8 *pack, int32 packLen)
{
  int32  checkSum = 0;
  
  while (--packLen >= 0)
  {
    checkSum += *pack++;
  }
  
  return checkSum;
}

int32 ipcSendbyMsg(int8 key, int8 *data, int32 len, int32 type)
{
  int32 msgId;
  int32 running = 1;
  ipcMessageSendInfo_t msgBuffer;
  int32 i = 0;
  
  msgId = msgget((key_t)key, 0666 | IPC_CREAT);//创建消息标识符key = 1234的消息队列。如果该队列已经存在，则直接返回该队列的标识符，以便向该消息队列收发消息
  if (msgId < 0)
  {
    LOG_ERROR("msgget failed with error\n");
    return TASK_ERROR;
  }

  while(running)
  {
    memset(&msgBuffer, 0x0, sizeof(ipcMsg_t));
     
    msgBuffer.msgType = type; //类型填充，在本例中没有特别含义
    
    if (len > MAX_TXT)
    {
      memcpy(msgBuffer.info, &data[i*MAX_TXT], MAX_TXT);
      msgBuffer.head.len = MAX_TXT;
      msgBuffer.head.status = IPC_MSG_PROCESS;
      msgBuffer.head.checkNum = getCheckSum(msgBuffer.info, msgBuffer.head.len);
      
      i++;
      len = len - MAX_TXT;
      if (msgsnd(msgId, (void *)&msgBuffer, MAX_TXT + sizeof(ipcMsgHeadInfo_t), 0) < 0) 
      {
        LOG_ERROR("msgget send  error\n");
        return TASK_ERROR;          
      }
    }
    else
    {
      memcpy(msgBuffer.info, &data[i*MAX_TXT], len);
      msgBuffer.head.len = len;
      msgBuffer.head.status = IPC_MSG_LAST;
      msgBuffer.head.checkNum = getCheckSum(msgBuffer.info, msgBuffer.head.len);
      if (msgsnd(msgId, (void *)&msgBuffer, len + sizeof(ipcMsgHeadInfo_t), 0) < 0) 
      {
        LOG_ERROR("msgget send  error\n");
        return TASK_ERROR;          
      }

      running = 0;
    }
  }

  return TASK_OK;
}

int32 ipcRevbyMsg(int8 key, int8 *data, int32 *len, int32 revType)
{
  int32 msgId;
  int32 running = 1;
  ipcMessageRevInfo_t msgBuffer;
  int32 i = 0;
  int64 msgRevLen = 0;
  
  msgId = msgget((key_t)key, 0666 | IPC_CREAT);//创建消息标识符key = 1234的消息队列。如果该队列已经存在，则直接返回该队列的标识符，以便向该消息队列收发消息
  if (msgId < 0)
  {
    LOG_ERROR("msgget failed with error\n");
    return TASK_ERROR;
  }

  while(running)
  {
    memset(&msgBuffer, 0x0, sizeof(ipcMsg_t));
    if (msgrcv(msgId, (void *)&msgBuffer, MAX_TXT + sizeof(ipcMsgHeadInfo_t),0, 0) < 0) 
    { //从消息队列接收消息，如果接收失败执行if语句并退出
      LOG_ERROR("msgget send  error\n");
      return TASK_ERROR; 
    }

    if (msgBuffer.head.checkNum != getCheckSum(msgBuffer.info, msgBuffer.head.len))
    {
      LOG_ERROR("check sum  error\n");
      return TASK_ERROR; 
    }

    if (msgBuffer.msgType = revType)
    {
      LOG_ERROR("rev type error\n");
      return TASK_ERROR; 
    }
    
    if (msgBuffer.head.status == IPC_MSG_PROCESS)
    {
      memcpy( &data[i*MAX_TXT], msgBuffer.info, msgBuffer.head.len);
      i++;
    }
    else if (msgBuffer.head.status == IPC_MSG_LAST)
    {
      memcpy( &data[i*MAX_TXT], msgBuffer.info, msgBuffer.head.len);
      running = 0;
    }
    else
    {
      LOG_ERROR("data status error\n");
      return TASK_ERROR; 
    }
  }

  if(msgctl(msgId,IPC_RMID,0)< 0)
	{
    LOG_ERROR("msgctl(IPC_RMID) error\n");
    return TASK_ERROR; 
	}
    
  return TASK_OK;
}

/* 
无名管道
1.1 管道相关的关键概念
  管道是Linux支持的最初Unix IPC形式之一，具有以下特点：
  管道是半双工的，数据只能向一个方向流动；需要双方通信时，需要建立起两个管道；
  只能用于父子进程或者兄弟进程之间（具有亲缘关系的进程）；
  单独构成一种独立的文件系统：管道对于管道两端的进程而言，就是一个文件，但它不是普通的文件，它不属于某种文件系统，
  而是自立门户，单独构成一种文件系统，并且只存在与内存中。
  数据的读出和写入：一个进程向管道中写的内容被管道另一端的进程读出。写入的内容每次都添加在管道缓冲区的末尾，
  并且每次都是从缓冲区的头部读出数据。

1.2管道的创建：
  #include <unistd.h>
  int pipe(int fd[2])
  该函数创建的管道的两端处于一个进程中间，在实际应用中没有太大意义，因此，一个进程在由pipe()创建管道后，
  一般再fork一个子进程，然后通过管道实现父子进程间的通信（因此也不难推出，只要两个进程中存在亲缘关系，
  这里的亲缘关系指的是具有共同的祖先，都可以采用管道方式来进行通信）。

1.3管道的读写规则：
  管道两端可分别用描述字fd[0]以及fd[1]来描述，需要注意的是，管道的两端是固定了任务的。即一端只能用于读，由描述字fd[0]表示，
  称其为管道读端；另一端则只能用于写，由描述字fd[1]来表示，称其为管道写端。如果试图从管道写端读取数据，
  或者向管道读端写入数据都将导致错误发生。一般文件的I/O函数都可以用于管道，如close、read、write等等。

  从管道中读取数据：
  *如果管道的写端不存在，则认为已经读到了数据的末尾，读函数返回的读出字节数为0；
  *当管道的写端存在时，如果请求的字节数目大于PIPE_BUF，则返回管道中现有的数据字节数，如果请求的字节数目不大于PIPE_BUF，
  则返回管道中现有数据字节数（此时，管道中数据量小于请求的数据量）；或者返回请求的字节数（此时，管道中数据量不小于请求的数据量）。
  
  注：（PIPE_BUF在include/linux/limits.h中定义，不同的内核版本可能会有所不同。Posix.1要求PIPE_BUF至少为512字节，
  red hat 7.2中为4096）。

  向管道中写入数据：对管道的写规则的验证1：写端对读端存在的依赖性
  向管道中写入数据时，linux将不保证写入的原子性，管道缓冲区一有空闲区域，写进程就会试图向管道写入数据。
  如果读进程不读走管道缓冲区中的数据，那么写操作将一直阻塞。 
  
  注：只有在管道的读端存在时，向管道中写入数据才有意义。否则，向管道中写入数据的进程将收到内核传来的SIFPIPE信号，
  应用程序可以处理该信号，也可以忽略（默认动作则是应用程序终止）。

  Broken pipe,原因就是该管道以及它的所有fork()产物的读端都已经被关闭。如果在父进程中保留读端，即在写完pipe后，
  再关闭父进程的读端，也会正常写入pipe，读者可自己验证一下该结论。因此，在向管道写入数据时，至少应该存在某一个进程，
  其中管道读端没有被关闭，否则就会出现上述错误（管道断裂,进程收到了SIGPIPE信号，默认动作是进程终止）

  对管道的写规则的验证2：linux不保证写管道的原子性验证
  
1.5管道的局限性
  管道的主要局限性正体现在它的特点上：
  只支持单向数据流；
  只能用于具有亲缘关系的进程之间；
  没有名字；
  管道的缓冲区是有限的（管道制存在于内存中，在管道创建时，为缓冲区分配一个页面大小）；
  管道所传送的是无格式字节流，这就要求管道的读出方和写入方必须事先约定好数据的格式，比如多少字节算作一个消息（或命令、或记录）等等；  

结论：
写入数目小于4096时写入是非原子的！ 
如果把父进程中的两次写入字节数都改为5000，则很容易得出下面结论： 
写入管道的数据量大于4096字节时，缓冲区的空闲空间将被写入数据（补齐），直到写完所有数据为止，如果没有进程读数据，则一直阻塞。

#example:

#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
int main(int argc,char *argv[])
{
	pid_t pid;
	int temp;
	int pipedes[2];
	char s[14]="test message!";
	char d[14];
	if((pipe(pipedes))==-1)
	{
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	
	if((pid=fork())==-1)
	{
		perror("fork");
		exit(EXIT_FAILURE);
	}
	else if(pid==0)
	{
		printf("now,write data to pipe\n");
		if((write(pipedes[1],s,14))==-1)
		{
			perror("write");
			exit(EXIT_FAILURE);
		}
		else
		{
			printf("the written data is :%s\n",s);
			exit(EXIT_SUCCESS);
		}
	}	
	else if(pid>0)
	{
		sleep(2);
		printf("now,read data from pipe\n");
		if(read(pipedes[0],d,14)==-1)
		{
			perror("read");
			exit(EXIT_FAILURE);
		}
		printf("the data from pipe is :%s\n",d);
	}
	return 0;
}

*/

/*
有名管道概述及相关API应用
2.1 有名管道相关的关键概念
  管道应用的一个重大限制是它没有名字，因此，只能用于具有亲缘关系的进程间通信，在有名管道（named pipe或FIFO）提出后，
  该限制得到了克服。FIFO不同于管道之处在于它提供一个路径名与之关联，以FIFO的文件形式存在于文件系统中。这样，
  即使与FIFO的创建进程不存在亲缘关系的进程，只要可以访问该路径，就能够彼此通过FIFO相互通信
  （能够访问该路径的进程以及FIFO的创建进程之间），因此，通过FIFO不相关的进程也能交换数据。
  值得注意的是，FIFO严格遵循先进先出（first in first out），对管道及FIFO的读总是从开始处返回数据，
  对它们的写则把数据添加到末尾。它们不支持诸如lseek()等文件定位操作。
  
2.2有名管道的创建
#include <sys/types.h>
#include <sys/stat.h>
  int mkfifo(const char * pathname, mode_t mode)
  该函数的第一个参数是一个普通的路径名，也就是创建后FIFO的名字。第二个参数与打开普通文件的open()函数中的mode 参数相同。 
  如果mkfifo的第一个参数是一个已经存在的路径名时，会返回EEXIST错误，所以一般典型的调用代码首先会检查是否返回该错误，
  如果确实返回该错误，那么只要调用打开FIFO的函数就可以了。一般文件的I/O函数都可以用于FIFO，如close、read、write等等。

2.3有名管道的打开规则
  有名管道比管道多了一个打开操作：open。
  FIFO的打开规则：
  如果当前打开操作是为读而打开FIFO时，若已经有相应进程为写而打开该FIFO，则当前打开操作将成功返回；
  否则，可能阻塞直到有相应进程为写而打开该FIFO（当前打开操作设置了阻塞标志）；或者，成功返回（当前打开操作没有设置阻塞标志）。
  如果当前打开操作是为写而打开FIFO时，如果已经有相应进程为读而打开该FIFO，则当前打开操作将成功返回；
  否则，可能阻塞直到有相应进程为读而打开该FIFO（当前打开操作设置了阻塞标志）；或者，
  返回ENXIO错误（当前打开操作没有设置阻塞标志）。

2.4有名管道的读写规则
  从FIFO中读取数据：
  约定：如果一个进程为了从FIFO中读取数据而阻塞打开FIFO，那么称该进程内的读操作为设置了阻塞标志的读操作。
  如果有进程写打开FIFO，且当前FIFO内没有数据，则对于设置了阻塞标志的读操作来说，将一直阻塞。
  对于没有设置阻塞标志读操作来说则返回-1，当前errno值为EAGAIN，提醒以后再试。
  对于设置了阻塞标志的读操作说，造成阻塞的原因有两种：当前FIFO内有数据，但有其它进程在读这些数据；
  另外就是FIFO内没有数据。解阻塞的原因则是FIFO中有新的数据写入，不论信写入数据量的大小，也不论读操作请求多少数据量。
  读打开的阻塞标志只对本进程第一个读操作施加作用，如果本进程内有多个读操作序列，则在第一个读操作被唤醒并完成读操作后，
  其它将要执行的读操作将不再阻塞，即使在执行读操作时，FIFO中没有数据也一样（此时，读操作返回0）。
  如果没有进程写打开FIFO，则设置了阻塞标志的读操作会阻塞。
  注：如果FIFO中有数据，则设置了阻塞标志的读操作不会因为FIFO中的字节数小于请求读的字节数而阻塞，此时，
  读操作会返回FIFO中现有的数据量。
  
  向FIFO中写入数据：
  约定：如果一个进程为了向FIFO中写入数据而阻塞打开FIFO，那么称该进程内的写操作为设置了阻塞标志的写操作。
  对于设置了阻塞标志的写操作：
  当要写入的数据量不大于PIPE_BUF时，linux将保证写入的原子性。如果此时管道空闲缓冲区不足以容纳要写入的字节数，
  则进入睡眠，直到当缓冲区中能够容纳要写入的字节数时，才开始进行一次性写操作。
  
  当要写入的数据量大于PIPE_BUF时，linux将不再保证写入的原子性。FIFO缓冲区一有空闲区域，
  写进程就会试图向管道写入数据，写操作在写完所有请求写的数据后返回。
  
  对于没有设置阻塞标志的写操作：
  当要写入的数据量大于PIPE_BUF时，linux将不再保证写入的原子性。在写满所有FIFO空闲缓冲区后，写操作返回。
  当要写入的数据量不大于PIPE_BUF时，linux将保证写入的原子性。如果当前FIFO空闲缓冲区能够容纳请求写入的字节数，
  写完后成功返回；如果当前FIFO空闲缓冲区不能够容纳请求写入的字节数，则返回EAGAIN错误，提醒以后再写；

不管写打开的阻塞标志是否设置，在请求写入的字节数大于4096时，都不保证写入的原子性。但二者有本质区别：
对于阻塞写来说，写操作在写满FIFO的空闲区域后，会一直等待，直到写完所有数据为止，请求写入的数据最终都会写入FIFO；
而非阻塞写则在写满FIFO的空闲区域后，就返回(实际写入的字节数)，所以有些数据最终不能够写入。
对于读操作的验证则比较简单，不再讨论。
*/


int32 ipcSendbyFifo(int8 *dir, int8 *data, int32 len, int32 type)
{
  int32 msgId;
  int32 running = 1;
  ipcMessageSendInfo_t msgBuffer;
  int32 i = 0;
  int32 ret = 0;
  
  if (access(dir, F_OK) < 0) 
	{
    ret = mkfifo(dir, 0766);
    if (ret < 0) 
		{
      LOG_ERROR("msgget failed with error\n");
      return TASK_ERROR;
    }
  }

  msgId = open(dir, O_WRONLY);
  if (msgId < 0) 
	{
    LOG_ERROR("fifo open failed with error\n");
    return TASK_ERROR;
  }
  
  while(running)
  {
    memset(&msgBuffer, 0x0, sizeof(ipcMsg_t));
     
    msgBuffer.msgType = type; //类型填充，在本例中没有特别含义
    
    if (len > MAX_TXT)
    {
      memcpy(msgBuffer.info, &data[i*MAX_TXT], MAX_TXT);
      msgBuffer.head.len = MAX_TXT;
      msgBuffer.head.status = IPC_MSG_PROCESS;
      msgBuffer.head.checkNum = getCheckSum(msgBuffer.info, msgBuffer.head.len);
      
      i++;
      len = len - MAX_TXT;
      if (write(msgId, (int8 *)&msgBuffer, sizeof(ipcMessageSendInfo_t)) < 0) 
      {
        LOG_ERROR("msgget send  error\n");
        close(msgId);
        return TASK_ERROR;          
      }
    }
    else
    {
      memcpy(msgBuffer.info, &data[i*MAX_TXT], len);
      msgBuffer.head.len = len;
      msgBuffer.head.status = IPC_MSG_LAST;
      msgBuffer.head.checkNum = getCheckSum(msgBuffer.info, msgBuffer.head.len);
      if (write(msgId, (int8 *)&msgBuffer, len + sizeof(msgBuffer.msgType) + sizeof(msgBuffer.head)) < 0) 
      {
        LOG_ERROR("msgget send  error\n");
        close(msgId);
        return TASK_ERROR;          
      }

      running = 0;
    }
  }

  close(msgId);
  return TASK_OK;
}


int32 ipcRevbyFifo(int8 *dir, int8 *data, int32 *len, int32 revType)
{
  int32 msgId;
  int32 running = 1;
  ipcMessageRevInfo_t msgBuffer;
  int32 i = 0;
  int64 msgRevLen = 0;
  
  msgId = open(dir, O_RDONLY);
  if (msgId < 0)
  {
    LOG_ERROR("msgget failed with error\n");
    return TASK_ERROR;
  }

  while(running)
  {
    memset(&msgBuffer, 0x0, sizeof(ipcMsg_t));
    if (read(msgId, (int8 *)&msgBuffer, MAX_TXT + sizeof(ipcMsgHeadInfo_t) + sizeof(int64)) < 0) 
    { 
      LOG_ERROR("msgget send  error\n");
      return TASK_ERROR; 
    }

    if (msgBuffer.head.checkNum != getCheckSum(msgBuffer.info, msgBuffer.head.len))
    {
      LOG_ERROR("check sum  error\n");
      return TASK_ERROR; 
    }

    if (msgBuffer.msgType = revType)
    {
      LOG_ERROR("rev type error\n");
      return TASK_ERROR; 
    }
    
    if (msgBuffer.head.status == IPC_MSG_PROCESS)
    {
      memcpy( &data[i*MAX_TXT], msgBuffer.info, msgBuffer.head.len);
      i++;
    }
    else if (msgBuffer.head.status == IPC_MSG_LAST)
    {
      memcpy( &data[i*MAX_TXT], msgBuffer.info, msgBuffer.head.len);
      running = 0;
    }
    else
    {
      LOG_ERROR("data status error\n");
      return TASK_ERROR; 
    }
  }

  return TASK_OK;
}

/*
linux 共享内存

  用共享内存通信的一个显而易见的好处是效率高，因为进程可以直接读写内存，而不需要任何数据的拷贝。
对于像管道和消息队列等通信方式，则需要在内核和用户空间进行四次的数据拷贝，而共享内存则只拷贝两次数据[1]：
一次从输入文件到共享内存区，另一次从共享内存区到输出文件。实际上，进程之间在共享内存时，并不总是读写少量数据后就解除映射，
有新的通信时，再重新建立共享内存区域。而是保持共享区域，直到通信完毕为止，这样，数据内容一直保存在共享内存中，并没有写回文件。共享内存中的内容往往是在解除映射时才写回文件的。因此，采用共享内存的通信方式效率是非常高的。
Linux的2.2.x内核支持多种共享内存方式，如mmap()系统调用，Posix共享内存，以及系统V共享内存。
linux发行版本如Redhat 8.0支持mmap()系统调用及系统V共享内存，但还没实现Posix共享内存，
本文将主要介绍mmap()系统调用及系统V共享内存API的原理及应用。

一、内核怎样保证各个进程寻址到同一个共享内存区域的内存页面
1、page cache及swap cache中页面的区分：一个被访问文件的物理页面都驻留在page cache或swap cache中，一个页面的所有信息由struct page来描述。struct page中有一个域为指针mapping ，它指向一个struct address_space类型结构。page cache或swap cache中的所有页面就是根据address_space结构以及一个偏移量来区分的。
2、文件与address_space结构的对应：一个具体的文件在打开后，内核会在内存中为之建立一个struct inode结构，其中的i_mapping域指向一个address_space结构。这样，一个文件就对应一个address_space结构，一个address_space与一个偏移量能够确定一个page cache 或swap cache中的一个页面。因此，当要寻址某个数据时，很容易根据给定的文件及数据在文件内的偏移量而找到相应的页面。
3、进程调用mmap()时，只是在进程空间内新增了一块相应大小的缓冲区，并设置了相应的访问标识，但并没有建立进程空间到物理页面的映射。因此，第一次访问该空间时，会引发一个缺页异常。
4、对于共享内存映射情况，缺页异常处理程序首先在swap cache中寻找目标页（符合address_space以及偏移量的物理页），如果找到，则直接返回地址；如果没有找到，则判断该页是否在交换区(swap area)，如果在，则执行一个换入操作；如果上述两种情况都不满足，处理程序将分配新的物理页面，并把它插入到page cache中。进程最终将更新进程页表。 
注：对于映射普通文件情况（非共享映射），缺页异常处理程序首先会在page cache中根据address_space以及数据偏移量寻找相应的页面。如果没有找到，则说明文件数据还没有读入内存，处理程序会从磁盘读入相应的页面，并返回相应地址，同时，进程页表也会更新。
5、所有进程在映射同一个共享内存区域时，情况都一样，在建立线性地址与物理地址之间的映射之后，不论进程各自的返回地址如何，实际访问的必然是同一个共享内存区域对应的物理页面。 
注：一个共享内存区域可以看作是特殊文件系统shm中的一个文件，shm的安装点在交换区上。
上面涉及到了一些数据结构，围绕数据结构理解问题会容易一些。

二、mmap()及其相关系统调用
mmap()系统调用使得进程之间通过映射同一个普通文件实现共享内存。普通文件被映射到进程地址空间后，
进程可以向访问普通内存一样对文件进行访问，不必再调用read()，write（）等操作。
注：实际上，mmap()系统调用并不是完全为了用于共享内存而设计的。它本身提供了不同于一般对普通文件的访问方式，
进程可以像读写内存一样对普通文件的操作。而Posix或系统V的共享内存IPC则纯粹用于共享目的，当然mmap()实现共享内存也是其主要应用之一。
1、mmap()系统调用形式如下：。

void* mmap ( void * addr , size_t len , int prot , int flags , int fd , off_t offset ) 
参数fd为即将映射到进程空间的文件描述字，一般由open()返回，同时，fd可以指定为-1，此时须指定flags参数中的MAP_ANON，
表明进行的是匿名映射（不涉及具体的文件名，避免了文件的创建及打开，很显然只能用于具有亲缘关系的进程间通信）。len是映射到调用进程地址空间的字节数，它从被映射文件开头offset个字节开始算起。prot 参数指定共享内存的访问权限。可取如下几个值的或：PROT_READ（可读） , PROT_WRITE （可写）, PROT_EXEC （可执行）, PROT_NONE（不可访问）。flags由以下几个常值指定：MAP_SHARED , MAP_PRIVATE , MAP_FIXED，其中，MAP_SHARED , MAP_PRIVATE必选其一，而MAP_FIXED则不推荐使用。offset参数一般设为0，表示从文件头开始映射。参数addr指定文件应被映射到进程空间的起始地址，一般被指定一个空指针，此时选择起始地址的任务留给内核来完成。函数的返回值为最后文件映射到进程空间的地址，进程可直接操作起始地址为该值的有效地址。这里不再详细介绍mmap()的参数，读者可参考mmap()手册页获得进一步的信息。
2、系统调用mmap()用于共享内存的两种方式：
（1）使用普通文件提供的内存映射：适用于任何进程之间； 此时，需要打开或创建一个文件，然后再调用mmap()；典型调用代码如下：
	fd=open(name, flag, mode);
if(fd<0)
	...
ptr=mmap(NULL, len , PROT_READ|PROT_WRITE, MAP_SHARED , fd , 0); 通过mmap()实现共享内存的通信方式有许多特点和要注意的地方，
我们将在范例中进行具体说明。
（2）使用特殊文件提供匿名内存映射：适用于具有亲缘关系的进程之间； 由于父子进程特殊的亲缘关系，
在父进程中先调用mmap()，然后调用fork()。那么在调用fork()之后，子进程继承父进程匿名映射后的地址空间，
同样也继承mmap()返回的地址，这样，父子进程就可以通过映射区域进行通信了。注意，这里不是一般的继承关系。一般来说，
子进程单独维护从父进程继承下来的一些变量。而mmap()返回的地址，却由父子进程共同维护。 
对于具有亲缘关系的进程实现共享内存最好的方式应该是采用匿名内存映射的方式。此时，不必指定具体的文件，
只要设置相应的标志即可，参见范例2。

3、系统调用munmap()
int munmap( void * addr, size_t len ) 
该调用在进程地址空间中解除一个映射关系，addr是调用mmap()时返回的地址，len是映射区的大小。当映射关系解除后，
对原来映射地址的访问将导致段错误发生。
4、系统调用msync()
int msync ( void * addr , size_t len, int flags) 
一般说来，进程在映射空间的对共享内容的改变并不直接写回到磁盘文件中，往往在调用munmap（）后才执行该操作。
可以通过调用msync()实现磁盘上文件内容与共享内存区的内容一致。

*/

int32 ipcMapMem(int8 *dir, int32 len, shmFunctionRun shmFunctionRun)
{
  int32 fd = -1;
  int8 *mem = NULL;
  
  fd=open(dir,O_CREAT|O_RDWR|O_TRUNC,00777);
  if (fd < 0)
  {
    LOG_ERROR("ipc map open error\n");
    return TASK_ERROR;
  }

  mem = (int8*)mmap( NULL,len,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0 );

  shmFunctionRun(mem, len);
  
  munmap( mem, len);
  close(fd);

  return TASK_OK;
}

/*
系统调用mmap()通过映射一个普通文件实现共享内存。系统V则是通过映射特殊文件系统shm中的文件实现进程间的共享内存通信。
也就是说，每个共享内存区域对应特殊文件系统shm中的一个文件（这是通过shmid_kernel结构联系起来的），后面还将阐述。

1、系统V共享内存原理
进程间需要共享的数据被放在一个叫做IPC共享内存区域的地方，所有需要访问该共享区域的进程都要把该共享区域映射到本进程的地址空间中去。系统V共享内存通过shmget获得或创建一个IPC共享内存区域，并返回相应的标识符。内核在保证shmget获得或创建一个共享内存区，初始化该共享内存区相应的shmid_kernel结构注同时，还将在特殊文件系统shm中，创建并打开一个同名文件，并在内存中建立起该文件的相应dentry及inode结构，新打开的文件不属于任何一个进程（任何进程都可以访问该共享内存区）。所有这一切都是系统调用shmget完成的。
注：每一个共享内存区都有一个控制结构struct shmid_kernel，shmid_kernel是共享内存区域中非常重要的一个数据结构，它是存储管理和文件系统结合起来的桥梁，定义如下：
struct shmid_kernel // private to the kernel 
{	
	struct kern_ipc_perm	shm_perm;
	struct file *		shm_file;
	int			id;
	unsigned long		shm_nattch;
	unsigned long		shm_segsz;
	time_t			shm_atim;
	time_t			shm_dtim;
	time_t			shm_ctim;
	pid_t			shm_cprid;
	pid_t			shm_lprid;
};
该结构中最重要的一个域应该是shm_file，它存储了将被映射文件的地址。
每个共享内存区对象都对应特殊文件系统shm中的一个文件，一般情况下，
特殊文件系统shm中的文件是不能用read()、write()等方法访问的，当采取共享内存的方式把其中的文件映射到进程地址空间后，
可直接采用访问内存的方式对其访问。

正如消息队列和信号灯一样，内核通过数据结构struct ipc_ids shm_ids维护系统中的所有共享内存区域。
对于系统V共享内存区来说，kern_ipc_perm的宿主是shmid_kernel结构，shmid_kernel是用来描述一个共享内存区域的，
这样内核就能够控制系统中所有的共享区域。同时，在shmid_kernel结构的file类型指针shm_file指向文件系统shm中相应的文件，
这样，共享内存区域就与shm文件系统中的文件对应起来。在创建了一个共享内存区域后，还要将它映射到进程地址空间，
系统调用shmat()完成此项功能。由于在调用shmget()时，已经创建了文件系统shm中的一个同名文件与共享内存区域相对应，
因此，调用shmat()的过程相当于映射文件系统shm中的同名文件过程，原理与mmap()大同小异。

2、系统V共享内存API
对于系统V共享内存，主要有以下几个API：shmget()、shmat()、shmdt()及shmctl()。
#include <sys/ipc.h>
#include <sys/shm.h>
shmget（）用来获得共享内存区域的ID，如果不存在指定的共享区域就创建相应的区域。
shmat()把共享内存区域映射到调用进程的地址空间中去，这样，进程就可以方便地对共享区域进行访问操作。
shmdt()调用用来解除进程对共享内存区域的映射。shmctl实现对共享内存区域的控制操作。这里我们不对这些系统调用作具体的介绍，
        读者可参考相应的手册页面，后面的范例中将给出它们的调用方法。
注：shmget的内部实现包含了许多重要的系统V共享内存机制；shmat在把共享内存区域映射到进程空间时，
    并不真正改变进程的页表。当进程第一次访问内存映射区域访问时，会因为没有物理页表的分配而导致一个缺页异常，
    然后内核再根据相应的存储管理机制为共享内存映射区域分配相应的页表。

3、系统V共享内存限制
在/proc/sys/kernel/目录下，记录着系统V共享内存的一下限制，如一个共享内存区的最大字节数shmmax，
系统范围内最大共享内存区标识符数shmmni等，可以手工对其调整，但不推荐这样做。

通过对试验结果分析，对比系统V与mmap()映射普通文件实现共享内存通信，可以得出如下结论：

1、	系统V共享内存中的数据，从来不写入到实际磁盘文件中去；
    而通过mmap()映射普通文件实现的共享内存通信可以指定何时将数据写入磁盘文件中。 
    注：前面讲到，系统V共享内存机制实际是通过映射特殊文件系统shm中的文件实现的，文件系统shm的安装点在交换分区上，
    系统重新引导后，所有的内容都丢失。
2、	系统V共享内存是随内核持续的，即使所有访问共享内存的进程都已经正常终止，共享内存区仍然存在（除非显式删除共享内存），
    在内核重新引导之前，对该共享内存区域的任何改写操作都将一直保留。
3、	通过调用mmap()映射普通文件进行进程间通信时，一定要注意考虑进程何时终止对通信的影响。
    而通过系统V共享内存实现通信的进程则不然。 注：这里没有给出shmctl的使用范例，原理与消息队列大同小异。

结论：
  共享内存允许两个或多个进程共享一给定的存储区，因为数据不需要来回复制，所以是最快的一种进程间通信机制。
共享内存可以通过mmap()映射普通文件（特殊情况下还可以采用匿名映射）机制实现，也可以通过系统V共享内存机制实现。
应用接口和原理很简单，内部机制复杂。为了实现更安全通信，往往还与信号灯等同步机制共同使用。

  共享内存涉及到了存储管理以及文件系统等方面的知识，深入理解其内部机制有一定的难度，关键还要紧紧抓住内核使用的重要数据结构。
系统V共享内存是以文件的形式组织在特殊文件系统shm中的。通过shmget可以创建或获得共享内存的标识符。
取得共享内存标识符后，要通过shmat将这个内存区映射到本进程的虚拟地址空间。

共享内存使用步骤

shmget() 根据key创建或者获取共享内存的shmid
shmat() 连接attach，把共享内存映射到本进程的地址空间，允许本进程使用共享内存
读写共享内存...
shmdt() 脱离detach，禁止本进程使用共享内存
shmctl(sid,IPC_RMID,0) 标记删除共享内存。也可以使用ipcrm命令删除

int shmget(key_t key, size_t size, int shmflg);

参数：
key：进程间事先约定的key，或者调用key_t ftok( char * fname, int id )获取
size：内存大小，字节。（当创建一个新的共享内存区时，size 的值必须大于 0 ；如果是访问一个已经存在的内存共享区，则 size 可以是 0 。）
shmflg：标志位IPC_CREATE、IPC_EXCL

IPC_CREATE : 调用 shmget 时，系统将此值与其他共享内存区的 key 进行比较。如果存在相同的 key ，说明共享内存区已存在，此时返回该共享内存区的shmid；如果不存在，则新建一个共享内存区并返回其shmid。
IPC_EXCL : 该宏必须和 IPC_CREATE 一起使用，否则没意义。当 shmflg 取 IPC_CREATE | IPC_EXCL 时，表示如果发现内存区已经存在则返回 -1，错误代码为 EEXIST 。
返回值：成功返回shmid；失败返回-1

一般我们创建共享内存的时候会在第一个进程中使用shmget来创建共享内存。调用int shmid = shmget(key, size, IPC_CREATE|0666);
而在其它进程中使用shmget和同样的key来获取到这个已经创建了的共享内存。也还是调用int shmid = shmget(key, size, IPC_CREATE|0666);

void *shmat(int shmid, const void *shmaddr, int shmflg);

参数：
shmid：shmget的返回值
shmaddr：共享内存连接到本进程后在本进程地址空间的内存地址。如果为NULL，则由内核选择一块空闲的地址。如果非空，则由该参数指定地址。一般传入NULL。
shmflg：一般为 0 ，则不设置任何限制权限。

SHM_RDONLY 只读
SHM_RND 如果指定了shmaddr，那么设置该标志位后，连接的内存地址会对SHMLBA向下取整对齐。
SHM_REMAP take-over region on attach  不清楚
返回值：成功后返回一个指向共享内存区的指针；否则返回 (void *) -1

通常使用 void ptr = shmat(shmid, NULL,0);

int shmdt(const void *shmaddr);

参数：
shmaddr：shmat 函数的返回值
返回值：成功返回0；失败返回-1

调用shmdt后，共享内存的shmid_ds结构会变化。其中shm_dtime指向当前时间，shm_lpid设置为当前pid，shm_nattch减一。如果shm_nattch变为0，并且该共享内存已被标记为删除，那么该共享内存会被删除。（也就是说，光调用shmdt，但是没调用删除，就不会删除。并且，只调用删除，没调用shmdt使得nattch=0，也不会删除）

fork()后，子进程继承父进程连接的共享内存。
execve()后，子进程detach所有连接的共享内存。
_exit()，detach所有共享内存。

最好不要依赖进程退出来detach，应该由应用程序调用shmdt。因为如果进程异常退出，系统不会detach。

使用ipcs命令可以一直看到挂接数为0的共享内存，这时候用相同的ID去挂接还是能挂接上的，说明这个共享内存一直是存在的。需要调用shmctl(sid,IPC_RMID,0)或者执行ipcrm删除。否则会导致相关的内存泄漏。

其他

int shmctl(int shmid, int cmd, struct shmid_ds *buf);
参数：
cmd

IPC_STAT 把shmid_ds从内核中复制到用户缓存
IPC_SET
IPC_RMID 标记删除。只有当nattch=0时才会真正删除。被标记删除的共享内存shm_perm.mode是SHM_DEST。 常用shmctl(sid,IPC_RMID,0)
IPC_INFO Linux特有。返回系统范围内的共享内存设置
SHM_INFO, SHM_STAT, SHM_LOCK, SHM_UNLOCK为Linux特有。

*/
int32 ipcShmMem(int32 key, int32 len, shmFunctionRun shmFunctionRun)
{
  int32 shmId = 0;
  int8 *shmMem = NULL;
  int32 ret;
  
	shmId=shmget((key_t)key,(size_t)len,0600|IPC_CREAT);
	if(shmId < 0)
	{
    LOG_ERROR("shmget   error\n");
    return TASK_ERROR;
	}
  
  shmMem = shmat(shmId,NULL,0);
	if(shmMem == NULL)
	{
    LOG_ERROR("shmMem attach error\n");
    return TASK_ERROR;
	}

  shmFunctionRun(shmMem, len);

  shmdt(shmMem);
	if(shmctl(shmId,IPC_RMID,0) < 0)
	{
    LOG_ERROR("shmMem attach error\n");
    return TASK_ERROR;
	}

  return TASK_OK;
}

