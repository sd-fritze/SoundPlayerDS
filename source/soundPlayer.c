#include <nds.h>
#include <maxmod9.h>
#include <stdio.h>
#include "sound/SoundPlayer.h"
#include "Filebrowser.h"
#include "Graphics.h"
#include "Gui.h"

FILE * sndFile;
MUSIC musik;
bool needsClosing;
unsigned char readBuffer[READ_BUF_SIZE];
unsigned char * readOff;
int dataLeft;
short Endof; // samples to endof

void InitMaxmod(void) {
	mm_ds_system sys;
	sys.mod_count 			= 0;
	sys.samp_count			= 0;
	sys.mem_bank			= 0;
	sys.fifo_channel		= FIFO_MAXMOD;
	mmInit( &sys );
}

/*
 * Note only use when m has it's decoder callbacks set, otherwhise it will fail
 */
void startStream(MUSIC * m, char * name, int bufferlength) {
	if(m->dec->open_file(name) == 0) {
		if(m->dec->get_nChannels()<3) {
			m->mstream.format = ( m->dec->get_nChannels() < 2? MM_STREAM_16BIT_MONO : MM_STREAM_16BIT_STEREO);
			m->mstream.sampling_rate = m->dec->get_sampleRate();
			m->mstream.buffer_length = bufferlength;// buffer length
			m->mstream.timer	= MM_TIMER0;		// use hardware timer 0
			m->mstream.manual	= true;				// use manual filling
			m->mstream.callback = (m->dec->callback);
			mmStreamOpen(&m->mstream);
			bgShow(bg2d);
		} else {
			iprintf("Channelcount too high!\n");
			needsClosing = true;
		}
	}
	/* be sure to reset everything */
	else {
		iprintf("Failed to play %s\n", name);
		m->dec->free_decoder();
		playing = false;
	}
}

void visualizeBuffer(s16 * buffer) {
	glBegin( GL_TRIANGLE_STRIP);
	glBindTexture( 0, 0 );

	int i;
	for(i = 0; i<64; i++) {
		// pick a "random" color
		glColor3b(0, *buffer&0xff, *buffer>>8);
		drawLine(i*4, (*buffer>>8)+96, i*4+4, (buffer[2]>>8)+96);
		buffer++;
	}
	glColor3b(255,255,255);
}
