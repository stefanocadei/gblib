#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "../h/gb_utility.h"
#include "../h/gb_serialapi.h"
#include "../h/gb_serialmanager.h"
#include "../h/gb_tcpmanager.h"
#include <sys/socket.h>

#ifndef BRAND
    #define BRAND   "GBLIBCLNT "
#endif

static char PREFIX[30];
static char PREFIX_SPACE[30];
static char debugString[200];

CONNECTION_INFO globalConnectionInfo;

void mainCustomTCPFragmentHandler(char *data,int n,CONNECTION_INFO *extrainfo)
{
        int i=0;
        CONNECTION_INFO connectionInfo=*extrainfo;
	globalConnectionInfo=connectionInfo;
        char clientIPAndPort[30];
        sprintf(clientIPAndPort,"%s:%d",connectionInfo.remoteIP,connectionInfo.remotePort);
        sprintf(debugString,"%s CLIENT1 TCPCLIENT FRAGMENT HANDLER FOR SPECIFIC SOCKET CONNECTED TO %s (%d bytes)-ECHO ACTION",PREFIX,clientIPAndPort,n);
        gb_cprintf(debugString,GB_CYAN);
        for(i=0;i<n;i++)
        {
                printf("%02x|",data[i]);
        }
        fflush(stdout);
	//echo
	send(connectionInfo.fd,data,n,0);
}

void mainCustomTCPFragmentHandler2(char *data,int n,CONNECTION_INFO *extrainfo)
{
        int i=0;
        CONNECTION_INFO connectionInfo=*extrainfo;
        char clientIPAndPort[30];
        sprintf(clientIPAndPort,"%s:%d",connectionInfo.remoteIP,connectionInfo.remotePort);
        sprintf(debugString,"%s CLIENT2 TCPCLIENT FRAGMENT HANDLER FOR SPECIFIC SOCKET CONNECTED TO %s (%d bytes)-NO ACTION",PREFIX,clientIPAndPort,n);
        gb_cprintf(debugString,GB_BLUE);
        for(i=0;i<n;i++)
        {
                printf("%02x|",data[i]);
        }
        fflush(stdout);
}



int main(int argc,char *argv[])
{
	int i=0;
	char libversion[5];
	gb_getGBLIBVersion(libversion);
    	sprintf(PREFIX,"[ %s ]",BRAND);
	memset(PREFIX_SPACE,0,sizeof(PREFIX_SPACE));
	for(i=0;i<strlen(PREFIX);i++)
	{
		PREFIX_SPACE[i]=' ';
	}
	sprintf(debugString,"%s LOADED [LIBS %s] ***************************",PREFIX,libversion);
	gb_cprintf(debugString,GB_CYAN);

	gb_initTCPManager();
	gb_connectToTCPServer(argv[1],2000,mainCustomTCPFragmentHandler);
	gb_connectToTCPServer(argv[1],2001,mainCustomTCPFragmentHandler2);

	while(1)
	{
		if(gb_kbhit())
		{
			int characterPressed=gb_getch();
			char tcpMsg[2];
			tcpMsg[0]=characterPressed;
			tcpMsg[1]=0;
			sprintf(debugString,"%s KEY '%c' PRESSED",PREFIX,characterPressed);
            		gb_cprintf(debugString,GB_MAGENTA);
			send(globalConnectionInfo.fd,tcpMsg,1,0);
		}
		usleep(50000);
	} 

	return 1;
}



