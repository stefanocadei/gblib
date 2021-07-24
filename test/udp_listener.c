#include <stdio.h>
#include <string.h>
#include "../h/gb_utility.h"
#include "../h/gb_udpmanager.h"

static char *PREFIX="[ GBLIB UDPTEST]";
static char debugString[200];

void udpEventHandler1(char *data,int n,GB_UDP_SOCKET_INFO udpSocketInfo)
{
	int i=0;
        char answer[30];
	sprintf(debugString,"%s UDP EVENT HANDLER 1 - %d BYTES RECEIVED ('%s') - MESSAGE CHECKING:",PREFIX,n,data);
        gb_cprintf(debugString,GB_CYAN);
        for(i=0;i<n;i++)
        {
                printf("%02x|",data[i]);
        }
        fflush(stdout);
        if(memcmp(data,"DISCOVERY_MSG",n)==0)
        {
                sprintf(debugString,"%s DISCOVERY MGS IDENTIFIED. SENDING ACKNOWLEDGE !!!",PREFIX);
                gb_cprintf(debugString,GB_CYAN);
                sprintf(answer, "%s","DISCOVERY_ACK");
                sendto(udpSocketInfo.fd, answer, strlen(answer),0, (struct sockaddr *)&udpSocketInfo.peeraddr, udpSocketInfo.socklen);
        }
	else
	{
		sprintf(debugString,"%s UNKNOWN MSG - NO ANSWER",PREFIX);
                gb_cprintf(debugString,GB_CYAN);
	}
}


void udpEventHandler2(char *data,int n,GB_UDP_SOCKET_INFO udpSocketInfo)
{
        int i=0;
        char answer[30];
        sprintf(debugString,"%s UDP EVENT HANDLER 2 - %d BYTES RECEIVED ('%s') - MESSAGE CHECKING:",PREFIX,n,data);
        gb_cprintf(debugString,GB_CYAN);
        for(i=0;i<n;i++)
        {
                printf("%02x|",data[i]);
        }
        fflush(stdout);
        if(memcmp(data,"DISCOVERY_MSG",n)==0)
        {
                sprintf(debugString,"%s DISCOVERY MGS IDENTIFIED. SENDING ACKNOWLEDGE !!!",PREFIX);
                gb_cprintf(debugString,GB_CYAN);
                sprintf(answer, "%s","DISCOVERY_ACK");
                sendto(udpSocketInfo.fd, answer, strlen(answer),0, (struct sockaddr *)&udpSocketInfo.peeraddr, udpSocketInfo.socklen);
        }
        else
        {
                sprintf(debugString,"%s UNKNOWN MSG - NO ANSWER",PREFIX);
                gb_cprintf(debugString,GB_CYAN);
        }
}


int main()
{
	sprintf(debugString,"%s UDP TESTER PROGRAM ********************************",PREFIX);
	gb_cprintf(debugString,GB_CYAN);
	
	gb_initUDPManager();
	gb_startUDPMulticastListener(3000,udpEventHandler1);
	gb_startUDPMulticastListener(3001,udpEventHandler2);	
	
	sprintf(debugString,"%s AFTER UDP SERVER INSTANTIATION",PREFIX);
	gb_cprintf(debugString,GB_CYAN);
	getchar();
	return 1;
}
