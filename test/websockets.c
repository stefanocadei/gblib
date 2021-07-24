#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "../h/gb_utility.h"
#include "../h/gb_serialapi.h"
#include "../h/gb_serialmanager.h"
#include "../h/gb_tcpmanager.h"
#include "../h/gb_udpmanager.h"
#include "../h/gb_crypto.h"

#ifndef BRAND
    #define BRAND   "GBWSTEST "
#endif

static char PREFIX[30];
static char PREFIX_SPACE[30];
static char debugString[300];

static int webSocketMgrId=-1;


void customWebSocketFragmentArrivedEventHandler(char *data,int n,CONNECTION_INFO *extrainfo)
{
	int i=0;
	CONNECTION_INFO connectionInfo = *((CONNECTION_INFO*)extrainfo);
	/*****************************************************************************/
	/* SENDING WEBSOCKET RESPONSE						     					 */ 
	/*****************************************************************************/
	char plainWebSocketResponse[GBLIB_WEBSOCKET_BUFFER_SIZE];
	char fullWebSocketResponse[GBLIB_WEBSOCKET_BUFFER_SIZE];	
	sprintf(plainWebSocketResponse,"R %s",data);	
	int len = gb_encapsulatePlainDataIntoWebSocketData(plainWebSocketResponse,strlen(plainWebSocketResponse),connectionInfo.webSocketLastFrameType,NULL,fullWebSocketResponse);
	send(connectionInfo.fd,fullWebSocketResponse,len,0);
	printf("\n\e[1;32m%s SENDING CUSTOM WEBSOCKET RESPONSE OF %d BYTES:\e[0m",PREFIX,len);
	
	int stopHeaderIndex = (fullWebSocketResponse[1]<126?2:4)+0; //trailing zero means answer has no mask
	for(i=0;i<len;i++)
	{
		if(i<stopHeaderIndex)
		{
			printf("\e[1;31m");
		}
		if(i==stopHeaderIndex)
		{
			printf("\e[0m");
		}
		if(i<stopHeaderIndex)
		{
			printf("%02x|",fullWebSocketResponse[i]&0xFF);
		}
		else
		{
			if((fullWebSocketResponse[i]&0xFF)<0x20)
			{
				printf("%02x|",fullWebSocketResponse[i]&0xFF);
			}
			else
			{
				printf("%c",fullWebSocketResponse[i]&0xFF);
			}
		}
	}
	fflush(stdout);
	/*****************************************************************************/
}

int main(int argc,char *argv[])
{
	int i=0;
	char libversion[5];
	gb_getGBLIBVersion(libversion);
    sprintf(PREFIX,"[ %s MAIN]",BRAND);
	memset(PREFIX_SPACE,0,sizeof(PREFIX_SPACE));
	for(i=0;i<strlen(PREFIX);i++)
	{
		PREFIX_SPACE[i]=' ';
	}
	sprintf(debugString,"%s LOADED [LIBS %s] ***************************",PREFIX,libversion);
	gb_cprintf(debugString,GB_CYAN);

	//instantiating gbWebSocketServer
	gb_initTCPManager();	
	webSocketMgrId=gb_startWebSocketServer(30001,customWebSocketFragmentArrivedEventHandler);

	//standard input keyboard manager 
	while(1)
	{
		if(gb_kbhit())
		{
			int characterPressed=gb_getch();
			char asynchronousMessage[100];
			
			sprintf(debugString,"%s KEY '%c' PRESSED",PREFIX,characterPressed);
            gb_cprintf(debugString,GB_MAGENTA);
			
			if(characterPressed=='1')
			{	
				sprintf(asynchronousMessage,"%s","WEBSOCKET SERVER TEXT ASYNCHRONOUS EVENT");
				gb_sendDataOnAllTCPSocketsBoundToWebSocketServer(webSocketMgrId,asynchronousMessage,strlen(asynchronousMessage),WEBSOCKET_FRAMETYPE_TEXT);
			}
			else if(characterPressed=='2')
			{
				sprintf(asynchronousMessage,"%s","WEBSOCKET SERVER BINARY ASYNCHRONOUS EVENT");
				gb_sendDataOnAllTCPSocketsBoundToWebSocketServer(webSocketMgrId,asynchronousMessage,strlen(asynchronousMessage),WEBSOCKET_FRAMETYPE_BINARY);
			}				
		}
		usleep(50000);
	} 
	return 1;
}



