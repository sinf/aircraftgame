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
	NUM_MODELS
} ModelID;

#define BAD_MODEL_ID NUM_MODELS

void unpack_models( void );

/* warning: screws up the modelview matrix */
void draw_model( ModelID m, Real x, Real y, Real yaw, Real roll );

#endif
