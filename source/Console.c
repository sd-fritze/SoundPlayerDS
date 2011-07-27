#include "Console.h"
#include "Graphics.h"

// Graphics
#include "font.h"

int consoleID;
static int consolex, consoley;

void glConsoleInit(u8 * texture, u16 * pal) {

	if(!texture)
		texture = (u8*)fontBitmap;
	if(!pal)
		pal = (u16*)fontPal;
	glGenTextures(1, &consoleID);
	glBindTexture(0, consoleID);
	glTexImage2D(0, 0, GL_RGB256, TEXTURE_SIZE_128 , TEXTURE_SIZE_128, 0, TEXGEN_TEXCOORD, (u8*)texture);
	glColorTableEXT( 0, 0, 256, 0, 0, (u16*)pal);
}

void putChar(char c, int x, int y) {
	int u1 = c % 16;
	int v1 = (c - u1)/16;
	u1 <<= 7; // *8 = <<3 + <<4 for t16
	v1 <<= 7;

	glBegin(GL_QUAD);
	glTexCoord2t16(u1, v1);
	glVertex3v16(x, y, 0);
	glTexCoord2t16(u1+128, v1);
	glVertex3v16(x+8, y,0);
	glTexCoord2t16(u1+128, v1+128);
	glVertex3v16(x+8, y+8,0);
	glTexCoord2t16(u1, v1+128);
	glVertex3v16(x, y+8,0);
	glEnd();

}

void glresetConsole(void) {
	consolex = 0;
	consoley = 0;
}

void glsetConsolePos(int x, int y) {
	consolex = x;
	consoley = y;
}

void glPrint(char * text, int limit) {
	glBindTexture(0, consoleID);
	int teller = 0;
	while(*text) {
		putChar(*text, consolex, consoley);
		consolex+=8;
		/*
		if(consolex > 248) {
			consoley+=8;
			consolex = 0;
		}
		if(consoley > 184) {
			consoley = 0;
		}
		 */
		text++;
		teller++;
		if(teller == limit)
			break;
	}
}
