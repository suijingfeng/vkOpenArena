#ifndef TR_SKY_H_
#define TR_SKY_H_

#include "tr_image.h"


typedef struct {
	float       cloudHeight;
	struct image_s *   outerbox[6];
    struct image_s *   innerbox[6];
} skyParms_t;

void RB_StageIteratorSky( void );

void R_InitSkyTexCoords( float cloudLayerHeight );

#endif
