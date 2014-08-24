#include <stdlib.h>
#include <GL/gl.h>
#include "support.h"
#include "graphics.h"
#include "lowgfx.h"

#define BLOB_TEX_S 32

static void generate_blob_texels( U32 texels[BLOB_TEX_S][BLOB_TEX_S] )
{
	int x, y;
	for( y=0; y<BLOB_TEX_S; y++ ) {
		for( x=0; x<BLOB_TEX_S; x++ ) {
			const U32 r = (BLOB_TEX_S-1) * (BLOB_TEX_S-1);
			int dx = 2 * x + 1 - BLOB_TEX_S;
			int dy = 2 * y + 1 - BLOB_TEX_S;
			U32 d = dy*dy + dx*dx;
			#if 0
			if ( r < d ) {
				d = 0;
			} else {
				d = ( r - d << 8 ) / r;
				ASSERT( d <= 255 );
				d <<= 24;
			}
			d |= 0xFFFFFF;
			texels[y][x] = d;
			#else
			texels[y][x] = d <= r ? 0xFFFFFFFF : 0xFFFFFF;
			#endif
		}
	}
}

static void generate_blob_texture( void )
{
	U32 texels[BLOB_TEX_S][BLOB_TEX_S];
	GLuint tex;
	generate_blob_texels( texels );
	glGenTextures( 1, &tex );
	glBindTexture( GL_TEXTURE_2D, tex );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, BLOB_TEX_S, BLOB_TEX_S, 0, GL_RGBA, GL_UNSIGNED_BYTE, &texels[0][0] );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
}

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
	
	generate_blob_texture();
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

#if DEBUG
static int mat_level = 0;
#endif

void mat_push( void )
{
	#if DEBUG
	++mat_level;
	#endif
	glPushMatrix();
}

void mat_pop( void )
{
	#if DEBUG
	--mat_level;
	ASSERT( mat_level >= 0 );
	#endif
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

void draw_models( unsigned num_models, ModelID m, const float *matr )
{
	U8 num_indices = MODEL_INFO[m].num_indices;
	U8 const *index_p = model_indices_unpacked[m];
	unsigned n;
	
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

struct BlobVertex {
	float x, y;
	float s, t;
};

void draw_blobs( unsigned num_blobs, const GfxBlob blobs[] )
{
	struct BlobVertex (*quads)[4];
	U32 *colors;
	unsigned n;
	
	quads = alloca( num_blobs * sizeof(struct BlobVertex) * 4 );
	colors = alloca( num_blobs * 4 * 4 );
	
	for( n=0; n<num_blobs; n++ ) {
		
		float rx = blobs[n].scale_x * 0.5f;
		float ry = blobs[n].scale_y * 0.5f;
		float west = blobs[n].x - rx;
		float north = blobs[n].y - ry;
		float east = blobs[n].x + rx;
		float south = blobs[n].y + ry;
		unsigned i;
		
		quads[n][0].x = west;
		quads[n][0].y = north;
		quads[n][0].s = 0;
		quads[n][0].t = 0;
		
		quads[n][1].x = west;
		quads[n][1].y = south;
		quads[n][1].s = 0;
		quads[n][1].t = 1;
		
		quads[n][2].x = east;
		quads[n][2].y = south;
		quads[n][2].s = 1;
		quads[n][2].t = 1;
		
		quads[n][3].x = east;
		quads[n][3].y = north;
		quads[n][3].s = 1;
		quads[n][3].t = 0;
		
		for( i=0; i<4; i++ )
			colors[4*n+i] = blobs[n].color;
	}
	
	glEnable( GL_TEXTURE_2D );
	glEnableClientState( GL_COLOR_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	
	glColorPointer( 4, GL_UNSIGNED_BYTE, 0, colors );
	glVertexPointer( 2, GL_FLOAT, 4 * sizeof( float ), &quads[0][0].x );
	glTexCoordPointer( 2, GL_FLOAT, 4 * sizeof( float ), &quads[0][0].s );
	glDrawArrays( GL_QUADS, 0, num_blobs * 4 );
	
	glDisable( GL_TEXTURE_2D );
	glDisableClientState( GL_COLOR_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
}

void draw_triangle_strip( unsigned num_verts, const GfxVertex verts[] )
{
	glEnableClientState( GL_COLOR_ARRAY );
	glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof( GfxVertex ), &verts[0].color );
	glVertexPointer( 2, GL_FLOAT, sizeof( GfxVertex ), &verts[0].x );
	glDrawArrays( GL_TRIANGLE_STRIP, 0, num_verts );
	glDisableClientState( GL_COLOR_ARRAY );
}
