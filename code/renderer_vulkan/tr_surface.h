#ifndef RB_SURFACE_H_
#define RB_SURFACE_H_

#include "../renderercommon/tr_types.h"
#include "surface_type.h"
#include "tr_shader.h"

#define	MAX_GRID_SIZE		65			// max dimensions of a grid mesh in memory


typedef void (* Fn_RB_SurfaceTable_t)( void *); 

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


void RB_BeginSurface(struct shader_s * const shader, int fogNum, struct shaderCommands_s * const pTess);
void RB_EndSurface( struct shaderCommands_s * const pTess );
void RB_CheckOverflow( uint32_t verts, uint32_t indexes, struct shaderCommands_s * const pTess);
void RB_AddQuadStampExt(const float origin[3], vec3_t left, vec3_t up, const uint8_t * const color,
        float s1, float t1, float s2, float t2 );

void R_AddDrawSurf( surfaceType_t *surface, struct shader_s *shader, int fogIndex, int dlightMap );


#endif
