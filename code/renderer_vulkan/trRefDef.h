#ifndef TR_REF_DEF_H_
#define TR_REF_DEF_H_ 

#include "../qcommon/q_shared.h"

#include "../renderercommon/tr_types.h"

#include "surface_type.h"


// 12 bits
// see QSORT_SHADERNUM_SHIFT

typedef struct {
        refEntity_t e;
        // compensate for non-normalized axis
        float axisLength;	
        // true for bmodels that touch a dlight
        qboolean needDlights;
        qboolean lightingCalculated;
        float lightDir[3]; // normalized direction towards light
        float ambientLight[3];	// color normalized to 0-255
        unsigned char ambientLightRGBA[4]; // 32 bit rgba packed
        float directedLight[3];
} trRefEntity_t;


//===============================================================================



// trRefdef_t holds everything that comes in refdef_t,
// as well as the locally generated scene information
typedef struct trRefdef_s {

	int x, y, width, height;
	float fov_x, fov_y;
	float vieworg[3];
	float viewaxis[3][3];		// transformation matrix

	int time;
	// time in milliseconds for shader effects and other time dependent rendering issues
	int rdflags;			// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte areamask[MAX_MAP_AREA_BYTES];
    
	qboolean AreamaskModified;	// qtrue if areamask changed since last scene
	float floatTime;			// tr.refdef.time / 1000.0

	// text messages for deform text shaders
	char text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];


	unsigned int num_entities;
	trRefEntity_t * entities;

	unsigned int num_dlights;
	struct dlight_s	* dlights;

	unsigned int numPolys;
	struct srfPoly_s * polys;

	unsigned int numDrawSurfs;
	struct drawSurf_s * drawSurfs;
} trRefdef_t;

#endif
