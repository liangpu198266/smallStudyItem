AC_INIT([sunny liang], [2.0], [877677512@qq.com])
AM_INIT_AUTOMAKE([
  -Wall foreign
])
AC_CONFIG_HEADERS([config.h])
AM_PROG_AR
AC_PROG_CC

# Checks for libraries.  
#需要加入的内容，因为使用了静态库
AC_PROG_RANLIB

AC_CONFIG_FILES([
  Makefile
  src_code/test/Makefile
  src_code/code/Makefile
  src_code/ae_lib/Makefile
  src_code/libev/Makefile
])
AC_OUTPUT

