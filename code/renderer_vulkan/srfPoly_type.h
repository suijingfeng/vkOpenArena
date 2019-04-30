#ifndef SRF_POLY_TYPE_H_
#define SRF_POLY_TYPE_H_

#include "surface_type.h"
#include "../renderercommon/tr_types.h"

// when cgame directly specifies a polygon, it becomes a srfPoly_t
// as soon as it is called
typedef struct srfPoly_s
{
	surfaceType_t	surfaceType;
	qhandle_t		hShader;
	int				fogIndex;
	int				numVerts;
	polyVert_t		*verts;
} srfPoly_t;


#endif
