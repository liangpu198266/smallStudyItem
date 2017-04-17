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
} ipcMsg_t;// ��Ϣ��ʽ������ն�һ��

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
linux�½��̼�ͨ�ŵļ�����Ҫ�ֶμ�飺
�ܵ���Pipe���������ܵ���named pipe�����ܵ������ھ�����Ե��ϵ���̼��ͨ�ţ������ܵ��˷��˹ܵ�û�����ֵ����ƣ���ˣ������йܵ������еĹ����⣬
  ������������Ե��ϵ���̼��ͨ�ţ�
  
�źţ�Signal�����ź��ǱȽϸ��ӵ�ͨ�ŷ�ʽ������֪ͨ���ܽ�����ĳ���¼��������������ڽ��̼�ͨ���⣬���̻����Է����źŸ����̱���
  linux����֧��Unix�����ź����庯��sigal�⣬��֧���������Posix.1��׼���źź���sigaction��ʵ���ϣ��ú����ǻ���BSD�ģ�BSDΪ��ʵ�ֿɿ��źŻ��ƣ�
  ���ܹ�ͳһ����ӿڣ���sigaction��������ʵ����signal��������
  
���ģ�Message�����У���Ϣ���У�����Ϣ��������Ϣ�����ӱ�����Posix��Ϣ����system V��Ϣ���С����㹻Ȩ�޵Ľ��̿���������������Ϣ��
  �������Ȩ�޵Ľ�������Զ��߶����е���Ϣ����Ϣ���п˷����źų�����Ϣ���٣��ܵ�ֻ�ܳ����޸�ʽ�ֽ����Լ���������С���޵�ȱ�㡣
  
�����ڴ棺ʹ�ö�����̿��Է���ͬһ���ڴ�ռ䣬�����Ŀ���IPC��ʽ�����������ͨ�Ż�������Ч�ʽϵͶ���Ƶġ�����������ͨ�Ż��ƣ�
  ���ź������ʹ�ã����ﵽ���̼��ͬ�������⡣
  
�ź�����semaphore������Ҫ��Ϊ���̼��Լ�ͬһ���̲�ͬ�߳�֮���ͬ���ֶΡ�

�׽ӿڣ�Socket������Ϊһ��Ľ��̼�ͨ�Ż��ƣ������ڲ�ͬ����֮��Ľ��̼�ͨ�š��������Unixϵͳ��BSD��֧���������ģ�
  ������һ�������ֲ��������Unixϵͳ�ϣ�Linux��System V�ı��ֶ�֧���׽��֡�
*/

/*
linux ��Ϣ����

  ��Ϣ���У�Ҳ�������Ķ��У��ܹ��˷�����unixͨ�Ż��Ƶ�һЩȱ�㡣��Ϊ����unixͨ�Ż���֮һ���ź��ܹ����͵���Ϣ�����ޣ�
������ȻPOSIX 1003.1b���źŵ�ʵʱ�Է��������ع㣬ʹ���ź��ڴ�����Ϣ�����������൱�̶ȵĸĽ��������ź�����ͨ�ŷ�ʽ����"��ʱ"��ͨ�ŷ�ʽ��
��Ҫ������źŵĽ�����ĳ��ʱ�䷶Χ�ڶ��ź�������Ӧ����˸��ź�����ڽ����źŽ��̵����������ڲ������壬
�ź������ݵ���Ϣ�ǽӽ�������̳����ĸ��process-persistent����
�� ��¼ 1���ܵ��������ܵ��������ܵ����ǵ��͵�����̳���IPC�����ң�ֻ�ܴ����޸�ʽ���ֽ������ɻ��Ӧ�ó��򿪷��������㣬
���⣬���Ļ�������СҲ�ܵ����ơ�
  ��Ϣ���о���һ����Ϣ���������԰���Ϣ����һ����¼�������ض��ĸ�ʽ�Լ��ض������ȼ�������Ϣ������дȨ�޵Ľ��̿������а���һ���Ĺ����������Ϣ��
����Ϣ�����ж�Ȩ�޵Ľ�������Դ���Ϣ�����ж�����Ϣ����Ϣ���������ں˳����ģ��μ� ��¼ 1����
  Ŀǰ��Ҫ���������͵���Ϣ���У�POSIX��Ϣ�����Լ�ϵͳV��Ϣ���У�ϵͳV��Ϣ����Ŀǰ������ʹ�á����ǵ�����Ŀ���ֲ�ԣ�
�¿�����Ӧ�ó���Ӧ����ʹ��POSIX��Ϣ���С�

һ����Ϣ���л�������

  ϵͳV��Ϣ���������ں˳����ģ�ֻ�����ں����������ʾɾ��һ����Ϣ����ʱ������Ϣ���вŻ�������ɾ����
���ϵͳ�м�¼��Ϣ���е����ݽṹ��struct ipc_ids msg_ids��λ���ں��У�ϵͳ�е�������Ϣ���ж������ڽṹmsg_ids���ҵ�������ڡ�

  ��Ϣ���о���һ����Ϣ������ÿ����Ϣ���ж���һ������ͷ���ýṹstruct msg_queue���������μ� ��¼ 2��������ͷ�а����˸���Ϣ���еĴ�����Ϣ��
������Ϣ���м�ֵ���û�ID����ID����Ϣ��������Ϣ��Ŀ�ȵȣ�������¼���������Ϣ���ж�д���̵�ID�����߿��Է�����Щ��Ϣ��Ҳ�����������е�ĳЩ��Ϣ��
 
  struct ipc_ids msg_ids���ں��м�¼��Ϣ���е�ȫ�����ݽṹ��struct msg_queue��ÿ����Ϣ���еĶ���ͷ��
  
����������Ϣ����
����Ϣ���еĲ����޷��������������ͣ�

1�� �򿪻򴴽���Ϣ���� 
��Ϣ���е��ں˳�����Ҫ��ÿ����Ϣ���ж���ϵͳ��Χ�ڶ�ӦΨһ�ļ�ֵ�����ԣ�Ҫ���һ����Ϣ���е������֣�ֻ���ṩ����Ϣ���еļ�ֵ���ɣ�
ע����Ϣ����������������ϵͳ��Χ��Ψһ�ļ�ֵ���ɵģ�����ֵ���Կ�����Ӧϵͳ�ڵ�һ��·����

2�� ��д����
��Ϣ��д�����ǳ��򵥣��Կ�����Ա��˵��ÿ����Ϣ���������µ����ݽṹ��
  struct msgbuf{
  long mtype;
  char mtext[1];
  };
  
mtype��Ա������Ϣ���ͣ�����Ϣ�����ж�ȡ��Ϣ��һ����Ҫ���ݾ�����Ϣ�����ͣ�mtext����Ϣ���ݣ���Ȼ���Ȳ�һ��Ϊ1��
��ˣ����ڷ�����Ϣ��˵������Ԥ��һ��msgbuf��������д����Ϣ���ͺ����ݣ�������Ӧ�ķ��ͺ������ɣ��Զ�ȡ��Ϣ��˵��
���ȷ�������һ��msgbuf��������Ȼ�����Ϣ����û��������ɡ�

3�� ��û�������Ϣ�������ԣ�
��Ϣ���е���Ϣ�����϶���������Ϣ����ͷ�У���ˣ����Է���һ����������Ϣ����ͷ�Ľṹ(struct msqid_ds���� ��¼ 2)��
��������Ϣ���е����ԣ�ͬ���������ø����ݽṹ��

��Ϣ����API

1���ļ�������ֵ
#include <sys/types.h>
#include <sys/ipc.h>
key_t ftok (char*pathname, char proj)��

��������·��pathname���Ӧ��һ����ֵ���ú�����ֱ�Ӷ���Ϣ���в��������ڵ���ipc(MSGGET,��)��msgget()�������Ϣ����������ǰ��
����Ҫ���øú��������͵ĵ��ô����ǣ�
key=ftok(path_ptr, 'a');
    ipc_id=ipc(MSGGET, (int)key, flags,0,NULL,0);
    ��
    
2��linuxΪ����ϵͳV���̼�ͨ�ŵ����ַ�ʽ����Ϣ���С��źŵơ������ڴ������ṩ��һ��ͳһ���û����棺
int ipc(unsigned int call, int first, int second, int third, void * ptr, long fifth);
��һ������ָ����IPC����Ĳ�����ʽ������Ϣ���ж��Թ������ֲ�����MSGSND��MSGRCV��MSGGET�Լ�MSGCTL���ֱ��������Ϣ���з�����Ϣ��
����Ϣ���ж�ȡ��Ϣ���򿪻򴴽���Ϣ���С�������Ϣ���У�first��������Ψһ��IPC�������潫�������ֲ�����
int ipc( MSGGET, intfirst, intsecond, intthird, void*ptr, longfifth); 
��ò�����Ӧ��ϵͳV����Ϊ��int msgget( (key_t)first��second)��
int ipc( MSGCTL, intfirst, intsecond, intthird, void*ptr, longfifth) 
��ò�����Ӧ��ϵͳV����Ϊ��int msgctl( first��second, (struct msqid_ds*) ptr)��
int ipc( MSGSND, intfirst, intsecond, intthird, void*ptr, longfifth); 
��ò�����Ӧ��ϵͳV����Ϊ��int msgsnd( first, (struct msgbuf*)ptr, second, third)��
int ipc( MSGRCV, intfirst, intsecond, intthird, void*ptr, longfifth); 
��ò�����Ӧ��ϵͳV����Ϊ��int msgrcv( first��(struct msgbuf*)ptr, second, fifth,third)��
ע�����˲����Ų���ϵͳ����ipc()�����������ڲ���ϵͳV����POSIX���̼�ͨ��API��ԭ�����£�
��Ȼ��ϵͳ�����ṩ��ͳһ���û����棬����������������ԣ����Ĳ����������ܸ����ض���ʵ�����壨����first��second����������������һ���̶�����ɿ������㡣
����ipc�ֲ���˵�ģ�ipc()��linux�����еģ���д����ʱӦע��������ֲ�����⣻
��ϵͳ���õ�ʵ�ֲ����ǰ�ϵͳV IPC���������˷�װ��û���κ�Ч���ϵ����ƣ�
ϵͳV��IPC�����API�������࣬��ʽҲ�ϼ�ࡣ

3.ϵͳV��Ϣ����API
ϵͳV��Ϣ����API�����ĸ���ʹ��ʱ��Ҫ��������ͷ�ļ���
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

1��int msgget(key_t key, int msgflg)
����key��һ����ֵ����ftok��ã�msgflg������һЩ��־λ���õ��÷����뽡ֵkey���Ӧ����Ϣ���������֡�
��������������£��õ��ý�����һ���µ���Ϣ���У�
���û����Ϣ�����뽡ֵkey���Ӧ������msgflg�а�����IPC_CREAT��־λ��
key����ΪIPC_PRIVATE��
����msgflg����Ϊ���£�IPC_CREAT��IPC_EXCL��IPC_NOWAIT�����ߵĻ�����
���÷��أ��ɹ�������Ϣ���������֣����򷵻�-1��
ע������key���óɳ���IPC_PRIVATE������ζ���������̲��ܷ��ʸ���Ϣ���У�ֻ��ζ�ż��������µ���Ϣ���С�

2��int msgrcv(int msqid, struct msgbuf *msgp, int msgsz, long msgtyp, int msgflg);
��ϵͳ���ô�msgid�������Ϣ�����ж�ȡһ����Ϣ��������Ϣ�洢��msgpָ���msgbuf�ṹ�С�
msqidΪ��Ϣ���������֣���Ϣ���غ�洢��msgpָ��ĵ�ַ��
msgszָ��msgbuf��mtext��Ա�ĳ��ȣ�����Ϣ���ݵĳ��ȣ���
msgtypΪ�����ȡ����Ϣ���ͣ�����Ϣ��־msgflg����Ϊ���¼�����ֵ�Ļ�
  IPC_NOWAIT ���û��������������Ϣ�������������أ���ʱ��errno=ENOMSG
  IPC_EXCEPT ��msgtyp>0���ʹ�ã����ض����е�һ�����Ͳ�Ϊmsgtyp����Ϣ
  IPC_NOERROR ���������������������Ϣ���ݴ����������msgsz�ֽڣ���Ѹ���Ϣ�ضϣ��ضϲ��ֽ���ʧ��
msgrcv�ֲ�����ϸ��������Ϣ����ȡ��ֵͬʱ(>0; <0; =0)�����ý�������Ϣ�����е��ĸ���Ϣ��
msgrcv()���������������������
��Ϣ����������������������Ϣ��
msqid�������Ϣ���б�ɾ����
����msgrcv�����Ľ��̱��ź��жϣ�
���÷��أ��ɹ����ض�����Ϣ��ʵ���ֽ��������򷵻�-1��

3��int msgsnd(int msqid, struct msgbuf *msgp, int msgsz, int msgflg);
��msgid�������Ϣ���з���һ����Ϣ���������͵���Ϣ�洢��msgpָ���msgbuf�ṹ�У���Ϣ�Ĵ�С��msgzeָ����
�Է�����Ϣ��˵���������msgflg��־ΪIPC_NOWAIT��ָ������Ϣ����û���㹻�ռ�����Ҫ���͵���Ϣʱ��msgsnd�Ƿ�ȴ������msgsnd()�ȴ������������֣�
��ǰ��Ϣ�Ĵ�С�뵱ǰ��Ϣ�����е��ֽ���֮�ͳ�������Ϣ���е���������
��ǰ��Ϣ���е���Ϣ������λ"��"����С����Ϣ���е�����������λ"�ֽ���"������ʱ����Ȼ��Ϣ�����е���Ϣ��Ŀ�ܶ࣬�������϶�ֻ��һ���ֽڡ�
msgsnd()���������������������
������������������������Ϣ�����������ɸ���Ϣ�Ŀռ䣻
msqid�������Ϣ���б�ɾ����
����msgsnd�����Ľ��̱��ź��жϣ�
���÷��أ��ɹ�����0�����򷵻�-1��

4��int msgctl(int msqid, int cmd, struct msqid_ds *buf);
��ϵͳ���ö���msqid��ʶ����Ϣ����ִ��cmd��������������cmd������IPC_STAT��IPC_SET ��IPC_RMID��
IPC_STAT��������������ȡ��Ϣ������Ϣ�����ص���Ϣ������bufָ���msqid�ṹ�У�
IPC_SET������������������Ϣ���е����ԣ�Ҫ���õ����Դ洢��bufָ���msqid�ṹ�У����������԰�����
msg_perm.uid��msg_perm.gid��msg_perm.mode�Լ�msg_qbytes��ͬʱ��ҲӰ��msg_ctime��Ա��
IPC_RMID��ɾ��msqid��ʶ����Ϣ���У�
���÷��أ��ɹ�����0�����򷵻�-1��

������Ϣ���е�����
ÿ����Ϣ���е��������������ɵ��ֽ������������ƣ���ֵ��ϵͳ��ͬ����ͬ���ں����Ӧ��ʵ���У������redhat 8.0�����ƣ�����μ� ��¼ 3��
��һ��������ÿ����Ϣ�����������ɵ������Ϣ������redhad 8.0�У�������������Ϣ����������Լ�ģ���Ϣ����ҪС����Ϣ���е��������ֽ�������
ע�������������������ÿ����Ϣ���ж��Եģ�ϵͳ����Ϣ���е����ƻ���ϵͳ��Χ�ڵ������Ϣ���и������Լ�����ϵͳ��Χ�ڵ������Ϣ����
һ����˵��ʵ�ʿ��������в��ᳬ��������ơ�

С�᣺
��Ϣ������ܵ��Լ������ܵ���ȣ����и��������ԣ����ȣ����ṩ�и�ʽ�ֽ����������ڼ��ٿ�����Ա�Ĺ�������
��Σ���Ϣ�������ͣ���ʵ��Ӧ���У�����Ϊ���ȼ�ʹ�á��������ǹܵ��Լ������ܵ������ܱȵġ�ͬ������Ϣ���п����ڼ������̼临�ã�
�������⼸�������Ƿ������Ե��ϵ����һ���������ܵ������ƣ�����Ϣ���������ں˳����ģ��������ܵ�������̳�������ȣ���������ǿ��Ӧ�ÿռ����
*/

/*
��Ϣ����API

��������Ϣ���л�ȡ���Ѵ�����Ϣ����
#include <sys/msg.h> ------------------------------------
int msgget(key_t key, int msgflg);
����	����
key	��ֵ��ÿ����Ϣ����keyֵ��ͬ������ʹ��ftok���ɶ��ڵ�key
msgflg	IPC_CREAT: ���û�иö��У��򴴽��ö��С�����ö����Ѿ����ڣ����ظö���ID.IPC_CREAT & IPC_EXCL: ����ö��в����ڴ�����������ڷ���ʧ��EEXIST.

����ж�����д����
#include <sys/msg.h>
-----------------------------------------
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp,int msgflg);
����	����
msqid	��Ϣ���е�������ID
msgp	ִ����Ϣ��������ָ�룬�����洢��Ϣ����ʽ���£�
msgsz	��Ϣ�Ĵ�С
msgflg	IPC_NOWAIT: �����Ϣ������û�����ݣ������̷��ز��õȴ���MSG_NOERROR:�����Ϣ���г��ȴ���msgsz,�ض���Ϣ��
struct msgbuf 
 {
      long mtype;       //message type, must be > 0
      char mtext[1];    //message data
 };
 
msgtyp:  ��Ϣ��͡� ȡֵ���£�
  msgtyp = 0 ��������ͣ�ֱ�ӷ�����Ϣ����еĵ�һ�
  msgtyp > 0 ,���ص�һ� msgtyp�c msgbuf�Y���w�е�mtype��ͬ����Ϣ
  msgtyp <0 , ���ص�һ� mtypeС춵��msgtyp�^��ֵ����Ϣ

msgflg:ȡֵ���£�
IPC_NOWAIT ,������
IPC_NOERROR ������Ϣ�L�ȳ��^�Δ�msgsz���t�ؔ���Ϣ�������e��

������Ϣ��������
#include <sys/msg.h> ---------------------
int msgctl(int msqid, int cmd, struct msqid_ds *buf);
����	����
msgid	��Ϣ����id
cmd	����ִ�е�����
bug	ִ��msqid_ds��ָ��
��һ����cmd��������ϸ����

cmd	����
IPC_STAT	ȡ�ô˶��е�msqid_ds�ṹ��������bufָ��Ľṹ�С�
IPC_SET	����������������Ϣ���е����ԣ�Ҫ���õ����Դ洢��buf�С�
IPC_RMID	���ں���ɾ�� msqid ��ʶ����Ϣ���С�
kernel����IPC����
����	����
auto_msgmni	����ϵͳmemory���ӣ��Ƴ�����namespace�������Ƴ��Զ���ȡmsgmni��ֵ
msgmni	���ļ�ָ����Ϣ���б�ʶ�������Ŀ����ϵͳ��Χ�������ٸ���Ϣ���С�
msgmnb	���ļ�ָ��һ����Ϣ���е���󳤶ȣ�bytes����
msgmax	���ļ�ָ���˴�һ�����̷��͵���һ�����̵���Ϣ����󳤶ȣ�bytes����
������Ϣ���Ը���ipcs -l���ϵͳ����Ϣ

------ Messages: Limits --------
max queues system wide = 2779
max size of message (bytes) = 8192
default max size of queue (bytes) = 16384
����kernel.txt���Ե�֪��auto_msgmni��ֵĬ����1��������Ϊ0��ʱ�򣬾Ͳ����Զ���ȡmsgmni
*/

/*
 ϵͳ����IPCͨѶ������Ϣ���С������ڴ�ʱ������ָ��һ��IDֵ��ͨ������£���idֵͨ��ftok�����õ���
ftokԭ�����£�
key_t ftok( char * fname, int id )

fname��ʱ��ָ�����ļ���(���ļ������Ǵ��ڶ��ҿ��Է��ʵ�)��id������ţ���ȻΪint������ֻ��8�����ر�ʹ��(0-255)��

���ɹ�ִ�е�ʱ��һ��key_tֵ���ᱻ���أ����� -1 �����ء�

   ��һ���UNIXʵ���У��ǽ��ļ��������ڵ��ȡ����ǰ���������ŵõ�key_t�ķ���ֵ����ָ���ļ��������ڵ��Ϊ65538�������16����Ϊ 0x010002��
   ����ָ����IDֵΪ38�������16����Ϊ0x26��������key_t����ֵΪ0x26010002��
   
��ѯ�ļ������ڵ�ŵķ����ǣ� ls -i

ftok������
  ����pathnameָ�����ļ�����Ŀ¼�����ƣ��Լ�proj_id����ָ�������֣�ftok����ΪIPC��������һ��Ψһ�Եļ�ֵ��
��ʵ��Ӧ���У������ײ�����һ������ǣ���proj_id��ͬ������£�ֻҪ�ļ�����Ŀ¼�����Ʋ��䣬�Ϳ���ȷ��ftok����ʼ��һ�µļ�ֵ��
Ȼ���������Ⲣ����ȫ��ȷ���п��ܸ�Ӧ�ÿ������º����޵����塣��Ϊftok��ʵ�ִ��������ķ��գ�
���ڷ���ͬһ�����ڴ�Ķ�������Ⱥ����ftok������ʱ����У����pathnameָ�����ļ�����Ŀ¼����ɾ�������´�����
���ļ�ϵͳ�ḳ�����ͬ���ļ�����Ŀ¼���µ�i�ڵ���Ϣ��������Щ���������õ�ftok��Ȼ�����������أ����õ��ļ�ֵȴ�����ܱ�֤��ͬ��
�ɴ˿�����ɵĺ���ǣ�ԭ����Щ������ͼ����һ����ͬ�Ĺ����ڴ����Ȼ���������Ǹ��Եõ��ļ�ֵ��ͬ��ʵ���Ͻ���ָ��Ĺ����ڴ治��һ�£�
�����Щ�����ڴ涼�õ���������������Ӧ�����еĹ����б����ϲ��ᱨ���κδ���Ȼ��ͨ��һ�������ڴ����������ݴ����Ŀ�Ľ��޷�ʵ�֡�
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
  
  msgId = msgget((key_t)key, 0666 | IPC_CREAT);//������Ϣ��ʶ��key = 1234����Ϣ���С�����ö����Ѿ����ڣ���ֱ�ӷ��ظö��еı�ʶ�����Ա������Ϣ�����շ���Ϣ
  if (msgId < 0)
  {
    LOG_ERROR("msgget failed with error\n");
    return TASK_ERROR;
  }

  while(running)
  {
    memset(&msgBuffer, 0x0, sizeof(ipcMsg_t));
     
    msgBuffer.msgType = type; //������䣬�ڱ�����û���ر���
    
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
  
  msgId = msgget((key_t)key, 0666 | IPC_CREAT);//������Ϣ��ʶ��key = 1234����Ϣ���С�����ö����Ѿ����ڣ���ֱ�ӷ��ظö��еı�ʶ�����Ա������Ϣ�����շ���Ϣ
  if (msgId < 0)
  {
    LOG_ERROR("msgget failed with error\n");
    return TASK_ERROR;
  }

  while(running)
  {
    memset(&msgBuffer, 0x0, sizeof(ipcMsg_t));
    if (msgrcv(msgId, (void *)&msgBuffer, MAX_TXT + sizeof(ipcMsgHeadInfo_t),0, 0) < 0) 
    { //����Ϣ���н�����Ϣ���������ʧ��ִ��if��䲢�˳�
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
�����ܵ�
1.1 �ܵ���صĹؼ�����
  �ܵ���Linux֧�ֵ����Unix IPC��ʽ֮һ�����������ص㣺
  �ܵ��ǰ�˫���ģ�����ֻ����һ��������������Ҫ˫��ͨ��ʱ����Ҫ�����������ܵ���
  ֻ�����ڸ��ӽ��̻����ֵܽ���֮�䣨������Ե��ϵ�Ľ��̣���
  ��������һ�ֶ������ļ�ϵͳ���ܵ����ڹܵ����˵Ľ��̶��ԣ�����һ���ļ�������������ͨ���ļ�����������ĳ���ļ�ϵͳ��
  ���������Ż�����������һ���ļ�ϵͳ������ֻ�������ڴ��С�
  ���ݵĶ�����д�룺һ��������ܵ���д�����ݱ��ܵ���һ�˵Ľ��̶�����д�������ÿ�ζ�����ڹܵ���������ĩβ��
  ����ÿ�ζ��Ǵӻ�������ͷ���������ݡ�

1.2�ܵ��Ĵ�����
  #include <unistd.h>
  int pipe(int fd[2])
  �ú��������Ĺܵ������˴���һ�������м䣬��ʵ��Ӧ����û��̫�����壬��ˣ�һ����������pipe()�����ܵ���
  һ����forkһ���ӽ��̣�Ȼ��ͨ���ܵ�ʵ�ָ��ӽ��̼��ͨ�ţ����Ҳ�����Ƴ���ֻҪ���������д�����Ե��ϵ��
  �������Ե��ϵָ���Ǿ��й�ͬ�����ȣ������Բ��ùܵ���ʽ������ͨ�ţ���

1.3�ܵ��Ķ�д����
  �ܵ����˿ɷֱ���������fd[0]�Լ�fd[1]����������Ҫע����ǣ��ܵ��������ǹ̶�������ġ���һ��ֻ�����ڶ�����������fd[0]��ʾ��
  ����Ϊ�ܵ����ˣ���һ����ֻ������д����������fd[1]����ʾ������Ϊ�ܵ�д�ˡ������ͼ�ӹܵ�д�˶�ȡ���ݣ�
  ������ܵ�����д�����ݶ������´�������һ���ļ���I/O�������������ڹܵ�����close��read��write�ȵȡ�

  �ӹܵ��ж�ȡ���ݣ�
  *����ܵ���д�˲����ڣ�����Ϊ�Ѿ����������ݵ�ĩβ�����������صĶ����ֽ���Ϊ0��
  *���ܵ���д�˴���ʱ�����������ֽ���Ŀ����PIPE_BUF���򷵻عܵ������е������ֽ��������������ֽ���Ŀ������PIPE_BUF��
  �򷵻عܵ������������ֽ�������ʱ���ܵ���������С��������������������߷���������ֽ�������ʱ���ܵ�����������С�����������������
  
  ע����PIPE_BUF��include/linux/limits.h�ж��壬��ͬ���ں˰汾���ܻ�������ͬ��Posix.1Ҫ��PIPE_BUF����Ϊ512�ֽڣ�
  red hat 7.2��Ϊ4096����

  ��ܵ���д�����ݣ��Թܵ���д�������֤1��д�˶Զ��˴��ڵ�������
  ��ܵ���д������ʱ��linux������֤д���ԭ���ԣ��ܵ�������һ�п�������д���̾ͻ���ͼ��ܵ�д�����ݡ�
  ��������̲����߹ܵ��������е����ݣ���ôд������һֱ������ 
  
  ע��ֻ���ڹܵ��Ķ��˴���ʱ����ܵ���д�����ݲ������塣������ܵ���д�����ݵĽ��̽��յ��ں˴�����SIFPIPE�źţ�
  Ӧ�ó�����Դ�����źţ�Ҳ���Ժ��ԣ�Ĭ�϶�������Ӧ�ó�����ֹ����

  Broken pipe,ԭ����Ǹùܵ��Լ���������fork()����Ķ��˶��Ѿ����رա�����ڸ������б������ˣ�����д��pipe��
  �ٹرո����̵Ķ��ˣ�Ҳ������д��pipe�����߿��Լ���֤һ�¸ý��ۡ���ˣ�����ܵ�д������ʱ������Ӧ�ô���ĳһ�����̣�
  ���йܵ�����û�б��رգ�����ͻ�����������󣨹ܵ�����,�����յ���SIGPIPE�źţ�Ĭ�϶����ǽ�����ֹ��

  �Թܵ���д�������֤2��linux����֤д�ܵ���ԭ������֤
  
1.5�ܵ��ľ�����
  �ܵ�����Ҫ�������������������ص��ϣ�
  ֻ֧�ֵ�����������
  ֻ�����ھ�����Ե��ϵ�Ľ���֮�䣻
  û�����֣�
  �ܵ��Ļ����������޵ģ��ܵ��ƴ������ڴ��У��ڹܵ�����ʱ��Ϊ����������һ��ҳ���С����
  �ܵ������͵����޸�ʽ�ֽ��������Ҫ��ܵ��Ķ�������д�뷽��������Լ�������ݵĸ�ʽ����������ֽ�����һ����Ϣ����������¼���ȵȣ�  

���ۣ�
д����ĿС��4096ʱд���Ƿ�ԭ�ӵģ� 
����Ѹ������е�����д���ֽ�������Ϊ5000��������׵ó�������ۣ� 
д��ܵ�������������4096�ֽ�ʱ���������Ŀ��пռ佫��д�����ݣ����룩��ֱ��д����������Ϊֹ�����û�н��̶����ݣ���һֱ������

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
�����ܵ����������APIӦ��
2.1 �����ܵ���صĹؼ�����
  �ܵ�Ӧ�õ�һ���ش���������û�����֣���ˣ�ֻ�����ھ�����Ե��ϵ�Ľ��̼�ͨ�ţ��������ܵ���named pipe��FIFO�������
  �����Ƶõ��˿˷���FIFO��ͬ�ڹܵ�֮���������ṩһ��·������֮��������FIFO���ļ���ʽ�������ļ�ϵͳ�С�������
  ��ʹ��FIFO�Ĵ������̲�������Ե��ϵ�Ľ��̣�ֻҪ���Է��ʸ�·�������ܹ��˴�ͨ��FIFO�໥ͨ��
  ���ܹ����ʸ�·���Ľ����Լ�FIFO�Ĵ�������֮�䣩����ˣ�ͨ��FIFO����صĽ���Ҳ�ܽ������ݡ�
  ֵ��ע����ǣ�FIFO�ϸ���ѭ�Ƚ��ȳ���first in first out�����Թܵ���FIFO�Ķ����Ǵӿ�ʼ���������ݣ�
  �����ǵ�д���������ӵ�ĩβ�����ǲ�֧������lseek()���ļ���λ������
  
2.2�����ܵ��Ĵ���
#include <sys/types.h>
#include <sys/stat.h>
  int mkfifo(const char * pathname, mode_t mode)
  �ú����ĵ�һ��������һ����ͨ��·������Ҳ���Ǵ�����FIFO�����֡��ڶ������������ͨ�ļ���open()�����е�mode ������ͬ�� 
  ���mkfifo�ĵ�һ��������һ���Ѿ����ڵ�·����ʱ���᷵��EEXIST��������һ����͵ĵ��ô������Ȼ����Ƿ񷵻ظô���
  ���ȷʵ���ظô�����ôֻҪ���ô�FIFO�ĺ����Ϳ����ˡ�һ���ļ���I/O��������������FIFO����close��read��write�ȵȡ�

2.3�����ܵ��Ĵ򿪹���
  �����ܵ��ȹܵ�����һ���򿪲�����open��
  FIFO�Ĵ򿪹���
  �����ǰ�򿪲�����Ϊ������FIFOʱ�����Ѿ�����Ӧ����Ϊд���򿪸�FIFO����ǰ�򿪲������ɹ����أ�
  ���򣬿�������ֱ������Ӧ����Ϊд���򿪸�FIFO����ǰ�򿪲���������������־�������ߣ��ɹ����أ���ǰ�򿪲���û������������־����
  �����ǰ�򿪲�����Ϊд����FIFOʱ������Ѿ�����Ӧ����Ϊ�����򿪸�FIFO����ǰ�򿪲������ɹ����أ�
  ���򣬿�������ֱ������Ӧ����Ϊ�����򿪸�FIFO����ǰ�򿪲���������������־�������ߣ�
  ����ENXIO���󣨵�ǰ�򿪲���û������������־����

2.4�����ܵ��Ķ�д����
  ��FIFO�ж�ȡ���ݣ�
  Լ�������һ������Ϊ�˴�FIFO�ж�ȡ���ݶ�������FIFO����ô�Ƹý����ڵĶ�����Ϊ������������־�Ķ�������
  ����н���д��FIFO���ҵ�ǰFIFO��û�����ݣ������������������־�Ķ�������˵����һֱ������
  ����û������������־��������˵�򷵻�-1����ǰerrnoֵΪEAGAIN�������Ժ����ԡ�
  ����������������־�Ķ�����˵�����������ԭ�������֣���ǰFIFO�������ݣ��������������ڶ���Щ���ݣ�
  �������FIFO��û�����ݡ���������ԭ������FIFO�����µ�����д�룬������д���������Ĵ�С��Ҳ���۶��������������������
  ���򿪵�������־ֻ�Ա����̵�һ��������ʩ�����ã�������������ж�����������У����ڵ�һ�������������Ѳ���ɶ�������
  ������Ҫִ�еĶ�������������������ʹ��ִ�ж�����ʱ��FIFO��û������Ҳһ������ʱ������������0����
  ���û�н���д��FIFO����������������־�Ķ�������������
  ע�����FIFO�������ݣ���������������־�Ķ�����������ΪFIFO�е��ֽ���С����������ֽ�������������ʱ��
  �������᷵��FIFO�����е���������
  
  ��FIFO��д�����ݣ�
  Լ�������һ������Ϊ����FIFO��д�����ݶ�������FIFO����ô�Ƹý����ڵ�д����Ϊ������������־��д������
  ����������������־��д������
  ��Ҫд���������������PIPE_BUFʱ��linux����֤д���ԭ���ԡ������ʱ�ܵ����л���������������Ҫд����ֽ�����
  �����˯�ߣ�ֱ�������������ܹ�����Ҫд����ֽ���ʱ���ſ�ʼ����һ����д������
  
  ��Ҫд�������������PIPE_BUFʱ��linux�����ٱ�֤д���ԭ���ԡ�FIFO������һ�п�������
  д���̾ͻ���ͼ��ܵ�д�����ݣ�д������д����������д�����ݺ󷵻ء�
  
  ����û������������־��д������
  ��Ҫд�������������PIPE_BUFʱ��linux�����ٱ�֤д���ԭ���ԡ���д������FIFO���л�������д�������ء�
  ��Ҫд���������������PIPE_BUFʱ��linux����֤д���ԭ���ԡ������ǰFIFO���л������ܹ���������д����ֽ�����
  д���ɹ����أ������ǰFIFO���л��������ܹ���������д����ֽ������򷵻�EAGAIN���������Ժ���д��

����д�򿪵�������־�Ƿ����ã�������д����ֽ�������4096ʱ��������֤д���ԭ���ԡ��������б�������
��������д��˵��д������д��FIFO�Ŀ�������󣬻�һֱ�ȴ���ֱ��д����������Ϊֹ������д����������ն���д��FIFO��
��������д����д��FIFO�Ŀ�������󣬾ͷ���(ʵ��д����ֽ���)��������Щ�������ղ��ܹ�д�롣
���ڶ���������֤��Ƚϼ򵥣��������ۡ�
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
     
    msgBuffer.msgType = type; //������䣬�ڱ�����û���ر���
    
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
linux �����ڴ�

  �ù����ڴ�ͨ�ŵ�һ���Զ��׼��ĺô���Ч�ʸߣ���Ϊ���̿���ֱ�Ӷ�д�ڴ棬������Ҫ�κ����ݵĿ�����
������ܵ�����Ϣ���е�ͨ�ŷ�ʽ������Ҫ���ں˺��û��ռ�����Ĵε����ݿ������������ڴ���ֻ������������[1]��
һ�δ������ļ��������ڴ�������һ�δӹ����ڴ���������ļ���ʵ���ϣ�����֮���ڹ����ڴ�ʱ���������Ƕ�д�������ݺ�ͽ��ӳ�䣬
���µ�ͨ��ʱ�������½��������ڴ����򡣶��Ǳ��ֹ�������ֱ��ͨ�����Ϊֹ����������������һֱ�����ڹ����ڴ��У���û��д���ļ��������ڴ��е������������ڽ��ӳ��ʱ��д���ļ��ġ���ˣ����ù����ڴ��ͨ�ŷ�ʽЧ���Ƿǳ��ߵġ�
Linux��2.2.x�ں�֧�ֶ��ֹ����ڴ淽ʽ����mmap()ϵͳ���ã�Posix�����ڴ棬�Լ�ϵͳV�����ڴ档
linux���а汾��Redhat 8.0֧��mmap()ϵͳ���ü�ϵͳV�����ڴ棬����ûʵ��Posix�����ڴ棬
���Ľ���Ҫ����mmap()ϵͳ���ü�ϵͳV�����ڴ�API��ԭ��Ӧ�á�

һ���ں�������֤��������Ѱַ��ͬһ�������ڴ�������ڴ�ҳ��
1��page cache��swap cache��ҳ������֣�һ���������ļ�������ҳ�涼פ����page cache��swap cache�У�һ��ҳ���������Ϣ��struct page��������struct page����һ����Ϊָ��mapping ����ָ��һ��struct address_space���ͽṹ��page cache��swap cache�е�����ҳ����Ǹ���address_space�ṹ�Լ�һ��ƫ���������ֵġ�
2���ļ���address_space�ṹ�Ķ�Ӧ��һ��������ļ��ڴ򿪺��ں˻����ڴ���Ϊ֮����һ��struct inode�ṹ�����е�i_mapping��ָ��һ��address_space�ṹ��������һ���ļ��Ͷ�Ӧһ��address_space�ṹ��һ��address_space��һ��ƫ�����ܹ�ȷ��һ��page cache ��swap cache�е�һ��ҳ�档��ˣ���ҪѰַĳ������ʱ�������׸��ݸ������ļ����������ļ��ڵ�ƫ�������ҵ���Ӧ��ҳ�档
3�����̵���mmap()ʱ��ֻ���ڽ��̿ռ���������һ����Ӧ��С�Ļ�����������������Ӧ�ķ��ʱ�ʶ������û�н������̿ռ䵽����ҳ���ӳ�䡣��ˣ���һ�η��ʸÿռ�ʱ��������һ��ȱҳ�쳣��
4�����ڹ����ڴ�ӳ�������ȱҳ�쳣�������������swap cache��Ѱ��Ŀ��ҳ������address_space�Լ�ƫ����������ҳ��������ҵ�����ֱ�ӷ��ص�ַ�����û���ҵ������жϸ�ҳ�Ƿ��ڽ�����(swap area)������ڣ���ִ��һ��������������������������������㣬������򽫷����µ�����ҳ�棬���������뵽page cache�С��������ս����½���ҳ�� 
ע������ӳ����ͨ�ļ�������ǹ���ӳ�䣩��ȱҳ�쳣����������Ȼ���page cache�и���address_space�Լ�����ƫ����Ѱ����Ӧ��ҳ�档���û���ҵ�����˵���ļ����ݻ�û�ж����ڴ棬��������Ӵ��̶�����Ӧ��ҳ�棬��������Ӧ��ַ��ͬʱ������ҳ��Ҳ����¡�
5�����н�����ӳ��ͬһ�������ڴ�����ʱ�������һ�����ڽ������Ե�ַ�������ַ֮���ӳ��֮�󣬲��۽��̸��Եķ��ص�ַ��Σ�ʵ�ʷ��ʵı�Ȼ��ͬһ�������ڴ������Ӧ������ҳ�档 
ע��һ�������ڴ�������Կ����������ļ�ϵͳshm�е�һ���ļ���shm�İ�װ���ڽ������ϡ�
�����漰����һЩ���ݽṹ��Χ�����ݽṹ������������һЩ��

����mmap()�������ϵͳ����
mmap()ϵͳ����ʹ�ý���֮��ͨ��ӳ��ͬһ����ͨ�ļ�ʵ�ֹ����ڴ档��ͨ�ļ���ӳ�䵽���̵�ַ�ռ��
���̿����������ͨ�ڴ�һ�����ļ����з��ʣ������ٵ���read()��write�����Ȳ�����
ע��ʵ���ϣ�mmap()ϵͳ���ò�������ȫΪ�����ڹ����ڴ����Ƶġ��������ṩ�˲�ͬ��һ�����ͨ�ļ��ķ��ʷ�ʽ��
���̿������д�ڴ�һ������ͨ�ļ��Ĳ�������Posix��ϵͳV�Ĺ����ڴ�IPC�򴿴����ڹ���Ŀ�ģ���Ȼmmap()ʵ�ֹ����ڴ�Ҳ������ҪӦ��֮һ��
1��mmap()ϵͳ������ʽ���£���

void* mmap ( void * addr , size_t len , int prot , int flags , int fd , off_t offset ) 
����fdΪ����ӳ�䵽���̿ռ���ļ������֣�һ����open()���أ�ͬʱ��fd����ָ��Ϊ-1����ʱ��ָ��flags�����е�MAP_ANON��
�������е�������ӳ�䣨���漰������ļ������������ļ��Ĵ������򿪣�����Ȼֻ�����ھ�����Ե��ϵ�Ľ��̼�ͨ�ţ���len��ӳ�䵽���ý��̵�ַ�ռ���ֽ��������ӱ�ӳ���ļ���ͷoffset���ֽڿ�ʼ����prot ����ָ�������ڴ�ķ���Ȩ�ޡ���ȡ���¼���ֵ�Ļ�PROT_READ���ɶ��� , PROT_WRITE ����д��, PROT_EXEC ����ִ�У�, PROT_NONE�����ɷ��ʣ���flags�����¼�����ֵָ����MAP_SHARED , MAP_PRIVATE , MAP_FIXED�����У�MAP_SHARED , MAP_PRIVATE��ѡ��һ����MAP_FIXED���Ƽ�ʹ�á�offset����һ����Ϊ0����ʾ���ļ�ͷ��ʼӳ�䡣����addrָ���ļ�Ӧ��ӳ�䵽���̿ռ����ʼ��ַ��һ�㱻ָ��һ����ָ�룬��ʱѡ����ʼ��ַ�����������ں�����ɡ������ķ���ֵΪ����ļ�ӳ�䵽���̿ռ�ĵ�ַ�����̿�ֱ�Ӳ�����ʼ��ַΪ��ֵ����Ч��ַ�����ﲻ����ϸ����mmap()�Ĳ��������߿ɲο�mmap()�ֲ�ҳ��ý�һ������Ϣ��
2��ϵͳ����mmap()���ڹ����ڴ�����ַ�ʽ��
��1��ʹ����ͨ�ļ��ṩ���ڴ�ӳ�䣺�������κν���֮�䣻 ��ʱ����Ҫ�򿪻򴴽�һ���ļ���Ȼ���ٵ���mmap()�����͵��ô������£�
	fd=open(name, flag, mode);
if(fd<0)
	...
ptr=mmap(NULL, len , PROT_READ|PROT_WRITE, MAP_SHARED , fd , 0); ͨ��mmap()ʵ�ֹ����ڴ��ͨ�ŷ�ʽ������ص��Ҫע��ĵط���
���ǽ��ڷ����н��о���˵����
��2��ʹ�������ļ��ṩ�����ڴ�ӳ�䣺�����ھ�����Ե��ϵ�Ľ���֮�䣻 ���ڸ��ӽ����������Ե��ϵ��
�ڸ��������ȵ���mmap()��Ȼ�����fork()����ô�ڵ���fork()֮���ӽ��̼̳и���������ӳ���ĵ�ַ�ռ䣬
ͬ��Ҳ�̳�mmap()���صĵ�ַ�����������ӽ��̾Ϳ���ͨ��ӳ���������ͨ���ˡ�ע�⣬���ﲻ��һ��ļ̳й�ϵ��һ����˵��
�ӽ��̵���ά���Ӹ����̼̳�������һЩ��������mmap()���صĵ�ַ��ȴ�ɸ��ӽ��̹�ͬά���� 
���ھ�����Ե��ϵ�Ľ���ʵ�ֹ����ڴ���õķ�ʽӦ���ǲ��������ڴ�ӳ��ķ�ʽ����ʱ������ָ��������ļ���
ֻҪ������Ӧ�ı�־���ɣ��μ�����2��

3��ϵͳ����munmap()
int munmap( void * addr, size_t len ) 
�õ����ڽ��̵�ַ�ռ��н��һ��ӳ���ϵ��addr�ǵ���mmap()ʱ���صĵ�ַ��len��ӳ�����Ĵ�С����ӳ���ϵ�����
��ԭ��ӳ���ַ�ķ��ʽ����¶δ�������
4��ϵͳ����msync()
int msync ( void * addr , size_t len, int flags) 
һ��˵����������ӳ��ռ�ĶԹ������ݵĸı䲢��ֱ��д�ص������ļ��У������ڵ���munmap�������ִ�иò�����
����ͨ������msync()ʵ�ִ������ļ������빲���ڴ���������һ�¡�

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
ϵͳ����mmap()ͨ��ӳ��һ����ͨ�ļ�ʵ�ֹ����ڴ档ϵͳV����ͨ��ӳ�������ļ�ϵͳshm�е��ļ�ʵ�ֽ��̼�Ĺ����ڴ�ͨ�š�
Ҳ����˵��ÿ�������ڴ������Ӧ�����ļ�ϵͳshm�е�һ���ļ�������ͨ��shmid_kernel�ṹ��ϵ�����ģ������滹��������

1��ϵͳV�����ڴ�ԭ��
���̼���Ҫ��������ݱ�����һ������IPC�����ڴ�����ĵط���������Ҫ���ʸù�������Ľ��̶�Ҫ�Ѹù�������ӳ�䵽�����̵ĵ�ַ�ռ���ȥ��ϵͳV�����ڴ�ͨ��shmget��û򴴽�һ��IPC�����ڴ����򣬲�������Ӧ�ı�ʶ�����ں��ڱ�֤shmget��û򴴽�һ�������ڴ�������ʼ���ù����ڴ�����Ӧ��shmid_kernel�ṹעͬʱ�������������ļ�ϵͳshm�У���������һ��ͬ���ļ��������ڴ��н�������ļ�����Ӧdentry��inode�ṹ���´򿪵��ļ��������κ�һ�����̣��κν��̶����Է��ʸù����ڴ�������������һ�ж���ϵͳ����shmget��ɵġ�
ע��ÿһ�������ڴ�������һ�����ƽṹstruct shmid_kernel��shmid_kernel�ǹ����ڴ������зǳ���Ҫ��һ�����ݽṹ�����Ǵ洢������ļ�ϵͳ����������������������£�
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
�ýṹ������Ҫ��һ����Ӧ����shm_file�����洢�˽���ӳ���ļ��ĵ�ַ��
ÿ�������ڴ������󶼶�Ӧ�����ļ�ϵͳshm�е�һ���ļ���һ������£�
�����ļ�ϵͳshm�е��ļ��ǲ�����read()��write()�ȷ������ʵģ�����ȡ�����ڴ�ķ�ʽ�����е��ļ�ӳ�䵽���̵�ַ�ռ��
��ֱ�Ӳ��÷����ڴ�ķ�ʽ������ʡ�

������Ϣ���к��źŵ�һ�����ں�ͨ�����ݽṹstruct ipc_ids shm_idsά��ϵͳ�е����й����ڴ�����
����ϵͳV�����ڴ�����˵��kern_ipc_perm��������shmid_kernel�ṹ��shmid_kernel����������һ�������ڴ�����ģ�
�����ں˾��ܹ�����ϵͳ�����еĹ�������ͬʱ����shmid_kernel�ṹ��file����ָ��shm_fileָ���ļ�ϵͳshm����Ӧ���ļ���
�����������ڴ��������shm�ļ�ϵͳ�е��ļ���Ӧ�������ڴ�����һ�������ڴ�����󣬻�Ҫ����ӳ�䵽���̵�ַ�ռ䣬
ϵͳ����shmat()��ɴ���ܡ������ڵ���shmget()ʱ���Ѿ��������ļ�ϵͳshm�е�һ��ͬ���ļ��빲���ڴ��������Ӧ��
��ˣ�����shmat()�Ĺ����൱��ӳ���ļ�ϵͳshm�е�ͬ���ļ����̣�ԭ����mmap()��ͬС�졣

2��ϵͳV�����ڴ�API
����ϵͳV�����ڴ棬��Ҫ�����¼���API��shmget()��shmat()��shmdt()��shmctl()��
#include <sys/ipc.h>
#include <sys/shm.h>
shmget����������ù����ڴ������ID�����������ָ���Ĺ�������ʹ�����Ӧ������
shmat()�ѹ����ڴ�����ӳ�䵽���ý��̵ĵ�ַ�ռ���ȥ�����������̾Ϳ��Է���ضԹ���������з��ʲ�����
shmdt()��������������̶Թ����ڴ������ӳ�䡣shmctlʵ�ֶԹ����ڴ�����Ŀ��Ʋ������������ǲ�����Щϵͳ����������Ľ��ܣ�
        ���߿ɲο���Ӧ���ֲ�ҳ�棬����ķ����н��������ǵĵ��÷�����
ע��shmget���ڲ�ʵ�ְ����������Ҫ��ϵͳV�����ڴ���ƣ�shmat�ڰѹ����ڴ�����ӳ�䵽���̿ռ�ʱ��
    ���������ı���̵�ҳ�������̵�һ�η����ڴ�ӳ���������ʱ������Ϊû������ҳ��ķ��������һ��ȱҳ�쳣��
    Ȼ���ں��ٸ�����Ӧ�Ĵ洢�������Ϊ�����ڴ�ӳ�����������Ӧ��ҳ��

3��ϵͳV�����ڴ�����
��/proc/sys/kernel/Ŀ¼�£���¼��ϵͳV�����ڴ��һ�����ƣ���һ�������ڴ���������ֽ���shmmax��
ϵͳ��Χ��������ڴ�����ʶ����shmmni�ȣ������ֹ���������������Ƽ���������

ͨ�����������������Ա�ϵͳV��mmap()ӳ����ͨ�ļ�ʵ�ֹ����ڴ�ͨ�ţ����Եó����½��ۣ�

1��	ϵͳV�����ڴ��е����ݣ�������д�뵽ʵ�ʴ����ļ���ȥ��
    ��ͨ��mmap()ӳ����ͨ�ļ�ʵ�ֵĹ����ڴ�ͨ�ſ���ָ����ʱ������д������ļ��С� 
    ע��ǰ�潲����ϵͳV�����ڴ����ʵ����ͨ��ӳ�������ļ�ϵͳshm�е��ļ�ʵ�ֵģ��ļ�ϵͳshm�İ�װ���ڽ��������ϣ�
    ϵͳ�������������е����ݶ���ʧ��
2��	ϵͳV�����ڴ������ں˳����ģ���ʹ���з��ʹ����ڴ�Ľ��̶��Ѿ�������ֹ�������ڴ�����Ȼ���ڣ�������ʽɾ�������ڴ棩��
    ���ں���������֮ǰ���Ըù����ڴ�������κθ�д��������һֱ������
3��	ͨ������mmap()ӳ����ͨ�ļ����н��̼�ͨ��ʱ��һ��Ҫע�⿼�ǽ��̺�ʱ��ֹ��ͨ�ŵ�Ӱ�졣
    ��ͨ��ϵͳV�����ڴ�ʵ��ͨ�ŵĽ�����Ȼ�� ע������û�и���shmctl��ʹ�÷�����ԭ������Ϣ���д�ͬС�졣

���ۣ�
  �����ڴ����������������̹���һ�����Ĵ洢������Ϊ���ݲ���Ҫ���ظ��ƣ�����������һ�ֽ��̼�ͨ�Ż��ơ�
�����ڴ����ͨ��mmap()ӳ����ͨ�ļ�����������»����Բ�������ӳ�䣩����ʵ�֣�Ҳ����ͨ��ϵͳV�����ڴ����ʵ�֡�
Ӧ�ýӿں�ԭ��ܼ򵥣��ڲ����Ƹ��ӡ�Ϊ��ʵ�ָ���ȫͨ�ţ����������źŵƵ�ͬ�����ƹ�ͬʹ�á�

  �����ڴ��漰���˴洢�����Լ��ļ�ϵͳ�ȷ����֪ʶ������������ڲ�������һ�����Ѷȣ��ؼ���Ҫ����ץס�ں�ʹ�õ���Ҫ���ݽṹ��
ϵͳV�����ڴ������ļ�����ʽ��֯�������ļ�ϵͳshm�еġ�ͨ��shmget���Դ������ù����ڴ�ı�ʶ����
ȡ�ù����ڴ��ʶ����Ҫͨ��shmat������ڴ���ӳ�䵽�����̵������ַ�ռ䡣

�����ڴ�ʹ�ò���

shmget() ����key�������߻�ȡ�����ڴ��shmid
shmat() ����attach���ѹ����ڴ�ӳ�䵽�����̵ĵ�ַ�ռ䣬��������ʹ�ù����ڴ�
��д�����ڴ�...
shmdt() ����detach����ֹ������ʹ�ù����ڴ�
shmctl(sid,IPC_RMID,0) ���ɾ�������ڴ档Ҳ����ʹ��ipcrm����ɾ��

int shmget(key_t key, size_t size, int shmflg);

������
key�����̼�����Լ����key�����ߵ���key_t ftok( char * fname, int id )��ȡ
size���ڴ��С���ֽڡ���������һ���µĹ����ڴ���ʱ��size ��ֵ������� 0 ������Ƿ���һ���Ѿ����ڵ��ڴ湲�������� size ������ 0 ����
shmflg����־λIPC_CREATE��IPC_EXCL

IPC_CREATE : ���� shmget ʱ��ϵͳ����ֵ�����������ڴ����� key ���бȽϡ����������ͬ�� key ��˵�������ڴ����Ѵ��ڣ���ʱ���ظù����ڴ�����shmid����������ڣ����½�һ�������ڴ�����������shmid��
IPC_EXCL : �ú����� IPC_CREATE һ��ʹ�ã�����û���塣�� shmflg ȡ IPC_CREATE | IPC_EXCL ʱ����ʾ��������ڴ����Ѿ������򷵻� -1���������Ϊ EEXIST ��
����ֵ���ɹ�����shmid��ʧ�ܷ���-1

һ�����Ǵ��������ڴ��ʱ����ڵ�һ��������ʹ��shmget�����������ڴ档����int shmid = shmget(key, size, IPC_CREATE|0666);
��������������ʹ��shmget��ͬ����key����ȡ������Ѿ������˵Ĺ����ڴ档Ҳ���ǵ���int shmid = shmget(key, size, IPC_CREATE|0666);

void *shmat(int shmid, const void *shmaddr, int shmflg);

������
shmid��shmget�ķ���ֵ
shmaddr�������ڴ����ӵ������̺��ڱ����̵�ַ�ռ���ڴ��ַ�����ΪNULL�������ں�ѡ��һ����еĵ�ַ������ǿգ����ɸò���ָ����ַ��һ�㴫��NULL��
shmflg��һ��Ϊ 0 ���������κ�����Ȩ�ޡ�

SHM_RDONLY ֻ��
SHM_RND ���ָ����shmaddr����ô���øñ�־λ�����ӵ��ڴ��ַ���SHMLBA����ȡ�����롣
SHM_REMAP take-over region on attach  �����
����ֵ���ɹ��󷵻�һ��ָ�����ڴ�����ָ�룻���򷵻� (void *) -1

ͨ��ʹ�� void ptr = shmat(shmid, NULL,0);

int shmdt(const void *shmaddr);

������
shmaddr��shmat �����ķ���ֵ
����ֵ���ɹ�����0��ʧ�ܷ���-1

����shmdt�󣬹����ڴ��shmid_ds�ṹ��仯������shm_dtimeָ��ǰʱ�䣬shm_lpid����Ϊ��ǰpid��shm_nattch��һ�����shm_nattch��Ϊ0�����Ҹù����ڴ��ѱ����Ϊɾ������ô�ù����ڴ�ᱻɾ������Ҳ����˵�������shmdt������û����ɾ�����Ͳ���ɾ�������ң�ֻ����ɾ����û����shmdtʹ��nattch=0��Ҳ����ɾ����

fork()���ӽ��̼̳и��������ӵĹ����ڴ档
execve()���ӽ���detach�������ӵĹ����ڴ档
_exit()��detach���й����ڴ档

��ò�Ҫ���������˳���detach��Ӧ����Ӧ�ó������shmdt����Ϊ��������쳣�˳���ϵͳ����detach��

ʹ��ipcs�������һֱ�����ҽ���Ϊ0�Ĺ����ڴ棬��ʱ������ͬ��IDȥ�ҽӻ����ܹҽ��ϵģ�˵����������ڴ�һֱ�Ǵ��ڵġ���Ҫ����shmctl(sid,IPC_RMID,0)����ִ��ipcrmɾ��������ᵼ����ص��ڴ�й©��

����

int shmctl(int shmid, int cmd, struct shmid_ds *buf);
������
cmd

IPC_STAT ��shmid_ds���ں��и��Ƶ��û�����
IPC_SET
IPC_RMID ���ɾ����ֻ�е�nattch=0ʱ�Ż�����ɾ���������ɾ���Ĺ����ڴ�shm_perm.mode��SHM_DEST�� ����shmctl(sid,IPC_RMID,0)
IPC_INFO Linux���С�����ϵͳ��Χ�ڵĹ����ڴ�����
SHM_INFO, SHM_STAT, SHM_LOCK, SHM_UNLOCKΪLinux���С�

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

