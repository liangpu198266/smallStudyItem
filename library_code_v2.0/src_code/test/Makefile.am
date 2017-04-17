AUTOMAKE_OPTIONS = subdir-objects  #开启对文件夹下文件支持，否则会报错 : but option 'subdir-objects' is disabled

# Applications and libraries
bin_PROGRAMS          = testtool

testtool_CFLAGS   = -Wcast-align -DDEBUG_TO_SCREEN -g -ggdb3

AM_CPPFLAGS = -I../code/include/

testtool_SOURCES  =                          \
         	testMain.c                      

testtool_LDADD = ../code/libcode.a

testtool_LDFLAGS  = -pthread
