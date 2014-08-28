#include <string.h>
#include "system.h"
#include "support.h"

#define _GAME_INTERNALS
#include "game.h"

#define ENABLE_GODMODE 0
#define ENABLE_ENEMY_AIRCRAFT 1
#define DISARM_ENEMIES 0

World WORLD = {0};

static Thing *add_thing( ThingType type, Vec2 pos, Thing *parent );
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

static Thing *add_player( void )
{
	Thing *t = add_aircraft();
	#if ENABLE_GODMODE
	t->can_not_die = 1;
	#endif
	return t;
}

void init_world( void )
{
	WORLD.current_tick = 0;
	WORLD.things = WORLD.things_buf[0];
	
	WORLD.enemy_spawn_timer = 0;
	WORLD.num_things = 0;
	WORLD.player = add_player();
	
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

static Thing *add_thing( ThingType type, Vec2 pos, Thing *parent )
{
	Thing *thing;
	Real hp = REALF( MAX_THING_HP );
	
	if ( WORLD.num_things >= MAX_THINGS )
		return NULL;
	
	thing = list_append( WORLD.things, &WORLD.num_things, sizeof(Thing) );
	
	if ( type == T_BATTLESHIP )
		hp <<= 1;
	
	thing->type = type;
	thing->hp = hp;
	
	thing->phys.pos = pos;
	thing->phys.buoancy = REALF( 5.0f );
	thing->phys.mass = 20;
	
	if ( parent )
	{
		thing->parent = parent;
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
		
		t->phys.vel = vel;
		t->data.pt.type = type;
		
		t->dies_of_old_age = 1;
		t->phys.mass = 1;
		t->max_life_time = REALF( MAX_PARTICLE_TIME );
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
			t->phys.vel.x = vel_x - ( prng_next() & 0x7FF ) + 1024;
			t->phys.vel.y = vel_y - ( prng_next() & 0xFFF );
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
		t->phys.mass = 50;
		t->phys.buoancy = REALF( AIRCRAFT_BUOANCY );
		
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
		gs->phys.vel.x = ship_vel; /* Set the ship in slow horizontal motion */
		gs->phys.buoancy = REALF( 25.0f );
		gs->tilts_like_a_boat = 1;
		
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
		bs->phys.vel.x = ship_vel;
		bs->phys.buoancy = REALF( 25.0f );
		bs->tilts_like_a_boat = 1;
		/* bs->mass = 200; */
		
		add_thing( T_AAGUN, gun1_offset, bs )->phys.mass = 1;
		add_thing( T_AAGUN, gun2_offset, bs )->phys.mass = 1;
		add_thing( T_RADAR, radar_offset, bs )->phys.mass = 1;
	}
	
	return bs;
}

static void do_physics_step( PhysicsBlob *blob )
{
	/* The equation for "Time Corrected Verlet" (TCV) would be:
		Xi+1 = Xi + (Xi - Xi-1) * (DTi / DTi-1) + A * DTi * DTi
	The shitty Euler integration needs less code however:
		x = x0 + v * t + 1/2 * a * t * t
	*/
	
	
	const Vec2 accel = blob->accel;
	Vec2 pos = blob->pos, vel = blob->vel;
	DReal dt = REALF( W_TIMESTEP );
	
	blob->vel = vel = v_addmul( vel, accel, dt );
	blob->pos = v_addmul( pos, vel, dt );
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
	
	if ( victim->type < T_PROJECTILE && test_collision( &proj->phys.pos, &victim->phys.pos, r ) )
	{
		proj->hp = 0;
		victim->hp -= REALF( PROJECTILE_DAMAGE );
		add_smoke( proj->phys.pos );
		return 1;
	}
	
	return 0;
}

static void collide_projectile( unsigned num_things, Thing *all_things, Thing *proj, Thing *player )
{
	if ( proj->data.pr.dmg == DAMAGES_PLAYER ) {
		if ( player )
			collide_projectile_1( proj, player );
	} else {
		/* Note: The projectile is collision-tested against itself but collide_projectile_1() will reject because the victim is a projectile as well */
		unsigned n;
		for( n=0; n<num_things; n++ ) {
			Thing *victim = all_things + n;
			if ( victim != player && collide_projectile_1( proj, victim ) )
				break;
		}
	}
}

static void shoot_projectile( Thing *t, float angle, DamageType damg )
{
	Thing *p;
	
	p = add_thing( T_PROJECTILE, t->phys.pos, NULL );
	if ( p )
	{
		p->phys.vel = sin_cos_add_mul( angle, t->phys.vel, PROJECTILE_VEL );
		p->data.pr.dmg = damg;
		
		p->phys.buoancy = REALF( PROJECTILE_BUOANCY );
		p->phys.mass = PROJECTILE_MASS;
		
		#if 0
		/* Rock ships as they fire */
		if ( t->type == T_AAGUN )
			displace_water( t->phys.pos.x, REALF( 0.2 ) );
		#endif
	}
}

static void update_aircraft( Thing *t, int is_player )
{
	Aircraft *ac = &t->data.ac;
	float fangle = REALTOF( t->angle );
	int systems_online = 1;
	
	/* Cripple the aircraft when underwater */
	systems_online = ( t->underwater_time <= 0 );
	
	systems_online = 1;
	
	/* Limit velocity (air drag like behaviour). Makes the aircraft easier to control */
	t->phys.vel = adjust_vector_length( t->phys.vel, REALF(MAX_AIRCRAFT_VEL), 1 );
	
	if ( systems_online )
	{
		/* Calculate acceleration */
		if ( ac->throttle_on ) {
			float power = ac->is_heli ? AIRCRAFT_ENGINE_POWER*0.25f : AIRCRAFT_ENGINE_POWER;
			t->phys.accel = sin_cos_add_mul( fangle, t->phys.accel, power );
		} else if ( ac->is_heli ) {
			/*t->accel.x = 0;
			t->accel.y = 0;*/
			t->phys.vel.x *= 0.99;
			t->phys.vel.y *= 0.9;
		}
	}
	
	if ( ac->is_heli ) {
		t->phys.accel.y >>= 1;
	}
	
	t->data.ac.gun_timer -= REALF( W_TIMESTEP );
	if ( t->data.ac.gun_timer <= REALF(0) )
	{
		t->data.ac.gun_timer = 0;
		
		if ( t->data.ac.gun_trigger )
		{
			shoot_projectile( t, fangle, is_player ? DAMAGES_ENEMIES : DAMAGES_PLAYER );
			t->data.ac.gun_timer = REALF( AIRCRAFT_GUN_TIMER );
		}
	}
	
	if ( t->phys.pos.y < REALF(MAX_AIRCRAFT_ALTITUDE) )
	{
		/* TODO: limit aircraft height in a more "natural" way
		(the engine should stall above a certain height) */
		t->phys.pos.y = REALF(MAX_AIRCRAFT_ALTITUDE);
		t->phys.vel.y = abs( t->phys.vel.y );
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
	slow_rotate( &self->angle, &self->phys.pos, target, max_rot );
}

static void do_enemy_aircraft_logic( Thing *self )
{
	/* Just go forward by default */
	Vec2 target = self->phys.pos;
	target.x += self->phys.vel.x;
	target.y += self->phys.vel.y;
	
	if ( self->data.ac.is_heli && abs( self->phys.vel.y ) > REALF( 1 ) ) {
		target.y = self->phys.pos.y - REALF( 20 );
	}
	
	if ( WORLD.player )
	{
		/* Follow the player */
		target = WORLD.player->phys.pos;
	}
	
	#if 1
	/* Prevent diving into the ocean (extra intelligency; could do without) */
	target.y = MIN( target.y, REALF( W_WATER_LEVEL - 3 ) );
	#endif
	
	slow_rotate_thing( self, &target, REALF( AIRCRAFT_ROTATE_SPEED * W_TIMESTEP ) );
	
	self->data.ac.throttle_on = 1;
	#if DISARM_ENEMIES
	self->data.ac.gun_trigger = 0;
	#else
	self->data.ac.gun_trigger = ( WORLD.player != NULL ) && !( prng_next() & 0xFF );
	#endif
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
			x += WORLD.player->phys.pos.x;
		}
		
		t = spawn_funcs[ prng_next() & 0x3 ]();
		if ( t )
			t->phys.pos.x = x;
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

static void update_things( World *world )
{
	const unsigned tick_bit = world->current_tick & 1;
	
	Thing* const things_from = world->things_buf[tick_bit];
	Thing* const things_to = world->things_buf[!tick_bit];
	
	Thing* const old_player = world->player;
	const unsigned num_old_things = world->num_things;
	unsigned n;
	
	world->things = things_to;
	world->num_things = 0;
	
	/* maps old index to new index */
	unsigned short thing_index_remap[MAX_THINGS];
	memset( thing_index_remap, 0, sizeof(thing_index_remap) );
	
	for( n=0; n<num_old_things; n++ )
	{
		const int is_player = ( things_from + n ) == old_player;
		Thing t = things_from[n];
		Real water_h;
		
		t.age += REALF( W_TIMESTEP );
		
		/* Reset acceleration */
		t.phys.accel.x = 0;
		t.phys.accel.y = REALF( W_GRAVITY );
		
		/* Water physics */
		water_h = get_water_height( t.phys.pos.x );
		if ( t.phys.pos.y > water_h )
		{
			Real water_friction = REALF( 0.94 );
			
			t.underwater_time += REALF( W_TIMESTEP );
			t.phys.accel.y -= t.phys.buoancy;
			
			t.phys.vel.x = REAL_MUL( t.phys.vel.x, water_friction );
			t.phys.vel.y = REAL_MUL( t.phys.vel.y, water_friction );
			
			if ( t.type != T_PARTICLE ) {
				if ( t.phys.pos.y > water_h + REALF( W_WATER_DEATH_LEVEL ) )
					t.hp = 0;
			} else {
				t.phys.pos.y = water_h;
			}
		}
		else
		{
			t.underwater_time = 0;
		}
		
		/* Self-destruct timer */
		if ( t.dies_of_old_age ) {
			if ( t.age >= t.max_life_time )
				t.hp = 0;
		}
		
		/* Fake physics for objects that float on water */
		if ( t.tilts_like_a_boat ) {
			t.angle = measure_water_surface_angle( t.phys.pos.x );
		}
		
		switch( t.type )
		{
			case T_AIRCRAFT:
				if ( !is_player ) do_enemy_aircraft_logic( &t );
				update_aircraft( &t, is_player );
				break;
			
			case T_AAGUN:
				if ( world->player )
				{
					/* Aim the gun at player */
					Real r = REALF( W_TIMESTEP * AAGUN_ROTATE_SPEED );
					slow_rotate_thing( &t, &world->player->phys.pos, r );
					
					#if !DISARM_ENEMIES
					/* Shoot all the time */
					t.data.aa.gun_timer += REALF( W_TIMESTEP );
					if ( t.data.aa.gun_timer > REALF(AAGUN_GUN_TIMER) )
					{
						shoot_projectile( &t, REALTOF(t.angle), DAMAGES_PLAYER );
						t.data.aa.gun_timer = 0;
					}
					#endif
				}
				break;
			
			case T_PROJECTILE:
				collide_projectile( num_old_things, things_from, &t, old_player );
				break;
			
			case T_DEBRIS:
				if ( t.age > REALF( 7.0 ) )
					t.hp = 0;
				if ( ( prng_next() & 0x3 ) == 0 )
				{
					if ( !t.underwater_time )
						add_smoke( t.phys.pos );
				}
				t.angle += REALF( 5 * W_TIMESTEP );
				break;
			
			default:
				break;
		}
		
		if ( !t.parent )
		{
			do_physics_step( &t.phys );
			
			/* Water splashing effect */
			if ( t.phys.mass >= WATER_SPLASH_MIN_MASS && t.phys.pos.y > water_h )
			{
				const float thickness = 5000; /* larger value makes water move less */
				const float falloff = 2; /* falloff distance so that objects deep underwater don't make the surface move */
				Real f = REAL_MUL( REALF( falloff ) - MIN( t.phys.pos.y - water_h, REALF( falloff ) ), t.phys.mass * t.phys.vel.y ) / ( thickness * falloff );
				displace_water( t.phys.pos.x, f );
			}
		}
		
		/* Wrap around world edges */
		t.phys.pos.x = ( REALF( W_WIDTH ) + t.phys.pos.x ) % REALF( W_WIDTH );
		
		/* Kill things that sink too deep */
		if ( t.phys.pos.y > REALF( W_WATER_DEATH_LEVEL ) )
			t.hp = 0;
		
		if ( t.hp > 0 || t.can_not_die ) {
			/* Thing keeps on living. Write to the Thing list of the next frame */
			
			if ( world->num_things < MAX_THINGS )
			{
				unsigned index = world->num_things++;
				
				thing_index_remap[n] = index + 1;
				things_to[index] = t;
			}
		} else {
			/* Thing has died. It will not be written to the new Thing list */
			
			if ( t.type < T_PROJECTILE )
			{
				/* Throw pieces of debris */
				add_explosion( t.phys.pos, t.phys.vel.x, t.phys.vel.y );
				
				#if 0
				if ( t->type == T_GUNSHIP || t->type == T_BATTLESHIP )
				{
					/* Exploding ships cause a large water splash: */
					int k;
					for( k=0; k<5; k++ )
					{
						Vec2 pos;
						pos.x = t->phys.pos.x - 1024 + ( prng_next() & 0x7FF );
						pos.y = get_water_height( t->phys.pos.x );
					}
				}
				#endif
			}
		}
	}
	
	if ( old_player && thing_index_remap[old_player - things_from] )
		world->player = things_to + thing_index_remap[ old_player - things_from ] - 1;
	else
		world->player = NULL;
	
	for( n=0; n<world->num_things; n++ )
	{
		Thing *t = things_to + n;
		
		if ( t->parent )
		{
			unsigned old_parent_index = t->parent - things_from;
			unsigned parent_index = thing_index_remap[old_parent_index];
			
			if ( !parent_index ) {
				/* Parent has died */
				t->hp = 0;
				t->parent = NULL;
			} else {
				float m0, m1, m2, m3;
				float rx, ry;
				float a;
				Vec2 rw_pos; /* relative position in world space */
				
				t->parent = things_to + parent_index - 1;
				a = REALTOF( t->parent->angle );
				
				m0 = cosf( a );
				m1 = sinf( a );
				m2 = -m1;
				m3 = m0;
				
				rx = t->rel_pos.x;
				ry = t->rel_pos.y;
				
				rw_pos.x = rx * m0 + ry * m2;
				rw_pos.y = rx * m1 + ry * m3;
				
				/* Follow parent */
				t->phys.pos = t->parent->phys.pos;
				t->phys.pos.x += rw_pos.x;
				t->phys.pos.y += rw_pos.y;
				t->phys.vel = t->parent->phys.vel;
				t->phys.accel = t->parent->phys.accel;
			}
		}
	}
}

void update_world( void )
{
	update_water( &WORLD.water );
	update_things( &WORLD );
	
	if ( --WORLD.enemy_spawn_timer <= 0 )
	{
		WORLD.enemy_spawn_timer = ( ENEMY_SPAWN_INTERVAL * GAME_TICKS_PER_SEC );
		spawn_enemies();
	}
	
	++WORLD.current_tick;
}
