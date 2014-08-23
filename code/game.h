#ifndef _GAME_H
#define _GAME_H
#include "real.h"

#define STRUCT_PROTO( t ) struct t; typedef struct t t

STRUCT_PROTO( Aircraft );
STRUCT_PROTO( AAGun );
STRUCT_PROTO( Particle );
STRUCT_PROTO( Projectile );

STRUCT_PROTO( ThingData );
STRUCT_PROTO( Thing );
STRUCT_PROTO( World );

#define INVALID_THING_ID 0
typedef unsigned ThingID;

struct Aircraft {
	Real gun_timer;
	int gun_trigger;
	int throttle_on;
	int num_bombs;
};

struct AAGun {
	Real gun_timer;
};

struct Projectile {
	int owned_by_player;
	/* ThingID owner; */
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

struct ThingData {
	Aircraft ac;
	AAGun aa;
	Particle pt;
	Projectile pr;
};

struct Thing {
	ThingType type;
	ThingID id; /* unique thing ID, never the same */
	Real hp;
	Vec2 old_pos;
	Vec2 pos;
	Vec2 vel;
	Vec2 accel;
	Real angle;
	Thing *parent; /* If parent is non-NULL, then pos and old_pos are relative to parent */
	ThingID parent_id; /* If parent_id != parent->id, then we know that parent has died */
	Vec2 rel_pos; /* Relative position to parent */
	U32 age; /* unsigned Real; seconds */
	U32 underwater_time;
	Real buoancy;
	int mass;
	ThingData data;
};

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
typedef struct Water {
	/* 1-D heightmaps relative to W_WATER_LEVEL */
	Real buffers[3][WATER_RESOL];
	Real *old_z, *z, *temp; /* these point to buffers */
} Water;

#define MAX_THINGS 16536

struct World {
	Thing *player;
	int enemy_spawn_timer; /* (game ticks) */
	unsigned num_things;
	unsigned sorted_things[MAX_THINGS]; /* Thing indexes; Sorted by X coordinate for efficient collision testing */
	Thing things[MAX_THINGS];
	Water water;
};

extern struct World WORLD;

#define GAME_TICKS_PER_SEC 60

/* W_... = World constants: */

#define W_WIDTH 256.0 /* World width */
#define W_HEIGHT 60.0 /* World height (counting from water level) */
#define W_TIMESTEP (1.0f / GAME_TICKS_PER_SEC)
#define W_GRAVITY 9.81f

#define W_WATER_LEVEL 20.0f /* Water y coordinate */
#define W_WATER_BUOANCY 25.0f
#define W_WATER_DEATH_LEVEL (W_WATER_LEVEL+15)

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
