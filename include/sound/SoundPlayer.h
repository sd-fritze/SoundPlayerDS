#ifndef SOUND_PLAYER_H
#define SOUND_PLAYER_H

#include <nds.h>
#include <maxmod9.h>
#include <stdio.h>

#include "vbis.h"
#include "aac.h"
#include "mp3.h"

#define FILE_ERROR 1
#define MAX_VOLUME 127
#define MM_BUFFER_SIZE 8192
#define READ_BUF_SIZE 1940

typedef struct
{
	int (*open_file)(char * name);
	int (*get_sampleRate)(void);
	int (*get_nChannels)(void);
	int (*seek_percentage)(int perc);
	int (*get_percentage) (void);
	void (*free_decoder)(void);
	mm_word (*callback)( mm_word length, mm_addr dest, mm_stream_formats format );
}DECODER;

typedef struct 
{
	mm_stream mstream;
	u8 channels;
	DECODER * dec;
}MUSIC;

extern DECODER aac_dec;
extern DECODER m4a_dec;
extern DECODER vbis_dec;
extern DECODER mp3_dec;

extern FILE * sndFile;
extern MUSIC musik;
extern bool needsClosing;
extern unsigned char readBuffer[READ_BUF_SIZE];
extern unsigned char * readOff;
extern int dataLeft;
extern short Endof; // samples to endof

void InitMaxmod(void);
void startStream(MUSIC * m, char * name, int bufferlength);
void visualizeBuffer(s16 * buffer);

#endif /* SOUND_PLAYER_H */
