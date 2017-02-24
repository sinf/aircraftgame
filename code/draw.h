#ifndef _DRAW_H
#define _DRAW_H
#include "system.h"

#define MAX_RESY 640
#define r_resx 480
extern int r_resy;
extern int r_pitch;
extern U32 *r_canvas;

void render( void );

#endif

