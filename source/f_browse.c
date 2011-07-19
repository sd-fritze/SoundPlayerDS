#include "f_browse.h"
#include "vbis.h"
#include "mp4ff.h"
#include "aac.h"
#include "soundPlayer.h"

#define DECODER_AAC	1
#define DECODER_OGG 2

char dirEntries[256][32];
int entries;
int cursor;
bool playing = false;
MUSIC musik;

void cacheDir(char * path) {
	DIR *pdir;
	struct dirent *pent;
	struct stat statbuf;

	pdir=opendir(path);

	if (pdir) {
		int i = 0;
		while ((pent=readdir(pdir))!=NULL) {
			stat(pent->d_name,&statbuf);

			if(strcmp(".", pent->d_name) == 0 || strcmp("..", pent->d_name) == 0)
				continue;

			if(strstr(pent->d_name, ".m4a") || strstr(pent->d_name, ".M4A") || strstr(pent->d_name, ".ogg") || 
				strstr(pent->d_name, ".OGG") || strstr(pent->d_name, ".aac")) {
				strcpy(dirEntries[i], pent->d_name);
				entries++;
				iprintf("%s\n", dirEntries[i]);
				i++;
			}
		}
		closedir(pdir);
	}
}

void updateBrowser(void) {
	int keys = keysDown();
	if(keys) {
		consoleClear();
		if(keys & KEY_DOWN && cursor < (entries-1))
			cursor++;
		if(keys & KEY_UP && cursor >  0)
			cursor--;
		int i;
		for(i=0; i<entries; i++) {
			if(i == cursor)
				iprintf("*");
			iprintf("%s\n", dirEntries[i]);
		}
		if(keys & KEY_A) {
			if(playing)
			{
				musik.free_decoder();
				mmStreamClose();
			}
			if(strstr(dirEntries[cursor], ".m4a") || strstr(dirEntries[cursor], ".M4A")) {
				musik.open_file = mp4_openFile;
				musik.get_sampleRate = aac_get_sampleRate;
				musik.get_nChannels = aac_get_nChannels;
				musik.free_decoder = aac_freeDecoder;
				musik.mstream.callback = mp4_on_stream_request;
			} 
			else if(strstr(dirEntries[cursor], ".aac"))
			{
				musik.open_file = aac_openFile;
				musik.get_sampleRate = aac_get_sampleRate;
				musik.get_nChannels = aac_get_nChannels;
				musik.free_decoder = aac_freeDecoder;
				musik.mstream.callback = aac_on_stream_request;	
			}
				else {
				musik.open_file = vbis_openFile;
				musik.get_sampleRate = vbis_get_sampleRate;
				musik.get_nChannels = vbis_get_nChannels;
				musik.free_decoder = vbis_freeDecoder;
				musik.mstream.callback = vbis_on_stream_request;
			}
			startStream(&musik, dirEntries[cursor], 8192);
			playing = true;
		}
		
		if(keys & KEY_B) {
			musik.free_decoder();
			mmStreamClose();
			playing = false;
		}
	}

}
