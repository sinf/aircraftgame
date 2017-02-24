//#pragma GCC diagnostic ignored "-Wpadded"
//#define __NDS__ /* __NDS__ required for SDL.h to work with the gcc flag "-fshort-enums" */
#include "SDL.h"
#include "system.h"
#include "support.h"
#include "game.h"
#include "models.h"
#include "graphics.h"
#include "real.h"
#include "draw.h"

#define get_time_ms SDL_GetTicks

static SDL_Renderer *sdl_r;
static SDL_Window *sdl_w;
static SDL_Texture *sdl_t;

#ifdef DEBUG
static void sdl_fail( const char *s ) {
	printf( "%s, %s\n", s, SDL_GetError() );
}
#else
#define sdl_fail(s)
#endif

static void create_window( int w, int h )
{
	int f = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

	if ( SDL_Init( SDL_INIT_VIDEO )
	|| SDL_CreateWindowAndRenderer( w, h, f,
		&sdl_w, &sdl_r ) ) {
		sdl_fail( "Failed to open a window" );
		EXIT();
	}

	r_resy = r_resx*h/w;
	r_resy = MIN( r_resy, MAX_RESY );
	
	#ifdef DEBUG
	printf( "Output resolution %dx%d\n", w, h );
	printf( "Render resolution %dx%d\n", r_resx, r_resy );
	#endif

	sdl_t = SDL_CreateTexture( sdl_r, SDL_PIXELFORMAT_ABGR8888,
		SDL_TEXTUREACCESS_STREAMING, r_resx, r_resy );
	
	if ( !sdl_t ) {
		sdl_fail( "Failed to create a texture" );
		EXIT();
	}
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
	
	keyboard = SDL_GetKeyboardState( NULL );
	
	#define K_ESCAPE	SDL_SCANCODE_ESCAPE
	#define K_LEFT	SDL_SCANCODE_LEFT
	#define K_RIGHT	SDL_SCANCODE_RIGHT
	#define K_UP	SDL_SCANCODE_UP
	#define K_DOWN	SDL_SCANCODE_DOWN
	#define K_X	SDL_SCANCODE_X
	#define K_Z	SDL_SCANCODE_Z
	
	#define KEY_DOWN(key) (keyboard[key])
	
	if ( KEY_DOWN( K_ESCAPE ) )
		return 0;
	
	if ( KEY_DOWN( K_Z ) )
		init_world();
	
	Thing *player = WORLD.player;
	if ( player ) {
		Real a = (player)->angle;
		if ( KEY_DOWN(K_LEFT) ) a -= REALF( AIRCRAFT_ROTATE_SPEED * W_TIMESTEP );
		if ( KEY_DOWN(K_RIGHT) ) a += REALF( AIRCRAFT_ROTATE_SPEED * W_TIMESTEP );
		(player)->angle = ( REALF(2*PI) + a ) % REALF( 2 * PI );
		(player)->data.ac.gun_trigger = KEY_DOWN( K_X );
		(player)->data.ac.throttle_on = KEY_DOWN( K_UP );
	}
	
	return 1;
}

static void Main0( void )
{
	U32 nap = 1000 / GAME_TICKS_PER_SEC;
	U32 next_frame;
	
	STATIC_ASSERT( sizeof(U32) == 4 );
	
	create_window( SCREEN_W, SCREEN_H );
	setup2D();
	init_world();
	
	next_frame = get_time_ms();

	for( ;; )
	{
		if ( get_time_ms() > next_frame )
		{
			if ( !process_events() )
				break;
			
			update_world();

			void *tmp;
			if ( ! SDL_LockTexture( sdl_t, NULL, &tmp, &r_pitch ) ) {
				r_canvas = tmp;
				render();
				SDL_UnlockTexture( sdl_t );
				SDL_RenderSetViewport( sdl_r, NULL );
				SDL_RenderCopy( sdl_r, sdl_t, NULL, NULL );
				SDL_RenderPresent( sdl_r );
			}
			
			next_frame += nap;
		}
	}
	
	SDL_Quit();
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
