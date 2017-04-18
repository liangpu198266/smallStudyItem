#ifndef __AE_H__
#define __AE_H__

#include <time.h>
#include <stdlib.h>

/*
* �¼�ִ��״̬
*/
#define AE_OK 0     // �ɹ�
#define AE_ERR -1   // ����

/*
* �ļ��¼�״̬
*/
#define AE_NONE 0       // δ����
#define AE_READABLE 1   // �ɶ�
#define AE_WRITABLE 2   // ��д

/*
* ʱ�䴦������ִ�� flags
*/
#define AE_FILE_EVENTS 1
#define AE_TIME_EVENTS 2
#define AE_ALL_EVENTS (AE_FILE_EVENTS|AE_TIME_EVENTS)
#define AE_DONT_WAIT 4

/*
* ����ʱ���¼��Ƿ�Ҫ����ִ�е� flag
*/
#define AE_NOMORE -1

/* Macros */
#define AE_NOTUSED(V) ((void) V)

/*
* �¼�������״̬
*/
struct aeEventLoop;

/* Types and data structures */
typedef void aeFileProc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
typedef int aeTimeProc(struct aeEventLoop *eventLoop, long long id, void *clientData);
typedef void aeEventFinalizerProc(struct aeEventLoop *eventLoop, void *clientData);
typedef void aeBeforeSleepProc(struct aeEventLoop *eventLoop);

/* File event structure
*
* �ļ��¼��ṹ
*/
typedef struct aeFileEvent {
    // �¼��������룬ֵ������ AE_READABLE �� AE_WRITABLE ���������ߵĻ�
    int mask; /* one of AE_(READABLE|WRITABLE) */
    // д�¼�����
    aeFileProc *rfileProc;
    // ���¼�����
    aeFileProc *wfileProc;
    // ��·���ÿ��˽������
    void *clientData;
} aeFileEvent;

/* Time event structure
*
* ʱ���¼��ṹ
*/
typedef struct aeTimeEvent {

    // ʱ���¼���Ψһ��ʶ��
    long long id; /* time event identifier. */

    // �¼��ĵ���ʱ��
    long when_sec; /* seconds */
    long when_ms; /* milliseconds */

    // �¼�������
    aeTimeProc *timeProc;

    // �¼��ͷź���
    aeEventFinalizerProc *finalizerProc;

    // ��·���ÿ��˽������
    void *clientData;

    // ָ���¸�ʱ���¼��ṹ���γ�����
    struct aeTimeEvent *next;

} aeTimeEvent;

/* A fired event
*
* �Ѿ����¼�
*/
typedef struct aeFiredEvent {
    // �Ѿ����ļ�������
    int fd;
    // �¼��������룬������ AE_READABLE �� AE_WRITABLE
    int mask;
} aeFiredEvent;

/* State of an event based program 
*
* �¼���������״̬
*/
typedef struct aeEventLoop {
    // Ŀǰ��ע������������
    int maxfd;   /* highest file descriptor currently registered */
    // Ŀǰ��׷�ٵ����������
    int setsize; /* max number of file descriptors tracked */
    // ��������ʱ���¼� id
    long long timeEventNextId;
    // ���һ��ִ��ʱ���¼���ʱ��
    time_t lastTime;     /* Used to detect system clock skew */
    // ��ע����ļ��¼�
    aeFileEvent *events; /* Registered events */
    // �Ѿ������ļ��¼�
    aeFiredEvent *fired; /* Fired events */
    // ʱ���¼�
    aeTimeEvent *timeEventHead;
    // �¼��������Ŀ���
    int stop;
    // ��·���ÿ��˽������
    void *apidata; /* This is used for polling API specific data */
    // �ڴ����¼�ǰҪִ�еĺ���
    aeBeforeSleepProc *beforesleep;
} aeEventLoop;

/* Prototypes */
aeEventLoop *aeCreateEventLoop(int setsize);
void aeDeleteEventLoop(aeEventLoop *eventLoop);
void aeStop(aeEventLoop *eventLoop);
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
                      aeFileProc *proc, void *clientData);
void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask);
int aeGetFileEvents(aeEventLoop *eventLoop, int fd);
long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,
                            aeTimeProc *proc, void *clientData,
                            aeEventFinalizerProc *finalizerProc);
int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id);
int aeProcessEvents(aeEventLoop *eventLoop, int flags);
int aeWait(int fd, int mask, long long milliseconds);
void aeMain(aeEventLoop *eventLoop);
char *aeGetApiName(void);
void aeSetBeforeSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *beforesleep);

#endif
