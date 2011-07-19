#ifndef VBIS_H
#define VBIS_H

#include <nds.h>
#include <maxmod9.h>
#include <tremor/ivorbisfile.h>
#include "soundPlayer.h"

int	vbis_openFile(char * name);
int	vbis_get_sampleRate(void);
int vbis_get_nChannels(void);
void vbis_freeDecoder(void);
mm_word vbis_on_stream_request( mm_word length, mm_addr dest, mm_stream_formats format );

#endif
