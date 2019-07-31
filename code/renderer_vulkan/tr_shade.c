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
// tr_shade.c
/*

  THIS ENTIRE FILE IS BACK END

  This file deals with applying shaders to surface data in the tess struct.
*/

#include "tr_globals.h"
#include "ref_import.h"
#include "tr_backend.h"
#include "tr_cvar.h"
#include "R_ShaderCommands.h"

#include "tr_fog.h"
#include "tr_surface.h"
#include "tr_shade.h"
#include "tr_noise.h"
#include "RB_DeformGeometry.h"  // RB_WaveValue

extern struct shaderCommands_s tess;
extern void R_GetFogArray(struct fog_s * *const ppFogs, uint32_t* const pNum);


static void RB_CalcTransformTexCoords( const texModInfo_t * const pTmi, float * st )
{
	uint32_t i;
	uint32_t const nVert = tess.numVertexes;
	for ( i = 0; i < nVert; ++i, st += 2 )
	{
		float s = st[0];
		float t = st[1];

		st[0] = s * pTmi->matrix[0][0] + t * pTmi->matrix[1][0] + pTmi->translate[0];
		st[1] = s * pTmi->matrix[0][1] + t * pTmi->matrix[1][1] + pTmi->translate[1];
	}
}

static void RB_CalcStretchTexCoords( const waveForm_t *wf, float *st )
{
	float p = 1.0f / RB_WaveValue( wf->func, wf->base, wf->amplitude, wf->phase, wf->frequency );
	texModInfo_t tmi;

	tmi.matrix[0][0] = p;
	tmi.matrix[1][0] = 0;
	tmi.translate[0] = 0.5f - 0.5f * p;

	tmi.matrix[0][1] = 0;
	tmi.matrix[1][1] = p;
	tmi.translate[1] = 0.5f - 0.5f * p;

	RB_CalcTransformTexCoords( &tmi, st );
}



/*
====================================================================

COLORS

====================================================================
*/


static void RB_CalcColorFromEntity( unsigned char (* const dstColors)[4], const uint32_t nVerts )
{
	if ( backEnd.currentEntity )
	{
		uint32_t i;
		
        // unsigned char srColor[4];

        // memcpy(srColor, backEnd.currentEntity->e.shaderRGBA, 4);
        for ( i = 0; i < nVerts; ++i)
		{
			dstColors[i][0]=backEnd.currentEntity->e.shaderRGBA[0];
			dstColors[i][1]=backEnd.currentEntity->e.shaderRGBA[1];
			dstColors[i][2]=backEnd.currentEntity->e.shaderRGBA[2];
			dstColors[i][3]=backEnd.currentEntity->e.shaderRGBA[3];
            // memcpy(dstColors[i], srColor, 4);
		}
	}	
}



static void RB_CalcColorFromOneMinusEntity( unsigned char (*dstColors)[4] )
{
	if ( backEnd.currentEntity )
	{
    	unsigned char invModulate[4];

        invModulate[0] = 255 - backEnd.currentEntity->e.shaderRGBA[0];
        invModulate[1] = 255 - backEnd.currentEntity->e.shaderRGBA[1];
        invModulate[2] = 255 - backEnd.currentEntity->e.shaderRGBA[2];
        invModulate[3] = 255 - backEnd.currentEntity->e.shaderRGBA[3];
        // this trashes alpha, but the AGEN block fixes it
        
        uint32_t nVerts = tess.numVertexes; 
        uint32_t i;       
        for ( i = 0; i < nVerts; ++i )
        {
            memcpy(dstColors[i], invModulate, 4);
        }
    }
}


static void RB_CalcAlphaFromEntity(const uint32_t nVert, unsigned char (* const dstColors)[4])
{
	uint32_t i;

	if ( !backEnd.currentEntity )
		return;

    uint8_t A = backEnd.currentEntity->e.shaderRGBA[3];
	for ( i = 0; i < nVert; ++i)
	{
		dstColors[i][3] = A;
	}
}

static void RB_CalcAlphaFromOneMinusEntity(const uint32_t nVert, unsigned char (* const dstColors)[4])
{
	uint32_t i;

	if ( !backEnd.currentEntity )
		return;

    uint8_t A = backEnd.currentEntity->e.shaderRGBA[3];
	for ( i = 0; i < nVert; ++i)
	{
		dstColors[i][3] = 0xff - A;
	}
}


static void RB_CalcWaveColor( const waveForm_t* wf, const uint32_t nVert, unsigned char (* const dstColors)[4])
{
    int32_t color = 255.0f * RB_WaveValue( wf->func, wf->base, wf->amplitude, wf->phase, wf->frequency );

    if( color < 0 )
		color = 0;
	else if( color > 255 )
		color = 255;

    uint32_t i;  
	for(i = 0; i < nVert; ++i)
    {
		dstColors[i][0] = color;
    	dstColors[i][1] = color;
        dstColors[i][2] = color;
		dstColors[i][3] = 255;
    }
}


static void RB_CalcWaveAlpha( const waveForm_t *wf, const uint32_t nVert , unsigned char (*const pDstColors)[4] )
{
	int i;

	int v = 255 * RB_WaveValue( wf->func, wf->base, wf->amplitude, wf->phase, wf->frequency );
    if(v < 0) {
        v = 0;
    }
    else if(v > 255)
    {
        v = 255;
    }
    
	for ( i = 0; i < nVert; ++i)
	{
		pDstColors[i][3] = v;
	}
}


static void RB_CalcModulateColorsByFog( unsigned char *colors )
{
	uint32_t i;
	float texCoords[SHADER_MAX_VERTEXES][2];

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords( texCoords, tess.numVertexes );

	for ( i = 0; i < tess.numVertexes; ++i, colors += 4 )
    {
		float f = 1.0 - R_FogFactor( texCoords[i][0], texCoords[i][1] );
		colors[0] *= f;
		colors[1] *= f;
		colors[2] *= f;
	}
}


static void RB_CalcModulateAlphasByFog( unsigned char *colors )
{
	int		i;
	float	texCoords[SHADER_MAX_VERTEXES][2];

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords( texCoords, tess.numVertexes );

	for ( i = 0; i < tess.numVertexes; i++, colors += 4 ) {
		float f = 1.0 - R_FogFactor( texCoords[i][0], texCoords[i][1] );
		colors[3] *= f;
	}
}


static void RB_CalcModulateRGBAsByFog( unsigned char *colors )
{
	int		i;
	float	texCoords[SHADER_MAX_VERTEXES][2];

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords( texCoords, tess.numVertexes );

	for ( i = 0; i < tess.numVertexes; i++, colors += 4 )
    {
		float f = 1.0 - R_FogFactor( texCoords[i][0], texCoords[i][1] );
		colors[0] *= f;
		colors[1] *= f;
		colors[2] *= f;
		colors[3] *= f;
	}
}


/*
====================================================================

TEX COORDS

====================================================================
*/

/*
========================
To do the clipped fog plane really correctly, we should use
projected textures, but I don't trust the drivers and it
doesn't fit our shader data.
========================
*/


static void RB_CalcEnvironmentTexCoordsJO( float *st ) 
{
	int			i;
	float		*v, *normal;
	vec3_t		viewer;
	float		d;

	v = tess.xyz[0];
	normal = tess.normal[0];

	if (backEnd.currentEntity && backEnd.currentEntity->e.renderfx&RF_FIRST_PERSON)	//this is a view model so we must use world lights instead of vieworg
	{
		for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2 ) 
		{
			d = DotProduct (normal, backEnd.currentEntity->lightDir);
			st[0] = normal[0]*d - backEnd.currentEntity->lightDir[0];
			st[1] = normal[1]*d - backEnd.currentEntity->lightDir[1];
		}
	} else {	//the normal way
		for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2 ) 
		{
			VectorSubtract (backEnd.or.viewOrigin, v, viewer);
			R_FastNormalize1f(viewer);

			d = DotProduct (normal, viewer);
			st[0] = normal[0]*d - 0.5*viewer[0];
			st[1] = normal[1]*d - 0.5*viewer[1];
		}
	}
}

/*
** RB_CalcEnvironmentTexCoords
*/
static void RB_CalcEnvironmentTexCoords( float *st ) 
{
	int			i;
	float		*v, *normal;
	vec3_t		viewer, reflected;
	float		d;

	v = tess.xyz[0];
	normal = tess.normal[0];

	for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2 ) 
	{
		VectorSubtract (backEnd.or.viewOrigin, v, viewer);
		VectorNorm(viewer);

		d = DotProduct (normal, viewer);

		reflected[0] = normal[0]*2*d - viewer[0];
		reflected[1] = normal[1]*2*d - viewer[1];
		reflected[2] = normal[2]*2*d - viewer[2];

		st[0] = 0.5 + reflected[1] * 0.5;
		st[1] = 0.5 - reflected[2] * 0.5;
	}
}

/*
** RB_CalcCelTexCoords
	Butchered from JediOutcast source, note that this is not the same method as ZEQ2.
*/
static void RB_CalcCelTexCoords( float *st ) 
{
	int			i;
	float		*v, *normal;
	vec3_t		viewer, reflected, lightdir, directedLight;
	float		d, l, p;


	v = tess.xyz[0];
	normal = tess.normal[0];

	VectorCopy(backEnd.currentEntity->lightDir, lightdir);
	VectorCopy(backEnd.currentEntity->directedLight, directedLight);
	float light = (directedLight[0] + directedLight[1] + directedLight[2] / 3);
	p = 1.0f - (light / 255);

	for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2 ) 
	{
		VectorSubtract (backEnd.or.viewOrigin, v, viewer);
		R_FastNormalize1f(viewer);

		d = DotProduct (normal, viewer);

		l = DotProduct (normal, backEnd.currentEntity->lightDir);

		if (d < 0)d = 0;
		if (l < 0)l = 0;

		if (d < p)d = p;
		if (l < p)l = p;

		reflected[0] = normal[0]*1*(d+l) - (viewer[0] + lightdir[0] );
		reflected[1] = normal[1]*1*(d+l) - (viewer[1] + lightdir[1] );
		reflected[2] = normal[2]*1*(d+l) - (viewer[2] + lightdir[2] );

		st[0] = 0.5 + reflected[1] * 0.5;
		st[1] = 0.5 - reflected[2] * 0.5;

	}
}



/*
** RB_CalcEnvironmentCelShadeTexCoords
**
** RiO; celshade 1D environment map
*/


static void RB_CalcEnvironmentCelShadeTexCoords( float *st ) 
{
    int    i;
    float  *v, *normal;
    vec3_t lightDir;

    normal = tess.normal[0];
	v = tess.xyz[0];

	// Calculate only once
//	VectorCopy( backEnd.currentEntity->lightDir, lightDir );
//	if ( backEnd.currentEntity == &tr.worldEntity )
//		VectorSubtract( lightOrigin, v, lightDir );
//	else
	
    VectorCopy( backEnd.currentEntity->lightDir, lightDir );
	R_FastNormalize1f( lightDir );

    for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2 )
    {
		float d= DotProduct( normal, lightDir );
		st[0] = 0.5 + d * 0.5;
		st[1] = 0.5;
    }
}

/*
** RB_CalcTurbulentTexCoords
*/
void RB_CalcTurbulentTexCoords( const waveForm_t *wf, float *st )
{
	int i;
	float now;

	now = ( wf->phase + tess.shaderTime * wf->frequency );

	for ( i = 0; i < tess.numVertexes; i++, st += 2 )
	{
		float s = st[0];
		float t = st[1];

		st[0] = s + tr.sinTable[ ( ( int ) ( ( ( tess.xyz[i][0] + tess.xyz[i][2] )* 1.0/128 * 0.125 + now ) * FUNCTABLE_SIZE ) ) & ( FUNCTABLE_MASK ) ] * wf->amplitude;
		st[1] = t + tr.sinTable[ ( ( int ) ( ( tess.xyz[i][1] * 1.0/128 * 0.125 + now ) * FUNCTABLE_SIZE ) ) & ( FUNCTABLE_MASK ) ] * wf->amplitude;
	}
}

/*
** RB_CalcScaleTexCoords
*/
void RB_CalcScaleTexCoords( const float scale[2], float *st )
{
	int i;

	for ( i = 0; i < tess.numVertexes; i++, st += 2 )
	{
		st[0] *= scale[0];
		st[1] *= scale[1];
	}
}

/*
** RB_CalcScrollTexCoords
*/
void RB_CalcScrollTexCoords( const float scrollSpeed[2], float *st )
{
	int i;
	float timeScale = tess.shaderTime;
	float adjustedScrollS, adjustedScrollT;

	adjustedScrollS = scrollSpeed[0] * timeScale;
	adjustedScrollT = scrollSpeed[1] * timeScale;

	// clamp so coordinates don't continuously get larger, causing problems
	// with hardware limits
	adjustedScrollS = adjustedScrollS - floor( adjustedScrollS );
	adjustedScrollT = adjustedScrollT - floor( adjustedScrollT );

	for ( i = 0; i < tess.numVertexes; i++, st += 2 )
	{
		st[0] += adjustedScrollS;
		st[1] += adjustedScrollT;
	}
}



static void RB_CalcRotateTexCoords( float degsPerSecond, float *st )
{
	float timeScale = tess.shaderTime;
	texModInfo_t tmi;

	float degs = -degsPerSecond * timeScale;
	int index = degs * ( FUNCTABLE_SIZE / 360.0f );

	float sinValue = tr.sinTable[ index & FUNCTABLE_MASK ];
	float cosValue = tr.sinTable[ ( index + FUNCTABLE_SIZE / 4 ) & FUNCTABLE_MASK ];

	tmi.matrix[0][0] = cosValue;
	tmi.matrix[1][0] = -sinValue;
	tmi.translate[0] = 0.5 - 0.5 * cosValue + 0.5 * sinValue;

	tmi.matrix[0][1] = sinValue;
	tmi.matrix[1][1] = cosValue;
	tmi.translate[1] = 0.5 - 0.5 * sinValue - 0.5 * cosValue;

	RB_CalcTransformTexCoords( &tmi, st );
}


// TODO: refactor. There is a loop in there for now

void RB_CalcAtlasTexCoords( const atlas_t *at, float *st )
{
	texModInfo_t tmi;
	int w = (int)at->width;	
	int h = (int)at->height;

	int framex = 0;
    int framey = 0;

	// modes:
	// 0 - static / animated
	// 1 - entity alpha (i.e. cgame rocket smoke)

	if (at->mode == 1)	// follow alpha modulation
	{
		int frametotal = w * h;
		float alha = ((0.25+backEnd.currentEntity->e.shaderRGBA[3]) / (tr.identityLight * 256.0f));
		int framethere = frametotal - ((frametotal * alha));
		int f;
        framex = 0;
        for(f=0; f<framethere; f++)
        {
            framex +=1;

            if (framex >= w)
            {
                framey +=1;	// next row!
                framex = 0; // reset column
            }
        }
	}
	else	// static/animated
	{
		// Process frame sequence for animation
		
		{
			int framethere = (tess.shaderTime * at->fps) + at->frame;			

            int f;
            framex = 0;
            for(f=0; f<framethere; f++)
            {
                framex +=1;

                if (framex >= w){
                    framey +=1;	// next row!
                    framex = 0; // reset column
                }
                if (framey >= h){
                    framey = 0; // reset row
                    framex = 0; // reset column
                }
            }
		}
	}

	
	// now use that information to alter our coordinates

	tmi.matrix[0][0] = 1.0f / w;
	tmi.matrix[1][0] = 0;
	//tmi.matrix[2][0] = 0;
	tmi.translate[0] = ((1.0f / w) * framex);

	tmi.matrix[0][1] = 0;
	tmi.matrix[1][1] = 1.0f / h;
	//tmi.matrix[2][1] = 0;
	tmi.translate[1] = ((1.0f / h) * framey);

	RB_CalcTransformTexCoords( &tmi, st );
}

// leilei - reveal normals to GLSL for light processing. HACK HACK HACK HACK HACK HACK
void RB_CalcNormal( unsigned char *colors )
{
	int			i, numVertexes;
	float		*v = tess.xyz[0];
	float		*normal = ( float * ) tess.normal; 
	vec3_t			n, m;



	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4) {
		int y;
		float mid;
		for (y=0;y<3;y++){
				n[y] = normal[y];
				
//				colors[i*4+y] = n[y];
			}
		//VectorNormalize(n);

			mid = n[1] + n[2];
			if (mid < 0) mid *= -1;
			

	//		m[0] = 127 - (n[1]*128);
	//		m[1] = 127 - (n[2]*128);
	//		m[2] = 255 - (mid*128);

			m[0] = 127 + (n[0]*128);
			m[1] = 127 + (n[1]*128);
			m[2] = 127 + (n[2]*128);

		
		colors[i*4+0] = m[0];
		colors[i*4+1] = m[1];
		colors[i*4+2] = m[2];
		colors[i*4+3] = 255;
	}
}

// leilei celsperiment


void RB_CalcFlatAmbient( unsigned char *colors )
{
	int				i;
	float			*v, *normal;
	//float			incoming;
	//int				ambientLightInt;
	vec3_t			ambientLight;
	//vec3_t			lightDir;
	//vec3_t			directedLight;
	int				numVertexes;
	trRefEntity_t* ent = backEnd.currentEntity;
	//ambientLightInt = ent->ambientLightInt;
	VectorCopy( ent->ambientLight, ambientLight );
	//VectorCopy( ent->directedLight, directedLight );


	//lightDir[0] = 0;
	//lightDir[1] = 0;
	//lightDir[2] = 1;

	v = tess.xyz[0];
	normal = tess.normal[0];

	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4) {
		int j = ambientLight[0];
		if ( j > 255 ) {
			j = 255;
		}
		colors[i*4+0] = j;

		j = ambientLight[1];
		if ( j > 255 ) {
			j = 255;
		}
		colors[i*4+1] = j;

		j = ambientLight[2];
		if ( j > 255 ) {
			j = 255;
		}
		colors[i*4+2] = j;
		colors[i*4+3] = 255;
	}
}

/*
** Calculates specular coefficient and places it in the alpha channel
*/
vec3_t lightOrigin = { -960, 1980, 96 };		// FIXME: track dynamically

static void RB_CalcSpecularAlpha( unsigned char *alphas )
{
	int			i;
	float		*v, *normal;
	vec3_t		viewer,  reflected;
	float		l, d;
	int			b;
	vec3_t		lightDir;
	int			numVertexes;

	v = tess.xyz[0];
	normal = tess.normal[0];

	alphas += 3;

	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4, alphas += 4) {
		float ilength;

		VectorSubtract( lightOrigin, v, lightDir );
//		ilength = Q_rsqrt( DotProduct( lightDir, lightDir ) );
		VectorNorm( lightDir );

		// calculate the specular color
		d = DotProduct (normal, lightDir);
//		d *= ilength;

		// we don't optimize for the d < 0 case since this tends to
		// cause visual artifacts such as faceted "snapping"
		reflected[0] = normal[0]*2*d - lightDir[0];
		reflected[1] = normal[1]*2*d - lightDir[1];
		reflected[2] = normal[2]*2*d - lightDir[2];

		VectorSubtract (backEnd.or.viewOrigin, v, viewer);
		ilength = sqrtf( DotProduct( viewer, viewer ) );
		l = DotProduct (reflected, viewer);
		l *= ilength;

		if (l < 0) {
			b = 0;
		} else {
			l = l*l;
			l = l*l;
			b = l * 255;
			if (b > 255) {
				b = 255;
			}
		}

		*alphas = b;
	}
}

/*
** The basic vertex lighting calc
*/

void RB_CalcDiffuseColor( unsigned char (*colors)[4] )
{
	int				i;
	float			*v, *normal;
	float			incoming;
	trRefEntity_t	*ent;
//	unsigned char	ambientLightRGBA[4];
	vec3_t			ambientLight;
	vec3_t			lightDir;
	vec3_t			directedLight;
	int				numVertexes;
#if idppc_altivec
	vector unsigned char vSel = (vector unsigned char)(0x00, 0x00, 0x00, 0xff,
							   0x00, 0x00, 0x00, 0xff,
							   0x00, 0x00, 0x00, 0xff,
							   0x00, 0x00, 0x00, 0xff);
	vector float ambientLightVec;
	vector float directedLightVec;
	vector float lightDirVec;
	vector float normalVec0, normalVec1;
	vector float incomingVec0, incomingVec1, incomingVec2;
	vector float zero, jVec;
	vector signed int jVecInt;
	vector signed short jVecShort;
	vector unsigned char jVecChar, normalPerm;
#endif
	ent = backEnd.currentEntity;
//	ambientLightRGBA[0] = ent->ambientLightRGBA[0];
//  ambientLightRGBA[1] = ent->ambientLightRGBA[1];
//	ambientLightRGBA[2] = ent->ambientLightRGBA[2];
//	ambientLightRGBA[3] = ent->ambientLightRGBA[3];

#if idppc_altivec
	// A lot of this could be simplified if we made sure
	// entities light info was 16-byte aligned.
	jVecChar = vec_lvsl(0, ent->ambientLight);
	ambientLightVec = vec_ld(0, (vector float *)ent->ambientLight);
	jVec = vec_ld(11, (vector float *)ent->ambientLight);
	ambientLightVec = vec_perm(ambientLightVec,jVec,jVecChar);

	jVecChar = vec_lvsl(0, ent->directedLight);
	directedLightVec = vec_ld(0,(vector float *)ent->directedLight);
	jVec = vec_ld(11,(vector float *)ent->directedLight);
	directedLightVec = vec_perm(directedLightVec,jVec,jVecChar);	 

	jVecChar = vec_lvsl(0, ent->lightDir);
	lightDirVec = vec_ld(0,(vector float *)ent->lightDir);
	jVec = vec_ld(11,(vector float *)ent->lightDir);
	lightDirVec = vec_perm(lightDirVec,jVec,jVecChar);	 

	zero = (vector float)vec_splat_s8(0);
	VectorCopy( ent->lightDir, lightDir );
#else
	VectorCopy( ent->ambientLight, ambientLight );
	VectorCopy( ent->directedLight, directedLight );
	VectorCopy( ent->lightDir, lightDir );
#endif

	v = tess.xyz[0];
	normal = tess.normal[0];

#if idppc_altivec
	normalPerm = vec_lvsl(0,normal);
#endif
	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4) {
#if idppc_altivec
		normalVec0 = vec_ld(0,(vector float *)normal);
		normalVec1 = vec_ld(11,(vector float *)normal);
		normalVec0 = vec_perm(normalVec0,normalVec1,normalPerm);
		incomingVec0 = vec_madd(normalVec0, lightDirVec, zero);
		incomingVec1 = vec_sld(incomingVec0,incomingVec0,4);
		incomingVec2 = vec_add(incomingVec0,incomingVec1);
		incomingVec1 = vec_sld(incomingVec1,incomingVec1,4);
		incomingVec2 = vec_add(incomingVec2,incomingVec1);
		incomingVec0 = vec_splat(incomingVec2,0);
		incomingVec0 = vec_max(incomingVec0,zero);
		normalPerm = vec_lvsl(12,normal);
		jVec = vec_madd(incomingVec0, directedLightVec, ambientLightVec);
		jVecInt = vec_cts(jVec,0);	// RGBx
		jVecShort = vec_pack(jVecInt,jVecInt);		// RGBxRGBx
		jVecChar = vec_packsu(jVecShort,jVecShort);	// RGBxRGBxRGBxRGBx
		jVecChar = vec_sel(jVecChar,vSel,vSel);		// RGBARGBARGBARGBA replace alpha with 255
		vec_ste((vector unsigned int)jVecChar,0,(unsigned int *)&colors[i*4]);	// store color
#else
		incoming = DotProduct (normal, lightDir);
		if ( incoming <= 0 )
        {
			colors[i][0] = ent->ambientLightRGBA[0];
            colors[i][1] = ent->ambientLightRGBA[1];
			colors[i][2] = ent->ambientLightRGBA[2];
			colors[i][3] = ent->ambientLightRGBA[3];

			continue;
		}

		int R = (int)( ambientLight[0] + incoming * directedLight[0] );
		if ( R > 255 ) {
			R = 255;
		}
		colors[i][0] = R;

		int G = (int)( ambientLight[1] + incoming * directedLight[1] );
		if ( G > 255 ) {
			G = 255;
		}
		colors[i][1] = G;

		int B = (int)( ambientLight[2] + incoming * directedLight[2] );
		if ( B > 255 ) {
			B = 255;
		}
		colors[i][2] = B;

		colors[i][3] = 255;
#endif
	}
}


void RB_CalcFlatDirect( unsigned char *colors )
{
	int				i;
	float			*v, *normal;
	// float			incoming;
	trRefEntity_t	*ent;
	//int				ambientLightInt;
	vec3_t			ambientLight;
	//vec3_t			lightDir;
	vec3_t			directedLight;
	int				numVertexes;
	ent = backEnd.currentEntity;
	//ambientLightInt = ent->ambientLightInt;
	VectorCopy( ent->ambientLight, ambientLight );
	VectorCopy( ent->directedLight, directedLight );
	

	directedLight[0] -= ambientLight[0];
	directedLight[1] -= ambientLight[1];
	directedLight[2] -= ambientLight[2];

	if (directedLight[0] < 0) directedLight[0] = 0;	
	if (directedLight[1] < 0) directedLight[1] = 0;
	if (directedLight[2] < 0) directedLight[2] = 0;

	//lightDir[0] = 0;
	//lightDir[1] = 0;
	//lightDir[2] = 1;

	v = tess.xyz[0];
	normal = tess.normal[0];

	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4)
    {
		int j = directedLight[0];
		if ( j > 255 ) {
			j = 255;
		}
		colors[i*4+0] = j;

		j = directedLight[1];
		if ( j > 255 ) {
			j = 255;
		}
		colors[i*4+1] = j;

		j = directedLight[2];
		if ( j > 255 ) {
			j = 255;
		}
		colors[i*4+2] = j;
		colors[i*4+3] = 255;
	}
}



/*
=============================================================

SURFACE SHADERS

=============================================================
*/

/*
** RB_CalcDiffuseColor
**
** The basic vertex lighting calc

static void RB_CalcDiffuseColor(unsigned char (*colors)[4])
{

    //trRefEntity_t* ent = backEnd.currentEntity;
    vec3_t abtLit;
    abtLit[0] = backEnd.currentEntity->ambientLight[0];
    abtLit[1] = backEnd.currentEntity->ambientLight[1];
    abtLit[2] = backEnd.currentEntity->ambientLight[2];

    vec3_t drtLit;
    drtLit[0] = backEnd.currentEntity->directedLight[0];
    drtLit[1] = backEnd.currentEntity->directedLight[1];
    drtLit[2] = backEnd.currentEntity->directedLight[2];

    vec3_t litDir;
    litDir[0] = backEnd.currentEntity->lightDir[0];
    litDir[1] = backEnd.currentEntity->lightDir[1];
    litDir[2] = backEnd.currentEntity->lightDir[2];

    unsigned char ambLitRGBA[4];  
    
    ambLitRGBA[0] = backEnd.currentEntity->ambientLightRGBA[0];
    ambLitRGBA[1] = backEnd.currentEntity->ambientLightRGBA[1];
    ambLitRGBA[2] = backEnd.currentEntity->ambientLightRGBA[2];
    ambLitRGBA[3] = backEnd.currentEntity->ambientLightRGBA[3];

    int numVertexes = tess.numVertexes;
	int	i;
	for (i = 0; i < numVertexes; i++)
    {
		float incoming = DotProduct(tess.normal[i], litDir);
		
        if( incoming <= 0 )
        {
			colors[i][0] = ambLitRGBA[0];
            colors[i][1] = ambLitRGBA[1];
			colors[i][2] = ambLitRGBA[2];
			colors[i][3] = ambLitRGBA[3];
		}
        else
        {
            int r = abtLit[0] + incoming * drtLit[0];
            int g = abtLit[1] + incoming * drtLit[1];
            int b = abtLit[2] + incoming * drtLit[2];

            colors[i][0] = (r <= 255 ? r : 255);
            colors[i][1] = (g <= 255 ? g : 255);
            colors[i][2] = (b <= 255 ? b : 255);
            colors[i][3] = 255;
        }
	}
}
*/

// This fixed version comes from ZEQ2Lite
//Calculates specular coefficient and places it in the alpha channel
static void RB_CalcSpecularAlphaNew(unsigned char (*alphas)[4])
{
	int numVertexes = tess.numVertexes;
	
    int	i;
    for (i = 0 ; i < numVertexes; i++)
    {
        vec3_t lightDir, viewer, reflected;
		if ( backEnd.currentEntity == &tr.worldEntity )
        {
            // old compatibility with maps that use it on some models
            vec3_t lightOrigin = {-960, 1980, 96};		// FIXME: track dynamically
			VectorSubtract( lightOrigin, tess.xyz[i], lightDir );
        }
        else
        {
			VectorCopy( backEnd.currentEntity->lightDir, lightDir );
        }
		// calculate the specular color
		float d = 2*DotProduct(tess.normal[i], lightDir);

		// we don't optimize for the d < 0 case since this tends to
		// cause visual artifacts such as faceted "snapping"
		reflected[0] = tess.normal[i][0]*d - lightDir[0];
		reflected[1] = tess.normal[i][1]*d - lightDir[1];
		reflected[2] = tess.normal[i][2]*d - lightDir[2];

		VectorSubtract(backEnd.or.viewOrigin, tess.xyz[i], viewer);
		
        float l = DotProduct(reflected, viewer)/sqrtf(DotProduct(viewer, viewer));

		if(l < 0)
			alphas[i][3] = 0;
        else if(l >= 1)
			alphas[i][3] = 255;
        else
        {
			l = l*l;
            alphas[i][3] = l*l*255;
		}
	}
}


/*
===================
RB_FogPass
Blends a fog texture on top of everything else
===================
*/
void RB_SetTessFogColor(unsigned char (* const pcolor)[4], uint32_t fnum, uint32_t nVerts)
{

    fog_t* pFogs = NULL;
    uint32_t nFogs = 0;
    
    // fog_t* fog = tr.world->fogs + fnum;

    R_GetFogArray(&pFogs, &nFogs);
    
    //ri.Printf(PRINT_ALL, "number of fog: %d, fnumL %d\n", nFogs, fnum);

    uint32_t i; 
    for (i = 0; i < nVerts; ++i)
    {
        pcolor[i][0] = pFogs[fnum].colorRGBA[0];
        pcolor[i][1] = pFogs[fnum].colorRGBA[1];
        pcolor[i][2] = pFogs[fnum].colorRGBA[2];
        pcolor[i][3] = pFogs[fnum].colorRGBA[3];
        // ri.Printf(PRINT_ALL, "nVerts: %d, pcolor %d, %d, %d, %d\n", 
        //        i, pcolor[i][0], pcolor[i][1], pcolor[i][2], pcolor[i][3]);
    }
    
}


void RB_ComputeColors( struct shaderStage_s * const pStage )
{
	int	i;
	//
	// rgbGen
	//
	switch ( pStage->rgbGen )
	{
		case CGEN_IDENTITY:
			memset( tess.svars.colors, 0xff, tess.numVertexes * 4 );
			break;
		default:
		case CGEN_IDENTITY_LIGHTING:
			memset( tess.svars.colors, tr.identityLightByte, tess.numVertexes * 4 );
			break;
		case CGEN_LIGHTING_DIFFUSE:
			RB_CalcDiffuseColor( tess.svars.colors );
			break;
		case CGEN_EXACT_VERTEX:
			memcpy( tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof( tess.vertexColors[0] ) );
			break;
		case CGEN_CONST:
        {
            
            uint32_t nVerts = tess.numVertexes;

			for ( i = 0; i < nVerts; ++i )
            {
				tess.svars.colors[i][0] = pStage->constantColor[0];
                tess.svars.colors[i][1] = pStage->constantColor[1];
				tess.svars.colors[i][2] = pStage->constantColor[2];
				tess.svars.colors[i][3] = pStage->constantColor[3];
			}
        } break;
		case CGEN_VERTEX:
			if ( tr.identityLight == 1.0f )
			{
				memcpy( tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof( tess.vertexColors[0] ) );
			}
			else
			{
				for ( i = 0; i < tess.numVertexes; ++i )
				{
					tess.svars.colors[i][0] = tess.vertexColors[i][0] * tr.identityLight;
					tess.svars.colors[i][1] = tess.vertexColors[i][1] * tr.identityLight;
					tess.svars.colors[i][2] = tess.vertexColors[i][2] * tr.identityLight;
					tess.svars.colors[i][3] = tess.vertexColors[i][3];
				}
			}
			break;
		case CGEN_ONE_MINUS_VERTEX:
			if ( tr.identityLight == 1.0f )
			{
				for ( i = 0; i < tess.numVertexes; ++i )
				{
					tess.svars.colors[i][0] = 255 - tess.vertexColors[i][0];
					tess.svars.colors[i][1] = 255 - tess.vertexColors[i][1];
					tess.svars.colors[i][2] = 255 - tess.vertexColors[i][2];
				}
			}
			else
			{
				for ( i = 0; i < tess.numVertexes; ++i )
				{
					tess.svars.colors[i][0] = ( 255 - tess.vertexColors[i][0] ) * tr.identityLight;
					tess.svars.colors[i][1] = ( 255 - tess.vertexColors[i][1] ) * tr.identityLight;
					tess.svars.colors[i][2] = ( 255 - tess.vertexColors[i][2] ) * tr.identityLight;
				}
			}
			break;
		case CGEN_FOG:
		{
            RB_SetTessFogColor(tess.svars.colors, tess.fogNum, tess.numVertexes);
		}break;
		case CGEN_WAVEFORM:
			RB_CalcWaveColor( &pStage->rgbWave, tess.numVertexes, tess.svars.colors);
			break;
		case CGEN_ENTITY:
			RB_CalcColorFromEntity( tess.svars.colors, tess.numVertexes );
			break;
		case CGEN_ONE_MINUS_ENTITY:
			RB_CalcColorFromOneMinusEntity( tess.svars.colors );
			break;
	}

	//
	// alphaGen
	//
	switch ( pStage->alphaGen )
	{
	case AGEN_SKIP:
		break;
	case AGEN_IDENTITY:
		if ( pStage->rgbGen != CGEN_IDENTITY )
        {
			if ( ( pStage->rgbGen == CGEN_VERTEX && tr.identityLight != 1 ) ||
				 pStage->rgbGen != CGEN_VERTEX )
            {
				for ( i = 0; i < tess.numVertexes; ++i )
                {
					tess.svars.colors[i][3] = 0xff;
				}
			}
		}
		break;
	case AGEN_CONST:
		if ( pStage->rgbGen != CGEN_CONST )
        {
			for ( i = 0; i < tess.numVertexes; ++i )
            {
				tess.svars.colors[i][3] = pStage->constantColor[3];
			}
		}
		break;
	case AGEN_WAVEFORM:
		RB_CalcWaveAlpha( &pStage->alphaWave, tess.numVertexes, tess.svars.colors );
		break;
	 case AGEN_LIGHTING_SPECULAR:
		// RB_CalcSpecularAlpha( ( unsigned char * ) tess.svars.colors );
		RB_CalcSpecularAlphaNew(tess.svars.colors);
		break;
	case AGEN_ENTITY:
		RB_CalcAlphaFromEntity( tess.numVertexes, tess.svars.colors );
		break;
	case AGEN_ONE_MINUS_ENTITY:
		RB_CalcAlphaFromOneMinusEntity( tess.numVertexes, tess.svars.colors );
		break;
    case AGEN_VERTEX:
		if ( pStage->rgbGen != CGEN_VERTEX )
        {
			for ( i = 0; i < tess.numVertexes; ++i )
            {
				tess.svars.colors[i][3] = tess.vertexColors[i][3];
			}
		}
        break;
    case AGEN_ONE_MINUS_VERTEX:
        for ( i = 0; i < tess.numVertexes; ++i )
        {
			tess.svars.colors[i][3] = 255 - tess.vertexColors[i][3];
        }
        break;
	case AGEN_PORTAL:
		{

			for ( i = 0; i < tess.numVertexes; ++i )
			{
				vec3_t v;

				VectorSubtract( tess.xyz[i], backEnd.viewParms.or.origin, v );
				int alpha = sqrtf( v[0]*v[0]+v[1]*v[1]+v[2]*v[2] ) / tess.shader->portalRange * 255;


				if ( alpha < 0 )
				{
					alpha = 0;
				}
				else if ( alpha > 255 )
				{
					alpha = 255;
				}

				tess.svars.colors[i][3] = alpha;
			}
		}
		break;
	}

	//
	// fog adjustment for colors to fade out as fog increases
	//
	if ( tess.fogNum )
	{
		switch ( pStage->adjustColorsForFog )
		{
		case ACFF_MODULATE_RGB:
			RB_CalcModulateColorsByFog( ( unsigned char * ) tess.svars.colors );
			break;
		case ACFF_MODULATE_ALPHA:
			RB_CalcModulateAlphasByFog( ( unsigned char * ) tess.svars.colors );
			break;
		case ACFF_MODULATE_RGBA:
			RB_CalcModulateRGBAsByFog( ( unsigned char * ) tess.svars.colors );
			break;
		case ACFF_NONE:
			break;
		}
	}
}



void RB_CalcFogTexCoords( float (* const pST)[2], uint32_t nVerts )
{
	int			i;
	float		*v;
	float		eyeT;
	vec3_t		local;
	vec4_t		fogDistanceVector, fogDepthVector = {0, 0, 0, 0};

	fog_t* fog = NULL; // = tr.world->fogs + tess.fogNum;
    uint32_t NumFog = 0;
    R_GetFogArray(&fog, &NumFog);
    // ugly ? yes !
    fog += tess.fogNum;


	// all fogging distance is based on world Z units
	VectorSubtract( backEnd.or.origin, backEnd.viewParms.or.origin, local );
	fogDistanceVector[0] = -backEnd.or.modelMatrix[2];
	fogDistanceVector[1] = -backEnd.or.modelMatrix[6];
	fogDistanceVector[2] = -backEnd.or.modelMatrix[10];
	fogDistanceVector[3] = DotProduct( local, backEnd.viewParms.or.axis[0] );

	// scale the fog vectors based on the fog's thickness
	fogDistanceVector[0] *= fog->tcScale;
	fogDistanceVector[1] *= fog->tcScale;
	fogDistanceVector[2] *= fog->tcScale;
	fogDistanceVector[3] *= fog->tcScale;

	// rotate the gradient vector for this orientation
	if ( fog->hasSurface ) {
		fogDepthVector[0] = fog->surface[0] * backEnd.or.axis[0][0] + 
			fog->surface[1] * backEnd.or.axis[0][1] + fog->surface[2] * backEnd.or.axis[0][2];
		fogDepthVector[1] = fog->surface[0] * backEnd.or.axis[1][0] + 
			fog->surface[1] * backEnd.or.axis[1][1] + fog->surface[2] * backEnd.or.axis[1][2];
		fogDepthVector[2] = fog->surface[0] * backEnd.or.axis[2][0] + 
			fog->surface[1] * backEnd.or.axis[2][1] + fog->surface[2] * backEnd.or.axis[2][2];
		fogDepthVector[3] = -fog->surface[3] + DotProduct( backEnd.or.origin, fog->surface );

		eyeT = DotProduct( backEnd.or.viewOrigin, fogDepthVector ) + fogDepthVector[3];
	} else {
		eyeT = 1;	// non-surface fog always has eye inside
	}

	// see if the viewpoint is outside
	// this is needed for clipping distance even for constant fog

	qboolean eyeOutside = ( eyeT < 0 ) ? qtrue : qfalse;

    fogDistanceVector[3] += 1.0/512;

	// calculate density for each point
	for (i = 0, v = tess.xyz[0] ; i < tess.numVertexes ; ++i, v += 4)
    {
		// calculate the length in fog
		float s = DotProduct( v, fogDistanceVector ) + fogDistanceVector[3];
		float t = DotProduct( v, fogDepthVector ) + fogDepthVector[3];

		// partially clipped fogs use the T axis		
		if ( eyeOutside ) {
			if ( t < 1.0 ) {
				t = 1.0/32;	// point is outside, so no fogging
			} else {
				t = 1.0/32 + 30.0/32 * t / ( t - eyeT );	// cut the distance at the fog plane
			}
		} else {
			if ( t < 0 ) {
				t = 1.0/32;	// point is outside, so no fogging
			} else {
				t = 31.0/32;
			}
		}

		pST[i][0] = s;
		pST[i][1] = t;
	}
}



void RB_ComputeTexCoords( shaderStage_t * const pStage )
{
	uint32_t i;
	uint32_t b;

	for ( b = 0; b < NUM_TEXTURE_BUNDLES; ++b )
    {
		int tm;

		//
		// generate the texture coordinates
		//
		switch ( pStage->bundle[b].tcGen )
		{
            case TCGEN_IDENTITY:
                memset( tess.svars.texcoords[b], 0, sizeof( float ) * 2 * tess.numVertexes );
                break;
            case TCGEN_TEXTURE:
                for ( i = 0 ; i < tess.numVertexes ; ++i )
                {
                    tess.svars.texcoords[b][i][0] = tess.texCoords[i][0][0];
                    tess.svars.texcoords[b][i][1] = tess.texCoords[i][0][1];
                }
                break;
            case TCGEN_LIGHTMAP:
                for ( i = 0 ; i < tess.numVertexes ; ++i )
                {
                    tess.svars.texcoords[b][i][0] = tess.texCoords[i][1][0];
                    tess.svars.texcoords[b][i][1] = tess.texCoords[i][1][1];
                }
                break;
            case TCGEN_VECTOR:
                for ( i = 0 ; i < tess.numVertexes ; ++i )
                {
                    tess.svars.texcoords[b][i][0] = DotProduct( tess.xyz[i], pStage->bundle[b].tcGenVectors[0] );
                    tess.svars.texcoords[b][i][1] = DotProduct( tess.xyz[i], pStage->bundle[b].tcGenVectors[1] );
                }
                break;
            case TCGEN_FOG:
                RB_CalcFogTexCoords( tess.svars.texcoords[b], tess.numVertexes );
                break;
            case TCGEN_ENVIRONMENT_MAPPED:
                RB_CalcEnvironmentTexCoords( ( float * ) tess.svars.texcoords[b] );
                break;
           case TCGEN_ENVIRONMENT_MAPPED_WATER:
                RB_CalcEnvironmentTexCoordsJO( ( float * ) tess.svars.texcoords[b] ); 			
                break;
            case TCGEN_ENVIRONMENT_CELSHADE_MAPPED:
                RB_CalcEnvironmentCelShadeTexCoords( ( float * ) tess.svars.texcoords[b] );
                break;
            case TCGEN_ENVIRONMENT_CELSHADE_LEILEI:
                RB_CalcCelTexCoords( ( float * ) tess.svars.texcoords[b] );
                break;
            case TCGEN_BAD:
                return;
		}

		//
		// alter texture coordinates
		//
		for ( tm = 0; tm < pStage->bundle[b].numTexMods ; ++tm )
        {
			switch ( pStage->bundle[b].texMods[tm].type )
			{
			case TMOD_NONE:
				tm = TR_MAX_TEXMODS;		// break out of for loop
				break;

			case TMOD_TURBULENT:
				RB_CalcTurbulentTexCoords( &pStage->bundle[b].texMods[tm].wave, ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_ENTITY_TRANSLATE:
				RB_CalcScrollTexCoords( backEnd.currentEntity->e.shaderTexCoord, ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_SCROLL:
				RB_CalcScrollTexCoords( pStage->bundle[b].texMods[tm].scroll, ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_SCALE:
				RB_CalcScaleTexCoords( pStage->bundle[b].texMods[tm].scale, ( float * ) tess.svars.texcoords[b] );
				break;
			
			case TMOD_STRETCH:
				RB_CalcStretchTexCoords( &pStage->bundle[b].texMods[tm].wave, ( float * ) tess.svars.texcoords[b] );
				break;
			case TMOD_ATLAS:
				RB_CalcAtlasTexCoords(  &pStage->bundle[b].texMods[tm].atlas, ( float * ) tess.svars.texcoords[b] );
				break;
			case TMOD_TRANSFORM:
				RB_CalcTransformTexCoords( &pStage->bundle[b].texMods[tm], ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_ROTATE:
				RB_CalcRotateTexCoords( pStage->bundle[b].texMods[tm].rotateSpeed, ( float * ) tess.svars.texcoords[b] );
				break;

			default:
				ri.Error( ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'\n", 
                        pStage->bundle[b].texMods[tm].type, tess.shader->name );
				break;
			}
		}
	}
}
