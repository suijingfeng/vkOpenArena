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
#include "tr_local.h"


/*
=================
R_CullTriSurf

Returns true if the grid is completely culled away.
Also sets the clipped hint bit in tess
=================
*/
static qboolean	R_CullTriSurf( srfTriangles_t *cv )
{
	int boxCull = R_CullLocalBox( cv->bounds );

	if ( boxCull == CULL_OUT )
    {
		return qtrue;
	}
	return qfalse;
}


/*
=================
R_CullGrid

Returns true if the grid is completely culled away.
Also sets the clipped hint bit in tess
=================
*/
static qboolean	R_CullGrid( srfGridMesh_t *cv )
{
	int sphereCull;

	if ( tr.currentEntityNum != REFENTITYNUM_WORLD )
		sphereCull = R_CullLocalPointAndRadius( cv->localOrigin, cv->meshRadius );
    else
		sphereCull = R_CullPointAndRadius( cv->localOrigin, cv->meshRadius );

	
	// check for trivial reject
	if( sphereCull == CULL_OUT )
	{
		tr.pc.c_sphere_cull_patch_out++;
		return qtrue;
	}
	else if ( sphereCull == CULL_CLIP )
	{ 	// check bounding box if necessary
		tr.pc.c_sphere_cull_patch_clip++;

		int boxCull = R_CullLocalBox( cv->meshBounds );

		if ( boxCull == CULL_OUT ) 
		{
			tr.pc.c_box_cull_patch_out++;
			return qtrue;
		}
		else if ( boxCull == CULL_IN )
		{
			tr.pc.c_box_cull_patch_in++;
		}
		else
		{
			tr.pc.c_box_cull_patch_clip++;
		}
	}
	else
	{
		tr.pc.c_sphere_cull_patch_in++;
	}

	return qfalse;
}


/*
================
R_CullSurface

Tries to back face cull surfaces before they are lighted or added to the sorting list.

This will also allow mirrors on both sides of a model without recursion.
================
*/
static qboolean	R_CullSurface( surfaceType_t *surface, shader_t *shader )
{
	srfSurfaceFace_t *sface;

	if ( r_nocull->integer ) {
		return qfalse;
	}

	if ( *surface == SF_GRID ) {
		return R_CullGrid( (srfGridMesh_t *)surface );
	}

	if ( *surface == SF_TRIANGLES ) {
		return R_CullTriSurf( (srfTriangles_t *)surface );
	}

	if ( *surface != SF_FACE ) {
		return qfalse;
	}

	if ( shader->cullType == CT_TWO_SIDED ) {
		return qfalse;
	}

	// face culling
	if ( !r_facePlaneCull->integer ) {
		return qfalse;
	}

	sface = ( srfSurfaceFace_t * ) surface;
	float d = DotProduct (tr.or.viewOrigin, sface->plane.normal);

	// don't cull exactly on the plane, because there are levels of rounding
	// through the BSP, ICD, and hardware that may cause pixel gaps if an
	// epsilon isn't allowed here 
	if ( shader->cullType == CT_FRONT_SIDED )
    {
		if ( d < sface->plane.dist - 8 ) {
			return qtrue;
		}
	}
    else
    {
		if ( d > sface->plane.dist + 8 ) {
			return qtrue;
		}
	}

	return qfalse;
}


static int R_DlightFace( srfSurfaceFace_t *face, int dlightBits )
{
	int	i;

	for( i = 0 ; i < tr.refdef.num_dlights ; i++ )
    {
		if ( ! ( dlightBits & ( 1 << i ) ) )
        {
			continue;
		}

		dlight_t* dl = &tr.refdef.dlights[i];
		float d = DotProduct( dl->origin, face->plane.normal ) - face->plane.dist;
		
        if ( d < -dl->radius || d > dl->radius )
        {
			// dlight doesn't reach the plane
			dlightBits &= ~( 1 << i );
		}
	}

	if ( !dlightBits ) {
		tr.pc.c_dlightSurfacesCulled++;
	}

	face->dlightBits = dlightBits;
	return dlightBits;
}


static int R_DlightGrid( srfGridMesh_t *grid, int dlightBits )
{
	int	i;
	for ( i = 0 ; i < tr.refdef.num_dlights ; i++ )
    {
		if ( ! ( dlightBits & ( 1 << i ) ) ) {
			continue;
		}
		dlight_t* dl = &tr.refdef.dlights[i];

		if ( dl->origin[0] - dl->radius > grid->meshBounds[1][0]
			|| dl->origin[0] + dl->radius < grid->meshBounds[0][0]
			|| dl->origin[1] - dl->radius > grid->meshBounds[1][1]
			|| dl->origin[1] + dl->radius < grid->meshBounds[0][1]
			|| dl->origin[2] - dl->radius > grid->meshBounds[1][2]
			|| dl->origin[2] + dl->radius < grid->meshBounds[0][2] )
        {
			// dlight doesn't reach the bounds
			dlightBits &= ~( 1 << i );
		}
	}

	if ( !dlightBits ) {
		tr.pc.c_dlightSurfacesCulled++;
	}

	grid->dlightBits = dlightBits;
	return dlightBits;
}



static int R_DlightTrisurf( srfTriangles_t *surf, int dlightBits )
{
	// FIXME: more dlight culling to trisurfs...
	surf->dlightBits = dlightBits;
	return dlightBits;
#if 0
	int			i;
	dlight_t	*dl;

	for ( i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
		if ( ! ( dlightBits & ( 1 << i ) ) ) {
			continue;
		}
		dl = &tr.refdef.dlights[i];
		if ( dl->origin[0] - dl->radius > grid->meshBounds[1][0]
			|| dl->origin[0] + dl->radius < grid->meshBounds[0][0]
			|| dl->origin[1] - dl->radius > grid->meshBounds[1][1]
			|| dl->origin[1] + dl->radius < grid->meshBounds[0][1]
			|| dl->origin[2] - dl->radius > grid->meshBounds[1][2]
			|| dl->origin[2] + dl->radius < grid->meshBounds[0][2] ) {
			// dlight doesn't reach the bounds
			dlightBits &= ~( 1 << i );
		}
	}

	if ( !dlightBits ) {
		tr.pc.c_dlightSurfacesCulled++;
	}

	grid->dlightBits = dlightBits;
	return dlightBits;
#endif
}


/*
====================
R_DlightSurface

The given surface is going to be drawn, 
and it touches a leaf that is touched by one or more dlights, 
so try to throw out more dlights if possible.
====================
*/

static int R_DlightSurface( msurface_t *surf, int dlightBits )
{
	if ( *surf->data == SF_FACE )
    {
		dlightBits = R_DlightFace( (srfSurfaceFace_t *)surf->data, dlightBits );
	}
    else if ( *surf->data == SF_GRID )
    {
		dlightBits = R_DlightGrid( (srfGridMesh_t *)surf->data, dlightBits );
	}
    else if ( *surf->data == SF_TRIANGLES )
    {
		dlightBits = R_DlightTrisurf( (srfTriangles_t *)surf->data, dlightBits );
	}
    else
    {
		dlightBits = 0;
	}

	if ( dlightBits )
    {
		tr.pc.c_dlightSurfaces++;
	}

	return dlightBits;
}



/*
======================
R_AddWorldSurface
======================
*/
static void R_AddWorldSurface( msurface_t *surf, int dlightBits )
{
	if ( surf->viewCount == tr.viewCount ) {
		return;		// already in this view
	}

	surf->viewCount = tr.viewCount;
	// FIXME: bmodel fog?

	// try to cull before dlighting or adding
	if ( R_CullSurface( surf->data, surf->shader ) ) {
		return;
	}

	// check for dlighting
	if ( dlightBits ) {
		dlightBits = R_DlightSurface( surf, dlightBits );
		dlightBits = ( dlightBits != 0 );
	}

	R_AddDrawSurf( surf->data, surf->shader, surf->fogIndex, dlightBits );
}



/*
=============================================================

	WORLD MODEL

=============================================================
*/


static void R_RecursiveWorldNode( mnode_t *node, int planeBits, int dlightBits )
{

	do {

		// if the node wasn't marked as potentially visible, exit
		if (node->visframe != tr.visCount)
        {
			return;
		}

		// if the bounding volume is outside the frustum,
        // nothing inside can be visible OPTIMIZE: don't do this all the way to leafs?

		if ( !r_nocull->integer )
        {
			int	r;

			if ( planeBits & 1 )
            {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[0]);
				if (r == 2)
					return;						// culled
				else if ( r == 1 )
					planeBits &= ~1;			// all descendants will also be in front
			}

			if ( planeBits & 2 )
            {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[1]);
				if (r == 2)
					return;						// culled
                else if ( r == 1 )
                {
					planeBits &= ~2;			// all descendants will also be in front
				}
			}

			if ( planeBits & 4 ) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[2]);
				if (r == 2)
					return;						// culled
                else if ( r == 1 )
					planeBits &= ~4;			// all descendants will also be in front
			}

			if ( planeBits & 8 )
            {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[3]);
				if (r == 2)
					return;						// culled
                else if ( r == 1 ) {
					planeBits &= ~8;			// all descendants will also be in front
				}
			}
		}

		if ( node->contents != -1 ) {
			break;
		}

		// node is just a decision point, so go down both sides
		// since we don't care about sort orders, just go positive to negative

		// determine which dlights are needed
        int	newDlights[2]={0, 0};

		if ( dlightBits )
        {
			int	i;

			for ( i = 0 ; i < tr.refdef.num_dlights ; i++ )
            {
				if ( dlightBits & ( 1 << i ) )
                {
					dlight_t* dl = &tr.refdef.dlights[i];
					float dist = DotProduct( dl->origin, node->plane->normal ) - node->plane->dist;
					
					if ( dist > -dl->radius )
                    {
						newDlights[0] |= ( 1 << i );
					}
					if ( dist < dl->radius )
                    {
						newDlights[1] |= ( 1 << i );
					}
				}
			}
		}

		// recurse down the children, front side first
		R_RecursiveWorldNode (node->children[0], planeBits, newDlights[0] );

		// tail recurse
		node = node->children[1];
		dlightBits = newDlights[1];
	}while ( 1 );



    // leaf node, so add mark surfaces
    tr.pc.c_leafs++;

    // add to z buffer bounds
    if ( node->mins[0] < tr.viewParms.visBounds[0][0] ) {
        tr.viewParms.visBounds[0][0] = node->mins[0];
    }
    if ( node->mins[1] < tr.viewParms.visBounds[0][1] ) {
        tr.viewParms.visBounds[0][1] = node->mins[1];
    }
    if ( node->mins[2] < tr.viewParms.visBounds[0][2] ) {
        tr.viewParms.visBounds[0][2] = node->mins[2];
    }

    if ( node->maxs[0] > tr.viewParms.visBounds[1][0] ) {
        tr.viewParms.visBounds[1][0] = node->maxs[0];
    }
    if ( node->maxs[1] > tr.viewParms.visBounds[1][1] ) {
        tr.viewParms.visBounds[1][1] = node->maxs[1];
    }
    if ( node->maxs[2] > tr.viewParms.visBounds[1][2] ) {
        tr.viewParms.visBounds[1][2] = node->maxs[2];
    }

    // add the individual surfaces
    msurface_t** mark = node->firstmarksurface;
    int c = node->nummarksurfaces;
    while (c--)
    {
        // the surface may have already been added if it
        // spans multiple leafs
        msurface_t* surf = *mark;
        R_AddWorldSurface( surf, dlightBits );
        mark++;
    }

}


/*
===============
R_PointInLeaf
===============
*/
static mnode_t *R_PointInLeaf( const vec3_t p )
{
	if ( !tr.world ) {
		ri.Error(ERR_DROP, "R_PointInLeaf: bad model");
	}

	mnode_t* node = tr.world->nodes;
	while( 1 )
    {
		if (node->contents != -1) {
			break;
		}
		cplane_t* plane = node->plane;
		float d = DotProduct(p,plane->normal) - plane->dist;
		
        if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}
	
	return node;
}



static const unsigned char *R_ClusterPVS (int cluster)
{
	if (!tr.world->vis || cluster < 0 || cluster >= tr.world->numClusters )
    {
		return tr.world->novis;
	}

	return tr.world->vis + cluster * tr.world->clusterBytes;
}




/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current cluster
===============
*/
static void R_MarkLeaves (void)
{
	int	i;

	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if( r_lockpvs->integer ) {
		return;
	}

	// current viewcluster
	mnode_t* leaf = R_PointInLeaf( tr.viewParms.pvsOrigin );
	int cluster = leaf->cluster;

	// if the cluster is the same and the area visibility matrix
	// hasn't changed, we don't need to mark everything again

	// if r_showcluster was just turned on, remark everything 
	if ( tr.viewCluster == cluster && !tr.refdef.areamaskModified && !r_showcluster->modified )
    {
		return;
	}

	if ( r_showcluster->modified || r_showcluster->integer )
    {
		r_showcluster->modified = qfalse;
		if ( r_showcluster->integer ) {
			ri.Printf( PRINT_ALL, "cluster:%i  area:%i\n", cluster, leaf->area );
		}
	}

	tr.visCount++;
	tr.viewCluster = cluster;

	if( r_novis->integer || tr.viewCluster == -1 )
    {
		for (i=0 ; i<tr.world->numnodes ; i++)
        {
			if (tr.world->nodes[i].contents != CONTENTS_SOLID)
            {
				tr.world->nodes[i].visframe = tr.visCount;
			}
		}
		return;
	}

	const unsigned char	*vis = R_ClusterPVS (tr.viewCluster);
	
	for (i=0,leaf=tr.world->nodes ; i<tr.world->numnodes ; i++, leaf++)
    {
		cluster = leaf->cluster;
		if ( cluster < 0 || cluster >= tr.world->numClusters )
        {
			continue;
		}

		// check general pvs
		if ( !(vis[cluster>>3] & (1<<(cluster&7))) )
        {
			continue;
		}

		// check for door connection
		if( (tr.refdef.areamask[leaf->area>>3] & (1<<(leaf->area&7)) ) )
        {
			continue;		// not visible
		}

		mnode_t* parent = leaf;
		do {
			if (parent->visframe == tr.visCount)
				break;
			parent->visframe = tr.visCount;
			parent = parent->parent;
		} while (parent);
	}
}





/*
=============================================================
	BRUSH MODELS
=============================================================
*/

void R_AddBrushModelSurfaces ( trRefEntity_t *ent )
{
	model_t* pModel = R_GetModelByHandle( ent->e.hModel );
	bmodel_t* bmodel = pModel->bmodel;

	if ( R_CullLocalBox( bmodel->bounds ) == CULL_OUT )
		return;

	R_SetupEntityLighting( &tr.refdef, ent );
	R_DlightBmodel( bmodel );

    int i;
	for( i = 0 ; i < bmodel->numSurfaces ; i++ )
    {
		R_AddWorldSurface( bmodel->firstSurface + i, tr.currentEntity->needDlights );
	}
}



qboolean R_inPVS( const vec3_t p1, const vec3_t p2 )
{
	mnode_t* leaf= R_PointInLeaf( p1 );
	unsigned char* vis= ri.CM_ClusterPVS( leaf->cluster ); // why not R_ClusterPVS ??

    leaf = R_PointInLeaf( p2 );

	if( !(vis[leaf->cluster>>3] & (1<<(leaf->cluster&7))) )
    {
		return qfalse;
	}
	return qtrue;
}


void R_AddWorldSurfaces (void)
{
	tr.currentEntityNum = REFENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;

	// determine which leaves are in the PVS / areamask
	R_MarkLeaves();

	// clear out the visible min/max
	ClearBounds( tr.viewParms.visBounds[0], tr.viewParms.visBounds[1] );

	// perform frustum culling and add all the potentially visible surfaces
	if ( tr.refdef.num_dlights > 32 ) {
		tr.refdef.num_dlights = 32 ;
	}
	R_RecursiveWorldNode( tr.world->nodes, 15, ( 1 << tr.refdef.num_dlights ) - 1 );
}
