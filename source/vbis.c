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
mm_word vbis_on_stream_request( mm_word length, mm_addr dest, mm_stream_formats format ) {

	s16 *target = dest;
	int tlength = length;

	while(tlength) {
		int ret=ov_read(&vf,target,tlength*4, &current_section);
		/* Decoding error or EOF*/
		if(ret <= 0) {
			if(ret < 0)
				iprintf("Decoding error occured, playback stopped\n");
			ov_clear(&vf);
			needsClosing = true;
		}
		tlength -= ret/4;
		target +=ret/2;		// we increase a s16 pointer so half the byte size
	}
	visualizeBuffer(dest);
	return length;	/* Return how many samples are decoded */
}

void vbis_freeDecoder(void) {
	ov_clear(&vf);
}
