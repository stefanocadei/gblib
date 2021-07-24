#define _GNU_SOURCE 
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "../h/gb_utility.h"
#include "../h/gb_udpmanager.h"
#include <fcntl.h>
#include <netdb.h>

static char *PREFIX="[     GBLIB UDPMANAGER    ]"; /*0.26*/
static char debugString[200];

#define MAX_UDP_SOCKETS 20
int udp_socket_descriptors[MAX_UDP_SOCKETS];
gb_udpMulticastDatagramArrived udpEventHandlers[MAX_UDP_SOCKETS];

void* gb_manageUDPSocketDatagrams(void *params);

static char bDataConsumed=0;

void gb_udpDefaultEventHandler(char *data,int n,GB_UDP_SOCKET_INFO udpSocketInfo)
{
	int i=0;
	sprintf(debugString,"%s UDP MULTICAST LISTENER DEFAUL EVENT HANDLER - %d BYTES RECEIVED ('%s') - NO ACTION:",PREFIX,n,data);
        gb_cprintf(debugString,GB_YELLOW);
        for(i=0;i<n;i++)
        {
        	printf("%02x|",data[i]);
        }
        fflush(stdout); 
}

void gb_initUDPManager()
{
	int i=0;
	for(i=0;i<MAX_UDP_SOCKETS;i++)
	{
		udp_socket_descriptors[i] = -1;
		udpEventHandlers[i] = NULL;
	}
}

void gb_startUDPMulticastListener(int port,gb_udpMulticastDatagramArrived udpEventHandler)
{
	pthread_t udpListenerThread;
	GB_UDP_SOCKET_INFO udpSocketInfo;
	//struct in_addr ia;
	int sockfd;
	struct hostent *group;
	struct ip_mreq mreq;

	//multicast address
	char group_addr[16] = "224.0.0.1";


	if(udpEventHandler==NULL)
	{
		udpEventHandler=gb_udpDefaultEventHandler;
	}
	
	//socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
	if (sockfd < 0)
	{
		sprintf(debugString,"%s ERROR DURING SOCKET UDP CREATION",PREFIX);
		gb_cprintf(debugString,GB_RED);
		return;
	}

	//socket initialization
	bzero(&mreq, sizeof(struct ip_mreq));
	if((group=(struct hostent*)gethostbyname(group_addr))== (struct hostent *)0)    //PARTE OSCURA DI INIZIALIZZAZIONE SOCKET PERIL MULTICAST (DA APPROFONDIRE)
	{
		sprintf(debugString,"%s SOCKET UDP CREATED BUT NOT SUITABLE FOR UDP MULTICAST LISTENER",PREFIX);
		gb_cprintf(debugString,GB_RED);
		return;
	}
	//bcopy(&group->h_addr, (void*) &ia, group->h_length);
	/*bcopy(&ia, &mreq.imr_multiaddr.s_addr, sizeof(struct in_addr));
	mreq.imr_interface.s_addr=htonl(INADDR_ANY);
	if(setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(struct ip_mreq))== -1)
	{
		printf("lg setsockopt 2\n");
		//return;
	}*/

	//populatinf first part of data
	udpSocketInfo.fd=sockfd;
	udpSocketInfo.port=port;
	udpSocketInfo.eventHandler=udpEventHandler;
	
	bDataConsumed=0;
	pthread_create(&udpListenerThread,NULL,gb_manageUDPSocketDatagrams,(void*)&udpSocketInfo);
	while(bDataConsumed==0)
	{
		usleep(5000);
	}
	bDataConsumed=0;
	#ifndef AVOID_THREAD_NAMES
		pthread_setname_np(udpListenerThread,"GB_UDPListener");
	#endif
	pthread_detach(udpListenerThread);
}

void* gb_manageUDPSocketDatagrams(void *params)
{
	int i=0;
	int numBytesReceived;
	char recmsg[200];
	char m_bRun=0;
	GB_UDP_SOCKET_INFO udpSocketInfo=*((GB_UDP_SOCKET_INFO*)params);
	fd_set readFileDescriptorsSet;
	struct timeval fileDescriptorsCheckTimeout;
	int sockfd=udpSocketInfo.fd;
	struct sockaddr_in peeraddr;
	socklen_t socklen;

	//multicast address
	char group_addr[16] = "224.0.0.1";

	socklen=sizeof(struct sockaddr_in);
	memset(&peeraddr, 0, socklen);
	peeraddr.sin_family=AF_INET;
	peeraddr.sin_port=htons(udpSocketInfo.port);
	if(inet_pton(AF_INET, group_addr, &peeraddr.sin_addr) <=0)
	{
		sprintf(debugString,"%s UDP SOCKET BOUND TO PORT %d WRONG IP ADDRESS",PREFIX,udpSocketInfo.port);
		gb_cprintf(debugString,GB_CYAN);
		return NULL;
	}

	//bind
	if(bind(sockfd, (struct sockaddr *)&peeraddr, sizeof(struct sockaddr_in))== -1) 
	{
		sprintf(debugString,"%s UDPSOCKET BOUND TO PORT %d BIND ERROR",PREFIX,udpSocketInfo.port);
		gb_cprintf(debugString,GB_CYAN);
		return NULL;
	}

	fcntl(sockfd, F_SETFL, O_NONBLOCK);

	for(i=0;i<MAX_UDP_SOCKETS;i++)
        {
                if(udp_socket_descriptors[i]==-1)
                {
			//populating remaining data
                        udpSocketInfo.indexInsideGBQueueDescriptors = i;
                	udpSocketInfo.peeraddr                      = peeraddr;
			udpSocketInfo.socklen                       = socklen;
			
			//populating eventhandlers
			udp_socket_descriptors[i]                   = sockfd;
			udpEventHandlers[i]                         = udpSocketInfo.eventHandler;
			
			break;
		}
        }

	sprintf(debugString,"%s UDP SOCKET CREATED AND BOUND TO IPLOCAL_ENDPOINT %s:%d (fd=%d,gb_queueindex=%d))",PREFIX,group_addr,udpSocketInfo.port,udpSocketInfo.fd,udpSocketInfo.indexInsideGBQueueDescriptors);
	gb_cprintf(debugString,GB_YELLOW);

	
	bDataConsumed=1;
	
	m_bRun = 1;
	while (m_bRun)
	{
		int highestFileDescriptorToCheck = udpSocketInfo.fd+1; //+1 is required by poll system call
		fileDescriptorsCheckTimeout.tv_sec = 0;
		fileDescriptorsCheckTimeout.tv_usec = 200000;

		FD_ZERO(&readFileDescriptorsSet);
		FD_SET(udpSocketInfo.fd, &readFileDescriptorsSet);
		
		if (select(highestFileDescriptorToCheck, &readFileDescriptorsSet, NULL, NULL, &fileDescriptorsCheckTimeout) <= 0)
		{
			continue;
		}

		sprintf(debugString,"%s DATA AVAILABLE ON UDP SOCKET BOUND TO PORT %d (fd=%d,gb_queuenumber:%d)",PREFIX,udpSocketInfo.port,udpSocketInfo.fd,udpSocketInfo.indexInsideGBQueueDescriptors);
		gb_cprintf(debugString,GB_YELLOW);

		numBytesReceived = recvfrom(udpSocketInfo.fd, recmsg, sizeof(recmsg),0, (struct sockaddr *)&udpSocketInfo.peeraddr, &udpSocketInfo.socklen); 

		if (numBytesReceived > 0)
		{
			recmsg[numBytesReceived] = 0;
			sprintf(debugString,"%s UDP SOCKET BOUND TO PORT %d RECEIVED %d BYTES:",PREFIX,udpSocketInfo.port,numBytesReceived);
			gb_cprintf(debugString,GB_YELLOW);
			for(i=0;i<numBytesReceived;i++)	
			{
				printf("%02x|",recmsg[i]);
			}
			fflush(stdout);
			if(udpEventHandlers[udpSocketInfo.indexInsideGBQueueDescriptors]!=NULL)
			{
				udpEventHandlers[udpSocketInfo.indexInsideGBQueueDescriptors](recmsg,numBytesReceived,udpSocketInfo);
			}	
		}
		else
		{
			printf("\nError in reading (numbytesreceived=%d)",numBytesReceived);
			fflush(stdout);
		}
	}
	close(udpSocketInfo.fd);
	return NULL;
}
