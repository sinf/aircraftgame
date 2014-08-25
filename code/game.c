#include "system.h"
#include "support.h"

#define _GAME_INTERNALS
#include "game.h"

#define ENABLE_GODMODE 1
#define ENABLE_ENEMY_AIRCRAFT 1
#define DISARM_ENEMIES 0

World WORLD = {0};

static Thing *add_thing( ThingType type, Vec2 pos, Thing *parent );
static void remove_thing( Thing *t );
static Thing *add_aircraft( void );
static Thing *add_gunship( void );
static Thing *add_battleship( void );
static void add_smoke( Vec2 pos );
static void add_explosion( Vec2 pos, Real vel_x, Real vel_y );

static Real get_water_height( Real x );
static void displace_water( Real x, Real delta );

/* ***************************************************************** */
/* ***************************************************************** */

Vec2 sin_cos_add_mul( float t, Vec2 v, float m )
{
	v.x += REALF( cosf( t ) * m );
	v.y += REALF( sinf( t ) * m );
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
	thing->buoancy = REALF( 5.0f );
	thing->mass = 20;
	
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

static void add_smoke( Vec2 pos )
{
	add_particle( pos, 8, 8, PT_SMOKE );
}

/*
r = Falloff radius; f(r) = 0
a = -c/r²
c = Maximum value of the function; located at f(0)

The falloff function:
f(x) = ax² + c
*/

/* Might have to be fixed to take W_TIMESTEP into account */
static Real calc_explos_water_shock( Real ey )
{
	float y = REALTOF( REALF( W_WATER_LEVEL ) - ey );
	float r = 10.0; /* falloff radius */
	float c = 2.5; /* maximum force */
	float a = c / -(r*r);
	return REALF( MAX( 0, a*y*y + c ) );
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
			t->vel.x = vel_x - ( prng_next() & 0x7FF ) + 1024;
			t->vel.y = vel_y - ( prng_next() & 0xFFF );
		}
	}
	
	displace_water( pos.x, calc_explos_water_shock( pos.y ) );
}

/* $add_aircraft */
static Thing *add_aircraft( void )
{
	Vec2 zero = REAL_VEC( 0, 0 );
	Thing *t = add_thing( T_AIRCRAFT, zero, NULL );
	
	if ( t )
	{
		t->angle = REALF( -PI/2 );
		t->data.ac.num_bombs = 10;
		t->mass = 50;
		t->buoancy = REALF( AIRCRAFT_BUOANCY );
		
		/*t->data.ac.is_heli = 1;*/
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
		gs->buoancy = REALF( 25.0f );
		/* gs->mass = 100; */
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
		bs->buoancy = REALF( 25.0f );
		/* bs->mass = 200; */
		
		add_thing( T_AAGUN, gun1_offset, bs )->mass = 1;
		add_thing( T_AAGUN, gun2_offset, bs )->mass = 1;
		add_thing( T_RADAR, radar_offset, bs )->mass = 1;
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
	
	const Vec2 accel = t->accel;
	Vec2 pos = t->pos, vel = t->vel;
	DReal dt = REALF( W_TIMESTEP );
	
	t->old_pos = pos;
	t->vel = vel = v_addmul( vel, accel, dt );
	t->pos = v_addmul( pos, vel, dt );
}

/* only used by update_aircraft currently. should remove this */
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

static int collide_projectile_1( Thing *proj, Thing *victim )
{
	Real r = REALF( 1 );
	r <<= ( victim->type == T_GUNSHIP || victim->type == T_BATTLESHIP ) << 1;
	
	if ( victim->type < T_PROJECTILE && test_collision( &proj->pos, &victim->pos, r ) )
	{
		proj->hp = 0;
		victim->hp -= REALF( PROJECTILE_DAMAGE );
		add_smoke( proj->pos );
		return 1;
	}
	
	return 0;
}

static void collide_projectile( Thing *proj )
{
	if ( proj->data.pr.dmg == DAMAGES_PLAYER ) {
		if ( WORLD.player )
			collide_projectile_1( proj, WORLD.player );
	} else {
		/* Note: The projectile is collision-tested against itself but collide_projectile_1() will reject because the victim is a projectile as well */
		unsigned n;
		for( n=0; n<MAX_THINGS; n++ ) {
			Thing *victim = WORLD.things + n;
			if ( victim != WORLD.player && collide_projectile_1( proj, victim ) )
				break;
		}
	}
}

static void shoot_projectile( Thing *t, float angle )
{
	Thing *p;
	
	#if DISARM_ENEMIES
	if ( t != WORLD.player )
		return;
	#endif
	
	p = add_thing( T_PROJECTILE, t->pos, NULL );
	if ( p )
	{
		p->vel = sin_cos_add_mul( angle, t->vel, PROJECTILE_VEL );
		p->data.pr.dmg = ( t == WORLD.player ) ? DAMAGES_ENEMIES : DAMAGES_PLAYER;
		
		p->buoancy = REALF( PROJECTILE_BUOANCY );
		p->mass = PROJECTILE_MASS;
		
		#if 0
		/* Rock ships as they fire */
		if ( t->type == T_AAGUN )
			displace_water( t->pos.x, REALF( 0.1 ) );
		#endif
	}
}

static void update_aircraft( Thing *t )
{
	Aircraft *ac = &t->data.ac;
	float fangle = REALTOF( t->angle );
	int systems_online = 1;
	
	/* Cripple the aircraft when underwater */
	systems_online = ( t->underwater_time <= 0 );
	
	systems_online = 1;
	
	/* Limit velocity (air drag like behaviour). Makes the aircraft easier to control */
	t->vel = adjust_vector_length( t->vel, REALF(MAX_AIRCRAFT_VEL), 1 );
	
	if ( systems_online )
	{
		/* Calculate acceleration */
		if ( ac->throttle_on ) {
			float power = ac->is_heli ? AIRCRAFT_ENGINE_POWER*0.25f : AIRCRAFT_ENGINE_POWER;
			t->accel = sin_cos_add_mul( fangle, t->accel, power );
		} else if ( ac->is_heli ) {
			/*t->accel.x = 0;
			t->accel.y = 0;*/
			t->vel.x *= 0.99;
			t->vel.y *= 0.9;
		}
	}
	
	if ( ac->is_heli ) {
		t->accel.y >>= 1;
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
	
	if ( self->data.ac.is_heli && abs( self->vel.y ) > REALF( 1 ) ) {
		target.y = self->pos.y - REALF( 20 );
	}
	
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
#else
static Thing *add_heli( void )
{
	Thing *t = add_aircraft();
	if ( t ) {
		t->data.ac.is_heli = 1;
	}
	return t;
}
#endif

static void spawn_enemies( void )
{
	typedef Thing *(*SpawnFunc)( void );
	
	SpawnFunc spawn_funcs[] = {
		#if ENABLE_ENEMY_AIRCRAFT
		add_aircraft,
		add_heli,
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
	Real world_x = 0;
	
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
		Real old_mid, vel_y;
		
		old_mid = old_z[n];
		left = mid;
		mid = right;
		right = cur_z[(1+n)%WATER_RESOL];
		vel_y = REAL_DIV( abs( mid - old_mid ), REALF( W_TIMESTEP ) );
		
		/* Add particle effects if vertical velocity of the surface exceeds some thresholds */
		if ( vel_y > REALF( 8 ) ) {
			Vec2 pos;
			pos.x = world_x + ( prng_next() & REAL_FRACT_MASK );
			pos.y = REALF( W_WATER_LEVEL ) + mid;
			
			/* Add 'fog' (5) */
			add_particle( pos, 6, 16, PT_WATER2 );
			
			if ( vel_y > REALF( 10 ) ) {
				/* Add small drops (10) */
				add_particle( pos, 8, 4, PT_WATER1 );
			}
		}
		
		new_z[n] = 2*mid - old_mid + REAL_MUL( c, left + right - 2*mid );
		new_z[n] = REAL_MUL( REALF( WATER_DAMPING_FACTOR ), new_z[n] );
		
		world_x += REALF( WATER_ELEM_SPACING );
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
	h1 = w->z[(cell + 1) % WATER_RESOL];
	
	t = REAL_FRACT_PART( x );
	return REALF( W_WATER_LEVEL ) + h0 + REAL_MUL( t, h1 - h0 );
}

static void displace_water( Real x, Real delta )
{
	Water *w = &WORLD.water;
	w->z[ ( REALTOI( x ) + WATER_RESOL ) % WATER_RESOL ] += delta;
}

static Real get_avg_water_height( Real x0, Real x1, unsigned samples )
{
	Real x = x0;
	Real dx = ( x1 - x0 ) / samples;
	Real h = 0;
	while( x < x1 ) {
		h += get_water_height( x );
		x += dx;
	}
	return h / samples;
}

static Real measure_water_surface_angle( Real mid_x )
{
	unsigned s = 4;
	Real r = REALF( 5.0 );
	float a = atan2f( get_avg_water_height( mid_x + r, mid_x + 2 * r, s ) - get_avg_water_height( mid_x - 2 * r, mid_x - r, s ), r );
	return REALF( a );
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
		
		/* Reset acceleration */
		t->accel.x = 0;
		t->accel.y = REALF( W_GRAVITY );
		
		t->age += REALF( W_TIMESTEP );
		
		water_h = get_water_height( t->pos.x );
		
		if ( t->pos.y > water_h )
		{
			Real water_friction = REALF( 0.94 );
			
			t->underwater_time += REALF( W_TIMESTEP );
			t->accel.y -= t->buoancy;
			
			t->vel.x = REAL_MUL( t->vel.x, water_friction );
			t->vel.y = REAL_MUL( t->vel.y, water_friction );
			
			if ( t->type != T_PARTICLE ) {
				if ( t->pos.y > water_h + REALF( W_WATER_DEATH_LEVEL ) )
					t->hp = 0;
			} else {
				t->pos.y = water_h;
			}
		}
		else
		{	
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
				t->angle = measure_water_surface_angle( t->pos.x );
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
				/*
				if ( t->underwater_time )
					t->hp = 0;
				*/
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
				float m0, m1, m2, m3;
				float rx, ry;
				float a = REALTOF( t->parent->angle );
				Vec2 rw_pos; /* relative position in world space */
				
				m0 = cosf( a );
				m1 = sinf( a );
				m2 = -m1;
				m3 = m0;
				
				rx = t->rel_pos.x;
				ry = t->rel_pos.y;
				
				rw_pos.x = rx * m0 + ry * m2;
				rw_pos.y = rx * m1 + ry * m3;
				
				/* Follow parent */
				t->pos = t->parent->pos;
				t->pos.x += rw_pos.x;
				t->pos.y += rw_pos.y;
				t->vel = t->parent->vel;
				t->accel = t->parent->accel;
			}
		}
		else
		{
			/* Apply physics to everything */
			do_physics_step( t );
			
			if ( t->type != T_PARTICLE && t->pos.y > water_h )
			{
				const float thickness = 5000; /* larger value makes water move less */
				const float falloff = 2; /* falloff distance so that objects deep underwater don't make the surface move */
				Real f = REAL_MUL( REALF( falloff ) - MIN( t->pos.y - water_h, REALF( falloff ) ), t->mass * t->vel.y ) / ( thickness * falloff );
				displace_water( t->pos.x, f );
			}
		}
		
		/* Wrap around world edges */
		t->pos.x = ( REALF( W_WIDTH ) + t->pos.x ) % REALF( W_WIDTH );
		
		/* Kill things that sink too deep */
		if ( t->pos.y > REALF( W_WATER_DEATH_LEVEL ) )
			t->hp = 0;
		
		#if ENABLE_GODMODE
		if ( WORLD.player )
			WORLD.player->hp = REALF( 100 );
		#endif
		
		/* Kill things with no HP remaining */
		if ( t->hp <= 0 )
		{
			if ( t->type < T_PROJECTILE )
			{
				/* Throw pieces of debris */
				add_explosion( t->pos, t->vel.x, t->vel.y );
				
				#if 0
				if ( t->type == T_GUNSHIP || t->type == T_BATTLESHIP )
				{
					/* Exploding ships cause a large water splash: */
					int k;
					for( k=0; k<5; k++ )
					{
						Vec2 pos;
						pos.x = t->pos.x - 1024 + ( prng_next() & 0x7FF );
						pos.y = get_water_height( t->pos.x );
					}
				}
				#endif
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
