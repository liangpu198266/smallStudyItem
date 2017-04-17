/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */
#include "timeUtils.h"
#include "logprint.h"


/*
跟日期时间相关的shell命令
$ date                    // 显示当前日期
$ time                   // 显示程序运行的时间
$ hwclock             // 显示与设定硬件时钟
$ clock                  // 显示与设定硬件时钟，是hwclock的链接文件
$ cal                      // 显示日历

跟日期时间有关的数据结构
clock_t 结构

程序开始运行到此时所经过的CPU时钟计时单元数用clock数据类型表示。
 
typedef long clock_t;
#define CLOCKS_PER_SEC ((clock_t)1000)      // 每个时钟单元是1毫秒

time_t 结构

日历时间（Calendar Time）是通过time_t数据类型来表示的，用time_t表示的时间（日历时间）是从一个时间点（1970年1月1日0时0分0秒）到此时的秒数。
 
typedef long time_t;                     // 时间值

tm结构

通过tm结构来获得日期和时间
 
struct tm {
        int tm_sec;             秒 – 取值区间为[0,59]
        int tm_min;             分 - 取值区间为[0,59]
        int tm_hour;            时 - 取值区间为[0,23]
        int tm_mday;            一个月中的日期 - 取值区间为[1,31]
        int tm_mon;             月份（从一月开始，0代表一月） - 取值区间为[0,11]
        int tm_year;            年份，其值等于实际年份减去1900
        int tm_wday;            星期 – 取值区间为[0,6]，其中0代表星期天，1代表星期一
        int tm_yday;            从每年1月1日开始的天数– 取值区间[0,365]，其中0代表1月1日
        int tm_isdst;           夏令时标识符，夏令时tm_isdst为正；不实行夏令时tm_isdst为0；
};
tms结构

保存着一个进程及其子进程使用的CPU时间
struct tms{
       clock_t tms_utime;
       clock_t tms_stime;
       clock_t tms_cutime;
       clock_t tms_cstime;
}

Utimbuf结构

struct utimbuf{
       time_t     actime;           // 存取时间
       time_t     modtime;        // 修改时间
}
 
文件的时间
st_atime         文件数据的最后存取时间
st_mtime        文件数据的最后修改时间
st_ctime         文件数据的创建时间


timeval结构

struct timeval
{
    time_t tv_sec;
    susecond_t tv_usec; //当前妙内的微妙数
};

timer_struct结构

struct timer_struct { 
    unsigned long expires; //定时器被激活的时刻

    void (*fn)(void); //定时器激活后的处理函数 }
===========================================
跟日期时间相关的函数
-------------------------------------------
#include <time.h>
clock_t clock(void);                   
返回从程序开始运行到程序中调用clock()函数之间的CPU时钟计时单元数
-------------------------------------------

日历时间 
#include <time.h>
time_t time(time_t *calptr)；       
返回自1970-1-1:00:00:00以来经过的秒数累计值
-------------------------------------------

程序运行的时间
#include <sys/times.h>
clock_t times(struct tms *buf);   
返回自系统自举后经过的时钟滴答数
-------------------------------------------

将日历时间变换成本地时间，考虑到本地时区和夏令时标志。
#include <time.h>
struct tm *localtime(const time_t * calptr);
-------------------------------------------

将日历时间变换成国际标准时间的年月日分秒
#include <time.h>
struct tm *gmtime(const time_t *calptr);
-------------------------------------------

以本地时间的年月日为参数，将其变换成time_t值
#include <time.h>
time_t mktime(struct tm *tmptr);
-------------------------------------------

产生形式的26字节字符串，参数指向年月日等字符串的指针。与date命令输出形式类似
#include <time.h>
char *asctime(const struct tm *tmptr);
-------------------------------------------

产生形式的26字节字符串，参数指向日历时间的指针。
#include <time.h>
char *ctime(const time_t *calptr);
-------------------------------------------

格式化时间输出
#include <time.h>
size_t strftime(char *buf,size_t maxsize,const char *format,const struct tm *tmptr);

%a 星期几的简写 
%A 星期几的全称 
%b 月分的简写 
%B 月份的全称 
%c 标准的日期的时间串 
%C 年份的后两位数字 
%d 十进制表示的每月的第几天 
%D 月/天/年 
%e 在两字符域中，十进制表示的每月的第几天 
%F 年-月-日 
%g 年份的后两位数字，使用基于周的年 
%G 年分，使用基于周的年 
%h 简写的月份名 
%H 24小时制的小时 
%I 12小时制的小时 
%j 十进制表示的每年的第几天 
%m 十进制表示的月份 
%M 十时制表示的分钟数 
%n 新行符 
%p 本地的AM或PM的等价显示 
%r 12小时的时间 
%R 显示小时和分钟：hh:mm 
%S 十进制的秒数 
%t 水平制表符 
%T 显示时分秒：hh:mm:ss 
%u 每周的第几天，星期一为第一天 （值从0到6，星期一为0） 
%U 第年的第几周，把星期日做为第一天（值从0到53） 
%V 每年的第几周，使用基于周的年 
%w 十进制表示的星期几（值从0到6，星期天为0） 
%x 标准的日期串 
%X 标准的时间串 
%y 不带世纪的十进制年份（值从0到99） 
%Y 带世纪部分的十进制年份 
%z，%Z 时区名称，如果不能得到时区名称则返回空字符。 
%% 百分号 
-------------------------------------------

更改文件的存取和修改时间
#include <time.h>
int utime(const char pathname, const struct utimbuf *times)      ；
返回值：成功返回0，失败返回-1
-------------------------------------------

取得目前的时间

#include <time.h>
int gettimeofday ( struct& nbsptimeval * tv , struct timezone * tz )；


函数说明  gettimeofday()会把目前的时间有tv所指的结构返回，当地时区的信息则放到tz所指的结构中。
timeval结构定义为:
struct timeval{
long tv_sec; 秒
long tv_usec; 微秒
};
timezone 结构定义为:
struct timezone{
int tz_minuteswest; 和格林威治时间差了多少分钟
int tz_dsttime; 日光节约时间的状态
};

上述两个结构都定义在/usr/include/sys/time.h。tz_dsttime 所代表的状态如下
DST_NONE 不使用
DST_USA 美国
DST_AUST 澳洲
DST_WET 西欧
DST_MET 中欧
DST_EET 东欧
DST_CAN 加拿大
DST_GB 大不列颠
DST_RUM 罗马尼亚
DST_TUR 土耳其
DST_AUSTALT 澳洲（1986年以后）

返回值  成功则返回0，失败返回－1，错误代码存于errno。附加说明EFAULT指针tv和tz所指的内存空间超出存取权限。
-------------------------------------------

设置目前时间

#include<sys/time.h>
#include<unistd.h>
 
int settimeofday ( const& nbspstruct timeval *tv,const struct timezone *tz);
 
函数说明  settimeofday()会把目前时间设成由tv所指的结构信息，当地时区信息则设成tz所指的结构。详细的说明请参考gettimeofday()。注意，只有root权限才能使用此函数修改时间。
 
返回值  成功则返回0，失败返回－1，错误代码存于errno。
 
错误代码  EPERM 并非由root权限调用settimeofday（），权限不够。
EINVAL 时区或某个数据是不正确的，无法正确设置时间。

格式化时间输出
#include <time.h>
size_t strftime(char *buf,size_t maxsize,const char *format,const struct tm *tmptr);


更改文件的存取和修改时间
#include <time.h>
int utime(const char pathname, const struct utimbuf *times)      ；
返回值：成功返回0，失败返回-1
times 为空指针，存取和修改时间设置为当前时间
=================================
1000 毫秒 = 1秒
1000 微秒 = 1毫秒 
1000 纳秒 = 1微秒
1000 皮秒 = 1纳秒 
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

/*将日历时间变换成国际标准时间的年月日分秒*/
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
产生形式的26字节字符串，参数指向年月日等字符串的指针。与date命令输出形式类似
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
设置操作系统时间
参数:*dt数据格式为"2006-4-20 20:30:30"
调用方法:
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

