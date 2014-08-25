#ifndef _LOWGFX_H
#define _LOWGFX_H
#include "models.h"

void init_gfx( void );

void draw_box( float x, float y, float w, float h, U32 color );
void draw_grid( float x0, float y0, float cell_w, float cell_h, unsigned cells_x, unsigned cells_y, U32 color );

void mat_translate( float tx, float ty, float tz );
void mat_rotate( int axis, float angle );
void mat_scale( float x, float y, float z );
void mat_push( void );
void mat_pop( void );
void mat_store( float dst[16] );

typedef enum {
	BLOB_SHARP,
	BLOB_FUZZY
} BlobMode;

typedef struct GfxBlob {
	BlobMode mode;
	U32 color;
	float x, y;
	float scale_x, scale_y;
	/* float rotation_angle; */
} GfxBlob;

typedef struct GfxVertex {
	float x, y;
	U32 color;
} GfxVertex;

void draw_models( unsigned num_models, ModelID model_id, const float *matr4x4 );
void draw_blobs( unsigned num_blobs, const GfxBlob blobs[] );
void draw_triangle_strip( unsigned num_verts, const GfxVertex verts[] );

#endif
