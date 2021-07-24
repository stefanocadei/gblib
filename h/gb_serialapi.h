#ifndef GB_SERIAL_H
#define GB_SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif

int  gb_serialGetPortFd();
void gb_serialSetIOCTLattributes(int fn, int baud, char par, int dbit, int sbit);
int  gb_serialOpenRS232(char *dev, int baud, char par, int dbit, int sbit);
int  gb_serialOpenRS485(char *dev, int baud, char par, int dbit, int sbit, int uart);
void gb_serialClosePort(int fd);
int  gb_serialGetAvailableBytes(int fd);
int  gb_serialRX(int fd,char *buffer,int len);
int  gb_serialTX(int fd,char *buffer,int len);
int  gb_serialFlushInputBuffer(int fd);

#ifdef __cplusplus
}
#endif

#endif
