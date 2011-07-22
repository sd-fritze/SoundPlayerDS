#ifndef T_MP4
#define T_MP4

#include <nds.h>
#include <mp4ff.h>
#include "soundPlayer.h"

#define MP4_TRACK_AUDIO 1

/* 
 * MP4 files are handled differently then plain aac files, but
 * info like sample rate are stored in the same decoder structure
 */
int mp4_openFile(char * name);
int aac_openFile(char * name);
int aac_get_sampleRate(void);
int aac_get_nChannels(void);
void aac_freeDecoder(void);
mm_word mp4_on_stream_request( mm_word length, mm_addr dest, mm_stream_formats format );
mm_word aac_on_stream_request( mm_word length, mm_addr dest, mm_stream_formats format );

#endif
