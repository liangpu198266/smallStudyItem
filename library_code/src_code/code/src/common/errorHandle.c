/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */
#include <execinfo.h>
 
#include "logprint.h"

/* ��ӡ����ջ�������� */
#define DUMP_STACK_DEPTH_MAX 16

static int32 dumpTrace(void) 
{
  void *stack_trace[DUMP_STACK_DEPTH_MAX] = {0};
  char **stack_strings = NULL;
  int stack_depth = 0;
  int i = 0;

  /* ��ȡջ�и�����ú�����ַ */
  stack_depth = backtrace(stack_trace, DUMP_STACK_DEPTH_MAX);

  /* ���ҷ��ű��������õ�ַת��Ϊ�������� */
  stack_strings = (char **)backtrace_symbols(stack_trace, stack_depth);
  if (NULL == stack_strings) 
  {
    LOG_ERROR(" Memory is not enough while dump Stack Trace! \r\n");
    return TASK_ERROR;
  }

  /* ��ӡ����ջ */
  LOG_ERROR(" Stack Trace: \r\n");
  for (i = 0; i < stack_depth; ++i) 
  {
    LOG_ERROR(" [%d] %s \r\n", i, stack_strings[i]);
  }

  /* ��ȡ��������ʱ������ڴ���Ҫ�����ͷ� */
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
