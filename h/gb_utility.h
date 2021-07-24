#ifndef GB_UTILITY_H
#define GB_UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#define GB_DEFAULT_COLOR 0
#define GB_RED           1
#define GB_GREEN         2
#define GB_YELLOW        3
#define GB_BLUE          4
#define GB_MAGENTA       5
#define GB_CYAN          6
#define GB_WHITE         7

void gb_getGBLIBVersion(char *buffer);
int  gb_kbhit(void);
void gb_flushRawStdinBuffer();
int  gb_getch();
void gb_cprintf(char *string,int color);


#ifdef __cplusplus
}
#endif

#endif
