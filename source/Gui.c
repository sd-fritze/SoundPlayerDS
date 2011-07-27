#include <dirent.h>

#include "Graphics.h"
#include "Gui.h"
#include "Console.h"
#include "Filebrowser.h"

#define ICON_SIZE 32
#define MAX_VISIBLE  ((192/ICON_SIZE) + 2)
// data
#include "icon.h"

int textureID;
u32 * dispList;
u32 * folderList;
int tempy[2];
int timer, vy, y;

touchPosition touch;
int cursor;

void initGui(void) {
	initGl2D();
	/* Use default font */
	glConsoleInit(NULL, NULL);
	/* Generate texture for fbrowser icons */
	glGenTextures(1, &textureID);
	glBindTexture(0, textureID);
	glTexImage2D(0, 0, GL_RGB256, TEXTURE_SIZE_64 , TEXTURE_SIZE_32, 0, TEXGEN_TEXCOORD, (u8*)iconBitmap);
	glColorTableEXT( 0, 0, 256, 0, 0, (u16*)iconPal);
	/* Create a displaylist for every icon */
	dispList = glCreateQuadList(0, 0, 32, 32, 0, 0);
	folderList = glCreateQuadList(0,0,32,32,32,0);
	lcdMainOnBottom();
}

void updateScroll(int entries, struct dirent **dirList) {

	if(keysDown() & KEY_TOUCH) {
		timer = 0;
		tempy[0] = touch.py;
	}
	if(keysHeld() & KEY_TOUCH) {
		timer++;
		tempy[1] = touch.py;
	}

	if(keysUp() & KEY_TOUCH) {

		vy = -(((tempy[1] - tempy[0]))/(timer));
		if(vy > 13)
			vy = 13;
		if(vy<-13)
			vy = -13;
		vy <<= 2;
		// simple click when we didn't move that far
		if(abs(tempy[1]-tempy[0]) < 3 && timer < 40) {
			int toOpen = (y + tempy[1])/ICON_SIZE;
			if(toOpen < entries)
				openSelected(toOpen);
		}
	}
	/* Slow down / correct going out of bounds
	 * Todo: make the bounce back effect look smoother
	 */
	if(vy) {

		if((y < 0 && vy < 0) || (y> (entries*ICON_SIZE-192) && vy >0 )) {
			vy = -vy;
		}

		/* Intercept the bounce back */
		else if(y <0 && (y + (vy >> 2))>=0) {
			y+= 0 - y;
			vy = 0;
		} else if(y + (vy >> 2) <=(entries*ICON_SIZE-192) && y > entries*ICON_SIZE-192) {
			y-=  y - (entries*ICON_SIZE-192);
			vy = 0;
		}

		else {
			y += vy>>2;
			vy += (vy < 0? 1 : -1);
		}
	}

	int i;
	int start = y/ICON_SIZE; // the earliest name visible
	if(start < 0)
		start = 0;
	int tempy = start * ICON_SIZE - y;
	int end = MAX_VISIBLE - (MAX_VISIBLE - (entries - start));

	for(i=0; i<end; i++) {
		glBindTexture(0, textureID);

		/* Determine entry type and draw it's icon */
		if(dirList[i+start]->d_type == DT_DIR) {
			setPosQuadList(0, tempy, ICON_SIZE, tempy+ICON_SIZE, folderList);
			glCallList(folderList);
		} else {
			setPosQuadList(0, tempy, ICON_SIZE, tempy+ICON_SIZE, dispList);
			glCallList(dispList);
		}

		glsetConsolePos(ICON_SIZE, tempy+(ICON_SIZE/4)+4);
		glPrint(dirList[i+start]->d_name, 26);
		tempy+=ICON_SIZE;

	}

}
