#ifndef RB_SURFACE_H_
#define RB_SURFACE_H_

#include "../renderercommon/tr_types.h"
#include "tr_shader.h"
#include "surface_type.h"

#define	MAX_GRID_SIZE		65			// max dimensions of a grid mesh in memory


typedef void (* Fn_RB_SurfaceTable_t)(void *); 


extern const Fn_RB_SurfaceTable_t rb_surfaceTable[SF_NUM_SURFACE_TYPES];

typedef struct srfGridMesh_s {
	surfaceType_t	surfaceType;

	// dynamic lighting information
	int				dlightBits;

	// culling information
	vec3_t			meshBounds[2];
	vec3_t			localOrigin;
	float			meshRadius;

	// lod information, which may be different
	// than the culling information to allow for
	// groups of curves that LOD as a unit
	vec3_t			lodOrigin;
	float			lodRadius;
	int				lodFixed;
	int				lodStitched;

	// vertexes
	int				width, height;
	float			*widthLodError;
	float			*heightLodError;
	drawVert_t		verts[1];		// variable sized
} srfGridMesh_t;




//
// in memory representation
//

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2



void RB_CHECKOVERFLOW(uint32_t v, uint32_t i);


void RB_BeginSurface(shader_t *shader, int fogNum );
void RB_EndSurface(void);
void RB_CheckOverflow( int verts, int indexes );
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, byte *color, float s1, float t1, float s2, float t2 );
void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, byte *color );

void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, int fogIndex, int dlightMap );


#endif
