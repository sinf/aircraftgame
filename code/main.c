#include "system.h"
#include "support.h"
#include "game.h"
#include "models.h"
#include "graphics.h"
#include "real.h"

#define PROCESS_INPUT( player ) \
if ( player != NULL ) { \
	Real a = (player)->angle; \
	if ( KEY_DOWN(K_LEFT) ) a -= REALF( AIRCRAFT_ROTATE_SPEED * W_TIMESTEP ); \
	if ( KEY_DOWN(K_RIGHT) ) a += REALF( AIRCRAFT_ROTATE_SPEED * W_TIMESTEP ); \
	(player)->angle = ( REALF(2*PI) + a ) % REALF( 2 * PI ); \
	(player)->data.ac.gun_trigger = KEY_DOWN( K_X ); \
	(player)->data.ac.throttle_on = KEY_DOWN( K_UP ); \
}

#if USE_SDL
#define init_clock()
#define get_time_ms SDL_GetTicks
#else
void init_clock( void ) {}
U32 get_time_ms( void ) { return 0; /* todo?? */ }
#endif

#if USE_SDL
#pragma GCC diagnostic ignored "-Wpadded"
#define __NDS__ /* __NDS__ required for SDL.h to work with the gcc flag "-fshort-enums" */
#include "SDL.h"
#define swap_buffers SDL_GL_SwapBuffers
static void create_window( int resx, int resy )
{
	if ( SDL_Init( SDL_INIT_VIDEO ) || !SDL_SetVideoMode( resx, resy, 0, SDL_OPENGL ) )
		EXIT();
}
static int process_events( void )
{
	const Uint8 *keyboard;
	
	#if 1
	SDL_Event e;
	while( SDL_PollEvent(&e) )
	{
		if ( e.type == SDL_QUIT )
			return 0;
	}
	#else
	SDL_PumpEvents();
	#endif
	
	keyboard = SDL_GetKeyState( NULL );
	
	#define K_ESCAPE	SDLK_ESCAPE
	#define K_LEFT	SDLK_LEFT
	#define K_RIGHT	SDLK_RIGHT
	#define K_UP	SDLK_UP
	#define K_DOWN	SDLK_DOWN
	#define K_X	SDLK_x
	#define K_Z	SDLK_z
	
	#define KEY_DOWN(key) (keyboard[key])
	
	if ( KEY_DOWN( K_ESCAPE ) )
		return 0;
	
	if ( KEY_DOWN( K_Z ) )
		init_world();
	
	PROCESS_INPUT( WORLD.player );
	
	return 1;
}
#else
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#define swap_buffers() glXSwapBuffers( disp, win )
static Display *disp;
static Window win;
static void create_window( int resx, int resy )
{
	GLint vis_attribs[] = { GLX_RGBA, GLX_DOUBLEBUFFER, None };
	Window root;
	XVisualInfo *vis;
	XSetWindowAttributes swat;
	GLXContext ctx;
	
	/* Should check disp, vis for NULL */
	
	disp = XOpenDisplay( NULL );
	root = DefaultRootWindow( disp );
	vis = glXChooseVisual( disp, 0, vis_attribs );
	swat.colormap = XCreateColormap( disp, root, vis->visual, AllocNone );
	win = XCreateWindow( disp, root, 0, 0, resx, resy, 0, vis->depth,
		InputOutput, vis->visual, CWColormap, &swat );
	
	XMapRaised( disp, win );
	ctx = glXCreateContext( disp, vis, 0, GL_TRUE );
	glXMakeCurrent( disp, win, ctx );
}
static int process_events( void )
{
	char xkeys[32];
	XQueryKeymap( disp, xkeys );
	
	/* X11 keycodes obtained from the 'xev' program */
	
	#define K_ESCAPE 9
	#define K_LEFT 113
	#define K_RIGHT 114
	#define K_UP 111
	#define K_DOWN 116
	#define K_Z 52
	#define K_X 53
	
	#define KEY_DOWN(code) ( xkeys[(code)>>3] & (1<<((code)&0x7)) )
	
	if ( KEY_DOWN( K_ESCAPE ) )
		return 0;
	
	if ( KEY_DOWN( K_Z ) )
		init_world();
	
	PROCESS_INPUT( WORLD.player );
	
	return 1;
}
#endif

static void Main0( void )
{
	U32 nap = 1000 / GAME_TICKS_PER_SEC;
	U32 next_frame;
	
	extern U64 prng_state[2];
	prng_state[0] = 0x7c1a2c2a7d8ff9f7;
	prng_state[1] = 0x2236e6667b6f36a5;
	
	/*
	static U64 s[2] = {362436069, 521288629};
	*/
	
	STATIC_ASSERT( sizeof(U32) == 4 );
	
	create_window( SCREEN_W, SCREEN_H );
	setup2D();
	init_world();
	
	next_frame = get_time_ms();
	for( ;; )
	{
		if( get_time_ms() > next_frame )
		{
			if ( !process_events() )
				EXIT();
			
			update_world();
			render();
			swap_buffers();
			
			next_frame += nap;
		}
	}
	
	/* SDL_Quit() ?
		- would restore the proper video mode;
		- not very useful unless using fullscreen */
	
	EXIT();
}


#ifdef MAIN
int main( void )
#else
__attribute__((externally_visible)) void Main( void )
#endif
{
	if ( sizeof(void*) == 8 ) {
		/* Align stack to 16-byte boundary. Required on x86-64 */
		__asm volatile ( "and $~0xF, %sp" );
	}
	
	Main0();
	#ifdef MAIN
	return 0;
	#endif
}
