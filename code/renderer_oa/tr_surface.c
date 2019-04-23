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

/*

tr_surf.c: this entire file is back end.

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here
if you don't want to use the shader system.
*/

#include "tr_local.h"


extern shaderCommands_t	tess;
extern cvar_t* r_flares;

#define RB_CHECKOVERFLOW(v,i)   if (tess.numVertexes + (v) >= SHADER_MAX_VERTEXES || tess.numIndexes + (i) >= SHADER_MAX_INDEXES ) {RB_CheckOverflow(v,i);}


void RB_CheckOverflow( int verts, int indexes )
{
	if (tess.numVertexes + verts < SHADER_MAX_VERTEXES &&
		tess.numIndexes + indexes < SHADER_MAX_INDEXES) {
		return;
	}

	RB_EndSurface();

	if ( verts >= SHADER_MAX_VERTEXES ) {
		ri.Error(ERR_DROP, "RB_CheckOverflow: verts > MAX (%d > %d)", verts, SHADER_MAX_VERTEXES );
	}
	if ( indexes >= SHADER_MAX_INDEXES ) {
		ri.Error(ERR_DROP, "RB_CheckOverflow: indices > MAX (%d > %d)", indexes, SHADER_MAX_INDEXES );
	}

	RB_BeginSurface(tess.shader, tess.fogNum );
}


void RB_AddQuadStampExt(vec3_t origin, vec3_t left, vec3_t up, byte *color, float s1, float t1, float s2, float t2 )
{

	RB_CHECKOVERFLOW( 4, 6 );

	int ndx = tess.numVertexes;
	int nInd = tess.numIndexes;

	// constant normal all the way around
	vec3_t normal;
	VectorSubtract( ORIGIN, backEnd.viewParms.or.axis[0], normal );

	// triangle indexes for a simple quad
	tess.indexes[ nInd++ ] = ndx;
	tess.indexes[ nInd++ ] = ndx + 1;
	tess.indexes[ nInd++ ] = ndx + 3;

	tess.indexes[ nInd++ ] = ndx + 3;
	tess.indexes[ nInd++ ] = ndx + 1;
	tess.indexes[ nInd++ ] = ndx + 2;

	//
	tess.xyz[ndx][0] = origin[0] + left[0] + up[0];
	tess.xyz[ndx][1] = origin[1] + left[1] + up[1];
	tess.xyz[ndx][2] = origin[2] + left[2] + up[2];

	tess.normal[ndx][0] = normal[0];
	tess.normal[ndx][1] = normal[1];
	tess.normal[ndx][2] = normal[2];
	// standard square texture coordinates
	tess.texCoords[ndx][0][0] = tess.texCoords[ndx][1][0] = s1;
	tess.texCoords[ndx][0][1] = tess.texCoords[ndx][1][1] = t1;
	// constant color all the way around
	// should this be identity and let the shader specify from entity?
	tess.vertexColors[ndx][0] = color[0];
	tess.vertexColors[ndx][1] = color[1];
	tess.vertexColors[ndx][2] = color[2];
	tess.vertexColors[ndx][3] = color[3];
	
	//
	ndx++;
	tess.xyz[ndx][0] = origin[0] - left[0] + up[0];
	tess.xyz[ndx][1] = origin[1] - left[1] + up[1];
	tess.xyz[ndx][2] = origin[2] - left[2] + up[2];

	tess.normal[ndx][0] = normal[0];
	tess.normal[ndx][1] = normal[1];
	tess.normal[ndx][2] = normal[2];
	// standard square texture coordinates
	tess.texCoords[ndx][0][0] = tess.texCoords[ndx][1][0] = s2;
	tess.texCoords[ndx][0][1] = tess.texCoords[ndx][1][1] = t1;

	tess.vertexColors[ndx][0] = color[0];
	tess.vertexColors[ndx][1] = color[1];
	tess.vertexColors[ndx][2] = color[2];
	tess.vertexColors[ndx][3] = color[3];


	//
	ndx++;
	tess.xyz[ndx][0] = origin[0] - left[0] - up[0];
	tess.xyz[ndx][1] = origin[1] - left[1] - up[1];
	tess.xyz[ndx][2] = origin[2] - left[2] - up[2];

	tess.normal[ndx][0] = normal[0];
	tess.normal[ndx][1] = normal[1];
	tess.normal[ndx][2] = normal[2];
	
	tess.texCoords[ndx][0][0] = tess.texCoords[ndx][1][0] = s2;
	tess.texCoords[ndx][0][1] = tess.texCoords[ndx][1][1] = t2;

	tess.vertexColors[ndx][0] = color[0];
	tess.vertexColors[ndx][1] = color[1];
	tess.vertexColors[ndx][2] = color[2];
	tess.vertexColors[ndx][3] = color[3];


	//
	ndx++;
	tess.xyz[ndx][0] = origin[0] + left[0] - up[0];
	tess.xyz[ndx][1] = origin[1] + left[1] - up[1];
	tess.xyz[ndx][2] = origin[2] + left[2] - up[2];

	tess.normal[ndx][0] = normal[0];
	tess.normal[ndx][1] = normal[1];
	tess.normal[ndx][2] = normal[2];

	tess.texCoords[ndx][0][0] = tess.texCoords[ndx][1][0] = s1;
	tess.texCoords[ndx][0][1] = tess.texCoords[ndx][1][1] = t2;

	tess.vertexColors[ndx][0] = color[0];
	tess.vertexColors[ndx][1] = color[1];
	tess.vertexColors[ndx][2] = color[2];
	tess.vertexColors[ndx][3] = color[3];


	tess.numVertexes += 4;
	tess.numIndexes += 6;
}


void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, unsigned char *color )
{
	RB_AddQuadStampExt( origin, left, up, color, 0, 0, 1, 1 );
}

static void RB_SurfaceSprite( void )
{
	vec3_t left, up;

	// calculate the xyz locations for the four corners
	float radius = backEnd.currentEntity->e.radius;
	if ( backEnd.currentEntity->e.rotation == 0 )
    {
		VectorScale( backEnd.viewParms.or.axis[1], radius, left );
		VectorScale( backEnd.viewParms.or.axis[2], radius, up );
	}
    else
    {
		float ang = (M_PI / 180)* backEnd.currentEntity->e.rotation ;
		float s = sin( ang );
		float c = cos( ang );

		VectorScale( backEnd.viewParms.or.axis[1], c * radius, left );
		VectorMA( left, -s * radius, backEnd.viewParms.or.axis[2], left );

		VectorScale( backEnd.viewParms.or.axis[2], c * radius, up );
		VectorMA( up, s * radius, backEnd.viewParms.or.axis[1], up );
	}

	if ( backEnd.viewParms.isMirror )
    {
		VectorSubtract( ORIGIN, left, left );
	}

	RB_AddQuadStamp( backEnd.currentEntity->e.origin, left, up, backEnd.currentEntity->e.shaderRGBA );
}


/*
=============
RB_SurfacePolychain
=============
*/
static void RB_SurfacePolychain( srfPoly_t *p )
{
	int	i;

	RB_CHECKOVERFLOW( p->numVerts, 3*(p->numVerts - 2) );

	// fan triangles into the tess array
	int numv = tess.numVertexes;
	int sn = p->numVerts;
	for(i = 0; i < sn; i++)
    {
		VectorCopy( p->verts[i].xyz, tess.xyz[numv] );
		tess.texCoords[numv][0][0] = p->verts[i].st[0];
		tess.texCoords[numv][0][1] = p->verts[i].st[1];
		
		tess.vertexColors[numv][0] = p->verts[ i ].modulate[0];
		tess.vertexColors[numv][1] = p->verts[ i ].modulate[1];
		tess.vertexColors[numv][2] = p->verts[ i ].modulate[2];
		tess.vertexColors[numv][3] = p->verts[ i ].modulate[3];

		numv++;
	}

	// generate fan indexes into the tess array
	for ( i = 0; i < p->numVerts-2; i++ )
	{
		tess.indexes[tess.numIndexes + 0] = tess.numVertexes;
		tess.indexes[tess.numIndexes + 1] = tess.numVertexes + i + 1;
		tess.indexes[tess.numIndexes + 2] = tess.numVertexes + i + 2;
		tess.numIndexes += 3;
	}

	tess.numVertexes = numv;
}


/*
=============
RB_SurfaceTriangles
=============
*/
static void RB_SurfaceTriangles( srfTriangles_t *srf )
{
	int i;

    int	dlightBits = srf->dlightBits;
	tess.dlightBits |= dlightBits;

	RB_CHECKOVERFLOW( srf->numVerts, srf->numIndexes );

	for ( i = 0 ; i < srf->numIndexes ; i += 3 )
    {
		tess.indexes[ tess.numIndexes + i + 0 ] = tess.numVertexes + srf->indexes[ i + 0 ];
		tess.indexes[ tess.numIndexes + i + 1 ] = tess.numVertexes + srf->indexes[ i + 1 ];
		tess.indexes[ tess.numIndexes + i + 2 ] = tess.numVertexes + srf->indexes[ i + 2 ];
	}
	tess.numIndexes += srf->numIndexes;

	drawVert_t* dv = srf->verts;
	float* xyz = tess.xyz[ tess.numVertexes ];
	float* normal = tess.normal[ tess.numVertexes ];
	float* texCoords = tess.texCoords[ tess.numVertexes ][0];
	unsigned char *color = tess.vertexColors[ tess.numVertexes ];
	qboolean needsNormal = tess.shader->needsNormal;

	for ( i = 0 ; i < srf->numVerts ; i++, dv++, xyz += 4, normal += 4, texCoords += 4, color += 4 )
    {
		xyz[0] = dv->xyz[0];
		xyz[1] = dv->xyz[1];
		xyz[2] = dv->xyz[2];

		if ( needsNormal )
        {
			normal[0] = dv->normal[0];
			normal[1] = dv->normal[1];
			normal[2] = dv->normal[2];
		}

		texCoords[0] = dv->st[0];
		texCoords[1] = dv->st[1];

		texCoords[2] = dv->lightmap[0];
		texCoords[3] = dv->lightmap[1];

		color[0] = dv->color[0];
		color[1] = dv->color[1];
		color[2] = dv->color[2];
		color[3] = dv->color[3];

	}

	for ( i = 0 ; i < srf->numVerts ; i++ )
	{
		tess.vertexDlightBits[ tess.numVertexes + i] = dlightBits;
	}

	tess.numVertexes += srf->numVerts;
}



static void RB_SurfaceBeam( void )
{
    #define NUM_BEAM_SEGS 6
	int	i;
	vec3_t perpvec, direction, normalized_direction;
	vec3_t start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];

    
	direction[0] = backEnd.currentEntity->e.oldorigin[0] - backEnd.currentEntity->e.origin[0];
	direction[1] = backEnd.currentEntity->e.oldorigin[1] - backEnd.currentEntity->e.origin[1];
	direction[2] = backEnd.currentEntity->e.oldorigin[2] - backEnd.currentEntity->e.origin[2];


	// this rotate and negate guarantees a vector not colinear with the original
  	perpvec[1] = -direction[0];
	perpvec[2] = direction[1];
	perpvec[0] = direction[2];
    // actually can not guarantee,
    // assume forward = (1/sqrt(3), 1/sqrt(3), -1/sqrt(3)),
    // then right = (-1/sqrt(3), -1/sqrt(3), 1/sqrt(3))
    
    float sqlen = direction[0]*direction[0] + direction[1]*direction[1] + direction[2]*direction[2];
    if(0 == sqlen)
    {
        return;
    }

    float invLen = 1.0f / sqrtf(sqlen);
    normalized_direction[0] = direction[0] * invLen;
    normalized_direction[1] = direction[1] * invLen;
    normalized_direction[2] = direction[2] * invLen;


    float d = DotProduct(normalized_direction, perpvec);
	perpvec[0] -= d*normalized_direction[0];
	perpvec[1] -= d*normalized_direction[1];
	perpvec[2] -= d*normalized_direction[2];

    VectorScale( perpvec, 4, perpvec );

	for ( i = 0; i < NUM_BEAM_SEGS ; i++ )
	{
		RotateAroundUnitVector( start_points[i], normalized_direction, perpvec, (360.0/NUM_BEAM_SEGS)*i );
        //VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[i], direction, end_points[i] );
	}

	GL_Bind( tr.whiteImage );

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	qglColor3f( 1, 0, 0 );

	qglBegin( GL_TRIANGLE_STRIP );
	for ( i = 0; i <= NUM_BEAM_SEGS; i++ )
	{
		qglVertex3fv( start_points[ i % NUM_BEAM_SEGS] );
		qglVertex3fv( end_points[ i % NUM_BEAM_SEGS] );
	}
	qglEnd();
}

//================================================================================

static void DoRailCore( const vec3_t start, const vec3_t end, const vec3_t up, float len, float spanWidth )
{
	float t = len / 256.0f;

	RB_CHECKOVERFLOW( 4, 6 );

	int vbase = tess.numVertexes;
	float spanWidth2 = -spanWidth;

	// FIXME: use quad stamp?
	VectorMA( start, spanWidth, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] * 0.25;
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] * 0.25;
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] * 0.25;
	tess.numVertexes++;

	VectorMA( start, spanWidth2, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 1;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.numVertexes++;

	VectorMA( end, spanWidth, up, tess.xyz[tess.numVertexes] );

	tess.texCoords[tess.numVertexes][0][0] = t;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.numVertexes++;

	VectorMA( end, spanWidth2, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = t;
	tess.texCoords[tess.numVertexes][0][1] = 1;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = vbase;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 2;

	tess.indexes[tess.numIndexes++] = vbase + 2;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 3;
}


static void DoRailDiscs( int numSegs, const vec3_t start, const vec3_t dir, const vec3_t right, const vec3_t up )
{
	int i;
	vec3_t	pos[4];

	int	spanWidth = r_railWidth->integer;
	float scale = 0.25;

	if ( numSegs > 1 )
		numSegs--;
	if ( !numSegs )
		return;

	for ( i = 0; i < 4; i++ )
	{
		float c = cos( DEG2RAD( 45 + i * 90 ) );
		float s = sin( DEG2RAD( 45 + i * 90 ) );
		vec3_t v;
		v[0] = ( right[0] * c + up[0] * s ) * scale * spanWidth;
		v[1] = ( right[1] * c + up[1] * s ) * scale * spanWidth;
		v[2] = ( right[2] * c + up[2] * s ) * scale * spanWidth;
		VectorAdd( start, v, pos[i] );

		if ( numSegs > 1 )
		{
			// offset by 1 segment if we're doing a long distance shot
			VectorAdd( pos[i], dir, pos[i] );
		}
	}

	for ( i = 0; i < numSegs; i++ )
	{
		int j;

		RB_CHECKOVERFLOW( 4, 6 );

		for ( j = 0; j < 4; j++ )
		{
			VectorCopy( pos[j], tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = ( j < 2 );
			tess.texCoords[tess.numVertexes][0][1] = ( j && j != 3 );
			tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
			tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
			tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
			tess.numVertexes++;

			VectorAdd( pos[j], dir, pos[j] );
		}

		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 0;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 1;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 3;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 3;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 1;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 2;
	}
}


static void RB_SurfaceRailRings( void )
{

    int			numSegs;
	vec3_t		vec;
	vec3_t		right, up;
	vec3_t		start, end;

	refEntity_t *e = &backEnd.currentEntity->e;

	VectorCopy( e->oldorigin, start );
	VectorCopy( e->origin, end );

	// compute variables
	VectorSubtract( end, start, vec );
	float len = MakeTwoPerpVectors( vec, right, up );
	numSegs =  len / r_railSegmentLength->value;
	if ( numSegs <= 0 )
		numSegs = 1;


	VectorScale( vec, r_railSegmentLength->value, vec );

	DoRailDiscs( numSegs, start, vec, right, up );
}

/*
** RB_SurfaceRailCore
*/
static void RB_SurfaceRailCore( void )
{
	vec3_t		right;
	vec3_t		vec;
	vec3_t		start, end;
	vec3_t		v1, v2;

	refEntity_t* e = &backEnd.currentEntity->e;

	VectorCopy( e->oldorigin, start );
	VectorCopy( e->origin, end );

	VectorSubtract( end, start, vec );
	int len = VectorNormalize( vec );

	// compute side vector
	VectorSubtract( start, backEnd.viewParms.or.origin, v1 );
	//VectorNormalize( v1 );
	VectorSubtract( end, backEnd.viewParms.or.origin, v2 );
	//VectorNormalize( v2 );
	CrossProduct( v1, v2, right );
	VectorNormalize( right );

	DoRailCore( start, end, right, len, r_railCoreWidth->integer );
}

/*
** RB_SurfaceLightningBolt
*/
static void DoLightningCore( const vec3_t start, const vec3_t end, const vec3_t up, float len)
{
	float t = len / 256.0f;

	RB_CHECKOVERFLOW( 4, 6 );

    int nVert = tess.numVertexes;
    tess.numVertexes += 4; 

    int nIdx = tess.numIndexes;
    tess.numIndexes += 6;

	tess.indexes[nIdx++] = nVert;
	tess.indexes[nIdx++] = nVert + 1;
	tess.indexes[nIdx++] = nVert + 2;

	tess.indexes[nIdx++] = nVert + 2;
	tess.indexes[nIdx++] = nVert + 1;
	tess.indexes[nIdx++] = nVert + 3;

    const int spanWidth = 8;

    float temp[3];
    VectorScale(up, spanWidth, temp);

	// FIXME: use quad stamp?
	VectorAdd( start, temp, tess.xyz[nVert] );
	tess.texCoords[nVert][0][0] = 0;
	tess.texCoords[nVert][0][1] = 0;
	tess.vertexColors[nVert][0] = backEnd.currentEntity->e.shaderRGBA[0] * 0.25;
	tess.vertexColors[nVert][1] = backEnd.currentEntity->e.shaderRGBA[1] * 0.25;
	tess.vertexColors[nVert][2] = backEnd.currentEntity->e.shaderRGBA[2] * 0.25;
	

    nVert++;
	VectorSubtract( start, temp, tess.xyz[nVert] );
	tess.texCoords[nVert][0][0] = 0;
	tess.texCoords[nVert][0][1] = 1;
	tess.vertexColors[nVert][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[nVert][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[nVert][2] = backEnd.currentEntity->e.shaderRGBA[2];

    nVert++;
	VectorAdd( end, temp, tess.xyz[nVert] );
	tess.texCoords[nVert][0][0] = t;
	tess.texCoords[nVert][0][1] = 0;
	tess.vertexColors[nVert][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[nVert][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[nVert][2] = backEnd.currentEntity->e.shaderRGBA[2];

    nVert++;
	VectorSubtract( end, temp, tess.xyz[nVert] );
	tess.texCoords[nVert][0][0] = t;
	tess.texCoords[nVert][0][1] = 1;
	tess.vertexColors[nVert][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[nVert][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[nVert][2] = backEnd.currentEntity->e.shaderRGBA[2];
}


static void RB_SurfaceLightningBolt( void )
{
	vec3_t		right;
	vec3_t		vec;
	vec3_t		v1, v2;
	int			i;
    float       len;
	// compute variables
	VectorSubtract( backEnd.currentEntity->e.oldorigin, backEnd.currentEntity->e.origin, vec );
    
    // normalize vec
    float square = vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2];
    len = 1.0f / sqrtf(square);
    vec[0] *= len;
    vec[1] *= len;
    vec[2] *= len;
    len *= square;
    
	// compute side vector
	VectorSubtract( backEnd.currentEntity->e.origin, backEnd.viewParms.or.origin, v1 );
	VectorSubtract( backEnd.currentEntity->e.oldorigin, backEnd.viewParms.or.origin, v2 );
	CrossProduct( v1, v2, right );
	FastNormalize1f( right );
    
	for ( i = 0 ; i < 4 ; i++ )
    {
		vec3_t	temp;

		DoLightningCore( backEnd.currentEntity->e.origin, backEnd.currentEntity->e.oldorigin, right, len );
        
		RotateAroundUnitVector( temp, vec, right, 45 );
		VectorCopy( temp, right );
	}
}

/*
** VectorArrayNormalize
*
* The inputs to this routing seem to always be close to length = 1.0 (about 0.6 to 2.0)
* This means that we don't have to worry about zero length or enormously long vectors.
*/
static void VectorArrayNormalize(vec4_t *normals, unsigned int count)
{
//    assert(count);
        
#if idppc
    {
        register float half = 0.5;
        register float one  = 1.0;
        float *components = (float *)normals;
        
        // Vanilla PPC code, but since PPC has a reciprocal square root estimate instruction,
        // runs *much* faster than calling sqrt().  We'll use a single Newton-Raphson
        // refinement step to get a little more precision.  This seems to yield results
        // that are correct to 3 decimal places and usually correct to at least 4 (sometimes 5).
        // (That is, for the given input range of about 0.6 to 2.0).
        do {
            float x, y, z;
            float B, y0, y1;
            
            x = components[0];
            y = components[1];
            z = components[2];
            components += 4;
            B = x*x + y*y + z*z;

#ifdef __GNUC__            
            asm("frsqrte %0,%1" : "=f" (y0) : "f" (B));
#else
			y0 = __frsqrte(B);
#endif
            y1 = y0 + half*y0*(one - B*y0*y0);

            x = x * y1;
            y = y * y1;
            components[-4] = x;
            z = z * y1;
            components[-3] = y;
            components[-2] = z;
        } while(count--);
    }
#else // No assembly version for this architecture, or C_ONLY defined
	// given the input, it's safe to call VectorNormalizeFast
    while (count--) {
        FastNormalize1f(normals[0]);
        normals++;
    }
#endif

}


static void LerpMeshVertexes(md3Surface_t *surf, float backlerp)
{
	short	*oldXyz, *newXyz, *oldNormals, *newNormals;
	float	*outXyz, *outNormal;
	float	oldXyzScale, newXyzScale;
	float	oldNormalScale, newNormalScale;
	int		vertNum;
	unsigned lat, lng;
	int		numVerts;

	outXyz = tess.xyz[tess.numVertexes];
	outNormal = tess.normal[tess.numVertexes];

	newXyz = (short *)((byte *)surf + surf->ofsXyzNormals)
		+ (backEnd.currentEntity->e.frame * surf->numVerts * 4);
	newNormals = newXyz + 3;

	newXyzScale = MD3_XYZ_SCALE * (1.0 - backlerp);
	newNormalScale = 1.0 - backlerp;

	numVerts = surf->numVerts;

	if ( backlerp == 0 ) {
		//
		// just copy the vertexes
		//
		for (vertNum=0 ; vertNum < numVerts ; vertNum++,
			newXyz += 4, newNormals += 4,
			outXyz += 4, outNormal += 4) 
		{

			outXyz[0] = newXyz[0] * newXyzScale;
			outXyz[1] = newXyz[1] * newXyzScale;
			outXyz[2] = newXyz[2] * newXyzScale;

			lat = ( newNormals[0] >> 8 ) & 0xff;
			lng = ( newNormals[0] & 0xff );
			lat *= (FUNCTABLE_SIZE/256);
			lng *= (FUNCTABLE_SIZE/256);

			// decode X as cos( lat ) * sin( long )
			// decode Y as sin( lat ) * sin( long )
			// decode Z as cos( long )

			outNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			outNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			outNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];
		}
	} else {
		//
		// interpolate and copy the vertex and normal
		//
		oldXyz = (short *)((byte *)surf + surf->ofsXyzNormals)
			+ (backEnd.currentEntity->e.oldframe * surf->numVerts * 4);
		oldNormals = oldXyz + 3;

		oldXyzScale = MD3_XYZ_SCALE * backlerp;
		oldNormalScale = backlerp;

		for (vertNum=0 ; vertNum < numVerts ; vertNum++,
			oldXyz += 4, newXyz += 4, oldNormals += 4, newNormals += 4,
			outXyz += 4, outNormal += 4) 
		{
			vec3_t uncompressedOldNormal, uncompressedNewNormal;

			// interpolate the xyz
			outXyz[0] = oldXyz[0] * oldXyzScale + newXyz[0] * newXyzScale;
			outXyz[1] = oldXyz[1] * oldXyzScale + newXyz[1] * newXyzScale;
			outXyz[2] = oldXyz[2] * oldXyzScale + newXyz[2] * newXyzScale;

			// FIXME: interpolate lat/long instead?
			lat = ( newNormals[0] >> 8 ) & 0xff;
			lng = ( newNormals[0] & 0xff );
			lat *= 4;
			lng *= 4;
			uncompressedNewNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressedNewNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressedNewNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

			lat = ( oldNormals[0] >> 8 ) & 0xff;
			lng = ( oldNormals[0] & 0xff );
			lat *= 4;
			lng *= 4;

			uncompressedOldNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressedOldNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressedOldNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

			outNormal[0] = uncompressedOldNormal[0] * oldNormalScale + uncompressedNewNormal[0] * newNormalScale;
			outNormal[1] = uncompressedOldNormal[1] * oldNormalScale + uncompressedNewNormal[1] * newNormalScale;
			outNormal[2] = uncompressedOldNormal[2] * oldNormalScale + uncompressedNewNormal[2] * newNormalScale;

		}
    	VectorArrayNormalize((vec4_t *)tess.normal[tess.numVertexes], numVerts);
   	}
}

static void RB_SurfaceMesh(md3Surface_t *surface)
{
	int	j;
	float backlerp;


	if( backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame )
		backlerp = 0;
    else
		backlerp = backEnd.currentEntity->e.backlerp;

	RB_CHECKOVERFLOW( surface->numVerts, surface->numTriangles*3 );

	LerpMeshVertexes(surface, backlerp);

	int* triangles = (int *) ((unsigned char *)surface + surface->ofsTriangles);
	int indexes = surface->numTriangles * 3;
	int Bob = tess.numIndexes;
	int Doug = tess.numVertexes;
	
    for (j = 0 ; j < indexes ; j++)
    {
		tess.indexes[Bob + j] = Doug + triangles[j];
	}
	tess.numIndexes += indexes;

	float* texCoords = (float *) ((unsigned char*)surface + surface->ofsSt);

	int numVerts = surface->numVerts;
	
    for ( j = 0; j < numVerts; j++ )
    {
		tess.texCoords[Doug + j][0][0] = texCoords[j*2+0];
		tess.texCoords[Doug + j][0][1] = texCoords[j*2+1];
		// FIXME: fill in lightmapST for completeness?
	}

	tess.numVertexes += surface->numVerts;
}



static void RB_SurfaceFace( srfSurfaceFace_t *surf )
{
	int			i;
	unsigned	*indices, *tessIndexes;
	float		*v;
	float		*normal;
	int			ndx;
	int			Bob;
	int			numPoints;
	int			dlightBits;

	RB_CHECKOVERFLOW( surf->numPoints, surf->numIndices );

	dlightBits = surf->dlightBits;
	tess.dlightBits |= dlightBits;

	indices = ( unsigned * ) ( ( ( char  * ) surf ) + surf->ofsIndices );

	Bob = tess.numVertexes;
	tessIndexes = tess.indexes + tess.numIndexes;
	for ( i = surf->numIndices-1 ; i >= 0; i-- ) {
		tessIndexes[i] = indices[i] + Bob;
	}

	tess.numIndexes += surf->numIndices;

	numPoints = surf->numPoints;

	if ( tess.shader->needsNormal ) {
		normal = surf->plane.normal;
		for ( i = 0, ndx = tess.numVertexes; i < numPoints; i++, ndx++ ) {
			VectorCopy( normal, tess.normal[ndx] );
		}
	}

	for ( i = 0, v = surf->points[0], ndx = tess.numVertexes; i < numPoints; i++, v += VERTEXSIZE, ndx++ )
	{
		VectorCopy( v, tess.xyz[ndx]);
		tess.texCoords[ndx][0][0] = v[3];
		tess.texCoords[ndx][0][1] = v[4];
		tess.texCoords[ndx][1][0] = v[5];
		tess.texCoords[ndx][1][1] = v[6];


		union f32_u	cvt;
		cvt.f = v[7];
		tess.vertexColors[ndx][0] = cvt.uc[0];
		tess.vertexColors[ndx][1] = cvt.uc[1];
		tess.vertexColors[ndx][2] = cvt.uc[2];
		tess.vertexColors[ndx][3] = cvt.uc[3];


		tess.vertexDlightBits[ndx] = dlightBits;
	}


	tess.numVertexes += surf->numPoints;
}


static float LodErrorForVolume( vec3_t local, float radius )
{
	vec3_t		world;
	float		d;

	// never let it go negative
	if ( r_lodCurveError->value < 0 ) {
		return 0;
	}

	world[0] = local[0] * backEnd.or.axis[0][0] + local[1] * backEnd.or.axis[1][0] + 
		local[2] * backEnd.or.axis[2][0] + backEnd.or.origin[0];
	world[1] = local[0] * backEnd.or.axis[0][1] + local[1] * backEnd.or.axis[1][1] + 
		local[2] * backEnd.or.axis[2][1] + backEnd.or.origin[1];
	world[2] = local[0] * backEnd.or.axis[0][2] + local[1] * backEnd.or.axis[1][2] + 
		local[2] * backEnd.or.axis[2][2] + backEnd.or.origin[2];

	VectorSubtract( world, backEnd.viewParms.or.origin, world );
	d = DotProduct( world, backEnd.viewParms.or.axis[0] );

	if ( d < 0 ) {
		d = -d;
	}
	d -= radius;
	if ( d < 1 ) {
		d = 1;
	}

	return r_lodCurveError->value / d;
}

/*
=============
RB_SurfaceGrid

Just copy the grid of points and triangulate
=============
*/
static void RB_SurfaceGrid( srfGridMesh_t *cv ) {
	int		i, j;
	float	*xyz;
	float	*texCoords;
	float	*normal;
	unsigned char *color;
	drawVert_t	*dv;
	int		rows, irows, vrows;
	int		used;
	int		widthTable[MAX_GRID_SIZE];
	int		heightTable[MAX_GRID_SIZE];
	float	lodError;
	int		lodWidth, lodHeight;
	int		numVertexes;
	int		dlightBits;
	int		*vDlightBits;
	qboolean	needsNormal;

	dlightBits = cv->dlightBits;
	tess.dlightBits |= dlightBits;

	// determine the allowable discrepance
	lodError = LodErrorForVolume( cv->lodOrigin, cv->lodRadius );

	// determine which rows and columns of the subdivision
	// we are actually going to use
	widthTable[0] = 0;
	lodWidth = 1;
	for ( i = 1 ; i < cv->width-1 ; i++ ) {
		if ( cv->widthLodError[i] <= lodError ) {
			widthTable[lodWidth] = i;
			lodWidth++;
		}
	}
	widthTable[lodWidth] = cv->width-1;
	lodWidth++;

	heightTable[0] = 0;
	lodHeight = 1;
	for ( i = 1 ; i < cv->height-1 ; i++ ) {
		if ( cv->heightLodError[i] <= lodError ) {
			heightTable[lodHeight] = i;
			lodHeight++;
		}
	}
	heightTable[lodHeight] = cv->height-1;
	lodHeight++;


	// very large grids may have more points or indexes than can be fit
	// in the tess structure, so we may have to issue it in multiple passes

	used = 0;
	while ( used < lodHeight - 1 ) {
		// see how many rows of both verts and indexes we can add without overflowing
		do {
			vrows = ( SHADER_MAX_VERTEXES - tess.numVertexes ) / lodWidth;
			irows = ( SHADER_MAX_INDEXES - tess.numIndexes ) / ( lodWidth * 6 );

			// if we don't have enough space for at least one strip, flush the buffer
			if ( vrows < 2 || irows < 1 ) {
				RB_EndSurface();
				RB_BeginSurface(tess.shader, tess.fogNum );
			} else {
				break;
			}
		} while ( 1 );
		
		rows = irows;
		if ( vrows < irows + 1 ) {
			rows = vrows - 1;
		}
		if ( used + rows > lodHeight ) {
			rows = lodHeight - used;
		}

		numVertexes = tess.numVertexes;

		xyz = tess.xyz[numVertexes];
		normal = tess.normal[numVertexes];
		texCoords = tess.texCoords[numVertexes][0];
		color = ( unsigned char * ) &tess.vertexColors[numVertexes];
		vDlightBits = &tess.vertexDlightBits[numVertexes];
		needsNormal = tess.shader->needsNormal;

		for ( i = 0 ; i < rows ; i++ ) {
			for ( j = 0 ; j < lodWidth ; j++ ) {
				dv = cv->verts + heightTable[ used + i ] * cv->width
					+ widthTable[ j ];

				xyz[0] = dv->xyz[0];
				xyz[1] = dv->xyz[1];
				xyz[2] = dv->xyz[2];
				texCoords[0] = dv->st[0];
				texCoords[1] = dv->st[1];
				texCoords[2] = dv->lightmap[0];
				texCoords[3] = dv->lightmap[1];
				if ( needsNormal ) {
					normal[0] = dv->normal[0];
					normal[1] = dv->normal[1];
					normal[2] = dv->normal[2];
				}
				color[0] = dv->color[0];
				color[1] = dv->color[1];
				color[2] = dv->color[2];
				color[3] = dv->color[3];
				
				
				*vDlightBits++ = dlightBits;
				xyz += 4;
				normal += 4;
				texCoords += 4;
				color += 4;
			}
		}


		// add the indexes
		{
			int		numIndexes;
			int		w, h;

			h = rows - 1;
			w = lodWidth - 1;
			numIndexes = tess.numIndexes;
			for (i = 0 ; i < h ; i++) {
				for (j = 0 ; j < w ; j++) {
					int		v1, v2, v3, v4;
			
					// vertex order to be reckognized as tristrips
					v1 = numVertexes + i*lodWidth + j + 1;
					v2 = v1 - 1;
					v3 = v2 + lodWidth;
					v4 = v3 + 1;

					tess.indexes[numIndexes] = v2;
					tess.indexes[numIndexes+1] = v3;
					tess.indexes[numIndexes+2] = v1;
					
					tess.indexes[numIndexes+3] = v1;
					tess.indexes[numIndexes+4] = v3;
					tess.indexes[numIndexes+5] = v4;
					numIndexes += 6;
				}
			}

			tess.numIndexes = numIndexes;
		}

		tess.numVertexes += rows * lodWidth;

		used += rows - 1;
	}
}


/*
===========================================================================

NULL MODEL

===========================================================================
*/

/*
===================
RB_SurfaceAxis

Draws x/y/z lines from the origin for orientation debugging
===================
*/
static void RB_SurfaceAxis( void )
{
	GL_Bind( tr.whiteImage );
	GL_State( GLS_DEFAULT );
	qglLineWidth( 3 );
	qglBegin( GL_LINES );
	qglColor3f( 1,0,0 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 16,0,0 );
	qglColor3f( 0,1,0 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 0,16,0 );
	qglColor3f( 0,0,1 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 0,0,16 );
	qglEnd();
	qglLineWidth( 1 );
}

//===========================================================================

/*
====================
RB_SurfaceEntity

Entities that have a single procedurally generated surface
====================
*/
static void RB_SurfaceEntity( surfaceType_t *surfType )
{
	switch( backEnd.currentEntity->e.reType ) {
	case RT_SPRITE:
		RB_SurfaceSprite();
		break;
	case RT_BEAM:
		RB_SurfaceBeam();
		break;
	case RT_RAIL_CORE:
		RB_SurfaceRailCore();
		break;
	case RT_RAIL_RINGS:
		RB_SurfaceRailRings();
		break;
	case RT_LIGHTNING:
		RB_SurfaceLightningBolt();
		break;
	default:
		RB_SurfaceAxis();
		break;
	}
}


static void RB_SurfaceBad( surfaceType_t *surfType )
{
	ri.Printf( PRINT_ALL, "Bad surface tesselated.\n" );
}

static void RB_SurfaceSkip( void *surf )
{
}

static void RB_SurfaceFlare(srfFlare_t *surf)
{
	if (r_flares->integer)
		RB_AddFlare(surf, tess.fogNum, surf->origin, surf->color, surf->normal, r_flares->integer, 1.0f, 0);
}


void RB_MDRSurfaceAnim( mdrSurface_t *surface )
{
	int				j, k;
	float			backlerp;
	
	mdrBone_t		bones[MDR_MAX_BONES], *bonePtr;


	// don't lerp if lerping off, or this is the only frame, or the last frame...
	if (backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame) 
		backlerp = 0;	// if backlerp is 0, lerping is off and frontlerp is never used
	else  
		backlerp = backEnd.currentEntity->e.backlerp;

	mdrHeader_t* header = (mdrHeader_t *)((unsigned char *)surface + surface->ofsHeader);

	int frameSize = (size_t)( &((mdrFrame_t *)0)->bones[ header->numBones ] );

	mdrFrame_t* frame = (mdrFrame_t *)((byte *)header + header->ofsFrames +	backEnd.currentEntity->e.frame * frameSize );
	
    mdrFrame_t* oldFrame = (mdrFrame_t *)((byte *)header + header->ofsFrames + backEnd.currentEntity->e.oldframe * frameSize );

    RB_CheckOverflow( surface->numVerts, surface->numTriangles );

	int* triangles = (int *) ((byte *)surface + surface->ofsTriangles);
	int indexes	= surface->numTriangles * 3;
	int baseIndex = tess.numIndexes;
	int baseVertex = tess.numVertexes;
	
	// Set up all triangles.
	for (j = 0 ; j < indexes ; j++) 
	{
		tess.indexes[baseIndex + j] = baseVertex + triangles[j];
	}
	tess.numIndexes += indexes;

	//
	// lerp all the needed bones
	//
	if ( !backlerp ) 
	{
		// no lerping needed
		bonePtr = frame->bones;
	} 
	else 
	{
		bonePtr = bones;
	    int nBones = header->numBones;
        int n;
        float tmp;
		for ( n = 0 ; n < nBones; n++ ) 
		{        
            tmp = frame->bones[n].matrix[0][0];
            bones[n].matrix[0][0] = tmp + backlerp * (oldFrame->bones[n].matrix[0][0] - tmp);
            tmp = frame->bones[n].matrix[0][1];
            bones[n].matrix[0][1] = tmp + backlerp * (oldFrame->bones[n].matrix[0][1] - tmp);
            tmp = frame->bones[n].matrix[0][2];
            bones[n].matrix[0][2] = tmp + backlerp * (oldFrame->bones[n].matrix[0][2] - tmp);
            tmp = frame->bones[n].matrix[0][3];
            bones[n].matrix[0][3] = tmp + backlerp * (oldFrame->bones[n].matrix[0][3] - tmp);

            tmp = frame->bones[n].matrix[1][0];
            bones[n].matrix[1][0] = tmp + backlerp * (oldFrame->bones[n].matrix[1][0] - tmp);
            tmp = frame->bones[n].matrix[1][1];
            bones[n].matrix[1][1] = tmp + backlerp * (oldFrame->bones[n].matrix[1][1] - tmp);
            tmp = frame->bones[n].matrix[1][2];
            bones[n].matrix[1][2] = tmp + backlerp * (oldFrame->bones[n].matrix[1][2] - tmp);
            tmp = frame->bones[n].matrix[1][3];
            bones[n].matrix[1][3] = tmp + backlerp * (oldFrame->bones[n].matrix[1][3] - tmp);

            tmp = frame->bones[n].matrix[2][0];
            bones[n].matrix[2][0] = tmp + backlerp * (oldFrame->bones[n].matrix[2][0] - tmp);
            tmp = frame->bones[n].matrix[2][1];
            bones[n].matrix[2][1] = tmp + backlerp * (oldFrame->bones[n].matrix[2][1] - tmp);
            tmp = frame->bones[n].matrix[2][2];
            bones[n].matrix[2][2] = tmp + backlerp * (oldFrame->bones[n].matrix[2][2] - tmp);
            tmp = frame->bones[n].matrix[2][3];
            bones[n].matrix[2][3] = tmp + backlerp * (oldFrame->bones[n].matrix[2][3] - tmp);
		}
	}

	//
	// deform the vertexes by the lerped bones
	//
	mdrVertex_t* v = (mdrVertex_t *) ((unsigned char *)surface + surface->ofsVerts);
	int numVerts = surface->numVerts;
    for( j = 0; j < numVerts; j++ ) 
	{
		float tempVert[3] = {0, 0, 0};
        float tempNormal[3] = {0, 0, 0};
		
        mdrWeight_t	*w = v->weights;

        for ( k = 0 ; k < v->numWeights ; k++, w++ ) 
		{
			mdrBone_t* bone = bonePtr + w->boneIndex;
			
			tempVert[0] += w->boneWeight * ( DotProduct( bone->matrix[0], w->offset ) + bone->matrix[0][3] );
			tempVert[1] += w->boneWeight * ( DotProduct( bone->matrix[1], w->offset ) + bone->matrix[1][3] );
			tempVert[2] += w->boneWeight * ( DotProduct( bone->matrix[2], w->offset ) + bone->matrix[2][3] );
			
			tempNormal[0] += w->boneWeight * DotProduct( bone->matrix[0], v->normal );
			tempNormal[1] += w->boneWeight * DotProduct( bone->matrix[1], v->normal );
			tempNormal[2] += w->boneWeight * DotProduct( bone->matrix[2], v->normal );
		}

		tess.xyz[baseVertex + j][0] = tempVert[0];
		tess.xyz[baseVertex + j][1] = tempVert[1];
		tess.xyz[baseVertex + j][2] = tempVert[2];

		tess.normal[baseVertex + j][0] = tempNormal[0];
		tess.normal[baseVertex + j][1] = tempNormal[1];
		tess.normal[baseVertex + j][2] = tempNormal[2];

		tess.texCoords[baseVertex + j][0][0] = v->texCoords[0];
		tess.texCoords[baseVertex + j][0][1] = v->texCoords[1];

        
		//v = (mdrVertex_t *)&v->weights[v->numWeights];
        {
            mdrWeight_t* pTmp = &v->weights[v->numWeights];
            v = (mdrVertex_t *) pTmp;
        }
	}

	tess.numVertexes += surface->numVerts;
}


void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])( void *) =
{
	(void(*)(void*))RB_SurfaceBad,			// SF_BAD,
	(void(*)(void*))RB_SurfaceSkip,			// SF_Skip,
	(void(*)(void*))RB_SurfaceFace,			// SF_FACE,
	(void(*)(void*))RB_SurfaceGrid,			// SF_GRID,
	(void(*)(void*))RB_SurfaceTriangles,	// SF_TRIANGLES,
	(void(*)(void*))RB_SurfacePolychain,	// SF_POLY,
	(void(*)(void*))RB_SurfaceMesh,			// SF_MD3,
	(void(*)(void*))RB_MDRSurfaceAnim,		// SF_MDR,
	(void(*)(void*))RB_IQMSurfaceAnim,		// SF_IQM,
	(void(*)(void*))RB_SurfaceFlare,		// SF_FLARE,
	(void(*)(void*))RB_SurfaceEntity,		// SF_ENTITY
};
