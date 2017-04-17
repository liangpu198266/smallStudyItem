/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */
#include "timeUtils.h"
#include "logprint.h"


/*
������ʱ����ص�shell����
$ date                    // ��ʾ��ǰ����
$ time                   // ��ʾ�������е�ʱ��
$ hwclock             // ��ʾ���趨Ӳ��ʱ��
$ clock                  // ��ʾ���趨Ӳ��ʱ�ӣ���hwclock�������ļ�
$ cal                      // ��ʾ����

������ʱ���йص����ݽṹ
clock_t �ṹ

����ʼ���е���ʱ��������CPUʱ�Ӽ�ʱ��Ԫ����clock�������ͱ�ʾ��
 
typedef long clock_t;
#define CLOCKS_PER_SEC ((clock_t)1000)      // ÿ��ʱ�ӵ�Ԫ��1����

time_t �ṹ

����ʱ�䣨Calendar Time����ͨ��time_t������������ʾ�ģ���time_t��ʾ��ʱ�䣨����ʱ�䣩�Ǵ�һ��ʱ��㣨1970��1��1��0ʱ0��0�룩����ʱ��������
 
typedef long time_t;                     // ʱ��ֵ

tm�ṹ

ͨ��tm�ṹ��������ں�ʱ��
 
struct tm {
        int tm_sec;             �� �C ȡֵ����Ϊ[0,59]
        int tm_min;             �� - ȡֵ����Ϊ[0,59]
        int tm_hour;            ʱ - ȡֵ����Ϊ[0,23]
        int tm_mday;            һ�����е����� - ȡֵ����Ϊ[1,31]
        int tm_mon;             �·ݣ���һ�¿�ʼ��0����һ�£� - ȡֵ����Ϊ[0,11]
        int tm_year;            ��ݣ���ֵ����ʵ����ݼ�ȥ1900
        int tm_wday;            ���� �C ȡֵ����Ϊ[0,6]������0���������죬1��������һ
        int tm_yday;            ��ÿ��1��1�տ�ʼ�������C ȡֵ����[0,365]������0����1��1��
        int tm_isdst;           ����ʱ��ʶ��������ʱtm_isdstΪ������ʵ������ʱtm_isdstΪ0��
};
tms�ṹ

������һ�����̼����ӽ���ʹ�õ�CPUʱ��
struct tms{
       clock_t tms_utime;
       clock_t tms_stime;
       clock_t tms_cutime;
       clock_t tms_cstime;
}

Utimbuf�ṹ

struct utimbuf{
       time_t     actime;           // ��ȡʱ��
       time_t     modtime;        // �޸�ʱ��
}
 
�ļ���ʱ��
st_atime         �ļ����ݵ�����ȡʱ��
st_mtime        �ļ����ݵ�����޸�ʱ��
st_ctime         �ļ����ݵĴ���ʱ��


timeval�ṹ

struct timeval
{
    time_t tv_sec;
    susecond_t tv_usec; //��ǰ���ڵ�΢����
};

timer_struct�ṹ

struct timer_struct { 
    unsigned long expires; //��ʱ���������ʱ��

    void (*fn)(void); //��ʱ�������Ĵ����� }
===========================================
������ʱ����صĺ���
-------------------------------------------
#include <time.h>
clock_t clock(void);                   
���شӳ���ʼ���е������е���clock()����֮���CPUʱ�Ӽ�ʱ��Ԫ��
-------------------------------------------

����ʱ�� 
#include <time.h>
time_t time(time_t *calptr)��       
������1970-1-1:00:00:00���������������ۼ�ֵ
-------------------------------------------

�������е�ʱ��
#include <sys/times.h>
clock_t times(struct tms *buf);   
������ϵͳ�Ծٺ󾭹���ʱ�ӵδ���
-------------------------------------------

������ʱ��任�ɱ���ʱ�䣬���ǵ�����ʱ��������ʱ��־��
#include <time.h>
struct tm *localtime(const time_t * calptr);
-------------------------------------------

������ʱ��任�ɹ��ʱ�׼ʱ��������շ���
#include <time.h>
struct tm *gmtime(const time_t *calptr);
-------------------------------------------

�Ա���ʱ���������Ϊ����������任��time_tֵ
#include <time.h>
time_t mktime(struct tm *tmptr);
-------------------------------------------

������ʽ��26�ֽ��ַ���������ָ�������յ��ַ�����ָ�롣��date���������ʽ����
#include <time.h>
char *asctime(const struct tm *tmptr);
-------------------------------------------

������ʽ��26�ֽ��ַ���������ָ������ʱ���ָ�롣
#include <time.h>
char *ctime(const time_t *calptr);
-------------------------------------------

��ʽ��ʱ�����
#include <time.h>
size_t strftime(char *buf,size_t maxsize,const char *format,const struct tm *tmptr);

%a ���ڼ��ļ�д 
%A ���ڼ���ȫ�� 
%b �·ֵļ�д 
%B �·ݵ�ȫ�� 
%c ��׼�����ڵ�ʱ�䴮 
%C ��ݵĺ���λ���� 
%d ʮ���Ʊ�ʾ��ÿ�µĵڼ��� 
%D ��/��/�� 
%e �����ַ����У�ʮ���Ʊ�ʾ��ÿ�µĵڼ��� 
%F ��-��-�� 
%g ��ݵĺ���λ���֣�ʹ�û����ܵ��� 
%G ��֣�ʹ�û����ܵ��� 
%h ��д���·��� 
%H 24Сʱ�Ƶ�Сʱ 
%I 12Сʱ�Ƶ�Сʱ 
%j ʮ���Ʊ�ʾ��ÿ��ĵڼ��� 
%m ʮ���Ʊ�ʾ���·� 
%M ʮʱ�Ʊ�ʾ�ķ����� 
%n ���з� 
%p ���ص�AM��PM�ĵȼ���ʾ 
%r 12Сʱ��ʱ�� 
%R ��ʾСʱ�ͷ��ӣ�hh:mm 
%S ʮ���Ƶ����� 
%t ˮƽ�Ʊ�� 
%T ��ʾʱ���룺hh:mm:ss 
%u ÿ�ܵĵڼ��죬����һΪ��һ�� ��ֵ��0��6������һΪ0�� 
%U ����ĵڼ��ܣ�����������Ϊ��һ�죨ֵ��0��53�� 
%V ÿ��ĵڼ��ܣ�ʹ�û����ܵ��� 
%w ʮ���Ʊ�ʾ�����ڼ���ֵ��0��6��������Ϊ0�� 
%x ��׼�����ڴ� 
%X ��׼��ʱ�䴮 
%y �������͵�ʮ������ݣ�ֵ��0��99�� 
%Y �����Ͳ��ֵ�ʮ������� 
%z��%Z ʱ�����ƣ�������ܵõ�ʱ�������򷵻ؿ��ַ��� 
%% �ٷֺ� 
-------------------------------------------

�����ļ��Ĵ�ȡ���޸�ʱ��
#include <time.h>
int utime(const char pathname, const struct utimbuf *times)      ��
����ֵ���ɹ�����0��ʧ�ܷ���-1
-------------------------------------------

ȡ��Ŀǰ��ʱ��

#include <time.h>
int gettimeofday ( struct& nbsptimeval * tv , struct timezone * tz )��


����˵��  gettimeofday()���Ŀǰ��ʱ����tv��ָ�Ľṹ���أ�����ʱ������Ϣ��ŵ�tz��ָ�Ľṹ�С�
timeval�ṹ����Ϊ:
struct timeval{
long tv_sec; ��
long tv_usec; ΢��
};
timezone �ṹ����Ϊ:
struct timezone{
int tz_minuteswest; �͸�������ʱ����˶��ٷ���
int tz_dsttime; �չ��Լʱ���״̬
};

���������ṹ��������/usr/include/sys/time.h��tz_dsttime �������״̬����
DST_NONE ��ʹ��
DST_USA ����
DST_AUST ����
DST_WET ��ŷ
DST_MET ��ŷ
DST_EET ��ŷ
DST_CAN ���ô�
DST_GB ���е�
DST_RUM ��������
DST_TUR ������
DST_AUSTALT ���ޣ�1986���Ժ�

����ֵ  �ɹ��򷵻�0��ʧ�ܷ��أ�1������������errno������˵��EFAULTָ��tv��tz��ָ���ڴ�ռ䳬����ȡȨ�ޡ�
-------------------------------------------

����Ŀǰʱ��

#include<sys/time.h>
#include<unistd.h>
 
int settimeofday ( const& nbspstruct timeval *tv,const struct timezone *tz);
 
����˵��  settimeofday()���Ŀǰʱ�������tv��ָ�Ľṹ��Ϣ������ʱ����Ϣ�����tz��ָ�Ľṹ����ϸ��˵����ο�gettimeofday()��ע�⣬ֻ��rootȨ�޲���ʹ�ô˺����޸�ʱ�䡣
 
����ֵ  �ɹ��򷵻�0��ʧ�ܷ��أ�1������������errno��
 
�������  EPERM ������rootȨ�޵���settimeofday������Ȩ�޲�����
EINVAL ʱ����ĳ�������ǲ���ȷ�ģ��޷���ȷ����ʱ�䡣

��ʽ��ʱ�����
#include <time.h>
size_t strftime(char *buf,size_t maxsize,const char *format,const struct tm *tmptr);


�����ļ��Ĵ�ȡ���޸�ʱ��
#include <time.h>
int utime(const char pathname, const struct utimbuf *times)      ��
����ֵ���ɹ�����0��ʧ�ܷ���-1
times Ϊ��ָ�룬��ȡ���޸�ʱ������Ϊ��ǰʱ��
=================================
1000 ���� = 1��
1000 ΢�� = 1���� 
1000 ���� = 1΢��
1000 Ƥ�� = 1���� 
*/

double gettimeRunByClock(functionRun func)
{
  double duration;
  clock_t start, end;

  start = clock();
  func();
  end = clock();
  
  duration = (double)(end-start)/CLOCKS_PER_SEC;
  
  LOG_DBG("function run time %f seconds\n", duration);

  return duration;
}

void getNowTime(time_t *now)
{
  time(now);
  LOG_DBG("now time is %d\n", *now);
}

int32 gettimeRunBytimes(functionRun func)
{
  clock_t start, end;
  struct tms tms_start, tms_end;
  int32 duration;

  if (func == NULL)
  {
    LOG_ERROR("no function add\n");
    return TASK_ERROR;
  }
  
  start = times(&tms_start);
  func();
  end = times(&tms_end);

  duration = (end-start)/CLOCKS_PER_SEC;
  LOG_DBG("function run time %d seconds\n", duration);

  return duration;
}

void displayLocalTime(void)
{
    time_t now;
    struct tm *tm_now;
 
    time(&now);
    tm_now = localtime(&now);
 
    printf("now datetime: %d-%d-%d %d:%d:%d\n", 
      tm_now->tm_year, tm_now->tm_mon, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
}

/*������ʱ��任�ɹ��ʱ�׼ʱ��������շ���*/
void displayLocalTimeToInter(void)
{
#include <stdio.h>
#include <time.h>
 
int main(void)
{
    time_t now;
    struct tm *tm_now;
 
    time(&now);
    tm_now = gmtime(&now);
 
    printf("now datetime: %d-%d-%d %d:%d:%d\n", tm_now->tm_year, tm_now->tm_mon, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
 
    return(0);
}

}

/*
������ʽ��26�ֽ��ַ���������ָ�������յ��ַ�����ָ�롣��date���������ʽ����
#include <time.h>
char *asctime(const struct tm *tmptr);

$ gcc asctime.c -o asctime
$ ./asctime
now datetime: Tue Oct 30 05:22:21 2007
*/

void displayLocalDateTime(void)
{
  time_t now;
  struct tm *tm_now;
  char *datetime;

  time(&now);
  tm_now = localtime(&now);
  datetime = asctime(tm_now);

  printf("now datetime: %s\n", datetime);
}

void displayNowTime(void)
{
  struct timeval tv;
  
  gettimeofday(&tv,NULL);
  printf("tv_sec; %ld\n",tv.tv_sec);
  printf("tv_usec; %ld\n",tv.tv_usec);
}

/************************************************
���ò���ϵͳʱ��
����:*dt���ݸ�ʽΪ"2006-4-20 20:30:30"
���÷���:
char *pt="2006-4-20 20:30:30";
SetSystemTime(pt);
**************************************************/
int32 setSystemTime(char *dt)
{
  struct tm _tm;
  struct timeval tv;
  time_t timep;

  if (dt == NULL)
  {
    LOG_ERROR("no input date\n");
    return TASK_ERROR;
  }
  
  sscanf(dt, "%d-%d-%d %d:%d:%d", &_tm.tm_year,
                                  &_tm.tm_mon, &_tm.tm_mday,&_tm.tm_hour,
                                  &_tm.tm_min, &_tm.tm_sec);
  _tm.tm_mon = _tm.tm_mon + 1;
  _tm.tm_year = _tm.tm_year + 1900;

  timep = mktime(&_tm);
  tv.tv_sec = timep;
  tv.tv_usec = 0;
  if(settimeofday (&tv, (struct timezone *) 0) < 0)
  {
    LOG_ERROR("Set system datatime error!\n");
    return TASK_ERROR;
  }
  
  return TASK_OK;
}

