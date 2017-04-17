/*
 * Copyright (C) sunny liang
 * Copyright (C) <877677512@qq.com> 
 */

#ifndef __NETUTILS_H__
#define __NETUTILS_H__
#include "common.h"
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <linux/sockios.h>
#include <linux/if.h>
#include <net/if_arp.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <netinet/in.h>
//#include <sys/un.h>
#include <sys/epoll.h>
#include <linux/un.h>
#include<netinet/tcp.h>

#define POLL_MAX            1000
#define EPOLL_MAX           1000
#define EPOLL_MAX_FD_NUM    1000

typedef int32 (*netFunctionRun)(int32 fd, struct sockaddr_in *ipAddr);


/*多路tcp io server*/
int32 netTcpSockSelectmultiIOServer(int32 port, int32 timeDelay, uint8 *ipAddr, netFunctionRun function);

/*一路 tcp sock server*/
int32 netTcpSockSelectServer(int32 port, int32 timeDelay, uint8 *ipAddr, netFunctionRun function);

int32 netTcpSockPollMultiIOServer(int32 port, int32 timeDelay, uint8 *ipAddr, netFunctionRun function);
int32 netTcpSockEpollServer(int32 port, int32 timeDelay, uint8 *ipAddr, netFunctionRun function);
int32 netTcpSockClient(int32 port, int32 mode, uint8 *ipAddr, netFunctionRun function);
int32 netUdpSockClient(int32 port, uint8 *ipAddr, netFunctionRun function);
int32 netUdpSockServer(int32 port, uint8 *ipAddr, netFunctionRun function);
int32 netUdpBroadcastSend(int32 port, uint8 *ipAddr, netFunctionRun function);
int32 netUdpBroadcastRev(int32 port, netFunctionRun function);
int32 netUdpSend(int32 sock, struct sockaddr_in addr, uint8 *buff, int32 bufLen);
int32 netUdpGroupBroadcastClient(int32 port, uint8 *ipAddr, netFunctionRun function);
int32 netUdpGroupBroadcastServer(int32 port, uint8 *ipGroupAddr, uint8 *peerAddr, netFunctionRun function);
int32 netUdpRev(int32 sock, uint8 *ipAddr, uint8 *buff);
int32 netUnixSockClient(uint8 *fileDir, netFunctionRun function);
int32 netUnixSockServer(uint8 *fileDir, int32 timeDelay, netFunctionRun function);

#endif

