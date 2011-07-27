#include <nds.h>
#include <stdio.h>
#include <filesystem.h>
#include <fat.h>

#include "Filebrowser.h"
#include "Gui.h"
#include "Graphics.h"
#include "sound/SoundPlayer.h"

int main(int argc, char ** argv) {

	consoleDemoInit();
	fatInitDefault();
	InitMaxmod();
	initGui();

	while(1) {

		scanKeys();
		touchRead(&touch);
		updateBrowser();

		if(playing) {
			mmStreamUpdate();

			if(needsClosing) {
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
