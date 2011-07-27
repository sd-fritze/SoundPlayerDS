#ifndef T_CONSOLE_H
#define T_CONSOLE_H
#include <nds.h>

void glConsoleInit(u8 * texture, u16 * pal);
void putChar(char c, int x, int y);
void glPrint(char * text, int limit);
void glresetConsole(void);
void glsetConsolePos(int x, int y);

#endif /* CONSOLE_H */