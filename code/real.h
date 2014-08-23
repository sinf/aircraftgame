#ifndef _VEC_H
#define _VEC_H
#include "system.h"

#if 0
/* Serious precision problems. And code isn't any smaller */
typedef S16 Real;
typedef S32 DReal;
#else
#if 0
/* 64-bit division needs an external function: __divdi3() */
typedef S32 Real;
typedef S64 DReal;
#else
typedef S32 Real;
typedef S32 DReal;
#endif
#endif

#define REAL_FRACT_BITS 8
#define REAL_FRACT_MASK ((1<<REAL_FRACT_BITS)-1)
#define REAL_FRACT_PART( x ) ((x) & (1<<REAL_FRACT_BITS)-1)

#define REAL_VEC( x, y ) {REALF(x), REALF(y)} /* Converts 2 floats/ints -> Vec2 */
#define REALF(f) ((Real)((f) * (1<<REAL_FRACT_BITS))) /* Converts float -> Real */
#define REALI(i) ((i)<<REAL_FRACT_BITS) /* Converts int -> Real */

#define REALTOF(r) ( (float)(r) / (float)(1<<REAL_FRACT_BITS) ) /* Converts Real -> float */
#define REALTOI(r) ((r)>>REAL_FRACT_BITS) /* Converts Real -> int */

#define DREAL_MUL( a, b ) (((DReal)(a)*(DReal)(b)) >> REAL_FRACT_BITS) /* Multiplies 2 Real values */
#define DREAL_DIV( a, b ) ( ((DReal)(a)<<REAL_FRACT_BITS)/(b) ) /* Divides Real a by Real b */
#define REAL_MUL( a, b ) ((Real)DREAL_MUL(a,b))
#define REAL_DIV( a, b ) ((Real)DREAL_DIV(a,b))

typedef struct {
	Real x, y;
} Vec2;

Vec2 v_lerp( Vec2 a, Vec2 b, DReal t );
Vec2 v_addmul( Vec2 a, Vec2 b, DReal b_scale );
Vec2 v_sincos( Real a );
Vec2 v_rotate( Vec2 v, Vec2 sincos_a );
Vec2 v_sub( Vec2 a, Vec2 b ); /* a - b */
Real v_lensq( Vec2 v ); /* x² + y² */
Real v_length( Vec2 v ); /* sqrt( x² + y² ) */
Vec2 v_normalize( Vec2 v );

#endif
