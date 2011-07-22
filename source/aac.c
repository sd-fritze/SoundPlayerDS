/*
*	MP4 files can contain aac tracks, but they need to be proccessed
*	differently than just raw aac (.aac) files, which are easier
*	to handle.
* 	The streaming functions are based upon the example found in the helix
* 	aac decoder source
*/
#include <nds.h>
#include <helix/aacdec.h>
#include <helix/aaccommon.h>
#include "aac.h"
#include "soundPlayer.h"
#include "decoder_common.h"

/* Helix variabelen*/
HAACDecoder * decoder;
AACFrameInfo inf;

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
		if((infile = mp4ff_open_read(&mp4cb))) {
			if ((track = findAudioTrack(infile)) >= 0) {
				/* Decoder failed to initialize */
				if((decoder = AACInitDecoder())) {
					/* Decoder should be updated to decode raw blocks in the mp4 */
					inf.sampRateCore = mp4ff_get_sample_rate(infile, track);
					inf.nChans = mp4ff_get_channel_count(infile,track);
					/* AACSetRawBlockParams will fail if not set */
					inf.profile = AAC_PROFILE_LC;
					samples = mp4ff_num_samples(infile, track);
					if(!AACSetRawBlockParams(decoder, 0, &inf))
						return 0;
				}
			}
		}
	}
	return -1;	/* sndFile == NULL */
}

int aac_openFile(char * name) {
	readOff = readBuffer;
	if((sndFile = fopen(name, "rb"))) {
		if((decoder = AACInitDecoder())) {
			if(fill_readBuffer(readBuffer, &readOff, READ_BUF_SIZE, &dataLeft) == READ_BUF_SIZE) {
				int ret = 0;
				int bitOffset = 0, bitsAvail = dataLeft << 3;
				if(IS_ADIF(readBuffer))
					ret = UnpackADIFHeader((AACDecInfo*)decoder, &readOff, &bitOffset, &bitsAvail);
				else
					ret = UnpackADTSHeader((AACDecInfo*)decoder, &readOff, &bitOffset, &bitsAvail);
				if(!ret) {
					AACGetLastFrameInfo(decoder, &inf);
					readOff = readBuffer;
					dataLeft = READ_BUF_SIZE;
					return 0;
				}
			}
		}
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
	memset(&inf, 0, sizeof(AACFrameInfo));
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

mm_word aac_on_stream_request( mm_word length, mm_addr dest, mm_stream_formats format ) {
	int decoded = 0;

	/* AACDecode will always output 1024 samples per channel */
	if(length >=1024) {
		int ret = 1;
		/* At least the size of the header should be buffered */
		if(dataLeft < AAC_MAINBUF_SIZE)
			ret = fill_readBuffer(readBuffer, &readOff, AAC_MAINBUF_SIZE, &dataLeft);

		if(dataLeft != AAC_MAINBUF_SIZE && dataLeft == 0) {
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
		decoded +=1024;
	}
	visualizeBuffer(dest);
	return decoded;
}
