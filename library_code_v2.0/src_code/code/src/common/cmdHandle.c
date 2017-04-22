/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

/* ------------------------ Include ------------------------- */
#include "cmdHandle.h"
#include "logprint.h"
#include "object.h"
#include "modules.h"
#include "fileUtils.h"

/* ------------------------ Defines ------------------------- */
#define MAX_CMD_LEN 1024 /* max length of a command */


/* ------------------------ Types --------------------------- */
typedef enum
{
  CC_NO_TEST_ID             = 0,
  CC_TEST_MODULES,  
  CC_TEST_OBJFILE, 
  CC_TEST_FORK,
  CC_MAX_LENGTH
}cmdCode_t;


/* ------------------------ Macros -------------------------- */
typedef struct
{
  char cmd[MAX_CMD_LEN];
  cmdCode_t cmdCode;
  void (*func)(cmdCode_t cCode);
} cmdMap_t;


/* ---------------- XXX:   Global Variables ----------------- */


/* --------------- Local Function Prototypes ---------------- */
static void cmdtestCase(cmdCode_t cmdCode);
static void cmdtestModules(cmdCode_t cmdCode);
static void cmdtestFile(cmdCode_t cmdCode);
static void cmdtestFork(cmdCode_t cmdCode);

static cmdMap_t cmdMap[CC_MAX_LENGTH] = 
{
  { "test",       CC_NO_TEST_ID,          cmdtestCase },
  { "modules",    CC_TEST_MODULES,        cmdtestModules },
  { "objfile",    CC_TEST_OBJFILE,        cmdtestFile },
  { "fork",       CC_TEST_FORK,           cmdtestFork }
};

static void cmdtestCase(cmdCode_t cmdCode)
{
  printf(" only test cmd\n");
}

static void cmdtestModules(cmdCode_t cmdCode)
{
  printf(" only test modules\n");

  registerModules();

  unRegisterModules();
}

static void cmdtestFile(cmdCode_t cmdCode)
{
  objectNode_t *fileNode;
  int8 buf[100];

  memset(buf, 0x0, 100);
  
  printf(" only test modules\n");

  registerModules();
  
  fileNode = newObject(MODULES_FILE);
  if (fileNode == NULL)
  {
    displayErrorMsg("new obj file error\n");
  }

  fileNode->fOps->open(fileNode, "testFile", MODE_FILE_READWRITE | MODE_FILE_CREAT, 777);
  fileNode->fOps->write(fileNode, "This is test, hello world", strlen("This is test, hello world"));
  fileNode->fOps->close(fileNode);

  fileNode->fOps->open(fileNode, "testFile", MODE_FILE_READONLY, 777);
  fileNode->fOps->read(fileNode, buf, 100);
  fileNode->fOps->close(fileNode);

  printf("%s\n", buf);

  deleteObject(fileNode);
  
  unRegisterModules();
}

static void cmdtestFork(cmdCode_t cmdCode)
{
  printf(" only test fork\n");

}

/* ---------------- Static Global Variables ----------------- */
void printCmdHelp(void)
{
  printf(" exp: testtool test\n");
  printf(" test : general test item\n");
  printf(" modules : test modules\n");
  printf(" objfile : object file\n");
  printf(" fork : fork file\n");
}

int32 processCmd(int8 *cmd)
{
  int32 i;
  
  for (i = 0; i < CC_MAX_LENGTH;i++)
  {
    if (strncmp(cmdMap[i].cmd, cmd, MAX_CMD_LEN))
    {
      continue;
    }

    /* Found the command handler */
    cmdMap[i].func(cmdMap[i].cmdCode);
    return TASK_OK;    
  }

  return TASK_ERROR;
}


/***************system cmd parse**************/
/*
一、查询linux命令手册：

复制代码
#include<unistd.h>
#include<getopt.h>          //所在头文件 
int getopt(intargc, char * const argv[], const char *optstring);
int getopt_long(int argc, char * const argv[], const char *optstring,
                          const struct option *longopts, int*longindex);
int getopt_long_only(int argc, char * const argv[],const char *optstring,
                          const struct option *longopts, int*longindex);
extern char *optarg;         //系统声明的全局变量 
extern int optind, opterr, optopt;
复制代码
先拿最简单的 getopt 函数开刀，getopt_long 只是前者的增强版，功能多点而已。

二、getopt函数

1、定义：

int getopt(int argc, char * const argv[], const char *optstring);
2、描述：

getopt是用来解析命令行选项参数的，但是只能解析短选项: -d 100,不能解析长选项：--prefix
3、参数：

argc：main()函数传递过来的参数的个数
argv：main()函数传递过来的参数的字符串指针数组
optstring：选项字符串，告知 getopt()可以处理哪个选项以及哪个选项需要参数
4、返回：

如果选项成功找到，返回选项字母；如果所有命令行选项都解析完毕，返回 -1；如果遇到选项字符不在 optstring 中，返回字符 '?'；
如果遇到丢失参数，那么返回值依赖于 optstring 中第一个字符，如果第一个字符是 ':' 则返回':'，否则返回'?'并提示出错误信息。

5、下边重点举例说明optstring的格式意义：

char*optstring = “ab:c::”;
单个字符a         表示选项a没有参数            格式：-a即可，不加参数
单字符加冒号b:     表示选项b有且必须加参数      格式：-b 100或-b100,但-b=100错
单字符加2冒号c::   表示选项c可以有，也可以无     格式：-c200，其它格式错误
上面这个 optstring 在传入之后，getopt 函数将依次检查命令行是否指定了 -a， -b， -c(这需要多次调用 getopt 函数，直到其返回-1)，
当检查到上面某一个参数被指定时，函数会返回被指定的参数名称(即该字母)

getopt() 所设置的全局变量包括：
optarg——指向当前选项参数（如果有）的指针。
optind——再次调用 getopt() 时的下一个 argv 指针的索引。
optopt——最后一个已知选项。

optarg —— 指向当前选项参数(如果有)的指针。
optind —— 再次调用 getopt() 时的下一个 argv指针的索引。
optopt —— 最后一个未知选项。
opterr -—— 如果不希望getopt()打印出错信息，则只要将全域变量opterr设为0即可。

三、getopt_long函数

1、定义：

int getopt_long(int argc, char * const argv[], const char *optstring,

                                 const struct option *longopts,int *longindex);
2、描述：

包含 getopt 功能，增加了解析长选项的功能如：--prefix --help
3、参数：

longopts    指明了长参数的名称和属性
longindex   如果longindex非空，它指向的变量将记录当前找到参数符合longopts里的第几个元素的描述，即是longopts的下标值
4、返回：

对于短选项，返回值同getopt函数；对于长选项，如果flag是NULL，返回val，否则返回0；对于错误情况返回值同getopt函数
5、struct option
struct option {
const char  *name;       //参数名称 
int          has_arg;    //指明是否带有参数 
int          *flag;      /flag=NULL时,返回value;不为空时,*flag=val,返回0 
int          val;        //用于指定函数找到选项的返回值或flag非空时指定*flag的值 
}; 
6、参数说明：

has_arg  指明是否带参数值，其数值可选：
no_argument         表明长选项不带参数，如：--name, --help
required_argument  表明长选项必须带参数，如：--prefix /root或 --prefix=/root
optional_argument  表明长选项的参数是可选的，如：--help或 –prefix=/root，其它都是错误

*/

static void cmdOptsBuild(int8 *cmdOpts, struct option *longOpts, cmdParamSet_t *cmdSets, int32 cmdSetsLen)
{
  int32 i;

  for (i = 0; i < cmdSetsLen; i++)
  {
    if (i == 0)
    {
      sprintf(cmdOpts, "%s",cmdSets[i].cmd);
    }
    else
    {
      sprintf(cmdOpts, "%s%s",cmdOpts,cmdSets[i].cmd);
    }
    
    if (cmdSets->hasParam == CMD_REQUIRED_ARGUMENT)
    {
      sprintf(cmdOpts, "%s:",cmdOpts);    
    }
    else if (cmdSets->hasParam == CMD_OPTIONAL_ARGUMENT)
    {
      sprintf(cmdOpts, "%s::",cmdOpts);
    }

    longOpts[i].name = cmdSets[i].cmdName;
    longOpts[i].has_arg = cmdSets[i].hasParam;
    longOpts[i].flag = NULL;
    longOpts[i].val = cmdSets[i].cmd;
  }

  longOpts[i+1].name = NULL;
  longOpts[i+1].has_arg = 0;
  longOpts[i+1].flag = NULL;
  longOpts[i+1].val = 0;
}

static int32 cmdParamRun(int32 cmd, cmdParamSet_t *cmdSets, int32 cmdSetsLen)
{
  int32 i, ret;

  for (i = 0; i < cmdSetsLen; i++)
  {
    if (cmd == cmdSets[i].cmd)
    {
      ret = cmdSets[i].function(optarg);
    }
  }

  return ret;
}

int32 cmdParamParse(int32 argc, int8 *argv[], cmdParamSet_t *cmdSets, int32 cmdSetsLen)
{
  int32 cmdLen = 0, cmd, ret;
  int8 *cmdString = NULL;
  int32 cmdsLen = 2*cmdSetsLen+1;  
  struct option *longOpts = NULL;
    
  cmdString = malloc(cmdsLen);
  if (cmdString == NULL)
  {
    LOG_ERROR("cmdString string malloc error");
    return TASK_ERROR;
  }

  memset(cmdString, 0x0, cmdsLen);

  longOpts = malloc((cmdSetsLen + 1)*sizeof(struct option));
  if (longOpts == NULL)
  {
    LOG_ERROR("longOpts string malloc error");
    return TASK_ERROR;
  }

  memset(longOpts, 0x0, (cmdSetsLen + 1)*sizeof(struct option));
  
  cmdOptsBuild(cmdString, longOpts, cmdSets, cmdSetsLen);

  while ((cmd = getopt_long(argc, argv, cmdString, longOpts, NULL)) != -1)
  {
    ret = cmdParamRun(cmd, cmdSets, cmdSetsLen);
    if (ret == TASK_ERROR)
    {
      LOG_ERROR("longOpts string malloc error");
      return TASK_ERROR;
    }
  }

  LOG_DBG("cmdParamParse run ok");
  return TASK_OK;
}

