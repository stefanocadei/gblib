#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "../h/gb_utility.h"
#include "../h/gb_serialapi.h"
#include "../h/gb_serialmanager.h"
#include "../h/gb_tcpmanager.h"

#ifndef BRAND
    #define BRAND   "GBLIBTEST"
#endif

static char PREFIX[30];
static char PREFIX_SPACE[30];
static char debugString[200];

int tcpMgrId=-1;
int tcpMgrId2=-1;

void mainCustomSerialEventHandler(char *data, int n)
{
	sprintf(debugString,"%s CUSTOM SERIAL EVENT HANDLER (%d bytes). RELAYING DATA TO TCPMANAGER: %s",PREFIX,n,data);
	gb_cprintf(debugString,GB_CYAN);
	gb_sendDataOnAllTCPSocketsBoundToTCPServer(tcpMgrId,data,n);
	if(n==1 && data[0]=='a')
	{
		gb_sendDataOnAllTCPSocketsBoundToTCPServer(tcpMgrId2,data,n);
	}
}

void mainCustomTCPFragmentHandler(char *data,int n,CONNECTION_INFO *extrainfo)
{
        int i=0;
        CONNECTION_INFO connectionInfo=*extrainfo;
        char clientIPAndPort[30];
        sprintf(clientIPAndPort,"%s:%d",connectionInfo.remoteIP,connectionInfo.remotePort);
        sprintf(debugString,"%s TCPCLIENT FRAGMENT HANDLER FOR SPECIFIC SOCKET CONNECTED TO %s (%d bytes).Relaying to serial...",PREFIX,clientIPAndPort,n);
        gb_cprintf(debugString,GB_CYAN);
        for(i=0;i<n;i++)
        {
                printf("%02x|",data[i]);
        }
        fflush(stdout);
        gb_serialTX(gb_serialGetPortFd(),data,n);
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


	if(argc<6 || atoi(argv[6])==0)
	{
		gb_startSerialManager(argv[1],atoi(argv[2]),atoi(argv[3]),argv[4][0],atoi(argv[5]),NULL);
	}
	else
	{
		gb_startSerialManager(argv[1],atoi(argv[2]),atoi(argv[3]),argv[4][0],atoi(argv[5]),mainCustomSerialEventHandler);
	}

	gb_initTCPManager();	
	tcpMgrId=gb_startTCPServer(2000,mainCustomTCPFragmentHandler);
	tcpMgrId2=gb_startTCPServer(2001,NULL);	

	while(1)
	{
		if(gb_kbhit())
		{
			int characterPressed=gb_getch();
			char serialMsg[2];
			int serialFd=gb_serialGetPortFd();
			sprintf(debugString,"%s KEY '%c' PRESSED",PREFIX,characterPressed);
            		gb_cprintf(debugString,GB_MAGENTA);
			serialMsg[0]=characterPressed;
			serialMsg[1]=0;
			gb_serialTX(serialFd,serialMsg,sizeof(serialMsg));
		}
		usleep(50000);
	} 

	return 1;
}



