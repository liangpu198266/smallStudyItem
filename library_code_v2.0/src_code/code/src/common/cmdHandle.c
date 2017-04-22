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
һ����ѯlinux�����ֲ᣺

���ƴ���
#include<unistd.h>
#include<getopt.h>          //����ͷ�ļ� 
int getopt(intargc, char * const argv[], const char *optstring);
int getopt_long(int argc, char * const argv[], const char *optstring,
                          const struct option *longopts, int*longindex);
int getopt_long_only(int argc, char * const argv[],const char *optstring,
                          const struct option *longopts, int*longindex);
extern char *optarg;         //ϵͳ������ȫ�ֱ��� 
extern int optind, opterr, optopt;
���ƴ���
������򵥵� getopt ����������getopt_long ֻ��ǰ�ߵ���ǿ�棬���ܶ����ѡ�

����getopt����

1�����壺

int getopt(int argc, char * const argv[], const char *optstring);
2��������

getopt����������������ѡ������ģ�����ֻ�ܽ�����ѡ��: -d 100,���ܽ�����ѡ�--prefix
3��������

argc��main()�������ݹ����Ĳ����ĸ���
argv��main()�������ݹ����Ĳ������ַ���ָ������
optstring��ѡ���ַ�������֪ getopt()���Դ����ĸ�ѡ���Լ��ĸ�ѡ����Ҫ����
4�����أ�

���ѡ��ɹ��ҵ�������ѡ����ĸ���������������ѡ�������ϣ����� -1���������ѡ���ַ����� optstring �У������ַ� '?'��
���������ʧ��������ô����ֵ������ optstring �е�һ���ַ��������һ���ַ��� ':' �򷵻�':'�����򷵻�'?'����ʾ��������Ϣ��

5���±��ص����˵��optstring�ĸ�ʽ���壺

char*optstring = ��ab:c::��;
�����ַ�a         ��ʾѡ��aû�в���            ��ʽ��-a���ɣ����Ӳ���
���ַ���ð��b:     ��ʾѡ��b���ұ���Ӳ���      ��ʽ��-b 100��-b100,��-b=100��
���ַ���2ð��c::   ��ʾѡ��c�����У�Ҳ������     ��ʽ��-c200��������ʽ����
������� optstring �ڴ���֮��getopt ���������μ���������Ƿ�ָ���� -a�� -b�� -c(����Ҫ��ε��� getopt ������ֱ���䷵��-1)��
����鵽����ĳһ��������ָ��ʱ�������᷵�ر�ָ���Ĳ�������(������ĸ)

getopt() �����õ�ȫ�ֱ���������
optarg����ָ��ǰѡ�����������У���ָ�롣
optind�����ٴε��� getopt() ʱ����һ�� argv ָ���������
optopt�������һ����֪ѡ�

optarg ���� ָ��ǰѡ�����(�����)��ָ�롣
optind ���� �ٴε��� getopt() ʱ����һ�� argvָ���������
optopt ���� ���һ��δ֪ѡ�
opterr -���� �����ϣ��getopt()��ӡ������Ϣ����ֻҪ��ȫ�����opterr��Ϊ0���ɡ�

����getopt_long����

1�����壺

int getopt_long(int argc, char * const argv[], const char *optstring,

                                 const struct option *longopts,int *longindex);
2��������

���� getopt ���ܣ������˽�����ѡ��Ĺ����磺--prefix --help
3��������

longopts    ָ���˳����������ƺ�����
longindex   ���longindex�ǿգ���ָ��ı�������¼��ǰ�ҵ���������longopts��ĵڼ���Ԫ�ص�����������longopts���±�ֵ
4�����أ�

���ڶ�ѡ�����ֵͬgetopt���������ڳ�ѡ����flag��NULL������val�����򷵻�0�����ڴ����������ֵͬgetopt����
5��struct option
struct option {
const char  *name;       //�������� 
int          has_arg;    //ָ���Ƿ���в��� 
int          *flag;      /flag=NULLʱ,����value;��Ϊ��ʱ,*flag=val,����0 
int          val;        //����ָ�������ҵ�ѡ��ķ���ֵ��flag�ǿ�ʱָ��*flag��ֵ 
}; 
6������˵����

has_arg  ָ���Ƿ������ֵ������ֵ��ѡ��
no_argument         ������ѡ����������磺--name, --help
required_argument  ������ѡ�������������磺--prefix /root�� --prefix=/root
optional_argument  ������ѡ��Ĳ����ǿ�ѡ�ģ��磺--help�� �Cprefix=/root���������Ǵ���

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

