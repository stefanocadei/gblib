#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include "../h/gb_serialapi.h"
#include "../h/gb_utility.h"
#include <pthread.h>
#include <unistd.h>

#ifndef BRAND
	#define BRAND "GBLIB"
#endif

static char PREFIX[30];
void* procedureSerialManagement(void *params);
typedef void (*gb_serialEventCallback) (char* buffer,int n); 
gb_serialEventCallback mycallback=NULL;

static char bSomethingToTX=0;
static char bufferToTX[100];
static int  lenToTX=0;
static int  globalSerialPollTime=0;
static char bDebugSerial = 1; /*0.28*/

/*0.28*/
void gb_setDebugSerialManager(char bDebug)
{
	bDebugSerial = bDebug;
}

void gb_setSerialManagerCallback(gb_serialEventCallback coreCallback)
{
	mycallback=coreCallback;
}

void gb_setSerialCustomPollTime(int timeMilliseconds)
{
	globalSerialPollTime=timeMilliseconds;
}

void gb_defaultSerialEvent(char *data, int n)
{
	int i=0;
	printf("\n%s DEFAULT SERIAL EVENT HANDLER (%d bytes)-NO ACTION:",PREFIX,n);
	if(data!=NULL)
	{
		for(i=0;i<n;i++)
		{
			printf("%02x|",data[i]);
		}	
		fflush(stdout);
	}
}

void setValueToTX(char *buffer,int len)
{
	memset(bufferToTX,0,sizeof(bufferToTX));
	memcpy(bufferToTX,buffer,len);
	lenToTX=len;
	bSomethingToTX=1;
}

char isThereSomethingToTX()
{
	return bSomethingToTX;
}

static pthread_t serialThread;
static char serialName[100];
static int  serialBaudrate=0;
static char serialParams[3];

void gb_startSerialManager(char *serialNamePassed,int baudrate,char databits,char parity,char stopbits,void *customEventHandlerPointer)
{
	sprintf(PREFIX,"[ %s SERIAL  ]",BRAND);
	if(customEventHandlerPointer==NULL)
	{
		mycallback=gb_defaultSerialEvent;
	}
	else
	{
		mycallback=(gb_serialEventCallback)customEventHandlerPointer;
	}
	if(serialNamePassed!=NULL)
	{
		sprintf(serialName,"%s",serialNamePassed);
		serialBaudrate=baudrate;
		serialParams[0]=databits;
		serialParams[1]=parity;
		serialParams[2]=stopbits;
		pthread_create(&serialThread,NULL,procedureSerialManagement,NULL);
		#ifndef AVOID_THREAD_NAMES
			pthread_setname_np(serialThread,"GBSerialThread");
		#endif
		pthread_detach(serialThread);
	}
}

void* procedureSerialManagement(void *params)
{
    int fd=-1;
    int bytesAvailable=0;
    char readBytes[10000];
    char debugString[300];
    unsigned long cycleCounter=0;
    int byteRate                 = serialBaudrate/8;	
    int bytePeriodInMicroseconds = 1000000/byteRate;    
    int frameMaxLenInBytes       = 100;
    int serialPollTime		 = bytePeriodInMicroseconds * frameMaxLenInBytes * 2; //2 for round rtip
    time_t now;
    time_t previousLap;
    if(serialPollTime<2000)
    {
	serialPollTime=2000;
    }

    time(&previousLap);

    sprintf(debugString,"%s SERIAL MODULE LOADED **********",PREFIX);
    gb_cprintf(debugString,GB_GREEN);

    fd = gb_serialOpenRS232(serialName,serialBaudrate,serialParams[1],serialParams[0],serialParams[2]);

    if(fd<0)
    {
	sprintf(debugString,"%s ERROR: opening and settings serial port. Serial port not opened",PREFIX);
        gb_cprintf(debugString,GB_RED);
    }
    else
    {
        sprintf(debugString,"%s SUCCESS: opening and settings serial port",PREFIX);
    	gb_cprintf(debugString,GB_GREEN);
	sprintf(debugString,"%s Serial parameters are: %s %dbps %d %c %d", 
			     PREFIX,
			     serialName,
			     serialBaudrate,
			     serialParams[0],
			     serialParams[1],
			     serialParams[2]);
	gb_cprintf(debugString,GB_GREEN);
    }
    
    while(1)
    {
	cycleCounter++;
	time(&now);	
	if(globalSerialPollTime>0)
	{
		serialPollTime=globalSerialPollTime;
	}
        bytesAvailable = gb_serialGetAvailableBytes(fd);
        if (bytesAvailable>0)
        {
	    int i=0;
	    if(bDebugSerial==1)
	    {
	    	sprintf(debugString,"%s Data arrived from serial (%d bytes). PASSING DATA TO %s:",PREFIX,bytesAvailable,mycallback==NULL?"null":(mycallback==gb_defaultSerialEvent?"GBLIB DEFAULT SERIAL EVENT HANDLER":"CUSTOM SERIAL EVENT HANDLER"));
            	gb_cprintf(debugString,GB_GREEN);
	    }
            memset(readBytes,0,sizeof(readBytes));
            gb_serialRX(fd,readBytes,bytesAvailable);
            if(bDebugSerial==1)
	    {
	    	for(i=0;i<bytesAvailable;i++)
	    	{
	    		printf("%02x|",readBytes[i]);
            	}
	    }
	    fflush(stdout);
            if(mycallback!=NULL)
	    {
		mycallback(readBytes,bytesAvailable);
	    }
	}
	else
	{
		if(bDebugSerial==2/*(now-previousLap)>1*/)
		{
			printf("\n%s Nothing on serial..[polltime=%d us (%s %s)]",PREFIX,serialPollTime,globalSerialPollTime>0?"forced":"calculated",globalSerialPollTime>0?"":(serialPollTime==2000?"minimal":"(bitperiod_in_seconds*8)*1000000*bytesframe*2"));
			fflush(stdout);
			time(&previousLap);
		}
	}
        if(isThereSomethingToTX())
        {	
		int i=0;
        	sprintf(debugString,"%s There is something to tx on serial(%d bytes). Sending ...",PREFIX,lenToTX);
		gb_cprintf(debugString,GB_GREEN);
                for(i=0;i<lenToTX;i++)
		{
			printf("%02x|",bufferToTX[i]);
		}
		gb_serialTX(fd,bufferToTX,lenToTX);
		bSomethingToTX=0;
        }
	else
	{
		if(cycleCounter%250==0)
		{
			printf("\n%s Nothing to tx.",PREFIX);
			fflush(stdout);
		}
	}
        usleep(serialPollTime);
     }
}

