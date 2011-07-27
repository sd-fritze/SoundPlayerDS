#include <helix/mp3dec.h>
#include <helix/mp3common.h>
#include "mp3.h"
#include "decoder_common.h"
#include "soundPlayer.h"

#define IS_ID3_V2(p)		((p)[0] == 'I' && (p)[1] == 'D' && (p)[2] == '3')
HMP3Decoder * mdecoder;
MP3FrameInfo inf;

void parseID3_V2(FILE * fp) {
	rewind(fp);
	/* Jump to the sync save size field (4 bytes, where bit 7 is left unused
	 * to avoid finding keywords (which need to have that bit set)
	 */
	fseek(fp, 6, SEEK_SET);
	char size[4];
	fread(&size, 1, 4, fp);
	int hdrsize =  size[0]<<21|size[1]<<14|size[2]<<7|size[3];
	fseek(sndFile, hdrsize+10, SEEK_SET);
}

int mp3_openFile(char * name) {
	if((sndFile = fopen(name, "rb"))) {
		char magic[3];
		fread(&magic, 1, 3, sndFile);
		if(IS_ID3_V2(magic)) {
			parseID3_V2(sndFile);
		}
		if((fill_readBuffer(readBuffer, &readOff, READ_BUF_SIZE, &dataLeft))==READ_BUF_SIZE) {
			mdecoder = MP3InitDecoder();
			readOff = readBuffer;
			dataLeft = READ_BUF_SIZE;
			/* MP3 frame header (4 bytes, plus 2 if CRC)
			* not using the decode function as that function may try to clear the buffer
			* and so overwrite other data
			*/
			UnpackFrameHeader((MP3DecInfo *)mdecoder, readOff);
			/* Get info for mm stream init */
			MP3GetLastFrameInfo(mdecoder, &inf);
			return 0;
		}
	}
	return -1;
}

int mp3_get_sampleRate(void) {
	return inf.samprate;
}
int mp3_get_nChannels(void) {
	return inf.nChans;
}
void mp3_freeDecoder(void) {
	memset(&readBuffer[0], 0, READ_BUF_SIZE);
	MP3FreeDecoder(mdecoder);
	Endof = 0;
	dataLeft = 0;
	fclose(sndFile);
}
mm_word mp3_on_stream_request( mm_word length, mm_addr dest, mm_stream_formats format ) {
	int samps =0;
	/* Otherwhise we can possibly overwrite the data behind the buffer */
	if(length >= MAX_NGRAN*inf.nChans*MAX_NSAMP) {
		int ret = 0;
		/* Possible buffer-underrun */
		if(dataLeft < READ_BUF_SIZE) {
			ret = fill_readBuffer(readBuffer, &readOff, READ_BUF_SIZE, &dataLeft);
			if(dataLeft != READ_BUF_SIZE) {
				if(feof(sndFile) && !Endof)
					Endof = MM_BUFFER_SIZE;
				/* still pcm data in buffer */
				else if(length < Endof && Endof)
					return 0;
				/* File error */
				else {
					needsClosing = true;
					return 0;
				}
			}
		}

		/* check for errors */
		if((ret = MP3Decode(mdecoder, &readOff, &dataLeft, dest,0))) {
			iprintf("HELIX MP3 ERROR: %d\n", ret);
			needsClosing = true;
			return 0;
		}
		/* GCC can't know channels being only 1 or 2 */
		samps = inf.outputSamps >> (inf.nChans-1);
		dest+=	inf.outputSamps;
		length -=samps;
	}

	visualizeBuffer(dest);

	return samps;

}
