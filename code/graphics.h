#ifndef MAT_IDENTITY

extern const float COMMON_MATRICES[2][16];
#define MAT_IDENTITY (COMMON_MATRICES[0])
#define MAT_PROJECTION (COMMON_MATRICES[1])

#define SCREEN_W 1300
#define SCREEN_H 900

void setup2D( void );
void render( void );

#endif
