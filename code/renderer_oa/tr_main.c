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

#include "tr_local.h"


#define	DRAWSURF_MASK			(MAX_DRAWSURFS-1)
///////// globals ///////////


extern refimport_t ri;
extern cvar_t* r_fastsky;

trGlobals_t	tr;
shaderCommands_t tess;

static cvar_t* r_debugSurface;
static cvar_t* r_noportals;
static cvar_t* r_drawentities;		// disable/enable entity rendering
static cvar_t* r_znear; // near Z clip plane

static cvar_t* r_zproj; // z distance of projection plane


//////// statics ///////////

static const float s_flipMatrix[16] =
{
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};

// entities that will have procedurally generated surfaces will just point at this for their sorting surface
static surfaceType_t entitySurface = SF_ENTITY;



/*
=================
R_RotateForViewer

Sets up the modelview matrix for a given viewParm
=================
*/
static void R_RotateForViewer(void) 
{
	float	viewerMatrix[16];
	vec3_t	origin;

	memset(&tr.or, 0, sizeof(tr.or));
	tr.or.axis[0][0] = 1;
	tr.or.axis[1][1] = 1;
	tr.or.axis[2][2] = 1;
	VectorCopy(tr.viewParms.or.origin, tr.or.viewOrigin);

	// transform by the camera placement
	VectorCopy(tr.viewParms.or.origin, origin );

	viewerMatrix[0] = tr.viewParms.or.axis[0][0];
	viewerMatrix[4] = tr.viewParms.or.axis[0][1];
	viewerMatrix[8] = tr.viewParms.or.axis[0][2];
	viewerMatrix[12] = -origin[0] * viewerMatrix[0] -origin[1] * viewerMatrix[4] -origin[2] * viewerMatrix[8];

	viewerMatrix[1] = tr.viewParms.or.axis[1][0];
	viewerMatrix[5] = tr.viewParms.or.axis[1][1];
	viewerMatrix[9] = tr.viewParms.or.axis[1][2];
	viewerMatrix[13] = -origin[0] * viewerMatrix[1] -origin[1] * viewerMatrix[5] -origin[2] * viewerMatrix[9];

	viewerMatrix[2] = tr.viewParms.or.axis[2][0];
	viewerMatrix[6] = tr.viewParms.or.axis[2][1];
	viewerMatrix[10] = tr.viewParms.or.axis[2][2];
	viewerMatrix[14] = -origin[0] * viewerMatrix[2] -origin[1] * viewerMatrix[6] -origin[2] * viewerMatrix[10];

	viewerMatrix[3] = 0;
	viewerMatrix[7] = 0;
	viewerMatrix[11] = 0;
	viewerMatrix[15] = 1;

	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	MatrixMultiply4x4( viewerMatrix, s_flipMatrix, tr.or.modelMatrix );

	tr.viewParms.world = tr.or;
}


static void R_SetFarClip( void )
{
	float farthestCornerDistance = 0;
	int	i;

	// if not rendering the world (icons, menus, etc)
	// set a 2k far clip plane
	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		tr.viewParms.zFar = 2048;
		return;
	}

    
	// set far clipping planes dynamically
	for ( i = 0; i < 8; i++ )
	{
		vec3_t v;

		if ( i & 1 )
		{
			v[0] = tr.viewParms.visBounds[0][0];
		}
		else
		{
			v[0] = tr.viewParms.visBounds[1][0];
		}

		if ( i & 2 )
		{
			v[1] = tr.viewParms.visBounds[0][1];
		}
		else
		{
			v[1] = tr.viewParms.visBounds[1][1];
		}

		if ( i & 4 )
		{
			v[2] = tr.viewParms.visBounds[0][2];
		}
		else
		{
			v[2] = tr.viewParms.visBounds[1][2];
		}

		VectorSubtract( v, tr.viewParms.or.origin, v );

		float distance = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];

		if( distance > farthestCornerDistance )
		{
			farthestCornerDistance = distance;
		}
	}
	tr.viewParms.zFar = sqrt( farthestCornerDistance );
}


/*
=================
R_SetupFrustum

Set up the culling frustum planes for the current view using the results
we got from computing the first two rows of the projection matrix.
=================
*/
static void R_SetupFrustum(viewParms_t *dest, float xmin, float xmax, float ymax, float zProj)
{
	vec3_t ofsorigin;
	float oppleg, adjleg, length;
	int i;
	
	if( xmin == -xmax)
	{
		// symmetric case can be simplified
		VectorCopy(dest->or.origin, ofsorigin);

		length = sqrt(xmax * xmax + zProj * zProj);
		oppleg = xmax / length;
		adjleg = zProj / length;

		VectorScale(dest->or.axis[0], oppleg, dest->frustum[0].normal);
		VectorMA(dest->frustum[0].normal, adjleg, dest->or.axis[1], dest->frustum[0].normal);

		VectorScale(dest->or.axis[0], oppleg, dest->frustum[1].normal);
		VectorMA(dest->frustum[1].normal, -adjleg, dest->or.axis[1], dest->frustum[1].normal);
	}


	length = sqrt(ymax * ymax + zProj * zProj);
	oppleg = ymax / length;
	adjleg = zProj / length;

	VectorScale(dest->or.axis[0], oppleg, dest->frustum[2].normal);
	VectorMA(dest->frustum[2].normal, adjleg, dest->or.axis[2], dest->frustum[2].normal);

	VectorScale(dest->or.axis[0], oppleg, dest->frustum[3].normal);
	VectorMA(dest->frustum[3].normal, -adjleg, dest->or.axis[2], dest->frustum[3].normal);
	
	for (i=0 ; i<4 ; i++)
    {
		dest->frustum[i].type = PLANE_NON_AXIAL;
		dest->frustum[i].dist = DotProduct (ofsorigin, dest->frustum[i].normal);
		SetPlaneSignbits( &dest->frustum[i] );
	}
}


/*
===============
R_SetupProjectionZ

Sets the z-component transformation part in the projection matrix
===============
*/
static void R_SetupProjectionZ(viewParms_t *dest)
{
	float zNear	= r_znear->value;
	float zFar	= dest->zFar;	
	float depth	= zFar - zNear;

	dest->projectionMatrix[2] = 0;
	dest->projectionMatrix[6] = 0;
	dest->projectionMatrix[10] = -( zFar + zNear ) / depth;
	dest->projectionMatrix[14] = -2 * zFar * zNear / depth;
}

/*
=================
R_MirrorPoint
=================
*/
static void R_MirrorPoint(vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out)
{
	int		i;
	vec3_t	local;
	vec3_t	transformed;
	float	d;

	VectorSubtract( in, surface->origin, local );

	VectorClear( transformed );
	for( i = 0 ; i < 3 ; i++ )
    {
		d = DotProduct(local, surface->axis[i]);
		VectorMA( transformed, d, camera->axis[i], transformed );
	}

	VectorAdd( transformed, camera->origin, out );
}


static void R_MirrorVector(vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out)
{
	int	i;

    VectorClear( out );
	for ( i = 0 ; i < 3 ; i++ )
    {
		float d = DotProduct(in, surface->axis[i]);
		VectorMA( out, d, camera->axis[i], out );
	}
}

/*
=====================
PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points
=====================
*/

static void PlaneFromPoints(const float* a, const float* b, const float* c, float* plane, float* dis)
{
	float d1[3], d2[3];

	//VectorSubtract( b, a, d1 );
    d1[0] = b[0] - a[0];
    d1[1] = b[1] - a[1];
    d1[2] = b[2] - a[2];
    
    //VectorSubtract( c, a, d2 );
    d2[0] = c[0] - a[0];
    d2[1] = c[1] - a[1];
    d2[2] = c[2] - a[2];

	CrossProduct( d2, d1, plane );
   
    // VectorNormalize( plane )
	float invLen = plane[0]*plane[0] + plane[1]*plane[1] + plane[2]*plane[2];

	if(invLen == 0)
		return;

    invLen = 1.0f / sqrtf(invLen);

    plane[0] *= invLen;
    plane[1] *= invLen;
    plane[2] *= invLen;
    *dis = a[0]*plane[0] + a[1]*plane[1] + a[2]*plane[2];
}



static void R_PlaneForSurface(surfaceType_t *surfType, cplane_t *plane)
{
	srfTriangles_t	*tri;
	srfPoly_t		*poly;
	drawVert_t		*v1, *v2, *v3;

	if (!surfType)
    {
		memset (plane, 0, sizeof(*plane));
		plane->normal[0] = 1;
		return;
	}
	switch (*surfType)
    {
        case SF_FACE:
            *plane = ((srfSurfaceFace_t *)surfType)->plane;
            return;
        case SF_TRIANGLES:
            tri = (srfTriangles_t *)surfType;
            v1 = tri->verts + tri->indexes[0];
            v2 = tri->verts + tri->indexes[1];
            v3 = tri->verts + tri->indexes[2];

            PlaneFromPoints( v1->xyz, v2->xyz, v3->xyz, plane->normal, &plane->dist);
            return;
        case SF_POLY:
            poly = (srfPoly_t *)surfType;
            PlaneFromPoints( poly->verts[0].xyz, poly->verts[1].xyz, poly->verts[2].xyz, plane->normal, &plane->dist);
            return;
        default:
            memset (plane, 0, sizeof(*plane));
            plane->normal[0] = 1;		
            return;
	}
}

/*
=================
R_GetPortalOrientation

entityNum is the entity that the portal surface is a part of,
which may be moving and rotating.

Returns qtrue if it should be mirrored
=================
*/


static qboolean R_GetPortalOrientations( drawSurf_t *drawSurf, int entityNum, orientation_t *surface, orientation_t *camera, vec3_t pvsOrigin, qboolean *mirror )
{
	int			i;
	cplane_t	originalPlane, plane;
	trRefEntity_t	*e;
	float		d;
	vec3_t		transformed;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( drawSurf->surface, &originalPlane );

	// rotate the plane if necessary
	if ( entityNum != REFENTITYNUM_WORLD )
    {
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[entityNum];

		// get the orientation of the entity
		R_RotateForEntity( tr.currentEntity, &tr.viewParms, &tr.or );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld( originalPlane.normal, plane.normal );
		plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.or.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.or.origin );
	}
    else
    {
		plane = originalPlane;
	}

	VectorCopy( plane.normal, surface->axis[0] );
    MakeNormalVectors( plane.normal, surface->axis[1], surface->axis[2]);
    
    //VectorPerp( surface->axis[1], surface->axis[0] );
	//CrossProduct( surface->axis[0], surface->axis[1], surface->axis[2] );



	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, 
    // origin2 will be the origin of the camera
	for ( i = 0 ; i < tr.refdef.num_entities ; i++ )
    {
		e = &tr.refdef.entities[i];
		if ( e->e.reType != RT_PORTALSURFACE ) {
			continue;
		}

		d = DotProduct( e->e.origin, originalPlane.normal ) - originalPlane.dist;
		if ( d > 64 || d < -64) {
			continue;
		}

		// get the pvsOrigin from the entity
		VectorCopy( e->e.oldorigin, pvsOrigin );

		// if the entity is just a mirror, don't use as a camera point
		if ( e->e.oldorigin[0] == e->e.origin[0] && 
			e->e.oldorigin[1] == e->e.origin[1] && 
			e->e.oldorigin[2] == e->e.origin[2] ) {
			VectorScale( plane.normal, plane.dist, surface->origin );
			VectorCopy( surface->origin, camera->origin );
			VectorSubtract( vec3_origin, surface->axis[0], camera->axis[0] );
			VectorCopy( surface->axis[1], camera->axis[1] );
			VectorCopy( surface->axis[2], camera->axis[2] );

			*mirror = qtrue;
			return qtrue;
		}

		// project the origin onto the surface plane to get
		// an origin point we can rotate around
		d = DotProduct( e->e.origin, plane.normal ) - plane.dist;
		VectorMA( e->e.origin, -d, surface->axis[0], surface->origin );
			
		// now get the camera origin and orientation
		VectorCopy( e->e.oldorigin, camera->origin );
		AxisCopy( e->e.axis, camera->axis );
		VectorSubtract( vec3_origin, camera->axis[0], camera->axis[0] );
		VectorSubtract( vec3_origin, camera->axis[1], camera->axis[1] );

		// optionally rotate
		if ( e->e.oldframe )
        {
			// if a speed is specified
			if ( e->e.frame )
            {
				// continuous rotate
				d = (tr.refdef.time/1000.0f) * e->e.frame;
				VectorCopy( camera->axis[1], transformed );
				//RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
                PointRotateAroundVector(transformed, camera->axis[0], d, camera->axis[1]);

				CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
			} 
            else
            {
				// bobbing rotate, with skinNum being the rotation offset
				d = sin( tr.refdef.time * 0.003f );
				d = e->e.skinNum + d * 4;
				VectorCopy( camera->axis[1], transformed );
				//RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
				PointRotateAroundVector(transformed, camera->axis[0], d, camera->axis[1]);

                CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
			}
		}
		else if ( e->e.skinNum )
        {
			d = e->e.skinNum;
			VectorCopy( camera->axis[1], transformed );
			//RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
			PointRotateAroundVector(transformed, camera->axis[0], d, camera->axis[1]);

            CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
		}
		*mirror = qfalse;
		return qtrue;
	}

	// if we didn't locate a portal entity, don't render anything.
	// We don't want to just treat it as a mirror, because without a
	// portal entity the server won't have communicated a proper entity set in the snapshot

	// unfortunately, with local movement prediction it is easily possible
	// to see a surface before the server has communicated the matching
	// portal surface entity, so we don't want to print anything here...

	//ri.Printf( PRINT_ALL, "Portal surface without a portal entity\n" );

	return qfalse;
}

static qboolean IsMirror( const drawSurf_t *drawSurf, int entityNum )
{
	int			i;
	cplane_t	originalPlane, plane;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( drawSurf->surface, &originalPlane );

	// rotate the plane if necessary
	if ( entityNum != REFENTITYNUM_WORLD )
	{
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[entityNum];

		// get the orientation of the entity
		R_RotateForEntity( tr.currentEntity, &tr.viewParms, &tr.or );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld( originalPlane.normal, plane.normal );
		plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.or.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.or.origin );
	} 

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for ( i = 0 ; i < tr.refdef.num_entities ; i++ ) 
	{
		trRefEntity_t* e = &tr.refdef.entities[i];
		if ( e->e.reType != RT_PORTALSURFACE ) {
			continue;
		}

		float d = DotProduct( e->e.origin, originalPlane.normal ) - originalPlane.dist;
		if ( d > 64 || d < -64) {
			continue;
		}

		// if the entity is just a mirror, don't use as a camera point
		if ( e->e.oldorigin[0] == e->e.origin[0] && e->e.oldorigin[1] == e->e.origin[1] && e->e.oldorigin[2] == e->e.origin[2] ) 
		{
			return qtrue;
		}

		return qfalse;
	}
	return qfalse;
}

/*
** SurfIsOffscreen
**
** Determines if a surface is completely offscreen.
*/
static qboolean SurfIsOffscreen( const drawSurf_t *drawSurf, vec4_t clipDest[128] )
{
	float shortest = 100000000;
	int entityNum;
	int numTriangles;
	shader_t *shader;
	int		fogNum;
	int dlighted;
	vec4_t clip, eye;

	unsigned int pointOr = 0;
	unsigned int pointAnd = (unsigned int)~0;

	R_RotateForViewer();

	R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted );
	RB_BeginSurface( shader, fogNum );
	rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );

	assert( tess.numVertexes < 128 );
	int i;
	for ( i = 0; i < tess.numVertexes; i++ )
	{

		unsigned int pointFlags = 0;

		R_TransformModelToClip( tess.xyz[i], tr.or.modelMatrix, tr.viewParms.projectionMatrix, eye, clip );
		int j;
		for ( j = 0; j < 3; j++ )
		{
			if ( clip[j] >= clip[3] )
			{
				pointFlags |= (1 << (j*2));
			}
			else if ( clip[j] <= -clip[3] )
			{
				pointFlags |= ( 1 << (j*2+1));
			}
		}
		pointAnd &= pointFlags;
		pointOr |= pointFlags;
	}

	// trivially reject
	if ( pointAnd )
	{
		return qtrue;
	}

	// determine if this surface is backfaced and also determine the distance
	// to the nearest vertex so we can cull based on portal range.  Culling
	// based on vertex distance isn't 100% correct (we should be checking for
	// range to the surface), but it's good enough for the types of portals
	// we have in the game right now.
	numTriangles = tess.numIndexes / 3;

	for ( i = 0; i < tess.numIndexes; i += 3 )
	{
		vec3_t normal;
		VectorSubtract( tess.xyz[tess.indexes[i]], tr.viewParms.or.origin, normal );

		float len = VectorLengthSquared( normal );			// lose the sqrt
		if ( len < shortest )
		{
			shortest = len;
		}

		if ( DotProduct( normal, tess.normal[tess.indexes[i]] ) >= 0 )
		{
			numTriangles--;
		}
	}
	if ( !numTriangles )
	{
		return qtrue;
	}

	// mirrors can early out at this point, since we don't do a fade over distance
	// with them (although we could)
	if ( IsMirror( drawSurf, entityNum ) )
	{
		return qfalse;
	}

	if ( shortest > (tess.shader->portalRange*tess.shader->portalRange) )
	{
		return qtrue;
	}

	return qfalse;
}


/*
==========================================================================================

DRAWSURF SORTING

==========================================================================================
*/


static ID_INLINE void R_Radix( int byte, int size, drawSurf_t *source, drawSurf_t *dest )
{
  int count[ 256 ] = { 0 };
  int index[ 256 ] = { 0 };
  int           i;

  unsigned char* sortKey = ( (unsigned char *)&source[ 0 ].sort ) + byte;
  unsigned char* end = sortKey + ( size * sizeof( drawSurf_t ) );

  for( ; sortKey < end; sortKey += sizeof( drawSurf_t ) )
    ++count[ *sortKey ];

  for( i = 1; i < 256; ++i )
    index[ i ] = index[ i - 1 ] + count[ i - 1 ];

  sortKey = ( (unsigned char *)&source[ 0 ].sort ) + byte;
  for( i = 0; i < size; ++i, sortKey += sizeof( drawSurf_t ) )
    dest[ index[ *sortKey ]++ ] = source[ i ];
}

/*
===============
R_RadixSort

Radix sort with 4 byte size buckets
===============
*/
static void R_RadixSort( drawSurf_t *source, int size )
{
  static drawSurf_t scratch[ MAX_DRAWSURFS ];
#ifdef Q3_LITTLE_ENDIAN
  R_Radix( 0, size, source, scratch );
  R_Radix( 1, size, scratch, source );
  R_Radix( 2, size, source, scratch );
  R_Radix( 3, size, scratch, source );
#else
  R_Radix( 3, size, source, scratch );
  R_Radix( 2, size, scratch, source );
  R_Radix( 1, size, source, scratch );
  R_Radix( 0, size, scratch, source );
#endif //Q3_LITTLE_ENDIAN
}



/*
========================
R_MirrorViewBySurface: Returns qtrue if another view has been rendered
========================
*/
static qboolean R_MirrorViewBySurface (drawSurf_t *drawSurf, int entityNum)
{
	vec4_t			clipDest[128];
	orientation_t	surface, camera;

	// don't recursively mirror
	if(tr.viewParms.isPortal)
    {
		ri.Printf( PRINT_DEVELOPER, "WARNING: recursive mirror/portal found\n" );
		return qfalse;
	}

	if(r_noportals->integer || (r_fastsky->integer == 1) )
    {
		return qfalse;
	}

	// trivially reject portal/mirror
	if ( SurfIsOffscreen( drawSurf, clipDest ) )
    {
		return qfalse;
	}

	// save old viewParms so we can return to it after the mirror view
	viewParms_t oldParms = tr.viewParms;
	viewParms_t newParms = tr.viewParms;
    
	newParms.isPortal = qtrue;
	if ( !R_GetPortalOrientations( drawSurf, entityNum, &surface, &camera, newParms.pvsOrigin, &newParms.isMirror ) )
    {
		return qfalse;		// bad portal, no portalentity
	}

	R_MirrorPoint(oldParms.or.origin, &surface, &camera, newParms.or.origin );

	VectorSubtract( vec3_origin, camera.axis[0], newParms.portalPlane.normal );
	newParms.portalPlane.dist = DotProduct( camera.origin, newParms.portalPlane.normal );
	
	R_MirrorVector(oldParms.or.axis[0], &surface, &camera, newParms.or.axis[0]);
	R_MirrorVector(oldParms.or.axis[1], &surface, &camera, newParms.or.axis[1]);
	R_MirrorVector(oldParms.or.axis[2], &surface, &camera, newParms.or.axis[2]);

	// OPTIMIZE: restrict the viewport on the mirrored view

	// render the mirror view
	R_RenderView(&newParms);

	tr.viewParms = oldParms;

	return qtrue;
}


static void R_SortDrawSurfs( drawSurf_t *drawSurfs, int numDrawSurfs )
{
	shader_t		*shader;
	int				fogNum;
	int				entityNum;
	int				dlighted;
	int				i;

	// it is possible for some views to not have any surfaces
	if ( numDrawSurfs < 1 ) {
		// we still need to add it for hyperspace cases
		R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
		return;
	}

	// if we overflowed MAX_DRAWSURFS, the drawsurfs
	// wrapped around in the buffer and we will be missing
	// the first surfaces, not the last ones
	if ( numDrawSurfs > MAX_DRAWSURFS ) {
		numDrawSurfs = MAX_DRAWSURFS;
	}

	// sort the drawsurfs by sort type, then orientation, then shader
	R_RadixSort( drawSurfs, numDrawSurfs );

	// check for any pass through drawing, which
	// may cause another view to be rendered first
	for ( i = 0 ; i < numDrawSurfs ; i++ )
    {
		R_DecomposeSort( (drawSurfs+i)->sort, &entityNum, &shader, &fogNum, &dlighted );

		if ( shader->sort > SS_PORTAL ) {
			break;
		}

		// no shader should ever have this sort type
		if ( shader->sort == SS_BAD ) {
			ri.Error (ERR_DROP, "Shader '%s'with sort == SS_BAD", shader->name );
		}

		// if the mirror was completely clipped away, we may need to check another surface
		if ( R_MirrorViewBySurface( (drawSurfs+i), entityNum) )
        {
			// this is a debug option to see exactly what is being mirrored
			break;		// only one mirror view at a time
		}
	}

	R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
}


/*
=================
R_SpriteFogNum: See if a sprite is inside a fog volume
=================
*/
static int R_SpriteFogNum( trRefEntity_t *ent )
{
	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}
	int	i, j;
	for ( i = 1 ; i < tr.world->numfogs ; i++ )
    {
		fog_t *fog = &tr.world->fogs[i];
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


static void R_AddEntitySurfaces (void)
{

	for ( tr.currentEntityNum = 0; tr.currentEntityNum < tr.refdef.num_entities; tr.currentEntityNum++ )
    {
		trRefEntity_t* ent = tr.currentEntity = &tr.refdef.entities[tr.currentEntityNum];

		ent->needDlights = qfalse;

		// preshift the value we are going to OR into the drawsurf sort
		tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;

		//
		// the weapon model must be handled special --
		// we don't want the hacked weapon position showing in 
		// mirrors, because the true body position will already be drawn
		//
		if ( (ent->e.renderfx & RF_FIRST_PERSON) && tr.viewParms.isPortal) {
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
                if ( (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal)
                {
                    continue;
                }
                shader_t* shader = R_GetShaderByHandle( ent->e.customShader );
                R_AddDrawSurf( &entitySurface, shader, R_SpriteFogNum( ent ), 0 );
                break;

            case RT_MODEL:
                // we must set up parts of tr.or for model culling
                R_RotateForEntity( ent, &tr.viewParms, &tr.or );

                tr.currentModel = R_GetModelByHandle( ent->e.hModel );
                if (tr.currentModel == NULL)
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
                        break;
                    case MOD_BRUSH:
                        R_AddBrushModelSurfaces( ent );
                        break;
                    case MOD_BAD:		// null model axis
                        if ( (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal) {
                            break;
                        }
                        R_AddDrawSurf( &entitySurface, tr.defaultShader, 0, 0 );
                        break;
                    default:
                        ri.Error( ERR_DROP, "R_AddEntitySurfaces: Bad modeltype" );
                        break;
                    }
                }
                break;
            default:
                ri.Error( ERR_DROP, "R_AddEntitySurfaces: Bad reType" );
		}
	}

}


static void R_GenerateDrawSurfs( void )
{
	R_AddWorldSurfaces();

	R_AddPolygonSurfaces();

	// set the projection matrix with the minimum zfar now that we have the world bounded
	// this needs to be done before entities are added, 
    // because they use the projection matrix for lod calculation

	// dynamically compute far clip plane distance
	R_SetFarClip();

	// we know the size of the clipping volume. Now set the rest of the projection matrix.
	R_SetupProjectionZ(&tr.viewParms);

	if( r_drawentities->integer )
    {
		R_AddEntitySurfaces();
	}

}

/*
================
R_DebugPolygon
================
*/
static void R_DebugPolygon( int color, int numPoints, float *points )
{
	int		i;

	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	// draw solid shade

	glColor3f( color&1, (color>>1)&1, (color>>2)&1 );
	glBegin( GL_POLYGON );
	for ( i = 0 ; i < numPoints ; i++ ) {
		glVertex3fv( points + i * 3 );
	}
	glEnd();

	// draw wireframe outline
	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	glDepthRange( 0, 0 );
	glColor3f( 1, 1, 1 );
	glBegin( GL_POLYGON );
	for ( i = 0 ; i < numPoints ; i++ ) {
		glVertex3fv( points + i * 3 );
	}
	glEnd();
	glDepthRange( 0, 1 );
}

/*
====================
R_DebugGraphics

Visualization aid for movement clipping debugging
====================
*/
static void R_DebugGraphics( void ) 
{
	R_IssuePendingRenderCommands();

	GL_Bind( tr.whiteImage);
	GL_Cull( CT_FRONT_SIDED );
	ri.CM_DrawDebugSurface( R_DebugPolygon );
}





//==========================================================================================

static void R_SetupProjection(viewParms_t *dest, float zProj)
{
	float ymax = zProj * tan(dest->fovY * (M_PI / 360.0f));
	float ymin = -ymax;

	float xmax = zProj * tan(dest->fovX * (M_PI / 360.0f));
	float xmin = -xmax;

	float width = xmax - xmin;
	float height = ymax - ymin;
	
	dest->projectionMatrix[0] = 2 * zProj / width;
	dest->projectionMatrix[4] = 0;
	dest->projectionMatrix[8] = (xmax + xmin ) / width;
	dest->projectionMatrix[12] = 0;

	dest->projectionMatrix[1] = 0;
	dest->projectionMatrix[5] = 2 * zProj / height;
	dest->projectionMatrix[9] = ( ymax + ymin ) / height;	// normally 0
	dest->projectionMatrix[13] = 0;

	dest->projectionMatrix[3] = 0;
	dest->projectionMatrix[7] = 0;
	dest->projectionMatrix[11] = -1;
	dest->projectionMatrix[15] = 0;
	
	// Now that we have all the data for the projection matrix we can also setup the view frustum.
	R_SetupFrustum(dest, xmin, xmax, ymax, zProj);
}



void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, int fogIndex, int dlightMap )
{


	int	index = tr.refdef.numDrawSurfs & DRAWSURF_MASK;

	// instead of checking for overflow, we just mask the index, so it wraps around
	// the sort data is packed into a single 32 bit value so it can be
	// compared quickly during the qsorting process
	tr.refdef.drawSurfs[index].sort = (shader->sortedIndex << QSORT_SHADERNUM_SHIFT) 
		| tr.shiftedEntityNum | ( fogIndex << QSORT_FOGNUM_SHIFT ) | (int)dlightMap;
	tr.refdef.drawSurfs[index].surface = surface;
	tr.refdef.numDrawSurfs++;
}

/*
=================
R_DecomposeSort
=================
*/
void R_DecomposeSort( unsigned sort, int *entityNum, shader_t **shader, int *fogNum, int *dlightMap )
{
	*fogNum = ( sort >> QSORT_FOGNUM_SHIFT ) & 31;
	*shader = tr.sortedShaders[ ( sort >> QSORT_SHADERNUM_SHIFT ) & (MAX_SHADERS-1) ];
	*entityNum = ( sort >> QSORT_REFENTITYNUM_SHIFT ) & REFENTITYNUM_MASK;
	*dlightMap = sort & 3;
}



/*
 * A view may be either the actual camera view, or a mirror / remote location
 */
void R_RenderView(viewParms_t *parms)
{
	if ( (parms->viewportWidth <= 0) || (parms->viewportHeight <= 0) )
		return;

	tr.viewCount++;

	tr.viewParms = *parms;
	tr.viewParms.frameSceneNum = tr.frameSceneNum;
	tr.viewParms.frameCount = tr.frameCount;

	int firstDrawSurf = tr.refdef.numDrawSurfs;

	tr.viewCount++;

	// set viewParms.world
	R_RotateForViewer();

	R_SetupProjection(&tr.viewParms, r_zproj->value);

	R_GenerateDrawSurfs();

	R_SortDrawSurfs( tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf );


    if ( r_debugSurface->integer )
    {
	    // draw main system development information (surface outlines, etc)
    	R_DebugGraphics();
	}
}



void R_LocalNormalToWorld (vec3_t local, vec3_t world)
{
	world[0] = local[0] * tr.or.axis[0][0] + local[1] * tr.or.axis[1][0] + local[2] * tr.or.axis[2][0];
	world[1] = local[0] * tr.or.axis[0][1] + local[1] * tr.or.axis[1][1] + local[2] * tr.or.axis[2][1];
	world[2] = local[0] * tr.or.axis[0][2] + local[1] * tr.or.axis[1][2] + local[2] * tr.or.axis[2][2];
}


void R_LocalPointToWorld (vec3_t local, vec3_t world)
{
	world[0] = local[0] * tr.or.axis[0][0] + local[1] * tr.or.axis[1][0] + local[2] * tr.or.axis[2][0] + tr.or.origin[0];
	world[1] = local[0] * tr.or.axis[0][1] + local[1] * tr.or.axis[1][1] + local[2] * tr.or.axis[2][1] + tr.or.origin[1];
	world[2] = local[0] * tr.or.axis[0][2] + local[1] * tr.or.axis[1][2] + local[2] * tr.or.axis[2][2] + tr.or.origin[2];
}


/*
static void R_WorldToLocal(vec3_t world, vec3_t local)
{
	local[0] = DotProduct(world, tr.or.axis[0]);
	local[1] = DotProduct(world, tr.or.axis[1]);
	local[2] = DotProduct(world, tr.or.axis[2]);
}
*/


/*
==========================
R_TransformModelToClip

==========================
*/
void R_TransformModelToClip( const vec3_t src, const float *modelMatrix, const float *projectionMatrix,	vec4_t eye, vec4_t dst )
{
	int i;

	for ( i = 0 ; i < 4 ; i++ )
    {
		eye[i] = 
			src[0] * modelMatrix[ i + 0 * 4 ] +
			src[1] * modelMatrix[ i + 1 * 4 ] +
			src[2] * modelMatrix[ i + 2 * 4 ] +
                 1 * modelMatrix[ i + 3 * 4 ];
	}

	for ( i = 0 ; i < 4 ; i++ )
    {
		dst[i] = 
			eye[0] * projectionMatrix[ i + 0 * 4 ] +
			eye[1] * projectionMatrix[ i + 1 * 4 ] +
			eye[2] * projectionMatrix[ i + 2 * 4 ] +
			eye[3] * projectionMatrix[ i + 3 * 4 ];
	}
}

/*
==========================
R_TransformClipToWindow
==========================
*/
void R_TransformClipToWindow( const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window )
{
	normalized[0] = clip[0] / clip[3];
	normalized[1] = clip[1] / clip[3];
	normalized[2] = ( clip[2] + clip[3] ) / ( 2 * clip[3] );

	window[0] = 0.5f * ( 1.0f + normalized[0] ) * view->viewportWidth;
	window[1] = 0.5f * ( 1.0f + normalized[1] ) * view->viewportHeight;
	window[2] = normalized[2];

	window[0] = (int) ( window[0] + 0.5 );
	window[1] = (int) ( window[1] + 0.5 );
}

/*
=================
R_RotateForEntity

Generates an orientation for an entity and viewParms
Does NOT produce any GL calls
Called by both the front end and the back end
=================
*/
void R_RotateForEntity( const trRefEntity_t *ent, const viewParms_t *viewParms, orientationr_t *or )
{
	float	glMatrix[16];
	vec3_t	delta;
	float	axisLength;

	if ( ent->e.reType != RT_MODEL )
    {
		*or = viewParms->world;
		return;
	}

	VectorCopy( ent->e.origin, or->origin );

	VectorCopy( ent->e.axis[0], or->axis[0] );
	VectorCopy( ent->e.axis[1], or->axis[1] );
	VectorCopy( ent->e.axis[2], or->axis[2] );

	glMatrix[0] = or->axis[0][0];
	glMatrix[4] = or->axis[1][0];
	glMatrix[8] = or->axis[2][0];
	glMatrix[12] = or->origin[0];

	glMatrix[1] = or->axis[0][1];
	glMatrix[5] = or->axis[1][1];
	glMatrix[9] = or->axis[2][1];
	glMatrix[13] = or->origin[1];

	glMatrix[2] = or->axis[0][2];
	glMatrix[6] = or->axis[1][2];
	glMatrix[10] = or->axis[2][2];
	glMatrix[14] = or->origin[2];

	glMatrix[3] = 0;
	glMatrix[7] = 0;
	glMatrix[11] = 0;
	glMatrix[15] = 1;

	MatrixMultiply4x4( glMatrix, viewParms->world.modelMatrix, or->modelMatrix );

	// calculate the viewer origin in the model's space
	// needed for fog, specular, and environment mapping
	VectorSubtract( viewParms->or.origin, or->origin, delta );

	// compensate for scale in the axes if necessary
	if( ent->e.nonNormalizedAxes )
    {
		axisLength = VectorLength( ent->e.axis[0] );
		if ( !axisLength ) {
			axisLength = 0;
		} else {
			axisLength = 1.0f / axisLength;
		}
	}
    else
    {
		axisLength = 1.0f;
	}

	or->viewOrigin[0] = DotProduct( delta, or->axis[0] ) * axisLength;
	or->viewOrigin[1] = DotProduct( delta, or->axis[1] ) * axisLength;
	or->viewOrigin[2] = DotProduct( delta, or->axis[2] ) * axisLength;
}



void R_InitMain(void)
{
    r_noportals = ri.Cvar_Get ("r_noportals", "0", CVAR_CHEAT);
    r_debugSurface = ri.Cvar_Get ("r_debugSurface", "0", CVAR_CHEAT);
    r_drawentities = ri.Cvar_Get ("r_drawentities", "1", CVAR_CHEAT );


    r_znear = ri.Cvar_Get( "r_znear", "4", CVAR_CHEAT );
    ri.Cvar_CheckRange( r_znear, 0.001f, 200, qfalse );
    r_zproj = ri.Cvar_Get( "r_zproj", "64", CVAR_ARCHIVE );
}


