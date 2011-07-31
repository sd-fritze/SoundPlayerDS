#ifndef F_BROWSER_H
#define F_BROWSER_H

#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "sound/SoundPlayer.h"

extern int y;
extern bool playing;
extern u32 * dirDispList, fileDispList;
extern int IconsTextID;
void updateBrowser(void);
void openSelected(int selected);
u32 get_fileSize(FILE * fp);

#endif /* F_BROWSER_H */
