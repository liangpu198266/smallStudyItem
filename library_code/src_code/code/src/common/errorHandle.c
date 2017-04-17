/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */
#include <execinfo.h>
 
#include "logprint.h"

#define  DEFAULT_LOG_LENGTH  240

/* 打印调用栈的最大深度 */
#define DUMP_STACK_DEPTH_MAX 16

static int32 dumpTrace(void) 
{
  void *stack_trace[DUMP_STACK_DEPTH_MAX] = {0};
  char **stack_strings = NULL;
  int stack_depth = 0;
  int i = 0;

  /* 获取栈中各层调用函数地址 */
  stack_depth = backtrace(stack_trace, DUMP_STACK_DEPTH_MAX);

  /* 查找符号表将函数调用地址转换为函数名称 */
  stack_strings = (char **)backtrace_symbols(stack_trace, stack_depth);
  if (NULL == stack_strings) 
  {
    LOG_ERROR(" Memory is not enough while dump Stack Trace! \r\n");
    return TASK_ERROR;
  }

  /* 打印调用栈 */
  LOG_ERROR(" Stack Trace: \r\n");
  for (i = 0; i < stack_depth; ++i) 
  {
    LOG_ERROR(" [%d] %s \r\n", i, stack_strings[i]);
  }

  /* 获取函数名称时申请的内存需要自行释放 */
  free(stack_strings);
  stack_strings = NULL;

  return TASK_OK;
}


void displayErrorMsg(int8 *msg)
{
  perror(msg);
  if (dumpTrace() == TASK_ERROR)
  {
    LOG_ERROR("dump trace error\n");
  }
}

void dumpSigProcessTrace(int32 signum, siginfo_t *info, void *context)
{
  char buf[DEFAULT_LOG_LENGTH];
  snprintf(buf, sizeof(buf), " ");
  LOG_ERROR("%s", buf);
  LOG_ERROR("%s\r\n", buf);

  snprintf(buf, sizeof(buf), "------ Unexpected termination ------");
  LOG_ERROR("%s", buf);
  LOG_ERROR("%s\r\n", buf);

  snprintf(buf, sizeof(buf), "Signal:    %d (%s)", signum, strsignal(signum));
  LOG_ERROR("%s", buf);
  LOG_ERROR("%s\r\n", buf);

  snprintf(buf, sizeof(buf), "Process:   %d", getpid());
  LOG_ERROR("%s", buf);
  LOG_ERROR("%s\r\n", buf);

  dumpTrace();
}


