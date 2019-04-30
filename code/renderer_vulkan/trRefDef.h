#ifndef TR_REF_DEF_H_
#define TR_REF_DEF_H_ 

#include "../qcommon/q_shared.h"

#include "../renderercommon/tr_types.h"


#include "surface_type.h"


// 12 bits
// see QSORT_SHADERNUM_SHIFT

typedef struct {
	refEntity_t	e;

	float		axisLength;		// compensate for non-normalized axis

	qboolean	needDlights;	// true for bmodels that touch a dlight
	qboolean	lightingCalculated;
	vec3_t		lightDir;		// normalized direction towards light
	vec3_t		ambientLight;	// color normalized to 0-255
	unsigned char ambientLightRGBA[4]; // 32 bit rgba packed
    vec3_t		directedLight;
} trRefEntity_t;


//===============================================================================



// trRefdef_t holds everything that comes in refdef_t,
// as well as the locally generated scene information
typedef struct {
/*
    int			x, y, width, height;
	float		fov_x, fov_y;
	vec3_t		vieworg;
	float		viewaxis[3][3];		// transformation matrix

	int			time;				// time in milliseconds for shader effects and other time dependent rendering issues
	int			rdflags;			// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte		areamask[MAX_MAP_AREA_BYTES];


	// text messages for deform text shaders
	char		text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];
*/
    refdef_t    rd;
    qboolean	AreamaskModified;	// qtrue if areamask changed since last scene
	float		floatTime;			// tr.refdef.time / 1000.0
	int			num_entities;
	trRefEntity_t	*entities;

	int			num_dlights;
	struct dlight_s	*dlights;

	int			numPolys;
	struct srfPoly_s* polys;

	int			numDrawSurfs;
	struct drawSurf_s * drawSurfs;
} trRefdef_t;

#endif
