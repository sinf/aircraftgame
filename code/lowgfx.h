#ifndef _LOWGFX_H
#define _LOWGFX_H
#include "models.h"

extern float the_view_mat[16];
extern float *the_modelview_mat;
void mat_translate_f( float tx, float ty, float tz );
#define mat_translate(x,y,z) mat_translate_f(REALTOF(x),REALTOF(y),REALTOF(z))
void mat_rotate( int axis, float angle );
void mat_scale( float x, float y, float z );
void m_mult( float c[16], const float a[16], const float b[16] );
void mat_push( void );
void mat_pop( void );
void mat_load( float m[16] );
void mat_store( float dst[16] );

typedef enum {
	BLOB_SHARP,
	BLOB_FUZZY
} BlobMode;

typedef struct GfxBlob {
	BlobMode mode;
	U32 color;
	Real x, y;
	Real scale_x, scale_y;
	/* float rotation_angle; */
} GfxBlob;

typedef struct GfxVertex {
	float x, y;
	U32 color;
} GfxVertex;

void draw_models( unsigned num_models, ModelID model_id, const float *matr4x4 );
void draw_blobs( unsigned num_blobs, const GfxBlob blobs[] );
void draw_triangle_strip( unsigned num_verts, const GfxVertex verts[] );

void flush_models( void );
void push_model_mat( ModelID m );

#endif
