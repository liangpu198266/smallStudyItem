AUTOMAKE_OPTIONS = subdir-objects  #¿ªÆô¶ÔÎÄ¼þ¼ÐÏÂÎÄ¼þÖ§³Ö£¬·ñÔò»á±¨´í : but option 'subdir-objects' is disabled
# Applications and libraries
# noinst ¿¿¿¿¿¿¿¿¿¿¿¿¿make install ¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿

libae_a_CFLAGS   = -Wcast-align -DDEBUG_TO_SCREEN -g -ggdb3

noinst_LIBRARIES = libae.a

libae_a_SOURCES  =                          \
        	ae.c      		    \
        	ae_epoll.c       	    \
        	anet.c                      


libae_a_LDFLAGS  = -pthread
