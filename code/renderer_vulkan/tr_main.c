/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_main.c -- main control flow for each frame

#include "tr_globals.h"
#include "tr_cvar.h"
#include "tr_shader.h"

#include "matrix_multiplication.h"
#include "ref_import.h"

#include "R_PrintMat.h"
#include "tr_cmds.h"
#include "tr_scene.h"
#include "tr_world.h"
#include "tr_model.h"
#include "R_SortDrawSurfs.h"
#include "R_RotateForViewer.h"
#include "srfPoly_type.h"
#include "RB_DebugGraphics.h"
/*
=================
Setup that culling frustum planes for the current view
=================
*/
static void SetPlaneSignbits (cplane_t * const out)
{
	int	bits = 0;

	// for fast box on planeside test

    if (out->normal[0] < 0)
    {
        bits |= 1;
    }
    if (out->normal[1] < 0)
    {
        bits |= 2;
    }
    if (out->normal[2] < 0)
    {
        bits |= 4;
    }

	out->signbits = bits;
}


static void R_SetupFrustum (viewParms_t * const pViewParams)
{
	
    {
        float ang = pViewParams->fovX * (float)(M_PI / 360.0f);
        float xs = sin( ang );
        float xc = cos( ang );

        float temp1[3];
        float temp2[3];

        VectorScale( pViewParams->or.axis[0], xs, temp1 );
        VectorScale( pViewParams->or.axis[1], xc, temp2);

        VectorAdd(temp1, temp2, pViewParams->frustum[0].normal);
		pViewParams->frustum[0].dist = DotProduct (pViewParams->or.origin, pViewParams->frustum[0].normal);
        pViewParams->frustum[0].type = PLANE_NON_AXIAL;

        VectorSubtract(temp1, temp2, pViewParams->frustum[1].normal);
		pViewParams->frustum[1].dist = DotProduct (pViewParams->or.origin, pViewParams->frustum[1].normal);
        pViewParams->frustum[1].type = PLANE_NON_AXIAL;
    }

   
    {
        float ang = pViewParams->fovY * (float)(M_PI / 360.0f);
        float xs = sin( ang );
        float xc = cos( ang );
        float temp1[3];
        float temp2[3];

        VectorScale( pViewParams->or.axis[0], xs, temp1);
        VectorScale( pViewParams->or.axis[2], xc, temp2);

        VectorAdd(temp1, temp2, pViewParams->frustum[2].normal);
		pViewParams->frustum[2].dist = DotProduct (pViewParams->or.origin, pViewParams->frustum[2].normal);
        pViewParams->frustum[2].type = PLANE_NON_AXIAL;

        VectorSubtract(temp1, temp2, pViewParams->frustum[3].normal);
		pViewParams->frustum[3].dist = DotProduct (pViewParams->or.origin, pViewParams->frustum[3].normal);
		pViewParams->frustum[3].type = PLANE_NON_AXIAL;
    }


    uint32_t i = 0;
	for (i=0; i < 4; ++i)
    {
		// pViewParams->frustum[i].type = PLANE_NON_AXIAL;
		// pViewParams->frustum[i].dist = DotProduct (pViewParams->or.origin, pViewParams->frustum[i].normal);
		SetPlaneSignbits( &pViewParams->frustum[i] );
	}
}


/*
=================
R_SpriteFogNum

See if a sprite is inside a fog volume
=================
*/
int R_SpriteFogNum( trRefEntity_t *ent )
{
	int	i, j;
	fog_t* fog;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	for ( i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog = &tr.world->fogs[i];
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( ent->e.origin[j] - ent->e.radius >= fog->bounds[1][j] ) {
				break;
			}
			if ( ent->e.origin[j] + ent->e.radius <= fog->bounds[0][j] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}

	return 0;
}



//==========================================================================================

void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, int fogIndex, int dlightMap )
{
	// instead of checking for overflow, we just mask the index so it wraps around
	int index = tr.refdef.numDrawSurfs & DRAWSURF_MASK;
	// the sort data is packed into a single 32 bit value so it can be
	// compared quickly during the qsorting process
	tr.refdef.drawSurfs[index].sort = (shader->sortedIndex << QSORT_SHADERNUM_SHIFT) 
		| tr.shiftedEntityNum | ( fogIndex << QSORT_FOGNUM_SHIFT ) | dlightMap;
	tr.refdef.drawSurfs[index].surType = surface;
    ++tr.refdef.numDrawSurfs;
}


static void R_AddEntitySurfaces(viewParms_t * const pViewParam, const uint32_t numEntity, 
        trRefEntity_t * const pEntity)
{
    // entities that will have procedurally generated surfaces will just
    // point at this for their sorting surface
    static surfaceType_t entitySurface = SF_ENTITY;

    
	for ( tr.currentEntityNum = 0; 
	      tr.currentEntityNum < numEntity; //tr.refdef.num_entities; 
		  ++tr.currentEntityNum)
    {
        // ++tr.currentEntityNum;

		shader_t* shader;

        trRefEntity_t* ent = tr.currentEntity = &pEntity[tr.currentEntityNum];

		ent->needDlights = qfalse;

		// preshift the value we are going to OR into the drawsurf sort
		tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;

		//
		// the weapon model must be handled special --
		// we don't want the hacked weapon position showing in 
		// mirrors, because the true body position will already be drawn
		//
		if ( (ent->e.renderfx & RF_FIRST_PERSON) && pViewParam->isPortal)
        {
			continue;
		}

		// simple generated models, like sprites and beams, are not culled
		switch ( ent->e.reType )
        {
		case RT_PORTALSURFACE:
			break;		// don't draw anything
		case RT_SPRITE:
		case RT_BEAM:
		case RT_LIGHTNING:
		case RT_RAIL_CORE:
		case RT_RAIL_RINGS:
			// self blood sprites, talk balloons, etc should not be drawn in the primary
			// view.  We can't just do this check for all entities, because md3
			// entities may still want to cast shadows from them
			if ( (ent->e.renderfx & RF_THIRD_PERSON) && !pViewParam->isPortal)
            {
				continue;
			}
			shader = R_GetShaderByHandle( ent->e.customShader );
			R_AddDrawSurf( &entitySurface, shader, R_SpriteFogNum( ent ), 0 );
			break;

		case RT_MODEL:
			// we must set up parts of tr.or for model culling
			R_RotateForEntity( ent, pViewParam, &tr.or );

			tr.currentModel = R_GetModelByHandle( ent->e.hModel );
			if (!tr.currentModel)
            {
				R_AddDrawSurf( &entitySurface, tr.defaultShader, 0, 0 );
			}
            else
            {
				switch ( tr.currentModel->type )
                {
				case MOD_MESH:
					R_AddMD3Surfaces( ent );
					break;
				case MOD_MDR:
					R_MDRAddAnimSurfaces( ent );
					break;
				case MOD_IQM:
					R_AddIQMSurfaces( ent );
				case MOD_BRUSH:
					R_AddBrushModelSurfaces( ent );
					break;
				case MOD_BAD:		// null model axis
					if ( (ent->e.renderfx & RF_THIRD_PERSON) && !pViewParam->isPortal) {
						break;
					}
					shader = R_GetShaderByHandle( ent->e.customShader );
					R_AddDrawSurf( &entitySurface, tr.defaultShader, 0, 0 );
					break;
                }
			}
			break;
		default:
			ri.Error( ERR_DROP, "Add entity surfaces: Bad reType" );
		}
	}
}


static void R_SetupProjection( viewParms_t * const pViewParams, VkBool32 noWorld)
{
	float zFar;

	// set the projection matrix with the minimum zfar
	// now that we have the world bounded
	// this needs to be done before entities are added, 
    // because they use the projection matrix for lod calculation

	// dynamically compute far clip plane distance
	// if not rendering the world (icons, menus, etc), set a 2k far clip plane

	if ( noWorld )
    {
		pViewParams->zFar = zFar = 2048.0f;
	}
    else
    {
        float o[3];

        o[0] = pViewParams->or.origin[0];
        o[1] = pViewParams->or.origin[1];
        o[2] = pViewParams->or.origin[2];

        float farthestCornerDistance = 0;
        uint32_t i;

        // set far clipping planes dynamically
        for ( i = 0; i < 8; ++i )
        {
            float v[3];
     
            v[0] = ((i & 1) ? pViewParams->visBounds[0][0] : pViewParams->visBounds[1][0]) - o[0];
            v[1] = ((i & 2) ? pViewParams->visBounds[0][1] : pViewParams->visBounds[1][1]) - o[1];
            v[2] = ((i & 4) ? pViewParams->visBounds[0][2] : pViewParams->visBounds[1][2]) - o[0];

            float distance = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
            
            if( distance > farthestCornerDistance )
            {
                farthestCornerDistance = distance;
            }
        }
        
        pViewParams->zFar = zFar = sqrtf(farthestCornerDistance);
    }
	
	// set up projection matrix
	// update q3's proj matrix (opengl) to vulkan conventions: z - [0, 1] instead of [-1, 1] and invert y direction
    
    // Vulkan clip space has inverted Y and half Z.	
    float zNear	= r_znear->value;
	float p10 = -zFar / (zFar - zNear);

    float py = tan(pViewParams->fovY * (M_PI / 360.0f));
    float px = tan(pViewParams->fovX * (M_PI / 360.0f));

	pViewParams->projectionMatrix[0] = 1.0f / px;
	pViewParams->projectionMatrix[1] = 0.0f;
	pViewParams->projectionMatrix[2] = 0.0f;
	pViewParams->projectionMatrix[3] = 0.0f;
    
    pViewParams->projectionMatrix[4] = 0.0f;
	pViewParams->projectionMatrix[5] = -1.0f / py;
	pViewParams->projectionMatrix[6] = 0.0f;
	pViewParams->projectionMatrix[7] = 0.0f;

    pViewParams->projectionMatrix[8] = 0.0f;	// normally 0
	pViewParams->projectionMatrix[9] =  0.0f;
	pViewParams->projectionMatrix[10] = p10;
	pViewParams->projectionMatrix[11] = -1.0f;

    pViewParams->projectionMatrix[12] = 0.0f;
	pViewParams->projectionMatrix[13] = 0.0f;
	pViewParams->projectionMatrix[14] = zNear * p10;
	pViewParams->projectionMatrix[15] = 0.0f;
}


/*
=====================
Adds all the scene's polys into this view's drawsurf list
=====================
*/
static void R_AddPolygonSurfaces(const srfPoly_t* pPoly, const uint32_t numPolys )
{
	tr.currentEntityNum = REFENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;

    uint32_t i;

	for(i = 0; i < numPolys; ++i)
    {
		shader_t* sh = R_GetShaderByHandle( pPoly->hShader );
		R_AddDrawSurf( (surfaceType_t*) pPoly, sh, pPoly->fogIndex, qfalse );
        ++pPoly;
	}
}

/*
================
R_RenderView

A view may be either the actual camera view,
or a mirror / remote location
================
*/
void R_RenderView (viewParms_t *parms)
{
	int	firstDrawSurf = tr.refdef.numDrawSurfs;

	++tr.viewCount;

	tr.viewParms = *parms;

	// tr.viewCount++;

	// set viewParms.world
	R_RotateForViewer (&tr.viewParms, &tr.or);
    // Setup that culling frustum planes for the current view
	R_SetupFrustum (&tr.viewParms);

	R_AddWorldSurfaces (&tr.viewParms);

	R_AddPolygonSurfaces(tr.refdef.polys, tr.refdef.numPolys);

	// set the projection matrix with the minimum zfar
	// now that we have the world bounded
	// this needs to be done before entities are
	// added, because they use the projection matrix for LOD calculation
    //
        
	R_SetupProjection (&tr.viewParms, tr.refdef.rdflags & RDF_NOWORLDMODEL);

    if ( r_drawentities->integer ) {
	    R_AddEntitySurfaces (&tr.viewParms, tr.refdef.num_entities, tr.refdef.entities);
	}



	R_SortDrawSurfs( tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf );

    if ( r_debugSurface->integer )
    {
        // draw main system development information (surface outlines, etc)
		RB_DebugGraphics();
	}
}


/*
@@@@@@@@@@@@@@@@@@@@@
RE_RenderScene

Draw a 3D view into a part of the window, then return
to 2D drawing.

Rendering a scene may require multiple views to be rendered
to handle mirrors,
@@@@@@@@@@@@@@@@@@@@@
*/
void RE_RenderScene( const refdef_t *fd )
{
	if ( !tr.registered ) {
		return;
	}

	int startTime = ri.Milliseconds();
	qboolean customscrn = !(fd->rdflags & RDF_NOWORLDMODEL);

	memcpy( tr.refdef.text, fd->text, sizeof( tr.refdef.text ) );

	tr.refdef.x = fd->x;
	tr.refdef.y = fd->y;
	tr.refdef.width = fd->width;
	tr.refdef.height = fd->height;
	tr.refdef.fov_x = fd->fov_x;
	tr.refdef.fov_y = fd->fov_y;

	VectorCopy( fd->vieworg, tr.refdef.vieworg );
	VectorCopy( fd->viewaxis[0], tr.refdef.viewaxis[0] );
	VectorCopy( fd->viewaxis[1], tr.refdef.viewaxis[1] );
	VectorCopy( fd->viewaxis[2], tr.refdef.viewaxis[2] );

	tr.refdef.time = fd->time;
	tr.refdef.rdflags = fd->rdflags;
    
    
	tr.refdef.AreamaskModified = qfalse;
	
    if ( customscrn )
    {
		int	areaDiff = 0;
		int		i;

		// compare the area bits
		for (i = 0 ; i < MAX_MAP_AREA_BYTES; ++i)
		{
			areaDiff |= tr.refdef.areamask[i] ^ fd->areamask[i];
			tr.refdef.areamask[i] = fd->areamask[i];
		}

		if ( areaDiff ) {
            
            // ri.Printf(PRINT_ALL, "%d\n", i);

			// a door just opened or something
			tr.refdef.AreamaskModified = qtrue;
		}
	}



    R_SceneSetRefDef();
    
    // ri.Printf(PRINT_ALL, "(%d, %d, %d, %d)\n", tr.refdef.x, tr.refdef.y, tr.refdef.width, tr.refdef.height);
	// setup view parms for the initial view
	//
	// set up viewport
	// The refdef takes 0-at-the-top y coordinates
    // 0 +-------> x
    //   |
    //   |
    //   |
    //   y
    struct viewParms_s parms;
	memset( &parms, 0, sizeof( parms ) );
	// parms.isPortal = qfalse;

    parms.viewportX = fd->x;
	parms.viewportY =  fd->y;

    parms.viewportWidth = fd->width;
	parms.viewportHeight = fd->height;

	//parms.fovX = fd->fov_x;
	//parms.fovY = fd->fov_y;


    parms.fovX = tr.refdef.fov_x;
	parms.fovY = tr.refdef.fov_y;

	VectorCopy( fd->vieworg, parms.or.origin );
	VectorCopy( fd->viewaxis[0], parms.or.axis[0] );
	VectorCopy( fd->viewaxis[1], parms.or.axis[1] );
	VectorCopy( fd->viewaxis[2], parms.or.axis[2] );
	VectorCopy( fd->vieworg, parms.pvsOrigin );

    // Mat3x3Copy(parms.or.axis, fd->viewaxis);

	if ( (parms.viewportWidth > 0) && (parms.viewportHeight > 0) ) 
    {
		R_RenderView( &parms );
	}


	// the next scene rendered in this frame will tack on after this one
    R_TheNextScene();

	tr.frontEndMsec += ri.Milliseconds() - startTime;
}

/*
typedef struct {
/	orientationr_t	or;
	orientationr_t	world;
//	vec3_t		pvsOrigin;			// may be different than or.origin for portals
//	qboolean	isPortal;			// true if this view is through a portal
	qboolean	isMirror;			// the portal is a mirror, invert the face culling
	cplane_t	portalPlane;		// clip anything behind this if mirroring
//	int			viewportX, viewportY, viewportWidth, viewportHeight;
//	float		fovX, fovY;
	float		projectionMatrix[16] QALIGN(16);
	cplane_t	frustum[4];
	vec3_t		visBounds[2];
	float		zFar;
} viewParms_t;
*/
