/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#include "logprint.h"
#include "stringUtils.h"

/*�õ���һ��������Ҫ���ҵ��ַ�����λ��*/
int32 getFirstLocationAtString(const int8 *string,const int8 *findString)
{
  char *ptr = NULL;
  int32 ret = 0;
  
  ptr = index(string, findString[0]);  
  if (ptr == NULL)
  {
    LOG_ERROR("the need string doesn't find\n");
    return TASK_ERROR;
  }
  else
  {
    ret = strncmp(ptr, findString, strlen(findString));
    if (ret == 0)
    {
      LOG_DBG("the need string find\n");
      return TASK_ERROR;
    }
  }

  LOG_ERROR("the need string doesn't find\n");
  return TASK_ERROR;
}

/*�õ����һ��������Ҫ���ҵ��ַ�����λ��*/
int32 getLastLocationAtString(const int8 *string,const int8 *findString)
{
  char *ptr = NULL;
  int32 ret = 0;
  
  ptr = rindex(string, findString[0]);  
  if (ptr == NULL)
  {
    LOG_ERROR("the need string doesn't find\n");
    return TASK_ERROR;
  }
  else
  {
    ret = strncmp(ptr, findString, strlen(findString));
    if (ret == 0)
    {
      LOG_DBG("the need string find\n");
      return TASK_OK;
    }
  }

  LOG_ERROR("the need string doesn't find\n");
  return TASK_ERROR;
}

/*�ж��ַ����Ƿ����(���Դ�Сд)*/
int32 strCmpNoCaseSensitive(const int8 *s1, const int8 *s2)
{
  if (s1 == NULL || s2 == NULL)
  {
    LOG_ERROR("s1 or s2 is null\n");
    return TASK_ERROR;
  }
  
  if (strlen(s1) == strlen(s2))
  {
    if (!strcasecmp(s1, s2))
    {
      LOG_DBG("s1 = s2\n");
      return TASK_OK;
    }
  }

  return TASK_ERROR;
}

/*�ַ�������Сд��ĸת��Ϊ��д��ĸ*/
int32 stringToupper(int8 *string)
{
  int32 i;
  
  if (string == NULL)
  {
    LOG_ERROR("string is null\n");
    return TASK_ERROR;
  }

  LOG_DBG("string : %s\n", string);

  for (i = 0; i < sizeof(string); i++)
  {
     string[i] = toupper(string[i]);
  }

  LOG_DBG("string change : %s\n", string);

  return TASK_OK;
}

/*�ַ���:����д��ĸת��ΪСд��ĸ*/
int32 stringTolower(int8 *string)
{
  int32 i;
  
  if (string == NULL)
  {
    LOG_ERROR("string is null\n");
    return TASK_ERROR;
  }

  LOG_DBG("string : %s\n", string);

  for (i = 0; i < sizeof(string); i++)
  {
     string[i] = tolower(string[i]);
  }

  LOG_DBG("string change : %s\n", string);

  return TASK_OK;
}

/*�����ַ������״γ����Ӵ��ĵ�ַ*/

int8 *getStringAtFisrtFind(const int8 *string, const int8 *subStr)
{
  char *s = NULL;
  
  if (string == NULL || subStr == NULL)
  {
    LOG_ERROR("string is null\n");
    return TASK_ERROR;
  }

  s = strstr(string, subStr);
  if (s == NULL)
  {
    LOG_ERROR("string is null\n");
    return NULL;
  }

  return s;
}
