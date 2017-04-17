AUTOMAKE_OPTIONS = subdir-objects  #开启对文件夹下文件支持，否则会报错 : but option 'subdir-objects' is disabled

# Applications and libraries
bin_PROGRAMS          = testtool

testtool_CFLAGS   = -Wcast-align -DDEBUG_TO_SCREEN -g -ggdb3

AM_CPPFLAGS = -Icode/include/

testtool_SOURCES  =                          \
        test/testMain.c                      \
        code/src/common/cmdHandle.c          \
        code/src/common/errorHandle.c        \
        code/src/common/fileUtils.c          \
        code/src/common/logprint.c           \
        code/src/common/netUtils.c           \
        code/src/common/ioUtils.c            \
        code/src/common/lockUtils.c          \
        code/src/common/timeUtils.c          \
        code/src/common/sigUtils.c          \
        code/src/common/stringUtils.c        \
        code/src/common/pthreadUtils.c       \
	code/src/common/ipcUtils.c       \
        code/src/object/modules.c            \
        code/src/object/object.c             \
        code/src/object/objectBus.c          \
        code/src/object/objectDev.c          \
        code/src/object/objectFile.c         \
        code/src/object/objectSocket.c        


testtool_LDFLAGS  = -pthread
