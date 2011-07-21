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

/* Helix variabelen*/
HAACDecoder * decoder;
AACFrameInfo inf;
unsigned char readBuffer[AAC_MAINBUF_SIZE];
unsigned char * readOff;
int dataLeft;
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

	if((sndFile = fopen(name, "rb"))) {
		mp4cb.user_data = sndFile;
		/* Open mp4 file and loop through all atoms */
		infile = mp4ff_open_read(&mp4cb);
		if (!infile) {
			iprintf("Error opening file: %s\n", name);
			return -1;
		}
		if ((track = findAudioTrack(infile)) < 0) {
			iprintf("An audio track couldn't be found in this MP4\n");
			mp4ff_close(infile);
			fclose(sndFile);
			return -1;
		}
		/* Decoder failed to initialize */
		if(!(decoder = AACInitDecoder()))
			return -1;
		/* Decoder should be updated to decode raw blocks in the mp4 */
		inf.sampRateCore = mp4ff_get_sample_rate(infile, track);
		inf.nChans = mp4ff_get_channel_count(infile,track);
		/* AACSetRawBlockParams will fail if not set */
		inf.profile = AAC_PROFILE_LC;
		samples = mp4ff_num_samples(infile, track);
		if(AACSetRawBlockParams(decoder, 0, &inf))
			return -1;
		return 0;
	}
	return -1;	/* sndFile == NULL */
};

int aac_openFile(char * name) {
	memset(&inf, 0, sizeof(AACFrameInfo));
	if((sndFile = fopen(name, "rb"))) {
		/* Init decoder */
		if(!(decoder = AACInitDecoder()))//
			return -1;
		/* Decode one frame to get inf about the aac, otherwhise
		 * get_sampleRate etc will return the wrong values.
		 * Todo: check support for ADIF headers
		 */
		unsigned char * off = malloc(8);
		int left = fread(off, 1, 8, sndFile);
		/*
		if(IS_ADIF(off)) {

			// 4 bytes are read already 
			int toread = sizeof(ADIFHeader)-4;
			// Copybit not set, no copyright ID of 72 bytes following 
			if(!off[4])
				toread -= ADIF_COPYID_SIZE;
			//Including previous read bytes 
			unsigned char * more = realloc(off, toread + 8);
			if(!more) {
				free(off);
				return -1;
			}
			fread((off+8), 1, toread, sndFile);
			int numPCE = off[11 + ADIF_COPYID_SIZE*off[0]]+1;

			more = realloc(off, toread + 8 + numPCE * sizeof(ProgConfigElement));
			if(!more) {
				free(off);
				return -1;
			}
			off = more;
			fread(off + toread + 8, 1, numPCE * sizeof(ProgConfigElement), sndFile);
		}
		*/
		AACDecode(decoder, &off, &left, (short*)readBuffer);
		AACGetLastFrameInfo(decoder, &inf);
		free(off);
		rewind(sndFile);

		return 0;
	}
	return -1;	/* sndFile == NULL */
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
	dataLeft = 0;
	Endof = 0;
	trackSample = 0;
	AACFreeDecoder(decoder);
	fclose(sndFile);
}

/*
TODO: decode more than 1024 samples at a time and mix both the mp4 and aac on stream r
*/
mm_word mp4_on_stream_request( mm_word length, mm_addr dest, mm_stream_formats format ) {
	int decoded = 0;
	int ret = 0;
	/* We'll always output 1024 (nah almost) samples */
	if(length >=1024) {
		readOff = readBuffer;
		int read = 0;
		/* The decoder is always fed one aac frame */
		if(trackSample < samples) {
			read = mp4ff_read_sample_v2(infile, track, trackSample++,readOff);
			dataLeft+=read;
			memset(readBuffer+dataLeft, 0, AAC_MAINBUF_SIZE - dataLeft);
			ret = AACDecode(decoder, &readOff, &dataLeft, dest);
			if(trackSample == samples && !Endof)
				Endof = MM_BUFFER_SIZE;
			decoded += 1024;	// 1024 a channel
		}
		/* Either a sample read error occured , an decoding error occured or simply EOF */
		if((!read && !Endof) || ret || (length >= Endof && Endof)) {
			mp4ff_close(infile);
			needsClosing = true;
			return 0;
		}

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
	int read = fread(&readBuffer[dataLeft], 1, AAC_MAINBUF_SIZE - dataLeft,sndFile);
	if(read != (AAC_MAINBUF_SIZE-dataLeft)) {
		if(feof(sndFile) && !Endof) {
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

	/* We'll always output 1024 samples, so length should be at least 1024
	 * so that we won't corrupt any data in maxmods pcm buffer
	 */
	if(length >=1024) {
		int ret = 1;
		/* really needed, checking for underruns only won't work:
		* in some cases dataLeft will be <4, and so you don't have a complete
		* (ADTS)header resulting in decoder errors
		*/
		if(dataLeft < AAC_MAINBUF_SIZE * inf.nChans)
			ret = refill_readBuffer();

		if(!ret && dataLeft == 0) {
			/* No more to read, still able to decode something more */
			if(feof(sndFile) && !Endof)
				Endof = MM_BUFFER_SIZE;
			/* Still some data in maxmods buffer*/
			else if(length < Endof)
				return 0;
			else {
				needsClosing = true;
				return 0;
			}
		}
		/* Shouldn't be worth checking, as above should take care of no underflows */
		if((ret = AACDecode(decoder, &readOff, &dataLeft, dest)) == ERR_AAC_INDATA_UNDERFLOW) {}
		if(ret <-1) {
			iprintf("HELIX AAC ERROR: %d\n", ret);
			needsClosing = true;
			return 0;
		}
		
		dest+=1024*inf.nChans;
		decoded +=1024;
		length -=1024;
	}

	visualizeBuffer(dest);

	return decoded;
}
