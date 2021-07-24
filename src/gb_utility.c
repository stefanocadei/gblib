#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include "../h/gb_utility.h"

void gb_getGBLIBVersion(char *dest)
{
	if(dest!=NULL)
	{
		sprintf(dest,"%s","0.28");
	}
}

int gb_kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch != EOF){
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}

void gb_flushRawStdinBuffer()
{
    tcflush(STDIN_FILENO, TCIFLUSH);
}

int gb_getch()
{
    int ch;
    struct termios oldt;
    struct termios newt;
    tcgetattr(STDIN_FILENO, &oldt); /*store old settings */
    newt = oldt; /* copy old settings to new settings */
    newt.c_lflag &= ~(ICANON | ECHO); /* make one change to old settings in new settings */
    tcsetattr(STDIN_FILENO, TCSANOW, &newt); /*apply the new settings immediatly */
    ch = getchar(); /* standard getchar call */
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); /*reapply the old settings */
    return ch; /*return received char */
}

void gb_cprintf(char *string,int color)
{
    switch(color){
        case GB_DEFAULT_COLOR:
            printf("\n%s",string);
            break;
        case GB_RED:
            printf("\n\e[1;31m%s\e[0m",string);
            break;
        case GB_GREEN:
            printf("\n\e[1;32m%s\e[0m",string);
            break;
        case GB_YELLOW:
            printf("\n\e[1;33m%s\e[0m",string);
            break;
        case GB_BLUE:
            printf("\n\e[1;34m%s\e[0m",string);
            break;
        case GB_MAGENTA:
            printf("\n\e[1;35m%s\e[0m",string);
            break;
        case GB_CYAN:
            printf("\n\e[1;36m%s\e[0m",string);
            break;
	case GB_WHITE:
            printf("\n\e[1;37m%s\e[0m",string);
            break;
    }
    fflush(stdout);
}
