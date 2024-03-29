#ifndef VBIS_H
#define VBIS_H

#include <nds.h>
#include "SoundPlayer.h"

int	vbis_openFile(char * name);
int	vbis_get_sampleRate(void);
int vbis_get_nChannels(void);
int vbis_seek_percentage(int perc);
int vbis_get_percentage(void);
void vbis_freeDecoder(void);
mm_word vbis_on_stream_request( mm_word length, mm_addr dest, mm_stream_formats format );

#endif /* VBIS_H */
