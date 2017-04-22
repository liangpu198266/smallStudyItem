/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

/* ------------------------ Include ------------------------- */
#include "logprint.h"
#include "pthreadPool.h"

/*
创建线程函数——pthread_create()

复制代码
#include <pthread.h>
int pthread_create( pthread_t* thread,  const pthread_attr_t* attr,  void* (*start_routine)(void* ), void* arg );

 描述：
　　pthread_create()函数创建一个新的线程，通过线程属性对象attr指定属性。
　　被创建的线程继承了父线程的信号掩码，且它的等待信号是空的。

参数：
　　thread：NULL，或者指向一个pthread_t对象的指针，指向此函数执行后存储的一个新线程的线程ID。

　　attr：
　　指向 pthread_attr_t结构体的指针，此结构体指定了线程的执行属性。
　　如果attr为NULL，则被设置为默认的属性，pthread_attr_init()可以设置attr属性。
　　注意：如果在创建线程后编辑attr属性，线程的属性不受影响。

start_routine：
　　 线程的执行函数，arg为函数的参数。
　　如果start_routine()有返回值，调用pthread_exit()，start_routine()的返回值作为线程的退出状态。
　　在main()中的线程被调用有所不同。当它从main()返回，调用exit()，用main()的返回值作为退出状态。
arg：
　　传给start_routine的参数。
复制代码
Linux线程在核内是以轻量级进程的形式存在的，拥有独立的进程表项，而所有的创建、同步、删除等操作都在核外pthread库中进行。
pthread库使用一个管理线程（__pthread_manager()，每个进程独立且唯一）来管理线程的创建和终止，为线程分配线程ID，发送线程相关的信号（比如Cancel），
而主线程（pthread_create()）的调用者则通过管道将请求信息传给管理线程。



取消线程pthread_cancel()

 

复制代码
#include <pthread.h>
int pthread_cancel( pthread_t thread );

描述：
　　pthread_cancel()函数请求目标线程被取消（终止）。
　　取消类型（type）和状态（state）决定了取消函数是否会生效。
　　当取消动作被执行，目标线程的取消清除操作会被调用。
　　当最后的取消清除操作返回，目标线程的线程的析构函数会被调用。
　　当最后的析构函数返回，目标线程会被终止 (terminated)。
　　线程的取消过程同调用线程会异步执行。

参数：
　　thread： 欲取消的线程ID，可以通过pthread_create()或者pthread_self()函数取得。
复制代码
 


pthread_testcancel()

#include <pthread.h>
void pthread_testcancel( void );

描述：函数在运行的线程中创建一个取消点，如果cancellation无效则此函数不起作用。
 

 

 pthread_setcancelstate()


复制代码
#include <pthread.h>
int pthread_setcancelstate( int state, int* oldstate );

描述：
        pthread_setcancelstate() f函数设置线程取消状态为state，并且返回前一个取消点状态oldstate。

取消点有如下状态值：
        PTHREAD_CANCEL_DISABLE：取消请求保持等待，默认值。
        PTHREAD_CANCEL_ENABLE：取消请求依据取消类型执行；参考pthread_setcanceltype()。

参数：
        state：        新取消状态。
        oldstate：   指向本函数所存储的原取消状态的指针。
复制代码
 

pthread_setcanceltype()

复制代码
#include <pthread.h>
int pthread_setcanceltype( int type, int* oldtype );

描述：
        pthread_setcanceltype()函数设置运行线程的取消类型为type，并且返回原取消类型与oldtype。

取消类型值：
　　PTHREAD_CANCEL_ASYNCHRONOUS：如果取消有效，新的或者是等待的取消请求会立即执行。
　　PTHREAD_CANCEL_DEFERRED：如果取消有效，在遇到下一个取消点之前，取消请求会保持等待，默认值。
注意：标注POSIX和C库的调用不是异步取消安全地。

参数：
        type：        新取消类型
        oldtype：    指向该函数所存储的原取消类型的指针。
复制代码
 

 

什么是取消点（cancelation point）？

资料中说，根据POSIX标准，pthread_join()、pthread_testcancel()、 pthread_cond_wait()、pthread_cond_timedwait()、sem_wait()、sigwait()等函数以及 
read()、write()等会引起阻塞的系统调用都是Cancelation-point。而其他pthread函数都不会引起 Cancelation动作。但是pthread_cancel的手册页声称，
由于LinuxThread库与C库结合得不好，因而目前C库函数都不是 Cancelation-point；但CANCEL信号会使线程从阻塞的系统调用中退出，并置EINTR错误码，
因此可以在需要作为 Cancelation-point的系统调用前后调用pthread_testcancel()，从而达到POSIX标准所要求的目标，即如下代码段：

pthread_testcancel();
retcode = read(fd, buffer, length);
pthread_testcancel();
我发现，对于C库函数来说，几乎可以使线程挂起的函数都会响应CANCEL信号，终止线程，包括sleep、delay等延时函数，下面的例子对此会进行详细分析。


总结：

  创建线程

  起初，主程序main()包含了一个唯一的默认线程。程序员必须明确创建所有其它线程；
  pthread_create创建一个新的线程并使其执行，这个过程可以在你的代码里的任何地方调用多次；
  一个进程可以创建的线程的最大数量是依赖于实现的（The maximum number of threads that may be created by a process is implementation dependent. ）。
  线程一旦被创建，他们都是同等的，并且也可以创建其它线程。它们之间没有层次体系和依赖关系。
  终止线程

  一个线程有几种终止的方法：
  线程从它的起始程序中返回；
  线程调用了pthread_exit()函数；
  线程被另一个线程调用pthread_cancel()函数所取消；
  整个进程由于调用了exec或exit而退出。
  pthread_exit经常被用来明确的退出一个线程。通常，pthread_exit()函数在线程完成工作并不在需要时才被调用；
  如果主程序main()在一个线程被创建之前,使用pthread_exit()函数退出结束，另一些线程继续执行。否则，他们在主程序main()结束的时候会自动终止；
  清除：pthread_exit()函数不关闭文件，任何在线程中被打开的文件，在线程终止后仍然保持打开；
  通常，你可以通过pthread_exit()函数来操作程序的执行到结束的过程，当然，除非你想要传递一个返回值。然而，在主程序main()中，在主程序结束时，
  它所产生的线程有个显而易见的问题。如果你不用pthread_exit()，当main()结束是，主进程以及所有的线程都会终止。如果在main()中调用了pthread_exit()，
主进程和它所有的线程将会继续工作，尽管main()中的所有代码已经被执行了。

----------------------------------------------------------------------------------------------------------------------------------------------

线程连接（joining）和分离（detaching）函数：

pthread_join(threadid,status)
pthread_detach(threadid,status)
pthread_attr_setdetachstate(attr,detachstate)
pthread_attr_getdetachstate(attr,detachstate)
 

pthread_join(threadid,status)——等待线程终止

语法：

　　#include <pthread.h>

　　int pthread_join(pthread_t thread, void ** value_ptr);

描述：

　　pthread_join()将挂起调用线程的执行直到目标线程终止，除非目标线程已经终止了。

　　在一次成功调用pthread_join()并有非NULL的参数value_ptr，传给pthread_exit()终止线程的这个值以value_ptr作为引用是可用的。

　　当pthread_join()成功返回，目标线程就会终止。

　　对同一目标线程多次同时调用pthread_join()的结果是不确定的。

　　如果调用pthread_join()的线程被取消，那么目标线程将不会被分离。

 

　　一个线程是否退出或者保持为连接状态不明确会与{PTHREAD_THREADS_MAX}相背。

　　（It is unspecified whether a thread that has exited but remains unjoined counts against {PTHREAD_THREADS_MAX}.）

返回值

　　如果执行成功，pthread_join()会返回0；否则，会返回错误代码以表明错误类型。

原理

　　pthread_join()函数在多线程应用程序中的便捷性是令人满意的。如果没有其他状态作为参数传给start_routine()，程序员可以模拟这个函数。

 

pthread_detach——分离线程

语法：

　　#include <pthread.h>

　　int pthread_detach(pthread_t thread);

描述：

　　pthread_detach()函数在所指出线程终止时，该线程的内存空间可以被回收。

　　如果线程没有终止，pthread_detach()函数也不会令其终止。

　　对同一目标线程多次调用pthread_datach()的结果是不确定的。

返回值：

　　如果调用成功，pthread_detach()返回0；反之，会返回错误代码表明错误。

原理：

　　每一个被创建的线程，最终都需要调用pthread_join()函数或者pthread_detach()函数，因为与线程相关联的内存空间需要回收。

　　我们建议调用分离函数是不必要的；把线程创建时的属性置为分离状态就足够了，因为线程永远不需要动态分离。但是，需要的话仅限于如下两个原因：

在用pthread_join()进行取消操作时，用pthread_detach()函数分离pthread_join()正在等待的线程是很必要的。没有它，就必须用pthread_join()去试图分离另一个线程，这样，会无限期的延误取消处理，并且会引入新的pthread_join()调用，而它本身有可能需要取消处理。
为了脱离“初始线程”（在进程中设立服务线程是一种可取的办法）。
 

连接：

“连接”是线程间完成同步的一种方法，例如：



pthread_join()过程阻塞调用线程，直到被指定的threadid线程终止；

程序员能够获得目标线程终止的返回状态，如果目标线程调用pthread_exit()时指定；

一个连接线程跟一个pthread_join()调用对应。如果对同一个线程进行多次连接会发生逻辑错误；

还有两个同步方法：互斥锁和条件变量。

 

是否连接？

当一个线程被创建了，它有一个属性表明了它是否可以被连接和分离。只有线程是可连接的（joinable）才可以被连接。如果一个线程作为分离来创建的，
那么它永远不能被连接。

POSIX标准的最终方案线程被设计为可连接被创建；

为了明确创建线程的连接性和分离性，pthread_create()函数可使用attr参数。典型的4个步骤：

声明一个pthread_attr_t数据类型的pthread属性变量；
使用pthread_init()初始化属性变量；
使用pthread_attr_setdetachstate()设置属性为分离状态；
当使用完成后，使用pthread_attr_destroy()释放属性用到的资源。
 

分离

pthread_detach()函数能够明确的分离一个线程，即使被创建为可连接的。这个过程是不可逆的。

 

建议

如果一个线程需要连接，可以考虑创建时明确它是可连接的。这是因为可移动设备中并不是所有的实现，线程默认的是按照可连接创建的。
如果你预先知道一个线程永远不需要与另一个线程连接，可以考虑把它创建为分离状态。一些系统资源可能会因此而被释放。

----------------------------------------------------------------------------------------------------------------------------------------------

Mutex Variables（互斥量）

Mutex（互斥量）是“mutual exclusion”的缩写，互斥量最主要的用途是在多线程中对共享数据同进行写操作时同步线程并保护数据。
互斥量在保护共享数据资源时可以把它想象成一把锁，在Pthreads库中互斥量最基本的设计思想是在任一时间只有一个线程可以锁定（或拥有）互斥量，
因此，即使许多线程尝试去锁定一个互斥量时成功的只会有一个，只有在拥有互斥量的线程开锁后其它的线程才能锁定，就是说，线程必须排队访问被保护的数据。
互斥量常被用来防止竞争条件（race conditions），下面是一个涉及银行业务的竞争条件案例：
Thread 1	Thread 2	Balance
Read balance: $1000	 	$1000
 	Read balance: $1000	$1000
 	Deposit $200	$1000
Deposit $200	 	$1000
Update balance $1000+$200	 	$1200
 	Update balance $1000+$200	$1200
在上面的案例中，当线程使用共享数据资源Banlance时，互斥量会锁定“Balance”，如果不锁定，就会像表内情况一样，Balance的计算结果会混乱。
拥有互斥量的线程常常做的一个事情就是更改全局变量值，这是一个安全方法，确保在多个线程更改同一个变量时，它最终的值与唯一一个线程执行更改操作后
的结果是一致的，这个被更改的变量属于一个临界区（critical section）。

使用互斥量的一个标准的过程是：
  创建并初始化互斥量；
  多个线程试图锁定此互斥量；
  有且只有一个线程成功的拥有了这个互斥量；
  拥有互斥量的线程执行了一系列的操作；
  拥有互斥量的线程解锁互斥量；
  另一个线程获得此互斥量，并重复上面的过程；
  最终互斥量被销毁。
 

当许多线程竞争一个互斥量时，在请求时失败的线程会阻塞——无阻塞的请求是用“trylock”而不是用“lock”，所以pthread_mutex_trylock()与
pthread_mutex_lock()的区别就是在互斥量还没有解锁的情况下，前者不会阻塞线程。
在保护共享数据时，程序员的责任是确定每个线程需要用到互斥量，例如，如果有4个线程正在更改同一个数据，但是只有一个线程使用了互斥量，
那么这个数据仍然会损坏。
 

创建并销毁互斥量

 

函数：

pthread_mutex_init (mutex,attr)

pthread_mutex_destroy (mutex)

pthread_mutexattr_init (attr)

pthread_mutexattr_destroy (attr)

 

用法：

  互斥量必须要声明为pthread_mutex_t类型，并且在使用之前必须要被初始化。有两种方法初始化互斥量：
  静态初始化，例如：pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;
  动态初始化，使用pthread_mutex_init()过程，此函数可以设置互斥量属性对象attr；
  注意，互斥量初始状态是未被锁定的（unlocked）。

   

  对象attr常被用来设置互斥量对象的属性值的，而且必须是pthread_mutexattr_t类型（默认值是NULL）。Pthreads标准定义了3个互斥量可设置选项：
  Protocol：为互斥量指定用来防止优先级倒置的规则的；
  Prioceiling：为互斥量指定优先级上限的；
  Process-shared：为互斥量指定进程共享的。
  注意，不是所有的实现都要提供这三个可选的互斥量属性的。

 

pthread_mutexattr_init()和pthread_mutexattr_destroy()函数分别是用来创建和销毁互斥量属性的。
pthread_mutex_destroy()在互斥量不再被需要时，用来释放互斥量对象。
 

锁定和解锁互斥量

 

函数：

pthread_mutex_lock (mutex)
pthread_mutex_trylock (mutex)

pthread_mutex_unlock (mutex)

 

用法：

  线程用pthread_mutex_lock()函数通过指定的互斥量获得一个锁，如果互斥量已经被另一个线程锁定，那么这个请求会阻塞申请的线程，直到互斥量解锁。
  pthread_mutex_trylock()尝试锁定互斥量，然而，如果互斥量已经被锁定，那么此函数立即返回一个‘busy’的错误码，这个函数在优先级反转的情况下用来防止死锁是很有用的。
  如果拥有锁的线程调用pthread_mutex_unlock()函数，那么将会解锁该互斥量；如果其它线程将要为处理被保护的数据请求互斥量，
  那么拥有该互斥量的线程在完成数据操作后调用该函数。发生下列情况，会有错误码返回：
  如果互斥量已经被解锁；
  如果互斥量被另一个线程拥有。
  关于互斥量已经再也没有任何神奇的功用了……事实上，它们很类似于所有参与的线程之间的一个“君子协定”。

----------------------------------------------------------------------------------------------------------------------------------------------

条件变量（Condition Variables）

条件变量是什么？

条件变量为我们提供了另一种线程间同步的方法，然而，互斥量是通过控制线程访问数据来实现同步，条件变量允许线程同步是基于实际数据的值。
如果没有条件变量，程序员需要让线程不断地轮询，以检查是否满足条件。由于线程处在一个不间断的忙碌状态，所以这是相当耗资源的。
条件变量就是这么一个不需要轮询就可以解决这个问题的方法。
条件变量总是跟互斥锁（mutex lock）一起使用。

下面是使用条件变量的比较典型的过程：
  主线程
  声明并初始化需要同步的全局数据或变量（例如”count“）
  声明并初始化一个条件变量对象
  声明并初始化一个与条件变量关联的互斥量
  创建线程A和B并开始运行
  线程A
  线程运转至某一个条件被触发（例如,”count“必须达到某个值）
  锁定相关联的互斥量并检查全局变量的值
  调用pthread_con_wait()阻塞线程等待线程B的信号。请注意，调用pthread_con_wait()以自动的原子方式(atomically)解锁相关联的互斥量，以便于可以被线程B使用。
  当收到信号时，唤醒线程。互斥量被以自动的原子方式被锁定。
  明确的解锁互斥量。
  继续
  Thread B
  线程运转
  锁定相关联的互斥量
  更改线程A正在等待的全局变量的值
  检查线程A等待的变量值，如果满足条件，发信号给线程A
  解锁互斥量
  继续
  主线程
  Join / Continue
 

 

创建和销毁条件变量

函数：

pthread_cond_init (condition,attr)
pthread_cond_destroy (condition)

pthread_condattr_init (attr)

pthread_condattr_destroy (attr)

用法：

条件变量必须声明为pthread_cond_t类型，并且在使用前必须要初始化。初始化，有两种方法：
静态初始化，像这样声明：pthread_con_t myconvar = PTHREAD_CON_INITIALIZER；
动态初始化，使用pthread_cond_init()函数。用创建条件变量的ID作为件参数传给线程，这种方法允许设置条件变量对象属性attr。
可设置的attr对象经常用来设置条件变量的属性，条件变量只有一种属性：process-thread，它的作用是允许条件变量被其它进程的线程看到。
如果使用属性对象，必须是pthread_condattr_t类型（也可以赋值为NULL，作为默认值）。
        注意，不是所有的实现都用得着process-shared属性。

pthread_condattr_init()和pthread_condattr_destroy()函数是用来创建和销毁条件变量属性对象的。
当不再需要某条件变量时，可用pthread_cond_destroy()销毁。
 

条件变量的等待和信号发送

函数：

pthread_cond_wait (condition,mutex)
pthread_cond_signal (condition)

pthread_cond_broadcast (condition)

使用：

pthread_cond_wait()阻塞调用线程，直到指定的条件变量收到信号。当互斥量被锁定时，应该调用这个函数，并且在等待时自动释放这个互斥量，
在接收到信号后线程被唤醒，线程的互斥量会被自动锁定,程序员在线程中应当在此函数后解锁互斥量。
pthread_cond_signal()函数常用来发信号给（或唤醒）正在等待条件变量的另一个线程，在互斥量被锁定后应该调用这个函数，
并且为了pthread_cond_wait()函数的完成必须要解锁互斥量。
如果多个线程处于阻塞等待状态，那么必须要使用pthreads_cond_broadcast()函数，而不是pthread_cond_signal()。
在调用pthread_cond_wait()函数之前调用pthread_cond_signal()函数是个逻辑上的错误，所以，在使用这些函数时，
正确的锁定和解锁与条件变量相关的互斥量是非常必要的，例如：
在调用pthread_cond_wait()之前锁定互斥量失败，可致使其无法阻塞；
在调用pthread_cond_signal()之后解锁互斥量失败，则致使与之对应的pthread_cond_wait()函数无法完成，并仍保持阻塞状态

两个地方需要注意一下：
pthread_cond_wait()有解锁和锁定互斥量的操作，它所进行的操作大体有三步：解锁—阻塞监听—锁定，所以在监听线程的循环体里面有两次“锁定-解锁”的操作；
主函数main最后的pthread_cond_signal()这句必不可少，因为监听线程运转没有延时，在count的值达到COUNT_LIMIT-1时，已经处于waiting状态。 

*/
/* ------------------------ Defines ------------------------- */

