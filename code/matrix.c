#include "system.h"
#include "matrix.h"
#include "support.h"

const float M_IDENTITY[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};

void m_translate( float m[16], float x, float y, float z )
{
	m_copy( m, M_IDENTITY );
	m[12] = x;
	m[13] = y;
	m[14] = z;
}

#define setm(m,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15) do { \
	m[0]=m0;m[1]=m1;m[2]=m2;m[3]=m3;m[4]=m4;m[5]=m5;m[6]=m6;m[7]=m7;m[8]=m8;m[9]=m9;m[10]=m10;m[11]=m11;m[12]=m12;m[13]=m13;m[14]=m14;m[15]=m15; \
} while(0)

void m_rotate( float m[16], int axis, float angle )
{
	int I[][4] = {
		{5,10,6,9},
		{0,10,8,2},
		{0,5,1,4}
	};
	int *i = I[axis];
	int c0=i[0], c1=i[1], sn=i[2], sp=i[3];
	float c = cosf( angle );
	float s = sinf( angle );
	m_copy( m, M_IDENTITY );
	m[c0] = c;
	m[c1] = c;
	m[sn] = -s;
	m[sp] = s;
}

void m_scale( float m[16], float x, float y, float z )
{
	m_copy( m, M_IDENTITY );
	m[0] = x;
	m[5] = y;
	m[10] = z;
}

float *m_mult( float c[16], const float a[16], const float b[16] )
{
	int i, j, k;
	float c1[16];
	for( i=0; i<4; ++i ) {
		for( j=0; j<4; ++j ) {
			float s = 0;
			for( k=0; k<4; ++k )
				s += a[i+k*4] * b[k+j*4];
			c1[j*4+i] = s;
		}
	}
	m_copy( c, c1 );
	return c;
}

void m_mult_v3( float v[3], float m[16], float x, float y, float z )
{
	v[0] = x*m[0] + y*m[4] + z*m[8] + m[12];
	v[1] = x*m[1] + y*m[5] + z*m[9] + m[13];
	v[2] = x*m[2] + y*m[6] + z*m[10] + m[14];
}

#define MAT_STACK_DEPTH 32
float mat_stack[MAT_STACK_DEPTH][16] = {
	{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}
};
float *ms_cur = mat_stack[0];

void ms_push( void )
{
	float *stack_end = mat_stack[MAT_STACK_DEPTH];
	float *old = ms_cur;
	ms_cur += 16;
	ASSERT( ms_cur < stack_end );
	m_copy( ms_cur, old );
}

void ms_pop( void )
{
	ms_cur -= 16;
	ASSERT( ms_cur >= mat_stack[0] );
}

void ms_translate( float x, float y, float z )
{
	float m[16];
	m_translate( m, x, y, z );
	ms_postmult( m );
}

void ms_rotate( int axis, float angle )
{
	float m[16];
	m_rotate( m, axis, angle );
	ms_postmult( m );
}

void ms_scale( float x, float y, float z )
{
	float m[16];
	m_scale( m, x, y, z );
	ms_postmult( m );
}

void ms_premult( float m[16] )
{
	m_mult( ms_cur, m, ms_cur );
}

void ms_postmult( float m[16] )
{
	m_mult( ms_cur, ms_cur, m );
}

