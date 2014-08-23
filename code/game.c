#include "system.h"
#include "support.h"

#define _GAME_INTERNALS
#include "game.h"

#define ENABLE_GODMODE 1
#define ENABLE_ENEMY_AIRCRAFT 0

World WORLD = {0};

static Thing *add_thing( ThingType type, Vec2 pos, Thing *parent );
static void remove_thing( Thing *t );
static Thing *add_aircraft( void );
static Thing *add_gunship( void );
static Thing *add_battleship( void );
static void add_water_splash( Vec2 pos, Real vel_y );
static void add_smoke( Vec2 pos );
static void add_explosion( Vec2 pos, Real vel_x, Real vel_y );
static Real get_water_height( Real x );

/* ***************************************************************** */
/* ***************************************************************** */

Vec2 sin_cos_add_mul( float t, Vec2 v, float m )
{
	float fx, fy;
	sincosf( t, &fy, &fx );
	v.x += REALF( fx * m );
	v.y += REALF( fy * m );
	return v;
}

void init_world( void )
{
	WORLD.enemy_spawn_timer = 0;
	WORLD.num_things = 0;
	WORLD.player = add_aircraft();
	
	#if 0
	add_aircraft();
	add_gunship();
	add_battleship();
	#endif
	
	WORLD.water.z = WORLD.water.buffers[0];
	WORLD.water.old_z = WORLD.water.buffers[1];
	WORLD.water.temp = WORLD.water.buffers[2];
}

static void *list_append( void *list_base, unsigned *num_items, unsigned item_size )
{
	/* Warning: check for maximum item limit before calling this function! */
	char *p = list_base;
	p += *num_items * item_size;
	*num_items += 1;
	mem_clear( p, 0, item_size );
	return p;
}

static void list_remove( void *list_base, unsigned *num_items, unsigned item_size, void *item_p )
{
	char *c_list_base = list_base;
	char *c_item_p = item_p;
	
	U32 a = (U32) ( c_item_p - c_list_base ) / item_size;
	U32 b = *num_items - 1;
	
	if ( a != b )
		mem_copy( c_list_base + item_size * a, c_list_base + item_size * b, item_size );
	
	*num_items = b;
}

static void remove_thing( Thing *t )
{
	if ( t == WORLD.player )
		WORLD.player = NULL;
	
	list_remove( WORLD.things, &WORLD.num_things, sizeof(Thing), t );
}

static Thing *add_thing( ThingType type, Vec2 pos, Thing *parent )
{
	static ThingID next_thing_id = 0;
	Thing *thing;
	Real hp = REALF( MAX_THING_HP );
	
	if ( WORLD.num_things >= MAX_THINGS )
		return NULL;
	
	thing = list_append( WORLD.things, &WORLD.num_things, sizeof(Thing) );
	
	if ( type == T_BATTLESHIP )
		hp <<= 1;
	
	thing->type = type;
	thing->hp = hp;
	thing->id = ++next_thing_id;
	thing->pos = pos;
	
	if ( parent )
	{
		thing->parent = parent;
		thing->parent_id = parent->id;
		thing->rel_pos = pos;
	}
	
	return thing;
}

static void add_particle( Vec2 pos, U8 rs_vel_x, U8 rs_vel_y, ParticleType type )
{
	Thing *t = add_thing( T_PARTICLE, pos, NULL );
	if ( t )
	{
		Vec2 vel;
		
		S32 m = prng_next();
		S16 m1 = m;
		S16 m2 = m >> 16;
		
		vel.x = m1 >> rs_vel_x;
		vel.y = m2 >> rs_vel_y;
		
		if ( type == PT_WATER1 )
		{
			/* Make sure the small water drops launch upwards */
			vel.y -= 0x7FF;
		}
		
		t->vel = vel;
		t->data.pt.type = type;
	}
}

static void add_water_splash( Vec2 pos, Real vel_y )
{
	int n;
	Real wh = get_water_height( pos.x );
	
	if ( abs( pos.y - wh ) < REALF( 1.0 ) && abs( vel_y ) > REALF( 4 ) ) {
		for( n=0; n<10; n++ )
		{
			/* Add small drops (10) */
			add_particle( pos, 8, 4, PT_WATER1 );
		
			if ( n & 1 )
			{
				/* Add 'fog' (5) */
				add_particle( pos, 6, 16, PT_WATER2 );
			}
		}
	}
	
	n = pos.x / REALF( WATER_ELEM_SPACING );
	n %= WATER_RESOL;
	WORLD.water.z[n] += REAL_MUL( vel_y, REALF( WATER_SPLASH_FORCE ) );
}

static void add_smoke( Vec2 pos )
{
	add_particle( pos, 8, 8, PT_SMOKE );
}

static void add_explosion( Vec2 pos, Real vel_x, Real vel_y )
{
	int n;
	for( n=0; n<5; n++ )
	{
		Thing *t;
		t = add_thing( T_DEBRIS, pos, NULL );
		
		if ( t )
		{
			t->vel.x = vel_x - ( prng_next() & 0xAAAA & 0x1FFF ) + 1024;
			t->vel.y = vel_y - ( ~prng_next() & 0xAAAA & 0x1FFF );
		}
	}
}

static Thing *add_aircraft( void )
{
	Vec2 zero = REAL_VEC( 0, 0 );
	Thing *t = add_thing( T_AIRCRAFT, zero, NULL );
	
	if ( t )
	{
		t->angle = REALF( -PI/2 );
		t->data.ac.num_bombs = 10;
	}
	
	return t;
}

static Thing *add_gunship( void )
{
	Vec2 gun_pos = REAL_VEC( 1.2, -0.85 );
	Real ship_vel = REALF( 0.2 );
	Vec2 ship_pos = REAL_VEC( 0, W_WATER_LEVEL );
	
	Thing *gs = add_thing( T_GUNSHIP, ship_pos, NULL );
	if ( gs )
	{
		gs->vel.x = ship_vel; /* Set the ship in slow horizontal motion */
		add_thing( T_AAGUN, gun_pos, gs );
	}
	
	return gs;
}

static Thing *add_battleship( void )
{
	Vec2 gun1_offset = REAL_VEC( -3.9, -1.45 );
	Vec2 gun2_offset = REAL_VEC( 3.4, -1.45 );
	Vec2 radar_offset = REAL_VEC( -0.3, -3.5 );
	Vec2 ship_pos = REAL_VEC( -25, W_WATER_LEVEL );
	Real ship_vel = REALF( 0.15 );
	
	Thing *bs = add_thing( T_BATTLESHIP, ship_pos, NULL );
	if ( bs )
	{
		bs->vel.x = ship_vel;
		
		add_thing( T_AAGUN, gun1_offset, bs );
		add_thing( T_AAGUN, gun2_offset, bs );
		add_thing( T_RADAR, radar_offset, bs );
	}
	
	return bs;
}

static void do_physics_step( Thing *t )
{
	/* The equation for "Time Corrected Verlet" (TCV) would be:
		Xi+1 = Xi + (Xi - Xi-1) * (DTi / DTi-1) + A * DTi * DTi
	The shitty Euler integration needs less code however:
		x = x0 + v * t + 1/2 * a * t * t
	*/
	
	Real *pos = (Real*) &t->pos;
	Real *vel = (Real*) &t->vel;
	Real *accel = (Real*) &t->accel;
	DReal dt = REALF( W_TIMESTEP );
	int n;
	
	for( n=0; n<2; n++ )
	{
		DReal new_vel = vel[n] + DREAL_MUL( accel[n], dt );
		DReal new_pos = pos[n] + DREAL_MUL( new_vel, dt );
		
		vel[n] = new_vel;
		pos[n] = new_pos;
	}
}

static Vec2 adjust_vector_length( Vec2 v, Real max, int nop_if_less_than )
{
	float k, len = sqrtf( REAL_MUL( v.x, v.x ) + REAL_MUL( v.y, v.y ) );
	if ( nop_if_less_than && len < max )
		return v;
	k = len / max;
	v.x = v.x / k;
	v.y = v.y / k;
	return v;
}

static int test_collision( Vec2 *a, Vec2 *b, Real r )
{
	Real dx = a->x - b->x;
	Real dy = a->y - b->y;
	U32 d2 = (U32)( dx * dx ) + (U32)( dy * dy );
	U32 r2 = r * r;
	return ( d2 <= r2 );
}

static void collide_projectile( Thing *proj )
{
	unsigned n;
	for( n=0; n<MAX_THINGS; n++ )
	{
		Thing *thing = WORLD.things + n;
		Real r = REALF( 1 );
		
		r <<= ( thing->type == T_GUNSHIP || thing->type == T_BATTLESHIP ) << 1;
		
		if ( thing->type < T_PROJECTILE
			&& ( proj->data.pr.owned_by_player != ( thing == WORLD.player ) )
			&& test_collision( &proj->pos, &thing->pos, r ) )
		{
			proj->hp = 0;
			thing->hp -= REALF( PROJECTILE_DAMAGE );
			add_smoke( proj->pos );
			break;
		}
	}
}

#if 0
extern void test_audio( void );
#else
static void test_audio( void ) {}
#endif

static void shoot_projectile( Thing *t, float angle )
{
	Thing *p = add_thing( T_PROJECTILE, t->pos, NULL );
	if ( p )
	{
		p->vel = sin_cos_add_mul( angle, t->vel, PROJECTILE_VEL );
		p->data.pr.owned_by_player = ( t == WORLD.player );
		/* p->data.pr.owner = t->id; */
		
		if ( t == WORLD.player )
			test_audio();
	}
}

static void update_aircraft( Thing *t )
{
	Aircraft *ac = &t->data.ac;
	float fangle = REALTOF( t->angle );
	int systems_online = 1;
	
	/* Cripple the aircraft when underwater */
	systems_online = ( t->underwater_time <= 0 );
	
	/* Limit velocity (air drag like behaviour). Makes the aircraft easier to control */
	t->vel = adjust_vector_length( t->vel, REALF(MAX_AIRCRAFT_VEL), 1 );
	
	if ( systems_online )
	{
		/* Calculate acceleration */
		if ( ac->throttle_on )
			t->accel = sin_cos_add_mul( fangle, t->accel, AIRCRAFT_ENGINE_POWER );
	}
	
	t->data.ac.gun_timer -= REALF( W_TIMESTEP );
	if ( t->data.ac.gun_timer <= REALF(0) )
	{
		t->data.ac.gun_timer = 0;
		
		if ( t->data.ac.gun_trigger )
		{
			shoot_projectile( t, fangle );
			t->data.ac.gun_timer = REALF( AIRCRAFT_GUN_TIMER );
		}
	}
	
	if ( t->pos.y < REALF(MAX_AIRCRAFT_ALTITUDE) )
	{
		/* TODO: limit aircraft height in a more "natural" way
		(the engine should stall above a certain height) */
		t->pos.y = REALF(MAX_AIRCRAFT_ALTITUDE);
		t->vel.y = abs( t->vel.y );
	}
}

static void slow_rotate( Real *angle, Vec2 const *self, Vec2 const *target, Real max_rot )
{
	Real my_angle, dst_angle, delta;
	Real dx, dy;
	Real pi = REALF( PI );
	Real pi2 = REALF( PI * 2 );
	
	dx = ( target->x - self->x );
	dy = ( target->y - self->y );
	
	if ( abs(dx) > REALF(W_WIDTH / 2) )
		dx = -dx;
	
	dst_angle = REALF( atan2f( dy, dx ) ) + pi;
	my_angle = *angle;
	
	delta = (( dst_angle - my_angle + pi2 ) % pi2 ) - pi;
	delta = CLAMP( delta, -max_rot, max_rot );
	*angle = ( *angle + delta + pi2 ) % pi2;
	
	/*
	delta = (( dst_angle - my_angle + pi2 ) % pi2 ) - pi;
	delta = MIN( delta, max_rot+pi );
	delta -= pi;
	*angle = ( *angle + delta + pi2 ) % pi2;
	*/
	
	/*
	delta = (( dst_angle - my_angle + pi2 ) % pi2 ) - pi;
	delta = CLAMP( delta, -max_rot, max_rot );
	delta -= pi;
	*angle = ( *angle + delta + pi2 ) % pi2;
	*/
}

static void slow_rotate_thing( Thing *self, Vec2 *target, Real max_rot )
{
	slow_rotate( &self->angle, &self->pos, target, max_rot );
}

static void do_enemy_aircraft_logic( Thing *self )
{
	/* Just go forward by default */
	Vec2 target = self->pos;
	target.x += self->vel.x;
	target.y += self->vel.y;
	
	if ( WORLD.player )
	{
		/* Follow the player */
		target = WORLD.player->pos;
	}
	
	#if 1
	/* Prevent diving into the ocean (extra intelligency; could do without) */
	target.y = MIN( target.y, REALF( W_WATER_LEVEL - 3 ) );
	#endif
	
	slow_rotate_thing( self, &target, REALF( AIRCRAFT_ROTATE_SPEED * W_TIMESTEP ) );
	
	self->data.ac.throttle_on = 1;
	self->data.ac.gun_trigger = ( WORLD.player != NULL ) && !( prng_next() & 0xFF );
}

#if !ENABLE_ENEMY_AIRCRAFT
static Thing *get_null( void ) { return NULL; }
#endif

static void spawn_enemies( void )
{
	typedef Thing *(*SpawnFunc)( void );
	
	SpawnFunc spawn_funcs[] = {
		#if ENABLE_ENEMY_AIRCRAFT
		add_aircraft,
		add_aircraft,
		#else
		get_null,
		get_null,
		#endif
		add_gunship,
		add_battleship
	};
	
	int n = prng_next() & 0x7;
	while( n-- >= 0 )
	{
		Real x = prng_next();
		Thing *t;
		
		if ( WORLD.player )
		{
			/* Attempt to place enemies outside view (to prevent ugly pop-ins) */
			x %= REALF( W_WIDTH / 4 );
			x += REALF( W_WIDTH / 2 );
			x += WORLD.player->pos.x;
		}
		
		t = spawn_funcs[ prng_next() & 0x3 ]();
		if ( t )
			t->pos.x = x;
	}
}

static void update_water( Water w[1] )
{
	Real *old_z, *cur_z, *new_z;
	Real left, mid, right;
	double c0 = WATER_T * W_TIMESTEP / WATER_ELEM_SPACING;
	Real c = REALF( c0 * c0 );
	unsigned n;
	
	old_z = w->old_z;
	cur_z = w->z;
	new_z = w->temp;
	
	w->old_z = cur_z;
	w->z = new_z;
	w->temp = old_z;
	
	mid = cur_z[WATER_RESOL-1];
	right = cur_z[0];
	
	for( n=0; n<WATER_RESOL; n++ )
	{
		left = mid;
		mid = right;
		right = cur_z[(1+n)%WATER_RESOL];
		
		new_z[n] = 2*mid - old_z[n] + REAL_MUL( c, left + right - 2*mid );
		new_z[n] = REAL_MUL( REALF( WATER_DAMPING_FACTOR ), new_z[n] );
		
		/*
		new_z[n] = 2 * mid - old_z[n] + REAL_MUL( WATER_T, ( left + right - 2 * mid ) / 2 );
		*/
	}
}

static Real get_water_height( Real x )
{
	Water *w = &WORLD.water;
	Real h0, h1, t;
	int cell;
	
	cell = REALTOI( x );
	cell = ( cell + WATER_RESOL ) % WATER_RESOL;
	h0 = w->z[cell];
	h1 = w->z[cell];
	
	t = REAL_FRACT_PART( x );
	return REALF( W_WATER_LEVEL ) + h0 + REAL_MUL( t, h1 - h0 );
}

/* This function should handle everything that depends on time */
void update_world( void )
{
	unsigned n;
	
	update_water( &WORLD.water );
	for( n=0; n<WORLD.num_things; n++ )
	{
		Thing *t = WORLD.things + n;
		Real water_h;
		Vec2 orig_pos = t->pos;
		
		/* Reset acceleration */
		t->accel.x = 0;
		t->accel.y = REALF( W_GRAVITY );
		
		t->age += REALF( W_TIMESTEP );
		
		water_h = get_water_height( t->pos.x );
		
		if ( t->pos.y > water_h )
		{
			t->underwater_time += REALF( W_TIMESTEP );
			t->accel.y = REALF( W_WATER_BUOANCY );
			
			if ( t->type != T_PARTICLE )
			{
				if ( t->pos.y > water_h + REALF( W_WATER_DEATH_LEVEL ) )
					t->hp = 0;
				
				add_water_splash( t->pos, t->vel.y );
			}
			else {
				t->pos.y = water_h;
			}
		}
		else
		{
			if ( t->old_pos.y > water_h ) {
				if ( t->type != T_PARTICLE ) {
					/* Add water splash when an object gets out of the water */
					add_water_splash( t->pos, 0 );
				}
			}
			
			t->underwater_time = 0;
		}
		
		switch( t->type )
		{
			/* Animate motion here */
			
			case T_AIRCRAFT:
				if ( t != WORLD.player )
					do_enemy_aircraft_logic( t );
				update_aircraft( t );
				break;
			
			case T_GUNSHIP:
			case T_BATTLESHIP:
				/* Prevent ships from sinking */
				/*t->pos.y = REALF( W_WATER_LEVEL );*/
				
				t->pos.y = get_water_height( t->pos.x );
				
				t->vel.y = 0;
				t->accel.y = 0;
				break;
			
			case T_AAGUN:
				if ( WORLD.player )
				{
					/* Aim the gun at player */
					Real r = REALF( W_TIMESTEP * AAGUN_ROTATE_SPEED );
					slow_rotate_thing( t, &WORLD.player->pos, r );
					
					/* Shoot all the time */
					t->data.aa.gun_timer += REALF( W_TIMESTEP );
					if ( t->data.aa.gun_timer > REALF(AAGUN_GUN_TIMER) )
					{
						shoot_projectile( t, REALTOF(t->angle) );
						t->data.aa.gun_timer = 0;
					}
				}
				break;
			
			case T_PROJECTILE:
				if ( t->underwater_time )
					t->hp = 0;
				collide_projectile( t );
				break;
			
			case T_PARTICLE:
				if ( t->age >= REALF( MAX_PARTICLE_TIME ) )
					t->hp = 0; /* Die of old age */
				break;
			
			case T_DEBRIS:
				if ( t->age > REALF( 7.0 ) )
					t->hp = 0;
				if ( ( prng_next() & 0x3 ) == 0 )
				{
					if ( !t->underwater_time )
						add_smoke( t->pos );
				}
				
				t->angle += REALF( 5 * W_TIMESTEP );
				break;
			
			default:
				break;
		}
		
		if ( t->parent )
		{
			if ( t->parent_id != t->parent->id )
			{
				/* Parent has died. Just die */
				t->hp = 0;
			}
			else
			{
				/* Follow parent */
				t->pos = t->parent->pos;
				t->pos.x += t->rel_pos.x;
				t->pos.y += t->rel_pos.y;
				t->vel = t->parent->vel;
				t->accel = t->parent->accel;
			}
		}
		else
		{
			/* Apply physics to everything */
			do_physics_step( t );
		}
		
		/* Wrap around world edges */
		t->pos.x = ( REALF( W_WIDTH ) + t->pos.x ) % REALF( W_WIDTH );
		
		#if ENABLE_GODMODE
		if ( WORLD.player )
			WORLD.player->hp = REALF( 100 );
		#endif
		
		t->old_pos = orig_pos;
		
		/* Kill things with no HP remaining */
		if ( t->hp <= 0 )
		{
			if ( t->type < T_PROJECTILE )
			{
				/* Throw pieces of debris */
				add_explosion( t->pos, t->vel.x, t->vel.y );
				
				if ( t->type == T_GUNSHIP || t->type == T_BATTLESHIP )
				{
					/* Exploding ships cause a large water splash: */
					int k;
					for( k=0; k<5; k++ )
					{
						Vec2 pos;
						pos.x = t->pos.x - 1024 + ( prng_next() & 0x7FF );
						pos.y = get_water_height( t->pos.x );
						add_water_splash( pos, WATER_SPLASH_FORCE_EXPL );
					}
				}
			}
			
			remove_thing( t );
			
			if ( n > 0 )
			{
				/* remove_thing() has overwritten this Thing with some other Thing.
				Must process the "same" Thing again. */
				n--;
			}
		}
	}
	
	if ( --WORLD.enemy_spawn_timer <= 0 )
	{
		WORLD.enemy_spawn_timer = ( ENEMY_SPAWN_INTERVAL * GAME_TICKS_PER_SEC );
		spawn_enemies();
	}
}
