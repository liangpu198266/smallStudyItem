/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */


#include "cmdHandle.h"

/* ------------------------ Include ------------------------- */


/* ------------------------ Defines ------------------------- */


/* ------------------------ Types --------------------------- */


/* ------------------------ Macros -------------------------- */


/* ---------------- XXX:   Global Variables ----------------- */


/* ---------------- Static Global Variables ----------------- */



/* --------------- Local Function Prototypes ---------------- */

int32 main(int32 argc, char *argv[])
{

  if (argc <= 1 || argc > 2)
  {
    printCmdHelp();
    exit(0);
  }

  processCmd(argv[1]);
}

