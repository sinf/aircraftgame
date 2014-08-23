#ifndef UNROLL32

#include "system.h"
#include "real.h"

#define RGBA_32( r, g, b, a ) ((r)|((g)<<8)|((b)<<16)|((a)<<24))
#define RGB_32( r, g, b ) RGBA_32( r, g, b, 0xFF )

/* A compile-time assert() */
#define STATIC_ASSERT( cond ) if (0) { int _STATIC_ASSERT[1-2*(!(cond))]; (void) _STATIC_ASSERT; }

#define UNROLL8(x) x,x,x,x,x,x,x,x
#define UNROLL16(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x
#define UNROLL32(x) UNROLL16(x),UNROLL16(x)

#define PI 3.1415927410125732421875f

#define RADIANS(deg) ((deg)*(PI/180.0f)) /* Converts degrees to radians */
#define DEGREES(rad) ((rad)*(180.0f/PI)) /* Converts radians to degrees */

#define MAX(a,b) ( (a) > (b) ? (a) : (b) )
#define MIN(a,b) ( (a) < (b) ? (a) : (b) )
#define CLAMP(x,min,max) MIN(max,MAX((x),min))

extern U64 prng_next( void ); /* <- using this function might reduce code size */

#endif

