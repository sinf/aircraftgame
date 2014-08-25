#ifndef _MODELS_H
#define _MODELS_H
#include "real.h"

typedef enum {
	M_AIRCRAFT=0,
	M_RADAR,
	M_GUNSHIP,
	M_BATTLESHIP,
	M_DEBRIS,
	M_AAGUN_BARREL,
	M_AIRCRAFT_FLAME,
	M_HELI_BODY,
	M_HELI_ROTOR,
	NUM_MODELS
} ModelID;

#define BAD_MODEL_ID NUM_MODELS

/* Maximum vertices per ANY mesh */
#define MAX_MODEL_VERTS 40

/* MODEL_INFO_T flags */
#define F_NONE 0
#define F_2D 1
#define F_MIRROR 2

#define MODEL_DIMENSIONS(minfo) (3-((minfo).flags & F_2D))

typedef struct {
	U8 scale[3];
	U8 flags;
	U8 num_verts;
	U8 num_indices;
	U32 color;
} MODEL_INFO_T;

extern const MODEL_INFO_T MODEL_INFO[NUM_MODELS];

/* vertices expanded to wider integers */
S32 model_vertices_unpacked[NUM_MODELS][MAX_MODEL_VERTS*3];

/* pointers to parts of MODEL_INDICES */
U8 const *model_indices_unpacked[NUM_MODELS];

/* initializes *_unpacked */
void unpack_models( void );

#endif
