#include "support.h"

#if 1
U32 MWC( void )
{
	static U32 z = 362436069;
	static U32 w = 521288629;
	return _MWC( z, w );
}
#endif

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
