/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */
#include "sigUtils.h"
#include "logprint.h"

/*
常用信号类型即默认行为

信号名称	产生的效果	对进程默认的效果
SIGINT	Ctrl-C终端下产生	终止当前进程
SIGABRT	产生SIGABRT信号	默认终止进程，并产生core(核心转储)文件
SIGALRM	由定时器如alarm(),setitimer()等产生的定时器超时触发的信号	终止当前进程
SIGCHLD	子进程结束后向父进程发送	忽略
SIGBUS	总线错误，即发生了某种内存访问错误	终止当前进程并产生核心转储文件
SIGKILL	必杀信号，收到信号的进程一定结束，不能捕获	终止进程
SIGPIPE	管道断裂，向已关闭的管道写操作	进程终止
SIGIO	使用fcntl注册I/O事件,当管道或者socket上由I/O时产生此信号	终止当前进程
SIGQUIT	在终端下Ctrl-\产生	终止当前进程，并产生core文件
SIGSEGV	对内存无效的访问导致即常见的“段错误”	终止当前进程，并产生core文件
SIGSTOP	必停信号，不能被阻塞，不能被捕捉	停止当前进程
SIGTERM	终止进程的标准信号	终止当前进程
SIGUSR1：保留给用户使用的信号；
SIGUSR2：同SIGUSR1，保留给用户使用的信号。

//注册信号处理函数
linux主要有两个函数实现信号的安装：signal()、sigaction()。其中signal()只有两个参数，不支持信号传递信息，
主要是用于前32种非实时信号的安装；而sigaction()是较新的函数（由两个系统调用实现：sys_signal以及sys_rt_sigaction），
有三个参数，支持信号传递信息，主要用来与 sigqueue() 系统调用配合使用，当然，sigaction()同样支持非实时信号的安装。
sigaction()优于signal()主要体现在支持信号带有参数。

int sigaction ( int sig , const struct sigaction * restrict act ,\
                struct sigaction * restrict oact ) ;

// 设置信号集为全空
int sigemptyset (sigset_t * set ) ;
// 设置信号集为全满
int sigfillset ( sigset_t * set ) ;
// 将信号signum添加到信号集中
int sigaddset ( sigset_t * set , int signum) ;
// 去除信号集中signum信号
int sigdelset ( sigset_t * set , int signum ) ;
// 判断signum信号是否在信号集set中
int sigismember ( const sigset_t * set , int signum ) ;

flags相关的选项理解：

SA_SIGINFO: 设置此标志位表示可以使用sa_sigaction字段进行注册信号处理函数
SA_RESTART: 被中断的系统调用自动重启
SA_NODEFER: 在执行信号处理函数期间不屏蔽引起信号处理函数执行的信号

*/

/*
针对程序crash情况，回调函数调用栈
SIGABRT 6 C 由abort(3)发出的退出指令 
SIGSEGV 11 C 无效的内存引用 
SIGBUS 10,7,10 C 总线错误(错误的内存访问) 
SIGILL 4 C 非法指令 
*/
int32 sigProcessCrashRegister(void)
{
  int32 rv = TASK_OK;

  struct sigaction sa;

  /* Set up signal handler */
  sa.sa_sigaction = (void *)dumpSigProcessTrace;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_SIGINFO;

  /* Register signal handler */
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGBUS, &sa, NULL);
  sigaction(SIGILL, &sa, NULL);
  sigaction(SIGABRT, &sa, NULL);

  return rv;
}

/*
block集（阻塞集、屏蔽集）：一个进程所要屏蔽的信号，在对应要屏蔽的信号位置1
pending集（未决信号集）：如果某个信号在进程的阻塞集中，则也在未决集中对应位置1，表示该信号不能被递达，不会被处理
handler（信号处理函数集）：表示每个信号所对应的信号处理函数，当信号不在未决集中时，将被调用
 
以下是与信号阻塞及未决相关的函数操作：
#include <signal.h>

int  sigprocmask(int  how,  const  sigset_t *set, sigset_t *oldset))；

int sigpending(sigset_t *set));

int sigsuspend(const sigset_t *mask))；

sigprocmask()函数能够根据参数how来实现对信号集的操作，操作主要有三种：

  SIG_BLOCK 在进程当前阻塞信号集中添加set指向信号集中的信号，相当于：mask=mask|set
  SIG_UNBLOCK 如果进程阻塞信号集中包含set指向信号集中的信号，则解除对该信号的阻塞，相当于：mask=mask|~set
  SIG_SETMASK 更新进程阻塞信号集为set指向的信号集，相当于mask=set
  
sigpending(sigset_t *set))获得当前已递送到进程，却被阻塞的所有信号，在set指向的信号集中返回结果。

sigsuspend(const sigset_t *mask))用于在接收到某个信号之前, 临时用mask替换进程的信号掩码, 并暂停进程执行，直到收到信号为止。

sigsuspend 返回后将恢复调用之前的信号掩码。信号处理函数完成后，进程将继续执行。该系统调用始终返回-1，并将errno设置为EINTR

*/

int8 blockProcSig(sigset_t *set)
{
  if(sigprocmask(SIG_BLOCK, set,NULL) < 0)
  {
    LOG_ERROR("block signal set fail \n");
    return TASK_ERROR;
  }

  return TASK_OK;
}

int8 unblockProcSig(sigset_t *set)
{
  if(sigprocmask(SIG_UNBLOCK,set,NULL) < 0)
  {
    LOG_ERROR("unblock signal set fail \n");
    return TASK_ERROR;
  }

  return TASK_OK;
}

/*
定时器

现在的系统中很多程序不再使用alarm调用，而是使用setitimer调用来设置定时器，用getitimer来得到定时器的状态，

这两个调用的声明格式如下：

#include <sys/time.h>

int getitimer(int which, struct itimerval *curr_value); 
int setitimer(int which, const struct itimerval *new_value,struct itimerval *old_value);

参数:

第一个参数which指定定时器类型
第二个参数是结构itimerval的一个实例，结构itimerval形式
第三个参数可不做处理。
返回值:成功返回0失败返回-1

该系统调用给进程提供了三个定时器，它们各自有其独有的计时域，当其中任何一个到达，就发送一个相应的信号给进程，并使得计时器重新开始。三个计时器由参数which指定，如下所示：

TIMER_REAL：按实际时间计时，计时到达将给进程发送SIGALRM信号。

ITIMER_VIRTUAL：仅当进程执行时才进行计时。计时到达将发送SIGVTALRM信号给进程。

ITIMER_PROF：当进程执行时和系统为该进程执行动作时都计时。与ITIMER_VIR-TUAL是一对，该定时器经常用来统计进程在用户态和内核态花费的时间。计时到达将发送SIGPROF信号给进程。

定时器中的参数value用来指明定时器的时间，其结构如下：

struct itimerval {

        struct timeval it_interval;  //第一次之后每隔多长时间

        struct timeval it_value;  //第一次调用要多长时间 

};

该结构中timeval结构定义如下：

struct timeval {

        long tv_sec; //秒

        long tv_usec; //微秒，1秒 = 1000000 微秒

};

*/

int32 newTimer(int32 sig, int32 delayTime, sigFunction function)
{
  int32 ret = 0;
  
  struct itimerval it;
  
  it.it_value.tv_sec = delayTime;
  it.it_value.tv_usec = 0;
  
  it.it_interval.tv_sec = delayTime;
  it.it_interval.tv_usec = 0;

  signal(SIGALRM, function);
  ret = setitimer(ITIMER_REAL, &it, NULL);
  if (ret < 0)
  {
    LOG_ERROR("new timer error\n");
    return TASK_ERROR;
  }

  return TASK_OK;
}


