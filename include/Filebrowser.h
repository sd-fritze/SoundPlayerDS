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
extern MUSIC musik;
extern u32 * dirDispList, fileDispList;
extern int IconsTextID;
void updateBrowser(void);
void openSelected(int selected);

#endif /* F_BROWSER_H */
