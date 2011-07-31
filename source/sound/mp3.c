/*
 * Sources:
 * http://www.codeproject.com/KB/audio-video/mpegaudioinfo.aspx
 */
 
#include <helix/mp3dec.h>
#include <helix/mp3common.h>
#include "mp3.h"
#include "decoder_common.h"
#include "Filebrowser.h"	// needed to retrieve filesize

#define IS_ID3_V2(p)		((p)[0] == 'I' && (p)[1] == 'D' && (p)[2] == '3')
#define ID3_HDR_SIZE 10
HMP3Decoder * mdecoder;
MP3FrameInfo inf;

u32 firstFrame;	// offset in file of first frameheader
u32 fileSize;
u32 bitrate;

int findValidSync(unsigned char ** offset, int * bytes) {

	int ret;
	while(UnpackFrameHeader((MP3DecInfo *)mdecoder, *offset)<0) {
		// search for another syncword
findsync:
		*offset +=2;
		*bytes -=2;
		// we did enough this mp3 is likely corrupt
		if((ret=MP3FindSyncWord(*offset, *bytes))<0) {
			return -1;
		}
		*offset += ret;
		*bytes -=ret;
	}
	// eh we're decoding mp3, get back!
	if(((MP3DecInfo *)mdecoder)->layer!=3)
		goto findsync;

	return 0;
}

void parseID3_V2(FILE * fp) {
	rewind(fp);
	/* Jump to the sync save size field (4 bytes, where bit 7 is left unused
	 * to avoid finding keywords (which need to have that bit set)
	 */
	fseek(fp, 6, SEEK_SET);
	char temp[4];
	fread(&temp, 1, 4, fp);
	// extract header size to skip it
	firstFrame =  (temp[0]<<21|temp[1]<<14|temp[2]<<7|temp[3])+ID3_HDR_SIZE;
	fseek(sndFile, firstFrame, SEEK_SET);
	fread(&temp, 1, 3, fp);
	// some mp3's appear to have double headers
	if(IS_ID3_V2(temp)) {
		// get to the size field again
		fseek(fp, firstFrame+6, SEEK_SET);
		fread(&temp, 1, 4, fp);
		firstFrame +=  (temp[0]<<21|temp[1]<<14|temp[2]<<7|temp[3])+ID3_HDR_SIZE;
	}
	fseek(sndFile, firstFrame, SEEK_SET);
	fileSize -= firstFrame;
}

int mp3_openFile(char * name) {
	memset(&inf, 0, sizeof(inf));
	if((sndFile = fopen(name, "rb"))) {
		fileSize = get_fileSize(sndFile);
		char magic[3];
		fread(&magic, 1, 3, sndFile);
		if(IS_ID3_V2(magic)) {
			parseID3_V2(sndFile);
		}
		if((fill_readBuffer(readBuffer, &readOff, READ_BUF_SIZE, &dataLeft))==READ_BUF_SIZE) {
			mdecoder = MP3InitDecoder();
			if(!findValidSync(&readOff, &dataLeft)) {
				/* Get info for mm stream init */
				MP3GetLastFrameInfo(mdecoder, &inf);
				bitrate = inf.bitrate;
				iprintf("duration %d\n", (fileSize)*8/bitrate);
				return 0;
			}
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
int mp3_seek_percentage(int perc) {
	fseek(sndFile, (fileSize / 100)*perc + firstFrame, SEEK_SET);
	dataLeft = 0;
	/* we read max amount of data for one frame, so this should be enough
	 * to find at least one syncword, the streaming routine may then fill
	 * the buffer more when there's an underflow
	 */
	fill_readBuffer(readBuffer, &readOff, READ_BUF_SIZE, &dataLeft);
	// free decoder to prevent crashing
	MP3FreeDecoder(mdecoder);
	mdecoder = MP3InitDecoder();
	findValidSync(&readOff, &dataLeft);
	return 1;
}

int mp3_get_percentage() {
	u32 current = ftell(sndFile);
	current -= firstFrame;
	return ((current*16)/(fileSize/100));
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
			switch(ret) {
			case ERR_MP3_INDATA_UNDERFLOW :
			case ERR_MP3_MAINDATA_UNDERFLOW:
				return 0;
			case ERR_MP3_INVALID_FRAMEHEADER:
				findValidSync(&readOff, &dataLeft);
				return 0;
			default:
				iprintf("HELIX MP3 ERROR: %d", ret);
				needsClosing = true;
				return 0;
			}
		}
		/* GCC can't know channels being only 1 or 2 */
		samps = inf.outputSamps >> (inf.nChans-1);
		dest+=	inf.outputSamps;
		length -=samps;
	}
	visualizeBuffer(dest);

	return samps;
}
