/*stty -F /dev/ttyUSB0 4800 cs8 -cstopb -parodd -parenb*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*GB Includes for generic serial*/
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>

/*GB Includes for RS485 serial*/
#include <asm/ioctls.h>
#include <linux/serial.h>

#include "../h/gb_utility.h"

/* +---------------------------------------------------------------------------+ */
/* |                               D E F I N E S                               | */
/* +---------------------------------------------------------------------------+ */
// This definition should be presents into the patched kernel but seems that our
// kernel is not up-to-date so I defined it again here
#define SER_RS485_USE_GPIO      ( 1 << 5 )

// Mapped GPIO used as RS-485 DIR (from UART0 to UART5)
static char gpioDIR[] = {0, 0, 65, 0, 66, 0};
/* +---------------------------------------------------------------------------+ */

static char gb_bDebugMode=0;

static char debugString[200];
char *PREFIX = "[ GBLIB SERIAL  ]";

int gb_serialPortFd=-1;

int gb_serialGetPortFd()
{
	return gb_serialPortFd;
}

void gb_serialSetDebugMode(char bDebug)
{
    gb_bDebugMode = bDebug;
}

void gb_serialSetIOCTLattributes(int fn, int baud, char par, int dbit, int sbit) {
    struct termios opt;
    unsigned int bSpeed;

    tcgetattr(fn, &opt); // Get attributes from defined serial port
    /* ----- CONTROL OPTIONS ----- */
    switch (par) // Define parity mode
    {
        case 'N': // parity NONE
        case 'n':
            opt.c_cflag &= ~PARENB; // Parity control is DISABLED
            break;
        case 'O': // parity ODD
        case 'o':
            opt.c_cflag |= PARENB; // Parity control is ENABLED
            opt.c_cflag |= PARODD; // Define ODD parity mode
            break;
        case 'e': // parity EVEN
        case 'E':
            opt.c_cflag |= PARENB; // Parity control is ENABLED
            opt.c_cflag &= ~PARODD; // Define EVEN parity mode
            break;
        default: // None of the above
            opt.c_cflag &= ~PARENB; // Parity control is DISABLED
            break;
    }
    opt.c_cflag &= ~CSIZE; // Mask for data bit size (clear bits)
    switch (dbit) // Define data bit size
    {
        case 5:
            opt.c_cflag |= CS5; // Data is 5 bit wide
            break;
        case 6:
            opt.c_cflag |= CS6; // Data is 6 bit wide
            break;
        case 7:
            opt.c_cflag |= CS7; // Data is 7 bit wide
            break;
        case 8:
            opt.c_cflag |= CS8; // Data is 8 bit wide
            break;
        default: // None of the above
            opt.c_cflag |= CS8; // Data is 8 bit wide
            break;
    }
    switch (sbit) // Define data bit size
    {
        case 1:
            opt.c_cflag &= ~CSTOPB; // 1 Stop bit
            break;
        case 2:
            opt.c_cflag |= CSTOPB; // 2 Stop bits
            break;
        default: // None of the above
            opt.c_cflag &= ~CSTOPB; // 1 Stop bit
            break;
    }
    opt.c_cflag |= CLOCAL; // Do not change owner of port (always set)
    opt.c_cflag |= CREAD; // Receiver enabled (always set)
    opt.c_cflag &= ~CBAUD; // Clear mask for baud rate selection
    /* ----- LOCAL OPTIONS ----- */
    opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Raw Input (no wait for CR-LF)
    /* ----- INPUT OPTIONS ----- */
    if (opt.c_cflag & PARENB) // if the parity control is ENABLED...
        opt.c_iflag |= (INPCK | ISTRIP); // ...check and strip the parity bit...
    else // ...else...
        opt.c_iflag &= ~(INPCK | ISTRIP); // ...do not check and strip the parity bit

    opt.c_iflag &= ~(IGNPAR | PARMRK); // Ensure that IGNPAR and PARMRK are not enabled
    opt.c_iflag &= ~(IXON | IXOFF | IXANY); // Flow control is not enabled
    opt.c_iflag &= ~(INLCR | ICRNL | IUCLC); // No controls on CR and NL characters and on case (upper/lower)
    /* ----- OUTPUT OPTIONS ----- */
    opt.c_oflag &= ~OPOST; // No postprocessing options (raw data)
    opt.c_oflag &= ~(ONLCR | OCRNL | OLCUC); //No controls on CR and NL characters and on case (upper/lower)
    /* ----- INPUT (RX) AND OUTPUT (TX) SPEED ----- */
    switch (baud) // Define data bit size
    {
        case 1200:
            bSpeed = B1200; // Speed selected is 1200 baud
            break;
        case 2400:
            bSpeed = B2400; // Speed selected is 2400 baud
            break;
        case 4800:
            bSpeed = B4800; // Speed selected is 4800 baud
            break;
        case 9600:
            bSpeed = B9600; // Speed selected is 9600 baud
            break;
        case 19200:
            bSpeed = B19200; // Speed selected is 19200 baud
            break;
        case 38400:
            bSpeed = B38400; // Speed selected is 38400 baud
            break;
        case 57600:
            bSpeed = B57600; // Speed selected is 57600 baud
            break;
        case 115200:
            bSpeed = B115200; // Speed selected is 115200 baud
            break;
        default: // None of the above
            bSpeed = B9600; // Speed selected is 9600 baud
            break;
    }
    cfsetispeed(&opt, bSpeed);
    cfsetospeed(&opt, bSpeed);
    tcsetattr(fn, TCSANOW, &opt);
}

int gb_serialOpenRS232(char *dev, int baud, char par, int dbit, int sbit)
{
    int fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0)
    {
        return -1;
    }
    gb_serialPortFd=fd;
    gb_serialSetIOCTLattributes(fd, baud, par, dbit, sbit);
    return fd;
}

int gb_serialOpenRS485(char *dev, int baud, char par, int dbit, int sbit, int uart)
{
    long tiocsr485 = 0x542F;
    struct serial_rs485 ctrl485;

    int fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        return -1;
    }
    gb_serialSetIOCTLattributes(fd, baud, par, dbit, sbit);

    /* Set the port in 485 mode */
    ctrl485.flags = SER_RS485_ENABLED | SER_RS485_USE_GPIO;
    ctrl485.delay_rts_before_send = 200; // microseconds between DE active and start Tx
    ctrl485.delay_rts_after_send = 100; // microseconds between end Tx and DE inactive
    ctrl485.padding[0] = gpioDIR[uart];
    ctrl485.padding[1] = 0;
    ctrl485.padding[2] = 0;
    ctrl485.padding[3] = 0;
    ctrl485.padding[4] = 0;
    if (ioctl(fd, tiocsr485, &ctrl485)) {
        printf("%s: Unable to configure this port in RS-485 mode\n", dev);
        return ( -1);
    }
    return fd;
}

void gb_serialClosePort(int fd)
{
    if(fd!=-1)
        close(fd);
}

int gb_serialGetAvailableBytes(int fd)
{
    int bytes_avail = 0;
    if(fd!=-1){
        ioctl(fd, FIONREAD, &bytes_avail);
    }
    return bytes_avail;
}

int gb_serialFlushInputBuffer(int fd)
{
    int bytes_avail = 0;
    char *tempBuffer = NULL;

    bytes_avail = gb_serialGetAvailableBytes(fd);

    if(bytes_avail>0){
        tempBuffer = calloc(bytes_avail,sizeof(char));
        if(tempBuffer!=NULL){
            read(fd,tempBuffer,bytes_avail);
            free(tempBuffer);
        }
    }
    return bytes_avail;
}


int gb_serialTX(int fd,char *buffer,int len)
{
    int i=0;
    int sentBytes=0;
    if(buffer!=NULL)
    {
	sprintf(debugString,"%s Writing on serial (%d bytes):",PREFIX,len);
	gb_cprintf(debugString,GB_GREEN);
        for(i=0;i<len;i++)
	{
		printf("%02x|",buffer[i]);
	}
	fflush(stdout);
	sentBytes=write(fd,buffer,len);
    }
    return sentBytes;
}



int gb_serialRX(int fd,char *buffer,int len)
{
    int bytesRead=0;
    if(buffer!=NULL)
    {
        bytesRead=read(fd,buffer,len);
    }
    return bytesRead;
}

