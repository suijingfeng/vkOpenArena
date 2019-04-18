#ifndef TR_FLARE_H_
#define TR_FLARE_H_

#include "tr_surface.h"

typedef struct srfFlare_s {
	surfaceType_t	surfaceType;
	vec3_t			origin;
	vec3_t			normal;
	vec3_t			color;
} srfFlare_t;

void RB_SurfaceFlare(srfFlare_t *surf);


#endif
