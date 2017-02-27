#ifndef _DRAW_H
#define _DRAW_H
#include "system.h"

#define MAX_RESY 640
#define r_resx 720

#define SCREEN_W (2*r_resx)
#define SCREEN_H (2*r_resx*9/16)

extern int r_resy;
extern int r_pitch;
extern U32 *r_canvas;
extern int vertex_data_dim;

void set_mvp_matrix_f( const float m[16] );
void flip_mvp_matrix_z( void );
void draw_quads( const S32 verts[], const U8 idx[], int n_idx, U32 color );

void render( void );

#endif

