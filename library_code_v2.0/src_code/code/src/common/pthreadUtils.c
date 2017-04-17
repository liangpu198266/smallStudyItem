/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */
#include "pthreadUtils.h"
#include "logprint.h"

/*
========================================================================
线程的创建

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine) (void *), void *arg);

args:
    pthread_t *thread              : 指向新线程的线程号的指针，如果创建成功则将线程号写回thread指向的内存空间 
    const pthread_attr_t *attr     : 指向新线程属性的指针（调度策略，继承性，分离性…） 
    void *(*start_routine) (void *): 新线程要执行函数的地址（回调函数）
    void *arg                      : 指向新线程要执行函数的参数的指针

return: 
    线程创建的状态，0是成功，非0失败
    
每一个成功创建的线程都有一个线程号，我们可以通过 pthread_self() 这个函数获取调用这个函数的线程的线程号。
有一点我们需要注意，如果需要编译 pthread_XX() 相关的函数，我们需要在编译时加入 -pthread。
*/

/*
线程生命周期

1、当c程序运行时，首先运行main函数。在线程代码中，这个特殊的执行流被称作初始线程或者主线程。你可以在初始线程中做任何普通线程可以做的事情。

2、主线程的特殊性在于，它在main函数返回的时候，会导致进程结束，进程内所有的线程也将会结束。这可不是一个好的现象，
你可以在主线程中调用pthread_exit函数，这样进程就会等待所有线程结束时才终止。

3、主线程接受参数的方式是通过argc和argv，而普通的线程只有一个参数void*

4、在绝大多数情况下，主线程在默认堆栈上运行，这个堆栈可以增长到足够的长度。而普通线程的堆栈是受限制的，一旦溢出就会产生错误

++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
线程从创建、运行到结束总是处于下面五个状态之一：新建状态、就绪状态、运行状态、阻塞状态及终止状态

就绪状态：

当线程刚被创建时就处于就绪状态，或者当线程被解除阻塞以后也会处于就绪状态。就绪的线程在等待一个可用的处理器，
当一个运行的线程被抢占时，它立刻又回到就绪状态
运行：

当处理器选中一个就绪的线程执行时，它立刻变成运行状态
阻塞:

线程会在以下情况下发生阻塞：试图加锁一个已经被锁住的互斥量，等待某个条件变量，调用singwait等待尚未发生的信号，执行无法完成的I/O信号，由于内存页错误
终止：

线程通常启动函数中返回来终止自己，或者调用pthread_exit退出，或者取消线程

++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

线程的基本控制
线程的结束

要结束一个线程，我们通常有以下几种方法：
  方法 	说明
  exit() 	终止整个进程，并释放整个进程的内存空间
  pthread_exit() 	终止线程，但不释放非分离线程的内存空间
  pthread_cancel() 	其他线程通过信号取消当前线程

exit() 会终止整个进程，因此我们几乎不会使用它来结束线程。我们常用 pthread_cancel() 和 pthread_exit() 函数来结束线程。
这里，我们重点看看 pthread_exit() 这个函数:

void pthread_exit(void *retval);

args: 
    void *retval: 指向线程结束码的指针

return:
    无

  要结束一个线程，只需要在线程调用的函数中加入 pthread_exit(X) 即可，但有一点需要特别注意：
  如果一个线程是非分离的（默认情况下创建的线程都是非分离）并且没有对该线程使用 pthread_join() 的话，
  该线程结束后并不会释放其内存空间。这会导致该线程变成了“僵尸线程”。“僵尸线程”会占用大量的系统资源，因此我们要避免“僵尸线程”的出现。
线程的连接

  在上一部分我们提到了 pthread_join() 这个函数，在这一节，我们先来看看这个函数：

int pthread_join(pthread_t thread, void **retval);

args:
    pthread_t thread: 被连接线程的线程号
    void **retval   : 指向一个指向被连接线程的返回码的指针的指针

return:
    线程连接的状态，0是成功，非0是失败

  当调用 pthread_join() 时，当前线程会处于阻塞状态，直到被调用的线程结束后，当前线程才会重新开始执行。
  当 pthread_join() 函数返回后，被调用线程才算真正意义上的结束，它的内存空间也会被释放（如果被调用线程是非分离的）。这里有三点需要注意：

    被释放的内存空间仅仅是系统空间，你必须手动清除程序分配的空间，比如 malloc() 分配的空间。
    一个线程只能被一个线程所连接。
    被连接的线程必须是非分离的，否则连接会出错。

++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

线程取消

取消函数原型

int pthread_cancle（pthread_ttid）

取消tid指定的线程，成功返回0。取消只是发送一个请求，并不意味着等待线程终止，而且发送成功也不意味着tid一定会终止

用于取消一个线程，它通常需要被取消线程的配合。线程在很多时候会查看自己是否有取消请求。

如果有就主动退出，这些查看是否有取消的地方称为取消点。

取消状态，就是线程对取消信号的处理方式，忽略或者响应。线程创建时默认响应取消信号

 设置取消状态函数原型

int pthread_setcancelstate(int state, int*oldstate) 

设置本线程对Cancel信号的反应，state有两种值：PTHREAD_CANCEL_ENABLE（缺省）和PTHREAD_CANCEL_DISABLE，

 

分别表示收到信号后设为CANCLED状态和忽略CANCEL信号继续运行；old_state如果不为NULL则存入原来的Cancel状态以便恢复。

取消类型，是线程对取消信号的响应方式，立即取消或者延时取消。线程创建时默认延时取消

设置取消类型函数原型

int pthread_setcanceltype(int type, int*oldtype)

设置本线程取消动作的执行时机，type由两种取值：PTHREAD_CANCEL_DEFFERED和PTHREAD_CANCEL_ASYCHRONOUS，
仅当Cancel状态为Enable时有效，分别表示收到信号后继续运行至下一个取消点再退出和立即执行取消动作（退出）；
oldtype如果不为NULL则存入运来的取消动作类型值。

 

取消一个线程，它通常需要被取消线程的配合。线程在很多时候会查看自己是否有取消请求

如果有就主动退出，这些查看是否有取消的地方称为取消点


很多地方都是包含取消点，包括

pthread_join()、pthread_testcancel()、pthread_cond_wait()、 pthread_cond_timedwait()、sem_wait()、sigwait()、write、read,大多数会阻塞的系统调用
    
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
线程的分离

  在上一节中，我们在谈 pthread_join() 时说到了只有非分离的线程才能使用 pthread_join()，这节我们就来看看什么是线程的分离。
  在Linux中，一个线程要么是可连接的，要么是可分离的。当我们创建一个线程的时候，线程默认是可连接的。可连接和可分离的线程有以下的区别：
  线程类型 	说明
  可连接的线程 	能够被其他线程回收或杀死，在其被杀死前，内存空间不会自动被释放
  可分离的线程 	不能被其他线程回收或杀死，其内存空间在它终止时由系统自动释放

  我们可以看到，对于可连接的线程而而言，它不会自动释放其内存空间。因此对于这类线程，我们必须要配合使用 pthread_join() 函数。
  而对于可分离的函数，我们就不能使用 pthread_join() 函数。
要使线程分离，我们有两种方法：1）通过修改线程属性让其成为可分离线程；2）通过调用函数 pthread_detach() 使新的线程成为可分离线程。
下面是 pthread_detach() 函数的说明：

int pthread_detach(pthread_t thread);

args:
    pthread_t thread: 需要分离线程的线程号

return:
    线程分离的状态，0是成功，非0是失败

我们只需要提供需要分离线程的线程号，便可以使其由可连接的线程变为可分离的线程。


分离一个正在运行的线程并不影响它，仅仅是通知当前系统该线程结束时，其所属的资源可以回收。一个没有被分离的线程在终止时会保留它的虚拟内存，
包括他们的堆栈和其他系统资源，有时这种线程被称为“僵尸线程”。创建线程时默认是非分离的

如果线程具有分离属性，线程终止时会被立刻回收，回收将释放掉所有在线程终止时未释放的系统资源和进程资源，包括保存线程返回值的内存空间、
堆栈、保存寄存器的内存空间等。

终止被分离的线程会释放所有的系统资源，但是你必须释放由该线程占有的程序资源。由malloc或者mmap分配的内存可以在任何时候由任何线程释放，
条件变量、互斥量、信号灯可以由任何线程销毁，只要他们被解锁了或者没有线程等待。但是只有互斥量的主人才能解锁它，所以在线程终止前，你需要解锁互斥量

分离线程 pthread_detach(3C)是pthread_join(3C)的替代函数，可回收创建时detachstate属性设置为PTHREAD_CREATE_JOINABLE的线程的存储空间。

  设置线程分离状态的函数为pthread_attr_setdetachstate（pthread_attr_t *attr, int detachstate）。
  第二个参数可选为PTHREAD_CREATE_DETACHED（分离线程）和 PTHREAD _CREATE_JOINABLE（非分离线程）。这里要注意的一点是，
  如果设置一个线程为分离线程，而这个线程运行又非常快，它很可能在pthread_create函数返回之前就终止了，
  它终止以后就可能将线程号和系统资源移交给其他的线程使用，这样调用pthread_create的线程就得到了错误的线程号。
  要避免这种情况可以采取一定的同步措施，最简单的方法之一是可以在被创建的线程里调用pthread_cond_timewait函数，让这个线程等待一会儿，
  留出足够的时间让函数pthread_create返回。设置一段等待时间，是在多线程编程里常用的方法。但是注意不要使用诸如wait（）之类的函数，
  它们是使整个进程睡眠，并不能解决线程同步的问题。
另外一个可能常用的属性是线程的优先级，它存放在结构sched_param中。用函数pthread_attr_getschedparam和函数pthread_attr_setschedparam进行存放，
一般说来，我们总是先取优先级，对取得的值修改后再存放回去。

  在任意一个时间点上，线程是可结合（joinable）或者是可分离的（detached）。一个可结合线程是可以被其他线程收回资源和杀死的。
  在被回收之前，他的存储器资源（栈等）是不释放的。而对于detached状态的线程，其资源不能被别的线程收回和杀死，只有等到线程结束才能由系统自动释放

默认情况，线程状态被设置为结合的。所以为了避免资源泄漏等问题，一个线程应当是被显示的join或者detach的，否则线程的状态类似于进程中的Zombie Process。
会有部分资源没有被回收的。调用函数pthread_join，当等待线程没有终止时，主线程将处于阻塞状态。如果要避免阻塞，那么

在主线程中加入代码pthread_detach(thread_id)或者在被等待线程中加入pthread_detach(thread_self())

  注：对于线程进行join之后线程的状态将是detach状态（分离），同样的pthread_cancel函数可以对线程进行分离处理。所以，
  不能同时对一个线程进行join和detach操作

pthread_detach语法

int pthread_detach(thread_t tid);


#include <pthread.h>
pthread_t tid;
int ret;
//detach thread tid 
ret = pthread_detach(tid);

pthread_detach()函数用于指示应用程序在线程tid终止时回收其存储空间。如果tid尚未终

止，pthread_detach()不会终止该线程。

*/

/*
主线程

在上面的几节中，我们讲了线程的创建，结束和连接。这节我们来看一个特殊的线程 - 主线程。
在C程序中，main(int argc, char **argv) 就是一个主线程。我们可以在主线程中做任何普通线程可以做的事情，但它和一般的线程有有一个很大的区别：
主线程返回或者运行结束时会导致进程的结束，而进程的结束会导致进程中所有线程的结束。为了不让主线程结束所有的线程，根据我们之前所学的知识，
有这么几个解决办法：

    不让主线程返回或者结束（在 return 前加入 while(1) 语句）。
    调用 pthread_exit() 来结束主线程，当主线程 return 后，其内存空间会被释放。
    在 return 前调用 pthread_join()，这时主线程将被阻塞，直到被连接的线程执行结束后，才接着运行。

在这三种方法中，前两种很少被使用，第三种是常用的方法。
*/

/*
线程的同步
-------------------------------
互斥量

  简单来说，互斥量就是一把锁住共享内存空间的锁，有了它，同一时刻只有一个线程可以访问该内存空间。当一个线程锁住内存空间的互斥量后，
  其他线程就不能访问这个内存空间，直到锁住该互斥量的线程解开这个锁。
互斥量的初始化

对于一个互斥量，我们首先需要对它进行初始化，然后才能将其锁住和解锁。我们可以使用动态分配和静态分配两种方式初始化互斥量。
  分配方式 	说明
  动态分配 	调用pthread_mutex_init()函数，在释放互斥量内存空间前要调用pthread_mutex_destroy()函数
  静态分配 	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER

下面是 pthread_mutex_init() 和 pthread_mutex_lock() 函数的原型：

int pthread_mutex_init(pthread_mutex_t *restrict mutex,
                       const pthread_mutexattr_t *restrict attr);

args:
    pthread_mutex_t *restrict mutex         : 指向需要被初始化的互斥量的指针
    const pthread_mutexattr_t *restrict attr: 指向需要被初始化的互斥量的属性的指针

return:
    互斥量初始化的状态，0是成功，非0是失败



int pthread_mutex_destroy(pthread_mutex_t *mutex);

args:
    pthread_mutex_t *mutex: 指向需要被销毁的互斥量的指针

return:
    互斥量销毁的状态，0是成功，非0是失败

互斥量的操作

互斥量的基本操作有三种：
  互斥量操作方式 	说明
  pthread_mutex_lock() 	锁住互斥量，如果互斥量已经被锁住，那么会导致该线程阻塞。
  pthread_mutex_trylock() 	锁住互斥量，如果互斥量已经被锁住，不会导致线程阻塞。
  pthread_mutex_unlock() 	解锁互斥量，如果一个互斥量没有被锁住，那么解锁就会出错。

上面三个函数的原型：

int pthread_mutex_lock(pthread_mutex_t *mutex);

args:
    pthread_mutex_t *mutex: 指向需要被锁住的互斥量的指针

return:
    互斥量锁住的状态，0是成功，非0是失败



int pthread_mutex_trylock(pthread_mutex_t *mutex);

args:
    pthread_mutex_t *mutex: 指向需要被锁住的互斥量的指针
return:
    互斥量锁住的状态，0是成功，非0是失败



int pthread_mutex_unlock(pthread_mutex_t *mutex);

args:
    pthread_mutex_t *mutex: 指向需要被解锁的互斥量的指针

return:
    互斥量解锁的状态，0是成功，非0是失败

死锁

如果互斥量使用不当可能会造成死锁现象。死锁指的是两个或两个以上的线程在执行过程中，因争夺资源而造成的一种互相等待的现象。
比如线程1锁住了资源A，线程2锁住了资源B；我们再让线程1去锁住资源B，线程2去锁住资源A。因为资源A和B已经被线程1和2锁住了，
所以线程1和2都会被阻塞，他们会永远在等待对方资源的释放。

为了避免死锁的发生，我们应该注意以下几点；

    访问共享资源时需要加锁
    互斥量使用完之后需要销毁
    加锁之后一定要解锁
    互斥量加锁的范围要小
    互斥量的数量应该少

读写锁

读写锁和互斥量相似，不过具有更高的并行性。互斥量只有锁住和解锁两种状态，而读写锁可以设置读加锁，写加锁和不加锁三种状态。
对于写加锁状态而言，任何时刻只能有一个线程占有写加锁状态的读写锁；而对于读加锁状态而言，任何时刻可以有多个线程拥有读加锁状态的读写锁。
下面是一些读写锁的特性：

特性 	说明
1 	当读写锁是写加锁状态时，在这个锁被解锁之前，所有试图对这个锁加锁的线程都会被阻塞。
2 	当读写锁是读加锁状态时，所有处于读加锁状态的线程都可以对其进行加锁。
3 	当读写锁是读加锁状态时，所有处于写加锁状态的线程都必须阻塞直到所有的线程释放该锁。
4 	当读写锁是读加锁状态时，如果有线程试图以写模式对其加锁，那么读写锁会阻塞随后的读模式锁请求。
读写锁的初始化

同互斥量类似，我们需要先初始化读写锁，然后才能将其锁住和解锁。要初始化读写锁，我们使用 pthread_rwlock_init() 函数，
同互斥量类似，在释放读写锁内存空间前，我们需要调用 pthread_rwlock_destroy() 函数来销毁读写锁。

下面是 pthread_rwlock_init() 和 pthread_rwlock_destroy() 函数的原型：

int pthread_rwlock_init(pthread_rwlock_t *restrict rwlock,
                        const pthread_rwlockattr_t *restrict attr);

args:
    pthread_rwlock_t *restrict rwlock:          指向需要初始化的读写锁的指针
    const pthread_rwlockattr_t *restrict attr： 指向需要初始化的读写锁属性的指针 

return:
    读写锁初始化的状态，0是成功，非0是失败



int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);

args:
    pthread_rwlock_t *rwlock: 指向需要被销毁的读写锁的指针

return:
    读写锁销毁的状态，0是成功，非0是失败

读写锁的操作

同互斥量类似，读写锁的操作也分为阻塞和非阻塞，我们先来看看读写锁有哪些基本操作：

读写锁操作方式 	说明
int pthread_rwlock_rdlock() 	读写锁读加锁，会阻塞其他线程
int pthread_rwlock_tryrdlock() 	读写锁读加锁，不阻塞其他线程
int pthread_rwlock_wrlock() 	读写锁写加锁，会阻塞其他线程
int pthread_rwlock_trywrlock() 	读写锁写加锁，不阻塞其他线程
int pthread_rwlock_unlock() 	读写锁解锁

下面是这几个函数的原型：

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);

args:
    pthread_rwlock_t *rwlock: 指向需要加锁的读写锁的指针

return:
    读写锁加锁的状态，0是成功，非0是失败

int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);

args:
    pthread_rwlock_t *rwlock: 指向需要加锁的读写锁的指针

return:
    读写锁加锁的状态，0是成功，非0是失败



int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);

args:
    pthread_rwlock_t *rwlock: 指向需要加锁的读写锁的指针 

return:
    读写锁加锁的状态，0是成功，非0是失败



int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);

args:
    pthread_rwlock_t *rwlock: 指向需要加锁的读写锁的指针 

return:
    读写锁加锁的状态，0是成功，非0是失败



int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);

args:
    pthread_rwlock_t *rwlock: 指向需要解锁的读写锁的指针 

return:
    读写锁解锁的状态，0是成功，非0是失败
    
-----------------------
条件变量

条件变量是和互斥量一起使用的。如果一个线程被互斥量锁住，但这个线程却不能做任何事情时，我们应该释放互斥量，
让其他线程工作，在这种情况下，我们可以使用条件变量；如果某个线程需要等待系统处于某种状态才能运行，此时，我们也可以使用条件变量。
条件变量的初始化

同互斥量一样，条件变量可以使用动态分配和静态分配的方式进行初始化：
  分配方式 	说明
  动态分配 	调用pthread_cond_init()函数，在释放条件变量内存空间前需要调用pthread_cond_destroy()函数
  静态分配 	pthread_cond_t cond = PTHREAD_COND_INITIALIZER

下面是 pthread_cond_init() 和 pthread_cond_destroy() 函数的原型：

int pthread_cond_init(pthread_cond_t *restrict cond,
                      const pthread_condattr_t *restrict attr);

args:
    pthread_cond_t *restrict cond          : 指向需要初始化的条件变量的指针
    const pthread_condattr_t *restrict attr: 指向需要初始化的条件变量属性的指针

return:
    条件变量初始化的状态，0是成功，非0是失败



int pthread_cond_destroy(pthread_cond_t *cond);

args:
    pthread_cond_t *cond: 指向需要被销毁的条件变量的指针

return:
    条件变量销毁的状态，0是成功，非0是失败

条件变量的操作

条件变量的操作分为等待和唤醒，等待操作的函数有 pthread_cond_wait() 和 pthread_cond_timedwait()；
唤醒操作的函数有 pthread_cond_signal() 和 pthread_cond_broadcast()。

我们来看看 pthread_cond_wait() 是怎么使用的，下面是函数原型：

int pthread_cond_wait(pthread_cond_t *restrict cond,
                      pthread_mutex_t *restrict mutex);

args:
    pthread_cond_t *restrict cond  : 指向需要等待的条件变量的指针
    pthread_mutex_t *restrict mutex: 指向传入互斥量的指针

return:
    0是成功，非0是失败

当一个线程调用 pthread_cond_wait() 时，需要传入条件变量和互斥量，这个互斥量必须要是被锁住的。当传入这两个参数后，

    该线程将被放到等待条件的线程列表中
    互斥量被解锁

这两个操作都是原子操作。当这两个操作结束后，其他线程就可以工作了。当条件变量为真时，系统切换回这个线程，函数返回，互斥量重新被加锁。

当我们需要唤醒等待的线程时，我们需要调用线程的唤醒函数，下面是函数的原型：

int pthread_cond_signal(pthread_cond_t *cond);

args:
    pthread_cond_t *cond: 指向需要唤醒的条件变量的指针

return:
    0是成功，非0是失败



int pthread_cond_broadcast(pthread_cond_t *cond);

args:
    pthread_cond_t *cond: 指向需要唤醒的条件变量的指针 

return:
    0是成功，非0是失败

pthread_cond_signal() 和 pthread_cond_broadcast() 的区别在于前者用于唤醒一个等待条件的线程，而后者用于唤醒所有等待条件的线程。

*/

/*
线程属性
 POSIX 线程库定义了线程属性对象 pthread_attr_t ，它封装了线程的创建者可以访问和修改的线程属性。主要包括如下属性：

1. 作用域（scope）

2. 栈尺寸（stack size）

3. 栈地址（stack address）

4. 优先级（priority）

5. 分离的状态（detached state）

6. 调度策略和参数（scheduling policy and parameters）

//线程属性结构如下： 

typedef struct
                {
                       int                     detachstate;  //线程的分离状态
                       int                     schedpolicy;  //线程调度策略
                       struct sched_param      schedparam;   //线程的调度参数
                       int                     inheritsched; //线程的继承性
                       int                     scope;        //线程的作用域
                       size_t                  guardsize;    //线程栈末尾的警戒缓冲区大小
                       int                     stackaddr_set;
                       void *                  stackaddr;    //线程栈的位置
                       size_t                  stacksize;    //线程栈的大小
                }pthread_attr_t; 


然而我看到pthread.h中的定义是这样的，并没有看到具体的成员，只是分配了对应的存储空间

typedef union
{
  char __size[__SIZEOF_PTHREAD_ATTR_T];
  long int __align;
} pthread_attr_t;


原因，应该是Linux不希望用户在新建用户线程的时候可以直接访问线程属性的数据成员，因为可能用户在这里设置了未定义的数值导致线程奔溃，
用户只能通过调用Linux提供的结构体的初始化函数对其进行变量初始化。这样做的好处在下文中会说明。

通过设置属性，可以指定一种不同于缺省行为的行为。使用pthread_create()创建线程时或初始化同步变量时，可以指定属性对象。
一般情况下pthread_create属性参数缺省值通常就足够了。

属性对象是不透明的，并不能通过赋值直接进行修改。系统提供了用于初始化、配置和销毁每种对象类型。

初始化和配置属性后，属性便具有进程范围的作用域。使用属性时最好的方法即是在程序执行早期一次配置好所有必需的状态规范。
然后，根据需要引用相应的属性对象。

使用属性对象具有两个主要优点。

■ 使用属性对象可增加代码可移植性。

即使支持的属性可能会在实现之间有所变化，但您不需要修改用于创建线程实体的函数

调用。这些函数调用不需要进行修改，因为属性对象是隐藏在接口之后的。

如果目标系统支持的属性在当前系统中不存在，则必须显式提供才能管理新的属性。管

理这些属性是一项非常容易的移植任务，因为只需在明确定义的位置初始化属性对象一

次即可。

■ 应用程序中的状态规范已被简化。

例如，假设进程中可能存在多组线程。每组线程都提供单独的服务。每组线程都有各自

的状态要求。在应用程序执行初期的某一时间，可以针对每组线程初始化线程属性对象。以后所有线程的创建都会引用已经为这类线程初始化的属性对象。
初始化阶段是简单和局部的。将来就可以快速且可靠地进行任何修改。
1 初始化属性

pthread_attr_init()将对象属性初始化为其缺省值。存储空间是在执行期间由线程系统分配的。

函数原型

int pthread_attr_init(pthread_attr_t *tattr);


#include <pthread.h>
pthread_attr_t tattr;
int ret;
//initialize an attribute to the default value
ret = pthread_attr_init(&tattr);


2 销毁属性

函数原型

int pthread_attr_destroy(pthread_attr_t *tattr);



#include <pthread.h>
pthread_attr_t tattr;
int ret;
// destroy an attribute
ret = pthread_attr_destroy(&tattr);


3 设置分离状态

如果创建分离线程(PTHREAD_CREATE_DETACHED)，则该线程一退出，便可重用其线程ID和其他资源。如果调用线程不准备等待线程退出

函数原型

int pthread_attr_setdetachstate(pthread_attr_t *tattr,int detachstate);


#include <pthread.h>
pthread_attr_t tattr;
int ret;
// set the thread detach state 
ret = pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED);


如果使用PTHREAD_CREATE_JOINABLE创建非分离线程，则假设应用程序将等待线程完成。也就是说，程序将对线程执行pthread_join()。

非分离线程在终止后，必须要有一个线程用join来等待它。否则，不会释放该线程的资源以供新线程使用，而这通常会导致内存泄漏。
因此，如果不希望线程被等待，请将该线程作为分离线程来创建。

4 设置栈溢出保护区大小

pthread_attr_setguardsize()可以设置attr对象的guardsize。

函数原型

int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);

出于以下两个原因，为应用程序提供了guardsize属性：

■ 溢出保护可能会导致系统资源浪费。如果应用程序创建大量线程，并且已知这些线程永远不会溢出其栈，则可以关闭溢出保护区。
通过关闭溢出保护区，可以节省系统资源。

■ 线程在栈上分配大型数据结构时，可能需要较大的溢出保护区来检测栈溢出。

guardsize参数提供了对栈指针溢出的保护。如果创建线程的栈时使用了保护功能，则实现会在栈的溢出端分配额外内存。
此额外内存的作用与缓冲区一样，可以防止栈指针的栈溢出。如果应用程序溢出到此缓冲区中，这个错误可能会导致SIGSEGV信号被发送给该线程。
如果guardsize为零，则不会为使用attr创建的线程提供溢出保护区。如果guardsize大于零，则会为每个使用attr创建的线程提供大小至少为guardsize字节的溢出保护区。
缺省情况下，线程具有实现定义的非零溢出保护区。

允许合乎惯例的实现，将guardsize的值向上舍入为可配置的系统变量PAGESIZE的倍数。请参见sys/mman.h中的PAGESIZE。
如果实现将guardsize的值向上舍入为PAGESIZE的倍数，则以guardsize（先前调用pthread_attr_setguardsize()时指定的溢出保护区大小）
为单位存储对指定attr的pthread_attr_getguardsize()的调用。

 
5 设置竞争范围

请使用pthread_attr_setscope()建立线程的争用范围（PTHREAD_SCOPE_SYSTEM或PTHREAD_SCOPE_PROCESS）。 
使用PTHREAD_SCOPE_SYSTEM时，此线程将与系统中的所有线程进行竞争。使用PTHREAD_SCOPE_PROCESS时，此线程将与进程中的其他线程进行竞争。

函数原型：

int pthread_attr_setscope(pthread_attr_t *tattr,int scope);


#include <pthread.h>
pthread_attr_t tattr;
int ret;
//bound thread  
ret = pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM);
//unbound thread  
ret = pthread_attr_setscope(&tattr, PTHREAD_SCOPE_PROCESS);


6 设置线程并行级别

pthread_setconcurrency()通知系统其所需的并发级别。

函数原型：

int pthread_setconcurrency(int new_level);


7 设置调度策略

pthread_attr_setschedpolicy()设置调度策略。POSIX标准指定SCHED_FIFO（先入先出）、SCHED_RR（循环）或SCHED_OTHER（实现定义的方法）的调度策略属性

函数原型：

int pthread_attr_setschedpolicy(pthread_attr_t *tattr, int policy);


#include <pthread.h>
pthread_attr_t tattr;
int policy;
int ret;
// set the scheduling policy to SCHED_OTHER 
ret = pthread_attr_setschedpolicy(&tattr, SCHED_OTHER);


■ SCHED_FIFO

如果调用进程具有有效的用户ID0，则争用范围为系统(PTHREAD_SCOPE_SYSTEM)的先入

先出线程属于实时(RT)调度类。如果这些线程未被优先级更高的线程抢占，则会继续处

理该线程，直到该线程放弃或阻塞为止。对于具有进程争用范围

(PTHREAD_SCOPE_PROCESS))的线程或其调用进程没有有效用户ID 0的线程，请使用

SCHED_FIFO。SCHED_FIFO基于TS调度类。

■ SCHED_RR

如果调用进程具有有效的用户ID0，则争用范围为系统(PTHREAD_SCOPE_SYSTEM))的循环

线程属于实时(RT)调度类。如果这些线程未被优先级更高的线程抢占，并且这些线程没

有放弃或阻塞，则在系统确定的时间段内将一直执行这些线程。对于具有进程争用范围

(PTHREAD_SCOPE_PROCESS)的线程，请使用SCHED_RR（基于TS调度类）。此外，这些线

程的调用进程没有有效的用户ID0。

SCHED_FIFO和SCHED_RR在POSIX标准中是可选的，而且仅用于实时线程。

8 设置继承的调度策略

函数原型：

int pthread_attr_setinheritsched(pthread_attr_t *tattr, int inherit);


#include <pthread.h>
pthread_attr_t tattr;
int inherit;
int ret;
//use the current scheduling policy  
ret = pthread_attr_setinheritsched(&tattr, PTHREAD_EXPLICIT_SCHED);


inherit值如果是PTHREAD_INHERIT_SCHED表示新建的线程将继承创建者线程中定义的调度策略。将忽略在pthread_create()调用中定义的所有调度属性。

如果使用缺省值PTHREAD_EXPLICIT_SCHED，则将使用pthread_create()调用中的属性。

 
9 设置调度参数

函数原型：

int pthread_attr_setschedparam(pthread_attr_t *tattr, const struct sched_param *param);


#include <pthread.h>
pthread_attr_t tattr;
int newprio;
sched_param param;
newprio= 30;
//set the priority; others are unchanged  
param.sched_priority = newprio;
// set the new scheduling param  
ret = pthread_attr_setschedparam (&tattr,?m);


调度参数是在param结构中定义的。仅支持优先级参数。新创建的线程使用此优先级运行。

 

使用指定的优先级创建线程

创建线程之前，可以设置优先级属性。将使用在sched_param结构中指定的新优先级创建子

线程。此结构还包含其他调度信息。

创建子线程时建议执行以下操作：

■ 获取现有参数

■ 更改优先级

■ 创建子线程

■ 恢复原始优先级

 
10 设置栈大小

函数原型

int pthread_attr_setstacksize(pthread_attr_t *tattr,size_t size);


#include <pthread.h>
pthread_attr_t tattr;
size_t size;
int ret;
size = (PTHREAD_STACK_MIN + 0x4000);
//setting a new size  
ret = pthread_attr_setstacksize(&tattr,size);


stacksize属性定义系统分配的栈大小（以字节为单位）。size不应小于系统定义的最小栈大小。

size包含新线程使用的栈的字节数。如果size为零，则使用缺省大小。

PTHREAD_STACK_MIN是启动线程所需的栈空间量。此栈空间没有考虑执行应用程序代码所需的线程例程要求。
栈

通常，线程栈是从页边界开始的。任何指定的大小都被向上舍入到下一个页边界。不具备

访问权限的页将被附加到栈的溢出端。大多数栈溢出都会导致将SIGSEGV信号发送到违例线

程。将直接使用调用方分配的线程栈，而不进行修改。

指定栈时，还应使用PTHREAD_CREATE_JOINABLE创建线程。在该线程的pthread_join(3C)调

用返回之前，不会释放该栈。在该线程终止之前，不会释放该线程的栈。了解这类线程是

否已终止的唯一可靠方式是使用pthread_join(3C)。
为线程分配栈空间

一般情况下，不需要为线程分配栈空间。系统会为每个线程的栈分配1MB（对于32位系统）或2MB（对于64位系统）的虚拟内存，
而不保留任何交换空间。系统将使用mmap()的MAP_NORESERVE选项来进行分配。

系统创建的每个线程栈都具有红色区域。系统通过将页附加到栈的溢出端来创建红色区域，从而捕获栈溢出。此类页无效，
而且会导致内存（访问时）故障。红色区域将被附加到所有自动分配的栈，无论大小是由应用程序指定，还是使用缺省大小。

极少数情况下需要指定栈和/或栈大小。程序员很难了解是否指定了正确的大小。甚至符合ABI标准的程序也不能静态确定其栈大小。
栈大小取决于执行中特定运行时环境。
生成自己的栈

指定线程栈大小时，必须考虑被调用函数以及每个要调用的后续函数的分配需求。需要考虑的因素应包括调用序列需求、局部变量和信息结构。

有时，需要与缺省栈不同的栈。一般的情况是，线程需要的栈大小大于缺省栈大小。少数情况，缺省大小太大。
因为可能正在使用不足的虚拟内存创建大量线程，进而处理这些个缺省线程栈所需的大量兆字节的栈空间。

对栈的最大大小的限制通常较为明显，但对其最小大小的限制如何呢？必须存在足够的栈空间来处理推入栈的所有栈帧，及其局部变量等。

要获取对栈大小的绝对最小限制，请调用宏PTHREAD_STACK_MIN。PTHREAD_STACK_MIN宏将针对执行NULL过程的线程返回所需的栈空间量。
有用的线程所需的栈大小大于最小栈大小，因此缩小栈大小时应非常谨慎。

#include <pthread.h>
pthread_attr_t tattr;
pthread_t tid;
int ret;
size_t size = PTHREAD_STACK_MIN + 0x4000;
ret = pthread_attr_init(&tattr); //initialized with default attributes /

ret = pthread_attr_setstacksize(&tattr,size); // setting the size of the stack also

ret = pthread_create(&tid,&tattr,start_routine,arg); // only size specified in tattr   



</pre><pre>


11 设置栈地址和大小

pthread_attr_setstack()可以设置线程栈地址和大小。

函数原型：

int pthread_attr_setstack(pthread_attr_t *tattr,void *stackaddr,size_t stacksize);


#include <pthread.h>
pthread_attr_t tattr;
void *base;
size_t size;
int ret;
base= (void *) malloc(PTHREAD_STACK_MIN + 0x4000);
//setting a new address and size 
ret = pthread_attr_setstack(&tattr,base,PTHREAD_STACK_MIN + 0x4000);


stackaddr属性定义线程栈的基准（低位地址）。stacksize属性指定栈的大小。如果将stackaddr设置为非空值，而不是缺省的NULL，
则系统将在该地址初始化栈，假设大小为stacksize。

base包含新线程使用的栈的地址。如果base为NULL，则pthread_create(3C)将为大小至少为stacksize字节的新线程分配栈。

*/

#ifndef PTHREAD_STACK_MIN                  
#define PTHREAD_STACK_MIN                  16384 
#endif    

void *pthreadFunction(void *arg)
{

  pthread_exit(NULL);
}

int32 pthreadCreateDetached(pthread_t *pthreadId)
{
  static pthread_attr_t pthreadAttr;

  if (pthread_attr_init(&pthreadAttr))
  {
    LOG_ERROR("fail init attr pthread \n");
    return TASK_ERROR;
  }
  
  pthread_attr_setdetachstate(&pthreadAttr, PTHREAD_CREATE_DETACHED);
  pthread_attr_setinheritsched(&pthreadAttr, PTHREAD_INHERIT_SCHED);
  pthread_attr_setstacksize(&pthreadAttr, PTHREAD_STACK_MIN);

  if(pthread_create(pthreadId, &pthreadAttr, pthreadFunction, NULL))
  {
    LOG_ERROR("fail create new detached pthread \n");
    return TASK_ERROR;
  }

  LOG_DBG("create new detached pthread\n");

  /* Destry thread attribute */
  pthread_attr_destroy(&pthreadAttr);
    
  return TASK_OK;
}

