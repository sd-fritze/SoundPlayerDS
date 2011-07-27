#include "Graphics.h"

#define TEXTURE_PACK_T16(u, v)   ((((u) << 4) & 0xFFFF) | ((v) << 20))

void glVertex2v16(int x, int y) {
	GFX_VERTEX16 = ((x & 0xffff) | (y<<16));
}

u32 temlate_quadList[] = {
	16,
	FIFO_COMMAND_PACK(FIFO_BEGIN, FIFO_TEX_COORD, FIFO_VERTEX16, FIFO_TEX_COORD),
	GL_QUAD,
	0,
	0,
	0,
	0,
	FIFO_COMMAND_PACK(FIFO_VERTEX_XY, FIFO_TEX_COORD, FIFO_VERTEX_XY, FIFO_TEX_COORD),
	0,
	0,
	0,
	0,
	FIFO_COMMAND_PACK(FIFO_VERTEX_XY, FIFO_NOP, FIFO_NOP, FIFO_NOP),
	0,
	0,0,0,
};

void initGl2D(void) {

	videoSetMode(MODE_0_3D);
	vramSetBankA(VRAM_A_TEXTURE);
	vramSetBankE(VRAM_E_TEX_PALETTE);

	glInit();
	glEnable( GL_TEXTURE_2D );
	glClearColor( 0, 0, 0, 31 ); 	// BG must be opaque for AA to work
	glClearPolyID( 63 ); 			// BG must have a unique polygon ID for AA to work
	glClearDepth( GL_MAX_DEPTH );
	glViewport(0,0,255,191);		// set the viewport to screensize
	glColor( 0x7FFF ); 				// max color
	glPolyFmt( POLY_ALPHA(31) | POLY_CULL_NONE );  // geen dingen laten verdwijnen
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();				// reset view
	glOrthof32( 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1 << 12, 1 << 12 );
	gluLookAt(	0.0, 0.0, 1.0,		//camera possition
	            0.0, 0.0, 0.0,		//look at
	            0.0, 1.0, 0.0);		//up
}

/*	Creates a display list for a quad, be aware that this display list
 *  won't set a texture for you, when calling this list, you have to
 *  make sure the correct textures has been bound
 */
u32 * glCreateQuadList(int x, int y, int x2, int y2, int u1, int v1) {
	u32 * list;
	int u2 = u1 + (x2-x);
	int v2 = v1 + (y2-y);

	if(( list = (u32*)malloc(sizeof(temlate_quadList)))) {
		memcpy(list, temlate_quadList, sizeof(temlate_quadList));
		list[3] 	= TEXTURE_PACK_T16(u1, v1);
		list[4] 	= VERTEX_PACK(x, y);
		list[6] 	= TEXTURE_PACK_T16(u2, v1);
		list[8] 	= VERTEX_PACK(x2, y);
		list[9] 	= TEXTURE_PACK_T16(u2, v2);
		list[10] 	= VERTEX_PACK(x2, y2);
		list[11] 	= TEXTURE_PACK_T16(u1, v2);
		list[13] 	= VERTEX_PACK(x, y2);
		return list;
	}

	return NULL;
}

/*	Sets the position of the quad encapsulated in a
 *  display list and can additionally resize it with
 *  the 3rd and 4th parameters
 */
void setPosQuadList(int x, int y, int x2, int y2, u32 * list) {
	list[4] 	= VERTEX_PACK(x, y);
	list[8] 	= VERTEX_PACK(x2, y);
	list[10] 	= VERTEX_PACK(x2, y2);
	list[13] 	= VERTEX_PACK(x, y2);
}

/* You've to select the desired mode yourself */
void drawLine(int x, int y, int x2, int y2) {
	glVertex3v16(x,y,0);
	glVertex2v16(x2,y2);
	glVertex2v16(x2,y2);
}
