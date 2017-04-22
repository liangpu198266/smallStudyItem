/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#ifndef __CMDHANDLE_H__
#define __CMDHANDLE_H__
#include "common.h"
#include <getopt.h>


/* ------------------------ Include ------------------------- */


/* ------------------------ Defines ------------------------- */


/* ------------------------ Types --------------------------- */
typedef int32 (*cmdParamFunc)(int8 *param);

typedef struct cmdParamSet
{
  int8            *cmdName;
  int32           hasParam;
  int8            cmd;
  void            *data;
  cmdParamFunc    function;
}cmdParamSet_t;


/* ------------------------ Macros -------------------------- */
#define CMD_REQUIRED_ARGUMENT   required_argument
#define CMD_NO_ARGUMENT         no_argument
#define CMD_OPTIONAL_ARGUMENT   optional_argument


/* ---------------- XXX:   Global Variables ----------------- */


/* -------------- Global Function Prototypes ---------------- */
int32 processCmd(int8 *cmd);
void printCmdHelp(void);
int32 cmdParamParse(int32 argc, int8 *argv[], cmdParamSet_t *cmdSets, int32 cmdSetsLen);

#endif

