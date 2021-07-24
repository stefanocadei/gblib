#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "../h/gb_utility.h"
#include "../h/gb_serialapi.h"
#include "../h/gb_serialmanager.h"
#include "../h/gb_tcpmanager.h"
#include "../h/gb_udpmanager.h"

#ifndef BRAND
    #define BRAND   "GBLIBTEST"
#endif

static char PREFIX[30];
static char PREFIX_SPACE[30];
static char debugString[200];

int tcpMgrId=-1;

void getDeviceID(char *strDeviceID)
{
    char prefix[5];
    char tmpStr[10];
    char bIsARM=0;
    char command[200];
    char strMacAddress[20];
    FILE *file=NULL;

    system("uname -a | grep arm | wc -l > /tmp/isarm.txt");

    file=fopen("/tmp/isarm.txt","rb");
    if(file!=NULL)
    {
        fgets(tmpStr,sizeof(tmpStr),file);
        bIsARM=1;
        fclose(file);
    }

    if(!bIsARM)
    {
        sprintf(command,"%s","echo imx6 | sudo -S ifconfig | grep ether | awk {'print $2'} > /tmp/macaddress.txt");
        sprintf(prefix,"%s","X86_");
    }
    else
    {
        sprintf(command,"%s","sudo ifconfig | grep HWaddr | awk {'print $5'} > /tmp/macaddress.txt");
        sprintf(prefix,"%s","ARM_");
    }
    gb_cprintf(command,GB_CYAN);
    system(command);

    file=fopen("/tmp/macaddress.txt","rb");
    if(file!=NULL)
    {
        fgets(strMacAddress,sizeof(strMacAddress),file);
        fclose(file);
    }

    if(strDeviceID!=NULL)
    {
        sprintf(strDeviceID,"%s%s",prefix,strMacAddress);
    }
}

void serialEventHandler(char *data, int n)
{
	sprintf(debugString,"%s CUSTOM SERIAL EVENT HANDLER (%d bytes). RELAYING DATA TO TCPMANAGER: %s",PREFIX,n,data);
	gb_cprintf(debugString,GB_CYAN);
	gb_sendDataOnAllTCPSocketsBoundToTCPServer(tcpMgrId,data,n);
}

void tcpEventHandler(char *data,int n,CONNECTION_INFO *extrainfo)
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

void udpEventHandler(char *data,int n,GB_UDP_SOCKET_INFO udpSocketInfo)
{
        int i=0;
        char answer[100];
	char parameter[100];
	FILE *file=NULL;
        sprintf(debugString,"%s UDP EVENT HANDLER - %d BYTES RECEIVED ('%s') - MESSAGE CHECKING:",PREFIX,n,data);
        gb_cprintf(debugString,GB_CYAN);
        for(i=0;i<n;i++)
        {
                printf("%02x|",data[i]);
        }
        fflush(stdout);
        if(memcmp(data,"DISCOVERY_MSG",strlen("DISCOVERY_MSG"))==0)
        {
		char deviceID[50];
		int k=0;
		char *tmp=data+strlen("DISCOVERY_MSG")+1;
		while(*tmp!='E' && k<sizeof(parameter)-1)
		{
			parameter[k++]=*tmp;
			tmp++;
		}
		parameter[k-1]=0;
		sprintf(debugString,"%s DISCOVERY MGS IDENTIFIED. PARAMETER_PASSED=\"%s\".SENDING ACKNOWLEDGE !!!",PREFIX,parameter);
                gb_cprintf(debugString,GB_CYAN);
		file = fopen("/tmp/supervisorip.txt","wb");
		if(file!=NULL)
		{
			fputs(parameter,file);
			fclose(file);
		}
		getDeviceID(deviceID);                
		sprintf(answer, "DISCOVERY_ACK_%s_END",deviceID);
		sendto(udpSocketInfo.fd, answer, strlen(answer),0, (struct sockaddr *)&udpSocketInfo.peeraddr, udpSocketInfo.socklen);
        }
        else
        {
                sprintf(debugString,"%s UNKNOWN MSG - NO ANSWER",PREFIX);
                gb_cprintf(debugString,GB_CYAN);
        }
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

	
	//instantiating gbSerialManager
	if(argc<6 || atoi(argv[6])==0)
	{
		gb_startSerialManager(argv[1],atoi(argv[2]),atoi(argv[3]),argv[4][0],atoi(argv[5]),NULL);
	}
	else
	{
		gb_startSerialManager(argv[1],atoi(argv[2]),atoi(argv[3]),argv[4][0],atoi(argv[5]),serialEventHandler);
	}

	//instantiating gbTCPServer
	gb_initTCPManager();	
	tcpMgrId=gb_startTCPServer(2000,tcpEventHandler);

	//instantiating gbUDPListener
	gb_initUDPManager();
	gb_startUDPMulticastListener(3000,udpEventHandler);
	
	//standard input keyboard manager 
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



