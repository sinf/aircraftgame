#ifndef MAT_IDENTITY
#include "draw.h"

extern const float COMMON_MATRICES[2][16];
#define MAT_IDENTITY (COMMON_MATRICES[0])
#define MAT_PROJECTION (COMMON_MATRICES[1])

void setup2D( void );
void render( void );

#endif
