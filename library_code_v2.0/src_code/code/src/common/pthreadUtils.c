/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */
#include "pthreadUtils.h"
#include "logprint.h"

/*
========================================================================
�̵߳Ĵ���

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine) (void *), void *arg);

args:
    pthread_t *thread              : ָ�����̵߳��̺߳ŵ�ָ�룬��������ɹ����̺߳�д��threadָ����ڴ�ռ� 
    const pthread_attr_t *attr     : ָ�����߳����Ե�ָ�루���Ȳ��ԣ��̳��ԣ������ԡ��� 
    void *(*start_routine) (void *): ���߳�Ҫִ�к����ĵ�ַ���ص�������
    void *arg                      : ָ�����߳�Ҫִ�к����Ĳ�����ָ��

return: 
    �̴߳�����״̬��0�ǳɹ�����0ʧ��
    
ÿһ���ɹ��������̶߳���һ���̺߳ţ����ǿ���ͨ�� pthread_self() ���������ȡ��������������̵߳��̺߳š�
��һ��������Ҫע�⣬�����Ҫ���� pthread_XX() ��صĺ�����������Ҫ�ڱ���ʱ���� -pthread��
*/

/*
�߳���������

1����c��������ʱ����������main���������̴߳����У���������ִ������������ʼ�̻߳������̡߳�������ڳ�ʼ�߳������κ���ͨ�߳̿����������顣

2�����̵߳����������ڣ�����main�������ص�ʱ�򣬻ᵼ�½��̽��������������е��߳�Ҳ�����������ɲ���һ���õ�����
����������߳��е���pthread_exit�������������̾ͻ�ȴ������߳̽���ʱ����ֹ��

3�����߳̽��ܲ����ķ�ʽ��ͨ��argc��argv������ͨ���߳�ֻ��һ������void*

4���ھ����������£����߳���Ĭ�϶�ջ�����У������ջ�����������㹻�ĳ��ȡ�����ͨ�̵߳Ķ�ջ�������Ƶģ�һ������ͻ��������

++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
�̴߳Ӵ��������е��������Ǵ����������״̬֮һ���½�״̬������״̬������״̬������״̬����ֹ״̬

����״̬��

���̸߳ձ�����ʱ�ʹ��ھ���״̬�����ߵ��̱߳���������Ժ�Ҳ�ᴦ�ھ���״̬���������߳��ڵȴ�һ�����õĴ�������
��һ�����е��̱߳���ռʱ���������ֻص�����״̬
���У�

��������ѡ��һ���������߳�ִ��ʱ�������̱������״̬
����:

�̻߳�����������·�����������ͼ����һ���Ѿ�����ס�Ļ��������ȴ�ĳ����������������singwait�ȴ���δ�������źţ�ִ���޷���ɵ�I/O�źţ������ڴ�ҳ����
��ֹ��

�߳�ͨ�����������з�������ֹ�Լ������ߵ���pthread_exit�˳�������ȡ���߳�

++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

�̵߳Ļ�������
�̵߳Ľ���

Ҫ����һ���̣߳�����ͨ�������¼��ַ�����
  ���� 	˵��
  exit() 	��ֹ�������̣����ͷ��������̵��ڴ�ռ�
  pthread_exit() 	��ֹ�̣߳������ͷŷǷ����̵߳��ڴ�ռ�
  pthread_cancel() 	�����߳�ͨ���ź�ȡ����ǰ�߳�

exit() ����ֹ�������̣�������Ǽ�������ʹ�����������̡߳����ǳ��� pthread_cancel() �� pthread_exit() �����������̡߳�
��������ص㿴�� pthread_exit() �������:

void pthread_exit(void *retval);

args: 
    void *retval: ָ���߳̽������ָ��

return:
    ��

  Ҫ����һ���̣߳�ֻ��Ҫ���̵߳��õĺ����м��� pthread_exit(X) ���ɣ�����һ����Ҫ�ر�ע�⣺
  ���һ���߳��ǷǷ���ģ�Ĭ������´������̶߳��ǷǷ��룩����û�жԸ��߳�ʹ�� pthread_join() �Ļ���
  ���߳̽����󲢲����ͷ����ڴ�ռ䡣��ᵼ�¸��̱߳���ˡ���ʬ�̡߳�������ʬ�̡߳���ռ�ô�����ϵͳ��Դ���������Ҫ���⡰��ʬ�̡߳��ĳ��֡�
�̵߳�����

  ����һ���������ᵽ�� pthread_join() �������������һ�ڣ����������������������

int pthread_join(pthread_t thread, void **retval);

args:
    pthread_t thread: �������̵߳��̺߳�
    void **retval   : ָ��һ��ָ�������̵߳ķ������ָ���ָ��

return:
    �߳����ӵ�״̬��0�ǳɹ�����0��ʧ��

  ������ pthread_join() ʱ����ǰ�̻߳ᴦ������״̬��ֱ�������õ��߳̽����󣬵�ǰ�̲߳Ż����¿�ʼִ�С�
  �� pthread_join() �������غ󣬱������̲߳������������ϵĽ����������ڴ�ռ�Ҳ�ᱻ�ͷţ�����������߳��ǷǷ���ģ���������������Ҫע�⣺

    ���ͷŵ��ڴ�ռ������ϵͳ�ռ䣬������ֶ�����������Ŀռ䣬���� malloc() ����Ŀռ䡣
    һ���߳�ֻ�ܱ�һ���߳������ӡ�
    �����ӵ��̱߳����ǷǷ���ģ��������ӻ����

++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

�߳�ȡ��

ȡ������ԭ��

int pthread_cancle��pthread_ttid��

ȡ��tidָ�����̣߳��ɹ�����0��ȡ��ֻ�Ƿ���һ�����󣬲�����ζ�ŵȴ��߳���ֹ�����ҷ��ͳɹ�Ҳ����ζ��tidһ������ֹ

����ȡ��һ���̣߳���ͨ����Ҫ��ȡ���̵߳���ϡ��߳��ںܶ�ʱ���鿴�Լ��Ƿ���ȡ������

����о������˳�����Щ�鿴�Ƿ���ȡ���ĵط���Ϊȡ���㡣

ȡ��״̬�������̶߳�ȡ���źŵĴ���ʽ�����Ի�����Ӧ���̴߳���ʱĬ����Ӧȡ���ź�

 ����ȡ��״̬����ԭ��

int pthread_setcancelstate(int state, int*oldstate) 

���ñ��̶߳�Cancel�źŵķ�Ӧ��state������ֵ��PTHREAD_CANCEL_ENABLE��ȱʡ����PTHREAD_CANCEL_DISABLE��

 

�ֱ��ʾ�յ��źź���ΪCANCLED״̬�ͺ���CANCEL�źż������У�old_state�����ΪNULL�����ԭ����Cancel״̬�Ա�ָ���

ȡ�����ͣ����̶߳�ȡ���źŵ���Ӧ��ʽ������ȡ��������ʱȡ�����̴߳���ʱĬ����ʱȡ��

����ȡ�����ͺ���ԭ��

int pthread_setcanceltype(int type, int*oldtype)

���ñ��߳�ȡ��������ִ��ʱ����type������ȡֵ��PTHREAD_CANCEL_DEFFERED��PTHREAD_CANCEL_ASYCHRONOUS��
����Cancel״̬ΪEnableʱ��Ч���ֱ��ʾ�յ��źź������������һ��ȡ�������˳�������ִ��ȡ���������˳�����
oldtype�����ΪNULL�����������ȡ����������ֵ��

 

ȡ��һ���̣߳���ͨ����Ҫ��ȡ���̵߳���ϡ��߳��ںܶ�ʱ���鿴�Լ��Ƿ���ȡ������

����о������˳�����Щ�鿴�Ƿ���ȡ���ĵط���Ϊȡ����


�ܶ�ط����ǰ���ȡ���㣬����

pthread_join()��pthread_testcancel()��pthread_cond_wait()�� pthread_cond_timedwait()��sem_wait()��sigwait()��write��read,�������������ϵͳ����
    
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
�̵߳ķ���

  ����һ���У�������̸ pthread_join() ʱ˵����ֻ�зǷ�����̲߳���ʹ�� pthread_join()��������Ǿ�������ʲô���̵߳ķ��롣
  ��Linux�У�һ���߳�Ҫô�ǿ����ӵģ�Ҫô�ǿɷ���ġ������Ǵ���һ���̵߳�ʱ���߳�Ĭ���ǿ����ӵġ������ӺͿɷ�����߳������µ�����
  �߳����� 	˵��
  �����ӵ��߳� 	�ܹ��������̻߳��ջ�ɱ�������䱻ɱ��ǰ���ڴ�ռ䲻���Զ����ͷ�
  �ɷ�����߳� 	���ܱ������̻߳��ջ�ɱ�������ڴ�ռ�������ֹʱ��ϵͳ�Զ��ͷ�

  ���ǿ��Կ��������ڿ����ӵ��̶߳����ԣ��������Զ��ͷ����ڴ�ռ䡣��˶��������̣߳����Ǳ���Ҫ���ʹ�� pthread_join() ������
  �����ڿɷ���ĺ��������ǾͲ���ʹ�� pthread_join() ������
Ҫʹ�̷߳��룬���������ַ�����1��ͨ���޸��߳����������Ϊ�ɷ����̣߳�2��ͨ�����ú��� pthread_detach() ʹ�µ��̳߳�Ϊ�ɷ����̡߳�
������ pthread_detach() ������˵����

int pthread_detach(pthread_t thread);

args:
    pthread_t thread: ��Ҫ�����̵߳��̺߳�

return:
    �̷߳����״̬��0�ǳɹ�����0��ʧ��

����ֻ��Ҫ�ṩ��Ҫ�����̵߳��̺߳ţ������ʹ���ɿ����ӵ��̱߳�Ϊ�ɷ�����̡߳�


����һ���������е��̲߳���Ӱ������������֪ͨ��ǰϵͳ���߳̽���ʱ������������Դ���Ի��ա�һ��û�б�������߳�����ֹʱ�ᱣ�����������ڴ棬
�������ǵĶ�ջ������ϵͳ��Դ����ʱ�����̱߳���Ϊ����ʬ�̡߳��������߳�ʱĬ���ǷǷ����

����߳̾��з������ԣ��߳���ֹʱ�ᱻ���̻��գ����ս��ͷŵ��������߳���ֹʱδ�ͷŵ�ϵͳ��Դ�ͽ�����Դ�����������̷߳���ֵ���ڴ�ռ䡢
��ջ������Ĵ������ڴ�ռ�ȡ�

��ֹ��������̻߳��ͷ����е�ϵͳ��Դ������������ͷ��ɸ��߳�ռ�еĳ�����Դ����malloc����mmap������ڴ�������κ�ʱ�����κ��߳��ͷţ�
�������������������źŵƿ������κ��߳����٣�ֻҪ���Ǳ������˻���û���̵߳ȴ�������ֻ�л����������˲��ܽ��������������߳���ֹǰ������Ҫ����������

�����߳� pthread_detach(3C)��pthread_join(3C)������������ɻ��մ���ʱdetachstate��������ΪPTHREAD_CREATE_JOINABLE���̵߳Ĵ洢�ռ䡣

  �����̷߳���״̬�ĺ���Ϊpthread_attr_setdetachstate��pthread_attr_t *attr, int detachstate����
  �ڶ���������ѡΪPTHREAD_CREATE_DETACHED�������̣߳��� PTHREAD _CREATE_JOINABLE���Ƿ����̣߳�������Ҫע���һ���ǣ�
  �������һ���߳�Ϊ�����̣߳�������߳������ַǳ��죬���ܿ�����pthread_create��������֮ǰ����ֹ�ˣ�
  ����ֹ�Ժ�Ϳ��ܽ��̺߳ź�ϵͳ��Դ�ƽ����������߳�ʹ�ã���������pthread_create���߳̾͵õ��˴�����̺߳š�
  Ҫ��������������Բ�ȡһ����ͬ����ʩ����򵥵ķ���֮һ�ǿ����ڱ��������߳������pthread_cond_timewait������������̵߳ȴ�һ�����
  �����㹻��ʱ���ú���pthread_create���ء�����һ�εȴ�ʱ�䣬���ڶ��̱߳���ﳣ�õķ���������ע�ⲻҪʹ������wait����֮��ĺ�����
  ������ʹ��������˯�ߣ������ܽ���߳�ͬ�������⡣
����һ�����ܳ��õ��������̵߳����ȼ���������ڽṹsched_param�С��ú���pthread_attr_getschedparam�ͺ���pthread_attr_setschedparam���д�ţ�
һ��˵��������������ȡ���ȼ�����ȡ�õ�ֵ�޸ĺ��ٴ�Ż�ȥ��

  ������һ��ʱ����ϣ��߳��ǿɽ�ϣ�joinable�������ǿɷ���ģ�detached����һ���ɽ���߳��ǿ��Ա������߳��ջ���Դ��ɱ���ġ�
  �ڱ�����֮ǰ�����Ĵ洢����Դ��ջ�ȣ��ǲ��ͷŵġ�������detached״̬���̣߳�����Դ���ܱ�����߳��ջغ�ɱ����ֻ�еȵ��߳̽���������ϵͳ�Զ��ͷ�

Ĭ��������߳�״̬������Ϊ��ϵġ�����Ϊ�˱�����Դй©�����⣬һ���߳�Ӧ���Ǳ���ʾ��join����detach�ģ������̵߳�״̬�����ڽ����е�Zombie Process��
���в�����Դû�б����յġ����ú���pthread_join�����ȴ��߳�û����ֹʱ�����߳̽���������״̬�����Ҫ������������ô

�����߳��м������pthread_detach(thread_id)�����ڱ��ȴ��߳��м���pthread_detach(thread_self())

  ע�������߳̽���join֮���̵߳�״̬����detach״̬�����룩��ͬ����pthread_cancel�������Զ��߳̽��з��봦�����ԣ�
  ����ͬʱ��һ���߳̽���join��detach����

pthread_detach�﷨

int pthread_detach(thread_t tid);


#include <pthread.h>
pthread_t tid;
int ret;
//detach thread tid 
ret = pthread_detach(tid);

pthread_detach()��������ָʾӦ�ó������߳�tid��ֹʱ������洢�ռ䡣���tid��δ��

ֹ��pthread_detach()������ֹ���̡߳�

*/

/*
���߳�

������ļ����У����ǽ����̵߳Ĵ��������������ӡ������������һ��������߳� - ���̡߳�
��C�����У�main(int argc, char **argv) ����һ�����̡߳����ǿ��������߳������κ���ͨ�߳̿����������飬������һ����߳�����һ���ܴ������
���̷߳��ػ������н���ʱ�ᵼ�½��̵Ľ����������̵Ľ����ᵼ�½����������̵߳Ľ�����Ϊ�˲������߳̽������е��̣߳���������֮ǰ��ѧ��֪ʶ��
����ô��������취��

    �������̷߳��ػ��߽������� return ǰ���� while(1) ��䣩��
    ���� pthread_exit() ���������̣߳������߳� return �����ڴ�ռ�ᱻ�ͷš�
    �� return ǰ���� pthread_join()����ʱ���߳̽���������ֱ�������ӵ��߳�ִ�н����󣬲Ž������С�

�������ַ����У�ǰ���ֺ��ٱ�ʹ�ã��������ǳ��õķ�����
*/

/*
�̵߳�ͬ��
-------------------------------
������

  ����˵������������һ����ס�����ڴ�ռ��������������ͬһʱ��ֻ��һ���߳̿��Է��ʸ��ڴ�ռ䡣��һ���߳���ס�ڴ�ռ�Ļ�������
  �����߳̾Ͳ��ܷ�������ڴ�ռ䣬ֱ����ס�û��������߳̽⿪�������
�������ĳ�ʼ��

����һ��������������������Ҫ�������г�ʼ����Ȼ����ܽ�����ס�ͽ��������ǿ���ʹ�ö�̬����;�̬�������ַ�ʽ��ʼ����������
  ���䷽ʽ 	˵��
  ��̬���� 	����pthread_mutex_init()���������ͷŻ������ڴ�ռ�ǰҪ����pthread_mutex_destroy()����
  ��̬���� 	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER

������ pthread_mutex_init() �� pthread_mutex_lock() ������ԭ�ͣ�

int pthread_mutex_init(pthread_mutex_t *restrict mutex,
                       const pthread_mutexattr_t *restrict attr);

args:
    pthread_mutex_t *restrict mutex         : ָ����Ҫ����ʼ���Ļ�������ָ��
    const pthread_mutexattr_t *restrict attr: ָ����Ҫ����ʼ���Ļ����������Ե�ָ��

return:
    ��������ʼ����״̬��0�ǳɹ�����0��ʧ��



int pthread_mutex_destroy(pthread_mutex_t *mutex);

args:
    pthread_mutex_t *mutex: ָ����Ҫ�����ٵĻ�������ָ��

return:
    ���������ٵ�״̬��0�ǳɹ�����0��ʧ��

�������Ĳ���

�������Ļ������������֣�
  ������������ʽ 	˵��
  pthread_mutex_lock() 	��ס������������������Ѿ�����ס����ô�ᵼ�¸��߳�������
  pthread_mutex_trylock() 	��ס������������������Ѿ�����ס�����ᵼ���߳�������
  pthread_mutex_unlock() 	���������������һ��������û�б���ס����ô�����ͻ����

��������������ԭ�ͣ�

int pthread_mutex_lock(pthread_mutex_t *mutex);

args:
    pthread_mutex_t *mutex: ָ����Ҫ����ס�Ļ�������ָ��

return:
    ��������ס��״̬��0�ǳɹ�����0��ʧ��



int pthread_mutex_trylock(pthread_mutex_t *mutex);

args:
    pthread_mutex_t *mutex: ָ����Ҫ����ס�Ļ�������ָ��
return:
    ��������ס��״̬��0�ǳɹ�����0��ʧ��



int pthread_mutex_unlock(pthread_mutex_t *mutex);

args:
    pthread_mutex_t *mutex: ָ����Ҫ�������Ļ�������ָ��

return:
    ������������״̬��0�ǳɹ�����0��ʧ��

����

���������ʹ�ò������ܻ����������������ָ�����������������ϵ��߳���ִ�й����У���������Դ����ɵ�һ�ֻ���ȴ�������
�����߳�1��ס����ԴA���߳�2��ס����ԴB�����������߳�1ȥ��ס��ԴB���߳�2ȥ��ס��ԴA����Ϊ��ԴA��B�Ѿ����߳�1��2��ס�ˣ�
�����߳�1��2���ᱻ���������ǻ���Զ�ڵȴ��Է���Դ���ͷš�

Ϊ�˱��������ķ���������Ӧ��ע�����¼��㣻

    ���ʹ�����Դʱ��Ҫ����
    ������ʹ����֮����Ҫ����
    ����֮��һ��Ҫ����
    �����������ķ�ΧҪС
    ������������Ӧ����

��д��

��д���ͻ��������ƣ��������и��ߵĲ����ԡ�������ֻ����ס�ͽ�������״̬������д���������ö�������д�����Ͳ���������״̬��
����д����״̬���ԣ��κ�ʱ��ֻ����һ���߳�ռ��д����״̬�Ķ�д���������ڶ�����״̬���ԣ��κ�ʱ�̿����ж���߳�ӵ�ж�����״̬�Ķ�д����
������һЩ��д�������ԣ�

���� 	˵��
1 	����д����д����״̬ʱ���������������֮ǰ��������ͼ��������������̶߳��ᱻ������
2 	����д���Ƕ�����״̬ʱ�����д��ڶ�����״̬���̶߳����Զ�����м�����
3 	����д���Ƕ�����״̬ʱ�����д���д����״̬���̶߳���������ֱ�����е��߳��ͷŸ�����
4 	����д���Ƕ�����״̬ʱ��������߳���ͼ��дģʽ�����������ô��д�����������Ķ�ģʽ������
��д���ĳ�ʼ��

ͬ���������ƣ�������Ҫ�ȳ�ʼ����д����Ȼ����ܽ�����ס�ͽ�����Ҫ��ʼ����д��������ʹ�� pthread_rwlock_init() ������
ͬ���������ƣ����ͷŶ�д���ڴ�ռ�ǰ��������Ҫ���� pthread_rwlock_destroy() ���������ٶ�д����

������ pthread_rwlock_init() �� pthread_rwlock_destroy() ������ԭ�ͣ�

int pthread_rwlock_init(pthread_rwlock_t *restrict rwlock,
                        const pthread_rwlockattr_t *restrict attr);

args:
    pthread_rwlock_t *restrict rwlock:          ָ����Ҫ��ʼ���Ķ�д����ָ��
    const pthread_rwlockattr_t *restrict attr�� ָ����Ҫ��ʼ���Ķ�д�����Ե�ָ�� 

return:
    ��д����ʼ����״̬��0�ǳɹ�����0��ʧ��



int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);

args:
    pthread_rwlock_t *rwlock: ָ����Ҫ�����ٵĶ�д����ָ��

return:
    ��д�����ٵ�״̬��0�ǳɹ�����0��ʧ��

��д���Ĳ���

ͬ���������ƣ���д���Ĳ���Ҳ��Ϊ�����ͷ���������������������д������Щ����������

��д��������ʽ 	˵��
int pthread_rwlock_rdlock() 	��д���������������������߳�
int pthread_rwlock_tryrdlock() 	��д���������������������߳�
int pthread_rwlock_wrlock() 	��д��д�����������������߳�
int pthread_rwlock_trywrlock() 	��д��д�����������������߳�
int pthread_rwlock_unlock() 	��д������

�������⼸��������ԭ�ͣ�

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);

args:
    pthread_rwlock_t *rwlock: ָ����Ҫ�����Ķ�д����ָ��

return:
    ��д��������״̬��0�ǳɹ�����0��ʧ��

int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);

args:
    pthread_rwlock_t *rwlock: ָ����Ҫ�����Ķ�д����ָ��

return:
    ��д��������״̬��0�ǳɹ�����0��ʧ��



int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);

args:
    pthread_rwlock_t *rwlock: ָ����Ҫ�����Ķ�д����ָ�� 

return:
    ��д��������״̬��0�ǳɹ�����0��ʧ��



int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);

args:
    pthread_rwlock_t *rwlock: ָ����Ҫ�����Ķ�д����ָ�� 

return:
    ��д��������״̬��0�ǳɹ�����0��ʧ��



int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);

args:
    pthread_rwlock_t *rwlock: ָ����Ҫ�����Ķ�д����ָ�� 

return:
    ��д��������״̬��0�ǳɹ�����0��ʧ��
    
-----------------------
��������

���������Ǻͻ�����һ��ʹ�õġ����һ���̱߳���������ס��������߳�ȴ�������κ�����ʱ������Ӧ���ͷŻ�������
�������̹߳���������������£����ǿ���ʹ���������������ĳ���߳���Ҫ�ȴ�ϵͳ����ĳ��״̬�������У���ʱ������Ҳ����ʹ������������
���������ĳ�ʼ��

ͬ������һ����������������ʹ�ö�̬����;�̬����ķ�ʽ���г�ʼ����
  ���䷽ʽ 	˵��
  ��̬���� 	����pthread_cond_init()���������ͷ����������ڴ�ռ�ǰ��Ҫ����pthread_cond_destroy()����
  ��̬���� 	pthread_cond_t cond = PTHREAD_COND_INITIALIZER

������ pthread_cond_init() �� pthread_cond_destroy() ������ԭ�ͣ�

int pthread_cond_init(pthread_cond_t *restrict cond,
                      const pthread_condattr_t *restrict attr);

args:
    pthread_cond_t *restrict cond          : ָ����Ҫ��ʼ��������������ָ��
    const pthread_condattr_t *restrict attr: ָ����Ҫ��ʼ���������������Ե�ָ��

return:
    ����������ʼ����״̬��0�ǳɹ�����0��ʧ��



int pthread_cond_destroy(pthread_cond_t *cond);

args:
    pthread_cond_t *cond: ָ����Ҫ�����ٵ�����������ָ��

return:
    �����������ٵ�״̬��0�ǳɹ�����0��ʧ��

���������Ĳ���

���������Ĳ�����Ϊ�ȴ��ͻ��ѣ��ȴ������ĺ����� pthread_cond_wait() �� pthread_cond_timedwait()��
���Ѳ����ĺ����� pthread_cond_signal() �� pthread_cond_broadcast()��

���������� pthread_cond_wait() ����ôʹ�õģ������Ǻ���ԭ�ͣ�

int pthread_cond_wait(pthread_cond_t *restrict cond,
                      pthread_mutex_t *restrict mutex);

args:
    pthread_cond_t *restrict cond  : ָ����Ҫ�ȴ�������������ָ��
    pthread_mutex_t *restrict mutex: ָ���뻥������ָ��

return:
    0�ǳɹ�����0��ʧ��

��һ���̵߳��� pthread_cond_wait() ʱ����Ҫ�������������ͻ��������������������Ҫ�Ǳ���ס�ġ�������������������

    ���߳̽����ŵ��ȴ��������߳��б���
    ������������

��������������ԭ�Ӳ����������������������������߳̾Ϳ��Թ����ˡ�����������Ϊ��ʱ��ϵͳ�л�������̣߳��������أ����������±�������

��������Ҫ���ѵȴ����߳�ʱ��������Ҫ�����̵߳Ļ��Ѻ����������Ǻ�����ԭ�ͣ�

int pthread_cond_signal(pthread_cond_t *cond);

args:
    pthread_cond_t *cond: ָ����Ҫ���ѵ�����������ָ��

return:
    0�ǳɹ�����0��ʧ��



int pthread_cond_broadcast(pthread_cond_t *cond);

args:
    pthread_cond_t *cond: ָ����Ҫ���ѵ�����������ָ�� 

return:
    0�ǳɹ�����0��ʧ��

pthread_cond_signal() �� pthread_cond_broadcast() ����������ǰ�����ڻ���һ���ȴ��������̣߳����������ڻ������еȴ��������̡߳�

*/

/*
�߳�����
 POSIX �߳̿ⶨ�����߳����Զ��� pthread_attr_t ������װ���̵߳Ĵ����߿��Է��ʺ��޸ĵ��߳����ԡ���Ҫ�����������ԣ�

1. ������scope��

2. ջ�ߴ磨stack size��

3. ջ��ַ��stack address��

4. ���ȼ���priority��

5. �����״̬��detached state��

6. ���Ȳ��ԺͲ�����scheduling policy and parameters��

//�߳����Խṹ���£� 

typedef struct
                {
                       int                     detachstate;  //�̵߳ķ���״̬
                       int                     schedpolicy;  //�̵߳��Ȳ���
                       struct sched_param      schedparam;   //�̵߳ĵ��Ȳ���
                       int                     inheritsched; //�̵߳ļ̳���
                       int                     scope;        //�̵߳�������
                       size_t                  guardsize;    //�߳�ջĩβ�ľ��仺������С
                       int                     stackaddr_set;
                       void *                  stackaddr;    //�߳�ջ��λ��
                       size_t                  stacksize;    //�߳�ջ�Ĵ�С
                }pthread_attr_t; 


Ȼ���ҿ���pthread.h�еĶ����������ģ���û�п�������ĳ�Ա��ֻ�Ƿ����˶�Ӧ�Ĵ洢�ռ�

typedef union
{
  char __size[__SIZEOF_PTHREAD_ATTR_T];
  long int __align;
} pthread_attr_t;


ԭ��Ӧ����Linux��ϣ���û����½��û��̵߳�ʱ�����ֱ�ӷ����߳����Ե����ݳ�Ա����Ϊ�����û�������������δ�������ֵ�����̱߳�����
�û�ֻ��ͨ������Linux�ṩ�Ľṹ��ĳ�ʼ������������б�����ʼ�����������ĺô��������л�˵����

ͨ���������ԣ�����ָ��һ�ֲ�ͬ��ȱʡ��Ϊ����Ϊ��ʹ��pthread_create()�����߳�ʱ���ʼ��ͬ������ʱ������ָ�����Զ���
һ�������pthread_create���Բ���ȱʡֵͨ�����㹻�ˡ�

���Զ����ǲ�͸���ģ�������ͨ����ֱֵ�ӽ����޸ġ�ϵͳ�ṩ�����ڳ�ʼ�������ú�����ÿ�ֶ������͡�

��ʼ�����������Ժ����Ա���н��̷�Χ��������ʹ������ʱ��õķ��������ڳ���ִ������һ�����ú����б����״̬�淶��
Ȼ�󣬸�����Ҫ������Ӧ�����Զ���

ʹ�����Զ������������Ҫ�ŵ㡣

�� ʹ�����Զ�������Ӵ������ֲ�ԡ�

��ʹ֧�ֵ����Կ��ܻ���ʵ��֮�������仯����������Ҫ�޸����ڴ����߳�ʵ��ĺ���

���á���Щ�������ò���Ҫ�����޸ģ���Ϊ���Զ����������ڽӿ�֮��ġ�

���Ŀ��ϵͳ֧�ֵ������ڵ�ǰϵͳ�в����ڣ��������ʽ�ṩ���ܹ����µ����ԡ���

����Щ������һ��ǳ����׵���ֲ������Ϊֻ������ȷ�����λ�ó�ʼ�����Զ���һ

�μ��ɡ�

�� Ӧ�ó����е�״̬�淶�ѱ��򻯡�

���磬��������п��ܴ��ڶ����̡߳�ÿ���̶߳��ṩ�����ķ���ÿ���̶߳��и���

��״̬Ҫ����Ӧ�ó���ִ�г��ڵ�ĳһʱ�䣬�������ÿ���̳߳�ʼ���߳����Զ����Ժ������̵߳Ĵ������������Ѿ�Ϊ�����̳߳�ʼ�������Զ���
��ʼ���׶��Ǽ򵥺;ֲ��ġ������Ϳ��Կ����ҿɿ��ؽ����κ��޸ġ�
1 ��ʼ������

pthread_attr_init()���������Գ�ʼ��Ϊ��ȱʡֵ���洢�ռ�����ִ���ڼ����߳�ϵͳ����ġ�

����ԭ��

int pthread_attr_init(pthread_attr_t *tattr);


#include <pthread.h>
pthread_attr_t tattr;
int ret;
//initialize an attribute to the default value
ret = pthread_attr_init(&tattr);


2 ��������

����ԭ��

int pthread_attr_destroy(pthread_attr_t *tattr);



#include <pthread.h>
pthread_attr_t tattr;
int ret;
// destroy an attribute
ret = pthread_attr_destroy(&tattr);


3 ���÷���״̬

������������߳�(PTHREAD_CREATE_DETACHED)������߳�һ�˳�������������߳�ID��������Դ����������̲߳�׼���ȴ��߳��˳�

����ԭ��

int pthread_attr_setdetachstate(pthread_attr_t *tattr,int detachstate);


#include <pthread.h>
pthread_attr_t tattr;
int ret;
// set the thread detach state 
ret = pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED);


���ʹ��PTHREAD_CREATE_JOINABLE�����Ƿ����̣߳������Ӧ�ó��򽫵ȴ��߳���ɡ�Ҳ����˵�����򽫶��߳�ִ��pthread_join()��

�Ƿ����߳�����ֹ�󣬱���Ҫ��һ���߳���join���ȴ��������򣬲����ͷŸ��̵߳���Դ�Թ����߳�ʹ�ã�����ͨ���ᵼ���ڴ�й©��
��ˣ������ϣ���̱߳��ȴ����뽫���߳���Ϊ�����߳���������

4 ����ջ�����������С

pthread_attr_setguardsize()��������attr�����guardsize��

����ԭ��

int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);

������������ԭ��ΪӦ�ó����ṩ��guardsize���ԣ�

�� ����������ܻᵼ��ϵͳ��Դ�˷ѡ����Ӧ�ó��򴴽������̣߳�������֪��Щ�߳���Զ���������ջ������Թر������������
ͨ���ر���������������Խ�ʡϵͳ��Դ��

�� �߳���ջ�Ϸ���������ݽṹʱ��������Ҫ�ϴ����������������ջ�����

guardsize�����ṩ�˶�ջָ������ı�������������̵߳�ջʱʹ���˱������ܣ���ʵ�ֻ���ջ������˷�������ڴ档
�˶����ڴ�������뻺����һ�������Է�ֹջָ���ջ��������Ӧ�ó���������˻������У����������ܻᵼ��SIGSEGV�źű����͸����̡߳�
���guardsizeΪ�㣬�򲻻�Ϊʹ��attr�������߳��ṩ��������������guardsize�����㣬���Ϊÿ��ʹ��attr�������߳��ṩ��С����Ϊguardsize�ֽڵ������������
ȱʡ����£��߳̾���ʵ�ֶ���ķ��������������

����Ϻ�������ʵ�֣���guardsize��ֵ��������Ϊ�����õ�ϵͳ����PAGESIZE�ı�������μ�sys/mman.h�е�PAGESIZE��
���ʵ�ֽ�guardsize��ֵ��������ΪPAGESIZE�ı���������guardsize����ǰ����pthread_attr_setguardsize()ʱָ���������������С��
Ϊ��λ�洢��ָ��attr��pthread_attr_getguardsize()�ĵ��á�

 
5 ���þ�����Χ

��ʹ��pthread_attr_setscope()�����̵߳����÷�Χ��PTHREAD_SCOPE_SYSTEM��PTHREAD_SCOPE_PROCESS���� 
ʹ��PTHREAD_SCOPE_SYSTEMʱ�����߳̽���ϵͳ�е������߳̽��о�����ʹ��PTHREAD_SCOPE_PROCESSʱ�����߳̽�������е������߳̽��о�����

����ԭ�ͣ�

int pthread_attr_setscope(pthread_attr_t *tattr,int scope);


#include <pthread.h>
pthread_attr_t tattr;
int ret;
//bound thread  
ret = pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM);
//unbound thread  
ret = pthread_attr_setscope(&tattr, PTHREAD_SCOPE_PROCESS);


6 �����̲߳��м���

pthread_setconcurrency()֪ͨϵͳ������Ĳ�������

����ԭ�ͣ�

int pthread_setconcurrency(int new_level);


7 ���õ��Ȳ���

pthread_attr_setschedpolicy()���õ��Ȳ��ԡ�POSIX��׼ָ��SCHED_FIFO�������ȳ�����SCHED_RR��ѭ������SCHED_OTHER��ʵ�ֶ���ķ������ĵ��Ȳ�������

����ԭ�ͣ�

int pthread_attr_setschedpolicy(pthread_attr_t *tattr, int policy);


#include <pthread.h>
pthread_attr_t tattr;
int policy;
int ret;
// set the scheduling policy to SCHED_OTHER 
ret = pthread_attr_setschedpolicy(&tattr, SCHED_OTHER);


�� SCHED_FIFO

������ý��̾�����Ч���û�ID0�������÷�ΧΪϵͳ(PTHREAD_SCOPE_SYSTEM)������

�ȳ��߳�����ʵʱ(RT)�����ࡣ�����Щ�߳�δ�����ȼ����ߵ��߳���ռ����������

����̣߳�ֱ�����̷߳���������Ϊֹ�����ھ��н������÷�Χ

(PTHREAD_SCOPE_PROCESS))���̻߳�����ý���û����Ч�û�ID 0���̣߳���ʹ��

SCHED_FIFO��SCHED_FIFO����TS�����ࡣ

�� SCHED_RR

������ý��̾�����Ч���û�ID0�������÷�ΧΪϵͳ(PTHREAD_SCOPE_SYSTEM))��ѭ��

�߳�����ʵʱ(RT)�����ࡣ�����Щ�߳�δ�����ȼ����ߵ��߳���ռ��������Щ�߳�û

�з���������������ϵͳȷ����ʱ����ڽ�һֱִ����Щ�̡߳����ھ��н������÷�Χ

(PTHREAD_SCOPE_PROCESS)���̣߳���ʹ��SCHED_RR������TS�����ࣩ�����⣬��Щ��

�̵ĵ��ý���û����Ч���û�ID0��

SCHED_FIFO��SCHED_RR��POSIX��׼���ǿ�ѡ�ģ����ҽ�����ʵʱ�̡߳�

8 ���ü̳еĵ��Ȳ���

����ԭ�ͣ�

int pthread_attr_setinheritsched(pthread_attr_t *tattr, int inherit);


#include <pthread.h>
pthread_attr_t tattr;
int inherit;
int ret;
//use the current scheduling policy  
ret = pthread_attr_setinheritsched(&tattr, PTHREAD_EXPLICIT_SCHED);


inheritֵ�����PTHREAD_INHERIT_SCHED��ʾ�½����߳̽��̳д������߳��ж���ĵ��Ȳ��ԡ���������pthread_create()�����ж�������е������ԡ�

���ʹ��ȱʡֵPTHREAD_EXPLICIT_SCHED����ʹ��pthread_create()�����е����ԡ�

 
9 ���õ��Ȳ���

����ԭ�ͣ�

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


���Ȳ�������param�ṹ�ж���ġ���֧�����ȼ��������´������߳�ʹ�ô����ȼ����С�

 

ʹ��ָ�������ȼ������߳�

�����߳�֮ǰ�������������ȼ����ԡ���ʹ����sched_param�ṹ��ָ���������ȼ�������

�̡߳��˽ṹ����������������Ϣ��

�������߳�ʱ����ִ�����²�����

�� ��ȡ���в���

�� �������ȼ�

�� �������߳�

�� �ָ�ԭʼ���ȼ�

 
10 ����ջ��С

����ԭ��

int pthread_attr_setstacksize(pthread_attr_t *tattr,size_t size);


#include <pthread.h>
pthread_attr_t tattr;
size_t size;
int ret;
size = (PTHREAD_STACK_MIN + 0x4000);
//setting a new size  
ret = pthread_attr_setstacksize(&tattr,size);


stacksize���Զ���ϵͳ�����ջ��С�����ֽ�Ϊ��λ����size��ӦС��ϵͳ�������Сջ��С��

size�������߳�ʹ�õ�ջ���ֽ��������sizeΪ�㣬��ʹ��ȱʡ��С��

PTHREAD_STACK_MIN�������߳������ջ�ռ�������ջ�ռ�û�п���ִ��Ӧ�ó������������߳�����Ҫ��
ջ

ͨ�����߳�ջ�Ǵ�ҳ�߽翪ʼ�ġ��κ�ָ���Ĵ�С�����������뵽��һ��ҳ�߽硣���߱�

����Ȩ�޵�ҳ�������ӵ�ջ������ˡ������ջ������ᵼ�½�SIGSEGV�źŷ��͵�Υ����

�̡���ֱ��ʹ�õ��÷�������߳�ջ�����������޸ġ�

ָ��ջʱ����Ӧʹ��PTHREAD_CREATE_JOINABLE�����̡߳��ڸ��̵߳�pthread_join(3C)��

�÷���֮ǰ�������ͷŸ�ջ���ڸ��߳���ֹ֮ǰ�������ͷŸ��̵߳�ջ���˽������߳���

������ֹ��Ψһ�ɿ���ʽ��ʹ��pthread_join(3C)��
Ϊ�̷߳���ջ�ռ�

һ������£�����ҪΪ�̷߳���ջ�ռ䡣ϵͳ��Ϊÿ���̵߳�ջ����1MB������32λϵͳ����2MB������64λϵͳ���������ڴ棬
���������κν����ռ䡣ϵͳ��ʹ��mmap()��MAP_NORESERVEѡ�������з��䡣

ϵͳ������ÿ���߳�ջ�����к�ɫ����ϵͳͨ����ҳ���ӵ�ջ���������������ɫ���򣬴Ӷ�����ջ���������ҳ��Ч��
���һᵼ���ڴ棨����ʱ�����ϡ���ɫ���򽫱����ӵ������Զ������ջ�����۴�С����Ӧ�ó���ָ��������ʹ��ȱʡ��С��

�������������Ҫָ��ջ��/��ջ��С������Ա�����˽��Ƿ�ָ������ȷ�Ĵ�С����������ABI��׼�ĳ���Ҳ���ܾ�̬ȷ����ջ��С��
ջ��Сȡ����ִ�����ض�����ʱ������
�����Լ���ջ

ָ���߳�ջ��Сʱ�����뿼�Ǳ����ú����Լ�ÿ��Ҫ���õĺ��������ķ���������Ҫ���ǵ�����Ӧ���������������󡢾ֲ���������Ϣ�ṹ��

��ʱ����Ҫ��ȱʡջ��ͬ��ջ��һ�������ǣ��߳���Ҫ��ջ��С����ȱʡջ��С�����������ȱʡ��С̫��
��Ϊ��������ʹ�ò���������ڴ洴�������̣߳�����������Щ��ȱʡ�߳�ջ����Ĵ������ֽڵ�ջ�ռ䡣

��ջ������С������ͨ����Ϊ���ԣ���������С��С����������أ���������㹻��ջ�ռ�����������ջ������ջ֡������ֲ������ȡ�

Ҫ��ȡ��ջ��С�ľ�����С���ƣ�����ú�PTHREAD_STACK_MIN��PTHREAD_STACK_MIN�꽫���ִ��NULL���̵��̷߳��������ջ�ռ�����
���õ��߳������ջ��С������Сջ��С�������Сջ��СʱӦ�ǳ�������

#include <pthread.h>
pthread_attr_t tattr;
pthread_t tid;
int ret;
size_t size = PTHREAD_STACK_MIN + 0x4000;
ret = pthread_attr_init(&tattr); //initialized with default attributes /

ret = pthread_attr_setstacksize(&tattr,size); // setting the size of the stack also

ret = pthread_create(&tid,&tattr,start_routine,arg); // only size specified in tattr   



</pre><pre>


11 ����ջ��ַ�ʹ�С

pthread_attr_setstack()���������߳�ջ��ַ�ʹ�С��

����ԭ�ͣ�

int pthread_attr_setstack(pthread_attr_t *tattr,void *stackaddr,size_t stacksize);


#include <pthread.h>
pthread_attr_t tattr;
void *base;
size_t size;
int ret;
base= (void *) malloc(PTHREAD_STACK_MIN + 0x4000);
//setting a new address and size 
ret = pthread_attr_setstack(&tattr,base,PTHREAD_STACK_MIN + 0x4000);


stackaddr���Զ����߳�ջ�Ļ�׼����λ��ַ����stacksize����ָ��ջ�Ĵ�С�������stackaddr����Ϊ�ǿ�ֵ��������ȱʡ��NULL��
��ϵͳ���ڸõ�ַ��ʼ��ջ�������СΪstacksize��

base�������߳�ʹ�õ�ջ�ĵ�ַ�����baseΪNULL����pthread_create(3C)��Ϊ��С����Ϊstacksize�ֽڵ����̷߳���ջ��

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

