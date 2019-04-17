#ifndef TR_SKY_H_
#define TR_SKY_H_

#include "tr_image.h"

/*
============================================================

SKIES

============================================================
*/

typedef struct {
	float		cloudHeight;
	image_t		*outerbox[6], *innerbox[6];
} skyParms_t;

void RB_StageIteratorSky( void );

void R_InitSkyTexCoords( float cloudLayerHeight );

#endif
