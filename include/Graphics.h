#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <nds.h>

/* Initialiseer de 3d engine om "2d" te kunnen weergeven */
void initGl2D(void);

/* Creeër een display list voor een vierkant */
u32 * glCreateQuadList(int x, int y, int x2, int y2, int u1, int v1);
/* Verander de positie en optioneel ook de grootte van vierkantsdisplaylist */
void setPosQuadList(int x, int y, int x2, int y2, u32 * list);

/* Draws a lines by drawing a triangle with two points having the same coordinates 
 * Please be aware that you've to set the drawing mode (TRIANGLE(STRIP)) yourself
 * before executing this function
 */
void drawLine(int x, int y, int x2, int y2);

#endif /* GRAPHICS_H */