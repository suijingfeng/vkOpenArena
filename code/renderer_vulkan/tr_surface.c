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
// tr_surf.c
#include "tr_flares.h"
#include "tr_globals.h"
#include "vk_image.h"
#include "tr_cvar.h"
#include "tr_backend.h"
#include "ref_import.h"
#include "RB_SurfaceAnim.h"
#include "RB_DrawTris.h"
#include "RB_DrawNormals.h"
#include "tr_surface.h"
#include "R_ShaderCommands.h"
#include "srfTriangles_type.h"
#include "srfPoly_type.h"
#include "srfSurfaceFace_type.h"

/*

  THIS ENTIRE FILE IS BACK END

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here if you don't want to
use the shader system.
*/
extern struct shaderCommands_s tess;

extern void RB_StageIteratorGeneric( shaderCommands_t * const pTess, VkBool32 isPortal ,VkBool32 is2D);

void RB_CheckOverflow(uint32_t verts, uint32_t indexes, struct shaderCommands_s* const pTess)
{
    if ( (pTess->numVertexes + verts >= SHADER_MAX_VERTEXES) ||
         (pTess->numIndexes + indexes >= SHADER_MAX_INDEXES) )
    {
        RB_EndSurface(pTess);

/*
        if ( verts >= SHADER_MAX_VERTEXES ) {
            ri.Error(ERR_DROP, "CheckOverflow: verts > MAX (%d > %d)",
                    verts, SHADER_MAX_VERTEXES );
        }
        if ( indexes >= SHADER_MAX_INDEXES ) {
            ri.Error(ERR_DROP, "CheckOverflow: indices > MAX (%d > %d)",
                    indexes, SHADER_MAX_INDEXES );
        }
*/
        RB_BeginSurface(pTess->shader, pTess->fogNum, pTess );
    }
}




void RB_AddQuadStampExt(const float origin[3], vec3_t left, vec3_t up, const uint8_t * const color,
        float s1, float t1, float s2, float t2 )
{

	RB_CheckOverflow( 4, 6, &tess );

	const uint32_t ndx0 = tess.numVertexes;
    const uint32_t ndx1 = ndx0 + 1;
    const uint32_t ndx2 = ndx0 + 2;
    const uint32_t ndx3 = ndx0 + 3;


	// triangle indexes for a simple quad
	tess.indexes[ tess.numIndexes ] = ndx0;
	tess.indexes[ tess.numIndexes + 1 ] = ndx1;
	tess.indexes[ tess.numIndexes + 2 ] = ndx3;

	tess.indexes[ tess.numIndexes + 3 ] = ndx3;
	tess.indexes[ tess.numIndexes + 4 ] = ndx1;
	tess.indexes[ tess.numIndexes + 5 ] = ndx2;

	tess.xyz[ndx0][0] = origin[0] + left[0] + up[0];
	tess.xyz[ndx0][1] = origin[1] + left[1] + up[1];
	tess.xyz[ndx0][2] = origin[2] + left[2] + up[2];

	tess.xyz[ndx1][0] = origin[0] - left[0] + up[0];
	tess.xyz[ndx1][1] = origin[1] - left[1] + up[1];
	tess.xyz[ndx1][2] = origin[2] - left[2] + up[2];

	tess.xyz[ndx2][0] = origin[0] - left[0] - up[0];
	tess.xyz[ndx2][1] = origin[1] - left[1] - up[1];
	tess.xyz[ndx2][2] = origin[2] - left[2] - up[2];

	tess.xyz[ndx3][0] = origin[0] + left[0] - up[0];
	tess.xyz[ndx3][1] = origin[1] + left[1] - up[1];
	tess.xyz[ndx3][2] = origin[2] + left[2] - up[2];


	// constant normal all the way around
    float normal[3] = { -backEnd.viewParms.or.axis[0][0],
                        -backEnd.viewParms.or.axis[0][1],
                        -backEnd.viewParms.or.axis[0][2]};
	
    tess.normal[ndx0][0] = normal[0];
    tess.normal[ndx0][1] = normal[1];
    tess.normal[ndx0][2] = normal[2];

    tess.normal[ndx1][0] = normal[0];
    tess.normal[ndx1][1] = normal[1];
    tess.normal[ndx1][2] = normal[2];

    tess.normal[ndx2][0] = normal[0];
    tess.normal[ndx2][1] = normal[1];
    tess.normal[ndx2][2] = normal[2];

    tess.normal[ndx3][0] = normal[0];
    tess.normal[ndx3][1] = normal[1];
    tess.normal[ndx3][2] = normal[2];
	
	// standard square texture coordinates
	tess.texCoords[ndx0][0][0] = s1;
    tess.texCoords[ndx0][1][0] = s1;
	tess.texCoords[ndx0][0][1] = t1;
    tess.texCoords[ndx0][1][1] = t1;

	tess.texCoords[ndx1][0][0] = s2;
    tess.texCoords[ndx1][1][0] = s2;
	tess.texCoords[ndx1][0][1] = t1;
    tess.texCoords[ndx1][1][1] = t1;

	tess.texCoords[ndx2][0][0] = s2;
    tess.texCoords[ndx2][1][0] = s2;
	tess.texCoords[ndx2][0][1] = t2;
    tess.texCoords[ndx2][1][1] = t2;

	tess.texCoords[ndx3][0][0] = s1;
    tess.texCoords[ndx3][1][0] = s1;
	tess.texCoords[ndx3][0][1] = t2;
    tess.texCoords[ndx3][1][1] = t2;

	// constant color all the way around
	// should this be identity and let the shader specify from entity?
    // for elimate strict alias warnning on gcc 4.8
    /*
	* ( unsigned int * ) &tess.vertexColors[ndx0] = 
	* ( unsigned int * ) &tess.vertexColors[ndx1] = 
	* ( unsigned int * ) &tess.vertexColors[ndx2] = 
    * ( unsigned int * ) &tess.vertexColors[ndx3] = 
	* ( unsigned int * ) color;
    */
    
    tess.vertexColors[ndx0][0] = color[0];
	tess.vertexColors[ndx0][1] = color[1];
	tess.vertexColors[ndx0][2] = color[2];
	tess.vertexColors[ndx0][3] = color[3];

    tess.vertexColors[ndx1][0] = color[0];
	tess.vertexColors[ndx1][1] = color[1];
	tess.vertexColors[ndx1][2] = color[2];
	tess.vertexColors[ndx1][3] = color[3];
    
    tess.vertexColors[ndx2][0] = color[0];
	tess.vertexColors[ndx2][1] = color[1];
	tess.vertexColors[ndx2][2] = color[2];
	tess.vertexColors[ndx2][3] = color[3];
    
    tess.vertexColors[ndx3][0] = color[0];
	tess.vertexColors[ndx3][1] = color[1];
	tess.vertexColors[ndx3][2] = color[2];
	tess.vertexColors[ndx3][3] = color[3];
    

	tess.numVertexes += 4;
	tess.numIndexes += 6;
}


static void RB_SurfaceSprite(const refEntity_t* const pEnt, const viewParms_t * const pViewPar)
{

	vec3_t left, up;

	// calculate the xyz locations for the four corners
	float radius = pEnt->radius;
	if ( pEnt->rotation == 0 )
    {
		VectorScale( pViewPar->or.axis[1], radius, left );
		VectorScale( pViewPar->or.axis[2], radius, up );
	}
    else
    {
		float ang = (float)(M_PI / 180.0) * pEnt->rotation ;
		// float s = sin( ang );
		// float c = cos( ang );

        float c_r = cos( ang ) * radius;
        float s_r = sin( ang ) * radius;

        float tmpLeft[3];
        float tmpUp[3];
        
		VectorScale( pViewPar->or.axis[1], c_r, tmpLeft );        
        VectorScale( pViewPar->or.axis[2], c_r, tmpUp );
        
        // float tmp1[3];
        // float tmp2[3];
        // VectorScale( pViewPar->or.axis[1], s_r, tmp1 );
        // VectorScale( pViewPar->or.axis[2], s_r, tmp2 );   
        // VectorAdd(tmpUp, tmp1, up);
        // VectorSubtract( tmpLeft, tmp2, left);
        VectorMA( tmpLeft, -s_r, pViewPar->or.axis[2], left );
        VectorMA( tmpUp, s_r, pViewPar->or.axis[1], up );
	}

	if ( pViewPar->isMirror )
    {
        left[0] = -left[0];
        left[1] = -left[1];
        left[2] = -left[2];
	}

	RB_AddQuadStampExt( pEnt->origin, left, up, 
            pEnt->shaderRGBA, 0.0f, 0.0f, 1.0f, 1.0f );
}



static void RB_SurfacePolychain( const srfPoly_t * const p )
{
	int	i;

    const uint32_t NV = p->numVerts;
    const uint32_t NI = p->numVerts - 2;

	RB_CheckOverflow( NV, 3*NI, &tess );
	
	// fan triangles into the tess array
	uint32_t vCount = tess.numVertexes;
    uint32_t vBase = tess.numVertexes;
	uint32_t iCount = tess.numIndexes;
    tess.numIndexes  += 3*NI;
	tess.numVertexes += NV;


	for( i = 0; i < NV; ++i )
    {
		// VectorCopy( p->verts[i].xyz, tess.xyz[numv] );
		tess.xyz[vCount][0] = p->verts[i].xyz[0];
		tess.xyz[vCount][1] = p->verts[i].xyz[1];
		tess.xyz[vCount][2] = p->verts[i].xyz[2];

        tess.texCoords[vCount][0][0] = p->verts[i].st[0];
		tess.texCoords[vCount][0][1] = p->verts[i].st[1];
		
        // *(int *)&tess.vertexColors[numv] = *(int *)p->verts[ i ].modulate;
        tess.vertexColors[vCount][0] = p->verts[i].modulate[0];
		tess.vertexColors[vCount][1] = p->verts[i].modulate[1];
		tess.vertexColors[vCount][2] = p->verts[i].modulate[2];
		tess.vertexColors[vCount][3] = p->verts[i].modulate[3];
  
        //memcpy(tess.vertexColors[numv], p->verts[ i ].modulate, 4);
		++vCount;
	}

	// generate fan indexes into the tess array

	for ( i = 0; i < NI; ++i )
    {
		tess.indexes[iCount++] = vBase;
		tess.indexes[iCount++] = vBase + i + 1;
		tess.indexes[iCount++] = vBase + i + 2;
	}
}



static void RB_SurfaceTriangles( srfTriangles_t *srf )
{
	int			i;
	drawVert_t	*dv;
	float		*xyz, *normal, *texCoords;
	byte		*color;
	int			dlightBits;
	qboolean	needsNormal;

	dlightBits = srf->dlightBits;
	tess.dlightBits |= dlightBits;

	RB_CheckOverflow( srf->numVerts, srf->numIndexes, &tess );

	for ( i = 0 ; i < srf->numIndexes ; i += 3 ) {
		tess.indexes[ tess.numIndexes + i + 0 ] = tess.numVertexes + srf->indexes[ i + 0 ];
		tess.indexes[ tess.numIndexes + i + 1 ] = tess.numVertexes + srf->indexes[ i + 1 ];
		tess.indexes[ tess.numIndexes + i + 2 ] = tess.numVertexes + srf->indexes[ i + 2 ];
	}
	tess.numIndexes += srf->numIndexes;

	dv = srf->verts;
	xyz = tess.xyz[ tess.numVertexes ];
	normal = tess.normal[ tess.numVertexes ];
	texCoords = tess.texCoords[ tess.numVertexes ][0];
	color = tess.vertexColors[ tess.numVertexes ];
	needsNormal = tess.shader->needsNormal;

	for ( i = 0 ; i < srf->numVerts ; i++, dv++, xyz += 4, normal += 4, texCoords += 4, color += 4 ) {
		xyz[0] = dv->xyz[0];
		xyz[1] = dv->xyz[1];
		xyz[2] = dv->xyz[2];

		if ( needsNormal ) {
			normal[0] = dv->normal[0];
			normal[1] = dv->normal[1];
			normal[2] = dv->normal[2];
		}

		texCoords[0] = dv->st[0];
		texCoords[1] = dv->st[1];

		texCoords[2] = dv->lightmap[0];
		texCoords[3] = dv->lightmap[1];
        
        // *(int *)color = *(int *)dv->color;
        memcpy(color, dv->color, 4);
	}

	for ( i = 0 ; i < srf->numVerts ; i++ ) {
		tess.vertexDlightBits[ tess.numVertexes + i] = dlightBits;
	}

	tess.numVertexes += srf->numVerts;
}


static void RB_SurfaceBeam( refEntity_t * const e ) 
{
	vec3_t direction, normalized_direction;
	vec3_t oldorigin, origin;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];
    
    ri.Printf(PRINT_ALL, "RB_SurfaceBeam() get called! ");

	if ( VectorNormalize( normalized_direction ) == 0 )
		return;
}

//================================================================================

static void DoRailCore( const vec3_t start, const vec3_t end, const vec3_t up, float len, float spanWidth,
        const unsigned char* const pRGBA )
{
	float t = len / 256.0f;

//	int vbase = tess.numVertexes;

    const uint32_t n0 = tess.numVertexes;
    const uint32_t n1 = tess.numVertexes + 1;
    const uint32_t n2 = tess.numVertexes + 2;
    const uint32_t n3 = tess.numVertexes + 3;
    const uint32_t idx = tess.numIndexes;
    tess.numIndexes += 6;
    tess.numVertexes += 4;

    
    tess.indexes[idx  ] = n0;
	tess.indexes[idx+1] = n1;
	tess.indexes[idx+2] = n2;
	tess.indexes[idx+3] = n2;
	tess.indexes[idx+4] = n1;
	tess.indexes[idx+5] = n3;


	// FIXME: use quad stamp?
//	float spanWidth2 = -spanWidth;
//	VectorMA( start, spanWidth, up, tess.xyz[n0] );
//	VectorMA( start, spanWidth2, up, tess.xyz[n1] );
//	VectorMA( end, spanWidth, up, tess.xyz[n2] );
//	VectorMA( end, spanWidth2, up, tess.xyz[n3] );

    float tmp[3];
    VectorScale(up, spanWidth, tmp);
    
    VectorAdd( start, tmp, tess.xyz[n0] );
    VectorSubtract( start, tmp, tess.xyz[n1] );
	VectorAdd( end, tmp, tess.xyz[n2] );
	VectorSubtract( end, tmp, tess.xyz[n3] );


	tess.texCoords[n0][0][0] = 0.0f;
	tess.texCoords[n0][0][1] = 0.0f;

    tess.texCoords[n1][0][0] = 0.0f;
	tess.texCoords[n1][0][1] = 1.0f;
	
    tess.texCoords[n2][0][0] = t;
	tess.texCoords[n2][0][1] = 0.0f;
	
    tess.texCoords[n3][0][0] = t;
	tess.texCoords[n3][0][1] = 1.0f;


	tess.vertexColors[n0][0] = pRGBA[0] * 0.25f;
	tess.vertexColors[n0][1] = pRGBA[1] * 0.25f;
	tess.vertexColors[n0][2] = pRGBA[2] * 0.25f;

    tess.vertexColors[n1][0] = pRGBA[0];
	tess.vertexColors[n1][1] = pRGBA[1];
	tess.vertexColors[n1][2] = pRGBA[2];

	tess.vertexColors[n2][0] = pRGBA[0];
	tess.vertexColors[n2][1] = pRGBA[1];
	tess.vertexColors[n2][2] = pRGBA[2];

	tess.vertexColors[n3][0] = pRGBA[0];
	tess.vertexColors[n3][1] = pRGBA[1];
	tess.vertexColors[n3][2] = pRGBA[2];
}



static void RB_SurfaceRailRings(const refEntity_t * const e)
{
	vec3_t		dir;
	vec3_t		right, up;

	// compute variables
	VectorSubtract( e->origin, e->oldorigin, dir );
	int len = VectorNormalize( dir );
	MakeTwoPerpVectors( dir, right, up );
	
    int numSegs = ( len ) / r_railSegmentLength->value;
	if ( numSegs <= 0 ) {
		numSegs = 1;
	}

	VectorScale( dir, r_railSegmentLength->value, dir );

    //
	// DoRailDiscs( numSegs, e->oldorigin, vec, right, up );
    // DoRailDiscs( int numSegs, const vec3_t start, const vec3_t dir, const vec3_t right, const vec3_t up )
	
	vec3_t	pos[4];
	vec3_t	v;
	int		spanWidth = r_railWidth->integer;
	float		scale = 0.25;

	if ( numSegs > 1 )
		--numSegs;

    int i;

	for ( i = 0; i < 4; ++i )
	{
		float c = cos( DEG2RAD( 45 + i * 90 ) );
		float s = sin( DEG2RAD( 45 + i * 90 ) );
		v[0] = ( right[0] * c + up[0] * s ) * scale * spanWidth;
		v[1] = ( right[1] * c + up[1] * s ) * scale * spanWidth;
		v[2] = ( right[2] * c + up[2] * s ) * scale * spanWidth;
		VectorAdd( e->oldorigin, v, pos[i] );

		if ( numSegs > 1 )
		{
			// offset by 1 segment if we're doing a long distance shot
			VectorAdd( pos[i], dir, pos[i] );
		}
	}

	for ( i = 0; i < numSegs; ++i )
	{
		int j;

		RB_CheckOverflow( 4, 6, &tess );

        uint32_t nVert = tess.numVertexes;
        uint32_t nIdx = tess.numIndexes;
        tess.numVertexes += 4;
        tess.numIndexes += 6;

		tess.indexes[nIdx++] = nVert + 0;
		tess.indexes[nIdx++] = nVert + 1;
		tess.indexes[nIdx++] = nVert + 3;
		tess.indexes[nIdx++] = nVert + 3;
		tess.indexes[nIdx++] = nVert + 1;
		tess.indexes[nIdx++] = nVert + 2;


		for ( j = 0; j < 4; ++j )
		{
			VectorCopy( pos[j], tess.xyz[nVert] );
			tess.texCoords[nVert][0][0] = ( j < 2 );
			tess.texCoords[nVert][0][1] = ( j && j != 3 );
			tess.vertexColors[nVert][0] = e->shaderRGBA[0];
			tess.vertexColors[nVert][1] = e->shaderRGBA[1];
			tess.vertexColors[nVert][2] = e->shaderRGBA[2];
			++nVert;
			VectorAdd( pos[j], dir, pos[j] );
		}
	}

}


static void RB_SurfaceRailCore(const refEntity_t * const e, const viewParms_t * const pViewPar)
{
	vec3_t		right;
	vec3_t		vec;

	vec3_t		v1, v2;

	//VectorCopy( e->oldorigin, start );
	//VectorCopy( e->origin, end );


	// compute side vector
	VectorSubtract( e->oldorigin, pViewPar->or.origin, v1 );
	VectorSubtract( e->origin, pViewPar->or.origin, v2 );

    // VectorSubtract( e->origin, e->oldorigin, vec );
    // =>
    VectorSubtract( v2, v1, vec );
	
    VectorNormalize( v1 );
	VectorNormalize( v2 );
	VectorCross( v1, v2, right );
	VectorNormalize( right );

	DoRailCore( e->oldorigin, e->origin, right, VectorNormalize( vec ), r_railCoreWidth->integer,
            e->shaderRGBA);
}



static void RB_SurfaceLightningBolt( const refEntity_t * const e, const viewParms_t * const pViewPar)
{
	vec3_t		right;
	vec3_t		vec;
	vec3_t		v1, v2;

	// compute variables
	VectorSubtract( e->oldorigin, e->origin, vec );
	int len = VectorNormalize( vec );

	// compute side vector
	VectorSubtract( e->origin, pViewPar->or.origin, v1 );
	VectorNormalize( v1 );
	VectorSubtract( e->oldorigin, pViewPar->or.origin, v2 );
	VectorNormalize( v2 );
	VectorCross( v1, v2, right );
	VectorNormalize( right );
	uint32_t i;
	for ( i = 0 ; i < 4 ; ++i )
    {
		vec3_t	temp;

		DoRailCore( e->origin, e->oldorigin, right, len, 16, e->shaderRGBA);
		PointRotateAroundVector( temp, vec, right, 45 );
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
        // refinement step to get a little more precision.  This seems to yeild results
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
        VectorNorm(normals[0]);
        normals++;
    }
#endif

}


static void LerpMeshVertexes (md3Surface_t *surf, float backlerp) 
{
	short	*oldXyz, *newXyz, *oldNormals, *newNormals;
	float	*outXyz, *outNormal;
	float	oldXyzScale, newXyzScale;
	float	oldNormalScale, newNormalScale;
	int		vertNum;
	unsigned lat, lng;

	outXyz = tess.xyz[tess.numVertexes];
	outNormal = tess.normal[tess.numVertexes];

	newXyz = (short *)((byte *)surf + surf->ofsXyzNormals)
		+ (backEnd.currentEntity->e.frame * surf->numVerts * 4);
	newNormals = newXyz + 3;

	newXyzScale = MD3_XYZ_SCALE * (1.0 - backlerp);
	newNormalScale = 1.0 - backlerp;

	int numVerts = surf->numVerts;

	if ( backlerp == 0 )
    {
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
	}
    else
    {
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

/*
=============
RB_SurfaceMesh
=============
*/
void RB_SurfaceMesh(md3Surface_t *surface)
{
	int				j;
	float			backlerp;
	float			*texCoords;
	int				numVerts;

	if (  backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame ) {
		backlerp = 0;
	} else  {
		backlerp = backEnd.currentEntity->e.backlerp;
	}

	RB_CheckOverflow( surface->numVerts, surface->numTriangles*3, &tess );

	LerpMeshVertexes (surface, backlerp);

	int * triangles = (int *) ((byte *)surface + surface->ofsTriangles);
	int indexes = surface->numTriangles * 3;
	int Bob = tess.numIndexes;
	int Doug = tess.numVertexes;
	for (j = 0 ; j < indexes ; j++) {
		tess.indexes[Bob + j] = Doug + triangles[j];
	}
	tess.numIndexes += indexes;

	texCoords = (float *) ((byte *)surface + surface->ofsSt);

	numVerts = surface->numVerts;
	for ( j = 0; j < numVerts; j++ ) {
		tess.texCoords[Doug + j][0][0] = texCoords[j*2+0];
		tess.texCoords[Doug + j][0][1] = texCoords[j*2+1];
		// FIXME: fill in lightmapST for completeness?
	}

	tess.numVertexes += surface->numVerts;
}



static void RB_SurfaceFace( srfSurfaceFace_t * pSurf )
{
	int			i;

	float		*normal;
    int			numPoints;
	int			dlightBits;

	RB_CheckOverflow( pSurf->numPoints, pSurf->numIndices, &tess );

	dlightBits = pSurf->dlightBits;
	tess.dlightBits |= dlightBits;

	uint32_t * indices = ( uint32_t * ) ( ( ( char  * ) pSurf ) + pSurf->ofsIndices );

	uint32_t Bob = tess.numVertexes;
	uint32_t * tessIndexes = tess.indexes + tess.numIndexes;

	for ( i = pSurf->numIndices-1; i >= 0; --i )
    {
		tessIndexes[i] = indices[i] + Bob;
	}

	tess.numIndexes += pSurf->numIndices;

	float* v = pSurf->points[0];

	uint32_t ndx = tess.numVertexes;

	numPoints = pSurf->numPoints;

	if ( tess.shader->needsNormal ) {
		normal = pSurf->plane.normal;
		for ( i = 0, ndx = tess.numVertexes; i < numPoints; i++, ndx++ ) {
			VectorCopy( normal, tess.normal[ndx] );
		}
	}

	for ( i = 0, v = pSurf->points[0], ndx = tess.numVertexes; i < numPoints; i++, v += VERTEXSIZE, ndx++ ) {
		VectorCopy( v, tess.xyz[ndx]);
		tess.texCoords[ndx][0][0] = v[3];
		tess.texCoords[ndx][0][1] = v[4];
		tess.texCoords[ndx][1][0] = v[5];
		tess.texCoords[ndx][1][1] = v[6];
		//* ( unsigned int * ) &tess.vertexColors[ndx] = * ( unsigned int * ) &v[7];
        memcpy(tess.vertexColors[ndx], &v[7], 4);
		tess.vertexDlightBits[ndx] = dlightBits;
	}


	tess.numVertexes += pSurf->numPoints;
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
static void RB_SurfaceGrid( srfGridMesh_t *cv )
{
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
	rows = 0;
	while ( used < lodHeight - 1 )
    {
		// see how many rows of both verts and indexes we can add without overflowing
		do {
			vrows = ( SHADER_MAX_VERTEXES - tess.numVertexes ) / lodWidth;
			irows = ( SHADER_MAX_INDEXES - tess.numIndexes ) / ( lodWidth * 6 );

			// if we don't have enough space for at least one strip, flush the buffer
			if ( vrows < 2 || irows < 1 )
            {
				RB_EndSurface(&tess);
				RB_BeginSurface(tess.shader, tess.fogNum, &tess );
			}
            else
            {
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
				//* ( unsigned int * ) color = * ( unsigned int * ) dv->color;
				memcpy(color, dv->color, 4);
                *vDlightBits++ = dlightBits;
				xyz += 4;
				normal += 4;
				texCoords += 4;
				color += 4;
			}
		}


		// add the indexes
		{
			int h = rows - 1;
			int w = lodWidth - 1;
			int numIndexes = tess.numIndexes;

			for (i = 0 ; i < h ; ++i)
            {
				for (j = 0 ; j < w ; ++j)
                {			
					// vertex order to be reckognized as tristrips
					int v1 = numVertexes + i*lodWidth + j + 1;
					int v2 = v1 - 1;
					int v3 = v2 + lodWidth;
					int v4 = v3 + 1;

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
void RB_SurfaceAxis( int val )
{
    // FIXME: implement this
    //	VK_Bind( tr.whiteImage );
	ri.Printf( PRINT_ALL, "SurfaceAxis:%d \n", val);
}

//===========================================================================

/*
====================
Entities that have a single procedurally generated surface
====================
*/
void RB_SurfaceEntity( void * useless )
{
	switch( backEnd.currentEntity->e.reType )
    {
	case RT_SPRITE:
		RB_SurfaceSprite(&backEnd.currentEntity->e, &backEnd.viewParms);
		break;
	case RT_BEAM:
		RB_SurfaceBeam(&backEnd.currentEntity->e);
		break;
	case RT_RAIL_CORE:
		RB_SurfaceRailCore(&backEnd.currentEntity->e, &backEnd.viewParms);
		break;
	case RT_RAIL_RINGS:
        // its dont dependent on view parameter
		RB_SurfaceRailRings(&backEnd.currentEntity->e);
		break;
	case RT_LIGHTNING:
		RB_SurfaceLightningBolt(&backEnd.currentEntity->e, &backEnd.viewParms);
		break;
    case RT_PORTALSURFACE:
        ri.Printf(PRINT_ALL, "RT_PORTALSURFACE: just info for portals.\n");
        break;
    case RT_MODEL:
        RB_SurfaceAxis(RT_MODEL);
        break;
    case RT_POLY:
        RB_SurfaceAxis(RT_POLY);
        break;
	default:
		break;
	}
}



void RB_SurfaceBad( surfaceType_t *surfType )
{
	ri.Printf( PRINT_ALL, "Bad surface tesselated.\n" );
}

void RB_SurfaceSkip( surfaceType_t * psurf )
{
    
}

const Fn_RB_SurfaceTable_t rb_surfaceTable[SF_NUM_SURFACE_TYPES] =
{
	(void (* )(void *))RB_SurfaceBad,			// SF_BAD, 
	(void (* )(void *))RB_SurfaceSkip,			// SF_SKIP, 
	(void (* )(void *))RB_SurfaceFace,			// SF_FACE,
	(void (* )(void *))RB_SurfaceGrid,			// SF_GRID,
	(void (* )(void *))RB_SurfaceTriangles,	// SF_TRIANGLES,
	(void (* )(void *))RB_SurfacePolychain,	// SF_POLY,
	(void (* )(void *))RB_SurfaceMesh,			// SF_MD3,
	(void (* )(void *))RB_MDRSurfaceAnim,		// SF_MDR,
	(void (* )(void *))RB_IQMSurfaceAnim,		// SF_IQM,
	(void (* )(void *))RB_SurfaceFlare,		// SF_FLARE,
	(void (* )(void *))RB_SurfaceEntity,		// SF_ENTITY
};

/*
==============
We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due
to overflow.
==============
*/
void RB_BeginSurface(struct shader_s * const pShader, int fogNum, struct shaderCommands_s * const pTess )
{
	struct shader_s * pState = (pShader->remappedShader) ? pShader->remappedShader : pShader;

	pTess->numIndexes = 0;
	pTess->numVertexes = 0;
	pTess->shader = pState;
	pTess->fogNum = fogNum;
	pTess->dlightBits = 0;		// will be OR'd in by surface functions
	pTess->xstages = pState->stages;
	pTess->numPasses = pState->numUnfoggedPasses;

	pTess->shaderTime = R_GetRefFloatTime() - pTess->shader->timeOffset;
	if (pTess->shader->clampTime && pTess->shaderTime >= pTess->shader->clampTime)
    {
		pTess->shaderTime = pTess->shader->clampTime;
	}
}


void RB_EndSurface( shaderCommands_t * const pTess )
{
	// for debugging of sort order issues, stop rendering after a given sort value
	if ( r_debugSort->integer && r_debugSort->integer < pTess->shader->sort ) {
		return;
	}

	//
	// update performance counters
	//
    R_UpdatePerformanceCounters(pTess->numVertexes, pTess->numIndexes, pTess->numPasses);

	//
	// call off to shader specific tess end function
	//
    if (pTess->shader->isSky) {
		RB_StageIteratorSky(pTess);
    }


	RB_StageIteratorGeneric(pTess, backEnd.viewParms.isPortal, backEnd.projection2D);
    
	//
	// draw debugging stuff
	//
	if ( r_showtris->integer )
    {
		RB_DrawTris(pTess, backEnd.viewParms.isMirror, backEnd.projection2D);
	}
	if ( r_shownormals->integer )
    {
		RB_DrawNormals(pTess, backEnd.viewParms.isPortal, backEnd.projection2D);
	}
	// clear shader so we can tell we don't have any unclosed surfaces
	pTess->numIndexes = 0;
	pTess->numVertexes = 0;
}
