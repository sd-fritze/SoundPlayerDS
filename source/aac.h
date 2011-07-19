#ifndef T_MP4
#define T_MP4

#include <nds.h>
#include <stdio.h>
#include <stdint.h>
#include <mp4ff.h>
#include <aacdec.h>
#include "soundPlayer.h"
#include "f_browse.h"

/* A decode call can eat up to FAAD_MIN_STREAMSIZE(768) bytes per decoded channel,
   so at least so much bytes per channel should be available in this stream */
#define MIN_AAC_BUFFER 768
#define MP4_TRACK_AUDIO 1

int mp4_openFile(char * name);
int aac_openFile(char * name);
mm_word mp4_on_stream_request( mm_word length, mm_addr dest, mm_stream_formats format );
mm_word aac_on_stream_request( mm_word length, mm_addr dest, mm_stream_formats format );
int aac_get_sampleRate(void);
int aac_get_nChannels(void);
void aac_freeDecoder(void);

#endif
