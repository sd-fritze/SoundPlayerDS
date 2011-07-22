#include <dirent.h>
#include <unistd.h>
#include "f_browse.h"
#include "vbis.h"
#include "mp4ff.h"
#include "aac.h"
#include "mp3.h"
#include "soundPlayer.h"

#define DECODER_AAC	1
#define DECODER_OGG 2

struct dirent **namelist;
bool playing = false;
MUSIC musik;
char CWD[1024];
char filename[1024];

static int dirFilter(const struct dirent *dent)
{
	static struct stat buf;

	if(strcmp(".", dent->d_name) == 0) //never include .
		return 0;
	else if(strcmp("..", dent->d_name) == 0 && strstr(CWD, ":/")[2] == 0) //never include .. for root directory
		return 0;
	else if(stat(dent->d_name, &buf)) //never include something that fails stat()
		return 0;
	else if(S_ISDIR(buf.st_mode)) //include all directories
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

static int dirCompare(const struct dirent **dent1, const struct dirent **dent2)
{
	static struct stat buf;
	static char isDir[2];

	// push '..' to beginning
	if(strcmp("..", (*dent1)->d_name) == 0)
		return -1;
	else if(strcmp("..", (*dent2)->d_name) == 0)
		return 1;

	stat((*dent1)->d_name, &buf);
	isDir[0] = S_ISDIR(buf.st_mode);

	stat((*dent2)->d_name, &buf);
	isDir[1] = S_ISDIR(buf.st_mode);

	if(isDir[0] == isDir[1]) //sort by name
		return strcmp((*dent1)->d_name, (*dent2)->d_name);
	else
		return isDir[1] - isDir[0]; //put directories first
}

int scandir(const char *dir, struct dirent ***namelist, int(*filter)(const struct dirent *), int(*compar)(const struct dirent **, const struct dirent **))
{
	struct dirent **pList = NULL;
	struct dirent **temp	= NULL;
	struct dirent *dent;
	int numEntries = 0;

	DIR *dp = opendir(dir);
	if(dp == NULL)
		goto error;

	while((dent = readdir(dp)) != NULL) //read all the directory entries
	{
		if(filter(dent)) //filter out unwanted entries
		{
			temp = realloc(pList, sizeof(struct dirent*)*(numEntries+1)); //increase size of list
			if(temp)
			{
				pList = temp;
				pList[numEntries] = malloc(sizeof(struct dirent));
				if(pList[numEntries])
				{
					memcpy(pList[numEntries], dent, sizeof(struct dirent));
					numEntries++;
				}
				else //failed to allocate space for a directory entry
					goto error;
			}
			else //failed to allocate space for directory entry pointer
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
	if(pList) //clean up the list
	{
		int i;
		for(i = 0; i < numEntries; i++)
			free(pList[i]);
		free(pList);
	}
	*namelist = NULL;
	return -1;
}

void updateBrowser(void)
{
	static struct dirent **dirList = NULL;
	static struct dirent **temp;
	static int entries = 0;
	static int cursor = 0;
	static struct stat buf;
	static int keys;
	static int i;

	if(dirList == NULL) //initialize list
	{
		chdir("/"); //change to root directory
		getcwd(CWD, 1024); //get path

		//scan directory
		int numEntries = scandir(".", &temp, dirFilter, dirCompare);
		if(numEntries > 0)
		{
			int i;
			dirList = temp;
			temp = NULL;
			entries = numEntries;
			for(i = 0; i < entries; i++)
				iprintf("%s%s\n", i==cursor?"*":" ", dirList[i]->d_name);
		}
	}
	if(dirList == NULL)
		return;

	keys = keysDown();
	if(keys & (KEY_A|KEY_B|KEY_UP|KEY_DOWN))
	{
		consoleClear();

		//update the cursor position
		if(keys & KEY_DOWN && cursor < (entries-1))
			cursor++;
		if(keys & KEY_UP && cursor >	0)
			cursor--;

		//print out the list with the updated cursor position
		for(i = 0; i < entries; i++)
			iprintf("%s%s\n", i==cursor?"*":" ", dirList[i]->d_name);

		//select entry at cursor
		if(keys & KEY_A)
		{
			stat(dirList[cursor]->d_name, &buf);
			if(S_ISDIR(buf.st_mode)) //change to new directory
			{
				chdir(dirList[cursor]->d_name);
				getcwd(CWD, 1024);

				//scan new directory
				int numEntries = scandir(CWD, &temp, dirFilter, dirCompare);
				if(numEntries > 0)
				{
					//clean up old list
					for(i = 0; i < entries; i++)
						free(dirList[i]);
					free(dirList);

					//replace with new list
					dirList = temp;
					temp = NULL;
					entries = numEntries;

					//print new list
					consoleClear();
					cursor = 0;
					for(i = 0; i < entries; i++)
						iprintf("%s%s\n", i==cursor?"*":" ", dirList[i]->d_name);
				}
				else if(temp) //failed, so clean up
				{
					free(temp);
					temp = NULL;
				}
			}
			else //open file
			{
				memcpy(filename, CWD, 1024);
				strcat(filename, dirList[cursor]->d_name); //append filename to current working directory

				//stop playing current previous selection
				if(playing)
				{
					musik.free_decoder();
					mmStreamClose();
				}

				//play m4a file
				if(strstr(dirList[cursor]->d_name, ".m4a") || strstr(dirList[cursor]->d_name, ".M4A"))
				{
					musik.open_file        = mp4_openFile;
					musik.get_sampleRate   = aac_get_sampleRate;
					musik.get_nChannels    = aac_get_nChannels;
					musik.free_decoder     = aac_freeDecoder;
					musik.mstream.callback = mp4_on_stream_request;
				}
				//play aac file
				else if(strstr(dirList[cursor]->d_name, ".aac") || strstr(dirList[cursor]->d_name, ".AAC"))
				{
					musik.open_file        = aac_openFile;
					musik.get_sampleRate   = aac_get_sampleRate;
					musik.get_nChannels    = aac_get_nChannels;
					musik.free_decoder     = aac_freeDecoder;
					musik.mstream.callback = aac_on_stream_request;	
				}
				else if(strstr(dirList[cursor]->d_name, ".ogg") || strstr(dirList[cursor]->d_name, ".OGG"))
				{
					musik.open_file        = vbis_openFile;
					musik.get_sampleRate   = vbis_get_sampleRate;
					musik.get_nChannels    = vbis_get_nChannels;
					musik.free_decoder     = vbis_freeDecoder;
					musik.mstream.callback = vbis_on_stream_request;
				}
				else if(strstr(dirList[cursor]->d_name, ".mp3") || strstr(dirList[cursor]->d_name, ".MP3"))
				{
					musik.open_file        = mp3_openFile;
					musik.get_sampleRate   = mp3_get_sampleRate;
					musik.get_nChannels    = mp3_get_nChannels;
					musik.free_decoder     = mp3_freeDecoder;
					musik.mstream.callback = mp3_on_stream_request;
				}
				else
					musik.open_file = NULL;

				if(musik.open_file != NULL)
				{
					startStream(&musik, filename, 8192);
					playing = true;
				}
				else
					playing = false;
			}
		}
		
		if(keys & KEY_B)
		{
			musik.free_decoder();
			mmStreamClose();
			playing = false;
		}
	}
}

