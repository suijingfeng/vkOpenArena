#ifndef SRF_TRIANGLES_TYPE_H_
#define SRF_TRIANGLES_TYPE_H_

#include "surface_type.h"

// misc_models in maps are turned into direct geometry by q3map
typedef struct {
	surfaceType_t surfaceType;

	// dynamic lighting information
	int	dlightBits;

	// culling information (FIXME: use this!)
	float bounds[2][3];
	float localOrigin;
	float radius;

	// triangle definitions

	int	*       indexes;
	drawVert_t * verts;

    int	numIndexes;
	int	numVerts;

} srfTriangles_t;


#endif
