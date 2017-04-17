AUTOMAKE_OPTIONS = subdir-objects  #¿ªÆô¶ÔÎÄ¼þ¼ÐÏÂÎÄ¼þÖ§³Ö£¬·ñÔò»á±¨´í : but option 'subdir-objects' is disabled
# Applications and libraries
# noinst ¿¿¿¿¿¿¿¿¿¿¿¿¿make install ¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿

libcode_a_CFLAGS   = -Wcast-align -DDEBUG_TO_SCREEN -g -ggdb3

AM_CPPFLAGS = -Iinclude/

noinst_LIBRARIES = libcode.a

libcode_a_SOURCES  =                          \
        	src/common/cmdHandle.c          \
        	src/common/errorHandle.c        \
        	src/common/fileUtils.c          \
        	src/common/logprint.c           \
        	src/common/netUtils.c           \
        	src/common/ioUtils.c            \
        	src/common/lockUtils.c          \
        	src/common/timeUtils.c          \
        	src/common/sigUtils.c          \
        	src/common/stringUtils.c        \
        	src/common/pthreadUtils.c       \
		src/common/ipcUtils.c       \
        	src/object/modules.c            \
        	src/object/object.c             \
        	src/object/objectBus.c          \
        	src/object/objectDev.c          \
        	src/object/objectFile.c         \
        	src/object/objectSocket.c        


libcode_a_LDFLAGS  = -pthread
