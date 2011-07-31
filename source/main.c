#include <nds.h>
#include <stdio.h>
#include <filesystem.h>
#include <fat.h>

#include "Filebrowser.h"
#include "Gui.h"
#include "Graphics.h"
#include "sound/SoundPlayer.h"

int main(int argc, char ** argv) {

	fatInitDefault();
	InitMaxmod();
	initGui();
	
	while(1) {

		scanKeys();
		touchRead(&touch);
		updateBrowser();

		if(playing) {
			mmStreamUpdate();
			updateProgress(&musik);
			if(needsClosing) {
				needsClosing = false;
				closeDecoder();
			}
		}
		glFlush(0);
		swiWaitForVBlank();
	}
}
