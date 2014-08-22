#ifndef _SUPPORT_H
#define _SUPPORT_H
#include "system.h"
#include "real.h"

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

/* Fast random generator macros taken from
	http://www.cse.yorku.ca/~oz/marsaglia-rng.html
* credits to whoever made it */
#define _MWC_ZNEW(z) (z=36969*(z&65535)+(z>>16))
#define _MWC_WNEW(w) (w=18000*(w&65535)+(w>>16))
#define _MWC(z,w) ((_MWC_ZNEW(z)<<16)+_MWC_WNEW(w)) /* <- a macro is good for re-entrant stuff */
extern U32 MWC( void ); /* <- using this function might reduce code size */

#endif
