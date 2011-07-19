#ifndef DECODER_H
#define DECODER_H

#include <nds.h>
#include <maxmod9.h>

#define FILE_ERROR 1
#define MAX_VOLUME 127

extern bool needsClosing;

typedef struct 
{
	mm_stream mstream;
	mm_word	callback;
	int (*open_file)(char * name);
	int (*get_sampleRate)(void);
	int (*get_nChannels)(void);
	void (*free_decoder)(void);
	u8 channels;
}MUSIC;

void InitMaxmod(void);
void startStream(MUSIC * m, char * name, int bufferlength);
void visualizeBuffer(s16 * buffer);

#endif
