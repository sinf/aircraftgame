#include "system.h"
#include "support.h"

#include "game.h"
#include "graphics.h"
#include "models.h"
#include "opengl.h"

/* You can see this many game units horizontally on the screen */
#define HORZ_VISION_RANGE 110.0f

#define ENABLE_BLEND 1
#define ENABLE_WIREFRAME 0
#define ENABLE_GRID ENABLE_WIREFRAME

static const float MATRIX_DATA[2*16] = {
	#define MAT_IDENTITY MATRIX_DATA
	#define MAT_PROJECTION (MATRIX_DATA+16)
	
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
	
	/* The projection matrix is pre-scaled so that the Real type can be passed to GL as-is */
	#define COORD_SCALE ( 1.0f / (1<<REAL_FRACT_BITS) )
	
	#define P_SCREEN_RATIO ((float)SCREEN_H/(float)SCREEN_W)
	
	#define P_RIGHT (HORZ_VISION_RANGE/2.0)
	#define P_LEFT (-P_RIGHT)
	#define P_BOTTOM ( P_SCREEN_RATIO * HORZ_VISION_RANGE / 2.0 )
	#define P_TOP (-P_BOTTOM)
	#define P_NEAR -128.0
	#define P_FAR 128.0
	
	/* Le orthographic projection matrix: */
	2.0 / ( P_RIGHT - P_LEFT ) * COORD_SCALE, 0, 0, 0,
	0, 2.0 / ( P_TOP - P_BOTTOM ) * COORD_SCALE, 0, 0,
	0, 0, -1.0 / ( P_FAR - P_NEAR ) * COORD_SCALE, 0,
	-( P_RIGHT + P_LEFT ) / ( P_RIGHT - P_LEFT ), -( P_TOP + P_BOTTOM ) / ( P_TOP - P_BOTTOM ), -P_NEAR / ( P_FAR - P_NEAR ), 1
};

static void draw_bg_rect( Real tx, Real ty, Real top, Real bottom, U32 color )
{
	GLint quad[4*2];
	
	quad[0] = quad[6] = tx; /* left */
	quad[2] = quad[4] = tx + REALF( W_WIDTH ); /* right */
	quad[1] = quad[3] = ty + top; /* top */
	quad[5] = quad[7] = ty + bottom; /* bottom */
	
	set_color_p( (void*) &color );
	set_vertex_pointer( quad, 2, VT_INT );
	draw_quads( 1 );
}

void setup2D( void )
{
	glViewport( 0, 0, SCREEN_W, SCREEN_H );
	
	glMatrixMode( GL_PROJECTION );
	glLoadMatrixf( MAT_PROJECTION );
	glMatrixMode( GL_MODELVIEW );
	/* glLoadMatrixf( MAT_IDENTITY ); */
	
	glEnableClientState( GL_VERTEX_ARRAY );
	
	#if ENABLE_BLEND
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	#endif
	
	#if ENABLE_WIREFRAME
	glLineWidth( 2 );
	#endif
	
	unpack_models();
}

static void draw_blob( Real x, Real y, Real width, Real height, U32 color )
{
	#define N_BLOB_VERTS 9
	float verts[N_BLOB_VERTS*2];
	float a = 0.0f;
	float scale_x = width >> 1;
	float scale_y = height >> 1;
	int n;
	
	for( n=0; n<N_BLOB_VERTS; n++ )
	{
		float *v = verts + 2 * n;
		float xx, yy;
		sincosf( a, &xx, &yy );
		v[0] = x + xx * scale_x;
		v[1] = y + yy * scale_y;
		a += 2 * PI / N_BLOB_VERTS;
	}
	
	set_color_p( (void*) &color );
	set_vertex_pointer( verts, 2, VT_FLOAT );
	glDrawArrays( GL_TRIANGLE_FAN, 0, N_BLOB_VERTS );
}

static void draw_particle( Real x, Real y, Thing *thing )
{
	static const Real PARTICLE_SIZE[] = {
		REALF(0.5), REALF(0.5), /* water1 */
		REALF(2), REALF(1), /* water2*/
		REALF(1), REALF(1), /* smoke (size should be random) */
	};
	
	ParticleType type = thing->data.pt.type;
	U8 g, a;
	Real w, h;
	
	/* 8-bit grayscale values for particles:
		0xf4 ... water1
		0xf4 ... water2
		0x32 ... smoke */
	g = 0x32f4f4 >> ( type << 3 );
	
	#if ENABLE_BLEND
	/* Fade out */
	a = 0xFF - ( thing->age << 8 ) / ( REALF( MAX_PARTICLE_TIME ) + 1 );
	#else
	a = g;
	#endif
	
	w = PARTICLE_SIZE[type << 1];
	h = PARTICLE_SIZE[(type << 1) + 1];
	
	draw_blob( x, y, w, h, RGBA_32(g,g,g,a) );
}

#if ENABLE_WIREFRAME
static void draw_crosshair( Real x, Real y )
{
	#define CROSSHAIR_SIZE 1.5
	#define R ( CROSSHAIR_SIZE / 2.0 )
	
	S32 verts[2*2*2];
	Real r = REALF( 1.5 / 2 );
	
	verts[0] = x - r;
	verts[1] = y;
	verts[2] = x + r;
	verts[3] = y;
	
	verts[4] = x;
	verts[5] = y - r;
	verts[6] = x;
	verts[7] = y + r;
	
	set_color( 255, 0, 25, 128 );
	set_vertex_pointer( verts, 2, VT_INT );
	glDrawArrays( GL_LINES, 0, 4 );
}
#endif

#if !ENABLE_GRID
#define draw_grid(x,y)
#else
static void draw_grid( Real off_x, Real off_y )
{
	#define N_GRID_LINES_X ((int)W_WIDTH)
	#define N_GRID_LINES_Y ((int)W_HEIGHT)
	#define N_GRID_VERTS (N_GRID_LINES_X*2+N_GRID_LINES_Y*2)
	
	S32 verts[N_GRID_VERTS*2];
	S32 *v = verts;
	int n;
	
	off_y -= REALF( W_HEIGHT / 2 );
	
	for( n=0; n<N_GRID_LINES_X; n++ )
	{
		v[1] = off_y;
		v[3] = off_y + REALI( N_GRID_LINES_Y );
		v[0] = v[2] = off_x + REALI( n );
		v += 4;
	}
	
	for( n=0; n<N_GRID_LINES_Y; n++ )
	{
		v[0] = off_x;
		v[2] = off_x + REALI( N_GRID_LINES_X );
		v[1] = v[3] = off_y + REALI( n );
		v += 4;
	}
	
	glColor4ub( 255, 255, 255, 32 );
	glVertexPointer( 2, GL_INT, 0, verts );
	glDrawArrays( GL_LINES, 0, N_GRID_VERTS );
	
	glLineWidth( 4 );
	verts[0] = off_x;
	verts[1] = off_y;
	verts[2] = off_x;
	verts[3] = off_y + REALF( W_HEIGHT );
	glColor4ub( 255, 0, 0, 32 );
	glDrawArrays( GL_LINES, 0, 2 );
	glLineWidth( 2 );
}
#endif

static void draw_water( const Water w[1], S32 eye_x, S32 eye_y )
{
	S32 verts[(WATER_RESOL+1)*2*2];
	S32 offset_y = eye_y + REALF( W_WATER_LEVEL );
	S32 bottom = offset_y + REALF( 50.0 );
	unsigned n;
	S32 pos_x = eye_x;
	
	for( n=0; n<WATER_RESOL; n++ ) {
		unsigned n4 = n * 4;
		verts[n4] = pos_x;
		verts[n4+1] = w->z[n] + offset_y;
		verts[n4+2] = pos_x;
		verts[n4+3] = bottom;
		pos_x += REALF( WATER_ELEM_SPACING );
	}
	
	verts[4*WATER_RESOL+0] = pos_x;
	verts[4*WATER_RESOL+1] = verts[1];
	verts[4*WATER_RESOL+2] = pos_x;
	verts[4*WATER_RESOL+3] = bottom;
	
	glColor4ub( 0, 0, 255, 64 );
	glVertexPointer( 2, GL_INT, 0, verts );
	glDrawArrays( GL_TRIANGLE_STRIP, 0, 2*WATER_RESOL+2 );
}

void render_world( int pass, Real eye_x_offset )
{
	unsigned n;
	static Real eye_x = REALF( 0 );
	static Real eye_y = REALF( 0 );
	
	#if 1
	if ( WORLD.player )
	{
		eye_x = -WORLD.player->pos.x;
		eye_y = -WORLD.player->pos.y;
	}
	#endif
	
	eye_x += eye_x_offset;
	
	if ( pass == 0 )
	{
		/* Draw sky (also clears the framebuffer) */
		
		U32 sky_color = RGB_32( 0xd8, 0xd8, 0xac );
		
		draw_bg_rect(
			eye_x, eye_y,
			REALF( W_WATER_LEVEL - W_HEIGHT - 64 ),
			REALF( W_WATER_LEVEL + 64 ),
			sky_color );
		
		return;
	}
	
	#if ENABLE_WIREFRAME
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	#endif
	
	for( n=0; n<WORLD.num_things; n++ )
	{
		Thing *t = WORLD.things + n;
		Real x = t->pos.x;
		Real y = t->pos.y;
		Real yaw = t->angle;
		Real roll = 0;
		ModelID mdl = BAD_MODEL_ID;
		
		x += eye_x;
		y += eye_y;
		
		switch( t->type )
		{
			case T_AIRCRAFT:
				roll = yaw + REALF( PI );
				mdl = M_AIRCRAFT;
				break;
			
			case T_GUNSHIP:
				mdl = M_GUNSHIP;
				break;
			
			case T_AAGUN:
				draw_blob( x, y, REALF( 1.5 ), REALF( 1.5 ), RGBA_32(0,0,0,255) );
				mdl = M_AAGUN_BARREL;
				break;
			
			case T_RADAR:
				roll = t->age;
				yaw = REALF( -PI / 2 );
				mdl = M_RADAR;
				break;
			
			case T_BATTLESHIP:
				mdl = M_BATTLESHIP;
				break;
			
			case T_PROJECTILE:
				draw_blob( x, y, REALF(0.5), REALF(0.5), RGBA_32(0,0,0,255) );
				break;
			
			case T_PARTICLE:
				draw_particle( x, y, t );
				break;
			
			case T_DEBRIS:
				mdl = M_DEBRIS;
				roll = 2 * yaw + REALF( PI/3 );
				break;
			
			default:
				break;
		}
		
		if ( mdl != BAD_MODEL_ID )
		{
			draw_model( mdl, x, y, yaw, roll );
			
			if ( t->type == T_AIRCRAFT && t->data.ac.throttle_on && t->pos.y < REALF(W_WATER_LEVEL) )
				draw_model( M_AIRCRAFT_FLAME, 0, 0, 0, roll );
			
			glLoadMatrixf( MAT_IDENTITY );
		}
		
		#if ENABLE_WIREFRAME
		if ( t->type != T_PARTICLE && t->type != T_PROJECTILE )
			draw_crosshair( x, y );
		#endif
	}
	
	#if 1
	draw_water( &WORLD.water, eye_x, eye_y );
	#endif
	
	#if ENABLE_WIREFRAME
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	#endif
	
	#if 0
	/* Draw the sea */
	draw_bg_rect( eye_x, eye_y,
		REALF( W_WATER_LEVEL ),
		REALF( W_WATER_LEVEL+128 ),
		RGBA_32(  0, 0, 0x80, 0xAF ) );
	#endif
	
	#if ENABLE_GRID
	draw_grid( eye_x, eye_y );
	#endif
}

void render( void )
{
	int pass;
	for( pass=0; pass<2; pass++ )
	{
		render_world( pass, REALF( 0 ) );
		render_world( pass, REALF( W_WIDTH ) );
		render_world( pass, REALF( -W_WIDTH ) );
	}
	
	#if 0
	if ( WORLD.player )
	{
		int x = REALF( P_LEFT + 2 );
		int y = REALF( P_BOTTOM - 2 );
		int w = ( WORLD.player->hp );
		int h = REALF( 1 );
		
		glColor4ub( 255, 0, 0, 255 );
		glRecti( x, y, x+w, y+h );
	}
	#endif
}
