#ifndef SRF_SURFACE_FACE_TYPE_H_
#define SRF_SURFACE_FACE_TYPE_H_

#include "surface_type.h"
#include "../qcommon/q_shared.h"

#define	VERTEXSIZE	8
typedef struct {
	surfaceType_t surfaceType;
	cplane_t	plane;

	// dynamic lighting information
	int			dlightBits;

	// triangle definitions (no normals at points)
	int			numPoints;
	int			numIndices;
	int			ofsIndices;
	float		points[1][VERTEXSIZE];	// variable sized
										// there is a variable length list of indices here also
} srfSurfaceFace_t;


#endif
