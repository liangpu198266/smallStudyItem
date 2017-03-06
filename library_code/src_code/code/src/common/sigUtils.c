/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */
#include "sigUtils.h"
#include "logprint.h"

/*
�����ź����ͼ�Ĭ����Ϊ

�ź�����	������Ч��	�Խ���Ĭ�ϵ�Ч��
SIGINT	Ctrl-C�ն��²���	��ֹ��ǰ����
SIGABRT	����SIGABRT�ź�	Ĭ����ֹ���̣�������core(����ת��)�ļ�
SIGALRM	�ɶ�ʱ����alarm(),setitimer()�Ȳ����Ķ�ʱ����ʱ�������ź�	��ֹ��ǰ����
SIGCHLD	�ӽ��̽������򸸽��̷���	����
SIGBUS	���ߴ��󣬼�������ĳ���ڴ���ʴ���	��ֹ��ǰ���̲���������ת���ļ�
SIGKILL	��ɱ�źţ��յ��źŵĽ���һ�����������ܲ���	��ֹ����
SIGPIPE	�ܵ����ѣ����ѹرյĹܵ�д����	������ֹ
SIGIO	ʹ��fcntlע��I/O�¼�,���ܵ�����socket����I/Oʱ�������ź�	��ֹ��ǰ����
SIGQUIT	���ն���Ctrl-\����	��ֹ��ǰ���̣�������core�ļ�
SIGSEGV	���ڴ���Ч�ķ��ʵ��¼������ġ��δ���	��ֹ��ǰ���̣�������core�ļ�
SIGSTOP	��ͣ�źţ����ܱ����������ܱ���׽	ֹͣ��ǰ����
SIGTERM	��ֹ���̵ı�׼�ź�	��ֹ��ǰ����
SIGUSR1���������û�ʹ�õ��źţ�
SIGUSR2��ͬSIGUSR1���������û�ʹ�õ��źš�

//ע���źŴ�����
linux��Ҫ����������ʵ���źŵİ�װ��signal()��sigaction()������signal()ֻ��������������֧���źŴ�����Ϣ��
��Ҫ������ǰ32�ַ�ʵʱ�źŵİ�װ����sigaction()�ǽ��µĺ�����������ϵͳ����ʵ�֣�sys_signal�Լ�sys_rt_sigaction����
������������֧���źŴ�����Ϣ����Ҫ������ sigqueue() ϵͳ�������ʹ�ã���Ȼ��sigaction()ͬ��֧�ַ�ʵʱ�źŵİ�װ��
sigaction()����signal()��Ҫ������֧���źŴ��в�����

int sigaction ( int sig , const struct sigaction * restrict act ,\
                struct sigaction * restrict oact ) ;

// �����źż�Ϊȫ��
int sigemptyset (sigset_t * set ) ;
// �����źż�Ϊȫ��
int sigfillset ( sigset_t * set ) ;
// ���ź�signum��ӵ��źż���
int sigaddset ( sigset_t * set , int signum) ;
// ȥ���źż���signum�ź�
int sigdelset ( sigset_t * set , int signum ) ;
// �ж�signum�ź��Ƿ����źż�set��
int sigismember ( const sigset_t * set , int signum ) ;

flags��ص�ѡ����⣺

SA_SIGINFO: ���ô˱�־λ��ʾ����ʹ��sa_sigaction�ֶν���ע���źŴ�����
SA_RESTART: ���жϵ�ϵͳ�����Զ�����
SA_NODEFER: ��ִ���źŴ������ڼ䲻���������źŴ�����ִ�е��ź�

*/

/*
��Գ���crash������ص���������ջ
SIGABRT 6 C ��abort(3)�������˳�ָ�� 
SIGSEGV 11 C ��Ч���ڴ����� 
SIGBUS 10,7,10 C ���ߴ���(������ڴ����) 
SIGILL 4 C �Ƿ�ָ�� 
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
block���������������μ�����һ��������Ҫ���ε��źţ��ڶ�ӦҪ���ε��ź�λ��1
pending����δ���źż��������ĳ���ź��ڽ��̵��������У���Ҳ��δ�����ж�Ӧλ��1����ʾ���źŲ��ܱ��ݴ���ᱻ����
handler���źŴ�������������ʾÿ���ź�����Ӧ���źŴ����������źŲ���δ������ʱ����������
 
���������ź�������δ����صĺ���������
#include <signal.h>

int  sigprocmask(int  how,  const  sigset_t *set, sigset_t *oldset))��

int sigpending(sigset_t *set));

int sigsuspend(const sigset_t *mask))��

sigprocmask()�����ܹ����ݲ���how��ʵ�ֶ��źż��Ĳ�����������Ҫ�����֣�

  SIG_BLOCK �ڽ��̵�ǰ�����źż������setָ���źż��е��źţ��൱�ڣ�mask=mask|set
  SIG_UNBLOCK ������������źż��а���setָ���źż��е��źţ������Ը��źŵ��������൱�ڣ�mask=mask|~set
  SIG_SETMASK ���½��������źż�Ϊsetָ����źż����൱��mask=set
  
sigpending(sigset_t *set))��õ�ǰ�ѵ��͵����̣�ȴ�������������źţ���setָ����źż��з��ؽ����

sigsuspend(const sigset_t *mask))�����ڽ��յ�ĳ���ź�֮ǰ, ��ʱ��mask�滻���̵��ź�����, ����ͣ����ִ�У�ֱ���յ��ź�Ϊֹ��

sigsuspend ���غ󽫻ָ�����֮ǰ���ź����롣�źŴ�������ɺ󣬽��̽�����ִ�С���ϵͳ����ʼ�շ���-1������errno����ΪEINTR

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
��ʱ��

���ڵ�ϵͳ�кܶ������ʹ��alarm���ã�����ʹ��setitimer���������ö�ʱ������getitimer���õ���ʱ����״̬��

���������õ�������ʽ���£�

#include <sys/time.h>

int getitimer(int which, struct itimerval *curr_value); 
int setitimer(int which, const struct itimerval *new_value,struct itimerval *old_value);

����:

��һ������whichָ����ʱ������
�ڶ��������ǽṹitimerval��һ��ʵ�����ṹitimerval��ʽ
�����������ɲ�������
����ֵ:�ɹ�����0ʧ�ܷ���-1

��ϵͳ���ø������ṩ��������ʱ�������Ǹ���������еļ�ʱ�򣬵������κ�һ������ͷ���һ����Ӧ���źŸ����̣���ʹ�ü�ʱ�����¿�ʼ��������ʱ���ɲ���whichָ����������ʾ��

TIMER_REAL����ʵ��ʱ���ʱ����ʱ���ｫ�����̷���SIGALRM�źš�

ITIMER_VIRTUAL����������ִ��ʱ�Ž��м�ʱ����ʱ���ｫ����SIGVTALRM�źŸ����̡�

ITIMER_PROF��������ִ��ʱ��ϵͳΪ�ý���ִ�ж���ʱ����ʱ����ITIMER_VIR-TUAL��һ�ԣ��ö�ʱ����������ͳ�ƽ������û�̬���ں�̬���ѵ�ʱ�䡣��ʱ���ｫ����SIGPROF�źŸ����̡�

��ʱ���еĲ���value����ָ����ʱ����ʱ�䣬��ṹ���£�

struct itimerval {

        struct timeval it_interval;  //��һ��֮��ÿ���೤ʱ��

        struct timeval it_value;  //��һ�ε���Ҫ�೤ʱ�� 

};

�ýṹ��timeval�ṹ�������£�

struct timeval {

        long tv_sec; //��

        long tv_usec; //΢�룬1�� = 1000000 ΢��

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


