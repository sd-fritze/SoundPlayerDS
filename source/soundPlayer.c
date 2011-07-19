#include <nds.h>
#include <maxmod9.h>
#include "soundPlayer.h"

bool needsClosing = false;
// fast fourier transformation
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
	if(m->open_file(name) == 0) {
		m->mstream.format = ( m->get_nChannels() < 2? MM_STREAM_16BIT_MONO : MM_STREAM_16BIT_STEREO);
		m->mstream.sampling_rate = m->get_sampleRate();
		m->mstream.buffer_length = bufferlength;// buffer length
		m->mstream.timer	= MM_TIMER0;		// use hardware timer 0
		m->mstream.manual	= true;				// use manual filling
		mmStreamOpen(&m->mstream);
	}

}

void visualizeBuffer(s16 * buffer) {

	glBegin(GL_TRIANGLE_STRIP);
	glColor3b(0,0,255);

	int i, x = inttov16(-1);
	/* Display 64 samples */
	for(i=0; i<63; i++, buffer++) {
		glVertex3v16(x,((buffer[0]+buffer[1])>>4),0); // /2 and >>3 to make it fit in v16
		x+= 64;
		glVertex3v16(x, ((buffer[2]+buffer[3])>>4), 0);
		glVertex3v16(x, ((buffer[2]+buffer[3])>>4), 0);
		x+= 64;
	}
	glEnd();
}
