#ifndef _GAME_H
#define _GAME_H
#include "real.h"

#define STRUCT_PROTO( t ) struct t; typedef struct t t

STRUCT_PROTO( Aircraft );
STRUCT_PROTO( AAGun );
STRUCT_PROTO( Particle );
STRUCT_PROTO( Projectile );

STRUCT_PROTO( Thing );
STRUCT_PROTO( World );

#define INVALID_THING_ID 0
typedef unsigned ThingID;

struct Aircraft {
	Real gun_timer;
	int gun_trigger;
	int throttle_on;
	int num_bombs;
	int is_heli;
};

struct AAGun {
	Real gun_timer;
};

typedef enum {
	DAMAGES_PLAYER,
	DAMAGES_ENEMIES
	/*DAMAGES_EVERYONE*/
} DamageType;

struct Projectile {
	DamageType dmg;
};

typedef enum {
	PT_WATER1,
	PT_WATER2,
	PT_SMOKE,
	N_PARTICLE_TYPES
} ParticleType;

struct Particle {
	ParticleType type;
	/* All particles live exactly this many seconds */
	#define MAX_PARTICLE_TIME 1.5f
};

typedef enum {
	T_AIRCRAFT=0,
	T_GUNSHIP,
	T_BATTLESHIP,
	T_AAGUN,
	T_RADAR, /** Spinning radar disk on top of the battleship */
	T_PROJECTILE,
	T_PARTICLE, /** Just eye candy */
	T_DEBRIS,
	NUM_THING_TYPES
} ThingType;

typedef union {
	Aircraft ac;
	AAGun aa;
	Particle pt;
	Projectile pr;
} ThingData;

typedef struct PhysicsBlob {
	Vec2 pos;
	Vec2 vel;
	Vec2 accel;
	Real buoancy; /* vertical acceleration. positive is up */
	int mass; /* all masses should be in range 1 .. 255. Mass must not ever be 0 because of divisions! */
	/*
	Real radius;
	(a pointer to parent PhysicsBlob? handle attacked blobs in the physics code?)
	*/
} PhysicsBlob;

struct Thing {
	ThingType type;
	Real hp;
	PhysicsBlob phys;
	Real angle;
	Thing *parent; /* If parent is non-NULL, then pos and old_pos are relative to parent */
	Vec2 rel_pos; /* Relative position to parent */
	U32 age; /* unsigned Real; seconds */
	U32 underwater_time;
	ThingData data;
	/* Attribute bits. Default value should always be 0 */
	unsigned dies_of_old_age : 1;
	unsigned tilts_like_a_boat : 1;
	unsigned can_not_die : 1;
	U32 max_life_time; /* unsigned Real; seconds */
};

/* The object needs at least this much mass to be able to splash water */
#define WATER_SPLASH_MIN_MASS 5

/* How to fix the physics:
- Thing needs to have mass
- Mass affects velocity damping in water
- Mass affects air resistance
- Mass affects impacts (water splashing, projectile kickback)
- Thing needs some aerodynamics information for more accurate air/water friction
- Thing needs a buoancy factor. Some things float more easily (boat vs cannon ball)
*/

#define WATER_T 17
#define WATER_DAMPING_FACTOR 0.99
#define WATER_SPLASH_FORCE 0.004
#define WATER_SPLASH_FORCE_EXPL 8.0
#define WATER_RESOL 256
#define WATER_ELEM_SPACING ((W_WIDTH/(double)WATER_RESOL))

#define WATER_CELL_AT_X(x) (WATER_RESOL*(x)/REALF(W_WIDTH))
#define WATER_CELL_AT_X_WRAPPED(x) (WATER_CELL_AT_X(x)+WATER_RESOL)%WATER_RESOL)

typedef struct Water {
	/* 1-D heightmaps relative to W_WATER_LEVEL */
	Real buffers[3][WATER_RESOL];
	Real *old_z, *z, *temp; /* these point to buffers */
} Water;

#define MAX_THINGS 16536

struct World {
	unsigned current_tick; /* incremented by 1 every tick */
	Thing *player;
	int enemy_spawn_timer; /* (game ticks) */
	unsigned num_things;
	unsigned sorted_things[MAX_THINGS]; /* Thing indexes; Sorted by X coordinate for efficient collision testing */
	Thing *things; /* Things from the current frame */
	Thing things_buf[2][MAX_THINGS]; /* Things from the previous and current frame. The lowest bit in current_tick can be used as an index to get the current things */
	Water water;
};

extern struct World WORLD;

#define AIRCRAFT_BUOANCY 3.0

#define GAME_TICKS_PER_SEC 60

/* W_... = World constants: */

#define W_WIDTH 256.0 /* World width */
#define W_HEIGHT 60.0 /* World height (counting from water level) */
#define W_TIMESTEP (1.0f / GAME_TICKS_PER_SEC)
#define W_GRAVITY 9.81f

#define W_WATER_LEVEL 20.0f /* Water y coordinate */
#define W_WATER_DEPTH 30.0f
#define W_WATER_DEATH_LEVEL (W_WATER_LEVEL+35)

/* Time interval between enemy wave spawns (in seconds) */
#define ENEMY_SPAWN_INTERVAL 5

#define MAX_AIRCRAFT_VEL 1.5f
#define MAX_AIRCRAFT_ALTITUDE ((W_WATER_LEVEL-W_HEIGHT))
#define AIRCRAFT_ENGINE_POWER 105.0f
#define AIRCRAFT_GUN_TIMER 0.1
#define AIRCRAFT_ROTATE_SPEED 3.7f

#define PROJECTILE_VEL 64.0f
#define PROJECTILE_DAMAGE 24
#define PROJECTILE_BUOANCY 2.5
#define PROJECTILE_MASS 20

#define MAX_UNDERWATER_VEL 0.4f
#define MAX_THING_HP 80

#define AAGUN_GUN_TIMER 1.0
#define AAGUN_ROTATE_SPEED 1.0

void init_world( void );
void update_world( void );

#endif
