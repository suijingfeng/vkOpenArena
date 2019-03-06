#ifndef TR_GLOBALS_H_
#define TR_GLOBALS_H_

#include "tr_local.h"
#include "tr_model.h"


typedef struct {
	int		c_surfaces, c_shaders, c_vertexes, c_indexes, c_totalIndexes;
	
	int		c_dlightVertexes;
	int		c_dlightIndexes;

	int		msec;			// total msec for backend run
} backEndCounters_t;


/*
** trGlobals_t 
**
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/


typedef struct {
	qboolean				registered;		// cleared at shutdown, set at beginRegistration

	int						visCount;		// incremented every time a new vis cluster is entered
	int						frameCount;		// incremented every frame
	int						viewCount;		// incremented every view (twice a scene if portaled)
											// and every R_MarkFragments call

	qboolean				worldMapLoaded;
	world_t					*world;

	const unsigned char		*externalVisData;	// from RE_SetWorldVisData, shared with CM_Load

	image_t					*defaultImage;
	image_t					*scratchImage[32];
	image_t					*fogImage;
	image_t					*dlightImage;	// inverse-quare highlight for projective adding
	image_t					*whiteImage;			// full of 0xff
	image_t					*identityLightImage;	// full of tr.identityLightByte
//	image_t					*fontIamge;	

	shader_t				*defaultShader;
    shader_t                *cinematicShader;
	shader_t				*shadowShader;
	shader_t				*projectionShadowShader;
//	shader_t				*fontShader;

	int						numLightmaps;
	image_t					*lightmaps[MAX_LIGHTMAPS];

	trRefEntity_t			*currentEntity;
	trRefEntity_t			worldEntity;		// point currentEntity at this when rendering world
	int						currentEntityNum;
	int						shiftedEntityNum;	// currentEntityNum << QSORT_ENTITYNUM_SHIFT
	model_t					*currentModel;

	viewParms_t				viewParms;

	float					identityLight;		// 1.0 / ( 1 << overbrightBits )
	int						identityLightByte;	// identityLight * 255

	orientationr_t			or;					// for current entity

	trRefdef_t				refdef;

	int						viewCluster;

	vec3_t					sunLight;			// from the sky shader for this level
	vec3_t					sunDirection;

	frontEndCounters_t		pc;
	int						frontEndMsec;		// not in pc due to clearing issue

	//
	// put large tables at the end, so most elements will be
	// within the +/32K indexed range on risc processors
	//
	model_t					*models[MAX_MOD_KNOWN];
	int						numModels;

	int						numImages;
	image_t	*               images[MAX_DRAWIMAGES];

	// shader indexes from other modules will be looked up in tr.shaders[]
	// shader indexes from drawsurfs will be looked up in sortedShaders[]
	// lower indexed sortedShaders must be rendered first (opaque surfaces before translucent)
	int						numShaders;
	shader_t				*shaders[MAX_SHADERS];
	shader_t				*sortedShaders[MAX_SHADERS];

	int						numSkins;
	skin_t					*skins[MAX_SKINS];

	float					sinTable[FUNCTABLE_SIZE];
	float					squareTable[FUNCTABLE_SIZE];
	float					triangleTable[FUNCTABLE_SIZE];
	float					sawToothTable[FUNCTABLE_SIZE];
	float					inverseSawToothTable[FUNCTABLE_SIZE];
	float					fogTable[FOG_TABLE_SIZE];
} trGlobals_t;



// all of the information needed by the back end must be contained in a backEndData_t.
// This entire structure is duplicated so the front and back end can run in parallel
// on an SMP machine

typedef struct
{
	drawSurf_t	drawSurfs[MAX_DRAWSURFS];
	dlight_t	dlights[MAX_DLIGHTS];
	trRefEntity_t	entities[MAX_REFENTITIES];
	srfPoly_t	*polys;//[MAX_POLYS];
	polyVert_t	*polyVerts;//[MAX_POLYVERTS];
	renderCommandList_t	commands;
} backEndData_t;


// all state modified by the back end is seperated
// from the front end state
typedef struct {
	trRefdef_t	refdef;
	viewParms_t	viewParms;
	orientationr_t	or;
	backEndCounters_t	pc;
	qboolean	isHyperspace;
	trRefEntity_t	*currentEntity;

	qboolean	projection2D;	// if qtrue, drawstretchpic doesn't need to change modes
	unsigned char	Color2D[4];
	trRefEntity_t	entity2D;	// currentEntity will point at this when doing 2D rendering
} backEndState_t;

extern backEndState_t backEnd;
extern trGlobals_t	tr;
extern backEndData_t* backEndData;	// the second one may not be allocated
extern glconfig_t	glConfig;		// outside of TR since it shouldn't be cleared during ref re-init
#endif
