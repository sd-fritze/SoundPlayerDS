#include <dirent.h>
#include "prgr.h"
#include "Graphics.h"
#include "Gui.h"
#include "Console.h"
#include "Filebrowser.h"

// scrolling
#define ICON_SIZE        32
#define MAX_VISIBLE      ((192/ICON_SIZE) + 2)
#define MAX_SCROLL       13
#define SCROLL_MAGNITUDE 2
#define HISTORY          10
#define DEADZONE         5
//progress bar
#define PIXEL_PER_PERC	41 // 2.65*16 rounded upwards for more accuracy
// data
#include "icon.h"

//scrolling
int textureID;
u32 * musicList;
u32 * folderList;
int tempy[HISTORY];
int timer, vy, y;

touchPosition touch;
int cursor;
int bg2d;

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
	musicList = glCreateQuadList(0, 0, 32, 32, 0, 0);
	folderList = glCreateQuadList(0,0,32,32,32,0);
	/* Setup 2d for progress bar and for future stuff */
	videoSetModeSub(MODE_5_2D);
	vramSetBankC(VRAM_C_SUB_BG);
	/* setup a bg for progress bar */
	consoleDemoInit();
	bg2d = bgInitSub(0,  BgType_Text4bpp,  BgSize_T_512x256, 2, 0);
	u16 * map = bgGetMapPtr(bg2d);
	map += 11*32;
	int i,j;
	for(i=1; i<3; i++){
	for(j = 0; j<32; j++){
		map[j] = i;
	}
	map+=32;
	}
	dmaCopy(prgrPal, BG_PALETTE_SUB, prgrPalLen);
	dmaCopyHalfWords (3, prgrTiles, bgGetGfxPtr(bg2d), prgrTilesLen);
	bgHide(bg2d);
	lcdMainOnBottom();
}

void updateScroll(int entries, struct dirent **dirList) {
	int i;
	static int breach = 0;
	static int firsty = 0;

	if(keysDown() & KEY_TOUCH) {
		//reset timer
		timer = 0;

		//stop scrolling
		vy = 0;

		//reset deadzone
		breach = 0;
		firsty = touch.py;

		//fill the entire array with the new y position
		for(i = 0; i < HISTORY; i++)
			tempy[i] = touch.py;
	}
	if(keysHeld() & KEY_TOUCH) {
		//increment timer
		timer++;

		//move data one index lower
		for(i = 0; i < HISTORY-1; i++)
			tempy[i] = tempy[i+1];

		//put new y position in last slot
		tempy[HISTORY-1] = touch.py;

		//scroll with stylus
		y+= tempy[HISTORY-2] - tempy[HISTORY-1];

		//force to edge
		if(y > entries*ICON_SIZE-192)
			y = entries*ICON_SIZE-192;
		if(y < 0)
			y = 0;

		//check if exited deadzone
		if(!breach && abs(touch.py - firsty) > DEADZONE)
			breach = 1;
	}

	if(keysUp() & KEY_TOUCH) {
		//set scroll momentum
		if(breach && timer > 0)
				vy = -(((tempy[HISTORY-1]-tempy[0]))/(timer<HISTORY?timer:HISTORY));

		//clamp scroll momentum
		if(vy > MAX_SCROLL)
			vy = MAX_SCROLL;
		if(vy < -MAX_SCROLL)
			vy = -MAX_SCROLL;

		//scale scroll momentum
		vy <<= SCROLL_MAGNITUDE;

		//select if in the deadzone
		if(!breach) {
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

	int start = y/ICON_SIZE; // the earliest name visible
	if(start < 0)
		start = 0;
	int starty = start * ICON_SIZE - y;
	int end = entries - start;

	for(i=0; i<end; i++) {
		glBindTexture(0, textureID);

		/* Determine entry type and draw it's icon */
		if(dirList[i+start]->d_type == DT_DIR) {
			setPosQuadList(0, starty, ICON_SIZE, starty+ICON_SIZE, folderList);
			glCallList(folderList);
		} else {
			setPosQuadList(0, starty, ICON_SIZE, starty+ICON_SIZE, musicList);
			glCallList(musicList);
		}

		glsetConsolePos(ICON_SIZE, starty+(ICON_SIZE/4)+4);
		glPrint(dirList[i+start]->d_name, 26);
		starty+=ICON_SIZE;
	}
	
}

void updateProgress(MUSIC * m){
	static int prgrX;
	
	if(keysHeld() & KEY_TOUCH){
		prgrX = touch.px;
		bgSetScroll(bg2d, 256-prgrX, 0);
	}
	else if(keysUp() & KEY_TOUCH && prgrX){
		m->dec->seek_percentage((((prgrX)*16)/PIXEL_PER_PERC));
		prgrX = 0;
	}
	else
		bgSetScroll(bg2d, 256-((m->dec->get_percentage()*PIXEL_PER_PERC)>>8), 0);
	bgUpdate();
}

void closeDecoder(void){
	// graphics
	lcdMainOnBottom();
	//bgSetScroll(bg2d, 256, 0);
	bgHide(bg2d);
	// sound
	musik.dec->free_decoder();
	mmStreamClose();
	playing = false;
}
