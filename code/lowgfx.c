#include <stdlib.h>
#include <string.h>
#include <GL/gl.h>
#include "support.h"
#include "graphics.h"
#include "lowgfx.h"
#include "matrix.h"

void draw_models( unsigned num_models, ModelID m, const float *matr )
{
	//int wire = 0;
	int num_indices = MODEL_INFO[m].num_indices;
	U8 const *index_p = model_indices_unpacked[m];
	S32 const *verts = model_vertices_unpacked[m];
	U32 color = MODEL_INFO[m].color;
	unsigned n;

	//use_color( MODEL_INFO[m].color );
	//glVertexPointer( MODEL_DIMENSIONS( MODEL_INFO[m] ), GL_INT, 0, model_vertices_unpacked[m] );
	//if ( wire ) glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	vertex_data_dim = MODEL_DIMENSIONS( MODEL_INFO[m] );
	
	for( n=0; n<num_models; n++ ) {
		const float *mat = matr + 16*n;

		set_mvp_matrix_f( mat );
		draw_quads( verts, index_p, num_indices, color );

		if ( MODEL_INFO[m].flags & F_MIRROR ) {
			flip_mvp_matrix_z();
			draw_quads( verts, index_p, num_indices, color );
		}
	}
}

#define MAX_MODEL_INST 512
static float model_inst_matr[NUM_MODELS][MAX_MODEL_INST][16] = {{{0}}};
static unsigned model_inst_count[NUM_MODELS] = {0};

void push_model_mat( ModelID m )
{
	if ( model_inst_count[m] < MAX_MODEL_INST ) {
		unsigned i = model_inst_count[m]++;
		//m_mult( model_inst_matr[m][i], ms_cur, the_view_mat );
		m_copy( model_inst_matr[m][i], ms_cur );
	}
}

void flush_models( void )
{
	int n;
	for( n=0; n<NUM_MODELS; n++ ) {
		draw_models( model_inst_count[n], n, &model_inst_matr[n][0][0] );
		model_inst_count[n] = 0;
	}
	vertex_data_dim = 3;
}

