#include <GL/gl.h>
#include "support.h"
#include "graphics.h"
#include "lowgfx.h"

void init_gfx( void )
{
	glViewport( 0, 0, SCREEN_W, SCREEN_H );
	glMatrixMode( GL_PROJECTION );
	glLoadMatrixf( MAT_PROJECTION );
	glMatrixMode( GL_MODELVIEW );
	glLoadMatrixf( MAT_IDENTITY );
	
	
	glEnableClientState( GL_VERTEX_ARRAY );
	
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	
	glEnable( GL_POINT_SMOOTH );
	glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );
	glPointSize( 8 );
}

void mat_translate( float tx, float ty, float tz )
{
	glTranslatef( tx, ty, tz );
}

void mat_rotate( int axis, float angle )
{
	angle = DEGREES( angle );
	glRotatef( angle, axis == 0, axis == 1, axis == 2 );	
}

void mat_scale( float x, float y, float z )
{
	glScalef( x, y, z );
}

static int mat_level = 0;
void mat_push( void )
{
	++mat_level;
	glPushMatrix();
}

void mat_pop( void )
{
	if ( --mat_level < 0 ) {
		extern int puts( const char * );
		extern void abort( void );
		puts( "Matrix stack underflow\n" );
		abort();
	}
	glPopMatrix();
}

void mat_store( float dst[16] )
{
	glGetFloatv( GL_MODELVIEW_MATRIX, dst );
}

static void use_color( U32 color )
{
	glColor4ub( color & 0xFF, color >> 8 & 0xFF, color >> 16 & 0xFF, color >> 24 & 0xFF );
}

void draw_box( float x, float y, float w, float h, U32 color )
{
	use_color( color );
	glRectf( x, y, x+w, y+h );
}

void draw_grid( float x0, float y0, float cell_w, float cell_h, unsigned cells_x, unsigned cells_y, U32 color )
{
	float verts[2048][2][2];
	unsigned n;
	float x1 = x0 + cell_w * cells_x;
	float y1 = y0 + cell_h * cells_y;
	float x, y;
	
	for( x=x0, n=0; n<cells_x; n++ ) {
		verts[n][0][0] = x;
		verts[n][0][1] = y0;
		verts[n][1][0] = x;
		verts[n][1][1] = y1;
		x += cell_w;
	}
	
	use_color( color );
	glVertexPointer( 2, GL_FLOAT, 0, &verts[0][0][0] );
	glDrawArrays( GL_LINES, 0, cells_x * 2 );
	
	for( y=y0, n=0; n<cells_y; n++ ) {
		verts[n][0][0] = x0;
		verts[n][0][1] = y;
		verts[n][1][0] = x1;
		verts[n][1][1] = y;
		y += cell_h;
	}
	
	glDrawArrays( GL_LINES, 0, cells_y * 2 );
}

/*
void set_blending( int on )
{
	if ( on )
		glEnable( GL_BLEND );
	else
		glDisable( GL_BLEND );
}

void set_wireframe( int on )
{
	glLineWidth( on ? 2 : 1 );
	glPolygonMode( GL_FRONT_AND_BACK, on ? GL_LINE : GL_FILL );
}
*/

void draw_models( int num_models, ModelID m, const float *matr )
{
	U8 num_indices = MODEL_INFO[m].num_indices;
	U8 const *index_p = model_indices_unpacked[m];
	int n;
	
	use_color( MODEL_INFO[m].color );
	glVertexPointer( MODEL_DIMENSIONS( MODEL_INFO[m] ), GL_INT, 0, model_vertices_unpacked[m] );
	
	glPushMatrix();
	
	for( n=0; n<num_models; n++ ) {
		glLoadMatrixf( matr + 16 * n );
		glDrawElements( GL_QUADS, num_indices, GL_UNSIGNED_BYTE, index_p );
		if ( MODEL_INFO[m].flags & F_MIRROR ) {
			glScalef( 1, 1, -1 );
			glDrawElements( GL_QUADS, num_indices, GL_UNSIGNED_BYTE, index_p );
		}
	}
	
	glPopMatrix();
}

void draw_blobs( int num_blobs, const GfxBlob blobs[] )
{
	glEnableClientState( GL_COLOR_ARRAY );
	glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof( GfxBlob ), &blobs[0].color );
	glVertexPointer( 2, GL_FLOAT, sizeof( GfxBlob ), &blobs[0].x );
	glDrawArrays( GL_POINTS, 0, num_blobs );
	glDisableClientState( GL_COLOR_ARRAY );
}

void draw_triangle_strip( int num_verts, const GfxVertex verts[] )
{
	glEnableClientState( GL_COLOR_ARRAY );
	glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof( GfxVertex ), &verts[0].color );
	glVertexPointer( 2, GL_FLOAT, sizeof( GfxVertex ), &verts[0].x );
	glDrawArrays( GL_TRIANGLE_STRIP, 0, num_verts );
	glDisableClientState( GL_COLOR_ARRAY );
}
