#ifndef _DRAW_H
#define _DRAW_H
#include "system.h"

#define MAX_RESY 640
#define r_resx 480
extern int r_resy;
extern int r_pitch;
extern U32 *r_canvas;

void set_mvp_matrix_f( const float m[16] );
void draw_quads( const S32 verts[][3], const U8 idx[], int n_idx, U32 color );

void render( void );

#endif

