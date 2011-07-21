#ifndef MP3_H
#define MP3_H

#include <nds.h>
#include <helix/mp3dec.h>

#include "soundPlayer.h"
#include "f_browse.h"

int mp3_openFile(char * name);
int mp3_get_sampleRate(void);
int mp3_get_nChannels(void);
void mp3_freeDecoder(void);
mm_word mp3_on_stream_request( mm_word length, mm_addr dest, mm_stream_formats format );

#endif /* MP3_H */