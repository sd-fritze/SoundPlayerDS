#ifndef GUI_H
#define GUI_H

#include <nds.h>
#include "SoundPlayer.h"

extern touchPosition touch;
extern int y;
extern int bg2d;
void initGui(void);
void updateScroll(int entries, struct dirent **dirList);
void updateProgress(MUSIC * m);
void closeDecoder(void);
#endif /* GUI_H */
