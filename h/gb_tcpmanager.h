#ifdef __cplusplus
extern "C" {
#endif

#ifndef GBLIB_WEBSOCKET_BUFFER_SIZE 
	#define GBLIB_WEBSOCKET_BUFFER_SIZE 	   2500
#endif

typedef enum
{
        TCP_SERVER=0,
        TCP_CLIENT
}CONNECTION_TYPE;

typedef enum
{
        WEBSOCKET_FRAMETYPE_TEXT 	= 1,
        WEBSOCKET_FRAMETYPE_BINARY 	= 2
}WEBSOCKET_FRAMETYPE;

typedef struct
{
	CONNECTION_TYPE type;
	int  	fd;
	char 	remoteIP[20];
	int  	remotePort;
	int  	indexInsideGBQueueDescriptors;
	char 	bWebSocketHandshakeDone;						/*v0.11*/
	int	    webSocketLastFrameType;							/*v0.11*/
	char    bFirstMessageParsedWhenUsedAsTCPProxy;			/*v0.16*/
	char    pendingBuffer[GBLIB_WEBSOCKET_BUFFER_SIZE];		/*v0.23*/
	int     pendingLen;										/*v0.23*/
}CONNECTION_INFO;

typedef void (*gb_tcpFragmentArrivedEventHandler)(char *data,int n,CONNECTION_INFO *extrainfo);

int  gb_initTCPManager();
int  gb_startTCPServer(int port,gb_tcpFragmentArrivedEventHandler tcpFragmentHandlerPassed);
int  gb_connectToTCPServer(char *ip,int tcpPort,gb_tcpFragmentArrivedEventHandler tcpFragmentHandlerPassed);
int  gb_sendDataOnAllTCPSocketsBoundToTCPServer(int gbTCPManagerId, char *data,int n);
int  gb_sendDataOnAllTCPSocketsBoundToTCPServerExceptOne(int gbTCPManagerId, char *data,int n,int fd); 									/*v0.16*/
int  gb_getSocketBytesAvailable(int sockfd);
char gb_isTCPSocketConnected(int sockfd);
void gb_setFdExclusiveForItsCreator(int sockfd);
int  gb_startWebSocketServer(int port,gb_tcpFragmentArrivedEventHandler tcpFragmentHandlerPassed);
void gb_getSecWebSocketKey(char *data,int n,char *key);
void gb_webSocketServerInternalFragmentEventHandler(char *data,int n,CONNECTION_INFO *extrainfo);
int  gb_encapsulatePlainDataIntoWebSocketData(char *data,int n,int webSocketFrameType,char *optionalmask,char *destination);
int  gb_sendDataOnAllTCPSocketsBoundToWebSocketServer(int gbTCPManagerId, char *data,int n,int webSocketFrameType);
int  gb_sendDataOnAllTCPSocketsBoundToWebSocketServerExceptOne(int gbTCPManagerId, char *data,int n,int webSocketFrameType,int fd); 	/*0.23*/
int  gb_getNumClientsConnectedToTCPServer(int gb_tcpServerID); 																			/*0.15*/

typedef void(*GB_TCP_DISCONNECTION_EVENT_HANDLER)      (CONNECTION_INFO connectionInfo);												/*0.18*/
typedef void(*GB_TCP_SILENCE_LONGTIME_EVENT_HANDLER)   (CONNECTION_INFO connectionInfo);												/*0.18*/
typedef void(*GB_TCP_ACCEPTED_CONNECTION_EVENT_HANDLER)(CONNECTION_INFO connectionInfo);												/*0.19*/
typedef void(*GB_TCP_LISTEN_FAIL_EVENT_HANDLER)		   (int port);																	    /*0.21*/
typedef void(*GB_TCP_HEARTBEAT_EVENT_HANDLER)          (CONNECTION_INFO connectionInfo);                                                /*0.27*/		

#ifndef	GB_TCPMANAGER_MODULE
	extern GB_TCP_DISCONNECTION_EVENT_HANDLER       gb_customTCPDisconnectionEventHandler;												/*0.18*/
	extern GB_TCP_SILENCE_LONGTIME_EVENT_HANDLER    gb_customTCPSilenceLongTimeEventHandler;											/*0.18*/
	extern GB_TCP_ACCEPTED_CONNECTION_EVENT_HANDLER gb_customTCPAcceptedConnectionEventHandler; 										/*0.19*/
	extern GB_TCP_LISTEN_FAIL_EVENT_HANDLER			gb_customTCPLBindFailEventHandler;													/*0.21*/
#endif

void gb_setCustomTCPDisconnectionEventHandler     (GB_TCP_DISCONNECTION_EVENT_HANDLER    gbTcpDisconnectionEventHandlerPassed);			/*0.18*/	
void gb_setCustomTCPSilenceLongTimeEventHandler   (GB_TCP_SILENCE_LONGTIME_EVENT_HANDLER gbTCPSilenceLongTimeEventHandlerPassed);		/*0.18*/
void gb_setCustomTCPAcceptedConnectionEventHandler(GB_TCP_SILENCE_LONGTIME_EVENT_HANDLER gbTCPAcceptedConnectionEventHandlerPassed);	/*0.19*/
void gb_setCustomTCPBindFailEventHandler		  (GB_TCP_LISTEN_FAIL_EVENT_HANDLER      gbTCPBindFailEventHandler);                    /*0.21*/
void gb_setCustomTCPHeatBeatEventHandler(GB_TCP_HEARTBEAT_EVENT_HANDLER gbTCPHeartBeatEventHandler);                                    /*0.27*/			

int  gb_closeAllTCPSocketsBoundToTCPServer(int gbTCPManagerId);                                                                         /*0.18*/
void gb_setMaxTCPContemporaryConnections(int maxTCPContemporaryConnections);                                                            /*0.19*/
void gb_setCustomTCPConnectionTimeout(int timeoutSeconds);                                                                              /*0.19*/
void gb_setCustomTCPConnectionHeartBeatPeriod(int timeoutSeconds);                                                                      /*0.27*/
void gb_getSocketDescription(int sockFD,char *description,char bStringOrder);                                                           /*0.19*/
void gb_setDebugTCPManager(char bValue);                                                                                                /*0.21*/
int gb_getNumWebSocketReconstruction();                                                                                                 /*0.25*/
int gb_getNumWebSocketZeroLength();                                                                                                     /*0.25*/
int gb_getNumWebSocketForcedExits();                                                                                                    /*0.25*/
enum
{
	ORDER_REMOTE_LOCAL=0,
	ORDER_LOCAL_REMOTE=1
};
/******/

/*v0.16*/
typedef struct
{
	int clientTCPServerMgrId;
	int proxyTCPServerMgrId;
	int clientFD;
	int proxyFD;
}REMOTE_PORTFORWARDING_INFO;

void tcpRemotePortForwardingProxyFragmentHandler(char *data,int n,CONNECTION_INFO *extrainfo);
void tcpSingleConnectionRemotePortForwardingRemoteFragmentHandler(char *data,int n,CONNECTION_INFO *extrainfo);
int gb_startTCPRemotePortFormwardingProxy(int tcpPort);
#ifdef __cplusplus
}
#endif
