#include <stdlib.h>
#include <string.h>
#include <GL/gl.h>
#include "support.h"
#include "graphics.h"
#include "lowgfx.h"

enum { MAT_STACK_SIZE = 32 };
static float the_matrix_stack[MAT_STACK_SIZE][16] = {{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
float *the_modelview_mat = &the_matrix_stack[0][0];

/* Computes M*T where T is the translation matrix that contains tx,ty,tz */
static void m_translate( float m[16], float tx, float ty, float tz )
{
	m[12] += m[0]*tx + m[4]*ty + m[8]*tz;
	m[13] += m[1]*tx + m[5]*ty + m[9]*tz;
	m[14] += m[2]*tx + m[6]*ty + m[10]*tz;
	m[15] += m[3]*tx + m[7]*ty + m[11]*tz;
}
void mat_translate_f( float tx, float ty, float tz )
{
	m_translate( the_modelview_mat, tx, ty, tz );
}
void m_mult( float c[16], const float a[16], const float b[16] )
{
	int i, j, k;
	for( i=0; i<4; ++i ) {
		for( j=0; j<4; ++j ) {
			float s = 0;
			for( k=0; k<4; ++k )
				s += a[i*4+k] * b[k*4+j];
			c[i*4+j] = s;
		}
	}
}
void mat_rotate( int axis, float angle )
{
	float s, c, *in, *out;
	out = the_modelview_mat;
	mat_push();
	in = the_modelview_mat;
	mat_pop();
	s = sinf( angle );
	c = cosf( angle );
	switch( axis )
	{
		case 0:
			out[0] = in[0];
			out[1] = in[1];
			out[2] = in[2];
			out[3] = in[3];
			out[4] = (in[4]*c+in[8]*s);
			out[5] = (in[5]*c+in[9]*s);
			out[6] = (in[6]*c+in[10]*s);
			out[7] = (in[7]*c+in[11]*s);
			out[8] = (in[4]*-s+in[8]*c);
			out[9] = (in[5]*-s+in[9]*c);
			out[10] = (in[6]*-s+in[10]*c);
			out[11] = (in[7]*-s+in[11]*c);
			out[12] = in[12];
			out[13] = in[13];
			out[14] = in[14];
			out[15] = in[15];
			break;
		case 1:
			out[0] = (in[0]*c-in[8]*s);
			out[1] = (in[1]*c-in[9]*s);
			out[2] = (in[2]*c-in[10]*s);
			out[3] = (in[3]*c-in[11]*s);
			out[4] = in[4];
			out[5] = in[5];
			out[6] = in[6];
			out[7] = in[7];
			out[8] = (in[0]*s+in[8]*c);
			out[9] = (in[1]*s+in[9]*c);
			out[10] = (in[2]*s+in[10]*c);
			out[11] = (in[3]*s+in[11]*c);
			out[12] = in[12];
			out[13] = in[13];
			out[14] = in[14];
			out[15] = in[15];
			break;
		case 2:
			out[0] = (in[0]*c+in[4]*s);
			out[1] = (in[1]*c+in[5]*s);
			out[2] = (in[2]*c+in[6]*s);
			out[3] = (in[3]*c+in[7]*s);
			out[4] = (in[0]*-s+in[4]*c);
			out[5] = (in[1]*-s+in[5]*c);
			out[6] = (in[2]*-s+in[6]*c);
			out[7] = (in[3]*-s+in[7]*c);
			out[8] = in[8];
			out[9] = in[9];
			out[10] = in[10];
			out[11] = in[11];
			out[12] = in[12];
			out[13] = in[13];
			out[14] = in[14];
			out[15] = in[15];
			break;
		default:
			ASSERT( 0 );
	}
}
void mat_scale( float x, float y, float z )
{
	float *m = the_modelview_mat;
	int i;
	for( i=0; i<4; i++ ) {
		m[i] *= x;
		m[4+i] *= y;
		m[8+i] *= z;
	}
}
void mat_push( void )
{
	float *old = the_modelview_mat;
	the_modelview_mat += 16;
	ASSERT( the_modelview_mat < the_matrix_stack[MAT_STACK_SIZE] );
	memcpy( the_modelview_mat, old, sizeof( float ) * 16 );
}
void mat_pop( void )
{
	the_modelview_mat -= 16;
	ASSERT( the_modelview_mat >= the_matrix_stack[0] );
}
void mat_load( float m[16] )
{
	memcpy( the_modelview_mat, m, sizeof( float ) * 16 );
}
void mat_store( float dst[16] )
{
	memcpy( dst, the_modelview_mat, sizeof( float ) * 16 );
}

typedef S32 (Vertex[3]);
void draw_models( unsigned num_models, ModelID m, const float *matr )
{
	//int wire = 0;
	int num_indices = MODEL_INFO[m].num_indices;
	U8 const *index_p = model_indices_unpacked[m];
	S32 const *verts = model_vertices_unpacked[m];
	unsigned n;

	//use_color( MODEL_INFO[m].color );
	//glVertexPointer( MODEL_DIMENSIONS( MODEL_INFO[m] ), GL_INT, 0, model_vertices_unpacked[m] );
	//if ( wire ) glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	
	for( n=0; n<num_models; n++ ) {
		float *mat = matr + 16*n;
		set_mvp_matrix_f( mat );
		draw_quads( (const Vertex*) verts, index_p, num_indices, 0 );

		//glDrawElements( GL_QUADS, num_indices, GL_UNSIGNED_BYTE, index_p );
		if ( 0 ) { //MODEL_INFO[m].flags & F_MIRROR ) {
			mat_push();
			mat_load( mat );
			mat_scale( 1, 1, -1 );
			set_mvp_matrix_f( the_modelview_mat );
			draw_quads( (const Vertex*) verts, index_p, num_indices, 0 );
			mat_pop();
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
		m_mult( model_inst_matr[m][i], the_modelview_mat, the_view_mat );
		//mat_store( model_inst_matr[m][i] );
	}
}

void flush_models( void )
{
	int n;
	for( n=0; n<NUM_MODELS; n++ ) {
		draw_models( model_inst_count[n], n, &model_inst_matr[n][0][0] );
		model_inst_count[n] = 0;
	}
}

