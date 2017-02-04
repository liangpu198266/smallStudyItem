/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

/* ------------------------ Include ------------------------- */
#include "cmdHandle.h"
#include "logprint.h"
#include "object.h"
#include "modules.h"

/* ------------------------ Defines ------------------------- */
#define MAX_CMD_LEN 1024 /* max length of a command */


/* ------------------------ Types --------------------------- */
typedef enum
{
  CC_NO_TEST_ID             = 0,
  CC_TEST_MODULES,  
  CC_TEST_OBJFILE, 
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

static cmdMap_t cmdMap[CC_MAX_LENGTH] = 
{
  { "test",       CC_NO_TEST_ID,          cmdtestCase },
  { "modules",    CC_TEST_MODULES,        cmdtestModules },
  { "objfile",   CC_TEST_OBJFILE,        cmdtestFile },
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
  
  unRegisterModules();
}

/* ---------------- Static Global Variables ----------------- */
void printCmdHelp(void)
{
  printf(" exp: testtool test\n");
  printf(" test : general test item\n");
  printf(" modules : test modules\n");
  printf(" objfile : object file\n");
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


