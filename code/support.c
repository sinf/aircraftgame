#include "support.h"

#if 0

/* Fast random generator macros taken from
	http://www.cse.yorku.ca/~oz/marsaglia-rng.html
* credits to whoever made it */
#define _MWC_ZNEW(z) (z=36969*(z&65535)+(z>>16))
#define _MWC_WNEW(w) (w=18000*(w&65535)+(w>>16))
#define _MWC(z,w) ((_MWC_ZNEW(z)<<16)+_MWC_WNEW(w)) /* <- a macro is good for re-entrant stuff */

U32 MWC( void )
{
	static U32 z = 362436069;
	static U32 w = 521288629;
	return _MWC( z, w );
}
#endif

U64 prng_next( void )
{
	static U64 s[2] = {362436069, 521288629};
	U64 a = s[0], b = s[1];
	s[0] = b;
	a ^= a << 23;
	return ( s[1] = ( a ^ b ^ ( a >> 17 ) ^ ( b >> 26 ) ) ) + a;
}

#if 0
/* todo */

void init_clock( void )
{
}

U32 get_time_ms( void )
{
	return 0;
}

void sleep_ms( U32 n )
{
	(void) n;
}
#endif
