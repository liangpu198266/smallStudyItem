#include "netUtils.h"
#include "logprint.h"

typedef struct netServerContext_
{
    int32 cliCnt;                 /*客户端个数*/
    fd_set allfds;              /*句柄集合*/
    int32 maxfd;                  /*句柄最大值*/
}netServerContext_t;

typedef enum
{
  NET_DEFAULT_MODE = 0,
  NET_MULTI_MODE
}netSockCtlOpt_t; 

typedef enum
{
  NET_TCP_SOCK_CTL = 0,
  NET_TCP_SOCK_SET,       /*设置*/
  NET_TCP_SOCK_CLR        /*清除*/
}netSockTcpOpt_t; 

/*
五种I/O 模式：
【1】 阻塞 I/O (Linux下的I/O操作默认是阻塞I/O，即open和socket创建的I/O都是阻塞I/O)
【2】 非阻塞 I/O (可以通过fcntl或者open时使用O_NONBLOCK参数，将fd设置为非阻塞的I/O)
【3】 I/O 多路复用 (I/O多路复用，通常需要非阻塞I/O配合使用)
【4】 信号驱动 I/O (SIGIO)
【5】 异步 I/O

select I/O模型是一种异步I/O模型，在单线程中Linux/WinNT默认支持64个客户端套接字。

socket 编程几个核心概念:

阻塞: blocking
非阻塞: non-blocking
同步: synchronous
异步: asynchronous

  1. 同步，就是我客户端（c端调用者）调用一个功能，该功能没有结束前，我（c端调用者）死等结果。
  2. 异步，就是我（c端调用者）调用一个功能，不需要知道该功能结果，该功能有结果后通知我（c端调用者）即回调通知。
  同步/异步主要针对C端, 但是跟S端不是完全没有关系，同步/异步机制必须S端配合才能实现.同步/异步是由c端自己控制,但是S端是否阻塞/非阻塞, C端完全不需要关心.
  3. 阻塞，      就是调用我（s端被调用者，函数），我（s端被调用者，函数）没有接收完数据或者没有得到结果之前，我不会返回。
  4. 非阻塞，  就是调用我（s端被调用者，函数），我（s端被调用者，函数）立即返回，通过select通知调用者

IO模式:

  IO 读两阶段:
    等待数据准备 (Waiting for the data to be ready)
    将数据从内核拷贝到进程中 (Copying the data from the kernel to the process)
    
  由此产生的5种网络模式:
    阻塞 I/O（blocking IO）
    非阻塞 I/O（nonblocking IO）
    I/O 多路复用（ IO multiplexing）
    信号驱动 I/O（ signal driven IO）: 不常用,略过.
    异步 I/O（asynchronous IO）

IO多路复用是指内核一旦发现进程指定的一个或者多个IO条件准备读取，它就通知该进程。IO多路复用适用如下场合：
（1）当客户处理多个描述字时（一般是交互式输入和网络套接口），必须使用I/O复用。
（2）当一个客户同时处理多个套接口时，而这种情况是可能的，但很少出现。
（3）如果一个TCP服务器既要处理监听套接口，又要处理已连接套接口，一般也要用到I/O复用。
（4）如果一个服务器即要处理TCP，又要处理UDP，一般要使用I/O复用。
（5）如果一个服务器要处理多个服务或多个协议，一般要使用I/O复用。
与多进程和多线程技术相比，I/O多路复用技术的最大优势是系统开销小，系统不必创建进程/线程，也不必维护这些进程/线程，从而大大减小了系统的开销。

核心总结:

  阻塞 I/O（blocking IO）: IO 两阶段都阻塞
  非阻塞 I/O（nonblocking IO）:
      命名不精确, 精确定义应为: 部分阻塞,部分非阻塞
      
  I/O 多路复用（ IO multiplexing）:
      常说的 select，poll，epoll，有些地方也称这种IO方式为event driven IO
      
      关于 select:
        select 相当于一个代理中介, 进程在调用 select()函数时,被 block,
        而之后 select(poll, epoll 等)函数 会不断的轮询所负责的所有socket，当某个socket有数据到达了，就通知用户进程。
      select/epoll的优势并不是对于单个连接能处理得更快，而是在于能处理更多的连接。
      如果处理的连接数不是很高的话，使用select/epoll的web server不一定比使用multi-threading + blocking IO的web server性能更好，可能延迟还更大。

  异步 I/O（asynchronous IO）:
      linux下的asynchronous IO其实用得很少

*/

/*
socket函数的三个参数分别为：

domain：即协议域，又称为协议族（family）。常用的协议族有，AF_INET、AF_INET6、AF_LOCAL（或称AF_UNIX，Unix域socket）、AF_ROUTE等等。
        协议族决定了socket的地址类型，在通信中必须采用对应的地址，如AF_INET决定了要用ipv4地址（32位的）与端口号（16位的）的组合、
        AF_UNIX决定了要用一个绝对路径名作为地址。
type：指定socket类型。常用的socket类型有，SOCK_STREAM、SOCK_DGRAM、SOCK_RAW、SOCK_PACKET、SOCK_SEQPACKET等等（socket的类型有哪些？）。
protocol：故名思意，就是指定协议。常用的协议有，IPPROTO_TCP、IPPTOTO_UDP、IPPROTO_SCTP、IPPROTO_TIPC等，
          它们分别对应TCP传输协议、UDP传输协议、STCP传输协议、TIPC传输协议（这个协议我将会单独开篇讨论！）。
*/

/*
  linux下的socket INADDR_ANY表示的是一个服务器上所有的网卡（服务器可能不止一个网卡）多个本地ip地址都进行绑定端口号，进行侦听。
INADDR_ANY就是指定地址为0.0.0.0的地址,这个地址事实上表示不确定地址,或“所有地址”、“任意地址”。 一般来说，在各个系统中均定义成为0值。

  对于客户端如果绑定INADDR_ANY，情况类似。对于TCP而言，在connect()系统调用时将其绑顶到一具体的IP地址。
选择的依据是该地址所在 子网到目标地址是可达的（reachable). 这时通过getsockname（）系统调用就能得知具体使用哪一个地址。
对于UDP而言, 情况比较特殊。即使使用connect()系统调用也不会绑定到一具体地址。
这是因为对UDP使用connect()并不会真正向目标地址发送任何建立连 接的数据，也不会验证到目标地址的可达性。
它只是将目标地址的信息记录在内部的socket数据结构之中，共以后使用。
只有当调用 sendto()/send()时,由系统内核根据路由表决定由哪一个地址（网卡）发送UDP packet.

INADDR_ANY是ANY，是绑定地址0.0.0.0上的监听, 能收到任意一块网卡的连接；
INADDR_LOOPBACK, 也就是绑定地址LOOPBAC, 往往是127.0.0.1, 只能收到127.0.0.1上面的连接请求

*/

/*
listen()、connect()函数
  如果作为一个服务器，在调用socket()、bind()之后就会调用listen()来监听这个socket，如果客户端这时调用connect()发出连接请求，服务器端就会接收到这个请求。

int listen(int sockfd, int backlog);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

listen函数的第一个参数即为要监听的socket描述字，第二个参数为相应socket可以排队的最大连接个数。
socket()函数创建的socket默认是一个主动类型的，listen函数将socket变为被动类型的，等待客户的连接请求。

connect函数的第一个参数即为客户端的socket描述字，第二参数为服务器的socket地址，第三个参数为socket地址的长度。
客户端通过调用connect函数来建立与TCP服务器的连接。

accept()函数

TCP服务器端依次调用socket()、bind()、listen()之后，就会监听指定的socket地址了。
TCP客户端依次调用socket()、connect()之后就想TCP服务器发送了一个连接请求。
TCP服务器监听到这个请求之后，就会调用accept()函数取接收请求，这样连接就建立好了。
之后就可以开始网络I/O操作了，即类同于普通文件的读写I/O操作。

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

accept函数的第一个参数为服务器的socket描述字，第二个参数为指向struct sockaddr *的指针，用于返回客户端的协议地址，
第三个参数为协议地址的长度。如果accpet成功，那么其返回值是由内核自动生成的一个全新的描述字，代表与返回客户的TCP连接。

注意：accept的第一个参数为服务器的socket描述字，是服务器开始调用socket()函数生成的，称为监听socket描述字；
而accept函数返回的是已连接的socket描述字。一个服务器通常通常仅仅只创建一个监听socket描述字，它在该服务器的生命周期内一直存在。
内核为每个由服务器进程接受的客户连接创建了一个已连接socket描述字，当服务器完成了对某个客户的服务，相应的已连接socket描述字就被关闭。

select : 返回：做好准备的文件描述符的个数，超时为0，错误为 -1.

选项	参数 optval 类型	说明
SO_ACCEPTCONN	int	返回信息指示该套接字是否能监听，仅 getsockopt
SO_BROADCAST	int	如果 *optval 非0，广播数据包
SO_DEBUG	int	如果 *optval 非0，启动网络驱动调试功能
SO_DONTROUTE	int	如果 *optval 非0，绕过通常路由
SO_ERROR	int	返回挂起的套接字错误并清除，仅 getsockopt
SO_KEEPALIVE	int	如果 *optval 非0，启动周期性keep-alive消息
SO_LINGER	struct linger	当有未发送消息并且套接字关闭时，延迟时间
SO_OOBINLINE	int	如果 *optval 非0，将带外数据放到普通数据中
SO_RCVBUF	int	接收缓冲区的字节大小
SO_RCVLOWAT	int	接收调用中返回的数据最小字节数
SO_RCVTIMEO	struct timeval	套接字接收调用的超时值
SO_REUSEADDR	int	如果 *optval 非0，重用 bind 中的地址
SO_SNDBUF	int	发送缓冲区的字节大小
SO_SNDLOWAT	int	发送调用中发送的数据最小字节数
SO_SNDTIMEO	struct timeval	套接字发送调用的超时值
SO_TYPE	int	标识套接字类型，仅 getsockopt

*/

/*
inet_aton和inet_network和inet_addr三者比较

三者定义：

int inet_aton(const char *cp, struct in_addr *inp);//返回网络字节序

in_addr_t inet_addr(const char *cp);//返回网路字节序

in_addr_t inet_network(const char *cp);//返回主机字节序

  inet_addr和inet_network函数都是用于将字符串形式转换为整数形式用的，两者区别很小，
  inet_addr返回的整数形式是网 络字节序，而inet_network返回的整数形式是主机字节序。
（你一定会纳闷，为什么函数叫inet_network，却返回的是主机字节序，呵 呵，就是这么奇怪，你又有什么办法呢…）其他地方两者并无二异。
他俩都有一个小缺陷，那就是当IP是255.255.255.255时，这两个“小子” （对这两个函数的昵称，请谅解…^_^）会认为这是个无效的IP地址，
这是历史遗留问题，其实在目前大部分的路由器上，这个 255.255.255.255的IP都是有效的。

  inet_aton函数和上面这俩小子的区别就是在于他认为255.255.255.255是有效的，他不会冤枉这个看似特殊的IP地址。
  所以我们 建议你多多支持这个函数，那两个小子还是少用为好:)对了，inet_aton函数返回的是网络字节序的IP地址。
*/

/*
select、poll、epoll详解

IO多路复用是指内核一旦发现进程指定的一个或者多个IO条件准备读取，它就通知该进程。IO多路复用适用如下场合：

        当客户处理多个描述符时（一般是交互式输入和网络套接口），必须使用I/O复用。

        当一个客户同时处理多个套接口时，而这种情况是可能的，但很少出现。

        如果一个TCP服务器既要处理监听套接口，又要处理已连接套接口，一般也要用到I/O复用。

        如果一个服务器即要处理TCP，又要处理UDP，一般要使用I/O复用。

        如果一个服务器要处理多个服务或多个协议，一般要使用I/O复用。


与多进程和多线程技术相比，I/O多路复用技术的最大优势是系统开销小，系统不必创建进程/线程，也不必维护这些进程/线程，从而大大减小了系统的开销。

  目前支持I/O多路复用的系统调用有 select，pselect，poll，epoll，I/O多路复用就是通过一种机制，
一个进程可以监视多个描述符，一旦某个描述符就绪（一般是读就绪或者写就绪），
能够通知程序进行相应的读写操作。但select，pselect，poll，epoll本质上都是同步I/O，
因为他们都需要在读写事件就绪后自己负责进行读写，也就是说这个读写过程是阻塞的，而异步I/O则无需自己负责进行读写，
异步I/O的实现会负责把数据从内核拷贝到用户空间。

poll
基本原理：

    poll本质上和select没有区别，它将用户传入的数组拷贝到内核空间，然后查询每个fd对应的设备状态，如果设备就绪则在设备等待队列中加入一项并继续遍历，如果遍历完所有fd后没有发现就绪设备，则挂起当前进程，直到设备就绪或者主动超时，被唤醒后它又要再次遍历fd。这个过程经历了多次无谓的遍历。

它没有最大连接数的限制，原因是它是基于链表来存储的，但是同样有一个缺点：

        大量的fd的数组被整体复制于用户态和内核地址空间之间，而不管这样的复制是不是有意义。

        poll还有一个特点是“水平触发”，如果报告了fd后，没有被处理，那么下次poll时会再次报告该fd。

注意：

    从上面看，select和poll都需要在返回后，通过遍历文件描述符来获取已经就绪的socket。事实上，同时连接的大量客户端在一时刻可能只有很少的处于就绪状态，因此随着监视的描述符数量的增长，其效率也会线性下降。

---------------------------------------------------------------
epoll

epoll是在2.6内核中提出的，是之前的select和poll的增强版本。相对于select和poll来说，epoll更加灵活，没有描述符限制。epoll使用一个文件描述符管理多个描述符，将用户关系的文件描述符的事件存放到内核的一个事件表中，这样在用户空间和内核空间的copy只需一次。

基本原理：

    epoll支持水平触发和边缘触发，最大的特点在于边缘触发，它只告诉进程哪些fd刚刚变为就绪态，并且只会通知一次。还有一个特点是，epoll使用“事件”的就绪通知方式，通过epoll_ctl注册fd，一旦该fd就绪，内核就会采用类似callback的回调机制来激活该fd，epoll_wait便可以收到通知。

epoll的优点：

        没有最大并发连接的限制，能打开的FD的上限远大于1024（1G的内存上能监听约10万个端口）。

        效率提升，不是轮询的方式，不会随着FD数目的增加效率下降。只有活跃可用的FD才会调用callback函数；即Epoll最大的优点就在于它只管你“活跃”的连接，而跟连接总数无关，因此在实际的网络环境中，Epoll的效率就会远远高于select和poll。

        内存拷贝，利用mmap()文件映射内存加速与内核空间的消息传递；即epoll使用mmap减少复制开销。

epoll对文件描述符的操作有两种模式：LT（level trigger）和ET（edge trigger）。LT模式是默认模式，LT模式与ET模式的区别如下：

    LT模式：当epoll_wait检测到描述符事件发生并将此事件通知应用程序，应用程序可以不立即处理该事件。下次调用epoll_wait时，会再次响应应用程序并通知此事件。

    ET模式：当epoll_wait检测到描述符事件发生并将此事件通知应用程序，应用程序必须立即处理该事件。如果不处理，下次调用epoll_wait时，不会再次响应应用程序并通知此事件。

    LT模式

        LT(level triggered)是缺省的工作方式，并且同时支持block和no-block socket。在这种做法中，内核告诉你一个文件描述符是否就绪了，然后你可以对这个就绪的fd进行IO操作。如果你不作任何操作，内核还是会继续通知你的。

    ET模式

        ET(edge-triggered)是高速工作方式，只支持no-block socket。在这种模式下，当描述符从未就绪变为就绪时，内核通过epoll告诉你。然后它会假设你知道文件描述符已经就绪，并且不会再为那个文件描述符发送更多的就绪通知，直到你做了某些操作导致那个文件描述符不再为就绪状态了(比如，你在发送，接收或者接收请求，或者发送接收的数据少于一定量时导致了一个EWOULDBLOCK 错误）。但是请注意，如果一直不对这个fd作IO操作(从而导致它再次变成未就绪)，内核不会发送更多的通知(only once)。

        ET模式在很大程度上减少了epoll事件被重复触发的次数，因此效率要比LT模式高。epoll工作在ET模式的时候，必须使用非阻塞套接口，以避免由于一个文件句柄的阻塞读/阻塞写操作把处理多个文件描述符的任务饿死。

    在select/poll中，进程只有在调用一定的方法后，内核才对所有监视的文件描述符进行扫描，而epoll事先通过epoll_ctl()来注册一个文件描述符，一旦基于某个文件描述符就绪时，内核会采用类似callback的回调机制，迅速激活这个文件描述符，当进程调用epoll_wait()时便得到通知。(此处去掉了遍历文件描述符，而是通过监听回调的的机制。这正是epoll的魅力所在。)

注意：

    如果没有大量的idle-connection或者dead-connection，epoll的效率并不会比select/poll高很多，但是当遇到大量的idle-connection，就会发现epoll的效率大大高于select/poll。

综上，在选择select，poll，epoll时要根据具体的使用场合以及这三种方式的自身特点：

        表面上看epoll的性能最好，但是在连接数少并且连接都十分活跃的情况下，select和poll的性能可能比epoll好，毕竟epoll的通知机制需要很多函数回调。

        select低效是因为每次它都需要轮询。但低效也是相对的，视情况而定，也可通过良好的设计改善。

*/

/******************************************************************/
/************************tcp part**********************************/
/******************************************************************/

static int32 netTcpSockInitServer(int32 port, int32 mode, uint8 *ipAddr)
{
  int32 ret = 0;
  int32 listenfd;
  struct sockaddr_in servaddr;
  int32 opt = 1;

  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0) 
  {
    displayErrorMsg("tcp failed to create sock\n");
    return TASK_ERROR; 
  }

  if (mode == NET_MULTI_MODE)
  {
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) // 设置套接字地址可重用
    {
      displayErrorMsg("tcp failed to create sock\n");
      return TASK_ERROR; 
    }
  }
  
  bzero(&servaddr, sizeof(servaddr));

  servaddr.sin_family = PF_INET;
  servaddr.sin_port = htons(port);

  if (ipAddr == NULL)
  {
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  else
  {
    ret = inet_aton(ipAddr, (struct in_addr *)&servaddr.sin_addr.s_addr);
  }
  
  if (servaddr.sin_addr.s_addr == 0)
  {
    LOG_ERROR("error ip address error\n");
    close(listenfd);
    return TASK_ERROR;
  }
  
  ret = bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
  if(ret < 0)
  {
    displayErrorMsg("tcp failed to bind sock\n");
    close(listenfd);
    return TASK_ERROR; 
  }

  ret = listen(listenfd, 1);
  if(ret < 0)
  {
    displayErrorMsg("tcp failed to listen sock\n");
    close(listenfd);
    return TASK_ERROR; 
  }

  LOG_DBG("tcp socket init ok\n");
  return listenfd;
}

/*
FD_ZERO(fd_set *fdset)：清空fdset与所有文件句柄的联系。
FD_SET(int fd, fd_set *fdset)：建立文件句柄fd与fdset的联系。
FD_CLR(int fd, fd_set *fdset)：清除文件句柄fd与fdset的联系。
FD_ISSET(int fd, fdset *fdset)：检查fdset联系的文件句柄fd是否可读写，>0表示可读写。

	
#include <sys/select.h>   
    int select(int maxfdp, fd_set *readset, fd_set *writeset, fd_set *exceptset,struct timeval *timeout);
    
  int maxfdp是一个整数值，是指集合中所有文件描述符的范围，即所有文件描述符的最大值加1，不能错！
  在Windows中这个参数的值无所谓，可以设置不正确。 
    
  中间的三个参数 readset, writset, exceptset,指向描述符集。
  这些参数指明了我们关心哪些描述符，和需要满足什么条件(可写，可读，异常)。
  一个文件描述集保存在 fd_set 类型中。fd_set类型变量每一位代表了一个描述符。

理解select模型：
理解select模型的关键在于理解fd_set,为说明方便，取fd_set长度为1字节，fd_set中的每一bit可以对应一个文件描述符fd。
则1字节长的fd_set最大可以对应8个fd。
（1）执行fd_set set;FD_ZERO(&set);则set用位表示是0000,0000。
（2）若fd＝5,执行FD_SET(fd,&set);后set变为0001,0000(第5位置为1)
（3）若再加入fd＝2，fd=1,则set变为0001,0011
（4）执行select(6,&set,0,0,0)阻塞等待
（5）若fd=1,fd=2上都发生可读事件，则select返回，此时set变为0000,0011。注意：没有事件发生的fd=5被清空。

基于上面的讨论，可以轻松得出select模型的特点：
（1)可监控的文件描述符个数取决与sizeof(fd_set)的值。我这边服务器上sizeof(fd_set)＝512，每bit表示一个文件描述符，
则我服务器上支持的最大文件描述符是512*8=4096。据说可调，另有说虽然可调，但调整上限受于编译内核时的变量值。
（2）将fd加入select监控集的同时，还要再使用一个数据结构array保存放到select监控集中的fd，一是用于再select返回后，
array作为源数据和fd_set进行FD_ISSET判断。二是select返回后会把以前加入的但并无事件发生的fd清空，
则每次开始 select前都要重新从array取得fd逐一加入（FD_ZERO最先），扫描array的同时取得fd最大值maxfd，用于select的第一个参数。
（3）可见select模型必须在select前循环array（加fd，取maxfd），select返回后循环array（FD_ISSET判断是否有时间发生）。

*/


/*
Linux:C/Socket多路复用select 小全

循环服务器:UDP服务器 
   socket(...);
   bind(...);
   while(1)
    {
         recvfrom(...);
         process(...);
         sendto(...);
   }

循环服务器:TCP服务器 

   socket(...);
   bind(...);
   listen(...);
   while(1)
   {
           accept(...);
           while(1)
           {
                   read(...);
                   process(...);
                   write(...);
           }
           close(...);
   }

并发服务器:TCP服务器

  socket(...);
  bind(...);
  listen(...);
  while(1)
  {
        accept(...);
        if(fork(..)==0)
          {
              while(1)
               {        
                read(...);
                process(...);
                write(...);
               }
           close(...);
           exit(...);
          }
        close(...);
  }    

并发服务器:多路复用I/O,阻塞
  select默认就是阻塞式的，只有关注的描述符有效时，才会被唤醒执行相应的操作

  初始话(socket,bind,listen);
        
  while(1)
      {
      设置监听读写文件描述符(FD_*);   
      
      调用select;
      
      如果是倾听套接字就绪,说明一个新的连接请求建立
           { 
              建立连接(accept);
              读写数据
              close新建立连接fd
           }      
      } 

并发服务器:多路复用I/O,非阻塞

  初始话(socket,bind,listen);
        
  while(1)
      {
      设置监听读写文件描述符(FD_*);   
      
      调用select;
      
      如果是倾听套接字就绪,说明一个新的连接请求建立
           { 
              建立连接(accept);
              加入到监听文件描述符中去;//起到非阻塞
           }
        否则说明是一个已经连接过的描述符
          {
              进行操作(read或者write);
           }
                      
      } 

*/

static int32 netTcpSockAccept(int32 listenfd, netFunctionRun function)
{
  struct sockaddr_in clientaddr;
  int32 acceptfd;
  int32 clientLen;
  int32 ret = -1;

  clientLen = sizeof(clientaddr);
  acceptfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientLen);
  if (acceptfd <= 0) 
  {
    LOG_ERROR("tcp failed to acccept sock\n");
    return TASK_ERROR; 
  }

  ret = function(acceptfd, NULL);

  close(acceptfd);

  return NET_TCP_SOCK_SET;
}

/*
netTcpSockLoopServer 是阻塞的，非多路的
*/
int32 netTcpSockSelectServer(int32 port, int32 timeDelay, uint8 *ipAddr, netFunctionRun function)
{
  int32 ret;
  fd_set fdsr;
  int32 maxSock;
  struct timeval tv;

  maxSock = netTcpSockInitServer(port, NET_DEFAULT_MODE, ipAddr);
  if (maxSock == TASK_ERROR)
  {
    LOG_ERROR("netTcpSockInitServer error\n");
    return TASK_ERROR; 
  }
    
  while(1)
  {
    FD_ZERO(&fdsr);
    tv.tv_sec = timeDelay;
    tv.tv_usec = 0;
  
    FD_SET(maxSock, &fdsr);

    /*进程阻塞*/
    ret = select(maxSock + 1, &fdsr, NULL, NULL, &tv);
    if (ret < 0) 
    {
      LOG_ERROR("tcp failed to select sock\n");
      close(maxSock);
      return TASK_ERROR; 
    } 
    else if (ret == 0) 
    {
      LOG_DBG("tcp sock server timeout\n");
      continue;
    } 

    if (FD_ISSET(maxSock, &fdsr) )
    {
      ret = netTcpSockAccept(maxSock, function);
      if (ret == NET_TCP_SOCK_SET)
      {
        LOG_DBG("tcp sock server continue\n");
        continue;
      }
      else
      {
        LOG_DBG("tcp sock server break\n");
        break;
      }
    }
  }

  close(maxSock);

  return TASK_OK;
}

/*这里相当于接收一个连接，然后将新的连接加入新的文件描述符集，
然后将旧的连接从旧的文件描述符集去掉*/
static int32 netTcpSockMultiSelectIOAccept(fd_set fdsets, netServerContext_t *serverFd)
{
  struct sockaddr_in clientaddr;
  int32 acceptfd;
  int32 clientLen;
  int32 ret = -1;
  int32 oldfd= -1;

  clientLen = sizeof(clientaddr);
  acceptfd = accept(serverFd->maxfd, (struct sockaddr *)&clientaddr, &clientLen);
  if (acceptfd <= 0) 
  {
    LOG_ERROR("tcp failed to acccept sock\n");
    return TASK_ERROR; 
  }
  else
  {
    LOG_DBG("获取到从客户端%s : 端口%d的连接, 套接字描述符%d...\n",
                        inet_ntoa(clientaddr.sin_addr),
                        ntohs(clientaddr.sin_port),
                        acceptfd);
  }

  //  新加入的描述符，还没判断是否可以或者写
  //  将缓存的allset的对应acceptfd置位，下次循环时即可监听acceptfd
  FD_SET(acceptfd, &serverFd->allfds);

  if (acceptfd > serverFd->maxfd)
  {
    oldfd = serverFd->maxfd;
    serverFd->maxfd = acceptfd;
  }

  //  至此监听套接字的信息已经被处理, 应该先清楚对应位
  FD_CLR(oldfd, &fdsets);

  if(--serverFd->cliCnt <= 0)  //  nready用来辅助计数，这样就不要遍历整个client数组
  {
    return NET_TCP_SOCK_SET;
  }

  return NET_TCP_SOCK_CTL;
}

/*这里处理accept fd，查询是否有fd改变，有则会运行程序，然后从新描述符集中去掉fd*/
static int32 netTcpSockSelectMultiIOProc(fd_set fdsets, netServerContext_t *serverFd, netFunctionRun function)
{
  int32 fd;
  
  //  遍历套接字看那些客户端连接套接字有数据请求
  for(fd = 0;fd <= serverFd->maxfd && serverFd->cliCnt> 0;fd++)
  {
    if(FD_ISSET(fd, &fdsets))
    {
      //单进程的环境下，不可以阻塞在这里，可以选择非阻塞，线程，超时.也就无法防范拒绝服务的攻击
      //比较适合短连接的情况
      function(fd, NULL);

      FD_CLR(fd, &serverFd->allfds);                  //清除，表示已被处理
      
      close(fd);

      fd = -1;
    }
  }
  return TASK_OK;
}

/*本质是先处理侦听的fd然后处理accept的fd*/
//  首先检测服务器监听套接字有没有数据，
//  如果有的话说明监听到了客户端的，
//  应该调用accept来获取客户端的连接
//
//  接着检测客户端的连接套接字有没有数据连接
//  如果有的话，说明客户端跟服务器有通信请求

/*
accept惊群和select冲突
在socket中的影响，在linux 2.6版本之前的版本存在，在之后的版本中解决掉了。
1, 多线程阻塞 accept, 当有一个连接到达时，所有线程均被唤醒，其中只有一个线程会获得连接，其余线程继续回复睡眠。
  表面上看，就是一个线程会被唤醒，用户不可感知。--惊群
  tcp/ip详解卷2， 366页
  accept()
  {
  while(qlen==0)
  tsleep
  ...
  }

如果多线程accept, 则都在tsleep处挂起。当有一个连接时，所有线程的tsleep返回0. 最先运行的线程获得连接，其余线程由于qlen==0,继续tsleep

2，多线程阻塞select, 当有一个事件发生时，会唤醒所有线程，--select冲突
  表面上看，就是所有线程都返回了，用户可感知------和accept不同，accept最终只有一个线程返回。
  如果只有一个事件发生，那所有线程都返回了，一方面效率低下，一方面可能会对同一事件重复操作。
  iocp, epoll貌似没这方面问题。
  据说新的操作系统，accept的问题已解决。
*/

/*
第一，      若将NULL以形参传入，即不传入时间结构，就是将select置于阻塞状态，一定等到监视文件描述符集合中某个文件描述符发生变化为止；

第二，      第二，若将时间值设为0秒0毫秒，就变成一个纯粹的非阻塞函数，不管文件描述符是否有变化，都立刻返回继续执行，文件无变化返回0，有变化返回一个正值；

第三，      timeout的值大于0，这就是等待的超时时间，即select在timeout时间内阻塞，超时时间之内有事件到来就返回了，否则在超时后不管怎样一定返回，返回值同上述。
*/

/*
注意事项
       1)  select()函数会受到O_NDELAY标记和O_NONBLOCK标记的影 响，如果socket是阻塞的socket，则调用select()跟不调用select()时的效果是一样的，socket仍然是阻塞式TCP通讯，相 反，如果socket是非阻塞的socket，那么调用select()时就可以实现非阻塞式TCP通讯；

       2)  fd_set是一个位数组，其大小限制为__FD_SETSIZE（1024），位数组的每一位代表其对应的描述符是否需要被检查

       3)  select()函数基本上可以在所有支持文件描述符操作的系统平台上运 行(如：Linux 、Unix 、Windows、MacOS等)，可移植性好
*/

int32 netTcpSockSelectMultiIOServer(int32 port, int32 timeDelay, uint8 *ipAddr, netFunctionRun function)
{
  int32 ret;
  fd_set fdsr;
  int maxSock, sockfd;
  struct timeval tv;
  int32 i;
  
  netServerContext_t serverFd;

  serverFd.maxfd = netTcpSockInitServer(port, NET_MULTI_MODE, ipAddr);
  if (serverFd.maxfd == TASK_ERROR)
  {
    LOG_ERROR("netTcpSockInitServer error\n");
    return TASK_ERROR; 
  }
  
  memset(&serverFd, 0,sizeof(netServerContext_t));
  
  sockfd = serverFd.maxfd;
  maxSock = serverFd.maxfd;

  //  allret用于保存清楚完标志的fd_ret信息, 在每次处理完后，赋值给ret
  // 这里是第一个处理sockfd，其他新加入的在其后的accept中处理
  FD_ZERO(&serverFd.allfds);
  FD_SET(sockfd, &serverFd.allfds);
  
  while(1)
  {
    fdsr = serverFd.allfds;//和allset的搭配使得新加入的fd要等到下次select才会被监听
    
    /*每次调用select前都要重新设置文件描述符和时间，因为事件发生后，文件描述符和时间都被内核修改啦*/
    
    tv.tv_sec = timeDelay;
    tv.tv_usec = 0;
  
    ret = select(maxSock + 1, &fdsr, NULL, NULL, &tv);
    if (ret < 0) 
    {
      LOG_ERROR("tcp failed to select sock\n");
      close(sockfd);
      return TASK_ERROR; 
    } 
    else if (ret == 0) 
    {
      LOG_DBG("tcp sock server timeout\n");
      continue;
    } 
    
    serverFd.cliCnt = ret;

    if (FD_ISSET(sockfd, &fdsr) )
    {
      ret = netTcpSockMultiSelectIOAccept(fdsr, &serverFd);
      if (ret == NET_TCP_SOCK_SET)
      {
        LOG_DBG("tcp sock server continue\n");
        continue;
      }
    }
    /*新的接收fd，更新fdsets，然后找到那个fd有数据需要IO*/
    netTcpSockSelectMultiIOProc(fdsr, &serverFd, function);
  }

  close(sockfd);

  return TASK_OK;
}

/*
poll
1.      头文件
# include < sys/ poll. h>

2.      参数说明
int poll ( struct pollfd * fds, unsigned int nfds, int timeout);

和select()不一样，poll()没有使用低效的三个基于位的文件描述符set，而是采用了一个单独的结构体pollfd数组，由fds指针指向这个组。pollfd结构体定义如下：

 

struct pollfd

{

int fd;               文件描述符 

short events;         等待的事件 

short revents;        实际发生了的事件

} ;

typedef unsigned long   nfds_t;

struct pollfd * fds：是一个struct pollfd结构类型的数组，用于存放需要检测其状态的socket描述符；每当调用这个函数之后，系统不需要清空这个数组，操作起来比较方便；特别是对于 socket连接比较多的情况下，在一定程度上可以提高处理的效率；这一点与select()函数不同，调用select()函数之后，select() 函数需要清空它所检测的socket描述符集合，导致每次调用select()之前都必须把socket描述符重新加入到待检测的集合中；因此，select()函数适合于只检测少量socket描述符的情况，而poll()函数适合于大量socket描述符的情况；

    如果待检测的socket描述符为负值，则对这个描述符的检测就会被忽略，也就是不会对成员变量events进行检测，在events上注册的事件也会被忽略，poll()函数返回的时候，会把成员变量revents设置为0，表示没有事件发生；

 

经常检测的事件标记：

POLLIN/POLLRDNORM(可读)、

POLLOUT/POLLWRNORM(可写)、

POLLERR(出错)

 

合法的事件如下：

POLLIN              有数据可读。

POLLRDNORM       有普通数据可读。

POLLRDBAND        有优先数据可读。

POLLPRI              有紧迫数据可读。

POLLOUT             写数据不会导致阻塞。

POLLWRNORM       写普通数据不会导致阻塞。

POLLWRBAND        写优先数据不会导致阻塞。

POLLMSG SIGPOLL    消息可用。

 

此外，revents域中还可能返回下列事件：

POLLER               指定的文件描述符发生错误。

POLLHUP             指定的文件描述符挂起事件。

POLLNVAL            指定的文件描述符非法。

这些事件在events域中无意义，因为它们在合适的时候总是会从revents中返回。使用poll()和select()不一样，你不需要显式地请求异常情况报告。

 

POLLIN | POLLPRI等价于select()的读事件，

POLLOUT |POLLWRBAND等价于select()的写事件。

POLLIN等价于POLLRDNORM |POLLRDBAND，

而POLLOUT则等价于POLLWRNORM。

 

如果是对一个描述符上的多个事件感兴趣的话，可以把这些常量标记之间进行按位或运算就可以了；

比如：对socket描述符fd上的读、写、异常事件感兴趣，就可以这样做：

struct pollfd  fds;

fds[nIndex].events=POLLIN | POLLOUT | POLLERR；

 

当 poll()函数返回时，要判断所检测的socket描述符上发生的事件，可以这样做：

struct pollfd  fds;

检测可读TCP连接请求：

if((fds[nIndex].revents & POLLIN) == POLLIN){//接收数据/调用accept()接收连接请求}

 

检测可写：

if((fds[nIndex].revents & POLLOUT) == POLLOUT){//发送数据}

 

检测异常：

if((fds[nIndex].revents & POLLERR) == POLLERR){//异常处理}

 

nfds_t nfds：用于标记数组fds中的结构体元素的总数量；

 

timeout：是poll函数调用阻塞的时间，单位：毫秒；

如果timeout==0，那么 poll() 函数立即返回而不阻塞，

如果timeout==INFTIM，那么poll() 函数会一直阻塞下去，直到所检测的socket描述符上的感兴趣的事件发 生是才返回，如果感兴趣的事件永远不发生，那么poll()就会永远阻塞下去；

3.      返回值:
>0：数组fds中准备好读、写或出错状态的那些socket描述符的总数量；

==0：数组fds中没有任何socket描述符准备好读、写，或出错；此时poll超时，超时时间是timeout毫秒；换句话说，如果所检测的 socket描述符上没有任何事件发生的话，那么poll()函数会阻塞timeout所指定的毫秒时间长度之后返回，

-1： poll函数调用失败，同时会自动设置全局变量errno；errno为下列值之一：

4.      错误代码
EBADF            一个或多个结构体中指定的文件描述符无效。

EFAULTfds        指针指向的地址超出进程的地址空间。

EINTR            请求的事件之前产生一个信号，调用可以重新发起。

EINVALnfds       参数超出PLIMIT_NOFILE值。

ENOMEM         可用内存不足，无法完成请求。

 

5.      实现机制
poll是一个系统调用，其内核入口函数为sys_poll，sys_poll几乎不做任何处理直接调用do_sys_poll，do_sys_poll的执行过程可以分为三个部分：

 

    1)，将用户传入的pollfd数组拷贝到内核空间，因此拷贝操作和数组长度相关，时间上这是一个O（n）操作，这一步的代码在do_sys_poll中包括从函数开始到调用do_poll前的部分。

 

    2)，查询每个文件描述符对应设备的状态，如果该设备尚未就绪，则在该设备的等待队列中加入一项并继续查询下一设备的状态。查询完所有设备后如果没有一个设备就绪，这时则需要挂起当前进程等待，直到设备就绪或者超时，挂起操作是通过调用schedule_timeout执行的。设备就绪后进程被通知继续运行，这时再次遍历所有设备，以查找就绪设备。这一步因为两次遍历所有设备，时间复杂度也是O（n），这里面不包括等待时间。相关代码在do_poll函数中。

 

    3)，将获得的数据传送到用户空间并执行释放内存和剥离等待队列等善后工作，向用户空间拷贝数据与剥离等待队列等操作的的时间复杂度同样是O（n），具体代码包括do_sys_poll函数中调用do_poll后到结束的部分。

6.      注意事项
       1). poll() 函数不会受到socket描述符上的O_NDELAY标记和O_NONBLOCK标记的影响和制约，也就是说，不管socket是阻塞的还是非阻塞 的，poll()函数都不会收到影响；

       2). poll()函数则只有个别的的操作系统提供支持(如：SunOS、Solaris、AIX、HP提供 支持，但是Linux不提供支持)，可移植性差；

       3). 不依赖于 文件系统， 所以他所关心的文件描述符的数量是没有限制的

*/

static int32 netTcpSockMultiPollIOAccept(struct pollfd *clients, int32 *maxFd)
{
  struct sockaddr_in clientaddr;
  int32 acceptfd;
  int32 clientLen;
  int32 ret = -1;
  int32 oldfd= -1;
  int32 i;

  clientLen = sizeof(clientaddr);
  acceptfd = accept(*maxFd, (struct sockaddr *)&clientaddr, &clientLen);
  if (acceptfd <= 0) 
  {
    LOG_ERROR("tcp failed to acccept sock\n");
    return TASK_ERROR; 
  }
  else
  {
    LOG_DBG("获取到从客户端%s : 端口%d的连接, 套接字描述符%d...\n",
                        inet_ntoa(clientaddr.sin_addr),
                        ntohs(clientaddr.sin_port),
                        acceptfd);
  }

  for (i = 0; i < POLL_MAX; i++) 
  {
    if (clients[i].fd == -1) 
    {
      clients[i].fd = acceptfd;
      clients[i].events = POLLIN | POLLOUT;
      break;
    }
  }

  if (i == POLL_MAX) 
  {
    LOG_DBG("too many connection, more than %d\n", POLL_MAX);
    close(acceptfd);
    return NET_TCP_SOCK_SET;
  }

  if (i > *maxFd)
  {
    *maxFd = i;
  }
  
  return NET_TCP_SOCK_CTL;
}

static int32 netTcpSockPollMultiIOProc(struct pollfd* clients, int32 maxClient, int32 nready, netFunctionRun function)
{
  int32 connfd;
  int32 i;
  int32 ret = -1;
  
  if (nready == 0)
  {
    LOG_ERROR("no ready error\n");
    return TASK_ERROR;
  }

  for (i = 1; i< maxClient; i++)
  {
    connfd = clients[i].fd;

    if (connfd == -1)
    {
      continue;
    }

    if (clients[i].revents & (POLLIN | POLLOUT))
    {
      ret = function(connfd, NULL);
      if (ret == TASK_ERROR)
      {
        close(connfd);
        continue;
      }
    }

    if (--nready <= 0)//没有连接需要处理，退出循环
    {
       break;
    }
  }
  
  return TASK_OK;
}

int32 netTcpSockPollMultiIOServer(int32 port, int32 timeDelay, uint8 *ipAddr, netFunctionRun function)
{
  fd_set fdsr;
  int32 maxSock;
  struct timeval tv;
  int32 i;
  struct pollfd clients[POLL_MAX];
  int32 nready;
  
  int32 serverFd = -1;

  serverFd = netTcpSockInitServer(port, NET_MULTI_MODE, ipAddr);
  if (serverFd == TASK_ERROR)
  {
    LOG_ERROR("netTcpSockInitServer error\n");
    return TASK_ERROR; 
  }
  
  clients[0].fd = serverFd;
  clients[0].events = POLLIN | POLLOUT;

  for (i = 1; i< POLL_MAX; i++) 
  {
    clients[i].fd = -1; 
  }
    
  maxSock = serverFd + 1;

  while(1)
  {
    nready = poll(clients, maxSock + 1, timeDelay);
    if (nready < 0)
    {
      LOG_ERROR("poll error\n");
      break; 
    }
    else if (nready == 0)
    {
      LOG_ERROR("time out\n");
      break;
    }
    else
    {
      if (clients[i].revents & (POLLIN | POLLOUT))
      {
        if (netTcpSockMultiPollIOAccept(clients, &maxSock) == NET_TCP_SOCK_SET)
        {
          LOG_DBG("continue loop\n");
          continue;
        }

      }
      
      --nready;
    } 

    netTcpSockPollMultiIOProc(clients, maxSock, nready, function);
  }

  close(serverFd);
}


/*
epoll
epoll是一种高效的管理socket的模型，相对于select和poll来说具有更高的效率和易用性。传统的select以及poll的效率会随socket数量的线形递增而呈二次乃至三次方的下降，而epoll的性能不会随socket数量增加而下降。标准的linux-2.4.20内核不支持epoll，需要打patch。本文主要从linux-2.4.32和linux-2.6.10两个内核版本介绍epoll。
1.      头文件

#include <sys/epoll.h>
2.      参数说明

int epoll_create(int size);

int epoll_ctl(int epfd, int op, int fd, struct epoll_event event);

int epoll_wait(int epfd,struct epoll_event events,int maxevents,int timeout);

 

typedef union epoll_data

{

    void ptr;

    int fd;

    __uint32_t u32;

    __uint64_t u64;

} epoll_data_t;

epoll_data是一个联合体,借助于它应用程序可以保存很多类型的信息:fd、指针等等。有了它，应用程序就可以直接定位目标了。

struct epoll_event

{

    __uint32_t events;    / epoll events /

epoll_data_t data;    / User data variable /

};

 

epoll_event 结构体被用于注册所感兴趣的事件和回传所发生待处理的事件，其中

epoll_data_t 联合体用来保存触发事件的某个文件描述符相关的数据，例如一个client连接到服务器，服务器通过调用accept函数可以得到于这个client对应的socket文件描述符，可以把这文件描述符赋给epoll_data的fd字段以便后面的读写操作在这个文件描述符上进行。

events字段是表示感兴趣的事件和被触发的事件。可能的取值为：

EPOLLIN ： 表示对应的文件描述符可以读；

EPOLLOUT：表示对应的文件描述符可以写；

EPOLLPRI： 表示对应的文件描述符有紧急的数据可读

EPOLLERR： 表示对应的文件描述符发生错误；

EPOLLHUP：表示对应的文件描述符被挂断；

EPOLLET：  表示对应的文件描述符设定为edge模式；

 
3.      所用到的函数：

epoll不再是一个单独的系统调用，而是由epoll_create/epoll_ctl/epoll_wait三个系统调用组成
3.1.         epoll_create函数

函数声明：int epoll_create(int size)

   

该函数生成一个epoll专用的文件描述符，其中的参数是指定生成描述符的最大范围。在linux-2.4.32内核中根据size大小初始化哈希表的大小，在linux2.6.10内核中该参数无用，使用红黑树管理所有的文件描述符，而不是hash。其实是申请一个内核空间，用来存放你想关注的socket fd上是否发生以及发生了什么事件。

在用完之后，记得用close()来关闭这个创建出来的epoll句柄。
3.2.         epoll_ctl函数

函数声明：int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)

该函数用于控制某个文件描述符上的事件，可以注册事件，修改事件，删除事件。

相对于select模型中的FD_SET和FD_CLR宏。

 

参数：

epfd：由 epoll_create 生成的epoll专用的文件描述符；

 op：要进行的操作例如注册事件，可能的取值

        EPOLL_CTL_ADD  注册、

        EPOLL_CTL_MOD 修改、

        EPOLL_CTL_DEL   删除

fd：关联的文件描述符；

event：指向epoll_event的指针；

返回值：如果调用成功返回0,不成功返回-1
3.3.         epoll_wait函数

函数声明：int epoll_wait(int epfd,struct epoll_event events,int maxevents,int timeout)

 

该函数用于轮询I/O事件的发生，来查询所有的网络接口，看哪一个可以读，哪一个可以写了。相对于select模型中的select函数。

一般如果网络主循环是单独的线程的话，可以用-1来等，这样可以保证一些效率，如果是和主逻辑在同一个线程的话，则可以用0来保证主循环的效率。epoll_wait范围之后应该是一个循环，遍利所有的事件

 

参数：

epfd：由epoll_create 生成的epoll专用的文件描述符；

epoll_event：用于回传代处理事件的数组；

maxevents：每次能处理的事件数；

timeout：等待I/O事件发生的超时值（ms）；

          -1永不超时，直到有事件产生才触发，

          0立即返回。

 

返回值：返回发生事件数。-1有错误。

 
4.      epoll的ET模式与LT模式

ET（Edge Triggered）与LT（Level Triggered）的主要区别可以从下面的例子看出

eg：

1)． 标示管道读者的文件句柄注册到epoll中；

2)． 管道写者向管道中写入2KB的数据；

3)． 调用epoll_wait可以获得管道读者为已就绪的文件句柄；

4)． 管道读者读取1KB的数据

5)． 一次epoll_wait调用完成

如果是ET模式，管道中剩余的1KB被挂起，再次调用epoll_wait，得不到管道读者的文件句柄，除非有新的数据写入管道。如果是LT模式，只要管道中有数据可读，每次调用epoll_wait都会触发。

 

另一点区别就是设为ET模式的文件句柄必须是非阻塞的。

 
5.      epoll的实现

epoll的源文件在/usr/src/linux/fs/eventpoll.c，在module_init时注册一个文件系统 eventpoll_fs_type，对该文件系统提供两种操作poll和release，所以epoll_create返回的文件句柄可以被poll、 select或者被其它epoll epoll_wait。对epoll的操作主要通过三个系统调用实现：

1)． sys_epoll_create

2)． sys_epoll_ctl

3)． sys_epoll_wait

下面结合源码讲述这三个系统调用。

1)． long sys_epoll_create (int size)

sys_epoll_create(epoll_create对应的内核函数），这个函数主要是做一些准备工作，比如创建数据结构，初始化数据并最终返回一个文件描述符（表示新创建的虚拟epoll文件），这个操作可以认为是一个固定时间的操作。该系统调用主要分配文件句柄、inode以及file结构。

在linux-2.4.32内核中，使用hash保存所有注册到该epoll的文件句柄，在该系统调用中根据size大小分配hash的大小。具体为不小于size，但小于2size的2的某次方。最小为2的9次方（512），最大为2的17次方 （128 x 1024）。

在linux-2.6.10内核中，使用红黑树保存所有注册到该epoll的文件句柄，size参数未使用，只要大于零就行。

   

epoll是做为一个虚拟文件系统来实现的，这样做至少有以下两个好处：

    (1)，可以在内核里维护一些信息，这些信息在多次epoll_wait间是保持的，比如所有受监控的文件描述符。

    (2)，epoll本身也可以被poll/epoll;

 

具体epoll的虚拟文件系统的实现和性能分析无关，不再赘述。

 

2)．  long sys_epoll_ctl(int epfd, int op, int fd, struct epoll_event event)

 

sys_epoll_ctl(epoll_ctl对应的内核函数），需要明确的是每次调用sys_epoll_ctl只处理一个文件描述符，这里主要描述当op为EPOLL_CTL_ADD时的执行过程，sys_epoll_ctl做一些安全性检查后进入ep_insert，ep_insert里将 ep_poll_callback做为回掉函数加入设备的等待队列（假定这时设备尚未就绪），由于每次poll_ctl只操作一个文件描述符，因此也可以认为这是一个O(1)操作。

    ep_poll_callback函数很关键，它在所等待的设备就绪后被系统回掉，执行两个操作：

    (1)，将就绪设备加入就绪队列，这一步避免了像poll那样在设备就绪后再次轮询所有设备找就绪者，降低了时间复杂度，由O（n）到O（1）;   

    (2)，唤醒虚拟的epoll文件;

 

(1)． 注册句柄 op = EPOLL_CTL_ADD

注册过程主要包括：

A．将fd插入到hash（或rbtree）中，如果原来已经存在返回-EEXIST，

B．给fd注册一个回调函数，该函数会在fd有事件时调用，在该函数中将fd加入到epoll的就绪队列中。

C．检查fd当前是否已经有期望的事件产生。如果有，将其加入到epoll的就绪队列中，唤醒epoll_wait。

 

(2)． 修改事件 op = EPOLL_CTL_MOD

修改事件只是将新的事件替换旧的事件，然后检查fd是否有期望的事件。如果有，将其加入到epoll的就绪队列中，唤醒epoll_wait。

 

(3)． 删除句柄 op = EPOLL_CTL_DEL

将fd从hash（rbtree）中清除。

 

3)． long sys_epoll_wait(int epfd, struct epoll_event events, int maxevents,int timeout)

 

如果epoll的就绪队列为空，并且timeout非0，挂起当前进程，引起CPU调度。

如果epoll的就绪队列不空，遍历就绪队列。对队列中的每一个节点，获取该文件已触发的事件，判断其中是否有我们期待的事件，如果有，将其对应的epoll_event结构copy到用户events。

 

   sys_epoll_wait，这里实际执行操作的是ep_poll函数。该函数等待将进程自身插入虚拟epoll文件的等待队列，直到被唤醒（见上面ep_poll_callback函数描述），最后执行ep_events_transfer将结果拷贝到用户空间。由于只拷贝就绪设备信息，所以这里的拷贝是一个O(1）操作。

 

需要注意的是，在LT模式下，把符合条件的事件copy到用户空间后，还会把对应的文件重新挂接到就绪队列。所以在LT模式下，如果一次epoll_wait某个socket没有read/write完所有数据，下次epoll_wait还会返回该socket句柄。


6.      使用epoll的注意事项

1. ET模式比LT模式高效，但比较难控制。

2. 如果某个句柄期待的事件不变，不需要EPOLL_CTL_MOD，但每次读写后将该句柄modify一次有助于提高稳定性，特别在ET模式。

3. socket关闭后最好将该句柄从epoll中delete（EPOLL_CTL_DEL），虽然epoll自身有处理，但会使epoll的hash的节点数增多，影响搜索hash的速度。


*/

/*
EPOLL事件分发系统可以运转在两种模式下：
   Edge Triggered (ET)
   Level Triggered (LT)
接下来说明ET, LT这两种事件分发机制的不同。我们假定一个环境：
1. 我们已经把一个用来从管道中读取数据的文件句柄(RFD)添加到epoll描述符
2. 这个时候从管道的另一端被写入了2KB的数据
3. 调用epoll_wait(2)，并且它会返回RFD，说明它已经准备好读取操作
4. 然后我们读取了1KB的数据
5. 调用epoll_wait(2)......

Edge Triggered 工作模式：
如果我们在第1步将RFD添加到epoll描述符的时候使用了EPOLLET标志，那么在第5步调用epoll_wait(2)之后将有可能会挂起，因为 剩余的数据还存在于文件的输入缓冲区内，而且数据发出端还在等待一个针对已经发出数据的反馈信息。只有在监视的文件句柄上发生了某个事件的时候 ET 工作模式才会汇报事件。因此在第5步的时候，调用者可能会放弃等待仍在存在于文件输入缓冲区内的剩余数据。在上面的例子中，会有一个事件产生在RFD句柄 上，因为在第2步执行了一个写操作，然后，事件将会在第3步被销毁。因为第4步的读取操作没有读空文件输入缓冲区内的数据，因此我们在第5步调用 epoll_wait(2)完成后，是否挂起是不确定的。epoll工作在ET模式的时候，必须使用非阻塞套接口，以避免由于一个文件句柄的阻塞读/阻塞 写操作把处理多个文件描述符的任务饿死。最好以下面的方式调用ET模式的epoll接口，在后面会介绍避免可能的缺陷。
   i    基于非阻塞文件句柄
   ii   只有当read(2)或者write(2)返回EAGAIN时才需要挂起，等待

Level Triggered 工作模式
相反的，以LT方式调用epoll接口的时候，它就相当于一个速度比较快的poll(2)，并且无论后面的数据是否被使用，因此他们具有同样的职能。因为 即使使用ET模式的epoll，在收到多个chunk的数据的时候仍然会产生多个事件。调用者可以设定EPOLLONESHOT标志，在 epoll_wait(2)收到事件后epoll会与事件关联的文件句柄从epoll描述符中禁止掉。因此当EPOLLONESHOT设定后，使用带有 EPOLL_CTL_MOD标志的epoll_ctl(2)处理文件句柄就成为调用者必须作的事情。

以上翻译自man epoll.

然后详细解释ET, LT:

LT(level triggered)是缺省的工作方式，并且同时支持block和no-block socket.在这种做法中，内核告诉你一个文件描述符是否就绪了，然后你可以对这个就绪的fd进行IO操作。如果你不作任何操作，内核还是会继续通知你 的，所以，这种模式编程出错误可能性要小一点。传统的select/poll都是这种模型的代表．

ET(edge-triggered)是高速工作方式，只支持no-block socket。在这种模式下，当描述符从未就绪变为就绪时，内核通过epoll告诉你。然后它会假设你知道文件描述符已经就绪，并且不会再为那个文件描述 符发送更多的就绪通知，直到你做了某些操作导致那个文件描述符不再为就绪状态了(比如，你在发送，接收或者接收请求，或者发送接收的数据少于一定量时导致 了一个EWOULDBLOCK 错误）。但是请注意，如果一直不对这个fd作IO操作(从而导致它再次变成未就绪)，内核不会发送更多的通知(only once),不过在TCP协议中，ET模式的加速效用仍需要更多的benchmark确认。

在许多测试中我们会看到如果没有大量的idle-connection或者dead-connection，epoll的效率并不会比 select/poll高很多，但是当我们遇到大量的idle-connection(例如WAN环境中存在大量的慢速连接)，就会发现epoll的效率 大大高于select/poll。


epoll是linux系統最新的處理多連接的高效率模型， 工作在兩種方式下， EPOLLLT方式和EPOLLET方式。

EPOLLLT是系統默認， 工作在這種方式下， 程式設計師不易出問題， 在接收數據時，只要socket輸入緩存有數據，都能夠獲得EPOLLIN的持續通知， 同樣在發送數據時， 只要發送緩存夠用， 都會有持續不間斷的EPOLLOUT通知。

而對於EPOLLET是另外一種觸發方式， 比EPOLLLT要高效很多， 對程式設計師的要求也多些， 程式設計師必須小心使用，因為工作在此種方式下時， 在接收數據時， 如果有數據只會通知一次， 假如read時未讀完數據，那麼不會再有EPOLLIN的通知了， 直到下次有新的數據到達時為止； 當發送數據時， 如果發送緩存未滿也只有一次EPOLLOUT的通知， 除非你把發送緩存塞滿了， 才會有第二次EPOLLOUT通知的機會， 所以在此方式下read和write時都要處理好。 暫時寫到這裡， 留作備忘。

附加： 如果將一個socket描述符添加到兩個epoll中， 那麼即使在EPOLLET模式下， 只要前一個epoll_wait時，未讀完， 那麼後一個epoll_wait事件時， 也會得到讀的通知， 但前一個讀完的情況下， 後一個epoll就不會得到讀事件的通知了。。。。
*/

/*
在Linux上开发网络服务器的一些相关细节:poll与epoll
　　随着2.6内核对epoll的完全支持，网络上很多的文章和示例代码都提供了这样一个信息：使用epoll代替传统的 poll能给网络服务应用带来性能上的提升。但大多文章里关于性能提升的原因解释的较少，这里我将试分析一下内核（2.6.21.1）代码中poll与 epoll的工作原理，然后再通过一些测试数据来对比具体效果。 POLL：

先说poll，poll或select为大部分Unix/Linux程序员所熟悉，这俩个东西原理类似，性能上也不存在明显差异，但select对所监控的文件描述符数量有限制，所以这里选用poll做说明。
poll是一个系统调用，其内核入口函数为sys_poll，sys_poll几乎不做任何处理直接调用do_sys_poll，do_sys_poll的执行过程可以分为三个部分：
1，将用户传入的pollfd数组拷贝到内核空间，因为拷贝操作和数组长度相关，时间上这是一个O（n）操作，这一步的代码在do_sys_poll中包括从函数开始到调用do_poll前的部分。
2，查询每个文件描述符对应设备的状态，如果该设备尚未就绪，则在该设备的等待队列中加入一项并继续查询下一设备的状态。查询完所有设备后如果没有一个设备就绪，这时则需要挂起当前进程等待，直到设备就绪或者超时，挂起操作是通过调用schedule_timeout执行的。设备就绪后进程被通知继续运行，这时再次遍历所有设备，以查找就绪设备。这一步因为两次遍历所有设备，时间复杂度也是O（n），这里面不包括等待时间。相关代码在do_poll函数中。
3，将获得的数据传送到用户空间并执行释放内存和剥离等待队列等善后工作，向用户空间拷贝数据与剥离等待队列等操作的的时间复杂度同样是O（n），具体代码包括do_sys_poll函数中调用do_poll后到结束的部分。
EPOLL：
接下来分析epoll，与poll/select不同，epoll不再是一个单独的系统调用，而是由epoll_create/epoll_ctl/epoll_wait三个系统调用组成，后面将会看到这样做的好处。
先来看sys_epoll_create(epoll_create对应的内核函数），这个函数主要是做一些准备工作，比如创建数据结构，初始化数据并最终返回一个文件描述符（表示新创建的虚拟epoll文件），这个操作可以认为是一个固定时间的操作。
epoll是做为一个虚拟文件系统来实现的，这样做至少有以下两个好处：
1，可以在内核里维护一些信息，这些信息在多次epoll_wait间是保持的，比如所有受监控的文件描述符。
2， epoll本身也可以被poll/epoll;
具体epoll的虚拟文件系统的实现和性能分析无关，不再赘述。
在sys_epoll_create中还能看到一个细节，就是epoll_create的参数size在现阶段是没有意义的，只要大于零就行。

接着是sys_epoll_ctl(epoll_ctl对应的内核函数），需要明确的是每次调用sys_epoll_ctl只处理一个文件描述符，这里主要描述当op为EPOLL_CTL_ADD时的执行过程，sys_epoll_ctl做一些安全性检查后进入ep_insert，ep_insert里将 ep_poll_callback做为回掉函数加入设备的等待队列（假定这时设备尚未就绪），由于每次poll_ctl只操作一个文件描述符，因此也可以认为这是一个O(1)操作

ep_poll_callback函数很关键，它在所等待的设备就绪后被系统回掉，执行两个操作：

1，将就绪设备加入就绪队列，这一步避免了像poll那样在设备就绪后再次轮询所有设备找就绪者，降低了时间复杂度，由O（n）到O（1）;
2，唤醒虚拟的epoll文件;
最后是sys_epoll_wait，这里实际执行操作的是ep_poll函数。该函数等待将进程自身插入虚拟epoll文件的等待队列，直到被唤醒（见上面ep_poll_callback函数描述），最后执行ep_events_transfer将结果拷贝到用户空间。由于只拷贝就绪设备信息，所以这里的拷贝是一个O(1）操作。
还有一个让人关心的问题就是epoll对EPOLLET的处理，即边沿触发的处理，粗略看代码就是把一部分水平触发模式下内核做的工作交给用户来处理，直觉上不会对性能有太大影响，感兴趣的朋友欢迎讨论。
POLL/EPOLL对比：
表面上poll的过程可以看作是由一次epoll_create/若干次epoll_ctl/一次epoll_wait/一次close等系统调用构成，实际上epoll将poll分成若干部分实现的原因正是因为服务器软件中使用poll的特点（比如Web服务器）：
1，需要同时poll大量文件描述符;
2，每次poll完成后就绪的文件描述符只占所有被poll的描述符的很少一部分。
3，前后多次poll调用对文件描述符数组（ufds）的修改只是很小;
传统的poll函数相当于每次调用都重起炉灶，从用户空间完整读入ufds，完成后再次完全拷贝到用户空间，另外每次poll都需要对所有设备做至少做一次加入和删除等待队列操作，这些都是低效的原因。

epoll将以上情况都细化考虑，不需要每次都完整读入输出ufds，只需使用epoll_ctl调整其中一小部分，不需要每次epoll_wait都执行一次加入删除等待队列操作，另外改进后的机制使的不必在某个设备就绪后搜索整个设备数组进行查找，这些都能提高效率。另外最明显的一点，从用户的使用来说，使用epoll不必每次都轮询所有返回结果已找出其中的就绪部分，O（n）变O（1），对性能也提高不少。

此外这里还发现一点，是不是将epoll_ctl改成一次可以处理多个fd（像semctl那样）会提高些许性能呢？特别是在假设系统调用比较耗时的基础上。不过关于系统调用的耗时问题还会在以后分析。

POLL/EPOLL测试数据对比：
测试的环境：我写了三段代码来分别模拟服务器，活动的客户端，僵死的客户端，服务器运行于一个自编译的标准2.6.11内核系统上，硬件为 PIII933，两个客户端各自运行在另外的PC上，这两台PC比服务器的硬件性能要好，主要是保证能轻易让服务器满载，三台机器间使用一个100M交换机连接。
服务器接受并poll所有连接，如果有request到达则回复一个response，然后继续poll。
活动的客户端（Active Client）模拟若干并发的活动连接，这些连接不间断的发送请求接受回复。
僵死的客户端（zombie）模拟一些只连接但不发送请求的客户端，其目的只是占用服务器的poll描述符资源。
测试过程：保持10个并发活动连接，不断的调整僵并发连接数，记录在不同比例下使用poll与epoll的性能差别。僵死并发连接数根据比例分别是：0，10，20，40，80，160，320，640，1280，2560，5120，10240。
下图中横轴表示僵死并发连接与活动并发连接之比，纵轴表示完成40000次请求回复所花费的时间，以秒为单位。红色线条表示poll数据，绿色表示 epoll数据。可以看出，poll在所监控的文件描述符数量增加时，其耗时呈线性增长，而epoll则维持了一个平稳的状态，几乎不受描述符个数影响。
在监控的所有客户端都是活动时，poll的效率会略高于epoll（主要在原点附近，即僵死并发连接为0时，图上不易看出来），究竟epoll实现比poll复杂，监控少量描述符并非它的长处。

*/
static int32 netTcpSockEpollAccept(int32 listenfd)
{
  struct sockaddr_in clientaddr;
  int32 acceptfd;
  int32 clientLen;
  int32 ret = -1;

  clientLen = sizeof(clientaddr);
  acceptfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientLen);
  if (acceptfd <= 0) 
  {
    LOG_ERROR("tcp failed to acccept sock\n");
    return TASK_ERROR; 
  }

  return acceptfd;
}


int32 netTcpSockEpollServer(int32 port, int32 timeDelay, uint8 *ipAddr, netFunctionRun function)
{
  int32 epfd;
  int32 listenfd;
  int32 acceptFd;
  struct timeval tv; 
  int32 nfds;
  int32 i;
  
  //声明epoll_event结构体的变量,ev用于注册事件,数组用于回传要处理的事件
  struct epoll_event ev,events[EPOLL_MAX];

  epfd=epoll_create(EPOLL_MAX_FD_NUM); //创建epoll句柄

  listenfd = netTcpSockInitServer(port, NET_DEFAULT_MODE, ipAddr);;
  if (listenfd == TASK_ERROR)
  {
    LOG_ERROR("netTcpSockInitServer error\n");
    return TASK_ERROR; 
  }

  //设置与要处理的事件相关的文件描述符
  ev.data.fd=listenfd;
  //设置要处理的事件类型
  ev.events=EPOLLIN | EPOLLOUT;
  //注册epoll事件
  epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);

  while(1)
  {
    //等待epoll事件的发生
    nfds=epoll_wait(epfd,events,EPOLL_MAX,EPOLL_MAX_FD_NUM);

    for (i = 0; i < nfds;i++)
    {
      if(events[i].data.fd==listenfd)
      {
        acceptFd = netTcpSockEpollAccept(listenfd);
        if (acceptFd > 0)
        {
          //设置用于读操作的文件描述符
          ev.data.fd=acceptFd;
          //设置用于注测的读操作事件
          ev.events=EPOLLIN|EPOLLET;
          //注册event
          epoll_ctl(epfd,EPOLL_CTL_ADD,acceptFd,&ev);
        }
      }
      else if(events[i].events&(EPOLLIN | EPOLLOUT))
      {
         function(events[i].data.fd, NULL);
         close(events[i].data.fd);
      }
      else
      {
        LOG_ERROR("epoll event error\n");
      }
    }
  }
}

int32 netTcpSockClient(int32 port, int32 mode, uint8 *ipAddr, netFunctionRun function)
{
  int32 clientSock;
  struct sockaddr_in serverAddr;
  int32 ret = -1;
  int32 len;
  
  clientSock = socket(AF_INET,SOCK_STREAM,0);
  if(clientSock < 0)
  {
    LOG_ERROR("client socket error\n");
    return TASK_ERROR;
  }
  
  LOG_DBG("client socket create successfully.\n");

  memset(&serverAddr,0,sizeof(serverAddr));
  
  serverAddr.sin_family=AF_INET;
  serverAddr.sin_port=htons(port);
  if (ipAddr == NULL)
  {
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  else
  {
    inet_aton(ipAddr, (struct in_addr *)&serverAddr.sin_addr.s_addr);
  }

  if (serverAddr.sin_addr.s_addr == 0)
  {
    LOG_ERROR("error ip address error\n");
    close(clientSock);
    return TASK_ERROR;
  }
  
  len = sizeof(serverAddr);

  ret = connect(clientSock, (struct sockaddr *)&serverAddr, len);
  if (ret < 0)
  {
    LOG_ERROR("client socket connect error\n");
    close(clientSock);
    return TASK_ERROR;
  }

  function(clientSock, NULL);
  close(clientSock);

  return TASK_OK;
}


/******************************************************************/
/************************udp part**********************************/
/******************************************************************/

/*****
客户端和服务器之间的差别在于服务器必须使用bind()函数来绑定侦听的本地UDP端口，而客户端则可以不进行绑定，直接发送到服务器地址的某个端口地址。
与TCP程序设计相比较，UDP缺少了connect()、listen()及accept()函数，这是用于UDP协议无连接的特性，不用维护TCP的连接、断开等状态。

    UDP协议的服务器端程序设计的流程分为套接字建立、套接字与地址结构进行绑定、收发数据、关闭套接字等过程，
分别对应于函数socket()、bind()、sendto()、recvfrom()和close()。

    UDP协议的服务器端程序设计的流程分为套接字建立、收发数据、关闭套接字等过程，分别对应于函数socket()、sendto()、recvfrom()和close()。

*****/
/**
UDP协议程序设计中的几个问题

1 UDP报文丢失数据
  利用UDP协议进行数据收发的时候，在局域网内一般情况下数据的接收方均能接收到发送方的数据，除非连接双方的主机发生故障，否则不会发生接收不到数据的情况。
路由器要对转发的数据进行存储、处理、合法性判定、转发等操作，容易出现错误，所以很可能在路由器转发的过程中出现数据丢失的现象，
当UDP的数据报文丢失的时候，函数recvfrom()会一直阻塞，直到数据到来。

UDP报文丢失的对策
UDP协议中的数据报文丢失是先天性的，因为UDP是无连接的、不能保证发送数据的正确到达。如果数据报文在经过路由器的时候，被路由器丢弃，则主机C和主机S会对超时的数据进行重发。

2 UDP数据发送中的乱序

  UDP的数据包在网络上传输的时候，有可能造成数据的顺序更改，接收方的数据顺序和发送方的数据顺序发生了颠倒。
这主要是由于路由的不同和路由的存储转发的顺序不同造成的。

UDP乱序的对策
对于乱序的解决方法可以采用发送端在数据段中加入数据报序号的方法，这样接收端对接收到数据的头端进行简单地处理就可以重新获得原始顺序的数据

3 UDP缺乏流量控制
   UDP协议没有TCP协议所具有的滑动窗口概念，接收数据的时候直接将数据放到缓冲区中。
如果用户没有及时地从缓冲区中将数据复制出来，后面到来的数据会接着向缓冲区中放入。
当缓冲区满的时候，后面到来的数据会覆盖之前的数据而造成数据的丢失。

对策: 采用缓冲区

**/

/**
UDP协议中的connect()函数
UDP协议的套接字描述符在进行了数据收发之后，才能确定套接字描述符中所表示的发送方或者接收方的地址，否则仅能确定本地的地址。
例如客户端的套接字描述符在发送数据之前，只要确定建立正确就可以了，在发送的时候才确定发送目的方的地址；
服务器bind()函数也仅仅绑定了本地进行接收的地址和端口。

connect()函数在TCP协议中会发生三次握手，建立一个持续的连接，一般不用于UDP。在UDP协议中使用connect()函数的作用仅仅表示确定了另一方的地址，并没有其他的含义。

connect()函数在UDP协议中使用后会产生如下的副作用：

使用connect()函数绑定套接字后，发送操作不能再使用sendto()函数，要使用write()函数直接操作套接字文件描述符，不再指定目的地址和端口号。

使用connect()函数绑定套接字后，接收操作不能再使用recvfrom()函数，要使用read()类的函数，函数不会返回发送方的地址和端口号。

在使用多次connect()函数的时候，会改变原来套接字绑定的目的地址和端口号，用新绑定的地址和端口号代替，原有的绑定状态会失效。可以使用这种特点来断开原来的连接。

下面是一个使用connect()函数的例子，在发送数据之前，将套接字文件描述符与目的地址使用connect()函数进行了绑定，
之后使用write()函数发送数据并使用read()函数接收数据。

01  static void udpclie_echo(int s, struct sockaddr*to)
02  {
03      char buff[BUFF_LEN] = "UDP TEST";          向服务器端发送的数据
04      connect(s, to, sizeof(*to));               连接
05
06      n = write(s, buff, BUFF_LEN);              发送数据
07
08      read(s, buff, n);                          接收数据
09  }

**/

/***
UDP客户端编程框架

1. 建立套接字文件描述符，socket()；

2. 设置服务器地址和端口，struct sockaddr；

3. 向服务器发送数据，sendto()；

4. 接收服务器的数据，recvfrom()；

5. 关闭套接字，close()。
***/
int32 netUdpSockClient(int32 port, uint8 *ipAddr, netFunctionRun function)
{
	struct sockaddr_in serverAddr;
	int32 clientSock;
	int32 len;
  
  if ((clientSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
  {
    LOG_ERROR("udp client socket connect error\n");
    return TASK_ERROR;

  } 
  
  memset(&serverAddr, 0, sizeof(struct sockaddr_in));

  LOG_DBG("client socket create successfully.\n");

  serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

  if (ipAddr == NULL)
  {
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  else
  {
    inet_aton(ipAddr, (struct in_addr *)&serverAddr.sin_addr.s_addr);
  }
  
  if (serverAddr.sin_addr.s_addr == 0)
  {
    LOG_ERROR("error ip address error\n");
    close(clientSock);
    return TASK_ERROR;
  }

  len = sizeof(serverAddr);

  function(clientSock, &serverAddr);

  close(clientSock);
  
  return TASK_OK;
}

/***
UDP服务器编程框架
1. 建立套接字文件描述符，使用函数socket()，生成套接字文件描述符
2. 设置服务器地址和侦听端口，初始化要绑定的网络地址结构
3. 绑定侦听端口，使用bind()函数，将套接字文件描述符和一个地址类型变量进行绑定
4. 接收客户端的数据，使用recvfrom()函数接收客户端的网络数据。
5. 向客户端发送数据，使用sendto()函数向服务器主机发送数据。
6. 关闭套接字，使用close()函数释放资源。
***/

int32 netUdpSockServer(int32 port, uint8 *ipAddr, netFunctionRun function)
{
	struct sockaddr_in serverAddr;
	int32 serverSock;
	int32 len;
  
  if ((serverSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
  {
    LOG_ERROR("udp client socket connect error\n");
    return TASK_ERROR;

  } 
  
  memset(&serverAddr, 0, sizeof(struct sockaddr_in));

  LOG_DBG("client socket create successfully.\n");

  serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

  if (ipAddr == NULL)
  {
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  else
  {
    inet_aton(ipAddr, (struct in_addr *)&serverAddr.sin_addr.s_addr);
  }

  if ((bind(serverSock, (struct sockaddr *) &serverAddr, sizeof(serverAddr))) < 0) 
	{
    LOG_ERROR("udp client socket bind error\n");
    close(serverSock);
    return TASK_ERROR;
	} 
  else
  { 
		LOG_DBG("bind address to socket.\n\r");
  }
  
  function(serverSock, &serverAddr);
  
  close(serverSock);

  return TASK_OK;
}

/*
TCP 只支持单播
IPv6 不支持广播
IPv4 是可选的
广播和多播要求用于UDP或原始IP


*/

/*
如果没有设置BLOADCASE选项的不递送。 如果bind端口不匹配不递送该套接口 
如果绑定的不是INADDR_ANY话,那么BIND的地址和目的地址匹配才能递送:也就是说你必须BIND一个广播地址或者绑定INADDR_ANY
用ifconfig命令可以disable块网卡的BROADCAST标志，让其不能接受以太网广播。
也可以使用ioctl的SIOCSIFFLAGS方法去掉一个接口的标志IFF_BROADCAST，使之不能接受以太网广播。
*/
int32 netUdpBroadcastSend(int32 port, uint8 *ipAddr, netFunctionRun function)
{
	struct sockaddr_in serverAddr;
	int32 sock;
	int32 len;
  int32 opt;
  int32 ret = -1;
  
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
  {
    LOG_ERROR("udp socket connect error\n");
    return TASK_ERROR;
  } 
  
  memset(&serverAddr, 0, sizeof(struct sockaddr_in));

  opt = 1;
  ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));	
  if (ret < 0)
  {
    LOG_ERROR("udp socket opt error\n");
    close(sock);
    return TASK_ERROR;
  }
  
  LOG_DBG("client socket create successfully.\n");

  serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

  if (ipAddr == NULL)
  {
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  else
  {
    inet_aton(ipAddr, (struct in_addr *)&serverAddr.sin_addr.s_addr);
  }
  
  if (serverAddr.sin_addr.s_addr == 0)
  {
    LOG_ERROR("error ip address error\n");
    close(sock);
    return TASK_ERROR;
  }

  len = sizeof(serverAddr);

  function(sock, &serverAddr);

  close(sock);
  
  return TASK_OK;
}

int32 netUdpBroadcastRev(int32 port, netFunctionRun function)
{
  struct sockaddr_in s_addr;
	int32 sock;
  
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
  {
    LOG_ERROR("udp socket create error\n");
    return TASK_ERROR;
  } 

  memset(&s_addr, 0, sizeof(struct sockaddr_in));
  
  s_addr.sin_family = AF_INET;							
	s_addr.sin_port = htons(port);	
	s_addr.sin_addr.s_addr = INADDR_ANY;

  if ((bind(sock, (struct sockaddr *) &s_addr, sizeof(s_addr))) < 0) 
	{
    LOG_ERROR("udp client socket bind error\n");
    close(sock);
    return TASK_ERROR;
	} 
  else
  { 
		LOG_DBG("bind address to socket.\n\r");
  }

  function(sock, &s_addr);
  
  close(sock);

  return TASK_OK;
}


/*
  组播提供了在网络中进行一对多的发送的机制，组播可以是在一个网段内，也可以是跨网段的，不过跨网段需要交换机、路由器等网络设备支持组播。

  Hosts可以在任何时间加入或者离开组播组，对于组播组的成员没有所处位置的限制，也没有数量的限制，
D类互联网地址是用于组播的：224.0.0.0 - 239.255.255.255。

通过无连接Socket编程可以实现组播数据的发送和接收。组播数据只能通过一个网络接口发送，即使设备上有多个网络接口。

组播是一对多的传输机制，不能通过面向连接的Socket实现组播。

关于组播的相关内容，可参考组播和IGMP协议相关的文章。

创建了SOCK_DGRAM类型的socket以后，通过调用setsockopt()函数来控制该socket的组播，
函数原型：getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)，对于IPPROTO_IP level，optval有如下选择：
IP_ADD_MEMBERSHIP，加入指定的组播组。
IP_DROP_MEMBERSHIP，离开指定的组播组。
IP_MULTICAST_IF，指定发送组播数据的网络接口。
IP_MULTICAST_TTL，给出发送组播数据时的TTL，默认是1。
IP_MULTICAST_LOOP，发送组播数据的主机是否作为接收组播数据的组播成员。
下面的两个例子给出了发送和接收组播数据的实现，接收和发送组播数据的步骤是有区别的。


组播server，发送组播数据的例子

服务器端:
实现组播数据包发送的步骤如下：
创建AF_INET, SOCK_DGRAM的socket。
用组播IP地址和端口初始化sockaddr_in类型数据。
IP_MULTICAST_LOOP，设置本机是否作为组播组成员接收数据。
IP_MULTICAST_IF，设置发送组播数据的端口。
发送组播数据。

客户机端:
创建AF_INET, SOCK_DGRAM类型的socket。
设定 SO_REUSEADDR，允许多个应用绑定同一个本地端口接收数据包。
用bind绑定本地端口，IP为INADDR_ANY，从而能接收组播数据包。
采用 IP_ADD_MEMBERSHIP加入组播组，需针对每个端口采用 IP_ADD_MEMBERSHIP。
接收组播数据包。

注意:
 接收组播的网络端口需要设定一个IP地址，我调试的计算机有两个端口，我在第二个端口上接收组播，开始没有设定这个端口的IP地址，
 只是给出了组播路由到第二个端口，结果死活收不到数据，后来设了一个IP地址就ok了

服务器直播源会采用组播方式，服务器在接收组播的时候要注意一下两点：

  1、必须为接收组播的网卡配置组播路由，例如要在eth0网卡上接收239.10.10.100：5123的组播，则要添加组播路由239.10.10.0

   route -add net 239.10.10.0 netmask 255.255.255.0 dev eth0

  2、要确保服务器防火墙是关闭的，

查看防火墙状态  service iptables status

启动关闭防火墙   service iptables stop |start

查看防火墙是否开机自启动chkconfig
 iptables --list

如果3、5是ON的话那就是开机自动

  3、多网卡接收组播的时候，除了面所说的注意事项外，还要检查网关配置，因为机器要接收组播，必须先加入组播，
加入组播需要机器往外发送数据包，若网关设置不正确，那加入组播的报就不能到达管理组播的交换机，交换机也就不知道本机器需要收组播，
自然不会把组播包发过来。

*/

int32 netUdpGroupBroadcastClient(int32 port, uint8 *ipAddr, netFunctionRun function)
{
	struct sockaddr_in myAddr;
	int32 sock;
	int32 len;
  int32 opt;
  int32 ret = -1;
  
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
  {
    LOG_ERROR("udp socket connect error\n");
    return TASK_ERROR;
  } 
  
  memset(&myAddr, 0, sizeof(struct sockaddr_in));
  
  LOG_DBG("group send socket create successfully.\n");

  myAddr.sin_family = AF_INET;
	myAddr.sin_port = htons(port);
  inet_aton(ipAddr, (struct in_addr *)&myAddr.sin_addr.s_addr);

  if (myAddr.sin_addr.s_addr == 0)
  {
    LOG_ERROR("error ip address error\n");
    close(sock);
    return TASK_ERROR;
  }
  
  /* 绑定自己的端口和IP信息到socket上 */ 
  if (bind (sock, (struct sockaddr *) &myAddr, sizeof (struct sockaddr_in)) < 0)
  {     
    LOG_ERROR("bind ip address error\n");
    close(sock);
    return TASK_ERROR;
  }

  function(sock, &myAddr);

  close(sock);
  
  return TASK_OK;
}

/*
一般我们需要先使用gethostbyname,得到服务器的信息。然后使用socket(AF_INET,SOCK_DGRAM,0)建立套接字，
我们接着调用 setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&share,sizeof(share)),
其中，char share = 1, sockfd是socket建立的套接字，这一步就是允许了多进程共享同一个端口。
接着，是通用的bzero(), 给sockaddr_in填入信息，bind(),下来，我们要通知Linux kernel来的数据是广播数据，
这一步通过给optval付值来搞定，如 optval.imr_multiaddr.s_addr = inet_addr(\"224.0.0.1\"); 
optval.imr_interface.s_addr = htonl(INADDR_ANY); 先面的这一步，则用来使自己的主机加入一个广播组：
setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &optval, sizeof(command)); 
现在，你可以使用recvfrom()来接收多播数据了，当然，最后你还要使用： 
setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &optval, sizeof(optval)）； 来退出多播组。 
*/
int32 netUdpGroupBroadcastServer(int32 port, uint8 *ipGroupAddr, uint8 *peerAddr, netFunctionRun function)
{
	struct sockaddr_in serverAddr;
	int32 sock;
	int32 len;
  int32 ret = -1;
  struct in_addr ia;
  struct hostent *group;
	struct ip_mreq mreq;
  
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
  {
    LOG_ERROR("udp socket connect error\n");
    return TASK_ERROR;
  } 
  
  memset(&serverAddr, 0, sizeof(struct sockaddr_in));
  
  if ((group = gethostbyname(ipGroupAddr)) == (struct hostent *) 0) 
  {
    perror("gethostbyname");
    LOG_ERROR("udp socket opt error\n");
    close(sock);
    return TASK_ERROR;
  }

	bcopy((void *) group->h_addr, (void *) &ia, group->h_length);
	/* 设置组地址 */
	bcopy(&ia, &mreq.imr_multiaddr.s_addr, sizeof(struct in_addr));

  /* 设置发送组播消息的源主机的地址信息 */
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  
  /* 把本机加入组播地址，即本机网卡作为组播成员，只有加入组才能收到组播消息 */
  ret = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));	
  if (ret < 0)
  {
    LOG_ERROR("udp socket opt error\n");
    close(sock);
    return TASK_ERROR;
  }


  serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
  inet_aton(peerAddr, (struct in_addr *)&serverAddr.sin_addr.s_addr);
  
  if (serverAddr.sin_addr.s_addr == 0)
  {
    LOG_ERROR("error ip address error\n");
    close(sock);
    return TASK_ERROR;
  }

	if (bind(sock, (struct sockaddr *) &serverAddr,sizeof(struct sockaddr_in)) < 0) 
	{
    LOG_ERROR("bind ip address error\n");
    close(sock);
    return TASK_ERROR;
	}

  function(sock, &serverAddr);

  close(sock);
  
  return TASK_OK;
}

int32 netUdpRev(int32 sock, uint8 *ipAddr, uint8 *buff)
{
  struct sockaddr_in addr;
  socklen_t addrLen;
  int32 len;
  uint8 *str = NULL;
  
  if (sock < 0)
  {
    LOG_ERROR("udp socket fd error\n");
    return TASK_ERROR;
  }
  
  addrLen = sizeof(addr);

  len = recvfrom(sock, buff, sizeof(buff) - 1, 0,(struct sockaddr *) &addr, &addrLen);
	if (len < 0) 
	{
    LOG_ERROR("udp socket rev error\n");
    close(sock);
    return TASK_ERROR;
	}

  LOG_DBG("recive come from %s:%d\n\r",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

  str = inet_ntoa(addr.sin_addr);
  if (str != NULL && (sizeof(ipAddr) >= strlen(str)))
  {
    strcpy(ipAddr, str);
  }
  
  return len;
}

int32 netUdpSend(int32 sock, struct sockaddr_in addr, uint8 *buff, int32 bufLen)
{
  socklen_t addrLen;
  int32 len;
  uint8 *str = NULL;
  
  if (sock < 0)
  {
    LOG_ERROR("udp socket fd error\n");
    return TASK_ERROR;
  }
  
  addrLen = sizeof(addr);

  len = sendto(sock, buff, bufLen, 0,(struct sockaddr *) &addr, addrLen);
	if (len < 0) 
	{
    LOG_ERROR("udp socket send error\n");
    close(sock);
    return TASK_ERROR;
	}

  LOG_DBG("send success\n\r");
  
  return len;
}

/*
Linux下的IPC－UNIX Domain Socket 
 一、 概述

UNIX Domain Socket是在socket架构上发展起来的用于同一台主机的进程间通讯（IPC），它不需要经过网络协议栈，不需要打包拆包、计算校验和、维护序号和应答等，只是将应用层数据从一个进程拷贝到另一个进程。UNIX Domain Socket有SOCK_DGRAM或SOCK_STREAM两种工作模式，类似于UDP和TCP，但是面向消息的UNIX Domain Socket也是可靠的，消息既不会丢失也不会顺序错乱。

UNIX Domain Socket可用于两个没有亲缘关系的进程，是全双工的，是目前使用最广泛的IPC机制，比如X Window服务器和GUI程序之间就是通过UNIX Domain Socket通讯的。

二、工作流程

UNIX Domain socket与网络socket类似，可以与网络socket对比应用。

上述二者编程的不同如下：

    address family为AF_UNIX
    因为应用于IPC，所以UNIXDomain socket不需要IP和端口，取而代之的是文件路径来表示“网络地址”。这点体现在下面两个方面。
    地址格式不同，UNIXDomain socket用结构体sockaddr_un表示，是一个socket类型的文件在文件系统中的路径，这个socket文件由bind()调用创建，如果调用bind()时该文件已存在，则bind()错误返回。
    UNIX Domain Socket客户端一般要显式调用bind函数，而不象网络socket一样依赖系统自动分配的地址。客户端bind的socket文件名可以包含客户端的pid，这样服务器就可以区分不同的客户端。

UNIX Domain socket的工作流程简述如下（与网络socket相同）。

服务器端：创建socket—绑定文件（端口）—监听—接受客户端连接—接收/发送数据—…—关闭

客户端：创建socket—绑定文件（端口）—连接—发送/接收数据—…—关闭

三、阻塞和非阻塞（SOCK_STREAM方式）

读写操作有两种操作方式：阻塞和非阻塞。

1.阻塞模式下

阻塞模式下，发送数据方和接收数据方的表现情况如同命名管道，参见本人文章“Linux下的IPC－命名管道的使用（http://blog.csdn.net/guxch/article/details/6828452）”

2.非阻塞模式

在send或recv函数的标志参数中设置MSG_DONTWAIT，则发送和接收都会返回。如果没有成功，则返回值为-1，errno为EAGAIN 或 EWOULDBLOCK。 
*/

static int32 netUnixSockInitServer(uint8 *fileDir)
{
  int32 ret = 0;
  int32 listenfd;
  struct sockaddr_un servaddr;

  listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (listenfd < 0) 
  {
    displayErrorMsg("unix socket failed to create sock\n");
    return TASK_ERROR; 
  }

  bzero(&servaddr, sizeof(servaddr));

  servaddr.sun_family = AF_UNIX;
  strcpy(servaddr.sun_path, fileDir);
  
  ret = bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
  if(ret < 0)
  {
    displayErrorMsg("unix socket failed to bind sock\n");
    close(listenfd);
    return TASK_ERROR; 
  }

  ret = listen(listenfd, 1);
  if(ret < 0)
  {
    displayErrorMsg("unix socket failed to listen sock\n");
    close(listenfd);
    return TASK_ERROR; 
  }

  LOG_DBG("unix socket socket init ok\n");
  return listenfd;
}

static int32 netUnixSockAccept(int32 listenfd, netFunctionRun function)
{
  struct sockaddr_un clientaddr;
  int32 acceptfd;
  int32 clientLen;
  int32 ret = -1;

  clientLen = sizeof(clientaddr);
  acceptfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientLen);
  if (acceptfd <= 0) 
  {
    LOG_ERROR("unix socket failed to acccept sock\n");
    return TASK_ERROR; 
  }

  ret = function(acceptfd, NULL);

  close(acceptfd);

  return NET_TCP_SOCK_SET;
}

int32 netUnixSockServer(uint8 *fileDir, int32 timeDelay, netFunctionRun function)
{
  int32 ret;
  fd_set fdsr;
  int32 maxSock;
  struct timeval tv;

  unlink(fileDir);
  
  maxSock = netUnixSockInitServer(fileDir);
  if (maxSock == TASK_ERROR)
  {
    LOG_ERROR("unix socket Server error\n");
    return TASK_ERROR; 
  }
    
  while(1)
  {
    FD_ZERO(&fdsr);
    tv.tv_sec = timeDelay;
    tv.tv_usec = 0;
  
    FD_SET(maxSock, &fdsr);

    /*进程阻塞*/
    ret = select(maxSock + 1, &fdsr, NULL, NULL, &tv);
    if (ret < 0) 
    {
      LOG_ERROR("unix socket failed to select sock\n");
      close(maxSock);
      return TASK_ERROR; 
    } 
    else if (ret == 0) 
    {
      LOG_DBG("unix socket server timeout\n");
      continue;
    } 

    if (FD_ISSET(maxSock, &fdsr) )
    {
      ret = netUnixSockAccept(maxSock, function);
      if (ret == NET_TCP_SOCK_SET)
      {
        LOG_DBG("unix socket server continue\n");
        continue;
      }
      else
      {
        LOG_DBG("unix socket server break\n");
        break;
      }
    }
  }

  close(maxSock);
  unlink(fileDir);
  return TASK_OK;
}

int32 netUnixSockClient(uint8 *fileDir, netFunctionRun function)
{
  int32 clientSock;
  struct sockaddr_un serverAddr;
  int32 ret = -1;
  int32 len;
  
  clientSock = socket(AF_UNIX,SOCK_STREAM,0);
  if(clientSock < 0)
  {
    LOG_ERROR("client socket error\n");
    return TASK_ERROR;
  }
  
  LOG_DBG("client socket create successfully.\n");

  memset(&serverAddr,0,sizeof(serverAddr));
  
  serverAddr.sun_family=AF_UNIX;
  strcpy(serverAddr.sun_path, "server_socket");

  len = sizeof(serverAddr);

  ret = connect(clientSock, (struct sockaddr *)&serverAddr, len);
  if (ret < 0)
  {
    LOG_ERROR("client socket connect error\n");
    close(clientSock);
    return TASK_ERROR;
  }

  function(clientSock, NULL);
  close(clientSock);

  return TASK_OK;
}


/***************************************************/
/*
函数gethostbyname()是完成域名转换的。由于IP地址难以记忆和读写，所以为了方便，人们常常用域名来表示主机，这就需要进行域名和IP地址的转换。函数原型为：
　　struct hostent *gethostbyname(const char *name); 

当 gethostname()调用成功时，返回指向struct hosten的指针，当调用失败时返回-1。当调用gethostbyname时，你不能使用perror()函数来输出错误信息，而应该使用herror()函数来输出。 

函数返回为hosten的结构类型，它的定义如下：

truct hostent {
　 char *h_name;  主机的官方域名 official name of host
　　 char **h_aliases; 一个以NULL结尾的主机别名数组 
　　 int h_addrtype; 返回的地址类型，在Internet环境下为AF-INET 
　　int h_length;  地址的字节长度 
　　 char **h_addr_list;  一个以0结尾的数组，包含该主机的所有地址
　　};
　　#define h_addr h_addr_list[0] 在h-addr-list中的第一个地址

它的使用注意点是：

    这个指针指向一个静态数据，它会被后继的调用所覆盖。简单的说，它是多线程或者多进程不安全的。
    我们最好使用h_addr代理直接使用h_addr_list，这样能够提高日后的兼容性。
    h_addr是指向一个长度为h_length的主机地址，它不是网络格式，所以在赋值给struct in_addr时，应该通过htonl来转化。我们可以通过下面一个学习程序来说明这种情况。
    如果我们使用GNU环境，我们可以使用gethostbyname_r或者gethostbyname2_r来替换掉gethostbyname函数。它们能够良好的解决多线程或多进程安全性问题，并且提供选择地址族参数。

gethostbyname函数（为了获得对方的IP地址）

在linux环境下，如果连接网络 gethostbyname会连接DNS服务器（具体的服务器根据配置而定），DNS服务器会反回传入域名所对应的IP地址，如果是两台嵌入式设备连接（也是linux环境），那么此函数会在/etc/hosts/当前目录下寻找输入域名所对应的IP，所以，在两台设备进行通信之前，必须要在此文件下设置要连接的域名以及对应的IP地址

gethostname() ： 返回本地主机的标准主机名。

原型如下：

#include <unistd.h>

int gethostname(char *name, size_t len);

参数说明：

这个函数需要两个参数：

接收缓冲区name，其长度必须为len字节或是更长,存获得的主机名。

接收缓冲区name的最大长度

返回值：

如果函数成功，则返回0。如果发生错误则返回-1。错误号存放在外部变量errno中。


gethostbyname()函数说明——用域名或主机名获取IP地址
    包含头文件
    #include <netdb.h>
    #include <sys/socket.h>

    函数原型
    struct hostent *gethostbyname(const char *name);
    这个函数的传入值是域名或者主机名，例如"www.google.cn"等等。传出值，是一个hostent的结构。如果函数调用失败，将返回NULL。

*/
/*通过域名返回主机信息*/
int32 getHostNameInfoLocal(uint8 *hostName)
{
  if (hostName == NULL)
  {
    LOG_ERROR("hostName no buffer space\n");
    return TASK_ERROR;
  } 

  if (gethostname(hostName, sizeof(hostName)) < 0)
  {
    LOG_ERROR("gethostname error\n");
    return TASK_ERROR;
  }

  return TASK_OK;
}

int32 showHostInfoByDomain(uint8 *hostName)
{
  struct hostent *hptr;
  int8 *name, **pptr, str[32];
  int32 count = 0;
  
  if (hostName == NULL)
  {
    LOG_ERROR("hostName is empty\n");
    return TASK_ERROR;
  } 

  hptr = gethostbyname(hostName);
  if (hptr) 
  {
    printf("the offical name is %s.\n", hptr->h_name);
    
    for(pptr = hptr->h_aliases; *pptr != NULL; pptr++) 
    {
      printf("the alias name is %s\n", *pptr);
    }

    switch (hptr->h_addrtype) 
    {
      case AF_INET:
        printf("the address type is AF_INET.\n");
        break;
      case AF_INET6:
        printf("the address type is AF_INET6.\n");
        break;
      default:
        break;
    }
    
    printf("the address length is %d Bytes.\n", hptr->h_length);
    
    for (pptr = hptr->h_addr_list; *pptr != NULL; pptr++) 
    {
      count ++;
      printf("the %dth address is %s.\n", count, inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));  //即将转换成的点分十进制存到字符串 str 中返回，溢出则返回空指针
    }
  } 
  else 
  {
    LOG_ERROR("gethostbyname Error!\n");
    return TASK_ERROR;
  }

  return TASK_OK;
}

/*
getservbyname
struct servent *getservbyname(const char *name, const char *proto);

这个函数可以返回给定服务名和协议名的相关服务信息。

函数从 /etc/service 文件中根据服务名称到端口号的映射关系获取相关信息，如果函数执行成功，那么返回一个 struct servent 结构指针，该结构定义如下：

参数：服务名和协议名

返回值：

    一个指向 servent 结构体的指针
    空指针(发生错误)

从netdb.h头文件我们可以找到 servent 结构体的说明：

struct servent {
    char   *s_name;       official service name 
    char  **s_aliases;    other aliases 
    int     s_port;       port for this service 
    char  **s_proto;      protocol to use 
    };

返回的结构体中的端口号是按网络字节顺序保存的整数，输出的时候要用 ntohs() 函数转换按主机顺序保存的整数。
*/

int32 showServiceInfo(uint8 *service, uint8 *protocol)
{
  struct servent *sptr;
   
  if (service == NULL || protocol == NULL)
  {
    LOG_ERROR("server or proto is empty\n");
    return TASK_ERROR;
  }

  sptr = getservbyname(service, protocol);
  if (sptr) 
    {
        printf("the port of service %s using %s protocol is %d.\n", sptr->s_name, protocol, ntohs(sptr->s_port));  //将网络字节顺序的端口值转换成主机顺序
    } 
  else 
    {
      LOG_ERROR("getservbyname error\n");
      return TASK_ERROR;
    }

  return TASK_OK;
}

/*
gethostbyaddr(3)函数

这个函数，传入值是IP地址（注意，这里不是简单的字符串，需要先将字符串形式的IP地址由inet_aton转化一下），然后经过函数处理，将结果由返回值传出。返回值是一个hostent结构.因为有了hosten这个传出的结构，我们可以在这个结构里面找到我们想需要的信息。

有时我们知道一个IP地址，但是我们报告主机，而不是IP地址。一个服务器也许需要记录与其连接的客户端主机名，而不仅仅是IP地址。gethostbyaddr函数概要如下：
#include <sys/socket.h>
struct hostent *gethostbyaddr(
        const char *addr,
        int len,         
    int type);       
gethostbyaddr函数接受三个输入参数：
1 要转换为主机名的输入地址(addr)。对于AF_INET地址类型，这是指向地址结构中的sin_addr成员。
2 输入地址的长度。对于AF_INET类型，这个值为4；而对于AF_INET6类型，这个值为16。
3 输入地址的类型，这个值为AF_INET或是AF_INET6。

在这里要注意，第一个参数为一个字符指针，实质上允许接受多种格式的地址。我们需要将我们的地址指针转换为(char *)来满足编译。第二个参数指明了所提供的地址的长度。

第三个参数为所传递的地址的类型。对IPv4的网络为AF_INET，也许在将来，这个值将会是IPv6地址格式的AF_INET6。
---------------
正在想为什么会这样的时候，看到UNP里面的一句话： 按照DNS的说法，gethostbyaddr在in_addr.arpa域中向一个名字服务器查询PTR记录。

　　可能是我的电脑不是服务器吧，没有域名解析服务吧。所以不行。而本地的/etc/hosts差不多就是有这个功能。我就在想为什么gethostbyname会向/etc/hosts文件中查看信息，然后没有对应的话，就会返回上一级的DNS进行解析。而反向解析为什么不会自动解析呢？（Ps我想会不会是反向解析比较少用到，而且正向解析域名有层次关系，而IP没有层次关系，不方便处理吧。）我通过nslookup 115.239.211.110 进行查询时提示这个错误：

 

** server can't find 110.211.239.115.in-addr.arpa.: NXDOMAIN

　　好了，没错了，要使用这个函数，本地要有反向解析的服务。
*/

int32 showRemoteServeInfo(uint8 *ipAddr)
{
  int8 *ptr,**pptr;
  struct hostent *hptr;
  int8 str[32];
  int8 ipaddr[16];
  struct in_addr *hipaddr;
 
  /* 取得命令后第一个参数，即要解析的IP地址 */
  ptr = ipAddr;
  /* 调用inet_aton()，ptr就是以字符串存放的地方的指针，hipaddr是in_addr形式的地址 */
 
  if(!inet_aton(ptr,hipaddr))
  {
    LOG_ERROR("inet_aton error\n");
    return TASK_ERROR;
  }
 
  /* 调用gethostbyaddr()。调用结果都存在hptr中 */
  if((hptr = gethostbyaddr(hipaddr, 4, AF_INET)) == NULL )
  {
   LOG_ERROR("gethostbyaddr error for addr:%s\n", ptr);
   return TASK_ERROR; /* 如果调用gethostbyaddr发生错误，返回1 */
  }
 
  /* 将主机的规范名打出来 */
  printf("official hostname:%s\n",hptr->h_name);

  /* 主机可能有多个别名，将所有别名分别打出来 */
  for(pptr = hptr->h_aliases; *pptr != NULL; pptr++)
  {
    printf("  alias:%s\n",*pptr);
  }
  
  /* 根据地址类型，将地址打出来 */
  switch(hptr->h_addrtype)
  {
   case AF_INET:
   case AF_INET6:
    pptr=hptr->h_addr_list;
    /* 将刚才得到的所有地址都打出来。其中调用了inet_ntop()函数 */
    for(;*pptr!=NULL;pptr++)
    {
      printf("  address:%s\n", inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
    }
    break;
   default:
    LOG_ERROR("unknown address type\n");
    break;
  }
  
  return TASK_OK;
}

/*
Linux网络程序与内核交互的方法是通过ioctl来实现的，ioctl与网络协议栈进行交互，可得到网络接口的信息，网卡设备的映射属性和配置网络接口.并且还能够查看，修改，删除ARP高速缓存的信息，所以，我们有必要了解一下ioctl函数的具体实现.

2.相关结构体与相关函数

#include <sys/ioctl.h>

int ioctl(int d,int request,....);

参数:

d-文件描述符，这里是对网络套接字操作，显然是套接字描述符

request-请求码

省略的部分对应不同的内存缓冲区，而具体的内存缓冲区是由请求码request来决定的,下面看一下具体都有哪些相关缓冲区。

(1)网络接口请求结构ifreq

struct ifreq{
#define IFHWADDRLEN 6 //6个字节的硬件地址，即MAC
union{
char ifrn_name[IFNAMESIZ];//网络接口名称
}ifr_ifrn;
union{
struct sockaddr ifru_addr;//本地IP地址
struct sockaddr ifru_dstaddr;//目标IP地址
struct sockaddr ifru_broadaddr;//广播IP地址
struct sockaddr ifru_netmask;//本地子网掩码地址
struct sockaddr ifru_hwaddr;//本地MAC地址
short ifru_flags;//网络接口标记
int ifru_ivalue;//不同的请求含义不同
struct ifmap ifru_map;//网卡地址映射
int ifru_mtu;//最大传输单元
char ifru_slave[IFNAMSIZ];//占位符
char ifru_newname[IFNAMSIZE];//新名称
void __user* ifru_data;//用户数据
struct if_settings ifru_settings;//设备协议设置
}ifr_ifru;
}
#define ifr_name ifr_ifrn.ifrn_name;//接口名称
#define ifr_hwaddr ifr_ifru.ifru_hwaddr;//MAC
#define ifr_addr ifr_ifru.ifru_addr;//本地IP
#define ifr_dstaddr ifr_ifru.dstaddr;//目标IP
#define ifr_broadaddr ifr_ifru.broadaddr;//广播IP
#define ifr_netmask ifr_ifru.ifru_netmask;//子网掩码
#define ifr_flags ifr_ifru.ifru_flags;//标志
#define ifr_metric ifr_ifru.ifru_ivalue;//接口侧度
#define ifr_mtu ifr_ifru.ifru_mtu;//最大传输单元
#define ifr_map ifr_ifru.ifru_map;//设备地址映射
#define ifr_slave ifr_ifru.ifru_slave;//副设备
#define ifr_data ifr_ifru.ifru_data;//接口使用
#define ifr_ifrindex ifr_ifru.ifru_ivalue;//网络接口序号
#define ifr_bandwidth ifr_ifru.ifru_ivalue;//连接带宽
#define ifr_qlen ifr_ifru.ifru_ivalue;//传输单元长度
#define ifr_newname ifr_ifru.ifru_newname;//新名称
#define ifr_seeting ifr_ifru.ifru_settings;//设备协议设置

如果想获得网络接口的相关信息，就传入ifreq结构体.


(2)网卡设备属性ifmap

struct ifmap{//网卡设备的映射属性
unsigned long mem_start;//开始地址
unsigned long mem_end;//结束地址
unsigned short base_addr;//基地址
unsigned char irq;//中断号
unsigned char dma;//DMA
unsigned char port;//端口
}


(3)网络配置接口ifconf

struct ifconf{//网络配置结构体是一种缓冲区
int ifc_len;//缓冲区ifr_buf的大小
union{
char__user *ifcu_buf;//绘冲区指针
struct ifreq__user* ifcu_req;//指向ifreq指针
}ifc_ifcu;
};
#define ifc_buf ifc_ifcu.ifcu_buf;//缓冲区地址
#define ifc_req ifc_ifcu.ifcu_req;//ifc_req地址


(4)ARP高速缓存操作arpreq

/**
ARP高速缓存操作，包含IP地址和硬件地址的映射表
操作ARP高速缓存的命令字 SIOCDARP,SIOCGARP,SIOCSARP分别是删除ARP高速缓存的一条记录，获得ARP高速缓存的一条记录和修改ARP高速缓存的一条记录
struct arpreq{
struct sockaddr arp_pa;//协议地址
struct sockaddr arp_ha;//硬件地址
int arp_flags;//标记
struct sockaddr arp_netmask;//协议地址的子网掩码
char arp_dev[16];//查询网络接口的名称
}


3. 请求码request

SIOCGIFCONF 获取所有接口的清单

SIOCSIFADDR 设置接口地址

SIOCGIFADDR 获取接口地址

SIOCSIFFLAGS  设置接口标志

SIOCGIFFLAGS  获取接口标志

SIOCSIFDSTADDR  设置点到点地址

SIOCGIFDSTADDR  获取点到点地址

SIOCGIFBRDADDR  获取广播地址

SIOCSIFBRDADDR  设置广播地址

SIOCGIFNETMASK  获取子网掩码

SIOCSIFNETMASK  设置子网掩码

SIOCGIFMETRIC 获取接口的测度

SIOCSIFMETRIC 设置接口的测度

SIOCGIFMTU  获取接口MTU

SIOCSARP  创建/修改ARP表项

SIOCGARP  获取ARP表项

SIOCDARP  删除ARP表项

SIOCGIFMETRIC, SIOCSIFMETRIC
    使用 ifr_metric 读取 或 设置 设备的 metric 值. 该功能 目前 还没有 实现. 读取操作 使 ifr_metric 置 0, 而 设置操作 则 返回 EOPNOTSUPP.
SIOCGIFMTU, SIOCSIFMTU
    使用 ifr_mtu 读取 或 设置 设备的 MTU(最大传输单元). 设置 MTU 是 特权操作. 过小的 MTU 可能 导致 内核 崩溃.
SIOCGIFHWADDR, SIOCSIFHWADDR
    使用 ifr_hwaddr 读取 或 设置 设备的 硬件地址. 设置 硬件地址 是 特权操作.
SIOCSIFHWBROADCAST
    使用 ifr_hwaddr 读取 或 设置 设备的 硬件广播地址. 这是个 特权操作.
SIOCGIFMAP, SIOCSIFMAP
    使用 ifr_map 读取 或 设置 接口的 硬件参数. 设置 这个参数 是 特权操作. 

*/
int32 showAllIpaddrByeth(void)
{
  int32 i=0;
  int32 sockfd;
  struct ifconf ifconf;
  uint8 buf[512];

  struct ifreq *ifreq;
  
  //初始化ifconf
  ifconf.ifc_len = 512;
  ifconf.ifc_buf = buf;

  if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    LOG_ERROR("socket error\n");
    return TASK_ERROR;
  }  

  ioctl(sockfd, SIOCGIFCONF, &ifconf);    //获取所有接口信息
  
  //接下来一个一个的获取IP地址

  ifreq = (struct ifreq*)buf;  

  for(i=(ifconf.ifc_len/sizeof(struct ifreq)); i > 0; i--)
  {
    //if(ifreq->ifr_flags == AF_INET){            //for ipv4

    printf("name = [%s]\n", ifreq->ifr_name);
    printf("local addr = [%s]\n",inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr));
    ifreq++;
    //}
  }

  return TASK_OK;
}

int32 getMacaddrByEth(uint8 *ifName,uint8 *mac)
{
  int32 i;
  struct ifreq ifreq;
  int32 sock;

  if((sock=socket(AF_INET,SOCK_STREAM,0)) < 0)
  {
    LOG_ERROR("socket error\n");
    return TASK_ERROR;
  }
  
  strcpy(ifreq.ifr_name,ifName);
  if(ioctl(sock,SIOCGIFHWADDR,&ifreq)<0)
  {
    LOG_ERROR("socket SIOCGIFHWADDR error\n");
    return TASK_ERROR;
  }
  
  for (i=0; i<6; i++)
  {
    sprintf(mac+3*i, "%02x:", (unsigned char)ifreq.ifr_hwaddr.sa_data[i]);
  }
  
  mac[17]='\0';
  
  LOG_DBG("mac addr is: %s\n", mac);
  
  return TASK_OK;
}

int32 getIpaddrByEth(uint8 *eth, uint8 *ipAddr)
{
  int32 inet_sock;
  struct ifreq ifr;
  
  if (eth == NULL)
  {
    LOG_ERROR("eth is empty\n");
    return TASK_ERROR;
  }  
  
  inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (inet_sock < 0)
  {
    LOG_ERROR("socket error\n");
    return TASK_ERROR;
  }
  
  strcpy(ifr.ifr_name, eth);
  if (ioctl(inet_sock, SIOCGIFADDR, &ifr) < 0)
  {
    LOG_ERROR("socket SIOCGIFADDR error\n");
    return TASK_ERROR;
  }

  strcpy(ipAddr, inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));
  
  LOG_DBG("ipaddr is %s\n", ipAddr);

  return TASK_OK;
}

int showNetInterfaceInfo(void)
{
	int32 s;/* 套接字描述符 */
	int32 err = -1;/*错误值*/

	/* 建立一个数据报套接字 */
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)	
  {
		LOG_ERROR("socket() 出错\n");
		return TASK_ERROR;
	}

	/* 获得网络接口的名称 */
	{
		struct ifreq ifr;
		ifr.ifr_ifindex = 2; /* 获取第2个网络接口的名称 */
		err = ioctl(s, SIOCGIFNAME, &ifr);
		if(err)
    {
			LOG_ERROR("SIOCGIFNAME Error\n");
		}
    else
		{
			printf("the %dst interface is:%s\n",ifr.ifr_ifindex,ifr.ifr_name);
		}
	}

	/* 获得网络接口配置参数 */
	{
		/* 查询网卡“eth0”的情况 */
		struct ifreq ifr;
		memcpy(ifr.ifr_name, "eth0",5);

		/* 获取标记 */
		err = ioctl(s, SIOCGIFFLAGS, &ifr);
		if(!err)
    {
			printf("SIOCGIFFLAGS:%d\n",ifr.ifr_flags);
		}

		/* 获取METRIC */
		err = ioctl(s, SIOCGIFMETRIC, &ifr);
		if(!err)
    {
			printf("SIOCGIFMETRIC:%d\n",ifr.ifr_metric);
		}
		
		/* 获取MTU */		
		err = ioctl(s, SIOCGIFMTU, &ifr);
		if(!err)
    {
			printf("SIOCGIFMTU:%d\n",ifr.ifr_mtu);
		}	
		
		/* 获取MAC地址 */
		err = ioctl(s, SIOCGIFHWADDR, &ifr);
		if(!err)
    {
			unsigned char *hw = ifr.ifr_hwaddr.sa_data;
			printf("SIOCGIFHWADDR:%02x:%02x:%02x:%02x:%02x:%02x\n",hw[0],hw[1],hw[2],hw[3],hw[4],hw[5]);
		}	
		
		/* 获取网卡映射参数 */
		err = ioctl(s, SIOCGIFMAP, &ifr);
		if(!err)
    {
			printf("SIOCGIFMAP,mem_start:%d,mem_end:%d, base_addr:%d, dma:%d, port:%d\n",
				ifr.ifr_map.mem_start, 	/*开始地址*/
				ifr.ifr_map.mem_end,		/*结束地址*/
				ifr.ifr_map.base_addr,	/*基地址*/
				ifr.ifr_map.irq ,				/*中断*/
				ifr.ifr_map.dma ,				/*DMA*/
				ifr.ifr_map.port );			/*端口*/
		}
		
		/* 获取网卡序号 */
		err = ioctl(s, SIOCGIFINDEX, &ifr);
		if(!err)
    {
			printf("SIOCGIFINDEX:%d\n",ifr.ifr_ifindex);
		}
		
		/* 获取发送队列长度 */		
		err = ioctl(s, SIOCGIFTXQLEN, &ifr);
		if(!err)
    {
			printf("SIOCGIFTXQLEN:%d\n",ifr.ifr_qlen);
		}			
	}

	/* 获得网络接口IP地址 */
	{
		struct ifreq ifr;
		/* 方便操作设置指向sockaddr_in的指针 */
		struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
		char ip[16];/* 保存IP地址字符串 */
		memset(ip, 0, 16);
		memcpy(ifr.ifr_name, "eth0",5);/*查询eth0*/
		
		/* 查询本地IP地址 */		
		err = ioctl(s, SIOCGIFADDR, &ifr);
		if(!err)
    {
			/* 将整型转化为点分四段的字符串 */
			inet_ntop(AF_INET, &sin->sin_addr.s_addr, ip, 16 );
			printf("SIOCGIFADDR:%s\n",ip);
		}
		
		/* 查询广播IP地址 */
		err = ioctl(s, SIOCGIFBRDADDR, &ifr);
		if(!err)
    {
			/* 将整型转化为点分四段的字符串 */
			inet_ntop(AF_INET, &sin->sin_addr.s_addr, ip, 16 );
			printf("SIOCGIFBRDADDR:%s\n",ip);
		}
		
		/* 查询目的IP地址 */
		err = ioctl(s, SIOCGIFDSTADDR, &ifr);
		if(!err)
    {
			/* 将整型转化为点分四段的字符串 */
			inet_ntop(AF_INET, &sin->sin_addr.s_addr, ip, 16 );
			printf("SIOCGIFDSTADDR:%s\n",ip);
		}
		
		/* 查询子网掩码 */
		err = ioctl(s, SIOCGIFNETMASK, &ifr);
		if(!err)
    {
			/* 将整型转化为点分四段的字符串 */
			inet_ntop(AF_INET, &sin->sin_addr.s_addr, ip, 16 );
			printf("SIOCGIFNETMASK:%s\n",ip);
		}
	}

	/* 测试更改IP地址 */
#if 0
	{
		struct ifreq ifr;
		/* 方便操作设置指向sockaddr_in的指针 */
		struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
		char ip[16];/* 保存IP地址字符串 */
		int err = -1;
		
		/* 将本机IP地址设置为192.169.1.175 */
		printf("Set IP to 192.168.1.175\n");
		memset(&ifr, 0, sizeof(ifr));/*初始化*/
		memcpy(ifr.ifr_name, "eth0",5);/*对eth0网卡设置IP地址*/
		inet_pton(AF_INET, "192.168.1.175", &sin->sin_addr.s_addr);/*将字符串转换为网络字节序的整型*/
		sin->sin_family = AF_INET;/*协议族*/
		err = ioctl(s, SIOCSIFADDR, &ifr);/*发送设置本机IP地址请求命令*/
		if(err)
    {/*失败*/
			LOG_ERROR("SIOCSIFADDR error\n");
		}
    else
    {/*成功，再读取一下进行确认*/
			printf("check IP --");
			memset(&ifr, 0, sizeof(ifr));/*重新清0*/
			memcpy(ifr.ifr_name, "eth0",5);/*操作eth0*/
			ioctl(s, SIOCGIFADDR, &ifr);/*读取*/
			inet_ntop(AF_INET, &sin->sin_addr.s_addr, ip, 16);/*将IP地址转换为字符串*/
			printf("%s\n",ip);/*打印*/
		}		
	}
#endif
	close(s);
	return 0;
}

/*
#include <sys/socket.h>

  int setsockopt( int socket, int level, int option_name,const void *option_value, size_t option_len);
  
第一个参数socket是套接字描述符。

第二个参数level是被设置的选项的级别，如果想要在套接字级别上设置选项，就必须把level设置为SOL_SOCKET。

第三个参数 option_name指定准备设置的选项，option_name可以有哪些常用取值，这取决于level，以linux 2.6内核为例（在不同的平台上，这种关系可能会有不同），在套接字级别上(SOL_SOCKET)：


1. SO_BROADCAST 套接字选项

     本选项开启或禁止进程发送广播消息的能力。只有数据报套接字支持广播，并且还必须是在支持广播消息的网络上（例如以太网，令牌环网等）。我们不可能在点对点链路上进行广播，也不可能在基于连接的传输协议（例如TCP和SCTP）之上进行广播。

2. SO_DEBUG 套接字选项

     本选项仅由TCP支持。当给一个TCP套接字开启本选项时，内核将为TCP在该套接字发送和接受的所有分组保留详细跟踪信息。这些信息保存在内核的某个环形缓冲区中，并可使用trpt程序进行检查。

3. SO_KEEPALIVE 套接字选项

     给一个TCP套接字设置保持存活选项后，如果2小时内在该套接字的任何一方向上都没有数据交换，TCP就自动给对端发送一个保持存活探测分节。这是一个对端必须相应的TCP分节，它会导致以下3种情况之一。

（1）对端以期望的ACK响应。应用进程得不到通知（因为一切正常）。在又经过仍无动静的2小时后，TCP将发出另一个探测分节。

（2）对端以RST响应，它告知本端TCP：对端已崩溃且已重新启动。该套接字的待处理错误被置为ECONNRESET，套接字本身则被关闭。

（3）对端对保持存活探测分节没有任何响应。

     如果根本没有对TCP的探测分节的响应，该套接字的待处理错误就被置为ETIMEOUT，套接字本身则被关闭。然而如果该套接字收到一个ICMP错误作为某个探测分节的响应，那就返回响应的错误，套接字本身也被关闭。

     本选项的功能是检测对端主机是否崩溃或变的不可达（譬如拨号调制解调器连接掉线，电源发生故障等等）。如果对端进程崩溃，它的TCP将跨连接发送一个FIN，这可以通过调用select很容易的检测到。

     本选项一般由服务器使用，不过客户也可以使用。服务器使用本选项时因为他们花大部分时间阻塞在等待穿越TCP连接的输入上，也就是说在等待客户的请求。然而如果客户主机连接掉线，电源掉电或者系统崩溃，服务器进程将永远不会知道，并将继续等待永远不会到达的输入。我们称这种情况为半开连接。保持存活选项将检测出这些半开连接并终止他们。


4. SO_LINGER 套接字选项

      本选项指定close函数对面向连接的协议（例如TCP和SCTP，但不是UDP）如何操作。默认操作是close立即返回，但是如果有数据残留在套接字发送缓冲区中，系统将试着把这些数据发送给对端。

    SO_LINGER如果选择此选项，close或 shutdown将等到所有套接字里排队的消息成功发送或到达延迟时间后才会返回。否则，调用将立即返回。

 

              该选项的参数（option_value)是一个linger结构：

 
[cpp] view plaincopyprint?

    struct linger {  
        int   l_onoff;      
        int   l_linger;     
    };  


5. SO_RCVBUF和SO_SNDBUF套接字选项

     每个套接字都有一个发送缓冲区和一个接收缓冲区。

     接收缓冲区被TCP，UDP和SCTCP用来保存接收到的数据，直到由应用进程读取。对于TCP来说，套接字接收缓冲区可用空间的大小限制了TCP通告对端的窗口大小。TCP套接字接收缓冲区不可以溢出，因为不允许对端发出超过本端所通告窗口大小的数据。这就是TCP的流量控制，如果对端无视窗口大小而发出了超过窗口大小的数据，本端TCP将丢弃它们。然而对于UDP来说，当接收到的数据报装不进套接字接收缓冲区时，该数据报就被丢弃。回顾一下，UDP是没有流量控制的：较快的发送端可以很容易的淹没较慢的接收端，导致接收端的UDP丢弃数据报。

     这两个套接字选项允许我们改变着两个缓冲区的默认大小。对于不同的实现，默认值得大小可以有很大的差别。如果主机支持NFS，那么UDP发送缓冲区的大小经常默认为9000字节左右的一个值，而UDP接收缓冲区的大小则经常默认为40000字节左右的一个值。

     当设置TCP套接字接收缓冲区的大小时，函数调用的顺序很重要。这是因为TCP的出口规模选项时在建立连接时用SYN分节与对端互换得到的。对于客户，这意味着SO_RCVBUF选项必须在调用connect之前设置；对于服务器，这意味着该选项必须在调用listen之前给监听套接字设置。给已连接套接字设置该选项对于可能存在的出口规模选项没有任何影响，因为accept直到TCP的三路握手玩抽才会创建并返回已连接套接字。这就是必须给监听套接字设置本选项的原因。

 

6. SO_RCVLOWAT 和 SO_SNDLOWAT套接字选项

     每个套接字还有一个接收低水位标记和一个发送低水位标记。他们由select函数使用，这两个套接字选项允许我们修改这两个低水位标记。

     接收低水位标记是让select返回“可读”时，套接字接收缓冲区中所需的数据量。对于TCP，UDP和SCTP套接字，其默认值为1。发送低水位标记是让select返回“可写”时套接字发送缓冲区中所需的可用空间。对于TCP套接字，其默认值通常为2048。UDP也使用发送低水位标记，然而由于UDP套接字的发送缓冲区中可用空间的字节数从不改变（意味UDP并不为由应用进程传递给它的数据报保留副本），只要一个UDP套接字的发送缓冲区大小大于该套接字的低水位标记，该UDP套接字就总是可写。我们记得UDP并没有发送缓冲区，而只有发送缓冲区大小这个属性。

 

7. SO_RCVTIMEO 和 SO_SNDTIMEO套接字选项

     这两个选项允许我们给套接字的接收和发送设置一个超时值。注意，访问他们的getsockopt和setsockopt函数的参数是指向timeval结构的指针，与select所用参数相同。这可让我们用秒数和微妙数来规定超时。我们通过设置其值为0s和0μs来禁止超时。默认情况下着两个超时都是禁止的。

     接收超时影响5个输入函数：read,readv,recv,recvfrom和recvmsg。发送超时影响5个输出函数：write,writev,send,sendto和sendmsg。

 

8. SO_REUSEADDR 和 SO_REUSEPORT 套接字选项

     SO_REUSEADDR套接字选项能起到以下4个不同的功用。

（1）SO_REUSEADDR允许启动一个监听服务器并捆绑其众所周知的端口，即使以前建立的将该端口用作他们的本地端口的连接仍存在。这个条件通常是这样碰到的：

         （a）启动一个监听服务器；

         （b）连接请求到达，派生一个子进程来处理这个客户；

         （c）监听服务器终止，但子进程继续为现有连接上的客户提供服务；

         （d）重启监听服务器。      

     默认情况下，当监听服务器在步骤d通过调用socket，bind和listen重新启动时，由于他试图捆绑一个现有连接（即正由早先派生的那个子进程处理着的连接）上的端口，从而bind调用会失败。但是如果该服务器在socket和bind两个调用之间设置了SO_REUSEADDR套接字选项，那么将成功。所有TCP服务器都应该指定本套接字选项，以允许服务器在这种情况下被重新启动。

（2）SO_REUSEADDR允许在同一端口上启动同一服务器的多个实例，只要每个实例捆绑一个不同的本地IP地址即可。对于TCP，我们绝对不可能启动捆绑相同IP地址和相同端口号的多个服务器：这是完全重复的捆绑，即使我们给第二个服务器设置了SO_REUSEADDR套接字也不管用。

（3）SO_REUSEADDR 允许单个进程捆绑同一端口到多个套接字上，只要每次捆绑指定不同的本地IP地址即可。

（4）SO_REUSEADDR允许完全重复的捆绑：当一个IP地址和端口号已绑定到某个套接字上时，如果传输协议支持，同样的IP地址和端口还可以捆绑到另一个套接字上。一般来说本特性仅支持UDP套接字。

*/

int32 showSocketInfo(void)
{
	int32 rcvbuf_size;
	int32 sndbuf_size;
	int32 type=0;
	socklen_t size;
	int32 sock_fd;
	struct timeval set_time,ret_time;
  
	if((sock_fd=socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		LOG_ERROR("socket()  AF_INET 出错\n");
		return TASK_ERROR;
	}
  
	size=sizeof(sndbuf_size);
	getsockopt(sock_fd,SOL_SOCKET,SO_SNDBUF,&sndbuf_size,&size);
	printf("sndbuf_size=%d\n",sndbuf_size);
	printf("size=%d\n",size);
	size=sizeof(rcvbuf_size);
	getsockopt(sock_fd,SOL_SOCKET,SO_RCVBUF,&rcvbuf_size,&size);
	printf("rcvbuf_size=%d\n",rcvbuf_size);
	
	size=sizeof(type);
	getsockopt(sock_fd,SOL_SOCKET,SO_TYPE,&type,&size);
	printf("socket type=%d\n",type);
	size=sizeof(struct timeval);
	
	getsockopt(sock_fd,SOL_SOCKET,SO_SNDTIMEO,&ret_time,&size);
	printf("default:time out is %ds,and %dns\n",ret_time.tv_sec,ret_time.tv_usec);

	set_time.tv_sec=10;
	set_time.tv_usec=100;
	setsockopt(sock_fd,SOL_SOCKET,SO_SNDTIMEO,&set_time,size);
	getsockopt(sock_fd,SOL_SOCKET,SO_SNDTIMEO,&ret_time,&size);
	printf("after modify:time out is %ds,and %dns\n",ret_time.tv_sec,ret_time.tv_usec);

	int ttl=0;
	size=sizeof(ttl);
	getsockopt(sock_fd,IPPROTO_IP,IP_TTL,&ttl,&size);
	printf("the default ip  ttl is %d\n",ttl);
	int maxseg=0;
	size=sizeof(maxseg);
	getsockopt(sock_fd,IPPROTO_TCP,TCP_MAXSEG,&maxseg,&size);
	printf("the TCP max seg is %d\n",maxseg);
}

