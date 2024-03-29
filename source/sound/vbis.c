#include <tremor/ivorbisfile.h>
#include "vbis.h"

OggVorbis_File vf;
static int current_section;
vorbis_info *vi = NULL;

/*
Opens an OGG file
*/
int vbis_openFile(char * name) {
	FILE * fp = fopen(name, "rb");
	if(fp) {
		int ret = ov_open(fp, &vf, NULL,0);
		vi=ov_info(&vf,-1);
		return ret;
	}
	iprintf("File %s can't be opened\n", name);
	return FILE_ERROR;
}

int vbis_get_sampleRate(void) {
	return vi->rate;
}

int vbis_get_nChannels(void) {
	return vi->channels;
}
int vbis_get_percentage(void){
	return (int)((ov_time_tell(&vf)*16)/(ov_time_total(&vf, -1)/100));
}
int vbis_seek_percentage(int perc){
	int ret = ov_time_seek(&vf,perc*(ov_time_total(&vf, -1)/100));
	if(ret == 0)
		return 0;
	return -1;
}
mm_word vbis_on_stream_request( mm_word length, mm_addr dest, mm_stream_formats format ) {

	s16 *target = dest;
	int tlength = length;

	while(tlength) {
		/* Read enough bytes, 4* for stereo, 2*for mono */
		int ret=ov_read(&vf,target,tlength*vi->channels*2, &current_section);
		/* Decoding error or EOF*/
		if(ret <= 0) {
			if(ret < 0)
				iprintf("TREMOR OGG ERROR: %d!\n", ret);
			ov_clear(&vf);
			needsClosing = true;
			return 0;
		}
		tlength -= ret/(vi->channels*2);
		target +=ret/2; // we increase a s16 pointer so half the byte size
	}
	visualizeBuffer(dest);
	return length; /* Return how many samples are decoded */
}

void vbis_freeDecoder(void) {
	ov_clear(&vf);
}
