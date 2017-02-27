#ifndef _MATRIX_H
#define _MATRIX_H
#include "system.h"

extern const float M_IDENTITY[16];
#define m_copy(dst,src) mem_copy((dst),(src),sizeof(float)*16)

void m_translate( float m[16], float x, float y, float z );
void m_rotate( float m[16], int axis, float angle );
void m_scale( float m[16], float x, float y, float z );
float *m_mult( float m[16], const float a[16], const float b[16] );

void m_mult_v3( float v[3], float m[16], float x, float y, float z );

#define MAT_STACK_DEPTH 32
extern float mat_stack[MAT_STACK_DEPTH][16];
extern float *ms_cur;

void ms_push( void );
void ms_pop( void );
#define ms_load(m) m_copy(ms_cur,(m))
#define ms_store(m) m_copy((m),ms_cur)
void ms_translate( float x, float y, float z );
void ms_rotate( int axis, float angle );
void ms_scale( float x, float y, float z );
void ms_premult( float m[16] );
void ms_postmult( float m[16] );

#define ms_translate_r(x,y,z) ms_translate(REALTOF(x),REALTOF(y),REALTOF(z))

#endif

