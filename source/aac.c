/*
*	MP4 files can contain aac tracks, but they need to be proccessed
*	differently than just raw aac (.aac) files, which are easier
*	to handle.
* 	The streaming functions are based upon the example found in the helix
* 	aac decoder source
*/
#include <nds.h>
#include "aac.h"
#include "soundPlayer.h"

#define READ_BUF_SIZE 2 * AAC_MAINBUF_SIZE * AAC_MAX_NCHANS
#define AAC_BUFFER_SIZE	8192

s16 * PCMBUF = NULL; // will differ, depending on channel config

/* Helix variabelen*/
HAACDecoder * decoder;
AACFrameInfo inf;
unsigned char readBuffer[READ_BUF_SIZE];
unsigned char * readOff = NULL;
int dataLeft;
FILE * aacFile = NULL;
short Endof; // samples to endof

/* MP4ff variabelen */
int trackSample;
static int samples;
int track;

uint32_t read_callback(void *user_data, void *buffer, uint32_t length) {
	return fread(buffer, 1, length, (FILE*)user_data);
}
uint32_t seek_callback(void *user_data, uint64_t position) {
	return fseek((FILE*)user_data, position, SEEK_SET);
}
mp4ff_t *infile;
mp4ff_callback_t mp4cb = {
	.read = read_callback,
	.seek = seek_callback
};

int findAudioTrack(mp4ff_t * f) {
	int i, numTracks = mp4ff_total_tracks(f);
	for(i =0; i<numTracks; i++) {
		/* Found an audio track */
		if(mp4ff_get_track_type(f, i) == MP4_TRACK_AUDIO)
			return i;
	}
	return -1;
}
/*
 * Input	:	Filename
 * Output	: 	1 if succesful -1 if not
 */
int mp4_openFile(char * name) {

	/* Initialize callback structure */
	aacFile = fopen(name, "rb");
	mp4cb.user_data = aacFile;
	/* Open mp4 file and loop through all atoms */
	infile = mp4ff_open_read(&mp4cb);
	if (!infile) {
		iprintf("Error opening file: %s\n", name);
		return -1;
	}
	if ((track = findAudioTrack(infile)) < 0) {
		iprintf("An audio track couldn't be found in this MP4\n");
		mp4ff_close(infile);
		fclose(aacFile);
		return -1;
	}
	/* Decoder failed to initialize */
	if(!(decoder = AACInitDecoder()))
		return -1;
	/* Decoder should be updated to decode raw blocks in the mp4 */
	inf.sampRateCore = mp4ff_get_sample_rate(infile, track);
	inf.nChans = mp4ff_get_channel_count(infile,track);
	inf.profile = AAC_PROFILE_LC;
	samples = mp4ff_num_samples(infile, track);
	trackSample = 0;
	dataLeft = 0;

	if(AACSetRawBlockParams(decoder, 0, &inf))
		return -1;
	if(!(PCMBUF = malloc(4096)))
		return -1;
	return 0;
};

int aac_openFile(char * name) {
	memset(&inf, 0, sizeof(AACFrameInfo));
	aacFile = fopen(name, "rb");
	if(aacFile == NULL)
		return -1;

	if(!(PCMBUF = malloc(4096)))
		return -1;
	/* Read in the readBuffer to the maximum */
	fread(&readBuffer, 1, READ_BUF_SIZE, aacFile);
	//global_offset += READ_BUF_SIZE;
	/* Init decoder */
	if(!(decoder = AACInitDecoder()))//
		return -1;
	/* Decode one frame to get inf about the aac, otherwhise
	 * get_sampleRate etc will return the wrong values
	 */
	dataLeft = READ_BUF_SIZE;
	int ret = 0;
	readOff = (unsigned char*)&readBuffer;
	if((ret = AACDecode(decoder, &readOff, &dataLeft, (short*)PCMBUF))!=0) {
		iprintf("HELIX AAC ERROR: %d!\n", ret);
		return -1;
	}
	AACGetLastFrameInfo(decoder, &inf);
	Endof = 0;
	return 0;
}

/*
 * Note	: Use only after you've opened up a valid AAC file
 */
int aac_get_sampleRate(void) {
	return inf.sampRateCore;
}

/*
 * Note	: Use only after you've opened up a valid AAC file
 */
int aac_get_nChannels(void) {
	return inf.nChans;
}

void aac_freeDecoder(void) {
	AACFreeDecoder(decoder);
	fclose(aacFile);
	free(PCMBUF);
}

/*
TODO: decode more than 1024 samples at a time and mix both the mp4 and aac on stream r
*/
mm_word mp4_on_stream_request( mm_word length, mm_addr dest, mm_stream_formats format ) {
	int decoded = 0;
	/* We'll always output 1024 samples */
	if(length >=1024) {

		readOff = readBuffer;
		/* The decoder is always fed an aac frame, so this shouldn't be needed*/
		dataLeft = 0;
		while(AACDecode(decoder, &readOff, &dataLeft, (short*)PCMBUF) == ERR_AAC_INDATA_UNDERFLOW) {
			int read = mp4ff_read_sample_v2(infile, track, trackSample++,readOff);
			if(read==0 || trackSample > samples) { // read error or EOF both need closing
				mp4ff_close(infile);
				needsClosing = true;
				return 0;
			}
			dataLeft+=read;
			/* clear old data to prevent the decoder decoding this again */
			memset(readBuffer+dataLeft, 0, READ_BUF_SIZE - dataLeft);
		}
		/* "empty" our writeBuffer, so that we can reuse it */
		memcpy(dest, PCMBUF, inf.nChans * 1024 * 2); // we decoded 2048 samples
		decoded = 1024;	// 1024 a channel
	}
	visualizeBuffer(dest);
	return decoded;
}

static int refill_readBuffer(void) {
	/* Move the already present data to begin */
	memmove(&readBuffer, readOff, dataLeft);
	/* decoding will now start at the beginning */
	readOff = (unsigned char*)&readBuffer;
	/* Read enough to get a fresh complete filled buffer */
	int read = fread(&readBuffer[dataLeft], 1, READ_BUF_SIZE - dataLeft,aacFile);
	if(read != (READ_BUF_SIZE-dataLeft)) {
		if(feof(aacFile) && !Endof) {
			dataLeft+=read;
			return read;
		}
		return 0;
	}
	dataLeft+=read;
	return read;
}

mm_word aac_on_stream_request( mm_word length, mm_addr dest, mm_stream_formats format ) {
	int decoded = 0;
	/* We'll always output 1024 samples, so length should be at least 1024 */
	if(length >=1024) {
		int ret = 1;
		/* really needed, checking for underruns only won't work:
		* in some cases dataLeft will be <4, and so you don't have a complete
		* (ADTS)header, which is really needed
		*/
		if(dataLeft < AAC_MAINBUF_SIZE * inf.nChans)
			ret = refill_readBuffer();

		if(!ret && dataLeft == 0) {
			/* No more to read, still able to decode something more */
			if(feof(aacFile) && !Endof)
				Endof = length;
			/* Still some data in maxmods buffer*/
			else if(length < Endof)
				return 0;

			else {
				needsClosing = true;
				return 0;
			}
		}
		/* Shouldn't be worth checking, as above should take care of no underflows */
		if((ret = AACDecode(decoder, &readOff, &dataLeft, (short*)PCMBUF)) == ERR_AAC_INDATA_UNDERFLOW) {}
		if(ret <-1) {
			iprintf("HELIX AAC ERROR: %d\n", ret);
			needsClosing = true;
			return 0;
		}
		/* "empty" our writeBuffer, so that we can reuse it
		 * AACDecode will always output 1024 samples per channel
		 * hence the *2
		 */
		memcpy(dest, PCMBUF, inf.nChans * 1024 * 2);
		dest+=1024*inf.nChans;
		decoded +=1024;
		length -=1024;
	}

	visualizeBuffer(dest);

	return decoded;
}
