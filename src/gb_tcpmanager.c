#define _GNU_SOURCE 
#define GB_TCPMANAGER_MODULE        /*0.18*/
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>                 /*v0.11*/
#include <sys/ioctl.h>              /*v0.10*/
#include <poll.h>                   /*v0.10*/
#include <fcntl.h>                  /*v0.10*/
#include <semaphore.h>              /*v0.21*/
#include <errno.h>                  /*v0.26*/
#include "../h/gb_utility.h"
#include "../h/gb_tcpmanager.h"
#include "../h/gb_crypto.h"         /*v0.11*/

static char *PREFIX = "[     GBLIB TCPMANAGER    ]";                                /*0.23*/

static pthread_t tcpserverThread;
static char debugString[1500];                                                      /*0.20*/

#define MAX_TCP_SERVERS 					                   20
#define MAX_TCP_CONNECTIONS_FOR_SINGLE_TCP_SERVER 	       	 1000                   /*0.17*//*0.19*/
#define MAX_TCP_CLIENTS						                   20

static int customLimitedMaxTCPConnectionsForSingleTCPServer =  5;                   /*0.19*/
static int customTCPConnectionTimeoutInSeconds		       	= 60;                   /*0.19*/
static int customTCPConnectionHeartBeatInSecons		       	= 20;                   /*0.27*/

static char bDebugTCPManager = 0;                                                   /*0.21*/

void gb_setDebugTCPManager(char bValue)				
{
	bDebugTCPManager = bValue;
}

/*0.25*/
static int numWebSocketReconstructions=0;
static int numWebSocketZeroLength     =0;
static int numWebSocketForcedExits    =0;

/*0.25*/
int gb_getNumWebSocketReconstruction()
{
	return numWebSocketReconstructions;
}

/*0.25*/
int gb_getNumWebSocketZeroLength()
{
	return numWebSocketZeroLength;
}

/*0.25*/
int gb_getNumWebSocketForcedExits()
{
	return numWebSocketForcedExits;
}

/*******************************************************************************************/
/*      GBLIB TCPSERVERS DATA                                                              */
/*******************************************************************************************/
static int   server_socket_descriptors[MAX_TCP_SERVERS];
static int   n_active_servers = 0;
static int   n_active_connections_per_server[MAX_TCP_SERVERS];
static int   server_tcp_ports[MAX_TCP_SERVERS];
static int   socket_descriptors_dedicated_to_single_clients[MAX_TCP_SERVERS][MAX_TCP_CONNECTIONS_FOR_SINGLE_TCP_SERVER];
static sem_t sem_can_accept[MAX_TCP_SERVERS];
static sem_t sem_connectioninfo_client;														/*0.23*/
static sem_t sem_can_create_newtcpserver;													/*0.26*/

void* gb_tcpServerManageTCPConnections(void *params);
gb_tcpFragmentArrivedEventHandler tcpServerFragmentHandlers[MAX_TCP_SERVERS];
gb_tcpFragmentArrivedEventHandler tcpServerExtraFragmentHandlers[MAX_TCP_SERVERS];
static REMOTE_PORTFORWARDING_INFO remotePortForwardingInfo[MAX_TCP_SERVERS][MAX_TCP_CONNECTIONS_FOR_SINGLE_TCP_SERVER]; /*0.16*/

GB_TCP_DISCONNECTION_EVENT_HANDLER    	 gb_customTCPDisconnectionEventHandler      = NULL; /*0.18*/
GB_TCP_SILENCE_LONGTIME_EVENT_HANDLER 	 gb_customTCPSilenceLongTimeEventHandler    = NULL; /*0.18*/ 
GB_TCP_ACCEPTED_CONNECTION_EVENT_HANDLER gb_customTCPAcceptedConnectionEventHandler = NULL; /*0.19*/
GB_TCP_LISTEN_FAIL_EVENT_HANDLER         gb_customTCPBindFailEventHandler           = NULL; /*0.21*/
GB_TCP_HEARTBEAT_EVENT_HANDLER           gb_customTCPHeartBeatEventHandler          = NULL; /*0.27*/
/*******************************************************************************************/


/*******************************************************************************************/
/*      GBLIB TCPCLIENTS DATA                                                              */
/*******************************************************************************************/
static int n_active_clients = 0;
static int client_socket_descriptors[MAX_TCP_CLIENTS];
void* gb_procedureSingleTCPConnectionThread(void *params);
gb_tcpFragmentArrivedEventHandler tcpClientFragmentHandlers[MAX_TCP_CLIENTS];
/*******************************************************************************************/


/*******************************************************************************************/
/*      GBLIB DEFAULT TCP EVENT HANDLERS                                                   */
/*******************************************************************************************/

/*****************************************************************************/
void gb_tcpServerDefaultTCPFragmentHandler(char *data,int n,CONNECTION_INFO *extrainfo)
{
	int i=0;
    CONNECTION_INFO connectionInfo=*extrainfo;
    char clientIPAndPort[30];
    sprintf(clientIPAndPort,"%s:%d",connectionInfo.remoteIP,connectionInfo.remotePort);
	sprintf(debugString,"%s DEFAULT TCPSERVER FRAGMENT HANDLER FOR SPECIFIC SOCKET CONNECTED TO %s (%d bytes)-NO ACTION",PREFIX,clientIPAndPort,n);
    gb_cprintf(debugString,GB_YELLOW);
	for(i=0;i<n;i++)
	{
		printf("%02x|",data[i]);
	}
	fflush(stdout);
}
/*****************************************************************************/

/*****************************************************************************/
void gb_tcpClientDefaultTCPFragmentHandler(char *data,int n,CONNECTION_INFO *extrainfo)
{
    int i=0;
	CONNECTION_INFO connectionInfo=*extrainfo;
	char clientIPAndPort[30];
	sprintf(clientIPAndPort,"%s:%d",connectionInfo.remoteIP,connectionInfo.remotePort);
	sprintf(debugString,"%s DEFAULT TCPCLIENT FRAGMENT HANDLER FOR SPECIFIC SOCKET CONNECTED TO %s (%d bytes)-NO ACTION",PREFIX,clientIPAndPort,n);
	gb_cprintf(debugString,GB_YELLOW);
	for(i=0;i<n;i++)
	{
			printf("%02x|",data[i]);
	}
	fflush(stdout);
}
/*****************************************************************************/


/*******************************************************************************************/
/*      GBLIB WEBSOCKETS EVENT HANDLERS                                                    */
/*******************************************************************************************/

/*****************************************************************************/
void gb_decodeWebSocketNormalMessage(char *message, char *plainMessage, CONNECTION_INFO *extrainfo)
{
	/*****************************************************************************/
	/* DECODING WEBSOCKET MESSAGE                                                */ 
	/*****************************************************************************/
	int i=0;
	int k=0;
	
	//compute frame type
	char webSocketFrameType=message[0]&0x0F;
	extrainfo->webSocketLastFrameType=webSocketFrameType;
	
	//compute length
	int webSocketFrameLength      = (message[0+1]&0x7F);
	int indexFirstByteAfterLength = 0 + 2;
	if(webSocketFrameLength==126)
	{
		webSocketFrameLength = ((message[0+2]&0xFF)<<8) + (message[0+3]&0xFF);
		indexFirstByteAfterLength = 0 + 4;
	}
	
	if(bDebugTCPManager)
	{
		//draw header
		printf("\e[1;31mWSHEADER:");		
		for(;k<0+indexFirstByteAfterLength;k++)
		{
				printf("%02x|",message[k]&0xFF);
		}
		printf("\e[0m");
	}
	
	//compute mask
	char websocketMask[4];
	for(i=0;i<sizeof(websocketMask);i++)
	{
		websocketMask[i]=message[0+indexFirstByteAfterLength+i];
	}
	
	if(bDebugTCPManager)
	{
		printf("\e[1;32mWSPAYLOAD:");		
		for(k=0+indexFirstByteAfterLength;k<0+indexFirstByteAfterLength+4;k++)
		{
			printf("%02x|",message[k]&0xFF);
		}
		printf("\e[0m");
	}
	
	//decypher message
	for(i=indexFirstByteAfterLength+4;i<indexFirstByteAfterLength+4+webSocketFrameLength/*v0.13*/;i++)
	{
		plainMessage[i-0-indexFirstByteAfterLength-4]=message[i]^websocketMask[(i-0-indexFirstByteAfterLength-4)%4];
	}
	plainMessage[i-0-indexFirstByteAfterLength-4]=0;
	
	if(bDebugTCPManager)
	{
		sprintf(debugString,"%s \e[1;36mIDENTIFIED WEBSOCKET MESSAGE TYPE \"%s\" INSIDE TCP FRAGMENT: PAYLOAD LENGTH=%d BYTES, \e[1;32mMASK=%02x|%02x|%02x|%02x\e[0m: \"%s\"",
					PREFIX,
					(webSocketFrameType==0x02)?"binary":"text",
					webSocketFrameLength,
					websocketMask[0]&0xFF,
					websocketMask[1]&0xFF,
					websocketMask[2]&0xFF,
					websocketMask[3]&0xFF,
					plainMessage);
		gb_cprintf(debugString,GB_WHITE);
	}
}
/*****************************************************************************/

/*****************************************************************************/
void enqueueNewBytesOnWebSocketPendingBuffer(CONNECTION_INFO *dest, char *data, int n)
{
	if(dest!=NULL)
	{
		fflush(stdout);
		if(n<(sizeof(dest->pendingBuffer)-dest->pendingLen))
		{
			memcpy(dest->pendingBuffer+dest->pendingLen, data, n);
			dest->pendingLen += n;
			dest->pendingBuffer[dest->pendingLen]=0;
		}
	}
}
/*****************************************************************************/

/*****************************************************************************/
int extractWebSocketMessage(CONNECTION_INFO *src, char *dest)
{
	int lenToReturn=0;
	if(src!=NULL)
	{
		//websocket is a type-length-value protocol
		//if a pending part is present, surely it is a beginning part
		if(src->pendingLen>0)
		{
			int webSocketPayloadLength = (src->pendingBuffer[1]&0x7F);
			int webSocketFrameLength   = webSocketPayloadLength + 6;
			if (webSocketPayloadLength==126)
			{
				webSocketPayloadLength = ((src->pendingBuffer[2]&0xFF)<<8) + (src->pendingBuffer[3]&0xFF);
				webSocketFrameLength   = webSocketPayloadLength + 8;
			}
			if(src->pendingLen>=webSocketFrameLength)
			{
				memcpy(dest,src->pendingBuffer,webSocketFrameLength);
				lenToReturn = webSocketFrameLength;
				src->pendingLen -= lenToReturn;
				if(src->pendingLen > 0)
				{
					char *strTemp = calloc(src->pendingLen,sizeof(char));
					memcpy(strTemp, src->pendingBuffer+lenToReturn, src->pendingLen);
					memcpy(src->pendingBuffer, strTemp, src->pendingLen);
					src->pendingBuffer[src->pendingLen]=0;
					free(strTemp);
					printf("\n%s \e[1;35m[SOCK:%05d] Extracted WSMessage(%d bytes) but remaining WS message (%d bytes) shifted on buffer beginning\e[0m", PREFIX, src->fd/*0.25*/, lenToReturn/*0.25d*/, src->pendingLen);
					fflush(stdout);
					numWebSocketReconstructions++; /*0.25*/
				}
			}
		}
	}
	return lenToReturn;
}
/*****************************************************************************/

/*****************************************************************************/
void gb_webSocketServerInternalFragmentEventHandler(char *data,int n,CONNECTION_INFO *extrainfo)
{
	int i=0;
	CONNECTION_INFO connectionInfo=*extrainfo;
	char clientIPAndPort[30];
	char secWebSocketKey[30];
	char secWebSocketKeyExtended[100]; /*0.20*/
	char calculatedSHA1[21];
	char *calculatedBASE64SHA1=NULL;
	int base64OutputLength=0;
	char firstResponse[300];

	sprintf(clientIPAndPort,"%s:%d",connectionInfo.remoteIP,connectionInfo.remotePort);
    sprintf(debugString,"\n%s WEBSOCKET FRAGMENT HANDLER FOR SPECIFIC SOCKET CONNECTED TO %s (%d bytes):",PREFIX,clientIPAndPort,n);
    if(bDebugTCPManager)
	{
		gb_cprintf(debugString,GB_WHITE);
    }
	
	if(extrainfo->bWebSocketHandshakeDone==0)
	{
		printf("\n%s \e[1;33m[SOCK:%05d] WEBSOCKET FRAGMENT IS FIRST MESSAGE (PRELIMINARY UPGRADE MSG) (%03d bytes): \e[1;37m\"%.10s...\"\e[0m",PREFIX,extrainfo->fd,n,data);
		fflush(stdout);
		
		/*****************************************************************************/
		/* RAW BUFFER DEBUG PRINTING	     					     */ 
		/*****************************************************************************/
		if(bDebugTCPManager)
		{
			for(i=0;i<n;i++)
			{
					printf("%02x|",data[i]&0xFF);
			}
		}
		/*****************************************************************************/


		/*****************************************************************************/
		/* SEC-WEBSOCKET-KEY IDENTIFICATION AND EXTENSION WITH MAGIC NUMBER	         */ 
		/*****************************************************************************/
		gb_getSecWebSocketKey(data,n,secWebSocketKey);
		sprintf(debugString,"%s [SOCK:%05d] IDENTIFIED SEC-WEBSOCKET-KEY:\"%s\"",PREFIX,extrainfo->fd,secWebSocketKey);
		gb_cprintf(debugString,GB_WHITE);
		sprintf(secWebSocketKeyExtended,"%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11",secWebSocketKey);
		sprintf(debugString,"%s [SOCK:%05d] EXTENDED SEC-WEBSOCKET-KEY WITH MAGIC NUMBER:\"%s\"",PREFIX,extrainfo->fd,secWebSocketKeyExtended);
		gb_cprintf(debugString,GB_WHITE);
		/*****************************************************************************/
		
		
		/*****************************************************************************/
		/* SHA1 OF EXTENDED KEY							     */ 
		/*****************************************************************************/
		//gb_SHA1(calculatedSHA1,"dGhlIHNhbXBsZSBub25jZQ==258EAFA5-E914-47DA-95CA-C5AB0DC85B11",strlen("dGhlIHNhbXBsZSBub25jZQ==258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));
		gb_SHA1(calculatedSHA1,secWebSocketKeyExtended,strlen(secWebSocketKeyExtended));
		sprintf(debugString,"\n%s [SOCK:%05d] CALCULATED SHA1:",PREFIX, extrainfo->fd);
		printf("%s",debugString);
		for(i=0;i<sizeof(calculatedSHA1)-1;i++)
		{
			printf("%02x",calculatedSHA1[i]&0xFF);
		}
		printf("\n");
		/*****************************************************************************/
		
		
		/*****************************************************************************/
		/* BASE64 OF SHA1							     */ 
		/*****************************************************************************/
		calculatedBASE64SHA1 = gb_base64_encode((unsigned char*)calculatedSHA1, sizeof(calculatedSHA1)-1, &base64OutputLength);
		calculatedBASE64SHA1[base64OutputLength]=0;
		if(calculatedBASE64SHA1!=NULL)
		{
			sprintf(debugString,"%s [SOCK:%05d] CALCULATED BASE64: %s",PREFIX,extrainfo->fd,calculatedBASE64SHA1);
			printf("%s",debugString);
		}
		/*****************************************************************************/


		/*****************************************************************************/
		/* SENDING FIRST RESPONSE						     */ 
		/*****************************************************************************/
		sprintf(firstResponse,"HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\nSec-WebSocket-Protocol: binary\r\n\r\n",calculatedBASE64SHA1);
		free(calculatedBASE64SHA1);
		printf("\n%s [SOCK:%05d] SENDING WEBSOCKET HANDSHAKER RESPONSE: \"%s\"\n",PREFIX,extrainfo->fd,bDebugTCPManager==1?firstResponse:"HTTP/1.1...");
		fflush(stdout);		
		send(extrainfo->fd,firstResponse,strlen(firstResponse),MSG_NOSIGNAL/*0.18*/);
		/*****************************************************************************/

		extrainfo->bWebSocketHandshakeDone = 1;
	}
	else
	{	
		/*v0.13*/
		char webSocketFrame[1000];
		char clearWebSocketMessage[1000];
		clearWebSocketMessage[0]=0;
		
		enqueueNewBytesOnWebSocketPendingBuffer(extrainfo,data,n);
		int integerMessageLen  = 0;
		/*0.25*/
		int MAX_WS_EXTRACTIONS =20; 
		int currentWSExtraction= 0;
		/******/
		do
		{
			currentWSExtraction++;
			integerMessageLen = extractWebSocketMessage(extrainfo, webSocketFrame);
		
			if(integerMessageLen>0)
			{
				if(bDebugTCPManager)
				{
					printf(" \e[1;37mEXTRACTED MSG OF LEN=%d\e[0m ",integerMessageLen);
					fflush(stdout);
				}
				gb_decodeWebSocketNormalMessage(webSocketFrame,clearWebSocketMessage,extrainfo);
				if(tcpServerExtraFragmentHandlers[connectionInfo.indexInsideGBQueueDescriptors]!=NULL)
				{
					if(strlen(clearWebSocketMessage)>0) //provvisoria 0.25
					{
						tcpServerExtraFragmentHandlers[connectionInfo.indexInsideGBQueueDescriptors](clearWebSocketMessage,strlen(clearWebSocketMessage),extrainfo);
					}
					/*0.25*/
					else
					{
						numWebSocketZeroLength++;
					}
					/******/
				}
				else
				{
					/*****************************************************************************/
					/* SENDING DEFAULT WEBSOCKET RESPONSE						                 */ 
					/*****************************************************************************/
					char plainWebSocketResponse[1200]; /*0.20*/
					char fullWebSocketResponse[120];	
					sprintf(plainWebSocketResponse,"DEFAULT WEBSOCKET RESPONSE TO \"%s\"",clearWebSocketMessage);	
					int len = /*<=0.24*/ gb_encapsulatePlainDataIntoWebSocketData(plainWebSocketResponse,strlen(plainWebSocketResponse),extrainfo->webSocketLastFrameType,NULL,fullWebSocketResponse);
					send(extrainfo->fd,fullWebSocketResponse,len/*0.24*/,MSG_NOSIGNAL/*0.18*/);
					printf("\n\e[1;37m%s SENDING DEFAULT WEBSOCKET RESPONSE:\e[0m",PREFIX);
					for(i=0;i<strlen(fullWebSocketResponse);i++)
					{
						printf("%02x|",fullWebSocketResponse[i]&0xFF);
					}
					fflush(stdout);
					/*****************************************************************************/
				}
			}
		}while(integerMessageLen>0 && currentWSExtraction<MAX_WS_EXTRACTIONS);
		/*****************************************************************************/
		
		/*0.25*/
		if(currentWSExtraction>=MAX_WS_EXTRACTIONS)
		{
			numWebSocketForcedExits++;
		}
		/******/
	
	}
    fflush(stdout);
}
/*****************************************************************************/

/*****************************************************************************/
void gb_getSecWebSocketKey(char *data,int n,char *key)
{
	char *ptr=strstr(data,"Sec-WebSocket-Key: ");
	if(ptr!=NULL)
	{
		int k=0;
		ptr+=strlen("Sec-WebSocket-Key: ");
		while((*ptr)!='\r' && (*ptr)!='\n' && (*ptr)!=0)
		{
			key[k++]=*ptr;
			ptr++;
		}
		key[k]=0;
	}
}
/*****************************************************************************/

/*****************************************************************************/
int gb_encapsulatePlainDataIntoWebSocketData(char *data,int n,int webSocketFrameType,char *optionalMask,char *destination)
{
	int totalLength=0;
	if(destination!=NULL)
	{
		destination[0]=0x80|webSocketFrameType;
		
		if(n<126)
		{
			
			//snprintf(destination+2,n+1,"%s",data);	/*0.15*/
			if(optionalMask==NULL)
			{
				destination[1]=n;
				sprintf(destination+2,"%s",data); 		
				totalLength=n+2;
			}
			else
			{
				int i=0;
				destination[1]= 0x80 | n;
				//adding mask
				for(i=0;i<4;i++)
				{
					destination[2+i]=optionalMask[i];
				}
				sprintf(destination+6,"%s",data);
				//applying mask
				for(i=0;i<n;i++)
				{
					destination[6+i]=destination[6+i]^optionalMask[i%4];
				}
				totalLength=n+6;
			}
		}
		else
		{
			if(optionalMask==NULL)
			{
				destination[1]=126;
				destination[2]=(n&0xFF00)>>8;
				destination[3]=(n&0xFF);
				sprintf(destination+4,"%s",data);
				totalLength=n+4;
			}
			else
			{
				int i=0;
				destination[1]=0x80 | 126;
				destination[2]=(n&0xFF00)>>8;
				destination[3]=(n&0xFF);
				//adding mask
				for(int i=0;i<4;i++)
				{
					destination[4+i]=optionalMask[i];
				}
				sprintf(destination+8,"%s",data);
				//applying mask
				for(i=0;i<n;i++)
				{
					destination[i+8]=destination[8+i]^optionalMask[i%4];
				}
				totalLength=n+8;
			}
		}
	}
	return totalLength;
}
/*******************************************************************************************/


/*******************************************************************************************/
/*      GBLIB USEFUL FUNCTIONS NOT PRESENT INSIDE GLIBC         v0.10                      */
/*******************************************************************************************/
/*******************************************************************************************/
int gb_getSocketBytesAvailable(int sockfd)
{
	int bytes_available=0;
	ioctl(sockfd,FIONREAD,&bytes_available);
	return bytes_available;
}
/*******************************************************************************************/

/*******************************************************************************************/
char gb_isTCPSocketConnected(int sockfd)
{
	int numFileDescriptorsToPoll=1;	
	int pollTimeout=100; //ms
	int pollResult=0;	
	struct pollfd pollfd_struct;
		

	pollfd_struct.fd      = sockfd;
	pollfd_struct.events  = POLLIN | POLLHUP | POLLRDNORM;
	pollfd_struct.revents = 0;
	
	pollResult = poll(&pollfd_struct,numFileDescriptorsToPoll,pollTimeout);

	if(pollResult>0)
	{
		//socket state has changed in some ways
		int bytesAvailable = gb_getSocketBytesAvailable(sockfd);
		if (bytesAvailable>0)
		{
			//socket state change is due to data arrival, so it is not closed
			return 1;
		}
		else
		{	
			//socket state is due to connection closing
			return 0;
		}
	}
	else
	{
		//poll with no results, socket state has not changed, assume it is still connected
		return 1; 
	}
}
/*******************************************************************************************/

/*******************************************************************************************/
void gb_setFdExclusiveForItsCreator(int sockfd)
{
	fcntl(sockfd,F_SETFD,fcntl(sockfd,F_GETFD)|FD_CLOEXEC); /*this fd should not be used by child process*/ /*v0.10*/
}
/*******************************************************************************************/


/*******************************************************************************************/
/*      GBLIB TCP ENTRY POINTS		                                                       */
/*******************************************************************************************/
int gb_initTCPManager()
{
	int i=0;
	int j=0;
	n_active_servers=0;
	n_active_clients=0;
	sprintf(debugString,"%s INITIALIZING TCPMANAGER MODULE ****************",PREFIX);
	gb_cprintf(debugString,GB_YELLOW);
	
	sem_init(&sem_connectioninfo_client, 0, 1); /*0.23*/
	sem_init(&sem_can_create_newtcpserver,0,1); /*0.26*/
	
	for(i=0;i<MAX_TCP_SERVERS;i++)
	{
		server_socket_descriptors[i]      =-1;
		server_tcp_ports[i]		  = 0;
		n_active_connections_per_server[i] = 0;
		for(j=0;j<MAX_TCP_CONNECTIONS_FOR_SINGLE_TCP_SERVER;j++)
		{
			socket_descriptors_dedicated_to_single_clients[i][j]=-1;
		}
		tcpServerFragmentHandlers[i]=NULL;
		tcpServerExtraFragmentHandlers[i]=NULL;
	}
	for(i=0;i<MAX_TCP_CLIENTS;i++)
	{
			client_socket_descriptors[i]=-1;
			tcpClientFragmentHandlers[i]=NULL;
	}
	return 1;
}	
	
int gb_startTCPServer(int port,gb_tcpFragmentArrivedEventHandler tcpFragmentHandlerPassed)
{
	sem_wait(&sem_can_create_newtcpserver); /*0.26*/
	int i=0;
	int j=0;
	int indexCurrentServerDescriptors=-1;
	for(i=0;i<MAX_TCP_SERVERS;i++)
	{
		if(server_socket_descriptors[i]==-1)
		{
			indexCurrentServerDescriptors       =i;
			server_tcp_ports[i]					=port;
			n_active_connections_per_server[i]  =0;
			for(j=0;j<MAX_TCP_CONNECTIONS_FOR_SINGLE_TCP_SERVER;j++)
			{
				socket_descriptors_dedicated_to_single_clients[i][j]=-1;
			}
			tcpServerFragmentHandlers[i]=tcpFragmentHandlerPassed;
			tcpServerExtraFragmentHandlers[i]=NULL;
			break;
		}
	}
	
	if(indexCurrentServerDescriptors<0)
	{
		sprintf(debugString,"%s ERROR IN CREATING NEW TCPSERVER. Index of descriptors incorrect (<0)",PREFIX);
		gb_cprintf(debugString,GB_RED);
		return -1;	
	}
	else
	{
		if(tcpServerFragmentHandlers[indexCurrentServerDescriptors]==NULL)
		{
			sprintf(debugString,"%s TCPSERVER WARNING: NO TCP CUSTOM FRAGMENT EVENT HANDLER PASSED (TCPSERVER LISTENING ON PORT %d). USING DEFAULT",PREFIX,server_tcp_ports[indexCurrentServerDescriptors]);
			gb_cprintf(debugString,GB_YELLOW);
			tcpServerFragmentHandlers[indexCurrentServerDescriptors]=gb_tcpServerDefaultTCPFragmentHandler;
		}
		else
		{
			sprintf(debugString,"%s REGISTERING CUSTOM EVENT HANDLER FOR TCP FRAGMENT (TCPSERVER LISTENING ON PORT %d)",PREFIX,server_tcp_ports[indexCurrentServerDescriptors]);
			gb_cprintf(debugString,GB_YELLOW);	
		}
		
		int res = 0; /*0.26*/
		#ifdef x64   /*0.20*/
			res=pthread_create(&tcpserverThread,NULL,gb_tcpServerManageTCPConnections,(void*)(long int)indexCurrentServerDescriptors);
		#else
			res=pthread_create(&tcpserverThread,NULL,gb_tcpServerManageTCPConnections,(void*)indexCurrentServerDescriptors);
		#endif
		/*0.26*/
		if(res)
		{
			printf("\n%s \e[1;31m ERROR IN CREATING THREAD DEDICATED TO TCP LISTENER (MANAGE TCP ACCEPTS)\e[0m", PREFIX);
			fflush(stdout);
			sem_post(&sem_can_create_newtcpserver); /*0.26*/
		}
		/******/
		else
		{
			#ifndef AVOID_THREAD_NAMES 
				pthread_setname_np(tcpserverThread,"GB_TCPSrvMGR");
			#endif
			pthread_detach(tcpserverThread);
			
			sem_init(&(sem_can_accept[indexCurrentServerDescriptors]), 0, 1); /*0.21*/
		}
		return indexCurrentServerDescriptors;
	}
}

int gb_startWebSocketServer(int port,gb_tcpFragmentArrivedEventHandler webSocketFragmentHandlerPassed)
{
	int tcpManagerIndexInsideServerDescriptorsArray = gb_startTCPServer(port,gb_webSocketServerInternalFragmentEventHandler);
	if(webSocketFragmentHandlerPassed!=NULL)
	{
		tcpServerExtraFragmentHandlers[tcpManagerIndexInsideServerDescriptorsArray]=(void*)webSocketFragmentHandlerPassed;
		sprintf(debugString,"%s REGISTERING CUSTOM WEBSOCKET EVENT HANDLER FOR TCP FRAGMENT (TCPSERVER LISTENING ON PORT %d)",PREFIX,server_tcp_ports[tcpManagerIndexInsideServerDescriptorsArray]);
		gb_cprintf(debugString,GB_WHITE);
	}
	else
	{
		sprintf(debugString,"%s NO WEBSOCKET CUSTOM FRAGMENT EVENT HANDLER PASSED (TCPSERVER LISTENING ON PORT %d). USING DEFAULT",PREFIX,server_tcp_ports[tcpManagerIndexInsideServerDescriptorsArray]);
		gb_cprintf(debugString,GB_WHITE);
	}
	return tcpManagerIndexInsideServerDescriptorsArray;
}

int gb_connectToTCPServer(char *ip,int port,gb_tcpFragmentArrivedEventHandler tcpFragmentHandlerPassed)
{
	sem_wait(&sem_connectioninfo_client);/*0.23*/
	int i=0;
	static CONNECTION_INFO connectionInfo;
	//struct sockaddr_in clientaddr; //use below for getpeername
	pthread_t clientThread;

	//socket
	int client_socket_descriptor=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	struct sockaddr_in temp;
	temp.sin_family=AF_INET;
	temp.sin_port=htons(port);
	temp.sin_addr.s_addr=inet_addr(ip);
	bzero(&(temp.sin_zero),8);

	//connect
	int result = connect(client_socket_descriptor,(struct sockaddr*)&temp,sizeof(temp));
	
	/*0.25*/
	if(result<0)
	{
		close(client_socket_descriptor);
		client_socket_descriptor=-1;
		sem_post(&sem_connectioninfo_client);
	}
	/******/
	else
	{	
		//set socket exclusive	
		gb_setFdExclusiveForItsCreator(client_socket_descriptor); /*v0.10*/

		//manage connection
		n_active_clients++;
		connectionInfo.type=TCP_CLIENT;
		connectionInfo.fd=client_socket_descriptor;
		socklen_t temp_size=sizeof(temp);
		getpeername(client_socket_descriptor,(struct sockaddr*)&temp,&temp_size);
		sprintf(connectionInfo.remoteIP,"%s",inet_ntoa(temp.sin_addr));
		connectionInfo.remotePort=ntohs(temp.sin_port);

		for(i=0;i<MAX_TCP_CLIENTS;i++)
		{
			if(client_socket_descriptors[i]==-1)
			{
				connectionInfo.indexInsideGBQueueDescriptors 	=i;
				client_socket_descriptors[i]            		=client_socket_descriptor;
				tcpClientFragmentHandlers[i]					=tcpFragmentHandlerPassed;
				break;
			}
		}

		if(tcpClientFragmentHandlers[connectionInfo.indexInsideGBQueueDescriptors]==NULL)
		{
			sprintf(debugString,"%s TCPCLIENT WARNING: NO TCP FRAGMENT ARRIVED HANDLER PASSED",PREFIX);
			gb_cprintf(debugString,GB_YELLOW);
			tcpClientFragmentHandlers[connectionInfo.indexInsideGBQueueDescriptors]=gb_tcpClientDefaultTCPFragmentHandler;
		}
		if(pthread_create(&clientThread,NULL,gb_procedureSingleTCPConnectionThread,&connectionInfo))
		{
			printf("\n%s \e[1;31m ERROR IN CREATING THREAD RELATED TO OUTGOING TCP CLIENT CONNECTION\e[0m", PREFIX);
			fflush(stdout);
			//semaphore release in error conditions for client
			sem_post(&sem_connectioninfo_client);
		}
		#ifndef AVOID_THREAD_NAMES
			pthread_setname_np(clientThread,"GB_TCPConnMGR");
		#endif
		pthread_detach(clientThread);
	}
	return client_socket_descriptor;
}

/*0.16*/
/*******************************************************************************************/
int fdToExclude=-1; 
int gb_sendDataOnAllTCPSocketsBoundToTCPServerExceptOne(int gbTCPManagerId, char *data,int n,int fd)
{
	fdToExclude=fd;
	return gb_sendDataOnAllTCPSocketsBoundToTCPServer(gbTCPManagerId,data,n);
}
/*******************************************************************************************/

/*******************************************************************************************/
int gb_sendDataOnAllTCPSocketsBoundToTCPServer(int gbTCPManagerId, char *data,int n)
{
	int i=0;
	int j=0;
	int returnValue=0;
	if(bDebugTCPManager==1)
	{
		sprintf(debugString,"%s Received data TO TX on TCP socket (GBTCPMGR_ID=%d) if there are some clients connected. Data=",PREFIX,gbTCPManagerId);
		gb_cprintf(debugString,GB_YELLOW);
		for(i=0;i<n;i++)
		{
			printf("%02x|",data[i]);
		}
		fflush(stdout);
	}
	if(gbTCPManagerId<0)
	{
		sprintf(debugString,"%s GBTCPMGR_ID NOT VALID (HAS BEEN PASSED?). NO ACTION",PREFIX);
		gb_cprintf(debugString,GB_RED); 
	}
	else
	{
		if(n_active_connections_per_server[gbTCPManagerId]>0)
		{
			for(i=0;i<MAX_TCP_CONNECTIONS_FOR_SINGLE_TCP_SERVER;i++)
			{
				if(socket_descriptors_dedicated_to_single_clients[gbTCPManagerId][i]>=0 /*0.16*/&& socket_descriptors_dedicated_to_single_clients[gbTCPManagerId][i]!=fdToExclude)
				{
					char clientIP[20];
        			int  clientPort=0;
        			struct sockaddr_in clientaddr;
        			socklen_t clientaddr_size=sizeof(clientaddr);
					getpeername(socket_descriptors_dedicated_to_single_clients[gbTCPManagerId][i],(struct sockaddr*)&clientaddr,&clientaddr_size);
        			sprintf(clientIP,"%s",inet_ntoa(clientaddr.sin_addr));
        			clientPort=ntohs(clientaddr.sin_port);
					if(bDebugTCPManager==1)
					{
						sprintf(debugString,"%s Sending data on TCP socket connected with %s:%d (%d bytes):",PREFIX,clientIP,clientPort,n);
						gb_cprintf(debugString,GB_YELLOW);
						for(j=0;j<n;j++)
						{
							printf("%02x|",data[j]); //v0.9 j instead of i (2019-10-05)
						}		
						fflush(stdout);
					}
					send(socket_descriptors_dedicated_to_single_clients[gbTCPManagerId][i],data,n,MSG_NOSIGNAL/*0.18*/);
					returnValue=1; /*v0.12*/
				}
			}
		}
		else
		{
			sprintf(debugString,"%s No TX on any TCP sockets because no client connected [%d ACTIVE CONNECTIONS]",PREFIX,n_active_connections_per_server[gbTCPManagerId]);
            gb_cprintf(debugString,GB_YELLOW);
		}
	}
	fdToExclude=-1; /*0.16*/
	return returnValue; 
}
/*******************************************************************************************/

/*******************************************************************************************/
int gb_sendDataOnAllTCPSocketsBoundToWebSocketServer(int gbTCPManagerId, char *data,int n,int webSocketFrameType)
{
	char fullWebSocketResponse[5000]; /*0.14 120=>5000*/
	/*0.14*/
	if(n>=sizeof(fullWebSocketResponse))
	{
		n=sizeof(fullWebSocketResponse)-1;	
	}
	/******/	
	int len = /*<=0.24*/ gb_encapsulatePlainDataIntoWebSocketData(data,n,webSocketFrameType,NULL,fullWebSocketResponse);
	return gb_sendDataOnAllTCPSocketsBoundToTCPServer(gbTCPManagerId, fullWebSocketResponse,len/*0.24*/);
}
/*******************************************************************************************/

/*******************************************************************************************/
int gb_sendDataOnAllTCPSocketsBoundToWebSocketServerExceptOne(int gbTCPManagerId, char *data,int n,int webSocketFrameType,int fd)
{
	char fullWebSocketResponse[5000]; /*0.14 120=>5000*/
	/*0.14*/
	if(n>=sizeof(fullWebSocketResponse))
	{
		n=sizeof(fullWebSocketResponse)-1;	
	}
	/******/	
	int len = /*<=0.24*/ gb_encapsulatePlainDataIntoWebSocketData(data,n,webSocketFrameType,NULL,fullWebSocketResponse);
	fdToExclude = fd;
	return gb_sendDataOnAllTCPSocketsBoundToTCPServerExceptOne(gbTCPManagerId, fullWebSocketResponse,len/*0.24*/,fd);
}
/*******************************************************************************************/

void* gb_procedureSingleTCPConnectionThread(void *params)
{
	int i=0;
	CONNECTION_INFO connectionInfo=*((CONNECTION_INFO*)params);
	int socketDescriptor=connectionInfo.fd;
	
	if(connectionInfo.type==TCP_SERVER)
	{
		//semaphore release in normal case for one of the servers
		sem_post(&(sem_can_accept[connectionInfo.indexInsideGBQueueDescriptors])); /*0.21*/
	}
	/*0.23*/
	else if(connectionInfo.type==TCP_CLIENT)
	{
		//semaphore release in normal case for client
		sem_post(&sem_connectioninfo_client);
	}
	
    char dataArrived[2048]; 	/*0.15*/
	struct sockaddr_in clientaddr;
	socklen_t clientaddr_size=sizeof(clientaddr);

	getpeername(socketDescriptor,(struct sockaddr*)&clientaddr,&clientaddr_size);
    sprintf(connectionInfo.remoteIP,"%s",inet_ntoa(clientaddr.sin_addr));
    connectionInfo.remotePort=ntohs(clientaddr.sin_port);
	
    memset(dataArrived,0,sizeof(dataArrived));
	if(connectionInfo.type==TCP_SERVER)
	{
		printf("\n\e[1;32m%s [SOCK:%05d]\e[1;32m CREATED THREAD DEDICATED TO SINGLE TCP CONNECTION FOR %s:%d\e[0m",PREFIX,socketDescriptor,connectionInfo.remoteIP,connectionInfo.remotePort); 	/*0.17*/
	}
	else
	{
		printf("\n\e[1;32m%s [SOCK:%05d]\e[1;32m CREATED THREAD DEDICATED TO CLIENT TCP CONNECTION TO %s:%d\e[0m",PREFIX,socketDescriptor,connectionInfo.remoteIP,connectionInfo.remotePort); 	/*0.17*/
	}
	fflush(stdout);																					 			/*0.17*/

	int silenceCycleNum=0; /*v0.10*/
	int rawCycleNum=0;     /*v0.25*/
	while(1)
    {
        int i=0;
        int numBytesReceived=0;
		
		if(gb_isTCPSocketConnected(socketDescriptor)==0) //it takes 100ms
		{
			printf("\n%s \e[1;31m[SOCK:%05d] SOCKET CLOSED BY PEER %s:%d\e[0m",PREFIX,socketDescriptor,connectionInfo.remoteIP,connectionInfo.remotePort); /*0.17*/
            fflush(stdout);/*0.17*/
			close(socketDescriptor);
			if(gb_customTCPDisconnectionEventHandler!=NULL)
			{	
				gb_customTCPDisconnectionEventHandler(connectionInfo);
			}
            break;
		}
		
		if(gb_getSocketBytesAvailable(socketDescriptor)>0)
		{
			silenceCycleNum=0; /*0.18*/
			numBytesReceived=recv(socketDescriptor,dataArrived,sizeof(dataArrived),0);
			dataArrived[numBytesReceived]=0; /*0.15 not strictly necessary because data are managed as binary, but added to prevent potential programming mistakes*/
		    if(numBytesReceived>0)
		    {
				if(bDebugTCPManager)
				{
					sprintf(debugString,"%s [SOCK:%05d] Received %d bytes from TCP remote socket %s:%d, Data is:",PREFIX,socketDescriptor,numBytesReceived,connectionInfo.remoteIP,connectionInfo.remotePort); //v0.21
					gb_cprintf(debugString,GB_YELLOW);
					for(i=0;i<numBytesReceived;i++)
					{
						printf("%02x|",dataArrived[i]&0xFF); //v0.9 added &0xFF
					}
					fflush(stdout);
				}
				if(connectionInfo.type==TCP_SERVER)
				{
					if(tcpServerFragmentHandlers[connectionInfo.indexInsideGBQueueDescriptors]!=NULL)
					{
						tcpServerFragmentHandlers[connectionInfo.indexInsideGBQueueDescriptors](dataArrived,numBytesReceived,&connectionInfo);
					}
				}
				else
				{
					if(tcpClientFragmentHandlers[connectionInfo.indexInsideGBQueueDescriptors]!=NULL)
					{
						tcpClientFragmentHandlers[connectionInfo.indexInsideGBQueueDescriptors](dataArrived,numBytesReceived,&connectionInfo);
					}
				}
		    }
			else
			{
				printf("\n%s \e[1;43m[SOCK:%05d]\e[0m \e[1;31mREADING ERROR ON SINGLE CLIENT SOCKET. %s:%d. CLOSING THE SINGLE CLIENT SOCKET ...\e[0m",PREFIX,socketDescriptor,connectionInfo.remoteIP,connectionInfo.remotePort);/*0.17*/
				fflush(stdout);/*0.17*/				
				close(socketDescriptor);
				/*0.25*/
				if(gb_customTCPDisconnectionEventHandler!=NULL)
				{	
					gb_customTCPDisconnectionEventHandler(connectionInfo);
				}
				/*******/
				break;
			}
		}
		else
		{
			silenceCycleNum++; 	   /*0.18*/
			if((silenceCycleNum%(customTCPConnectionTimeoutInSeconds*5))==0) /*0.17*/ /*0.19 timeout*5clockseverysecond; example: 60sec= 60x5=300ticks (a tick every 200ms)*/
			{			
				if(bDebugTCPManager)
				{
					printf("\n%s \e[1;43m[SOCK:%05d]\e[0m\t\t\e[1;33mPERIODIC CHECK: NO BYTES AVAILABLE FOR %d SECS ON TCP SOCKET %s:%d\e[0m",PREFIX,socketDescriptor,customTCPConnectionTimeoutInSeconds, connectionInfo.remoteIP,connectionInfo.remotePort); 	/*0.17*/
					fflush(stdout);																									/*0.17*/
				}
				if(gb_customTCPSilenceLongTimeEventHandler!=NULL)
				{
					gb_customTCPSilenceLongTimeEventHandler(connectionInfo);
				}																					
			}
		}
		/*0.25*/
		if((rawCycleNum%200)==0)
		{
			if(bDebugTCPManager)
			{
				printf("\n%s \e[1;34m[SOCK:%05d]\t\tTHREAD DEDICATED TO [SOCK:%05d] ALIVE (RAWCOUNT=%08d)\e[0m",PREFIX,socketDescriptor,socketDescriptor,rawCycleNum);
				fflush(stdout);
			}
		}
		rawCycleNum++;
		if(rawCycleNum>10000000)
		{
			rawCycleNum=0;
		}
		/******/
		/*0.27*/
		if((rawCycleNum%(customTCPConnectionHeartBeatInSecons*5))==0)
		{
			if(bDebugTCPManager)
			{
				printf("\n%s \e[1;43m[SOCK:%05d]\e[0m\t\t\e[1;33mPERIODIC CHECK: TIME TO SEND HEARTBEAT AFTER %d SECS (IF CONFIGURED) %s:%d\e[0m",PREFIX,socketDescriptor,customTCPConnectionTimeoutInSeconds, connectionInfo.remoteIP,connectionInfo.remotePort);
				fflush(stdout);
			}
			if(gb_customTCPHeartBeatEventHandler!=NULL)
			{
				gb_customTCPHeartBeatEventHandler(connectionInfo);
			}	
		}
		/******/
		usleep(100000); //100ms that have to be added to 100ms of gb_isTCPSocketConnected
    }

	if(connectionInfo.type==TCP_SERVER)
	{
		for(i=0;i<MAX_TCP_CONNECTIONS_FOR_SINGLE_TCP_SERVER;i++)
		{
			if(socket_descriptors_dedicated_to_single_clients[connectionInfo.indexInsideGBQueueDescriptors][i]==socketDescriptor)
			{
				socket_descriptors_dedicated_to_single_clients[connectionInfo.indexInsideGBQueueDescriptors][i]=-1;
				n_active_connections_per_server[connectionInfo.indexInsideGBQueueDescriptors]--;
				break;
			}
		}
		sprintf(debugString,"%s [SOCK:%05d] SINGLE TCP CONNECTION WITH %s:%d CLOSED AND STRUCTURES CLEANED [%d ACTIVE CONNECTIONS REMAINING]",PREFIX,connectionInfo.fd,connectionInfo.remoteIP,connectionInfo.remotePort,n_active_connections_per_server[connectionInfo.indexInsideGBQueueDescriptors]);
		gb_cprintf(debugString,GB_YELLOW);
	}
	else
	{
		for(i=0;i<MAX_TCP_CLIENTS;i++)
		{
			if(client_socket_descriptors[i]==socketDescriptor)
			{
					client_socket_descriptors[i]=-1;
					n_active_clients--;
					break;
			}
		}
	}
	return NULL;
}

void* gb_tcpServerManageTCPConnections(void *params)
{
	#ifdef x64 /*0.20*/
		int indexCurrentServerDescriptors=(int)(long int)(params);
	#else
		int indexCurrentServerDescriptors=(int)(params);
	#endif
	
	pthread_t singleClientThread;

	//socket
	int server_socket_descriptor=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	struct sockaddr_in temp;
	/*v0.9*/ //to avoid EADDRINUSE error into bind
	int opt=1;
	setsockopt(server_socket_descriptor,SOL_SOCKET,SO_REUSEADDR,(char*)&opt,sizeof(opt));
	/******/
	/*v0.11*/ //to avoid TIME_WAIT status
	struct linger sl;
	sl.l_onoff = 1;		// non-zero value enables linger option in kernel
	sl.l_linger = 0;	// timeout interval in seconds
	setsockopt(server_socket_descriptor, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));
	/******/

	temp.sin_family=AF_INET;
	temp.sin_port=htons(server_tcp_ports[indexCurrentServerDescriptors]);
	temp.sin_addr.s_addr=INADDR_ANY;
	bzero(&(temp.sin_zero),8);

	//bind
	int result = bind(server_socket_descriptor, (struct sockaddr*)&temp, sizeof(struct sockaddr));
	if(result<0)
	{
		printf("\n\e[1;31m%s ERROR IN BINDING PORT %05d. PORT PROBABLY IN USE (indexCurrentServerDescriptors=%d)\e[0m",PREFIX,server_tcp_ports[indexCurrentServerDescriptors],indexCurrentServerDescriptors);
		fflush(stdout);
		printf("\n\e[1;31m%s ERROR: %d (%s)",PREFIX,errno,strerror(errno));
		fflush(stdout);
		if(gb_customTCPBindFailEventHandler!=NULL)
		{
			gb_customTCPBindFailEventHandler(server_tcp_ports[indexCurrentServerDescriptors]);
		}
		return NULL;
	}

	//listen
	listen(server_socket_descriptor,5);
	server_socket_descriptors[indexCurrentServerDescriptors]=server_socket_descriptor;
	n_active_servers++;
	printf("\n\e[1;33m%s \e[1;43m[SOCK:%05d]\e[0m \e[1;33mTCP SERVER LISTENER CREATED ON PORT %d (TOTAL %d TCPServers INSTANTIATED)\e[0m",PREFIX,server_socket_descriptor,server_tcp_ports[indexCurrentServerDescriptors],n_active_servers); /*0.17*/
	fflush(stdout);/*0.17*/	
	
	//set socket exclusive	
	gb_setFdExclusiveForItsCreator(server_socket_descriptor); /*v0.22*/

	sem_post(&sem_can_create_newtcpserver); /*0.26*/

	while(1)
	{
		CONNECTION_INFO connectionInfo;
		int i=0;
		char clientIP[20];
		int clientPort=0;
		struct sockaddr_in clientaddr;
		socklen_t clientaddr_size=sizeof(clientaddr);		

		sem_wait(&sem_can_accept[indexCurrentServerDescriptors]);

		//accept
		int socket_descriptor_dedicated_to_single_client=accept(server_socket_descriptor,0,0);
		
		//set socket exclusive
		gb_setFdExclusiveForItsCreator(socket_descriptor_dedicated_to_single_client); /*v0.10*/

		getpeername(socket_descriptor_dedicated_to_single_client,(struct sockaddr*)&clientaddr,&clientaddr_size);
		sprintf(clientIP,"%s",inet_ntoa(clientaddr.sin_addr));
		clientPort=ntohs(clientaddr.sin_port);
		if(indexCurrentServerDescriptors>=0)
		{	
			/*0.17*/
			if(n_active_connections_per_server[indexCurrentServerDescriptors]>=customLimitedMaxTCPConnectionsForSingleTCPServer/*0.19*/)
			{
				printf("\n%s \e\[1;43m[SOCK:%05d]\e[0m \e[1;31mIMPOSSIBILE TO ACCEPT NEW TCP CONNECTION. MAX_CLIENT_CONNECTION_REACHED!!!\e[0m",PREFIX,socket_descriptor_dedicated_to_single_client);
				fflush(stdout);
				char connectionRefusedMessage[100];
				sprintf(connectionRefusedMessage,"MAX_CLIENT_CONNECTION_REACHED (%d POSSIBLE CONTEMPORARY CONNECTIONS)",customLimitedMaxTCPConnectionsForSingleTCPServer); 
				send(socket_descriptor_dedicated_to_single_client,connectionRefusedMessage,strlen(connectionRefusedMessage),MSG_NOSIGNAL/*0.18*/);
				usleep(300000);				
				close(socket_descriptor_dedicated_to_single_client);				
				continue;
			}
			/******/
			for(i=0;i<MAX_TCP_CONNECTIONS_FOR_SINGLE_TCP_SERVER;i++)
			{
				if(socket_descriptors_dedicated_to_single_clients[indexCurrentServerDescriptors][i]==-1)
				{	
					socket_descriptors_dedicated_to_single_clients[indexCurrentServerDescriptors][i]=socket_descriptor_dedicated_to_single_client;
					n_active_connections_per_server[indexCurrentServerDescriptors]++;
					break;
				}
			}	
		}
		else
		{
			sprintf(debugString,"%s ERROR IN ACCEPTING NEW TCP CONNECTION REQUEST [%d SINGLE SERVER ACTIVE CONNECTIONS]", PREFIX,n_active_connections_per_server[indexCurrentServerDescriptors]);
            gb_cprintf(debugString,GB_YELLOW);
			continue; /*0.21*/
		}
		
		sprintf(debugString,"\n\n%s [SOCK:%05d] NEW TCP CONNECTION FROM %s:%d ACCEPTED [%d SINGLE SERVER ACTIVE CONNECTIONS]", PREFIX, socket_descriptor_dedicated_to_single_client, clientIP,clientPort,n_active_connections_per_server[indexCurrentServerDescriptors]);/*0.17*/
		gb_cprintf(debugString,GB_YELLOW);

		//manage connection
		connectionInfo.type                                  = TCP_SERVER;
		connectionInfo.fd                                    = socket_descriptor_dedicated_to_single_client;
		connectionInfo.indexInsideGBQueueDescriptors         = indexCurrentServerDescriptors;
		connectionInfo.bWebSocketHandshakeDone               = 0;
		connectionInfo.webSocketLastFrameType                = WEBSOCKET_FRAMETYPE_TEXT;
		connectionInfo.bFirstMessageParsedWhenUsedAsTCPProxy = 0; /*0.16*/
		
		/*0.19*/
		if(gb_customTCPAcceptedConnectionEventHandler!=NULL)
		{
			gb_customTCPAcceptedConnectionEventHandler(connectionInfo);
		}
	
		if(pthread_create(&singleClientThread,NULL,gb_procedureSingleTCPConnectionThread,(void*)&connectionInfo))
		{
			printf("\n%s \e[1;31m ERROR IN CREATING THREAD DEDICATED TO SINGLE CONNECTION\e[0m", PREFIX);
			fflush(stdout);
			//semaphore release in error conditions for server
			sem_post(&sem_can_accept[indexCurrentServerDescriptors]);
		}
		pthread_detach(singleClientThread);
		#ifndef AVOID_THREAD_NAMES
			pthread_setname_np(singleClientThread,"GB_TCPConnMGR");
		#endif
	}
	return NULL;
}

/*0.15*/
int gb_getNumClientsConnectedToTCPServer(int gb_tcpServerID)
{
	return n_active_connections_per_server[gb_tcpServerID];
}
/******/

/*0.16*/
void tcpRemotePortForwardingProxyFragmentHandler(char *data,int n,CONNECTION_INFO *extrainfo)
{
	int i=0;
	CONNECTION_INFO connectionInfo=*extrainfo;
	char clientIPAndPort[30];
	sprintf(clientIPAndPort,"%s:%d",connectionInfo.remoteIP,connectionInfo.remotePort);
	sprintf(debugString,"%s TCPPORTFORWARDING FRAGMENT HANDLER FOR SOCKET CONNECTED TO %s (%d bytes)",PREFIX,clientIPAndPort,n);
	gb_cprintf(debugString,GB_CYAN);
	for(i=0;i<n;i++)
	{
			printf("%02x|",data[i]);
	}
	fflush(stdout);
	if(extrainfo->bFirstMessageParsedWhenUsedAsTCPProxy==0)
	{
		int remoteTCPPortToOpen=0;
		//sscanf(data,"%5d",&remoteTCPPortToOpen);
		remoteTCPPortToOpen=50000;
		if(remoteTCPPortToOpen>0)
		{
			sprintf(debugString,"%s TCPPORTFORWARDING CONNECTED TO %s FIRST MESSAGE. REMOTE TCP PORT TO OPEN IDENTIFIED INSIDE MESSAGE (VALUE=%5d). OPENING DEDICATED THREAD ... ",PREFIX,clientIPAndPort,remoteTCPPortToOpen);
			//if is not alread opened ...         		
			int gbTCPPortForwardingForSpecificTCPPortId = gb_startTCPServer(remoteTCPPortToOpen,tcpSingleConnectionRemotePortForwardingRemoteFragmentHandler);
			for(i=0;i<MAX_TCP_CONNECTIONS_FOR_SINGLE_TCP_SERVER;i++)
			{
				REMOTE_PORTFORWARDING_INFO *currentPortForwardingInfo = &(remotePortForwardingInfo[gbTCPPortForwardingForSpecificTCPPortId][i]);			
				if(currentPortForwardingInfo!=NULL)
				{				
					currentPortForwardingInfo->clientTCPServerMgrId = gbTCPPortForwardingForSpecificTCPPortId;			
					currentPortForwardingInfo->clientFD = -1;				
					currentPortForwardingInfo->proxyFD = extrainfo->fd;
					currentPortForwardingInfo->proxyTCPServerMgrId=extrainfo->indexInsideGBQueueDescriptors;
				}
			}
			extrainfo->bFirstMessageParsedWhenUsedAsTCPProxy=1;
		}
	}
}

void tcpSingleConnectionRemotePortForwardingRemoteFragmentHandler(char *data,int n,CONNECTION_INFO *extrainfo)
{
	int i=0;
	int j=0;
	CONNECTION_INFO connectionInfo=*extrainfo;
	char clientIPAndPort[30];
	sprintf(clientIPAndPort,"%s:%d",connectionInfo.remoteIP,connectionInfo.remotePort);
	sprintf(debugString,"%s TCPPORTFORWARDING SPECIFIC THREAD FRAGMENT HANDLER FOR SOCKET CONNECTED TO %s (%d bytes)",PREFIX,clientIPAndPort,n);
	gb_cprintf(debugString,GB_CYAN);
	if(extrainfo!=NULL)
	{	
		REMOTE_PORTFORWARDING_INFO *currentPortForwardingInfo = NULL;
		for(i=0;i<MAX_TCP_CONNECTIONS_FOR_SINGLE_TCP_SERVER;i++)
		{
			REMOTE_PORTFORWARDING_INFO *currentPortForwardingInfo = &(remotePortForwardingInfo[extrainfo->indexInsideGBQueueDescriptors][i]);
			if(currentPortForwardingInfo!=NULL)
			{		
				if(currentPortForwardingInfo->clientFD==-1)
				{
					currentPortForwardingInfo->clientFD=extrainfo->fd;
					break;
				}
				else if(currentPortForwardingInfo->clientFD==extrainfo->fd)
				{
					break;
				}
			}
		}
		printf("\nHere");
		fflush(stdout);
		if(currentPortForwardingInfo!=NULL)
		{
			char clientIP[20];
			int  clientPort=0;
			struct sockaddr_in clientaddr;
			socklen_t clientaddr_size=sizeof(clientaddr);
			getpeername(currentPortForwardingInfo->proxyFD,(struct sockaddr*)&clientaddr,&clientaddr_size);
			sprintf(clientIP,"%s",inet_ntoa(clientaddr.sin_addr));
			clientPort=ntohs(clientaddr.sin_port);
			sprintf(debugString,"%s Sending data on TCP socket connected with %s:%d (%d bytes):",PREFIX,clientIP,clientPort,n);
			gb_cprintf(debugString,GB_YELLOW);
			for(j=0;j<n;j++)
			{
				printf("%02x|",data[j]); //v0.9 j instead of i (2019-10-05)
			}		
			fflush(stdout);
			send(currentPortForwardingInfo->proxyFD,data,n,MSG_NOSIGNAL/*0.18*/);
		}
	}
}

int gb_startTCPRemotePortFormwardingProxy(int tcpPort)
{
	int gbTCPPortForwardingId = gb_startTCPServer(tcpPort,tcpRemotePortForwardingProxyFragmentHandler);
	for(int i=0;i<MAX_TCP_CONNECTIONS_FOR_SINGLE_TCP_SERVER;i++)
	{
		remotePortForwardingInfo[gbTCPPortForwardingId][i].clientTCPServerMgrId = -1;
		remotePortForwardingInfo[gbTCPPortForwardingId][i].proxyTCPServerMgrId  = -1;
		remotePortForwardingInfo[gbTCPPortForwardingId][i].clientFD		= -1;
		remotePortForwardingInfo[gbTCPPortForwardingId][i].proxyFD		= -1;
	}
	return gbTCPPortForwardingId;
}
/******/

/*0.18*/
void gb_setCustomTCPDisconnectionEventHandler(GB_TCP_DISCONNECTION_EVENT_HANDLER gbTcpDisconnectionEventHandlerPassed)
{
	gb_customTCPDisconnectionEventHandler = gbTcpDisconnectionEventHandlerPassed;
}

/*0.18*/
void gb_setCustomTCPSilenceLongTimeEventHandler(GB_TCP_SILENCE_LONGTIME_EVENT_HANDLER gbTCPSilenceLongTimeEventHandlerPassed)
{
	gb_customTCPSilenceLongTimeEventHandler = gbTCPSilenceLongTimeEventHandlerPassed;
}

/*0.19*/
void gb_setCustomTCPAcceptedConnectionEventHandler(GB_TCP_ACCEPTED_CONNECTION_EVENT_HANDLER gbTCPAcceptedConnectionEventHandlerPassed)
{
	gb_customTCPAcceptedConnectionEventHandler = gbTCPAcceptedConnectionEventHandlerPassed;
}

/*0.21*/
void gb_setCustomTCPBindFailEventHandler(GB_TCP_LISTEN_FAIL_EVENT_HANDLER gbTCPBindFailEventHandler)
{
	gb_customTCPBindFailEventHandler = gbTCPBindFailEventHandler;
}

/*0.27*/
void gb_setCustomTCPHeatBeatEventHandler(GB_TCP_HEARTBEAT_EVENT_HANDLER gbTCPHeartBeatEventHandler)
{
	gb_customTCPHeartBeatEventHandler = gbTCPHeartBeatEventHandler;
}

/*0.18*/
int gb_closeAllTCPSocketsBoundToTCPServer(int gbTCPManagerId)
{
	int i=0;
	int returnValue=0;
	if(gbTCPManagerId<0)
	{
		sprintf(debugString,"%s GBTCPMGR_ID NOT VALID (HAS BEEN PASSED?). NO ACTION",PREFIX);
		gb_cprintf(debugString,GB_RED); 
	}
	else
	{
		if(n_active_connections_per_server[gbTCPManagerId]>0)
		{
			for(i=0;i<MAX_TCP_CONNECTIONS_FOR_SINGLE_TCP_SERVER;i++)
			{
				if(socket_descriptors_dedicated_to_single_clients[gbTCPManagerId][i]>=0)
				{	
					close(socket_descriptors_dedicated_to_single_clients[gbTCPManagerId][i]);
				}
			}
		}
		else
		{
			sprintf(debugString,"%s GLOBAL CLOSING CONNECTION NOT TAKEN BECAUSE NO CLIENTS CONNECTED [%d ACTIVE CONNECTIONS]",PREFIX,n_active_connections_per_server[gbTCPManagerId]);
                	gb_cprintf(debugString,GB_YELLOW);
		}
	}

	fdToExclude=-1;

	return returnValue; 
}
/*0.19*/
void gb_setMaxTCPContemporaryConnections(int maxTCPContemporaryConnections)
{
	if(customLimitedMaxTCPConnectionsForSingleTCPServer>MAX_TCP_CONNECTIONS_FOR_SINGLE_TCP_SERVER)
	{
		customLimitedMaxTCPConnectionsForSingleTCPServer=MAX_TCP_CONNECTIONS_FOR_SINGLE_TCP_SERVER;
	}
	customLimitedMaxTCPConnectionsForSingleTCPServer=maxTCPContemporaryConnections;
}

/*0.19*/
void gb_setCustomTCPConnectionTimeout(int timeoutSeconds)
{
	customTCPConnectionTimeoutInSeconds=timeoutSeconds;
}

/*0.27*/
void gb_setCustomTCPConnectionHeartBeatPeriod(int timeoutSeconds)
{
	customTCPConnectionHeartBeatInSecons=timeoutSeconds;
}

/*0.19*/
void gb_getSocketDescription(int sockFD, char *description, char bStringOrder)
{
	struct sockaddr_in temp;
	socklen_t temp_size=sizeof(temp);
	bzero(&(temp.sin_zero),8);

	if(description!=NULL)
	{  	
		if(bStringOrder==ORDER_REMOTE_LOCAL)
		{
			getpeername(sockFD,(struct sockaddr*)&temp,&temp_size);
			sprintf(description,"R:%.15s:%05d", inet_ntoa(temp.sin_addr),ntohs(temp.sin_port));
			getsockname(sockFD,(struct sockaddr*)&temp,&temp_size);
			sprintf(description+strlen(description),"-->L:%.15s:%05d:", inet_ntoa(temp.sin_addr),ntohs(temp.sin_port));
		}
		else if(bStringOrder==ORDER_LOCAL_REMOTE)
		{
			getsockname(sockFD,(struct sockaddr*)&temp,&temp_size);
			sprintf(description,"L:%.15s:%05d:", inet_ntoa(temp.sin_addr),ntohs(temp.sin_port));
			getpeername(sockFD,(struct sockaddr*)&temp,&temp_size);	
			sprintf(description+strlen(description),"-->R:%.15s:%05d", inet_ntoa(temp.sin_addr),ntohs(temp.sin_port));
		} 
	}
}
/******/
