#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include "Filebrowser.h"
#include "Console.h"
#include "Gui.h"

#define TYPE_DIR(n) ( n == DT_DIR? 1:0)

struct dirent **namelist;
static int entries;
static int cursor;
static struct dirent **dirList;
int random;
bool playing = false;
MUSIC musik;
char CWD[1024];
char filename[1024];

u8 * typeList;	// list containing the filetypes of the current files

static int dirFilter(const struct dirent *dent) {
	if(strcmp(".", dent->d_name) == 0) //never include .
		return 0;
	else if(strcmp("..", dent->d_name) == 0 && strstr(CWD, ":/")[2] == 0) //never include .. for root directory
		return 0;
	else if(TYPE_DIR(dent->d_type)) //include all directories
		return 1;
	return strstr(dent->d_name, ".m4a")	//include m4a
	       || strstr(dent->d_name, ".M4A")	//include M4A
	       || strstr(dent->d_name, ".ogg")	//include ogg
	       || strstr(dent->d_name, ".OGG")	//include OGG
	       || strstr(dent->d_name, ".aac")	//include aac
	       || strstr(dent->d_name, ".AAC") //include AAC
	       || strstr(dent->d_name, ".mp3") //include mp3
	       || strstr(dent->d_name, ".MP3");//include MP3
}

static int dirCompare(const struct dirent **dent1, const struct dirent **dent2) {
	char isDir[2];
	// push '..' to beginning
	if(strcmp("..", (*dent1)->d_name) == 0)
		return -1;
	else if(strcmp("..", (*dent2)->d_name) == 0)
		return 1;

	isDir[0] = TYPE_DIR((*dent1)->d_type);
	isDir[1] = TYPE_DIR((*dent2)->d_type);

	if(isDir[0] == isDir[1]) //sort by name
		return strcmp((*dent1)->d_name, (*dent2)->d_name);
	else
		return isDir[1] - isDir[0]; //put directories first
}

int scandir(const char *dir, struct dirent ***namelist, int(*filter)(const struct dirent *), int(*compar)(const struct dirent **, const struct dirent **)) {
	struct dirent **pList = NULL;
	struct dirent **temp	= NULL;
	struct dirent *dent;
	int numEntries = 0;

	DIR *dp = opendir(dir);
	if(dp == NULL)
		goto error;

	while((dent = readdir(dp)) != NULL) { //read all the directory entries
		if(filter(dent)) { //filter out unwanted entries
			temp = realloc(pList, sizeof(struct dirent*)*(numEntries+1)); //increase size of list
			if(temp) {
				pList = temp;
				pList[numEntries] = malloc(sizeof(struct dirent));
				if(pList[numEntries]) {
					memcpy(pList[numEntries], dent, sizeof(struct dirent));
					numEntries++;
				} else //failed to allocate space for a directory entry
					goto error;
			} else //failed to allocate space for directory entry pointer
				goto error;
		}
	}

	//sort the list
	qsort(pList, numEntries, sizeof(struct dirent*), (int (*)(const void*, const void*))compar);

	closedir(dp);
	*namelist = pList;
	return numEntries;

error:
	if(dp != NULL)
		closedir(dp);
	if(pList) { //clean up the list
		int i;
		for(i = 0; i < numEntries; i++)
			free(pList[i]);
		free(pList);
	}
	*namelist = NULL;
	return -1;
}

void updateBrowser(void) {
	int keys;
	struct dirent **temp;

	if(dirList == NULL) { //initialize list
		chdir("/"); //change to root directory
		getcwd(CWD, 1024); //get path

		//scan directory
		int numEntries = scandir(".", &temp, dirFilter, dirCompare);
		if(numEntries > 0) {

			dirList = temp;
			temp = NULL;
			entries = numEntries;
		}
	}

	keys = keysDown();
	if(keys & (KEY_A|KEY_B|KEY_X|KEY_Y) || random) {

		if(keys & KEY_X) {
			powerOff(  PM_BACKLIGHT_BOTTOM|PM_BACKLIGHT_TOP);
			random = true;
		}
		if(keys & KEY_Y)
			powerOn(  PM_BACKLIGHT_BOTTOM|PM_BACKLIGHT_TOP);

		if(random && !playing) {
			int old = cursor;
			while(cursor == old)
				cursor = rand() % entries;
		}
		//select entry at cursor
		if(keys & KEY_A || (random && !playing)) {
			openSelected(cursor);
		}

		if(keys & KEY_B) {
			random = 0;
			if(playing) {
				musik.free_decoder();
				mmStreamClose();
				playing = false;
			}
		}
	}
	if(!playing)
		updateScroll(entries, dirList);

}

void openSelected(int selected) {
	int i;
	struct dirent **temp = NULL;
	if(dirList[selected]->d_type==DT_DIR && !random) { //change to new directory
		chdir(dirList[selected]->d_name);
		getcwd(CWD, 1024);

		//scan new directory
		int numEntries = scandir(CWD, &temp, dirFilter, dirCompare);
		if(numEntries > 0) {
			//clean up old list
			for(i = 0; i < entries; i++)
				free(dirList[i]);
			free(dirList);

			//replace with new list
			dirList = temp;
			entries = numEntries;
			/* Reset positions */
			cursor = 0;
			y = 0;
		} else if(temp) { //failed, so clean up
			free(temp);
		}
	} else { //open file
		memcpy(filename, CWD, 1024);
		strcat(filename, dirList[selected]->d_name); //append filename to current working directory

		//stop playing current previous selection
		if(playing) {
			musik.free_decoder();
			mmStreamClose();
		}

		//play m4a file
		if(strstr(dirList[selected]->d_name, ".m4a") || strstr(dirList[selected]->d_name, ".M4A")) {
			musik.open_file        = mp4_openFile;
			musik.get_sampleRate   = aac_get_sampleRate;
			musik.get_nChannels    = aac_get_nChannels;
			musik.free_decoder     = aac_freeDecoder;
			musik.mstream.callback = mp4_on_stream_request;
		}
		//play aac file
		else if(strstr(dirList[selected]->d_name, ".aac") || strstr(dirList[selected]->d_name, ".AAC")) {
			musik.open_file        = aac_openFile;
			musik.get_sampleRate   = aac_get_sampleRate;
			musik.get_nChannels    = aac_get_nChannels;
			musik.free_decoder     = aac_freeDecoder;
			musik.mstream.callback = aac_on_stream_request;
		} else if(strstr(dirList[selected]->d_name, ".ogg") || strstr(dirList[selected]->d_name, ".OGG")) {
			musik.open_file        = vbis_openFile;
			musik.get_sampleRate   = vbis_get_sampleRate;
			musik.get_nChannels    = vbis_get_nChannels;
			musik.free_decoder     = vbis_freeDecoder;
			musik.mstream.callback = vbis_on_stream_request;
		} else if(strstr(dirList[selected]->d_name, ".mp3") || strstr(dirList[selected]->d_name, ".MP3")) {
			musik.open_file        = mp3_openFile;
			musik.get_sampleRate   = mp3_get_sampleRate;
			musik.get_nChannels    = mp3_get_nChannels;
			musik.free_decoder     = mp3_freeDecoder;
			musik.mstream.callback = mp3_on_stream_request;
		} else
			musik.open_file = NULL;

		if(musik.open_file != NULL) {
			startStream(&musik, filename, MM_BUFFER_SIZE);
			playing = true;
		} else
			playing = false;
	}
}
