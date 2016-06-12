# smallStudyItem
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
Part 1 how to build project by automake
1 autoscan命令生成一个名为configure.scan文件
2 将configure.scan改名为configure.in
3 修改configure.in

For Example:
===============================================================
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([netcmd], [1.0], [liangpu198266@163.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_PROG_RANLIB

# Checks for programs.
AC_PROG_CC
AC_CONFIG_FILES([
  Makefile
  src/Makefile
])
# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT
==================================================================

4 执行以下两个命令，分别生成aclocal.m4和configure文件 生成：aclocal autoconf
然后在src下创建Makefile.am

For example:
===================================================================
#applications and libraries
bin_PROGRAMS            = netcmd

#sub source code
netcmd_SOURCES          = netcmd.c fileUtils.c
netcmd_CFLAGS           =-DDEBUG_MODE -g
===================================================================
5 automake --add-missing
6 ./configure  && make

注意：如果有libtools错误
[1]libtoolize --version
[2]libtoolize --automake --copy --debug --force
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
