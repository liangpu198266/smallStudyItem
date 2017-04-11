#include "netUtils.h"
#include "logprint.h"

typedef struct netServerContext_
{
    int32 cliCnt;                 /*�ͻ��˸���*/
    fd_set allfds;              /*�������*/
    int32 maxfd;                  /*������ֵ*/
}netServerContext_t;

typedef enum
{
  NET_DEFAULT_MODE = 0,
  NET_MULTI_MODE
}netSockCtlOpt_t; 

typedef enum
{
  NET_TCP_SOCK_CTL = 0,
  NET_TCP_SOCK_SET,       /*����*/
  NET_TCP_SOCK_CLR        /*���*/
}netSockTcpOpt_t; 

/*
����I/O ģʽ��
��1�� ���� I/O (Linux�µ�I/O����Ĭ��������I/O����open��socket������I/O��������I/O)
��2�� ������ I/O (����ͨ��fcntl����openʱʹ��O_NONBLOCK��������fd����Ϊ��������I/O)
��3�� I/O ��·���� (I/O��·���ã�ͨ����Ҫ������I/O���ʹ��)
��4�� �ź����� I/O (SIGIO)
��5�� �첽 I/O

select I/Oģ����һ���첽I/Oģ�ͣ��ڵ��߳���Linux/WinNTĬ��֧��64���ͻ����׽��֡�

socket ��̼������ĸ���:

����: blocking
������: non-blocking
ͬ��: synchronous
�첽: asynchronous

  1. ͬ���������ҿͻ��ˣ�c�˵����ߣ�����һ�����ܣ��ù���û�н���ǰ���ң�c�˵����ߣ����Ƚ����
  2. �첽�������ң�c�˵����ߣ�����һ�����ܣ�����Ҫ֪���ù��ܽ�����ù����н����֪ͨ�ң�c�˵����ߣ����ص�֪ͨ��
  ͬ��/�첽��Ҫ���C��, ���Ǹ�S�˲�����ȫû�й�ϵ��ͬ��/�첽���Ʊ���S����ϲ���ʵ��.ͬ��/�첽����c���Լ�����,����S���Ƿ�����/������, C����ȫ����Ҫ����.
  3. ������      ���ǵ����ң�s�˱������ߣ����������ң�s�˱������ߣ�������û�н��������ݻ���û�еõ����֮ǰ���Ҳ��᷵�ء�
  4. ��������  ���ǵ����ң�s�˱������ߣ����������ң�s�˱������ߣ��������������أ�ͨ��select֪ͨ������

IOģʽ:

  IO �����׶�:
    �ȴ�����׼�� (Waiting for the data to be ready)
    �����ݴ��ں˿����������� (Copying the data from the kernel to the process)
    
  �ɴ˲�����5������ģʽ:
    ���� I/O��blocking IO��
    ������ I/O��nonblocking IO��
    I/O ��·���ã� IO multiplexing��
    �ź����� I/O�� signal driven IO��: ������,�Թ�.
    �첽 I/O��asynchronous IO��

IO��·������ָ�ں�һ�����ֽ���ָ����һ�����߶��IO����׼����ȡ������֪ͨ�ý��̡�IO��·�����������³��ϣ�
��1�����ͻ�������������ʱ��һ���ǽ���ʽ����������׽ӿڣ�������ʹ��I/O���á�
��2����һ���ͻ�ͬʱ�������׽ӿ�ʱ������������ǿ��ܵģ������ٳ��֡�
��3�����һ��TCP��������Ҫ��������׽ӿڣ���Ҫ�����������׽ӿڣ�һ��ҲҪ�õ�I/O���á�
��4�����һ����������Ҫ����TCP����Ҫ����UDP��һ��Ҫʹ��I/O���á�
��5�����һ��������Ҫ�������������Э�飬һ��Ҫʹ��I/O���á�
�����̺Ͷ��̼߳�����ȣ�I/O��·���ü��������������ϵͳ����С��ϵͳ���ش�������/�̣߳�Ҳ����ά����Щ����/�̣߳��Ӷ�����С��ϵͳ�Ŀ�����

�����ܽ�:

  ���� I/O��blocking IO��: IO ���׶ζ�����
  ������ I/O��nonblocking IO��:
      ��������ȷ, ��ȷ����ӦΪ: ��������,���ַ�����
      
  I/O ��·���ã� IO multiplexing��:
      ��˵�� select��poll��epoll����Щ�ط�Ҳ������IO��ʽΪevent driven IO
      
      ���� select:
        select �൱��һ�������н�, �����ڵ��� select()����ʱ,�� block,
        ��֮�� select(poll, epoll ��)���� �᲻�ϵ���ѯ�����������socket����ĳ��socket�����ݵ����ˣ���֪ͨ�û����̡�
      select/epoll�����Ʋ����Ƕ��ڵ��������ܴ���ø��죬���������ܴ����������ӡ�
      �����������������ǺܸߵĻ���ʹ��select/epoll��web server��һ����ʹ��multi-threading + blocking IO��web server���ܸ��ã������ӳٻ�����

  �첽 I/O��asynchronous IO��:
      linux�µ�asynchronous IO��ʵ�õú���

*/

/*
socket���������������ֱ�Ϊ��

domain����Э�����ֳ�ΪЭ���壨family�������õ�Э�����У�AF_INET��AF_INET6��AF_LOCAL�����AF_UNIX��Unix��socket����AF_ROUTE�ȵȡ�
        Э���������socket�ĵ�ַ���ͣ���ͨ���б�����ö�Ӧ�ĵ�ַ����AF_INET������Ҫ��ipv4��ַ��32λ�ģ���˿ںţ�16λ�ģ�����ϡ�
        AF_UNIX������Ҫ��һ������·������Ϊ��ַ��
type��ָ��socket���͡����õ�socket�����У�SOCK_STREAM��SOCK_DGRAM��SOCK_RAW��SOCK_PACKET��SOCK_SEQPACKET�ȵȣ�socket����������Щ������
protocol������˼�⣬����ָ��Э�顣���õ�Э���У�IPPROTO_TCP��IPPTOTO_UDP��IPPROTO_SCTP��IPPROTO_TIPC�ȣ�
          ���Ƿֱ��ӦTCP����Э�顢UDP����Э�顢STCP����Э�顢TIPC����Э�飨���Э���ҽ��ᵥ����ƪ���ۣ�����
*/

/*
  linux�µ�socket INADDR_ANY��ʾ����һ�������������е����������������ܲ�ֹһ���������������ip��ַ�����а󶨶˿ںţ�����������
INADDR_ANY����ָ����ַΪ0.0.0.0�ĵ�ַ,�����ַ��ʵ�ϱ�ʾ��ȷ����ַ,�����е�ַ�����������ַ���� һ����˵���ڸ���ϵͳ�о������Ϊ0ֵ��

  ���ڿͻ��������INADDR_ANY��������ơ�����TCP���ԣ���connect()ϵͳ����ʱ����󶥵�һ�����IP��ַ��
ѡ��������Ǹõ�ַ���� ������Ŀ���ַ�ǿɴ�ģ�reachable). ��ʱͨ��getsockname����ϵͳ���þ��ܵ�֪����ʹ����һ����ַ��
����UDP����, ����Ƚ����⡣��ʹʹ��connect()ϵͳ����Ҳ����󶨵�һ�����ַ��
������Ϊ��UDPʹ��connect()������������Ŀ���ַ�����κν����� �ӵ����ݣ�Ҳ������֤��Ŀ���ַ�Ŀɴ��ԡ�
��ֻ�ǽ�Ŀ���ַ����Ϣ��¼���ڲ���socket���ݽṹ֮�У����Ժ�ʹ�á�
ֻ�е����� sendto()/send()ʱ,��ϵͳ�ں˸���·�ɱ��������һ����ַ������������UDP packet.

INADDR_ANY��ANY���ǰ󶨵�ַ0.0.0.0�ϵļ���, ���յ�����һ�����������ӣ�
INADDR_LOOPBACK, Ҳ���ǰ󶨵�ַLOOPBAC, ������127.0.0.1, ֻ���յ�127.0.0.1�������������

*/

/*
listen()��connect()����
  �����Ϊһ�����������ڵ���socket()��bind()֮��ͻ����listen()���������socket������ͻ�����ʱ����connect()�����������󣬷������˾ͻ���յ��������

int listen(int sockfd, int backlog);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

listen�����ĵ�һ��������ΪҪ������socket�����֣��ڶ�������Ϊ��Ӧsocket�����Ŷӵ�������Ӹ�����
socket()����������socketĬ����һ���������͵ģ�listen������socket��Ϊ�������͵ģ��ȴ��ͻ�����������

connect�����ĵ�һ��������Ϊ�ͻ��˵�socket�����֣��ڶ�����Ϊ��������socket��ַ������������Ϊsocket��ַ�ĳ��ȡ�
�ͻ���ͨ������connect������������TCP�����������ӡ�

accept()����

TCP�����������ε���socket()��bind()��listen()֮�󣬾ͻ����ָ����socket��ַ�ˡ�
TCP�ͻ������ε���socket()��connect()֮�����TCP������������һ����������
TCP�������������������֮�󣬾ͻ����accept()����ȡ���������������Ӿͽ������ˡ�
֮��Ϳ��Կ�ʼ����I/O�����ˣ�����ͬ����ͨ�ļ��Ķ�дI/O������

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

accept�����ĵ�һ������Ϊ��������socket�����֣��ڶ�������Ϊָ��struct sockaddr *��ָ�룬���ڷ��ؿͻ��˵�Э���ַ��
����������ΪЭ���ַ�ĳ��ȡ����accpet�ɹ�����ô�䷵��ֵ�����ں��Զ����ɵ�һ��ȫ�µ������֣������뷵�ؿͻ���TCP���ӡ�

ע�⣺accept�ĵ�һ������Ϊ��������socket�����֣��Ƿ�������ʼ����socket()�������ɵģ���Ϊ����socket�����֣�
��accept�������ص��������ӵ�socket�����֡�һ��������ͨ��ͨ������ֻ����һ������socket�����֣����ڸ÷�����������������һֱ���ڡ�
�ں�Ϊÿ���ɷ��������̽��ܵĿͻ����Ӵ�����һ��������socket�����֣�������������˶�ĳ���ͻ��ķ�����Ӧ��������socket�����־ͱ��رա�

select : ���أ�����׼�����ļ��������ĸ�������ʱΪ0������Ϊ -1.

ѡ��	���� optval ����	˵��
SO_ACCEPTCONN	int	������Ϣָʾ���׽����Ƿ��ܼ������� getsockopt
SO_BROADCAST	int	��� *optval ��0���㲥���ݰ�
SO_DEBUG	int	��� *optval ��0�����������������Թ���
SO_DONTROUTE	int	��� *optval ��0���ƹ�ͨ��·��
SO_ERROR	int	���ع�����׽��ִ���������� getsockopt
SO_KEEPALIVE	int	��� *optval ��0������������keep-alive��Ϣ
SO_LINGER	struct linger	����δ������Ϣ�����׽��ֹر�ʱ���ӳ�ʱ��
SO_OOBINLINE	int	��� *optval ��0�����������ݷŵ���ͨ������
SO_RCVBUF	int	���ջ��������ֽڴ�С
SO_RCVLOWAT	int	���յ����з��ص�������С�ֽ���
SO_RCVTIMEO	struct timeval	�׽��ֽ��յ��õĳ�ʱֵ
SO_REUSEADDR	int	��� *optval ��0������ bind �еĵ�ַ
SO_SNDBUF	int	���ͻ��������ֽڴ�С
SO_SNDLOWAT	int	���͵����з��͵�������С�ֽ���
SO_SNDTIMEO	struct timeval	�׽��ַ��͵��õĳ�ʱֵ
SO_TYPE	int	��ʶ�׽������ͣ��� getsockopt

*/

/*
inet_aton��inet_network��inet_addr���߱Ƚ�

���߶��壺

int inet_aton(const char *cp, struct in_addr *inp);//���������ֽ���

in_addr_t inet_addr(const char *cp);//������·�ֽ���

in_addr_t inet_network(const char *cp);//���������ֽ���

  inet_addr��inet_network�����������ڽ��ַ�����ʽת��Ϊ������ʽ�õģ����������С��
  inet_addr���ص�������ʽ���� ���ֽ��򣬶�inet_network���ص�������ʽ�������ֽ���
����һ�������ƣ�Ϊʲô������inet_network��ȴ���ص��������ֽ��򣬺� �ǣ�������ô��֣�������ʲô�취�ء��������ط����߲��޶��졣
��������һ��Сȱ�ݣ��Ǿ��ǵ�IP��255.255.255.255ʱ����������С�ӡ� �����������������ǳƣ����½⡭^_^������Ϊ���Ǹ���Ч��IP��ַ��
������ʷ�������⣬��ʵ��Ŀǰ�󲿷ֵ�·�����ϣ���� 255.255.255.255��IP������Ч�ġ�

  inet_aton��������������С�ӵ����������������Ϊ255.255.255.255����Ч�ģ�������ԩ��������������IP��ַ��
  �������� ��������֧�����������������С�ӻ�������Ϊ��:)���ˣ�inet_aton�������ص��������ֽ����IP��ַ��
*/

/*
select��poll��epoll���

IO��·������ָ�ں�һ�����ֽ���ָ����һ�����߶��IO����׼����ȡ������֪ͨ�ý��̡�IO��·�����������³��ϣ�

        ���ͻ�������������ʱ��һ���ǽ���ʽ����������׽ӿڣ�������ʹ��I/O���á�

        ��һ���ͻ�ͬʱ�������׽ӿ�ʱ������������ǿ��ܵģ������ٳ��֡�

        ���һ��TCP��������Ҫ��������׽ӿڣ���Ҫ�����������׽ӿڣ�һ��ҲҪ�õ�I/O���á�

        ���һ����������Ҫ����TCP����Ҫ����UDP��һ��Ҫʹ��I/O���á�

        ���һ��������Ҫ�������������Э�飬һ��Ҫʹ��I/O���á�


�����̺Ͷ��̼߳�����ȣ�I/O��·���ü��������������ϵͳ����С��ϵͳ���ش�������/�̣߳�Ҳ����ά����Щ����/�̣߳��Ӷ�����С��ϵͳ�Ŀ�����

  Ŀǰ֧��I/O��·���õ�ϵͳ������ select��pselect��poll��epoll��I/O��·���þ���ͨ��һ�ֻ��ƣ�
һ�����̿��Լ��Ӷ����������һ��ĳ��������������һ���Ƕ���������д��������
�ܹ�֪ͨ���������Ӧ�Ķ�д��������select��pselect��poll��epoll�����϶���ͬ��I/O��
��Ϊ���Ƕ���Ҫ�ڶ�д�¼��������Լ�������ж�д��Ҳ����˵�����д�����������ģ����첽I/O�������Լ�������ж�д��
�첽I/O��ʵ�ֻḺ������ݴ��ں˿������û��ռ䡣

poll
����ԭ��

    poll�����Ϻ�selectû�����������û���������鿽�����ں˿ռ䣬Ȼ���ѯÿ��fd��Ӧ���豸״̬������豸���������豸�ȴ������м���һ������������������������fd��û�з��־����豸�������ǰ���̣�ֱ���豸��������������ʱ�������Ѻ�����Ҫ�ٴα���fd��������̾����˶����ν�ı�����

��û����������������ƣ�ԭ�������ǻ����������洢�ģ�����ͬ����һ��ȱ�㣺

        ������fd�����鱻���帴�����û�̬���ں˵�ַ�ռ�֮�䣬�����������ĸ����ǲ��������塣

        poll����һ���ص��ǡ�ˮƽ�����������������fd��û�б�������ô�´�pollʱ���ٴα����fd��

ע�⣺

    �����濴��select��poll����Ҫ�ڷ��غ�ͨ�������ļ�����������ȡ�Ѿ�������socket����ʵ�ϣ�ͬʱ���ӵĴ����ͻ�����һʱ�̿���ֻ�к��ٵĴ��ھ���״̬��������ż��ӵ���������������������Ч��Ҳ�������½���

---------------------------------------------------------------
epoll

epoll����2.6�ں�������ģ���֮ǰ��select��poll����ǿ�汾�������select��poll��˵��epoll������û�����������ơ�epollʹ��һ���ļ������������������������û���ϵ���ļ����������¼���ŵ��ں˵�һ���¼����У��������û��ռ���ں˿ռ��copyֻ��һ�Ρ�

����ԭ��

    epoll֧��ˮƽ�����ͱ�Ե�����������ص����ڱ�Ե��������ֻ���߽�����Щfd�ոձ�Ϊ����̬������ֻ��֪ͨһ�Ρ�����һ���ص��ǣ�epollʹ�á��¼����ľ���֪ͨ��ʽ��ͨ��epoll_ctlע��fd��һ����fd�������ں˾ͻ��������callback�Ļص������������fd��epoll_wait������յ�֪ͨ��

epoll���ŵ㣺

        û����󲢷����ӵ����ƣ��ܴ򿪵�FD������Զ����1024��1G���ڴ����ܼ���Լ10����˿ڣ���

        Ч��������������ѯ�ķ�ʽ����������FD��Ŀ������Ч���½���ֻ�л�Ծ���õ�FD�Ż����callback��������Epoll�����ŵ��������ֻ���㡰��Ծ�������ӣ��������������޹أ������ʵ�ʵ����绷���У�Epoll��Ч�ʾͻ�ԶԶ����select��poll��

        �ڴ濽��������mmap()�ļ�ӳ���ڴ�������ں˿ռ����Ϣ���ݣ���epollʹ��mmap���ٸ��ƿ�����

epoll���ļ��������Ĳ���������ģʽ��LT��level trigger����ET��edge trigger����LTģʽ��Ĭ��ģʽ��LTģʽ��ETģʽ���������£�

    LTģʽ����epoll_wait��⵽�������¼������������¼�֪ͨӦ�ó���Ӧ�ó�����Բ�����������¼����´ε���epoll_waitʱ�����ٴ���ӦӦ�ó���֪ͨ���¼���

    ETģʽ����epoll_wait��⵽�������¼������������¼�֪ͨӦ�ó���Ӧ�ó����������������¼�������������´ε���epoll_waitʱ�������ٴ���ӦӦ�ó���֪ͨ���¼���

    LTģʽ

        LT(level triggered)��ȱʡ�Ĺ�����ʽ������ͬʱ֧��block��no-block socket�������������У��ں˸�����һ���ļ��������Ƿ�����ˣ�Ȼ������Զ����������fd����IO����������㲻���κβ������ں˻��ǻ����֪ͨ��ġ�

    ETģʽ

        ET(edge-triggered)�Ǹ��ٹ�����ʽ��ֻ֧��no-block socket��������ģʽ�£�����������δ������Ϊ����ʱ���ں�ͨ��epoll�����㡣Ȼ�����������֪���ļ��������Ѿ����������Ҳ�����Ϊ�Ǹ��ļ����������͸���ľ���֪ͨ��ֱ��������ĳЩ���������Ǹ��ļ�����������Ϊ����״̬��(���磬���ڷ��ͣ����ջ��߽������󣬻��߷��ͽ��յ���������һ����ʱ������һ��EWOULDBLOCK ���󣩡�������ע�⣬���һֱ�������fd��IO����(�Ӷ��������ٴα��δ����)���ں˲��ᷢ�͸����֪ͨ(only once)��

        ETģʽ�ںܴ�̶��ϼ�����epoll�¼����ظ������Ĵ��������Ч��Ҫ��LTģʽ�ߡ�epoll������ETģʽ��ʱ�򣬱���ʹ�÷������׽ӿڣ��Ա�������һ���ļ������������/����д�����Ѵ������ļ������������������

    ��select/poll�У�����ֻ���ڵ���һ���ķ������ں˲Ŷ����м��ӵ��ļ�����������ɨ�裬��epoll����ͨ��epoll_ctl()��ע��һ���ļ���������һ������ĳ���ļ�����������ʱ���ں˻��������callback�Ļص����ƣ�Ѹ�ټ�������ļ��������������̵���epoll_wait()ʱ��õ�֪ͨ��(�˴�ȥ���˱����ļ�������������ͨ�������ص��ĵĻ��ơ�������epoll���������ڡ�)

ע�⣺

    ���û�д�����idle-connection����dead-connection��epoll��Ч�ʲ������select/poll�ߺܶ࣬���ǵ�����������idle-connection���ͻᷢ��epoll��Ч�ʴ�����select/poll��

���ϣ���ѡ��select��poll��epollʱҪ���ݾ����ʹ�ó����Լ������ַ�ʽ�������ص㣺

        �����Ͽ�epoll��������ã��������������ٲ������Ӷ�ʮ�ֻ�Ծ������£�select��poll�����ܿ��ܱ�epoll�ã��Ͼ�epoll��֪ͨ������Ҫ�ܶຯ���ص���

        select��Ч����Ϊÿ��������Ҫ��ѯ������ЧҲ����Եģ������������Ҳ��ͨ�����õ���Ƹ��ơ�

*/

/******************************************************************/
/************************tcp part**********************************/
/******************************************************************/

static int32 netTcpSockInitServer(int32 port, int32 mode, uint8 *ipAddr)
{
  int32 ret = 0;
  int32 listenfd;
  struct sockaddr_in servaddr;
  int32 opt = 1;

  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0) 
  {
    displayErrorMsg("tcp failed to create sock\n");
    return TASK_ERROR; 
  }

  if (mode == NET_MULTI_MODE)
  {
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) // �����׽��ֵ�ַ������
    {
      displayErrorMsg("tcp failed to create sock\n");
      return TASK_ERROR; 
    }
  }
  
  bzero(&servaddr, sizeof(servaddr));

  servaddr.sin_family = PF_INET;
  servaddr.sin_port = htons(port);

  if (ipAddr == NULL)
  {
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  else
  {
    ret = inet_aton(ipAddr, (struct in_addr *)&servaddr.sin_addr.s_addr);
  }
  
  if (servaddr.sin_addr.s_addr == 0)
  {
    LOG_ERROR("error ip address error\n");
    close(listenfd);
    return TASK_ERROR;
  }
  
  ret = bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
  if(ret < 0)
  {
    displayErrorMsg("tcp failed to bind sock\n");
    close(listenfd);
    return TASK_ERROR; 
  }

  ret = listen(listenfd, 1);
  if(ret < 0)
  {
    displayErrorMsg("tcp failed to listen sock\n");
    close(listenfd);
    return TASK_ERROR; 
  }

  LOG_DBG("tcp socket init ok\n");
  return listenfd;
}

/*
FD_ZERO(fd_set *fdset)�����fdset�������ļ��������ϵ��
FD_SET(int fd, fd_set *fdset)�������ļ����fd��fdset����ϵ��
FD_CLR(int fd, fd_set *fdset)������ļ����fd��fdset����ϵ��
FD_ISSET(int fd, fdset *fdset)�����fdset��ϵ���ļ����fd�Ƿ�ɶ�д��>0��ʾ�ɶ�д��

	
#include <sys/select.h>   
    int select(int maxfdp, fd_set *readset, fd_set *writeset, fd_set *exceptset,struct timeval *timeout);
    
  int maxfdp��һ������ֵ����ָ�����������ļ��������ķ�Χ���������ļ������������ֵ��1�����ܴ�
  ��Windows�����������ֵ����ν���������ò���ȷ�� 
    
  �м���������� readset, writset, exceptset,ָ������������
  ��Щ����ָ�������ǹ�����Щ������������Ҫ����ʲô����(��д���ɶ����쳣)��
  һ���ļ������������� fd_set �����С�fd_set���ͱ���ÿһλ������һ����������

���selectģ�ͣ�
���selectģ�͵Ĺؼ��������fd_set,Ϊ˵�����㣬ȡfd_set����Ϊ1�ֽڣ�fd_set�е�ÿһbit���Զ�Ӧһ���ļ�������fd��
��1�ֽڳ���fd_set�����Զ�Ӧ8��fd��
��1��ִ��fd_set set;FD_ZERO(&set);��set��λ��ʾ��0000,0000��
��2����fd��5,ִ��FD_SET(fd,&set);��set��Ϊ0001,0000(��5λ��Ϊ1)
��3�����ټ���fd��2��fd=1,��set��Ϊ0001,0011
��4��ִ��select(6,&set,0,0,0)�����ȴ�
��5����fd=1,fd=2�϶������ɶ��¼�����select���أ���ʱset��Ϊ0000,0011��ע�⣺û���¼�������fd=5����ա�

������������ۣ��������ɵó�selectģ�͵��ص㣺
��1)�ɼ�ص��ļ�����������ȡ����sizeof(fd_set)��ֵ������߷�������sizeof(fd_set)��512��ÿbit��ʾһ���ļ���������
���ҷ�������֧�ֵ�����ļ���������512*8=4096����˵�ɵ�������˵��Ȼ�ɵ����������������ڱ����ں�ʱ�ı���ֵ��
��2����fd����select��ؼ���ͬʱ����Ҫ��ʹ��һ�����ݽṹarray����ŵ�select��ؼ��е�fd��һ��������select���غ�
array��ΪԴ���ݺ�fd_set����FD_ISSET�жϡ�����select���غ�����ǰ����ĵ������¼�������fd��գ�
��ÿ�ο�ʼ selectǰ��Ҫ���´�arrayȡ��fd��һ���루FD_ZERO���ȣ���ɨ��array��ͬʱȡ��fd���ֵmaxfd������select�ĵ�һ��������
��3���ɼ�selectģ�ͱ�����selectǰѭ��array����fd��ȡmaxfd����select���غ�ѭ��array��FD_ISSET�ж��Ƿ���ʱ�䷢������

*/


/*
Linux:C/Socket��·����select Сȫ

ѭ��������:UDP������ 
   socket(...);
   bind(...);
   while(1)
    {
         recvfrom(...);
         process(...);
         sendto(...);
   }

ѭ��������:TCP������ 

   socket(...);
   bind(...);
   listen(...);
   while(1)
   {
           accept(...);
           while(1)
           {
                   read(...);
                   process(...);
                   write(...);
           }
           close(...);
   }

����������:TCP������

  socket(...);
  bind(...);
  listen(...);
  while(1)
  {
        accept(...);
        if(fork(..)==0)
          {
              while(1)
               {        
                read(...);
                process(...);
                write(...);
               }
           close(...);
           exit(...);
          }
        close(...);
  }    

����������:��·����I/O,����
  selectĬ�Ͼ�������ʽ�ģ�ֻ�й�ע����������Чʱ���Żᱻ����ִ����Ӧ�Ĳ���

  ��ʼ��(socket,bind,listen);
        
  while(1)
      {
      ���ü�����д�ļ�������(FD_*);   
      
      ����select;
      
      ����������׽��־���,˵��һ���µ�����������
           { 
              ��������(accept);
              ��д����
              close�½�������fd
           }      
      } 

����������:��·����I/O,������

  ��ʼ��(socket,bind,listen);
        
  while(1)
      {
      ���ü�����д�ļ�������(FD_*);   
      
      ����select;
      
      ����������׽��־���,˵��һ���µ�����������
           { 
              ��������(accept);
              ���뵽�����ļ���������ȥ;//�𵽷�����
           }
        ����˵����һ���Ѿ����ӹ���������
          {
              ���в���(read����write);
           }
                      
      } 

*/

static int32 netTcpSockAccept(int32 listenfd, netFunctionRun function)
{
  struct sockaddr_in clientaddr;
  int32 acceptfd;
  int32 clientLen;
  int32 ret = -1;

  clientLen = sizeof(clientaddr);
  acceptfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientLen);
  if (acceptfd <= 0) 
  {
    LOG_ERROR("tcp failed to acccept sock\n");
    return TASK_ERROR; 
  }

  ret = function(acceptfd, NULL);

  close(acceptfd);

  return NET_TCP_SOCK_SET;
}

/*
netTcpSockLoopServer �������ģ��Ƕ�·��
*/
int32 netTcpSockSelectServer(int32 port, int32 timeDelay, uint8 *ipAddr, netFunctionRun function)
{
  int32 ret;
  fd_set fdsr;
  int32 maxSock;
  struct timeval tv;

  maxSock = netTcpSockInitServer(port, NET_DEFAULT_MODE, ipAddr);
  if (maxSock == TASK_ERROR)
  {
    LOG_ERROR("netTcpSockInitServer error\n");
    return TASK_ERROR; 
  }
    
  while(1)
  {
    FD_ZERO(&fdsr);
    tv.tv_sec = timeDelay;
    tv.tv_usec = 0;
  
    FD_SET(maxSock, &fdsr);

    /*��������*/
    ret = select(maxSock + 1, &fdsr, NULL, NULL, &tv);
    if (ret < 0) 
    {
      LOG_ERROR("tcp failed to select sock\n");
      close(maxSock);
      return TASK_ERROR; 
    } 
    else if (ret == 0) 
    {
      LOG_DBG("tcp sock server timeout\n");
      continue;
    } 

    if (FD_ISSET(maxSock, &fdsr) )
    {
      ret = netTcpSockAccept(maxSock, function);
      if (ret == NET_TCP_SOCK_SET)
      {
        LOG_DBG("tcp sock server continue\n");
        continue;
      }
      else
      {
        LOG_DBG("tcp sock server break\n");
        break;
      }
    }
  }

  close(maxSock);

  return TASK_OK;
}

/*�����൱�ڽ���һ�����ӣ�Ȼ���µ����Ӽ����µ��ļ�����������
Ȼ�󽫾ɵ����ӴӾɵ��ļ���������ȥ��*/
static int32 netTcpSockMultiSelectIOAccept(fd_set fdsets, netServerContext_t *serverFd)
{
  struct sockaddr_in clientaddr;
  int32 acceptfd;
  int32 clientLen;
  int32 ret = -1;
  int32 oldfd= -1;

  clientLen = sizeof(clientaddr);
  acceptfd = accept(serverFd->maxfd, (struct sockaddr *)&clientaddr, &clientLen);
  if (acceptfd <= 0) 
  {
    LOG_ERROR("tcp failed to acccept sock\n");
    return TASK_ERROR; 
  }
  else
  {
    LOG_DBG("��ȡ���ӿͻ���%s : �˿�%d������, �׽���������%d...\n",
                        inet_ntoa(clientaddr.sin_addr),
                        ntohs(clientaddr.sin_port),
                        acceptfd);
  }

  //  �¼��������������û�ж��Ƿ���Ի���д
  //  �������allset�Ķ�Ӧacceptfd��λ���´�ѭ��ʱ���ɼ���acceptfd
  FD_SET(acceptfd, &serverFd->allfds);

  if (acceptfd > serverFd->maxfd)
  {
    oldfd = serverFd->maxfd;
    serverFd->maxfd = acceptfd;
  }

  //  ���˼����׽��ֵ���Ϣ�Ѿ�������, Ӧ���������Ӧλ
  FD_CLR(oldfd, &fdsets);

  if(--serverFd->cliCnt <= 0)  //  nready�������������������Ͳ�Ҫ��������client����
  {
    return NET_TCP_SOCK_SET;
  }

  return NET_TCP_SOCK_CTL;
}

/*���ﴦ��accept fd����ѯ�Ƿ���fd�ı䣬��������г���Ȼ���������������ȥ��fd*/
static int32 netTcpSockSelectMultiIOProc(fd_set fdsets, netServerContext_t *serverFd, netFunctionRun function)
{
  int32 fd;
  
  //  �����׽��ֿ���Щ�ͻ��������׽�������������
  for(fd = 0;fd <= serverFd->maxfd && serverFd->cliCnt> 0;fd++)
  {
    if(FD_ISSET(fd, &fdsets))
    {
      //�����̵Ļ����£��������������������ѡ����������̣߳���ʱ.Ҳ���޷������ܾ�����Ĺ���
      //�Ƚ��ʺ϶����ӵ����
      function(fd, NULL);

      FD_CLR(fd, &serverFd->allfds);                  //�������ʾ�ѱ�����
      
      close(fd);

      fd = -1;
    }
  }
  return TASK_OK;
}

/*�������ȴ���������fdȻ����accept��fd*/
//  ���ȼ������������׽�����û�����ݣ�
//  ����еĻ�˵���������˿ͻ��˵ģ�
//  Ӧ�õ���accept����ȡ�ͻ��˵�����
//
//  ���ż��ͻ��˵������׽�����û����������
//  ����еĻ���˵���ͻ��˸���������ͨ������

/*
accept��Ⱥ��select��ͻ
��socket�е�Ӱ�죬��linux 2.6�汾֮ǰ�İ汾���ڣ���֮��İ汾�н�����ˡ�
1, ���߳����� accept, ����һ�����ӵ���ʱ�������߳̾������ѣ�����ֻ��һ���̻߳������ӣ������̼߳����ظ�˯�ߡ�
  �����Ͽ�������һ���̻߳ᱻ���ѣ��û����ɸ�֪��--��Ⱥ
  tcp/ip����2�� 366ҳ
  accept()
  {
  while(qlen==0)
  tsleep
  ...
  }

������߳�accept, ����tsleep�����𡣵���һ������ʱ�������̵߳�tsleep����0. �������е��̻߳�����ӣ������߳�����qlen==0,����tsleep

2�����߳�����select, ����һ���¼�����ʱ���ỽ�������̣߳�--select��ͻ
  �����Ͽ������������̶߳������ˣ��û��ɸ�֪------��accept��ͬ��accept����ֻ��һ���̷߳��ء�
  ���ֻ��һ���¼��������������̶߳������ˣ�һ����Ч�ʵ��£�һ������ܻ��ͬһ�¼��ظ�������
  iocp, epollò��û�ⷽ�����⡣
  ��˵�µĲ���ϵͳ��accept�������ѽ����
*/

/*
��һ��      ����NULL���βδ��룬��������ʱ��ṹ�����ǽ�select��������״̬��һ���ȵ������ļ�������������ĳ���ļ������������仯Ϊֹ��

�ڶ���      �ڶ�������ʱ��ֵ��Ϊ0��0���룬�ͱ��һ������ķ����������������ļ��������Ƿ��б仯�������̷��ؼ���ִ�У��ļ��ޱ仯����0���б仯����һ����ֵ��

������      timeout��ֵ����0������ǵȴ��ĳ�ʱʱ�䣬��select��timeoutʱ������������ʱʱ��֮�����¼������ͷ����ˣ������ڳ�ʱ�󲻹�����һ�����أ�����ֵͬ������
*/

/*
ע������
       1)  select()�������ܵ�O_NDELAY��Ǻ�O_NONBLOCK��ǵ�Ӱ �죬���socket��������socket�������select()��������select()ʱ��Ч����һ���ģ�socket��Ȼ������ʽTCPͨѶ���� �������socket�Ƿ�������socket����ô����select()ʱ�Ϳ���ʵ�ַ�����ʽTCPͨѶ��

       2)  fd_set��һ��λ���飬���С����Ϊ__FD_SETSIZE��1024����λ�����ÿһλ�������Ӧ���������Ƿ���Ҫ�����

       3)  select()���������Ͽ���������֧���ļ�������������ϵͳƽ̨���� ��(�磺Linux ��Unix ��Windows��MacOS��)������ֲ�Ժ�
*/

int32 netTcpSockSelectMultiIOServer(int32 port, int32 timeDelay, uint8 *ipAddr, netFunctionRun function)
{
  int32 ret;
  fd_set fdsr;
  int maxSock, sockfd;
  struct timeval tv;
  int32 i;
  
  netServerContext_t serverFd;

  serverFd.maxfd = netTcpSockInitServer(port, NET_MULTI_MODE, ipAddr);
  if (serverFd.maxfd == TASK_ERROR)
  {
    LOG_ERROR("netTcpSockInitServer error\n");
    return TASK_ERROR; 
  }
  
  memset(&serverFd, 0,sizeof(netServerContext_t));
  
  sockfd = serverFd.maxfd;
  maxSock = serverFd.maxfd;

  //  allret���ڱ���������־��fd_ret��Ϣ, ��ÿ�δ�����󣬸�ֵ��ret
  // �����ǵ�һ������sockfd�������¼����������accept�д���
  FD_ZERO(&serverFd.allfds);
  FD_SET(sockfd, &serverFd.allfds);
  
  while(1)
  {
    fdsr = serverFd.allfds;//��allset�Ĵ���ʹ���¼����fdҪ�ȵ��´�select�Żᱻ����
    
    /*ÿ�ε���selectǰ��Ҫ���������ļ���������ʱ�䣬��Ϊ�¼��������ļ���������ʱ�䶼���ں��޸���*/
    
    tv.tv_sec = timeDelay;
    tv.tv_usec = 0;
  
    ret = select(maxSock + 1, &fdsr, NULL, NULL, &tv);
    if (ret < 0) 
    {
      LOG_ERROR("tcp failed to select sock\n");
      close(sockfd);
      return TASK_ERROR; 
    } 
    else if (ret == 0) 
    {
      LOG_DBG("tcp sock server timeout\n");
      continue;
    } 
    
    serverFd.cliCnt = ret;

    if (FD_ISSET(sockfd, &fdsr) )
    {
      ret = netTcpSockMultiSelectIOAccept(fdsr, &serverFd);
      if (ret == NET_TCP_SOCK_SET)
      {
        LOG_DBG("tcp sock server continue\n");
        continue;
      }
    }
    /*�µĽ���fd������fdsets��Ȼ���ҵ��Ǹ�fd��������ҪIO*/
    netTcpSockSelectMultiIOProc(fdsr, &serverFd, function);
  }

  close(sockfd);

  return TASK_OK;
}

/*
poll
1.      ͷ�ļ�
# include < sys/ poll. h>

2.      ����˵��
int poll ( struct pollfd * fds, unsigned int nfds, int timeout);

��select()��һ����poll()û��ʹ�õ�Ч����������λ���ļ�������set�����ǲ�����һ�������Ľṹ��pollfd���飬��fdsָ��ָ������顣pollfd�ṹ�嶨�����£�

 

struct pollfd

{

int fd;               �ļ������� 

short events;         �ȴ����¼� 

short revents;        ʵ�ʷ����˵��¼�

} ;

typedef unsigned long   nfds_t;

struct pollfd * fds����һ��struct pollfd�ṹ���͵����飬���ڴ����Ҫ�����״̬��socket��������ÿ�������������֮��ϵͳ����Ҫ���������飬���������ȽϷ��㣻�ر��Ƕ��� socket���ӱȽ϶������£���һ���̶��Ͽ�����ߴ����Ч�ʣ���һ����select()������ͬ������select()����֮��select() ������Ҫ�����������socket���������ϣ�����ÿ�ε���select()֮ǰ�������socket���������¼��뵽�����ļ����У���ˣ�select()�����ʺ���ֻ�������socket���������������poll()�����ʺ��ڴ���socket�������������

    ���������socket������Ϊ��ֵ���������������ļ��ͻᱻ���ԣ�Ҳ���ǲ���Գ�Ա����events���м�⣬��events��ע����¼�Ҳ�ᱻ���ԣ�poll()�������ص�ʱ�򣬻�ѳ�Ա����revents����Ϊ0����ʾû���¼�������

 

���������¼���ǣ�

POLLIN/POLLRDNORM(�ɶ�)��

POLLOUT/POLLWRNORM(��д)��

POLLERR(����)

 

�Ϸ����¼����£�

POLLIN              �����ݿɶ���

POLLRDNORM       ����ͨ���ݿɶ���

POLLRDBAND        ���������ݿɶ���

POLLPRI              �н������ݿɶ���

POLLOUT             д���ݲ��ᵼ��������

POLLWRNORM       д��ͨ���ݲ��ᵼ��������

POLLWRBAND        д�������ݲ��ᵼ��������

POLLMSG SIGPOLL    ��Ϣ���á�

 

���⣬revents���л����ܷ��������¼���

POLLER               ָ�����ļ���������������

POLLHUP             ָ�����ļ������������¼���

POLLNVAL            ָ�����ļ��������Ƿ���

��Щ�¼���events���������壬��Ϊ�����ں��ʵ�ʱ�����ǻ��revents�з��ء�ʹ��poll()��select()��һ�����㲻��Ҫ��ʽ�������쳣������档

 

POLLIN | POLLPRI�ȼ���select()�Ķ��¼���

POLLOUT |POLLWRBAND�ȼ���select()��д�¼���

POLLIN�ȼ���POLLRDNORM |POLLRDBAND��

��POLLOUT��ȼ���POLLWRNORM��

 

����Ƕ�һ���������ϵĶ���¼�����Ȥ�Ļ������԰���Щ�������֮����а�λ������Ϳ����ˣ�

���磺��socket������fd�ϵĶ���д���쳣�¼�����Ȥ���Ϳ�����������

struct pollfd  fds;

fds[nIndex].events=POLLIN | POLLOUT | POLLERR��

 

�� poll()��������ʱ��Ҫ�ж�������socket�������Ϸ������¼���������������

struct pollfd  fds;

���ɶ�TCP��������

if((fds[nIndex].revents & POLLIN) == POLLIN){//��������/����accept()������������}

 

����д��

if((fds[nIndex].revents & POLLOUT) == POLLOUT){//��������}

 

����쳣��

if((fds[nIndex].revents & POLLERR) == POLLERR){//�쳣����}

 

nfds_t nfds�����ڱ������fds�еĽṹ��Ԫ�ص���������

 

timeout����poll��������������ʱ�䣬��λ�����룻

���timeout==0����ô poll() �����������ض���������

���timeout==INFTIM����ôpoll() ������һֱ������ȥ��ֱ��������socket�������ϵĸ���Ȥ���¼��� ���ǲŷ��أ��������Ȥ���¼���Զ����������ôpoll()�ͻ���Զ������ȥ��

3.      ����ֵ:
>0������fds��׼���ö���д�����״̬����Щsocket����������������

==0������fds��û���κ�socket������׼���ö���д���������ʱpoll��ʱ����ʱʱ����timeout���룻���仰˵����������� socket��������û���κ��¼������Ļ�����ôpoll()����������timeout��ָ���ĺ���ʱ�䳤��֮�󷵻أ�

-1�� poll��������ʧ�ܣ�ͬʱ���Զ�����ȫ�ֱ���errno��errnoΪ����ֵ֮һ��

4.      �������
EBADF            һ�������ṹ����ָ�����ļ���������Ч��

EFAULTfds        ָ��ָ��ĵ�ַ�������̵ĵ�ַ�ռ䡣

EINTR            ������¼�֮ǰ����һ���źţ����ÿ������·���

EINVALnfds       ��������PLIMIT_NOFILEֵ��

ENOMEM         �����ڴ治�㣬�޷��������

 

5.      ʵ�ֻ���
poll��һ��ϵͳ���ã����ں���ں���Ϊsys_poll��sys_poll���������κδ���ֱ�ӵ���do_sys_poll��do_sys_poll��ִ�й��̿��Է�Ϊ�������֣�

 

    1)�����û������pollfd���鿽�����ں˿ռ䣬��˿������������鳤����أ�ʱ��������һ��O��n����������һ���Ĵ�����do_sys_poll�а����Ӻ�����ʼ������do_pollǰ�Ĳ��֡�

 

    2)����ѯÿ���ļ���������Ӧ�豸��״̬��������豸��δ���������ڸ��豸�ĵȴ������м���һ�������ѯ��һ�豸��״̬����ѯ�������豸�����û��һ���豸��������ʱ����Ҫ����ǰ���̵ȴ���ֱ���豸�������߳�ʱ�����������ͨ������schedule_timeoutִ�еġ��豸��������̱�֪ͨ�������У���ʱ�ٴα��������豸���Բ��Ҿ����豸����һ����Ϊ���α��������豸��ʱ�临�Ӷ�Ҳ��O��n���������治�����ȴ�ʱ�䡣��ش�����do_poll�����С�

 

    3)������õ����ݴ��͵��û��ռ䲢ִ���ͷ��ڴ�Ͱ���ȴ����е��ƺ��������û��ռ俽�����������ȴ����еȲ����ĵ�ʱ�临�Ӷ�ͬ����O��n��������������do_sys_poll�����е���do_poll�󵽽����Ĳ��֡�

6.      ע������
       1). poll() ���������ܵ�socket�������ϵ�O_NDELAY��Ǻ�O_NONBLOCK��ǵ�Ӱ�����Լ��Ҳ����˵������socket�������Ļ��Ƿ����� �ģ�poll()�����������յ�Ӱ�죻

       2). poll()������ֻ�и���ĵĲ���ϵͳ�ṩ֧��(�磺SunOS��Solaris��AIX��HP�ṩ ֧�֣�����Linux���ṩ֧��)������ֲ�Բ

       3). �������� �ļ�ϵͳ�� �����������ĵ��ļ���������������û�����Ƶ�

*/

static int32 netTcpSockMultiPollIOAccept(struct pollfd *clients, int32 *maxFd)
{
  struct sockaddr_in clientaddr;
  int32 acceptfd;
  int32 clientLen;
  int32 ret = -1;
  int32 oldfd= -1;
  int32 i;

  clientLen = sizeof(clientaddr);
  acceptfd = accept(*maxFd, (struct sockaddr *)&clientaddr, &clientLen);
  if (acceptfd <= 0) 
  {
    LOG_ERROR("tcp failed to acccept sock\n");
    return TASK_ERROR; 
  }
  else
  {
    LOG_DBG("��ȡ���ӿͻ���%s : �˿�%d������, �׽���������%d...\n",
                        inet_ntoa(clientaddr.sin_addr),
                        ntohs(clientaddr.sin_port),
                        acceptfd);
  }

  for (i = 0; i < POLL_MAX; i++) 
  {
    if (clients[i].fd == -1) 
    {
      clients[i].fd = acceptfd;
      clients[i].events = POLLIN | POLLOUT;
      break;
    }
  }

  if (i == POLL_MAX) 
  {
    LOG_DBG("too many connection, more than %d\n", POLL_MAX);
    close(acceptfd);
    return NET_TCP_SOCK_SET;
  }

  if (i > *maxFd)
  {
    *maxFd = i;
  }
  
  return NET_TCP_SOCK_CTL;
}

static int32 netTcpSockPollMultiIOProc(struct pollfd* clients, int32 maxClient, int32 nready, netFunctionRun function)
{
  int32 connfd;
  int32 i;
  int32 ret = -1;
  
  if (nready == 0)
  {
    LOG_ERROR("no ready error\n");
    return TASK_ERROR;
  }

  for (i = 1; i< maxClient; i++)
  {
    connfd = clients[i].fd;

    if (connfd == -1)
    {
      continue;
    }

    if (clients[i].revents & (POLLIN | POLLOUT))
    {
      ret = function(connfd, NULL);
      if (ret == TASK_ERROR)
      {
        close(connfd);
        continue;
      }
    }

    if (--nready <= 0)//û��������Ҫ�����˳�ѭ��
    {
       break;
    }
  }
  
  return TASK_OK;
}

int32 netTcpSockPollMultiIOServer(int32 port, int32 timeDelay, uint8 *ipAddr, netFunctionRun function)
{
  fd_set fdsr;
  int32 maxSock;
  struct timeval tv;
  int32 i;
  struct pollfd clients[POLL_MAX];
  int32 nready;
  
  int32 serverFd = -1;

  serverFd = netTcpSockInitServer(port, NET_MULTI_MODE, ipAddr);
  if (serverFd == TASK_ERROR)
  {
    LOG_ERROR("netTcpSockInitServer error\n");
    return TASK_ERROR; 
  }
  
  clients[0].fd = serverFd;
  clients[0].events = POLLIN | POLLOUT;

  for (i = 1; i< POLL_MAX; i++) 
  {
    clients[i].fd = -1; 
  }
    
  maxSock = serverFd + 1;

  while(1)
  {
    nready = poll(clients, maxSock + 1, timeDelay);
    if (nready < 0)
    {
      LOG_ERROR("poll error\n");
      break; 
    }
    else if (nready == 0)
    {
      LOG_ERROR("time out\n");
      break;
    }
    else
    {
      if (clients[i].revents & (POLLIN | POLLOUT))
      {
        if (netTcpSockMultiPollIOAccept(clients, &maxSock) == NET_TCP_SOCK_SET)
        {
          LOG_DBG("continue loop\n");
          continue;
        }

      }
      
      --nready;
    } 

    netTcpSockPollMultiIOProc(clients, maxSock, nready, function);
  }

  close(serverFd);
}


/*
epoll
epoll��һ�ָ�Ч�Ĺ���socket��ģ�ͣ������select��poll��˵���и��ߵ�Ч�ʺ������ԡ���ͳ��select�Լ�poll��Ч�ʻ���socket���������ε������ʶ����������η����½�����epoll�����ܲ�����socket�������Ӷ��½�����׼��linux-2.4.20�ں˲�֧��epoll����Ҫ��patch��������Ҫ��linux-2.4.32��linux-2.6.10�����ں˰汾����epoll��
1.      ͷ�ļ�

#include <sys/epoll.h>
2.      ����˵��

int epoll_create(int size);

int epoll_ctl(int epfd, int op, int fd, struct epoll_event event);

int epoll_wait(int epfd,struct epoll_event events,int maxevents,int timeout);

 

typedef union epoll_data

{

    void ptr;

    int fd;

    __uint32_t u32;

    __uint64_t u64;

} epoll_data_t;

epoll_data��һ��������,��������Ӧ�ó�����Ա���ܶ����͵���Ϣ:fd��ָ��ȵȡ���������Ӧ�ó���Ϳ���ֱ�Ӷ�λĿ���ˡ�

struct epoll_event

{

    __uint32_t events;    / epoll events /

epoll_data_t data;    / User data variable /

};

 

epoll_event �ṹ�屻����ע��������Ȥ���¼��ͻش���������������¼�������

epoll_data_t �������������津���¼���ĳ���ļ���������ص����ݣ�����һ��client���ӵ���������������ͨ������accept�������Եõ������client��Ӧ��socket�ļ������������԰����ļ�����������epoll_data��fd�ֶ��Ա����Ķ�д����������ļ��������Ͻ��С�

events�ֶ��Ǳ�ʾ����Ȥ���¼��ͱ��������¼������ܵ�ȡֵΪ��

EPOLLIN �� ��ʾ��Ӧ���ļ����������Զ���

EPOLLOUT����ʾ��Ӧ���ļ�����������д��

EPOLLPRI�� ��ʾ��Ӧ���ļ��������н��������ݿɶ�

EPOLLERR�� ��ʾ��Ӧ���ļ���������������

EPOLLHUP����ʾ��Ӧ���ļ����������Ҷϣ�

EPOLLET��  ��ʾ��Ӧ���ļ��������趨Ϊedgeģʽ��

 
3.      ���õ��ĺ�����

epoll������һ��������ϵͳ���ã�������epoll_create/epoll_ctl/epoll_wait����ϵͳ�������
3.1.         epoll_create����

����������int epoll_create(int size)

   

�ú�������һ��epollר�õ��ļ������������еĲ�����ָ�����������������Χ����linux-2.4.32�ں��и���size��С��ʼ����ϣ��Ĵ�С����linux2.6.10�ں��иò������ã�ʹ�ú�����������е��ļ���������������hash����ʵ������һ���ں˿ռ䣬������������ע��socket fd���Ƿ����Լ�������ʲô�¼���

������֮�󣬼ǵ���close()���ر��������������epoll�����
3.2.         epoll_ctl����

����������int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)

�ú������ڿ���ĳ���ļ��������ϵ��¼�������ע���¼����޸��¼���ɾ���¼���

�����selectģ���е�FD_SET��FD_CLR�ꡣ

 

������

epfd���� epoll_create ���ɵ�epollר�õ��ļ���������

 op��Ҫ���еĲ�������ע���¼������ܵ�ȡֵ

        EPOLL_CTL_ADD  ע�ᡢ

        EPOLL_CTL_MOD �޸ġ�

        EPOLL_CTL_DEL   ɾ��

fd���������ļ���������

event��ָ��epoll_event��ָ�룻

����ֵ��������óɹ�����0,���ɹ�����-1
3.3.         epoll_wait����

����������int epoll_wait(int epfd,struct epoll_event events,int maxevents,int timeout)

 

�ú���������ѯI/O�¼��ķ���������ѯ���е�����ӿڣ�����һ�����Զ�����һ������д�ˡ������selectģ���е�select������

һ�����������ѭ���ǵ������̵߳Ļ���������-1���ȣ��������Ա�֤һЩЧ�ʣ�����Ǻ����߼���ͬһ���̵߳Ļ����������0����֤��ѭ����Ч�ʡ�epoll_wait��Χ֮��Ӧ����һ��ѭ�����������е��¼�

 

������

epfd����epoll_create ���ɵ�epollר�õ��ļ���������

epoll_event�����ڻش��������¼������飻

maxevents��ÿ���ܴ�����¼�����

timeout���ȴ�I/O�¼������ĳ�ʱֵ��ms����

          -1������ʱ��ֱ�����¼������Ŵ�����

          0�������ء�

 

����ֵ�����ط����¼�����-1�д���

 
4.      epoll��ETģʽ��LTģʽ

ET��Edge Triggered����LT��Level Triggered������Ҫ������Դ���������ӿ���

eg��

1)�� ��ʾ�ܵ����ߵ��ļ����ע�ᵽepoll�У�

2)�� �ܵ�д����ܵ���д��2KB�����ݣ�

3)�� ����epoll_wait���Ի�ùܵ�����Ϊ�Ѿ������ļ������

4)�� �ܵ����߶�ȡ1KB������

5)�� һ��epoll_wait�������

�����ETģʽ���ܵ���ʣ���1KB�������ٴε���epoll_wait���ò����ܵ����ߵ��ļ�������������µ�����д��ܵ��������LTģʽ��ֻҪ�ܵ��������ݿɶ���ÿ�ε���epoll_wait���ᴥ����

 

��һ�����������ΪETģʽ���ļ���������Ƿ������ġ�

 
5.      epoll��ʵ��

epoll��Դ�ļ���/usr/src/linux/fs/eventpoll.c����module_initʱע��һ���ļ�ϵͳ eventpoll_fs_type���Ը��ļ�ϵͳ�ṩ���ֲ���poll��release������epoll_create���ص��ļ�������Ա�poll�� select���߱�����epoll epoll_wait����epoll�Ĳ�����Ҫͨ������ϵͳ����ʵ�֣�

1)�� sys_epoll_create

2)�� sys_epoll_ctl

3)�� sys_epoll_wait

������Դ�뽲��������ϵͳ���á�

1)�� long sys_epoll_create (int size)

sys_epoll_create(epoll_create��Ӧ���ں˺����������������Ҫ����һЩ׼�����������紴�����ݽṹ����ʼ�����ݲ����շ���һ���ļ�����������ʾ�´���������epoll�ļ������������������Ϊ��һ���̶�ʱ��Ĳ�������ϵͳ������Ҫ�����ļ������inode�Լ�file�ṹ��

��linux-2.4.32�ں��У�ʹ��hash��������ע�ᵽ��epoll���ļ�������ڸ�ϵͳ�����и���size��С����hash�Ĵ�С������Ϊ��С��size����С��2size��2��ĳ�η�����СΪ2��9�η���512�������Ϊ2��17�η� ��128 x 1024����

��linux-2.6.10�ں��У�ʹ�ú������������ע�ᵽ��epoll���ļ������size����δʹ�ã�ֻҪ��������С�

   

epoll����Ϊһ�������ļ�ϵͳ��ʵ�ֵģ����������������������ô���

    (1)���������ں���ά��һЩ��Ϣ����Щ��Ϣ�ڶ��epoll_wait���Ǳ��ֵģ����������ܼ�ص��ļ���������

    (2)��epoll����Ҳ���Ա�poll/epoll;

 

����epoll�������ļ�ϵͳ��ʵ�ֺ����ܷ����޹أ�����׸����

 

2)��  long sys_epoll_ctl(int epfd, int op, int fd, struct epoll_event event)

 

sys_epoll_ctl(epoll_ctl��Ӧ���ں˺���������Ҫ��ȷ����ÿ�ε���sys_epoll_ctlֻ����һ���ļ���������������Ҫ������opΪEPOLL_CTL_ADDʱ��ִ�й��̣�sys_epoll_ctl��һЩ��ȫ�Լ������ep_insert��ep_insert�ｫ ep_poll_callback��Ϊ�ص����������豸�ĵȴ����У��ٶ���ʱ�豸��δ������������ÿ��poll_ctlֻ����һ���ļ������������Ҳ������Ϊ����һ��O(1)������

    ep_poll_callback�����ܹؼ����������ȴ����豸������ϵͳ�ص���ִ������������

    (1)���������豸����������У���һ����������poll�������豸�������ٴ���ѯ�����豸�Ҿ����ߣ�������ʱ�临�Ӷȣ���O��n����O��1��;   

    (2)�����������epoll�ļ�;

 

(1)�� ע���� op = EPOLL_CTL_ADD

ע�������Ҫ������

A����fd���뵽hash����rbtree���У����ԭ���Ѿ����ڷ���-EEXIST��

B����fdע��һ���ص��������ú�������fd���¼�ʱ���ã��ڸú����н�fd���뵽epoll�ľ��������С�

C�����fd��ǰ�Ƿ��Ѿ����������¼�����������У�������뵽epoll�ľ��������У�����epoll_wait��

 

(2)�� �޸��¼� op = EPOLL_CTL_MOD

�޸��¼�ֻ�ǽ��µ��¼��滻�ɵ��¼���Ȼ����fd�Ƿ����������¼�������У�������뵽epoll�ľ��������У�����epoll_wait��

 

(3)�� ɾ����� op = EPOLL_CTL_DEL

��fd��hash��rbtree���������

 

3)�� long sys_epoll_wait(int epfd, struct epoll_event events, int maxevents,int timeout)

 

���epoll�ľ�������Ϊ�գ�����timeout��0������ǰ���̣�����CPU���ȡ�

���epoll�ľ������в��գ������������С��Զ����е�ÿһ���ڵ㣬��ȡ���ļ��Ѵ������¼����ж������Ƿ��������ڴ����¼�������У������Ӧ��epoll_event�ṹcopy���û�events��

 

   sys_epoll_wait������ʵ��ִ�в�������ep_poll�������ú����ȴ������������������epoll�ļ��ĵȴ����У�ֱ�������ѣ�������ep_poll_callback���������������ִ��ep_events_transfer������������û��ռ䡣����ֻ���������豸��Ϣ����������Ŀ�����һ��O(1��������

 

��Ҫע����ǣ���LTģʽ�£��ѷ����������¼�copy���û��ռ�󣬻���Ѷ�Ӧ���ļ����¹ҽӵ��������С�������LTģʽ�£����һ��epoll_waitĳ��socketû��read/write���������ݣ��´�epoll_wait���᷵�ظ�socket�����


6.      ʹ��epoll��ע������

1. ETģʽ��LTģʽ��Ч�����Ƚ��ѿ��ơ�

2. ���ĳ������ڴ����¼����䣬����ҪEPOLL_CTL_MOD����ÿ�ζ�д�󽫸þ��modifyһ������������ȶ��ԣ��ر���ETģʽ��

3. socket�رպ���ý��þ����epoll��delete��EPOLL_CTL_DEL������Ȼepoll�����д�������ʹepoll��hash�Ľڵ������࣬Ӱ������hash���ٶȡ�


*/

/*
EPOLL�¼��ַ�ϵͳ������ת������ģʽ�£�
   Edge Triggered (ET)
   Level Triggered (LT)
������˵��ET, LT�������¼��ַ����ƵĲ�ͬ�����Ǽٶ�һ��������
1. �����Ѿ���һ�������ӹܵ��ж�ȡ���ݵ��ļ����(RFD)��ӵ�epoll������
2. ���ʱ��ӹܵ�����һ�˱�д����2KB������
3. ����epoll_wait(2)���������᷵��RFD��˵�����Ѿ�׼���ö�ȡ����
4. Ȼ�����Ƕ�ȡ��1KB������
5. ����epoll_wait(2)......

Edge Triggered ����ģʽ��
��������ڵ�1����RFD��ӵ�epoll��������ʱ��ʹ����EPOLLET��־����ô�ڵ�5������epoll_wait(2)֮���п��ܻ������Ϊ ʣ������ݻ��������ļ������뻺�����ڣ��������ݷ����˻��ڵȴ�һ������Ѿ��������ݵķ�����Ϣ��ֻ���ڼ��ӵ��ļ�����Ϸ�����ĳ���¼���ʱ�� ET ����ģʽ�Ż�㱨�¼�������ڵ�5����ʱ�򣬵����߿��ܻ�����ȴ����ڴ������ļ����뻺�����ڵ�ʣ�����ݡ�������������У�����һ���¼�������RFD��� �ϣ���Ϊ�ڵ�2��ִ����һ��д������Ȼ���¼������ڵ�3�������١���Ϊ��4���Ķ�ȡ����û�ж����ļ����뻺�����ڵ����ݣ���������ڵ�5������ epoll_wait(2)��ɺ��Ƿ�����ǲ�ȷ���ġ�epoll������ETģʽ��ʱ�򣬱���ʹ�÷������׽ӿڣ��Ա�������һ���ļ������������/���� д�����Ѵ������ļ���������������������������ķ�ʽ����ETģʽ��epoll�ӿڣ��ں������ܱ�����ܵ�ȱ�ݡ�
   i    ���ڷ������ļ����
   ii   ֻ�е�read(2)����write(2)����EAGAINʱ����Ҫ���𣬵ȴ�

Level Triggered ����ģʽ
�෴�ģ���LT��ʽ����epoll�ӿڵ�ʱ�������൱��һ���ٶȱȽϿ��poll(2)���������ۺ���������Ƿ�ʹ�ã�������Ǿ���ͬ����ְ�ܡ���Ϊ ��ʹʹ��ETģʽ��epoll�����յ����chunk�����ݵ�ʱ����Ȼ���������¼��������߿����趨EPOLLONESHOT��־���� epoll_wait(2)�յ��¼���epoll�����¼��������ļ������epoll�������н�ֹ������˵�EPOLLONESHOT�趨��ʹ�ô��� EPOLL_CTL_MOD��־��epoll_ctl(2)�����ļ�����ͳ�Ϊ�����߱����������顣

���Ϸ�����man epoll.

Ȼ����ϸ����ET, LT:

LT(level triggered)��ȱʡ�Ĺ�����ʽ������ͬʱ֧��block��no-block socket.�����������У��ں˸�����һ���ļ��������Ƿ�����ˣ�Ȼ������Զ����������fd����IO����������㲻���κβ������ں˻��ǻ����֪ͨ�� �ģ����ԣ�����ģʽ��̳����������ҪСһ�㡣��ͳ��select/poll��������ģ�͵Ĵ���

ET(edge-triggered)�Ǹ��ٹ�����ʽ��ֻ֧��no-block socket��������ģʽ�£�����������δ������Ϊ����ʱ���ں�ͨ��epoll�����㡣Ȼ�����������֪���ļ��������Ѿ����������Ҳ�����Ϊ�Ǹ��ļ����� �����͸���ľ���֪ͨ��ֱ��������ĳЩ���������Ǹ��ļ�����������Ϊ����״̬��(���磬���ڷ��ͣ����ջ��߽������󣬻��߷��ͽ��յ���������һ����ʱ���� ��һ��EWOULDBLOCK ���󣩡�������ע�⣬���һֱ�������fd��IO����(�Ӷ��������ٴα��δ����)���ں˲��ᷢ�͸����֪ͨ(only once),������TCPЭ���У�ETģʽ�ļ���Ч������Ҫ�����benchmarkȷ�ϡ�

�������������ǻῴ�����û�д�����idle-connection����dead-connection��epoll��Ч�ʲ������ select/poll�ߺܶ࣬���ǵ���������������idle-connection(����WAN�����д��ڴ�������������)���ͻᷢ��epoll��Ч�� ������select/poll��


epoll��linuxϵ�y���µ�̎����B�ӵĸ�Ч��ģ�ͣ� �����ڃɷN��ʽ�£� EPOLLLT��ʽ��EPOLLET��ʽ��

EPOLLLT��ϵ�yĬ�J�� �������@�N��ʽ�£� ��ʽ�OӋ�����׳����}�� �ڽ��Ք����r��ֻҪsocketݔ�뾏���Д��������܉�@��EPOLLIN�ĳ��m֪ͨ�� ͬ���ڰl�͔����r�� ֻҪ�l�;�����ã� �����г��m���g���EPOLLOUT֪ͨ��

�����EPOLLET������һ�N�|�l��ʽ�� ��EPOLLLTҪ��Ч�ܶ࣬ ����ʽ�OӋ����Ҫ��Ҳ��Щ�� ��ʽ�OӋ�����С��ʹ�ã���鹤���ڴ˷N��ʽ�r�� �ڽ��Ք����r�� ����Д���ֻ��֪ͨһ�Σ� ����read�rδ�x�ꔵ�������N��������EPOLLIN��֪ͨ�ˣ� ֱ���´����µĔ������_�r��ֹ�� ���l�͔����r�� ����l�;���δ�MҲֻ��һ��EPOLLOUT��֪ͨ�� ������Ѱl�;������M�ˣ� �ŕ��еڶ���EPOLLOUT֪ͨ�ęC���� �����ڴ˷�ʽ��read��write�r��Ҫ̎��á� ���r�����@�e�� ����������

���ӣ� �����һ��socket��������ӵ��ɂ�epoll�У� ���N��ʹ��EPOLLETģʽ�£� ֻҪǰһ��epoll_wait�r��δ�x�꣬ ���N��һ��epoll_wait�¼��r�� Ҳ���õ��x��֪ͨ�� ��ǰһ���x�����r�£� ��һ��epoll�Ͳ����õ��x�¼���֪ͨ�ˡ�������
*/

/*
��Linux�Ͽ��������������һЩ���ϸ��:poll��epoll
��������2.6�ں˶�epoll����ȫ֧�֣������Ϻܶ�����º�ʾ�����붼�ṩ������һ����Ϣ��ʹ��epoll���洫ͳ�� poll�ܸ��������Ӧ�ô��������ϵ�������������������������������ԭ����͵Ľ��٣������ҽ��Է���һ���ںˣ�2.6.21.1��������poll�� epoll�Ĺ���ԭ��Ȼ����ͨ��һЩ�����������ԱȾ���Ч���� POLL��

��˵poll��poll��selectΪ�󲿷�Unix/Linux����Ա����Ϥ������������ԭ�����ƣ�������Ҳ���������Բ��죬��select������ص��ļ����������������ƣ���������ѡ��poll��˵����
poll��һ��ϵͳ���ã����ں���ں���Ϊsys_poll��sys_poll���������κδ���ֱ�ӵ���do_sys_poll��do_sys_poll��ִ�й��̿��Է�Ϊ�������֣�
1�����û������pollfd���鿽�����ں˿ռ䣬��Ϊ�������������鳤����أ�ʱ��������һ��O��n����������һ���Ĵ�����do_sys_poll�а����Ӻ�����ʼ������do_pollǰ�Ĳ��֡�
2����ѯÿ���ļ���������Ӧ�豸��״̬��������豸��δ���������ڸ��豸�ĵȴ������м���һ�������ѯ��һ�豸��״̬����ѯ�������豸�����û��һ���豸��������ʱ����Ҫ����ǰ���̵ȴ���ֱ���豸�������߳�ʱ�����������ͨ������schedule_timeoutִ�еġ��豸��������̱�֪ͨ�������У���ʱ�ٴα��������豸���Բ��Ҿ����豸����һ����Ϊ���α��������豸��ʱ�临�Ӷ�Ҳ��O��n���������治�����ȴ�ʱ�䡣��ش�����do_poll�����С�
3������õ����ݴ��͵��û��ռ䲢ִ���ͷ��ڴ�Ͱ���ȴ����е��ƺ��������û��ռ俽�����������ȴ����еȲ����ĵ�ʱ�临�Ӷ�ͬ����O��n��������������do_sys_poll�����е���do_poll�󵽽����Ĳ��֡�
EPOLL��
����������epoll����poll/select��ͬ��epoll������һ��������ϵͳ���ã�������epoll_create/epoll_ctl/epoll_wait����ϵͳ������ɣ����潫�ῴ���������ĺô���
������sys_epoll_create(epoll_create��Ӧ���ں˺����������������Ҫ����һЩ׼�����������紴�����ݽṹ����ʼ�����ݲ����շ���һ���ļ�����������ʾ�´���������epoll�ļ������������������Ϊ��һ���̶�ʱ��Ĳ�����
epoll����Ϊһ�������ļ�ϵͳ��ʵ�ֵģ����������������������ô���
1���������ں���ά��һЩ��Ϣ����Щ��Ϣ�ڶ��epoll_wait���Ǳ��ֵģ����������ܼ�ص��ļ���������
2�� epoll����Ҳ���Ա�poll/epoll;
����epoll�������ļ�ϵͳ��ʵ�ֺ����ܷ����޹أ�����׸����
��sys_epoll_create�л��ܿ���һ��ϸ�ڣ�����epoll_create�Ĳ���size���ֽ׶���û������ģ�ֻҪ��������С�

������sys_epoll_ctl(epoll_ctl��Ӧ���ں˺���������Ҫ��ȷ����ÿ�ε���sys_epoll_ctlֻ����һ���ļ���������������Ҫ������opΪEPOLL_CTL_ADDʱ��ִ�й��̣�sys_epoll_ctl��һЩ��ȫ�Լ������ep_insert��ep_insert�ｫ ep_poll_callback��Ϊ�ص����������豸�ĵȴ����У��ٶ���ʱ�豸��δ������������ÿ��poll_ctlֻ����һ���ļ������������Ҳ������Ϊ����һ��O(1)����

ep_poll_callback�����ܹؼ����������ȴ����豸������ϵͳ�ص���ִ������������

1���������豸����������У���һ����������poll�������豸�������ٴ���ѯ�����豸�Ҿ����ߣ�������ʱ�临�Ӷȣ���O��n����O��1��;
2�����������epoll�ļ�;
�����sys_epoll_wait������ʵ��ִ�в�������ep_poll�������ú����ȴ������������������epoll�ļ��ĵȴ����У�ֱ�������ѣ�������ep_poll_callback���������������ִ��ep_events_transfer������������û��ռ䡣����ֻ���������豸��Ϣ����������Ŀ�����һ��O(1��������
����һ�����˹��ĵ��������epoll��EPOLLET�Ĵ��������ش����Ĵ������Կ�������ǰ�һ����ˮƽ����ģʽ���ں����Ĺ��������û�������ֱ���ϲ����������̫��Ӱ�죬����Ȥ�����ѻ�ӭ���ۡ�
POLL/EPOLL�Աȣ�
������poll�Ĺ��̿��Կ�������һ��epoll_create/���ɴ�epoll_ctl/һ��epoll_wait/һ��close��ϵͳ���ù��ɣ�ʵ����epoll��poll�ֳ����ɲ���ʵ�ֵ�ԭ��������Ϊ�����������ʹ��poll���ص㣨����Web����������
1����Ҫͬʱpoll�����ļ�������;
2��ÿ��poll��ɺ�������ļ�������ֻռ���б�poll���������ĺ���һ���֡�
3��ǰ����poll���ö��ļ����������飨ufds�����޸�ֻ�Ǻ�С;
��ͳ��poll�����൱��ÿ�ε��ö�����¯����û��ռ���������ufds����ɺ��ٴ���ȫ�������û��ռ䣬����ÿ��poll����Ҫ�������豸��������һ�μ����ɾ���ȴ����в�������Щ���ǵ�Ч��ԭ��

epoll�����������ϸ�����ǣ�����Ҫÿ�ζ������������ufds��ֻ��ʹ��epoll_ctl��������һС���֣�����Ҫÿ��epoll_wait��ִ��һ�μ���ɾ���ȴ����в���������Ľ���Ļ���ʹ�Ĳ�����ĳ���豸���������������豸������в��ң���Щ�������Ч�ʡ����������Ե�һ�㣬���û���ʹ����˵��ʹ��epoll����ÿ�ζ���ѯ���з��ؽ�����ҳ����еľ������֣�O��n����O��1����������Ҳ��߲��١�

�������ﻹ����һ�㣬�ǲ��ǽ�epoll_ctl�ĳ�һ�ο��Դ�����fd����semctl�����������Щ�������أ��ر����ڼ���ϵͳ���ñȽϺ�ʱ�Ļ����ϡ���������ϵͳ���õĺ�ʱ���⻹�����Ժ������

POLL/EPOLL�������ݶԱȣ�
���ԵĻ�������д�����δ������ֱ�ģ�����������Ŀͻ��ˣ������Ŀͻ��ˣ�������������һ���Ա���ı�׼2.6.11�ں�ϵͳ�ϣ�Ӳ��Ϊ PIII933�������ͻ��˸��������������PC�ϣ�����̨PC�ȷ�������Ӳ������Ҫ�ã���Ҫ�Ǳ�֤�������÷��������أ���̨������ʹ��һ��100M���������ӡ�
���������ܲ�poll�������ӣ������request������ظ�һ��response��Ȼ�����poll��
��Ŀͻ��ˣ�Active Client��ģ�����ɲ����Ļ���ӣ���Щ���Ӳ���ϵķ���������ܻظ���
�����Ŀͻ��ˣ�zombie��ģ��һЩֻ���ӵ�����������Ŀͻ��ˣ���Ŀ��ֻ��ռ�÷�������poll��������Դ��
���Թ��̣�����10����������ӣ����ϵĵ�������������������¼�ڲ�ͬ������ʹ��poll��epoll�����ܲ�𡣽����������������ݱ����ֱ��ǣ�0��10��20��40��80��160��320��640��1280��2560��5120��10240��
��ͼ�к����ʾ����������������������֮�ȣ������ʾ���40000������ظ������ѵ�ʱ�䣬����Ϊ��λ����ɫ������ʾpoll���ݣ���ɫ��ʾ epoll���ݡ����Կ�����poll������ص��ļ���������������ʱ�����ʱ��������������epoll��ά����һ��ƽ�ȵ�״̬��������������������Ӱ�졣
�ڼ�ص����пͻ��˶��ǻʱ��poll��Ч�ʻ��Ը���epoll����Ҫ��ԭ�㸽������������������Ϊ0ʱ��ͼ�ϲ��׿�������������epollʵ�ֱ�poll���ӣ���������������������ĳ�����

*/
static int32 netTcpSockEpollAccept(int32 listenfd)
{
  struct sockaddr_in clientaddr;
  int32 acceptfd;
  int32 clientLen;
  int32 ret = -1;

  clientLen = sizeof(clientaddr);
  acceptfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientLen);
  if (acceptfd <= 0) 
  {
    LOG_ERROR("tcp failed to acccept sock\n");
    return TASK_ERROR; 
  }

  return acceptfd;
}


int32 netTcpSockEpollServer(int32 port, int32 timeDelay, uint8 *ipAddr, netFunctionRun function)
{
  int32 epfd;
  int32 listenfd;
  int32 acceptFd;
  struct timeval tv; 
  int32 nfds;
  int32 i;
  
  //����epoll_event�ṹ��ı���,ev����ע���¼�,�������ڻش�Ҫ������¼�
  struct epoll_event ev,events[EPOLL_MAX];

  epfd=epoll_create(EPOLL_MAX_FD_NUM); //����epoll���

  listenfd = netTcpSockInitServer(port, NET_DEFAULT_MODE, ipAddr);;
  if (listenfd == TASK_ERROR)
  {
    LOG_ERROR("netTcpSockInitServer error\n");
    return TASK_ERROR; 
  }

  //������Ҫ������¼���ص��ļ�������
  ev.data.fd=listenfd;
  //����Ҫ������¼�����
  ev.events=EPOLLIN | EPOLLOUT;
  //ע��epoll�¼�
  epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);

  while(1)
  {
    //�ȴ�epoll�¼��ķ���
    nfds=epoll_wait(epfd,events,EPOLL_MAX,EPOLL_MAX_FD_NUM);

    for (i = 0; i < nfds;i++)
    {
      if(events[i].data.fd==listenfd)
      {
        acceptFd = netTcpSockEpollAccept(listenfd);
        if (acceptFd > 0)
        {
          //�������ڶ��������ļ�������
          ev.data.fd=acceptFd;
          //��������ע��Ķ������¼�
          ev.events=EPOLLIN|EPOLLET;
          //ע��event
          epoll_ctl(epfd,EPOLL_CTL_ADD,acceptFd,&ev);
        }
      }
      else if(events[i].events&(EPOLLIN | EPOLLOUT))
      {
         function(events[i].data.fd, NULL);
         close(events[i].data.fd);
      }
      else
      {
        LOG_ERROR("epoll event error\n");
      }
    }
  }
}

int32 netTcpSockClient(int32 port, int32 mode, uint8 *ipAddr, netFunctionRun function)
{
  int32 clientSock;
  struct sockaddr_in serverAddr;
  int32 ret = -1;
  int32 len;
  
  clientSock = socket(AF_INET,SOCK_STREAM,0);
  if(clientSock < 0)
  {
    LOG_ERROR("client socket error\n");
    return TASK_ERROR;
  }
  
  LOG_DBG("client socket create successfully.\n");

  memset(&serverAddr,0,sizeof(serverAddr));
  
  serverAddr.sin_family=AF_INET;
  serverAddr.sin_port=htons(port);
  if (ipAddr == NULL)
  {
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  else
  {
    inet_aton(ipAddr, (struct in_addr *)&serverAddr.sin_addr.s_addr);
  }

  if (serverAddr.sin_addr.s_addr == 0)
  {
    LOG_ERROR("error ip address error\n");
    close(clientSock);
    return TASK_ERROR;
  }
  
  len = sizeof(serverAddr);

  ret = connect(clientSock, (struct sockaddr *)&serverAddr, len);
  if (ret < 0)
  {
    LOG_ERROR("client socket connect error\n");
    close(clientSock);
    return TASK_ERROR;
  }

  function(clientSock, NULL);
  close(clientSock);

  return TASK_OK;
}


/******************************************************************/
/************************udp part**********************************/
/******************************************************************/

/*****
�ͻ��˺ͷ�����֮��Ĳ�����ڷ���������ʹ��bind()�������������ı���UDP�˿ڣ����ͻ�������Բ����а󶨣�ֱ�ӷ��͵���������ַ��ĳ���˿ڵ�ַ��
��TCP���������Ƚϣ�UDPȱ����connect()��listen()��accept()��������������UDPЭ�������ӵ����ԣ�����ά��TCP�����ӡ��Ͽ���״̬��

    UDPЭ��ķ������˳�����Ƶ����̷�Ϊ�׽��ֽ������׽������ַ�ṹ���а󶨡��շ����ݡ��ر��׽��ֵȹ��̣�
�ֱ��Ӧ�ں���socket()��bind()��sendto()��recvfrom()��close()��

    UDPЭ��ķ������˳�����Ƶ����̷�Ϊ�׽��ֽ������շ����ݡ��ر��׽��ֵȹ��̣��ֱ��Ӧ�ں���socket()��sendto()��recvfrom()��close()��

*****/
/**
UDPЭ���������еļ�������

1 UDP���Ķ�ʧ����
  ����UDPЭ����������շ���ʱ���ھ�������һ����������ݵĽ��շ����ܽ��յ����ͷ������ݣ���������˫���������������ϣ����򲻻ᷢ�����ղ������ݵ������
·����Ҫ��ת�������ݽ��д洢�������Ϸ����ж���ת���Ȳ��������׳��ִ������Ժܿ�����·����ת���Ĺ����г������ݶ�ʧ������
��UDP�����ݱ��Ķ�ʧ��ʱ�򣬺���recvfrom()��һֱ������ֱ�����ݵ�����

UDP���Ķ�ʧ�ĶԲ�
UDPЭ���е����ݱ��Ķ�ʧ�������Եģ���ΪUDP�������ӵġ����ܱ�֤�������ݵ���ȷ���������ݱ����ھ���·������ʱ�򣬱�·����������������C������S��Գ�ʱ�����ݽ����ط���

2 UDP���ݷ����е�����

  UDP�����ݰ��������ϴ����ʱ���п���������ݵ�˳����ģ����շ�������˳��ͷ��ͷ�������˳�����˵ߵ���
����Ҫ������·�ɵĲ�ͬ��·�ɵĴ洢ת����˳��ͬ��ɵġ�

UDP����ĶԲ�
��������Ľ���������Բ��÷��Ͷ������ݶ��м������ݱ���ŵķ������������ն˶Խ��յ����ݵ�ͷ�˽��м򵥵ش���Ϳ������»��ԭʼ˳�������

3 UDPȱ����������
   UDPЭ��û��TCPЭ�������еĻ������ڸ���������ݵ�ʱ��ֱ�ӽ����ݷŵ��������С�
����û�û�м�ʱ�شӻ������н����ݸ��Ƴ��������浽�������ݻ�����򻺳����з��롣
������������ʱ�򣬺��浽�������ݻḲ��֮ǰ�����ݶ�������ݵĶ�ʧ��

�Բ�: ���û�����

**/

/**
UDPЭ���е�connect()����
UDPЭ����׽����������ڽ����������շ�֮�󣬲���ȷ���׽���������������ʾ�ķ��ͷ����߽��շ��ĵ�ַ���������ȷ�����صĵ�ַ��
����ͻ��˵��׽����������ڷ�������֮ǰ��ֻҪȷ��������ȷ�Ϳ����ˣ��ڷ��͵�ʱ���ȷ������Ŀ�ķ��ĵ�ַ��
������bind()����Ҳ�������˱��ؽ��н��յĵ�ַ�Ͷ˿ڡ�

connect()������TCPЭ���лᷢ���������֣�����һ�����������ӣ�һ�㲻����UDP����UDPЭ����ʹ��connect()���������ý�����ʾȷ������һ���ĵ�ַ����û�������ĺ��塣

connect()������UDPЭ����ʹ�ú��������µĸ����ã�

ʹ��connect()�������׽��ֺ󣬷��Ͳ���������ʹ��sendto()������Ҫʹ��write()����ֱ�Ӳ����׽����ļ�������������ָ��Ŀ�ĵ�ַ�Ͷ˿ںš�

ʹ��connect()�������׽��ֺ󣬽��ղ���������ʹ��recvfrom()������Ҫʹ��read()��ĺ������������᷵�ط��ͷ��ĵ�ַ�Ͷ˿ںš�

��ʹ�ö��connect()������ʱ�򣬻�ı�ԭ���׽��ְ󶨵�Ŀ�ĵ�ַ�Ͷ˿ںţ����°󶨵ĵ�ַ�Ͷ˿ںŴ��棬ԭ�еİ�״̬��ʧЧ������ʹ�������ص����Ͽ�ԭ�������ӡ�

������һ��ʹ��connect()���������ӣ��ڷ�������֮ǰ�����׽����ļ���������Ŀ�ĵ�ַʹ��connect()���������˰󶨣�
֮��ʹ��write()�����������ݲ�ʹ��read()�����������ݡ�

01  static void udpclie_echo(int s, struct sockaddr*to)
02  {
03      char buff[BUFF_LEN] = "UDP TEST";          ��������˷��͵�����
04      connect(s, to, sizeof(*to));               ����
05
06      n = write(s, buff, BUFF_LEN);              ��������
07
08      read(s, buff, n);                          ��������
09  }

**/

/***
UDP�ͻ��˱�̿��

1. �����׽����ļ���������socket()��

2. ���÷�������ַ�Ͷ˿ڣ�struct sockaddr��

3. ��������������ݣ�sendto()��

4. ���շ����������ݣ�recvfrom()��

5. �ر��׽��֣�close()��
***/
int32 netUdpSockClient(int32 port, uint8 *ipAddr, netFunctionRun function)
{
	struct sockaddr_in serverAddr;
	int32 clientSock;
	int32 len;
  
  if ((clientSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
  {
    LOG_ERROR("udp client socket connect error\n");
    return TASK_ERROR;

  } 
  
  memset(&serverAddr, 0, sizeof(struct sockaddr_in));

  LOG_DBG("client socket create successfully.\n");

  serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

  if (ipAddr == NULL)
  {
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  else
  {
    inet_aton(ipAddr, (struct in_addr *)&serverAddr.sin_addr.s_addr);
  }
  
  if (serverAddr.sin_addr.s_addr == 0)
  {
    LOG_ERROR("error ip address error\n");
    close(clientSock);
    return TASK_ERROR;
  }

  len = sizeof(serverAddr);

  function(clientSock, &serverAddr);

  close(clientSock);
  
  return TASK_OK;
}

/***
UDP��������̿��
1. �����׽����ļ���������ʹ�ú���socket()�������׽����ļ�������
2. ���÷�������ַ�������˿ڣ���ʼ��Ҫ�󶨵������ַ�ṹ
3. �������˿ڣ�ʹ��bind()���������׽����ļ���������һ����ַ���ͱ������а�
4. ���տͻ��˵����ݣ�ʹ��recvfrom()�������տͻ��˵��������ݡ�
5. ��ͻ��˷������ݣ�ʹ��sendto()����������������������ݡ�
6. �ر��׽��֣�ʹ��close()�����ͷ���Դ��
***/

int32 netUdpSockServer(int32 port, uint8 *ipAddr, netFunctionRun function)
{
	struct sockaddr_in serverAddr;
	int32 serverSock;
	int32 len;
  
  if ((serverSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
  {
    LOG_ERROR("udp client socket connect error\n");
    return TASK_ERROR;

  } 
  
  memset(&serverAddr, 0, sizeof(struct sockaddr_in));

  LOG_DBG("client socket create successfully.\n");

  serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

  if (ipAddr == NULL)
  {
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  else
  {
    inet_aton(ipAddr, (struct in_addr *)&serverAddr.sin_addr.s_addr);
  }

  if ((bind(serverSock, (struct sockaddr *) &serverAddr, sizeof(serverAddr))) < 0) 
	{
    LOG_ERROR("udp client socket bind error\n");
    close(serverSock);
    return TASK_ERROR;
	} 
  else
  { 
		LOG_DBG("bind address to socket.\n\r");
  }
  
  function(serverSock, &serverAddr);
  
  close(serverSock);

  return TASK_OK;
}

/*
TCP ֻ֧�ֵ���
IPv6 ��֧�ֹ㲥
IPv4 �ǿ�ѡ��
�㲥�ͶಥҪ������UDP��ԭʼIP


*/

/*
���û������BLOADCASEѡ��Ĳ����͡� ���bind�˿ڲ�ƥ�䲻���͸��׽ӿ� 
����󶨵Ĳ���INADDR_ANY��,��ôBIND�ĵ�ַ��Ŀ�ĵ�ַƥ����ܵ���:Ҳ����˵�����BINDһ���㲥��ַ���߰�INADDR_ANY
��ifconfig�������disable��������BROADCAST��־�����䲻�ܽ�����̫���㲥��
Ҳ����ʹ��ioctl��SIOCSIFFLAGS����ȥ��һ���ӿڵı�־IFF_BROADCAST��ʹ֮���ܽ�����̫���㲥��
*/
int32 netUdpBroadcastSend(int32 port, uint8 *ipAddr, netFunctionRun function)
{
	struct sockaddr_in serverAddr;
	int32 sock;
	int32 len;
  int32 opt;
  int32 ret = -1;
  
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
  {
    LOG_ERROR("udp socket connect error\n");
    return TASK_ERROR;
  } 
  
  memset(&serverAddr, 0, sizeof(struct sockaddr_in));

  opt = 1;
  ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));	
  if (ret < 0)
  {
    LOG_ERROR("udp socket opt error\n");
    close(sock);
    return TASK_ERROR;
  }
  
  LOG_DBG("client socket create successfully.\n");

  serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

  if (ipAddr == NULL)
  {
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  else
  {
    inet_aton(ipAddr, (struct in_addr *)&serverAddr.sin_addr.s_addr);
  }
  
  if (serverAddr.sin_addr.s_addr == 0)
  {
    LOG_ERROR("error ip address error\n");
    close(sock);
    return TASK_ERROR;
  }

  len = sizeof(serverAddr);

  function(sock, &serverAddr);

  close(sock);
  
  return TASK_OK;
}

int32 netUdpBroadcastRev(int32 port, netFunctionRun function)
{
  struct sockaddr_in s_addr;
	int32 sock;
  
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
  {
    LOG_ERROR("udp socket create error\n");
    return TASK_ERROR;
  } 

  memset(&s_addr, 0, sizeof(struct sockaddr_in));
  
  s_addr.sin_family = AF_INET;							
	s_addr.sin_port = htons(port);	
	s_addr.sin_addr.s_addr = INADDR_ANY;

  if ((bind(sock, (struct sockaddr *) &s_addr, sizeof(s_addr))) < 0) 
	{
    LOG_ERROR("udp client socket bind error\n");
    close(sock);
    return TASK_ERROR;
	} 
  else
  { 
		LOG_DBG("bind address to socket.\n\r");
  }

  function(sock, &s_addr);
  
  close(sock);

  return TASK_OK;
}


/*
  �鲥�ṩ���������н���һ�Զ�ķ��͵Ļ��ƣ��鲥��������һ�������ڣ�Ҳ�����ǿ����εģ�������������Ҫ��������·�����������豸֧���鲥��

  Hosts�������κ�ʱ���������뿪�鲥�飬�����鲥��ĳ�Աû������λ�õ����ƣ�Ҳû�����������ƣ�
D�໥������ַ�������鲥�ģ�224.0.0.0 - 239.255.255.255��

ͨ��������Socket��̿���ʵ���鲥���ݵķ��ͺͽ��ա��鲥����ֻ��ͨ��һ������ӿڷ��ͣ���ʹ�豸���ж������ӿڡ�

�鲥��һ�Զ�Ĵ�����ƣ�����ͨ���������ӵ�Socketʵ���鲥��

�����鲥��������ݣ��ɲο��鲥��IGMPЭ����ص����¡�

������SOCK_DGRAM���͵�socket�Ժ�ͨ������setsockopt()���������Ƹ�socket���鲥��
����ԭ�ͣ�getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)������IPPROTO_IP level��optval������ѡ��
IP_ADD_MEMBERSHIP������ָ�����鲥�顣
IP_DROP_MEMBERSHIP���뿪ָ�����鲥�顣
IP_MULTICAST_IF��ָ�������鲥���ݵ�����ӿڡ�
IP_MULTICAST_TTL�����������鲥����ʱ��TTL��Ĭ����1��
IP_MULTICAST_LOOP�������鲥���ݵ������Ƿ���Ϊ�����鲥���ݵ��鲥��Ա��
������������Ӹ����˷��ͺͽ����鲥���ݵ�ʵ�֣����պͷ����鲥���ݵĲ�����������ġ�


�鲥server�������鲥���ݵ�����

��������:
ʵ���鲥���ݰ����͵Ĳ������£�
����AF_INET, SOCK_DGRAM��socket��
���鲥IP��ַ�Ͷ˿ڳ�ʼ��sockaddr_in�������ݡ�
IP_MULTICAST_LOOP�����ñ����Ƿ���Ϊ�鲥���Ա�������ݡ�
IP_MULTICAST_IF�����÷����鲥���ݵĶ˿ڡ�
�����鲥���ݡ�

�ͻ�����:
����AF_INET, SOCK_DGRAM���͵�socket��
�趨 SO_REUSEADDR��������Ӧ�ð�ͬһ�����ض˿ڽ������ݰ���
��bind�󶨱��ض˿ڣ�IPΪINADDR_ANY���Ӷ��ܽ����鲥���ݰ���
���� IP_ADD_MEMBERSHIP�����鲥�飬�����ÿ���˿ڲ��� IP_ADD_MEMBERSHIP��
�����鲥���ݰ���

ע��:
 �����鲥������˿���Ҫ�趨һ��IP��ַ���ҵ��Եļ�����������˿ڣ����ڵڶ����˿��Ͻ����鲥����ʼû���趨����˿ڵ�IP��ַ��
 ֻ�Ǹ������鲥·�ɵ��ڶ����˿ڣ���������ղ������ݣ���������һ��IP��ַ��ok��

������ֱ��Դ������鲥��ʽ���������ڽ����鲥��ʱ��Ҫע��һ�����㣺

  1������Ϊ�����鲥�����������鲥·�ɣ�����Ҫ��eth0�����Ͻ���239.10.10.100��5123���鲥����Ҫ����鲥·��239.10.10.0

   route -add net 239.10.10.0 netmask 255.255.255.0 dev eth0

  2��Ҫȷ������������ǽ�ǹرյģ�

�鿴����ǽ״̬  service iptables status

�����رշ���ǽ   service iptables stop |start

�鿴����ǽ�Ƿ񿪻�������chkconfig
 iptables --list

���3��5��ON�Ļ��Ǿ��ǿ����Զ�

  3�������������鲥��ʱ�򣬳�������˵��ע�������⣬��Ҫ����������ã���Ϊ����Ҫ�����鲥�������ȼ����鲥��
�����鲥��Ҫ�������ⷢ�����ݰ������������ò���ȷ���Ǽ����鲥�ı��Ͳ��ܵ�������鲥�Ľ�������������Ҳ�Ͳ�֪����������Ҫ���鲥��
��Ȼ������鲥����������

*/

int32 netUdpGroupBroadcastClient(int32 port, uint8 *ipAddr, netFunctionRun function)
{
	struct sockaddr_in myAddr;
	int32 sock;
	int32 len;
  int32 opt;
  int32 ret = -1;
  
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
  {
    LOG_ERROR("udp socket connect error\n");
    return TASK_ERROR;
  } 
  
  memset(&myAddr, 0, sizeof(struct sockaddr_in));
  
  LOG_DBG("group send socket create successfully.\n");

  myAddr.sin_family = AF_INET;
	myAddr.sin_port = htons(port);
  inet_aton(ipAddr, (struct in_addr *)&myAddr.sin_addr.s_addr);

  if (myAddr.sin_addr.s_addr == 0)
  {
    LOG_ERROR("error ip address error\n");
    close(sock);
    return TASK_ERROR;
  }
  
  /* ���Լ��Ķ˿ں�IP��Ϣ��socket�� */ 
  if (bind (sock, (struct sockaddr *) &myAddr, sizeof (struct sockaddr_in)) < 0)
  {     
    LOG_ERROR("bind ip address error\n");
    close(sock);
    return TASK_ERROR;
  }

  function(sock, &myAddr);

  close(sock);
  
  return TASK_OK;
}

/*
һ��������Ҫ��ʹ��gethostbyname,�õ�����������Ϣ��Ȼ��ʹ��socket(AF_INET,SOCK_DGRAM,0)�����׽��֣�
���ǽ��ŵ��� setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&share,sizeof(share)),
���У�char share = 1, sockfd��socket�������׽��֣���һ�����������˶���̹���ͬһ���˿ڡ�
���ţ���ͨ�õ�bzero(), ��sockaddr_in������Ϣ��bind(),����������Ҫ֪ͨLinux kernel���������ǹ㲥���ݣ�
��һ��ͨ����optval��ֵ���㶨���� optval.imr_multiaddr.s_addr = inet_addr(\"224.0.0.1\"); 
optval.imr_interface.s_addr = htonl(INADDR_ANY); �������һ����������ʹ�Լ�����������һ���㲥�飺
setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &optval, sizeof(command)); 
���ڣ������ʹ��recvfrom()�����նಥ�����ˣ���Ȼ������㻹Ҫʹ�ã� 
setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &optval, sizeof(optval)���� ���˳��ಥ�顣 
*/
int32 netUdpGroupBroadcastServer(int32 port, uint8 *ipGroupAddr, uint8 *peerAddr, netFunctionRun function)
{
	struct sockaddr_in serverAddr;
	int32 sock;
	int32 len;
  int32 ret = -1;
  struct in_addr ia;
  struct hostent *group;
	struct ip_mreq mreq;
  
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
  {
    LOG_ERROR("udp socket connect error\n");
    return TASK_ERROR;
  } 
  
  memset(&serverAddr, 0, sizeof(struct sockaddr_in));
  
  if ((group = gethostbyname(ipGroupAddr)) == (struct hostent *) 0) 
  {
    perror("gethostbyname");
    LOG_ERROR("udp socket opt error\n");
    close(sock);
    return TASK_ERROR;
  }

	bcopy((void *) group->h_addr, (void *) &ia, group->h_length);
	/* �������ַ */
	bcopy(&ia, &mreq.imr_multiaddr.s_addr, sizeof(struct in_addr));

  /* ���÷����鲥��Ϣ��Դ�����ĵ�ַ��Ϣ */
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  
  /* �ѱ��������鲥��ַ��������������Ϊ�鲥��Ա��ֻ�м���������յ��鲥��Ϣ */
  ret = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));	
  if (ret < 0)
  {
    LOG_ERROR("udp socket opt error\n");
    close(sock);
    return TASK_ERROR;
  }


  serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
  inet_aton(peerAddr, (struct in_addr *)&serverAddr.sin_addr.s_addr);
  
  if (serverAddr.sin_addr.s_addr == 0)
  {
    LOG_ERROR("error ip address error\n");
    close(sock);
    return TASK_ERROR;
  }

	if (bind(sock, (struct sockaddr *) &serverAddr,sizeof(struct sockaddr_in)) < 0) 
	{
    LOG_ERROR("bind ip address error\n");
    close(sock);
    return TASK_ERROR;
	}

  function(sock, &serverAddr);

  close(sock);
  
  return TASK_OK;
}

int32 netUdpRev(int32 sock, uint8 *ipAddr, uint8 *buff)
{
  struct sockaddr_in addr;
  socklen_t addrLen;
  int32 len;
  uint8 *str = NULL;
  
  if (sock < 0)
  {
    LOG_ERROR("udp socket fd error\n");
    return TASK_ERROR;
  }
  
  addrLen = sizeof(addr);

  len = recvfrom(sock, buff, sizeof(buff) - 1, 0,(struct sockaddr *) &addr, &addrLen);
	if (len < 0) 
	{
    LOG_ERROR("udp socket rev error\n");
    close(sock);
    return TASK_ERROR;
	}

  LOG_DBG("recive come from %s:%d\n\r",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

  str = inet_ntoa(addr.sin_addr);
  if (str != NULL && (sizeof(ipAddr) >= strlen(str)))
  {
    strcpy(ipAddr, str);
  }
  
  return len;
}

int32 netUdpSend(int32 sock, struct sockaddr_in addr, uint8 *buff, int32 bufLen)
{
  socklen_t addrLen;
  int32 len;
  uint8 *str = NULL;
  
  if (sock < 0)
  {
    LOG_ERROR("udp socket fd error\n");
    return TASK_ERROR;
  }
  
  addrLen = sizeof(addr);

  len = sendto(sock, buff, bufLen, 0,(struct sockaddr *) &addr, addrLen);
	if (len < 0) 
	{
    LOG_ERROR("udp socket send error\n");
    close(sock);
    return TASK_ERROR;
	}

  LOG_DBG("send success\n\r");
  
  return len;
}

/*
Linux�µ�IPC��UNIX Domain Socket 
 һ�� ����

UNIX Domain Socket����socket�ܹ��Ϸ�չ����������ͬһ̨�����Ľ��̼�ͨѶ��IPC����������Ҫ��������Э��ջ������Ҫ������������У��͡�ά����ź�Ӧ��ȣ�ֻ�ǽ�Ӧ�ò����ݴ�һ�����̿�������һ�����̡�UNIX Domain Socket��SOCK_DGRAM��SOCK_STREAM���ֹ���ģʽ��������UDP��TCP������������Ϣ��UNIX Domain SocketҲ�ǿɿ��ģ���Ϣ�Ȳ��ᶪʧҲ����˳����ҡ�

UNIX Domain Socket����������û����Ե��ϵ�Ľ��̣���ȫ˫���ģ���Ŀǰʹ����㷺��IPC���ƣ�����X Window��������GUI����֮�����ͨ��UNIX Domain SocketͨѶ�ġ�

������������

UNIX Domain socket������socket���ƣ�����������socket�Ա�Ӧ�á�

�������߱�̵Ĳ�ͬ���£�

    address familyΪAF_UNIX
    ��ΪӦ����IPC������UNIXDomain socket����ҪIP�Ͷ˿ڣ�ȡ����֮�����ļ�·������ʾ�������ַ������������������������档
    ��ַ��ʽ��ͬ��UNIXDomain socket�ýṹ��sockaddr_un��ʾ����һ��socket���͵��ļ����ļ�ϵͳ�е�·�������socket�ļ���bind()���ô������������bind()ʱ���ļ��Ѵ��ڣ���bind()���󷵻ء�
    UNIX Domain Socket�ͻ���һ��Ҫ��ʽ����bind����������������socketһ������ϵͳ�Զ�����ĵ�ַ���ͻ���bind��socket�ļ������԰����ͻ��˵�pid�������������Ϳ������ֲ�ͬ�Ŀͻ��ˡ�

UNIX Domain socket�Ĺ������̼������£�������socket��ͬ����

�������ˣ�����socket�����ļ����˿ڣ������������ܿͻ������ӡ�����/�������ݡ������ر�

�ͻ��ˣ�����socket�����ļ����˿ڣ������ӡ�����/�������ݡ������ر�

���������ͷ�������SOCK_STREAM��ʽ��

��д���������ֲ�����ʽ�������ͷ�������

1.����ģʽ��

����ģʽ�£��������ݷ��ͽ������ݷ��ı��������ͬ�����ܵ����μ��������¡�Linux�µ�IPC�������ܵ���ʹ�ã�http://blog.csdn.net/guxch/article/details/6828452����

2.������ģʽ

��send��recv�����ı�־����������MSG_DONTWAIT�����ͺͽ��ն��᷵�ء����û�гɹ����򷵻�ֵΪ-1��errnoΪEAGAIN �� EWOULDBLOCK�� 
*/

static int32 netUnixSockInitServer(uint8 *fileDir)
{
  int32 ret = 0;
  int32 listenfd;
  struct sockaddr_un servaddr;

  listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (listenfd < 0) 
  {
    displayErrorMsg("unix socket failed to create sock\n");
    return TASK_ERROR; 
  }

  bzero(&servaddr, sizeof(servaddr));

  servaddr.sun_family = AF_UNIX;
  strcpy(servaddr.sun_path, fileDir);
  
  ret = bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
  if(ret < 0)
  {
    displayErrorMsg("unix socket failed to bind sock\n");
    close(listenfd);
    return TASK_ERROR; 
  }

  ret = listen(listenfd, 1);
  if(ret < 0)
  {
    displayErrorMsg("unix socket failed to listen sock\n");
    close(listenfd);
    return TASK_ERROR; 
  }

  LOG_DBG("unix socket socket init ok\n");
  return listenfd;
}

static int32 netUnixSockAccept(int32 listenfd, netFunctionRun function)
{
  struct sockaddr_un clientaddr;
  int32 acceptfd;
  int32 clientLen;
  int32 ret = -1;

  clientLen = sizeof(clientaddr);
  acceptfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientLen);
  if (acceptfd <= 0) 
  {
    LOG_ERROR("unix socket failed to acccept sock\n");
    return TASK_ERROR; 
  }

  ret = function(acceptfd, NULL);

  close(acceptfd);

  return NET_TCP_SOCK_SET;
}

int32 netUnixSockServer(uint8 *fileDir, int32 timeDelay, netFunctionRun function)
{
  int32 ret;
  fd_set fdsr;
  int32 maxSock;
  struct timeval tv;

  unlink(fileDir);
  
  maxSock = netUnixSockInitServer(fileDir);
  if (maxSock == TASK_ERROR)
  {
    LOG_ERROR("unix socket Server error\n");
    return TASK_ERROR; 
  }
    
  while(1)
  {
    FD_ZERO(&fdsr);
    tv.tv_sec = timeDelay;
    tv.tv_usec = 0;
  
    FD_SET(maxSock, &fdsr);

    /*��������*/
    ret = select(maxSock + 1, &fdsr, NULL, NULL, &tv);
    if (ret < 0) 
    {
      LOG_ERROR("unix socket failed to select sock\n");
      close(maxSock);
      return TASK_ERROR; 
    } 
    else if (ret == 0) 
    {
      LOG_DBG("unix socket server timeout\n");
      continue;
    } 

    if (FD_ISSET(maxSock, &fdsr) )
    {
      ret = netUnixSockAccept(maxSock, function);
      if (ret == NET_TCP_SOCK_SET)
      {
        LOG_DBG("unix socket server continue\n");
        continue;
      }
      else
      {
        LOG_DBG("unix socket server break\n");
        break;
      }
    }
  }

  close(maxSock);
  unlink(fileDir);
  return TASK_OK;
}

int32 netUnixSockClient(uint8 *fileDir, netFunctionRun function)
{
  int32 clientSock;
  struct sockaddr_un serverAddr;
  int32 ret = -1;
  int32 len;
  
  clientSock = socket(AF_UNIX,SOCK_STREAM,0);
  if(clientSock < 0)
  {
    LOG_ERROR("client socket error\n");
    return TASK_ERROR;
  }
  
  LOG_DBG("client socket create successfully.\n");

  memset(&serverAddr,0,sizeof(serverAddr));
  
  serverAddr.sun_family=AF_UNIX;
  strcpy(serverAddr.sun_path, "server_socket");

  len = sizeof(serverAddr);

  ret = connect(clientSock, (struct sockaddr *)&serverAddr, len);
  if (ret < 0)
  {
    LOG_ERROR("client socket connect error\n");
    close(clientSock);
    return TASK_ERROR;
  }

  function(clientSock, NULL);
  close(clientSock);

  return TASK_OK;
}


/***************************************************/
/*
����gethostbyname()���������ת���ġ�����IP��ַ���Լ���Ͷ�д������Ϊ�˷��㣬���ǳ�������������ʾ�����������Ҫ����������IP��ַ��ת��������ԭ��Ϊ��
����struct hostent *gethostbyname(const char *name); 

�� gethostname()���óɹ�ʱ������ָ��struct hosten��ָ�룬������ʧ��ʱ����-1��������gethostbynameʱ���㲻��ʹ��perror()���������������Ϣ����Ӧ��ʹ��herror()����������� 

��������Ϊhosten�Ľṹ���ͣ����Ķ������£�

truct hostent {
�� char *h_name;  �����Ĺٷ����� official name of host
���� char **h_aliases; һ����NULL��β�������������� 
���� int h_addrtype; ���صĵ�ַ���ͣ���Internet������ΪAF-INET 
����int h_length;  ��ַ���ֽڳ��� 
���� char **h_addr_list;  һ����0��β�����飬���������������е�ַ
����};
����#define h_addr h_addr_list[0] ��h-addr-list�еĵ�һ����ַ

����ʹ��ע����ǣ�

    ���ָ��ָ��һ����̬���ݣ����ᱻ��̵ĵ��������ǡ��򵥵�˵�����Ƕ��̻߳��߶���̲���ȫ�ġ�
    �������ʹ��h_addr����ֱ��ʹ��h_addr_list�������ܹ�����պ�ļ����ԡ�
    h_addr��ָ��һ������Ϊh_length��������ַ�������������ʽ�������ڸ�ֵ��struct in_addrʱ��Ӧ��ͨ��htonl��ת�������ǿ���ͨ������һ��ѧϰ������˵�����������
    �������ʹ��GNU���������ǿ���ʹ��gethostbyname_r����gethostbyname2_r���滻��gethostbyname�����������ܹ����õĽ�����̻߳����̰�ȫ�����⣬�����ṩѡ���ַ�������

gethostbyname������Ϊ�˻�öԷ���IP��ַ��

��linux�����£������������ gethostbyname������DNS������������ķ������������ö�������DNS�������ᷴ�ش�����������Ӧ��IP��ַ���������̨Ƕ��ʽ�豸���ӣ�Ҳ��linux����������ô�˺�������/etc/hosts/��ǰĿ¼��Ѱ��������������Ӧ��IP�����ԣ�����̨�豸����ͨ��֮ǰ������Ҫ�ڴ��ļ�������Ҫ���ӵ������Լ���Ӧ��IP��ַ

gethostname() �� ���ر��������ı�׼��������

ԭ�����£�

#include <unistd.h>

int gethostname(char *name, size_t len);

����˵����

���������Ҫ����������

���ջ�����name���䳤�ȱ���Ϊlen�ֽڻ��Ǹ���,���õ���������

���ջ�����name����󳤶�

����ֵ��

��������ɹ����򷵻�0��������������򷵻�-1������Ŵ�����ⲿ����errno�С�


gethostbyname()����˵����������������������ȡIP��ַ
    ����ͷ�ļ�
    #include <netdb.h>
    #include <sys/socket.h>

    ����ԭ��
    struct hostent *gethostbyname(const char *name);
    ��������Ĵ���ֵ����������������������"www.google.cn"�ȵȡ�����ֵ����һ��hostent�Ľṹ�������������ʧ�ܣ�������NULL��

*/
/*ͨ����������������Ϣ*/
int32 getHostNameInfoLocal(uint8 *hostName)
{
  if (hostName == NULL)
  {
    LOG_ERROR("hostName no buffer space\n");
    return TASK_ERROR;
  } 

  if (gethostname(hostName, sizeof(hostName)) < 0)
  {
    LOG_ERROR("gethostname error\n");
    return TASK_ERROR;
  }

  return TASK_OK;
}

int32 showHostInfoByDomain(uint8 *hostName)
{
  struct hostent *hptr;
  int8 *name, **pptr, str[32];
  int32 count = 0;
  
  if (hostName == NULL)
  {
    LOG_ERROR("hostName is empty\n");
    return TASK_ERROR;
  } 

  hptr = gethostbyname(hostName);
  if (hptr) 
  {
    printf("the offical name is %s.\n", hptr->h_name);
    
    for(pptr = hptr->h_aliases; *pptr != NULL; pptr++) 
    {
      printf("the alias name is %s\n", *pptr);
    }

    switch (hptr->h_addrtype) 
    {
      case AF_INET:
        printf("the address type is AF_INET.\n");
        break;
      case AF_INET6:
        printf("the address type is AF_INET6.\n");
        break;
      default:
        break;
    }
    
    printf("the address length is %d Bytes.\n", hptr->h_length);
    
    for (pptr = hptr->h_addr_list; *pptr != NULL; pptr++) 
    {
      count ++;
      printf("the %dth address is %s.\n", count, inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));  //����ת���ɵĵ��ʮ���ƴ浽�ַ��� str �з��أ�����򷵻ؿ�ָ��
    }
  } 
  else 
  {
    LOG_ERROR("gethostbyname Error!\n");
    return TASK_ERROR;
  }

  return TASK_OK;
}

/*
getservbyname
struct servent *getservbyname(const char *name, const char *proto);

����������Է��ظ�����������Э��������ط�����Ϣ��

������ /etc/service �ļ��и��ݷ������Ƶ��˿ںŵ�ӳ���ϵ��ȡ�����Ϣ���������ִ�гɹ�����ô����һ�� struct servent �ṹָ�룬�ýṹ�������£�

��������������Э����

����ֵ��

    һ��ָ�� servent �ṹ���ָ��
    ��ָ��(��������)

��netdb.hͷ�ļ����ǿ����ҵ� servent �ṹ���˵����

struct servent {
    char   *s_name;       official service name 
    char  **s_aliases;    other aliases 
    int     s_port;       port for this service 
    char  **s_proto;      protocol to use 
    };

���صĽṹ���еĶ˿ں��ǰ������ֽ�˳�򱣴�������������ʱ��Ҫ�� ntohs() ����ת��������˳�򱣴��������
*/

int32 showServiceInfo(uint8 *service, uint8 *protocol)
{
  struct servent *sptr;
   
  if (service == NULL || protocol == NULL)
  {
    LOG_ERROR("server or proto is empty\n");
    return TASK_ERROR;
  }

  sptr = getservbyname(service, protocol);
  if (sptr) 
    {
        printf("the port of service %s using %s protocol is %d.\n", sptr->s_name, protocol, ntohs(sptr->s_port));  //�������ֽ�˳��Ķ˿�ֵת��������˳��
    } 
  else 
    {
      LOG_ERROR("getservbyname error\n");
      return TASK_ERROR;
    }

  return TASK_OK;
}

/*
gethostbyaddr(3)����

�������������ֵ��IP��ַ��ע�⣬���ﲻ�Ǽ򵥵��ַ�������Ҫ�Ƚ��ַ�����ʽ��IP��ַ��inet_atonת��һ�£���Ȼ�󾭹���������������ɷ���ֵ����������ֵ��һ��hostent�ṹ.��Ϊ����hosten��������Ľṹ�����ǿ���������ṹ�����ҵ���������Ҫ����Ϣ��

��ʱ����֪��һ��IP��ַ���������Ǳ���������������IP��ַ��һ��������Ҳ����Ҫ��¼�������ӵĿͻ���������������������IP��ַ��gethostbyaddr������Ҫ���£�
#include <sys/socket.h>
struct hostent *gethostbyaddr(
        const char *addr,
        int len,         
    int type);       
gethostbyaddr���������������������
1 Ҫת��Ϊ�������������ַ(addr)������AF_INET��ַ���ͣ�����ָ���ַ�ṹ�е�sin_addr��Ա��
2 �����ַ�ĳ��ȡ�����AF_INET���ͣ����ֵΪ4��������AF_INET6���ͣ����ֵΪ16��
3 �����ַ�����ͣ����ֵΪAF_INET����AF_INET6��

������Ҫע�⣬��һ������Ϊһ���ַ�ָ�룬ʵ����������ܶ��ָ�ʽ�ĵ�ַ��������Ҫ�����ǵĵ�ַָ��ת��Ϊ(char *)��������롣�ڶ�������ָ�������ṩ�ĵ�ַ�ĳ��ȡ�

����������Ϊ�����ݵĵ�ַ�����͡���IPv4������ΪAF_INET��Ҳ���ڽ��������ֵ������IPv6��ַ��ʽ��AF_INET6��
---------------
������Ϊʲô��������ʱ�򣬿���UNP�����һ�仰�� ����DNS��˵����gethostbyaddr��in_addr.arpa������һ�����ַ�������ѯPTR��¼��

�����������ҵĵ��Բ��Ƿ������ɣ�û��������������ɡ����Բ��С������ص�/etc/hosts��������������ܡ��Ҿ�����Ϊʲôgethostbyname����/etc/hosts�ļ��в鿴��Ϣ��Ȼ��û�ж�Ӧ�Ļ����ͻ᷵����һ����DNS���н��������������Ϊʲô�����Զ������أ���Ps����᲻���Ƿ�������Ƚ����õ�������������������в�ι�ϵ����IPû�в�ι�ϵ�������㴦��ɡ�����ͨ��nslookup 115.239.211.110 ���в�ѯʱ��ʾ�������

 

** server can't find 110.211.239.115.in-addr.arpa.: NXDOMAIN

�������ˣ�û���ˣ�Ҫʹ���������������Ҫ�з�������ķ���
*/

int32 showRemoteServeInfo(uint8 *ipAddr)
{
  int8 *ptr,**pptr;
  struct hostent *hptr;
  int8 str[32];
  int8 ipaddr[16];
  struct in_addr *hipaddr;
 
  /* ȡ��������һ����������Ҫ������IP��ַ */
  ptr = ipAddr;
  /* ����inet_aton()��ptr�������ַ�����ŵĵط���ָ�룬hipaddr��in_addr��ʽ�ĵ�ַ */
 
  if(!inet_aton(ptr,hipaddr))
  {
    LOG_ERROR("inet_aton error\n");
    return TASK_ERROR;
  }
 
  /* ����gethostbyaddr()�����ý��������hptr�� */
  if((hptr = gethostbyaddr(hipaddr, 4, AF_INET)) == NULL )
  {
   LOG_ERROR("gethostbyaddr error for addr:%s\n", ptr);
   return TASK_ERROR; /* �������gethostbyaddr�������󣬷���1 */
  }
 
  /* �������Ĺ淶������� */
  printf("official hostname:%s\n",hptr->h_name);

  /* ���������ж�������������б����ֱ����� */
  for(pptr = hptr->h_aliases; *pptr != NULL; pptr++)
  {
    printf("  alias:%s\n",*pptr);
  }
  
  /* ���ݵ�ַ���ͣ�����ַ����� */
  switch(hptr->h_addrtype)
  {
   case AF_INET:
   case AF_INET6:
    pptr=hptr->h_addr_list;
    /* ���ղŵõ������е�ַ������������е�����inet_ntop()���� */
    for(;*pptr!=NULL;pptr++)
    {
      printf("  address:%s\n", inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
    }
    break;
   default:
    LOG_ERROR("unknown address type\n");
    break;
  }
  
  return TASK_OK;
}

/*
Linux����������ں˽����ķ�����ͨ��ioctl��ʵ�ֵģ�ioctl������Э��ջ���н������ɵõ�����ӿڵ���Ϣ�������豸��ӳ�����Ժ���������ӿ�.���һ��ܹ��鿴���޸ģ�ɾ��ARP���ٻ������Ϣ�����ԣ������б�Ҫ�˽�һ��ioctl�����ľ���ʵ��.

2.��ؽṹ������غ���

#include <sys/ioctl.h>

int ioctl(int d,int request,....);

����:

d-�ļ��������������Ƕ������׽��ֲ�������Ȼ���׽���������

request-������

ʡ�ԵĲ��ֶ�Ӧ��ͬ���ڴ滺��������������ڴ滺��������������request��������,���濴һ�¾��嶼����Щ��ػ�������

(1)����ӿ�����ṹifreq

struct ifreq{
#define IFHWADDRLEN 6 //6���ֽڵ�Ӳ����ַ����MAC
union{
char ifrn_name[IFNAMESIZ];//����ӿ�����
}ifr_ifrn;
union{
struct sockaddr ifru_addr;//����IP��ַ
struct sockaddr ifru_dstaddr;//Ŀ��IP��ַ
struct sockaddr ifru_broadaddr;//�㲥IP��ַ
struct sockaddr ifru_netmask;//�������������ַ
struct sockaddr ifru_hwaddr;//����MAC��ַ
short ifru_flags;//����ӿڱ��
int ifru_ivalue;//��ͬ�������岻ͬ
struct ifmap ifru_map;//������ַӳ��
int ifru_mtu;//����䵥Ԫ
char ifru_slave[IFNAMSIZ];//ռλ��
char ifru_newname[IFNAMSIZE];//������
void __user* ifru_data;//�û�����
struct if_settings ifru_settings;//�豸Э������
}ifr_ifru;
}
#define ifr_name ifr_ifrn.ifrn_name;//�ӿ�����
#define ifr_hwaddr ifr_ifru.ifru_hwaddr;//MAC
#define ifr_addr ifr_ifru.ifru_addr;//����IP
#define ifr_dstaddr ifr_ifru.dstaddr;//Ŀ��IP
#define ifr_broadaddr ifr_ifru.broadaddr;//�㲥IP
#define ifr_netmask ifr_ifru.ifru_netmask;//��������
#define ifr_flags ifr_ifru.ifru_flags;//��־
#define ifr_metric ifr_ifru.ifru_ivalue;//�ӿڲ��
#define ifr_mtu ifr_ifru.ifru_mtu;//����䵥Ԫ
#define ifr_map ifr_ifru.ifru_map;//�豸��ַӳ��
#define ifr_slave ifr_ifru.ifru_slave;//���豸
#define ifr_data ifr_ifru.ifru_data;//�ӿ�ʹ��
#define ifr_ifrindex ifr_ifru.ifru_ivalue;//����ӿ����
#define ifr_bandwidth ifr_ifru.ifru_ivalue;//���Ӵ���
#define ifr_qlen ifr_ifru.ifru_ivalue;//���䵥Ԫ����
#define ifr_newname ifr_ifru.ifru_newname;//������
#define ifr_seeting ifr_ifru.ifru_settings;//�豸Э������

�����������ӿڵ������Ϣ���ʹ���ifreq�ṹ��.


(2)�����豸����ifmap

struct ifmap{//�����豸��ӳ������
unsigned long mem_start;//��ʼ��ַ
unsigned long mem_end;//������ַ
unsigned short base_addr;//����ַ
unsigned char irq;//�жϺ�
unsigned char dma;//DMA
unsigned char port;//�˿�
}


(3)�������ýӿ�ifconf

struct ifconf{//�������ýṹ����һ�ֻ�����
int ifc_len;//������ifr_buf�Ĵ�С
union{
char__user *ifcu_buf;//�����ָ��
struct ifreq__user* ifcu_req;//ָ��ifreqָ��
}ifc_ifcu;
};
#define ifc_buf ifc_ifcu.ifcu_buf;//��������ַ
#define ifc_req ifc_ifcu.ifcu_req;//ifc_req��ַ


(4)ARP���ٻ������arpreq

/**
ARP���ٻ������������IP��ַ��Ӳ����ַ��ӳ���
����ARP���ٻ���������� SIOCDARP,SIOCGARP,SIOCSARP�ֱ���ɾ��ARP���ٻ����һ����¼�����ARP���ٻ����һ����¼���޸�ARP���ٻ����һ����¼
struct arpreq{
struct sockaddr arp_pa;//Э���ַ
struct sockaddr arp_ha;//Ӳ����ַ
int arp_flags;//���
struct sockaddr arp_netmask;//Э���ַ����������
char arp_dev[16];//��ѯ����ӿڵ�����
}


3. ������request

SIOCGIFCONF ��ȡ���нӿڵ��嵥

SIOCSIFADDR ���ýӿڵ�ַ

SIOCGIFADDR ��ȡ�ӿڵ�ַ

SIOCSIFFLAGS  ���ýӿڱ�־

SIOCGIFFLAGS  ��ȡ�ӿڱ�־

SIOCSIFDSTADDR  ���õ㵽���ַ

SIOCGIFDSTADDR  ��ȡ�㵽���ַ

SIOCGIFBRDADDR  ��ȡ�㲥��ַ

SIOCSIFBRDADDR  ���ù㲥��ַ

SIOCGIFNETMASK  ��ȡ��������

SIOCSIFNETMASK  ������������

SIOCGIFMETRIC ��ȡ�ӿڵĲ��

SIOCSIFMETRIC ���ýӿڵĲ��

SIOCGIFMTU  ��ȡ�ӿ�MTU

SIOCSARP  ����/�޸�ARP����

SIOCGARP  ��ȡARP����

SIOCDARP  ɾ��ARP����

SIOCGIFMETRIC, SIOCSIFMETRIC
    ʹ�� ifr_metric ��ȡ �� ���� �豸�� metric ֵ. �ù��� Ŀǰ ��û�� ʵ��. ��ȡ���� ʹ ifr_metric �� 0, �� ���ò��� �� ���� EOPNOTSUPP.
SIOCGIFMTU, SIOCSIFMTU
    ʹ�� ifr_mtu ��ȡ �� ���� �豸�� MTU(����䵥Ԫ). ���� MTU �� ��Ȩ����. ��С�� MTU ���� ���� �ں� ����.
SIOCGIFHWADDR, SIOCSIFHWADDR
    ʹ�� ifr_hwaddr ��ȡ �� ���� �豸�� Ӳ����ַ. ���� Ӳ����ַ �� ��Ȩ����.
SIOCSIFHWBROADCAST
    ʹ�� ifr_hwaddr ��ȡ �� ���� �豸�� Ӳ���㲥��ַ. ���Ǹ� ��Ȩ����.
SIOCGIFMAP, SIOCSIFMAP
    ʹ�� ifr_map ��ȡ �� ���� �ӿڵ� Ӳ������. ���� ������� �� ��Ȩ����. 

*/
int32 showAllIpaddrByeth(void)
{
  int32 i=0;
  int32 sockfd;
  struct ifconf ifconf;
  uint8 buf[512];

  struct ifreq *ifreq;
  
  //��ʼ��ifconf
  ifconf.ifc_len = 512;
  ifconf.ifc_buf = buf;

  if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    LOG_ERROR("socket error\n");
    return TASK_ERROR;
  }  

  ioctl(sockfd, SIOCGIFCONF, &ifconf);    //��ȡ���нӿ���Ϣ
  
  //������һ��һ���Ļ�ȡIP��ַ

  ifreq = (struct ifreq*)buf;  

  for(i=(ifconf.ifc_len/sizeof(struct ifreq)); i > 0; i--)
  {
    //if(ifreq->ifr_flags == AF_INET){            //for ipv4

    printf("name = [%s]\n", ifreq->ifr_name);
    printf("local addr = [%s]\n",inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr));
    ifreq++;
    //}
  }

  return TASK_OK;
}

int32 getMacaddrByEth(uint8 *ifName,uint8 *mac)
{
  int32 i;
  struct ifreq ifreq;
  int32 sock;

  if((sock=socket(AF_INET,SOCK_STREAM,0)) < 0)
  {
    LOG_ERROR("socket error\n");
    return TASK_ERROR;
  }
  
  strcpy(ifreq.ifr_name,ifName);
  if(ioctl(sock,SIOCGIFHWADDR,&ifreq)<0)
  {
    LOG_ERROR("socket SIOCGIFHWADDR error\n");
    return TASK_ERROR;
  }
  
  for (i=0; i<6; i++)
  {
    sprintf(mac+3*i, "%02x:", (unsigned char)ifreq.ifr_hwaddr.sa_data[i]);
  }
  
  mac[17]='\0';
  
  LOG_DBG("mac addr is: %s\n", mac);
  
  return TASK_OK;
}

int32 getIpaddrByEth(uint8 *eth, uint8 *ipAddr)
{
  int32 inet_sock;
  struct ifreq ifr;
  
  if (eth == NULL)
  {
    LOG_ERROR("eth is empty\n");
    return TASK_ERROR;
  }  
  
  inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (inet_sock < 0)
  {
    LOG_ERROR("socket error\n");
    return TASK_ERROR;
  }
  
  strcpy(ifr.ifr_name, eth);
  if (ioctl(inet_sock, SIOCGIFADDR, &ifr) < 0)
  {
    LOG_ERROR("socket SIOCGIFADDR error\n");
    return TASK_ERROR;
  }

  strcpy(ipAddr, inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));
  
  LOG_DBG("ipaddr is %s\n", ipAddr);

  return TASK_OK;
}

int showNetInterfaceInfo(void)
{
	int32 s;/* �׽��������� */
	int32 err = -1;/*����ֵ*/

	/* ����һ�����ݱ��׽��� */
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)	
  {
		LOG_ERROR("socket() ����\n");
		return TASK_ERROR;
	}

	/* �������ӿڵ����� */
	{
		struct ifreq ifr;
		ifr.ifr_ifindex = 2; /* ��ȡ��2������ӿڵ����� */
		err = ioctl(s, SIOCGIFNAME, &ifr);
		if(err)
    {
			LOG_ERROR("SIOCGIFNAME Error\n");
		}
    else
		{
			printf("the %dst interface is:%s\n",ifr.ifr_ifindex,ifr.ifr_name);
		}
	}

	/* �������ӿ����ò��� */
	{
		/* ��ѯ������eth0������� */
		struct ifreq ifr;
		memcpy(ifr.ifr_name, "eth0",5);

		/* ��ȡ��� */
		err = ioctl(s, SIOCGIFFLAGS, &ifr);
		if(!err)
    {
			printf("SIOCGIFFLAGS:%d\n",ifr.ifr_flags);
		}

		/* ��ȡMETRIC */
		err = ioctl(s, SIOCGIFMETRIC, &ifr);
		if(!err)
    {
			printf("SIOCGIFMETRIC:%d\n",ifr.ifr_metric);
		}
		
		/* ��ȡMTU */		
		err = ioctl(s, SIOCGIFMTU, &ifr);
		if(!err)
    {
			printf("SIOCGIFMTU:%d\n",ifr.ifr_mtu);
		}	
		
		/* ��ȡMAC��ַ */
		err = ioctl(s, SIOCGIFHWADDR, &ifr);
		if(!err)
    {
			unsigned char *hw = ifr.ifr_hwaddr.sa_data;
			printf("SIOCGIFHWADDR:%02x:%02x:%02x:%02x:%02x:%02x\n",hw[0],hw[1],hw[2],hw[3],hw[4],hw[5]);
		}	
		
		/* ��ȡ����ӳ����� */
		err = ioctl(s, SIOCGIFMAP, &ifr);
		if(!err)
    {
			printf("SIOCGIFMAP,mem_start:%d,mem_end:%d, base_addr:%d, dma:%d, port:%d\n",
				ifr.ifr_map.mem_start, 	/*��ʼ��ַ*/
				ifr.ifr_map.mem_end,		/*������ַ*/
				ifr.ifr_map.base_addr,	/*����ַ*/
				ifr.ifr_map.irq ,				/*�ж�*/
				ifr.ifr_map.dma ,				/*DMA*/
				ifr.ifr_map.port );			/*�˿�*/
		}
		
		/* ��ȡ������� */
		err = ioctl(s, SIOCGIFINDEX, &ifr);
		if(!err)
    {
			printf("SIOCGIFINDEX:%d\n",ifr.ifr_ifindex);
		}
		
		/* ��ȡ���Ͷ��г��� */		
		err = ioctl(s, SIOCGIFTXQLEN, &ifr);
		if(!err)
    {
			printf("SIOCGIFTXQLEN:%d\n",ifr.ifr_qlen);
		}			
	}

	/* �������ӿ�IP��ַ */
	{
		struct ifreq ifr;
		/* �����������ָ��sockaddr_in��ָ�� */
		struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
		char ip[16];/* ����IP��ַ�ַ��� */
		memset(ip, 0, 16);
		memcpy(ifr.ifr_name, "eth0",5);/*��ѯeth0*/
		
		/* ��ѯ����IP��ַ */		
		err = ioctl(s, SIOCGIFADDR, &ifr);
		if(!err)
    {
			/* ������ת��Ϊ����Ķε��ַ��� */
			inet_ntop(AF_INET, &sin->sin_addr.s_addr, ip, 16 );
			printf("SIOCGIFADDR:%s\n",ip);
		}
		
		/* ��ѯ�㲥IP��ַ */
		err = ioctl(s, SIOCGIFBRDADDR, &ifr);
		if(!err)
    {
			/* ������ת��Ϊ����Ķε��ַ��� */
			inet_ntop(AF_INET, &sin->sin_addr.s_addr, ip, 16 );
			printf("SIOCGIFBRDADDR:%s\n",ip);
		}
		
		/* ��ѯĿ��IP��ַ */
		err = ioctl(s, SIOCGIFDSTADDR, &ifr);
		if(!err)
    {
			/* ������ת��Ϊ����Ķε��ַ��� */
			inet_ntop(AF_INET, &sin->sin_addr.s_addr, ip, 16 );
			printf("SIOCGIFDSTADDR:%s\n",ip);
		}
		
		/* ��ѯ�������� */
		err = ioctl(s, SIOCGIFNETMASK, &ifr);
		if(!err)
    {
			/* ������ת��Ϊ����Ķε��ַ��� */
			inet_ntop(AF_INET, &sin->sin_addr.s_addr, ip, 16 );
			printf("SIOCGIFNETMASK:%s\n",ip);
		}
	}

	/* ���Ը���IP��ַ */
#if 0
	{
		struct ifreq ifr;
		/* �����������ָ��sockaddr_in��ָ�� */
		struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
		char ip[16];/* ����IP��ַ�ַ��� */
		int err = -1;
		
		/* ������IP��ַ����Ϊ192.169.1.175 */
		printf("Set IP to 192.168.1.175\n");
		memset(&ifr, 0, sizeof(ifr));/*��ʼ��*/
		memcpy(ifr.ifr_name, "eth0",5);/*��eth0��������IP��ַ*/
		inet_pton(AF_INET, "192.168.1.175", &sin->sin_addr.s_addr);/*���ַ���ת��Ϊ�����ֽ��������*/
		sin->sin_family = AF_INET;/*Э����*/
		err = ioctl(s, SIOCSIFADDR, &ifr);/*�������ñ���IP��ַ��������*/
		if(err)
    {/*ʧ��*/
			LOG_ERROR("SIOCSIFADDR error\n");
		}
    else
    {/*�ɹ����ٶ�ȡһ�½���ȷ��*/
			printf("check IP --");
			memset(&ifr, 0, sizeof(ifr));/*������0*/
			memcpy(ifr.ifr_name, "eth0",5);/*����eth0*/
			ioctl(s, SIOCGIFADDR, &ifr);/*��ȡ*/
			inet_ntop(AF_INET, &sin->sin_addr.s_addr, ip, 16);/*��IP��ַת��Ϊ�ַ���*/
			printf("%s\n",ip);/*��ӡ*/
		}		
	}
#endif
	close(s);
	return 0;
}

/*
#include <sys/socket.h>

  int setsockopt( int socket, int level, int option_name,const void *option_value, size_t option_len);
  
��һ������socket���׽�����������

�ڶ�������level�Ǳ����õ�ѡ��ļ��������Ҫ���׽��ּ���������ѡ��ͱ����level����ΪSOL_SOCKET��

���������� option_nameָ��׼�����õ�ѡ�option_name��������Щ����ȡֵ����ȡ����level����linux 2.6�ں�Ϊ�����ڲ�ͬ��ƽ̨�ϣ����ֹ�ϵ���ܻ��в�ͬ�������׽��ּ�����(SOL_SOCKET)��


1. SO_BROADCAST �׽���ѡ��

     ��ѡ������ֹ���̷��͹㲥��Ϣ��������ֻ�����ݱ��׽���֧�ֹ㲥�����һ���������֧�ֹ㲥��Ϣ�������ϣ�������̫�������ƻ����ȣ������ǲ������ڵ�Ե���·�Ͻ��й㲥��Ҳ�������ڻ������ӵĴ���Э�飨����TCP��SCTP��֮�Ͻ��й㲥��

2. SO_DEBUG �׽���ѡ��

     ��ѡ�����TCP֧�֡�����һ��TCP�׽��ֿ�����ѡ��ʱ���ں˽�ΪTCP�ڸ��׽��ַ��ͺͽ��ܵ����з��鱣����ϸ������Ϣ����Щ��Ϣ�������ں˵�ĳ�����λ������У�����ʹ��trpt������м�顣

3. SO_KEEPALIVE �׽���ѡ��

     ��һ��TCP�׽������ñ��ִ��ѡ������2Сʱ���ڸ��׽��ֵ��κ�һ�����϶�û�����ݽ�����TCP���Զ����Զ˷���һ�����ִ��̽��ֽڡ�����һ���Զ˱�����Ӧ��TCP�ֽڣ����ᵼ������3�����֮һ��

��1���Զ���������ACK��Ӧ��Ӧ�ý��̵ò���֪ͨ����Ϊһ�������������־������޶�����2Сʱ��TCP��������һ��̽��ֽڡ�

��2���Զ���RST��Ӧ������֪����TCP���Զ��ѱ��������������������׽��ֵĴ����������ΪECONNRESET���׽��ֱ����򱻹رա�

��3���Զ˶Ա��ִ��̽��ֽ�û���κ���Ӧ��

     �������û�ж�TCP��̽��ֽڵ���Ӧ�����׽��ֵĴ��������ͱ���ΪETIMEOUT���׽��ֱ����򱻹رա�Ȼ��������׽����յ�һ��ICMP������Ϊĳ��̽��ֽڵ���Ӧ���Ǿͷ�����Ӧ�Ĵ����׽��ֱ���Ҳ���رա�

     ��ѡ��Ĺ����Ǽ��Զ������Ƿ�������Ĳ��ɴƩ�粦�ŵ��ƽ�������ӵ��ߣ���Դ�������ϵȵȣ�������Զ˽��̱���������TCP�������ӷ���һ��FIN�������ͨ������select�����׵ļ�⵽��

     ��ѡ��һ���ɷ�����ʹ�ã������ͻ�Ҳ����ʹ�á�������ʹ�ñ�ѡ��ʱ��Ϊ���ǻ��󲿷�ʱ�������ڵȴ���ԽTCP���ӵ������ϣ�Ҳ����˵�ڵȴ��ͻ�������Ȼ������ͻ��������ӵ��ߣ���Դ�������ϵͳ���������������̽���Զ����֪�������������ȴ���Զ���ᵽ������롣���ǳ��������Ϊ�뿪���ӡ����ִ��ѡ�������Щ�뿪���Ӳ���ֹ���ǡ�


4. SO_LINGER �׽���ѡ��

      ��ѡ��ָ��close�������������ӵ�Э�飨����TCP��SCTP��������UDP����β�����Ĭ�ϲ�����close�������أ�������������ݲ������׽��ַ��ͻ������У�ϵͳ�����Ű���Щ���ݷ��͸��Զˡ�

    SO_LINGER���ѡ���ѡ�close�� shutdown���ȵ������׽������Ŷӵ���Ϣ�ɹ����ͻ򵽴��ӳ�ʱ���Ż᷵�ء����򣬵��ý��������ء�

 

              ��ѡ��Ĳ�����option_value)��һ��linger�ṹ��

 
[cpp] view plaincopyprint?

    struct linger {  
        int   l_onoff;      
        int   l_linger;     
    };  


5. SO_RCVBUF��SO_SNDBUF�׽���ѡ��

     ÿ���׽��ֶ���һ�����ͻ�������һ�����ջ�������

     ���ջ�������TCP��UDP��SCTCP����������յ������ݣ�ֱ����Ӧ�ý��̶�ȡ������TCP��˵���׽��ֽ��ջ��������ÿռ�Ĵ�С������TCPͨ��Զ˵Ĵ��ڴ�С��TCP�׽��ֽ��ջ������������������Ϊ������Զ˷�������������ͨ�洰�ڴ�С�����ݡ������TCP���������ƣ�����Զ����Ӵ��ڴ�С�������˳������ڴ�С�����ݣ�����TCP���������ǡ�Ȼ������UDP��˵�������յ������ݱ�װ�����׽��ֽ��ջ�����ʱ�������ݱ��ͱ��������ع�һ�£�UDP��û���������Ƶģ��Ͽ�ķ��Ͷ˿��Ժ����׵���û�����Ľ��նˣ����½��ն˵�UDP�������ݱ���

     �������׽���ѡ���������Ǹı���������������Ĭ�ϴ�С�����ڲ�ͬ��ʵ�֣�Ĭ��ֵ�ô�С�����кܴ�Ĳ���������֧��NFS����ôUDP���ͻ������Ĵ�С����Ĭ��Ϊ9000�ֽ����ҵ�һ��ֵ����UDP���ջ������Ĵ�С�򾭳�Ĭ��Ϊ40000�ֽ����ҵ�һ��ֵ��

     ������TCP�׽��ֽ��ջ������Ĵ�Сʱ���������õ�˳�����Ҫ��������ΪTCP�ĳ��ڹ�ģѡ��ʱ�ڽ�������ʱ��SYN�ֽ���Զ˻����õ��ġ����ڿͻ�������ζ��SO_RCVBUFѡ������ڵ���connect֮ǰ���ã����ڷ�����������ζ�Ÿ�ѡ������ڵ���listen֮ǰ�������׽������á����������׽������ø�ѡ����ڿ��ܴ��ڵĳ��ڹ�ģѡ��û���κ�Ӱ�죬��Ϊacceptֱ��TCP����·�������Żᴴ���������������׽��֡�����Ǳ���������׽������ñ�ѡ���ԭ��

 

6. SO_RCVLOWAT �� SO_SNDLOWAT�׽���ѡ��

     ÿ���׽��ֻ���һ�����յ�ˮλ��Ǻ�һ�����͵�ˮλ��ǡ�������select����ʹ�ã��������׽���ѡ�����������޸���������ˮλ��ǡ�

     ���յ�ˮλ�������select���ء��ɶ���ʱ���׽��ֽ��ջ������������������������TCP��UDP��SCTP�׽��֣���Ĭ��ֵΪ1�����͵�ˮλ�������select���ء���д��ʱ�׽��ַ��ͻ�����������Ŀ��ÿռ䡣����TCP�׽��֣���Ĭ��ֵͨ��Ϊ2048��UDPҲʹ�÷��͵�ˮλ��ǣ�Ȼ������UDP�׽��ֵķ��ͻ������п��ÿռ���ֽ����Ӳ��ı䣨��ζUDP����Ϊ��Ӧ�ý��̴��ݸ��������ݱ�������������ֻҪһ��UDP�׽��ֵķ��ͻ�������С���ڸ��׽��ֵĵ�ˮλ��ǣ���UDP�׽��־����ǿ�д�����Ǽǵ�UDP��û�з��ͻ���������ֻ�з��ͻ�������С������ԡ�

 

7. SO_RCVTIMEO �� SO_SNDTIMEO�׽���ѡ��

     ������ѡ���������Ǹ��׽��ֵĽ��պͷ�������һ����ʱֵ��ע�⣬�������ǵ�getsockopt��setsockopt�����Ĳ�����ָ��timeval�ṹ��ָ�룬��select���ò�����ͬ�������������������΢�������涨��ʱ������ͨ��������ֵΪ0s��0��s����ֹ��ʱ��Ĭ���������������ʱ���ǽ�ֹ�ġ�

     ���ճ�ʱӰ��5�����뺯����read,readv,recv,recvfrom��recvmsg�����ͳ�ʱӰ��5�����������write,writev,send,sendto��sendmsg��

 

8. SO_REUSEADDR �� SO_REUSEPORT �׽���ѡ��

     SO_REUSEADDR�׽���ѡ����������4����ͬ�Ĺ��á�

��1��SO_REUSEADDR��������һ��������������������������֪�Ķ˿ڣ���ʹ��ǰ�����Ľ��ö˿��������ǵı��ض˿ڵ������Դ��ڡ��������ͨ�������������ģ�

         ��a������һ��������������

         ��b���������󵽴����һ���ӽ�������������ͻ���

         ��c��������������ֹ�����ӽ��̼���Ϊ���������ϵĿͻ��ṩ����

         ��d������������������      

     Ĭ������£��������������ڲ���dͨ������socket��bind��listen��������ʱ����������ͼ����һ���������ӣ������������������Ǹ��ӽ��̴����ŵ����ӣ��ϵĶ˿ڣ��Ӷ�bind���û�ʧ�ܡ���������÷�������socket��bind��������֮��������SO_REUSEADDR�׽���ѡ���ô���ɹ�������TCP��������Ӧ��ָ�����׽���ѡ����������������������±�����������

��2��SO_REUSEADDR������ͬһ�˿�������ͬһ�������Ķ��ʵ����ֻҪÿ��ʵ������һ����ͬ�ı���IP��ַ���ɡ�����TCP�����Ǿ��Բ���������������ͬIP��ַ����ͬ�˿ںŵĶ����������������ȫ�ظ������󣬼�ʹ���Ǹ��ڶ���������������SO_REUSEADDR�׽���Ҳ�����á�

��3��SO_REUSEADDR ��������������ͬһ�˿ڵ�����׽����ϣ�ֻҪÿ������ָ����ͬ�ı���IP��ַ���ɡ�

��4��SO_REUSEADDR������ȫ�ظ������󣺵�һ��IP��ַ�Ͷ˿ں��Ѱ󶨵�ĳ���׽�����ʱ���������Э��֧�֣�ͬ����IP��ַ�Ͷ˿ڻ�����������һ���׽����ϡ�һ����˵�����Խ�֧��UDP�׽��֡�

*/

int32 showSocketInfo(void)
{
	int32 rcvbuf_size;
	int32 sndbuf_size;
	int32 type=0;
	socklen_t size;
	int32 sock_fd;
	struct timeval set_time,ret_time;
  
	if((sock_fd=socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		LOG_ERROR("socket()  AF_INET ����\n");
		return TASK_ERROR;
	}
  
	size=sizeof(sndbuf_size);
	getsockopt(sock_fd,SOL_SOCKET,SO_SNDBUF,&sndbuf_size,&size);
	printf("sndbuf_size=%d\n",sndbuf_size);
	printf("size=%d\n",size);
	size=sizeof(rcvbuf_size);
	getsockopt(sock_fd,SOL_SOCKET,SO_RCVBUF,&rcvbuf_size,&size);
	printf("rcvbuf_size=%d\n",rcvbuf_size);
	
	size=sizeof(type);
	getsockopt(sock_fd,SOL_SOCKET,SO_TYPE,&type,&size);
	printf("socket type=%d\n",type);
	size=sizeof(struct timeval);
	
	getsockopt(sock_fd,SOL_SOCKET,SO_SNDTIMEO,&ret_time,&size);
	printf("default:time out is %ds,and %dns\n",ret_time.tv_sec,ret_time.tv_usec);

	set_time.tv_sec=10;
	set_time.tv_usec=100;
	setsockopt(sock_fd,SOL_SOCKET,SO_SNDTIMEO,&set_time,size);
	getsockopt(sock_fd,SOL_SOCKET,SO_SNDTIMEO,&ret_time,&size);
	printf("after modify:time out is %ds,and %dns\n",ret_time.tv_sec,ret_time.tv_usec);

	int ttl=0;
	size=sizeof(ttl);
	getsockopt(sock_fd,IPPROTO_IP,IP_TTL,&ttl,&size);
	printf("the default ip  ttl is %d\n",ttl);
	int maxseg=0;
	size=sizeof(maxseg);
	getsockopt(sock_fd,IPPROTO_TCP,TCP_MAXSEG,&maxseg,&size);
	printf("the TCP max seg is %d\n",maxseg);
}

