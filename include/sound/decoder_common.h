#ifndef DECODER_COMMON_H
#define DECODER_COMMON_H
#include <nds.h>

/* Fills readbuffer and updates offset & left:
 * 
 * buffer:	buffer to fill
 * offset:	offset to start at
 * bufSize:	size of the readbuffer
 * left:	data left in the readbuffer
 */
int fill_readBuffer(u8 * buffer, u8 ** offset, int bufSize, int * left);

#endif /* DECODER_COMMON_H */