#include <stdio.h>
#include "decoder_common.h"
#include "SoundPlayer.h"

int fill_readBuffer(u8 * buffer, u8 ** offset, int bufSize, int * left) {
	/* Move chunk at end to start */
	memmove(buffer, *offset, *left);
	int ret = fread(buffer+*left, 1, bufSize-*left, sndFile);
	*offset = buffer;
	*left += ret;
	
	return ret;
}
