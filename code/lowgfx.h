#ifndef _LOWGFX_H
#define _LOWGFX_H
#include "models.h"

extern float the_view_mat[16];

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

