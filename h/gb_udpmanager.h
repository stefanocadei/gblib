#ifdef __cplusplus
extern "C" {
#endif

#include <netinet/in.h>

typedef struct
{
        int fd;
        int port;
        socklen_t socklen;
        struct sockaddr_in peeraddr;
        int  indexInsideGBQueueDescriptors;
        void *eventHandler;
}GB_UDP_SOCKET_INFO;


typedef void (*gb_udpMulticastDatagramArrived)(char *data,int n,GB_UDP_SOCKET_INFO udpSocketInfo);

void gb_initUDPManager();
void gb_startUDPMulticastListener(int port,gb_udpMulticastDatagramArrived udpEventHandler);

#ifdef __cplusplus
}
#endif
