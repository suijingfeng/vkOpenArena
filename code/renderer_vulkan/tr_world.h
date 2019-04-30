#ifndef TR_WORLD_H_
#define TR_WORLD_H_

#include "viewParms.h"
#include "trRefDef.h"

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


#define	CONTENTS_NODE		-1
typedef struct mnode_s {
	// common with leaf and node
	int			contents;		// -1 for nodes, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current
	vec3_t		mins, maxs;		// for bounding box culling
	struct mnode_s	*parent;

	// node specific
	cplane_t	*plane;
	struct mnode_s	*children[2];	

	// leaf specific
	int			cluster;
	int			area;

	struct msurface_s * *firstmarksurface;
	int			nummarksurfaces;
} mnode_t;



typedef struct world_s{
	char		name[MAX_QPATH];		// ie: maps/tim_dm2.bsp
	char		baseName[MAX_QPATH];	// ie: tim_dm2

	int			dataSize;

	int			numShaders;
	dshader_t	*shaders;

	struct bmodel_s * bmodels;

	int			numplanes;
	cplane_t	*planes;

	int			numnodes;		// includes leafs
	int			numDecisionNodes;
	mnode_t		*nodes;

	int			numsurfaces;
	struct msurface_s* surfaces;

	int			nummarksurfaces;
	struct msurface_s** marksurfaces;

	int			numfogs;
	fog_t		*fogs;

	vec3_t		lightGridOrigin;
	vec3_t		lightGridSize;
	vec3_t		lightGridInverseSize;
	int			lightGridBounds[3];
	byte		*lightGridData;


	int			numClusters;
	int			clusterBytes;
	const unsigned char	*vis;			// may be passed in by CM_LoadMap to save space

	byte		*novis;			// clusterBytes of 0xff

	char		*entityString;
	char		*entityParsePoint;
} world_t;

//======================================================================

/*
============================================================

WORLD MAP

============================================================
*/

void R_AddBrushModelSurfaces( trRefEntity_t *e );
void R_AddWorldSurfaces(viewParms_t * const pViewParams);
void R_GetWorldBaseName(char* checkname);
void SetTessFogColor(unsigned char (*pcolor)[4], int fnum, int nvert);

#endif
