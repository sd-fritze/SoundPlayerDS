#include <nds.h>
#include <stdio.h>

#include <filesystem.h>
#include <fat.h>
#include "f_browse.h"
#include "soundPlayer.h"

int main(int argc, char ** argv) {

	videoSetMode(MODE_0_3D);
	videoSetModeSub(MODE_5_2D);
	glInit();

	glEnable(GL_ANTIALIAS);
 
	glClearColor(0,0,0,31); 
	glClearPolyID(63);
	glClearDepth(0x7FFF);
 
	glViewport(0,0,255,191);
 
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(70, 256.0 / 192.0, 0.1, 100);
 
	glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
gluLookAt(	0.0, 0.0, 1.0,		//camera possition 
				0.0, 0.0, 0.0,		//look at
				0.0, 1.0, 0.0);		//up

	consoleDemoInit();
	nitroFSInit();
	//fatInitDefault();
	InitMaxmod();
	cacheDir("/");
	while(1) {
		scanKeys();
		updateBrowser();
		glMatrixMode(GL_MODELVIEW);	
		if(playing)
		{
			mmStreamUpdate();
			if(needsClosing)
			{
				needsClosing = false;
				musik.free_decoder();
				mmStreamClose();
				playing = false;
				
			}	
		}
		glFlush(0);	
		swiWaitForVBlank();
	}
}
