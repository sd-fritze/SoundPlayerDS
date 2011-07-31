#include "SoundPlayer.h"
/*
 * Decoder configurations, support for other formats can easily be added
 * by adding the right function pointers in a DECODER struct
 */
DECODER m4a_dec = {
	mp4_openFile,
	aac_get_sampleRate,
	aac_get_nChannels,
	m4a_seek_percentage,
	m4a_get_percentage,
	aac_freeDecoder,
	mp4_on_stream_request,
};

DECODER aac_dec = {
	aac_openFile,
	aac_get_sampleRate,
	aac_get_nChannels,
	aac_seek_percentage,
	aac_get_percentage,
	aac_freeDecoder,
	aac_on_stream_request,
};

DECODER vbis_dec = {
	vbis_openFile, //
	vbis_get_sampleRate,
	vbis_get_nChannels,
	vbis_seek_percentage,
	vbis_get_percentage,
	vbis_freeDecoder,
	vbis_on_stream_request,
};
	
DECODER mp3_dec = {
	mp3_openFile,
	mp3_get_sampleRate,
	mp3_get_nChannels,
	mp3_seek_percentage,
	mp3_get_percentage,
	mp3_freeDecoder,
	mp3_on_stream_request,
};	
	