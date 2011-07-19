/*
*	MP4 files can contain aac tracks, but they need to be proccessed
*	differently than just raw aac (.aac) files, which are easier
*	to handle.
* 	The streaming functions are based upon the example found in the helix
* 	aac decoder source
*/

#include "aac.h"
#include "soundPlayer.h"

#define READ_BUF_SIZE 2 * AAC_MAINBUF_SIZE * AAC_MAX_NCHANS
s16 * PCMBUF = NULL; // 2*1024 samples, for stereo output

/* Helix */
HAACDecoder * decoder;
AACFrameInfo inf;
unsigned char * readBuffer = NULL;
unsigned char * readOff = NULL;
int dataLeft;
FILE * aacFile = NULL;
 
/* MP4ff */
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
	if(!(readBuffer = malloc(READ_BUF_SIZE)) || !(PCMBUF = malloc(4096)))
		return -1;
	return 0;
};

int aac_openFile(char * name) {
	memset(&inf, 0, sizeof(AACFrameInfo));
	aacFile = fopen(name, "rb");
	if(aacFile == NULL)
		return -1;

	if(!(readBuffer = malloc(READ_BUF_SIZE)) || !(PCMBUF = malloc(4096)))
		return -1;
	/* Read in the readBuffer to the maximum */
	fread(readBuffer, 1, READ_BUF_SIZE, aacFile);
	/* Init decoder */
	if(!(decoder = AACInitDecoder()))
		return -1;
	/* Decode one frame to get inf about the aac, otherwhise
	 * get_sampleRate etc will return the wrong values
	 */
	dataLeft = READ_BUF_SIZE;
	int ret = 0;
	readOff = readBuffer;
	if((ret = AACDecode(decoder, &readOff, &dataLeft, (short*)PCMBUF))!=0)
	{
		iprintf("First decode error %d!\n", ret);
		return -1;
	}
	iprintf("%p, %p",readOff, readBuffer);
	AACGetLastFrameInfo(decoder, &inf);
	iprintf("%d\n", inf.sampRateCore);
	return 0;
}

/* 
 * Note		: Use only after you've opened up a valid AAC file
 */
int aac_get_sampleRate(void)
{	
	return inf.sampRateCore;
}

/*
 * Note		: Use only after you've opened up a valid AAC file
 */ 
int aac_get_nChannels(void)
{
	return inf.nChans;
}

void aac_freeDecoder(void) {
	AACFreeDecoder(decoder);
	fclose(aacFile);
	free(PCMBUF);
	free(readBuffer);
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
			if(read==0 || trackSample > samples) // read error or EOF both need closing
			{
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

mm_word aac_on_stream_request( mm_word length, mm_addr dest, mm_stream_formats format ) {
	int decoded = 0;
	/* We'll always output 1024 samples, so length should be at least 1024 */
	if(length >=1024) {
		int ret;
		int read = 0;
		
		while((ret = AACDecode(decoder, &readOff, &dataLeft, (short*)PCMBUF)) == ERR_AAC_INDATA_UNDERFLOW) {
			/* When readOff == readBuffer, there's no data Left */
			memmove(readBuffer, readOff, dataLeft);
			readOff = readBuffer;
			read = fread(readBuffer + dataLeft, 1, READ_BUF_SIZE - dataLeft,aacFile);
			if(read != (READ_BUF_SIZE-dataLeft) && feof(aacFile))
			{
				needsClosing = true;
				return 0;
			}
			if(ret <-1)
				iprintf("Decoding error lhelixaac: %d\n", ret);
			dataLeft+=read;//
		}
		iprintf("%d, %p, %p\n", read, readOff, readBuffer);
		/* "empty" our writeBuffer, so that we can reuse it */
		memcpy(dest, PCMBUF, inf.nChans * 1024 * 2); // we decoded 1024 * channels samples
		dest+=1024*inf.nChans;
		decoded +=1024;
		length -=1024;
	}
	
	visualizeBuffer(dest);
	
	return decoded;
}
