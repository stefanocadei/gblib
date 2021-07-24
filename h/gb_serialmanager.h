#ifdef __cplusplus
extern "C" {
#endif

typedef void (*gb_serialEventCallback) (char* buffer,int n);
void gb_startSerialManager(char *serialNamePassed,int baudrate,char databits,char parity,char stopbits,void* customEventHandlerPointer);
void gb_setSerialManagerCallback(gb_serialEventCallback coreCallback);
void gb_setSerialCustomPollTime(int timeMilliseconds);
void gb_setDebugSerialManager(char bDebug); /*0.28*/

#ifdef __cplusplus
}
#endif 
