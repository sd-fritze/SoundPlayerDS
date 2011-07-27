#ifndef GUI_H
#define GUI_H

#include <nds.h>

extern touchPosition touch;
extern int y;
void initGui(void);
void updateScroll(int entries, struct dirent **dirList);

#endif /* GUI_H */
