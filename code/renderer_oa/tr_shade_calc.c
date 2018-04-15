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
// tr_shade_calc.c

#include "tr_local.h"

extern backEndState_t backEnd;
extern shaderCommands_t	tess;
extern trGlobals_t tr;
extern refimport_t ri;

#define	WAVEVALUE(table, base, amplitude, phase, freq)  ((base) + table[ (unsigned int)( ( ( (phase) + tess.shaderTime * (freq) ) * FUNCTABLE_SIZE ) ) & FUNCTABLE_MASK ] * (amplitude))


static float *TableForFunc( genFunc_t func ) 
{
	switch ( func )
	{
        case GF_SIN:
            return tr.sinTable;
        case GF_TRIANGLE:
            return tr.triangleTable;
        case GF_SQUARE:
            return tr.squareTable;
        case GF_SAWTOOTH:
            return tr.sawToothTable;
        case GF_INVERSE_SAWTOOTH:
            return tr.inverseSawToothTable;
        case GF_NONE:
        default:
            break;
	}

	ri.Error( ERR_DROP, "TableForFunc called with invalid function '%d' in shader '%s'", func, tess.shader->name );
	return NULL;
}



/*
** Evaluates a given waveForm_t, referencing backEnd.refdef.time directly
*/
static float EvalWaveForm( const waveForm_t *wf ) 
{
	float* table = TableForFunc( wf->func );

    float wavevalue = wf->base + table[ (unsigned int)( ( wf->phase + tess.shaderTime * wf->frequency) * FUNCTABLE_SIZE ) & FUNCTABLE_MASK ] * wf->amplitude;

	return wavevalue;
}

static float EvalWaveFormClamped( const waveForm_t *wf )
{
	float glow  = EvalWaveForm( wf );

	if ( glow < 0 )
	{
		return 0;
	}

	if ( glow > 1 )
	{
		return 1;
	}

	return glow;
}

/*
** RB_CalcStretchTexCoords
*/
void RB_CalcStretchTexCoords( const waveForm_t *wf, float *st )
{

	float p = 1.0f / EvalWaveForm( wf );
	texModInfo_t tmi;
	tmi.matrix[0][0] = p;
	tmi.matrix[1][0] = 0;
	tmi.translate[0] = 0.5f - 0.5f * p;

	tmi.matrix[0][1] = 0;
	tmi.matrix[1][1] = p;
	tmi.translate[1] = 0.5f - 0.5f * p;

	RB_CalcTransformTexCoords( &tmi, st );
}


// leilei - this is for celshading
void RB_CalcLightscaleTexCoords(float *st )
{
	texModInfo_t tmi;

	vec3_t directedLight;
	VectorCopy( backEnd.currentEntity->directedLight, directedLight );
	//light = DotProduct (directedLight, lightDir);
	float light = ((directedLight[0] + directedLight[1] + directedLight[2]) * 0.333) / 255;
	if (light > 1)
		light = 1.0f;

	float p = 1.0f - (light * 0.7f);

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

DEFORMATIONS

====================================================================
*/

/*
========================
RB_CalcDeformVertexes

========================
*/
void RB_CalcDeformVertexes( deformStage_t *ds )
{
	int i;
	vec3_t	offset;
	float	scale;
	float	*xyz = ( float * ) tess.xyz;
	float	*normal = ( float * ) tess.normal;
	float	*table;

	if ( ds->deformationWave.frequency == 0 )
	{
		scale = EvalWaveForm( &ds->deformationWave );

		for ( i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 )
		{
			VectorScale( normal, scale, offset );
			
			xyz[0] += offset[0];
			xyz[1] += offset[1];
			xyz[2] += offset[2];
		}
	}
	else
	{
		table = TableForFunc( ds->deformationWave.func );

		for ( i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 )
		{
			float off = ( xyz[0] + xyz[1] + xyz[2] ) * ds->deformationSpread;

			scale = WAVEVALUE( table, ds->deformationWave.base, 
				ds->deformationWave.amplitude,
				ds->deformationWave.phase + off,
				ds->deformationWave.frequency );

			VectorScale( normal, scale, offset );
			
			xyz[0] += offset[0];
			xyz[1] += offset[1];
			xyz[2] += offset[2];
		}
	}
}

/*
=========================
RB_CalcDeformNormals

Wiggle the normals for wavy environment mapping
=========================
*/
void RB_CalcDeformNormals( deformStage_t *ds )
{
	int i;
	float* xyz = ( float * ) tess.xyz;
	float* normal = ( float * ) tess.normal;

	for ( i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 )
    {
		float scale = 0.98f;
		scale = R_NoiseGet4f( xyz[0] * scale, xyz[1] * scale, xyz[2] * scale, tess.shaderTime * ds->deformationWave.frequency );
		normal[ 0 ] += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = R_NoiseGet4f( 100 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale, tess.shaderTime * ds->deformationWave.frequency );
		normal[ 1 ] += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = R_NoiseGet4f( 200 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale, tess.shaderTime * ds->deformationWave.frequency );
		normal[ 2 ] += ds->deformationWave.amplitude * scale;

		FastVectorNormalize( normal );
	}
}


void RB_CalcDeformNormalsEvenMore( deformStage_t *ds )
{
	int i;
	float* xyz = ( float * ) tess.xyz;
	float* normal = ( float * ) tess.normal;

	for ( i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 )
    {
		float scale = 5.98f;
		scale = R_NoiseGet4f( xyz[0] * scale, xyz[1] * scale, xyz[2] * scale, tess.shaderTime * ds->deformationWave.frequency );
		normal[ 0 ] += ds->deformationWave.amplitude * scale;

		scale = 5.98f;
		scale = R_NoiseGet4f( 100 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale, tess.shaderTime * ds->deformationWave.frequency );
		normal[ 1 ] += ds->deformationWave.amplitude * scale;

		scale = 5.98f;
		scale = R_NoiseGet4f( 200 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,	tess.shaderTime * ds->deformationWave.frequency );
		normal[ 2 ] += ds->deformationWave.amplitude * scale;

		FastVectorNormalize( normal );
	}
}



void OldRB_CalcBulgeVertexes( deformStage_t *ds )
{

	const float *st = (const float *) tess.texCoords[0];
	float* xyz = (float *) tess.xyz;
	float* normal = (float *) tess.normal;
	float now = backEnd.refdef.time * ds->bulgeSpeed * 0.001f;

    int i;
	for(i = 0; i < tess.numVertexes; i++, xyz += 4, st += 4, normal += 4 )
    {
		int64_t off = (float)( FUNCTABLE_SIZE / (M_PI*2) ) * ( st[0] * ds->bulgeWidth + now );
		float scale = tr.sinTable[ off & FUNCTABLE_MASK ] * ds->bulgeHeight;
			
		xyz[0] += normal[0] * scale;
		xyz[1] += normal[1] * scale;
		xyz[2] += normal[2] * scale;
	}
}


// leilei - adapted a bit from the jk2 source, for performance

void RB_CalcBulgeVertexes( deformStage_t *ds )
{
	int i;
	float* xyz = ( float * ) tess.xyz;
	float* normal = ( float * ) tess.normal;

	if ( ds->bulgeSpeed == 0.0f && ds->bulgeWidth == 0.0f )
	{
		// We don't have a speed and width, so just use height to expand uniformly
		for ( i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 ) 
		{
			xyz[0] += normal[0] * ds->bulgeHeight;
			xyz[1] += normal[1] * ds->bulgeHeight;
			xyz[2] += normal[2] * ds->bulgeHeight;
		}	
	}
	else
	{
		const float *st = ( const float * ) tess.texCoords[0];
		float now = backEnd.refdef.time * ds->bulgeSpeed * 0.001f;

        for ( i = 0; i < tess.numVertexes; i++, xyz += 4, st += 4, normal += 4 )
        {
            int off = (float)( FUNCTABLE_SIZE / (M_PI*2) ) * ( st[0] * ds->bulgeWidth + now );
            float scale = tr.sinTable[ off & FUNCTABLE_MASK ] * ds->bulgeHeight;
                
            xyz[0] += normal[0] * scale;
            xyz[1] += normal[1] * scale;
            xyz[2] += normal[2] * scale;
        }
	}
}



/*
======================
RB_CalcMoveVertexes

A deformation that can move an entire surface along a wave path
======================
*/
void RB_CalcMoveVertexes( deformStage_t *ds )
{
	int	i;
	vec3_t offset;

	float* table = TableForFunc( ds->deformationWave.func );

    float scale = ds->deformationWave.base + ds->deformationWave.amplitude * 
    table[ (unsigned int)( ( ( ds->deformationWave.phase + tess.shaderTime * ds->deformationWave.frequency ) * FUNCTABLE_SIZE ) ) & FUNCTABLE_MASK ];


	VectorScale( ds->moveVector, scale, offset );

	float* xyz = ( float * ) tess.xyz;
	for ( i = 0; i < tess.numVertexes; i++, xyz += 4 )
    {
		VectorAdd( xyz, offset, xyz );
	}
}


/*
=============
DeformText

Change a polygon into a bunch of text polygons
=============
*/
void DeformText( const char *text )
{
	int		i;
	vec3_t	origin, width, height;
	int		ch;
	unsigned char color[4];

	height[0] = 0;
	height[1] = 0;
	height[2] = -1;
	CrossProduct( tess.normal[0], height, width );

	// find the midpoint of the box
	vec3_t mid = {0};
	float bottom = 999999;
	float top = -999999;

	for ( i = 0 ; i < 4 ; i++ )
    {
		VectorAdd( tess.xyz[i], mid, mid );
		if ( tess.xyz[i][2] < bottom ) {
			bottom = tess.xyz[i][2];
		}
		if ( tess.xyz[i][2] > top ) {
			top = tess.xyz[i][2];
		}
	}
	VectorScale( mid, 0.25f, origin );

	// determine the individual character size
	height[0] = 0;
	height[1] = 0;
	height[2] = ( top - bottom ) * 0.5f;

	VectorScale( width, height[2] * -0.75f, width );

	// determine the starting position
	int len = strlen( text );
	VectorMA( origin, (len-1), width, origin );

	// clear the shader indexes
	tess.numIndexes = 0;
	tess.numVertexes = 0;

	color[0] = color[1] = color[2] = color[3] = 255;

	// draw each character
	for ( i = 0 ; i < len ; i++ )
    {
		ch = text[i];
		ch &= 255;

		if ( ch != ' ' )
        {
			int row = ch>>4;
			int col = ch&15;

			int frow = row*0.0625f;
			int fcol = col*0.0625f;
			int size = 0.0625f;

			RB_AddQuadStampExt( origin, width, height, color, fcol, frow, fcol + size, frow + size );
		}
		VectorMA( origin, -2, width, origin );
	}
}

/*
==================
GlobalVectorToLocal
==================
*/
static void GlobalVectorToLocal( const vec3_t in, vec3_t out )
{
	out[0] = DotProduct( in, backEnd.or.axis[0] );
	out[1] = DotProduct( in, backEnd.or.axis[1] );
	out[2] = DotProduct( in, backEnd.or.axis[2] );
}

/*
=====================
AutospriteDeform

Assuming all the triangles for this shader are independant
quads, rebuild them as forward facing sprites
=====================
*/
static void AutospriteDeform( void )
{
	int		i;
	float	*xyz;
	vec3_t	mid, delta;
	float	radius;
	vec3_t	left, up;
	vec3_t	leftDir, upDir;

	if ( tess.numVertexes & 3 ) {
		ri.Printf( PRINT_WARNING, "Autosprite shader %s had odd vertex count\n", tess.shader->name );
	}
	if ( tess.numIndexes != ( tess.numVertexes >> 2 ) * 6 ) {
		ri.Printf( PRINT_WARNING, "Autosprite shader %s had odd index count\n", tess.shader->name );
	}

	int oldVerts = tess.numVertexes;
	tess.numVertexes = 0;
	tess.numIndexes = 0;

	if ( backEnd.currentEntity != &tr.worldEntity ) {
		GlobalVectorToLocal( backEnd.viewParms.or.axis[1], leftDir );
		GlobalVectorToLocal( backEnd.viewParms.or.axis[2], upDir );
	} else {
		VectorCopy( backEnd.viewParms.or.axis[1], leftDir );
		VectorCopy( backEnd.viewParms.or.axis[2], upDir );
	}

	for ( i = 0 ; i < oldVerts ; i+=4 ) {
		// find the midpoint
		xyz = tess.xyz[i];

		mid[0] = 0.25f * (xyz[0] + xyz[4] + xyz[8] + xyz[12]);
		mid[1] = 0.25f * (xyz[1] + xyz[5] + xyz[9] + xyz[13]);
		mid[2] = 0.25f * (xyz[2] + xyz[6] + xyz[10] + xyz[14]);

		VectorSubtract( xyz, mid, delta );
		radius = VectorLength( delta ) * 0.707f;		// / sqrt(2)

		VectorScale( leftDir, radius, left );
		VectorScale( upDir, radius, up );

		if ( backEnd.viewParms.isMirror ) {
			VectorSubtract( vec3_origin, left, left );
		}

	  // compensate for scale in the axes if necessary
  	if ( backEnd.currentEntity->e.nonNormalizedAxes ) {
      float axisLength;
		  axisLength = VectorLength( backEnd.currentEntity->e.axis[0] );
  		if ( !axisLength ) {
	  		axisLength = 0;
  		} else {
	  		axisLength = 1.0f / axisLength;
  		}
      VectorScale(left, axisLength, left);
      VectorScale(up, axisLength, up);
    }

		RB_AddQuadStamp( mid, left, up, tess.vertexColors[i] );
	}
}


/*
=====================
Autosprite2Deform

Autosprite2 will pivot a rectangular quad along the center of its long axis
=====================
*/
int edgeVerts[6][2] = {
	{ 0, 1 },
	{ 0, 2 },
	{ 0, 3 },
	{ 1, 2 },
	{ 1, 3 },
	{ 2, 3 }
};

static void Autosprite2Deform( void )
{
	int		i, j, k;
	int		indexes;
	float	*xyz;
	vec3_t	forward;

	if ( tess.numVertexes & 3 ) {
		ri.Printf( PRINT_WARNING, "Autosprite2 shader %s had odd vertex count", tess.shader->name );
	}
	if ( tess.numIndexes != ( tess.numVertexes >> 2 ) * 6 ) {
		ri.Printf( PRINT_WARNING, "Autosprite2 shader %s had odd index count", tess.shader->name );
	}

	if ( backEnd.currentEntity != &tr.worldEntity ) {
		GlobalVectorToLocal( backEnd.viewParms.or.axis[0], forward );
	} else {
		VectorCopy( backEnd.viewParms.or.axis[0], forward );
	}

	// this is a lot of work for two triangles...
	// we could precalculate a lot of it is an issue, but it would mess up
	// the shader abstraction
	for ( i = 0, indexes = 0 ; i < tess.numVertexes ; i+=4, indexes+=6 ) {
		float	lengths[2];
		int		nums[2];
		vec3_t	mid[2];
		vec3_t	major, minor;
		float	*v1, *v2;

		// find the midpoint
		xyz = tess.xyz[i];

		// identify the two shortest edges
		nums[0] = nums[1] = 0;
		lengths[0] = lengths[1] = 999999;

		for ( j = 0 ; j < 6 ; j++ ) {
			float	l;
			vec3_t	temp;

			v1 = xyz + 4 * edgeVerts[j][0];
			v2 = xyz + 4 * edgeVerts[j][1];

			VectorSubtract( v1, v2, temp );
			
			l = DotProduct( temp, temp );
			if ( l < lengths[0] ) {
				nums[1] = nums[0];
				lengths[1] = lengths[0];
				nums[0] = j;
				lengths[0] = l;
			} else if ( l < lengths[1] ) {
				nums[1] = j;
				lengths[1] = l;
			}
		}

		for ( j = 0 ; j < 2 ; j++ ) {
			v1 = xyz + 4 * edgeVerts[nums[j]][0];
			v2 = xyz + 4 * edgeVerts[nums[j]][1];

			mid[j][0] = 0.5f * (v1[0] + v2[0]);
			mid[j][1] = 0.5f * (v1[1] + v2[1]);
			mid[j][2] = 0.5f * (v1[2] + v2[2]);
		}

		// find the vector of the major axis
		VectorSubtract( mid[1], mid[0], major );

		// cross this with the view direction to get minor axis
		CrossProduct( major, forward, minor );
		VectorNormalize( minor );
		
		// re-project the points
		for ( j = 0 ; j < 2 ; j++ ) {
			float	l;

			v1 = xyz + 4 * edgeVerts[nums[j]][0];
			v2 = xyz + 4 * edgeVerts[nums[j]][1];

			l = 0.5 * sqrt( lengths[j] );
			
			// we need to see which direction this edge
			// is used to determine direction of projection
			for ( k = 0 ; k < 5 ; k++ ) {
				if ( tess.indexes[ indexes + k ] == i + edgeVerts[nums[j]][0]
					&& tess.indexes[ indexes + k + 1 ] == i + edgeVerts[nums[j]][1] ) {
					break;
				}
			}

			if ( k == 5 ) {
				VectorMA( mid[j], l, minor, v1 );
				VectorMA( mid[j], -l, minor, v2 );
			} else {
				VectorMA( mid[j], -l, minor, v1 );
				VectorMA( mid[j], l, minor, v2 );
			}
		}
	}
}


void RB_DeformTessGeometry( void )
{
	int	i;

	for( i = 0 ; i < tess.shader->numDeforms ; i++ )
    {
		deformStage_t *ds = &tess.shader->deforms[ i ];

		switch ( ds->deformation )
        {
            case DEFORM_NONE:
                break;
            case DEFORM_NORMALS:
                RB_CalcDeformNormals( ds );
                break;
            case DEFORM_WAVE:
                RB_CalcDeformVertexes( ds );
                break;
            case DEFORM_BULGE:
                RB_CalcBulgeVertexes( ds );
                break;
            case DEFORM_MOVE:
                RB_CalcMoveVertexes( ds );
                break;
            case DEFORM_PROJECTION_SHADOW:
                RB_ProjectionShadowDeform();
                break;
            case DEFORM_AUTOSPRITE:
                AutospriteDeform();
                break;
            case DEFORM_AUTOSPRITE2:
                Autosprite2Deform();
                break;
            case DEFORM_TEXT0:
            case DEFORM_TEXT1:
            case DEFORM_TEXT2:
            case DEFORM_TEXT3:
            case DEFORM_TEXT4:
            case DEFORM_TEXT5:
            case DEFORM_TEXT6:
            case DEFORM_TEXT7:
                DeformText( backEnd.refdef.text[ds->deformation - DEFORM_TEXT0] );
                break;
            default: break;
		}
	}
}


/*
====================================================================

COLORS

====================================================================
*/

void RB_CalcColorFromEntity( unsigned char *dstColors )
{
	if ( !backEnd.currentEntity )
		return;

	int *pColors = ( int * ) dstColors;
	int c = * ( int * ) backEnd.currentEntity->e.shaderRGBA;
    int i;
	for ( i = 0; i < tess.numVertexes; i++, pColors++ )
	{
		*pColors = c;
	}
}


void RB_CalcColorFromOneMinusEntity( unsigned char *dstColors )
{
	if ( !backEnd.currentEntity )
		return;

	int *pColors = ( int * ) dstColors;
	unsigned char invModulate[4];
	invModulate[0] = 255 - backEnd.currentEntity->e.shaderRGBA[0];
	invModulate[1] = 255 - backEnd.currentEntity->e.shaderRGBA[1];
	invModulate[2] = 255 - backEnd.currentEntity->e.shaderRGBA[2];
	invModulate[3] = 255 - backEnd.currentEntity->e.shaderRGBA[3];	// this trashes alpha, but the AGEN block fixes it

	int c = * ( int * ) invModulate;
    int i;
	for ( i = 0; i < tess.numVertexes; i++, pColors++ )
	{
		*pColors = c;
	}
}


void RB_CalcAlphaFromEntity( unsigned char *dstColors )
{
	if ( !backEnd.currentEntity )
		return;

	dstColors += 3;
	
    int	i;
	for ( i = 0; i < tess.numVertexes; i++, dstColors += 4 )
	{
		*dstColors = backEnd.currentEntity->e.shaderRGBA[3];
	}
}


void RB_CalcAlphaFromOneMinusEntity( unsigned char *dstColors )
{
	if ( !backEnd.currentEntity )
		return;

	dstColors += 3;
    int i;
	for ( i = 0; i < tess.numVertexes; i++, dstColors += 4 )
	{
		*dstColors = 0xff - backEnd.currentEntity->e.shaderRGBA[3];
	}
}


void RB_CalcWaveColor( const waveForm_t *wf, unsigned char *dstColors )
{
	int i;
	float glow;
	int *colors = ( int * ) dstColors;
	unsigned char color[4];


    if ( wf->func == GF_NOISE )
		glow = wf->base + R_NoiseGet4f( 0, 0, 0, ( tess.shaderTime + wf->phase ) * wf->frequency ) * wf->amplitude;
	else
		glow = EvalWaveForm( wf ) * tr.identityLight;
	
	if ( glow < 0 )
		glow = 0;
	else if ( glow > 1 )
		glow = 1;

	int v = 255 * glow;
	color[0] = color[1] = color[2] = v;
	color[3] = 255;
	v = *(int *)color;
	
	for ( i = 0; i < tess.numVertexes; i++, colors++ ) {
		*colors = v;
	}
}


void RB_CalcWaveAlpha( const waveForm_t *wf, unsigned char *dstColors )
{
	float glow = EvalWaveFormClamped( wf );
	int v = 255 * glow;
    int i;
	for ( i = 0; i < tess.numVertexes; i++, dstColors += 4 )
	{
		dstColors[3] = v;
	}
}


void RB_CalcModulateColorsByFog( unsigned char *colors )
{
	float texCoords[SHADER_MAX_VERTEXES][2];

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords( texCoords[0] );
    int i;
	for ( i = 0; i < tess.numVertexes; i++, colors += 4 )
    {
		float f = 1.0 - R_FogFactor( texCoords[i][0], texCoords[i][1] );
		colors[0] *= f;
		colors[1] *= f;
		colors[2] *= f;
	}
}


void RB_CalcModulateAlphasByFog( unsigned char *colors )
{
	float texCoords[SHADER_MAX_VERTEXES][2];

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords( texCoords[0] );
    int i;
	for ( i = 0; i < tess.numVertexes; i++, colors += 4 )
    {
		float f = 1.0 - R_FogFactor( texCoords[i][0], texCoords[i][1] );
		colors[3] *= f;
	}
}


void RB_CalcModulateRGBAsByFog( unsigned char *colors )
{
	float texCoords[SHADER_MAX_VERTEXES][2];

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords( texCoords[0] );
    int i;
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
RB_CalcFogTexCoords

To do the clipped fog plane really correctly, we should use projected textures,
but I don't trust the drivers and it doesn't fit our shader data.
========================
*/
void RB_CalcFogTexCoords( float *st )
{
	int			i;
	float		*v;
	float		s, t;
	float		eyeT;
	qboolean	eyeOutside;
	vec3_t		local;
	vec4_t		fogDistanceVector, fogDepthVector = {0, 0, 0, 0};

	fog_t* fog = tr.world->fogs + tess.fogNum;

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
	if ( fog->hasSurface )
    {
		fogDepthVector[0] = fog->surface[0] * backEnd.or.axis[0][0] + fog->surface[1] * backEnd.or.axis[0][1] + fog->surface[2] * backEnd.or.axis[0][2];
		fogDepthVector[1] = fog->surface[0] * backEnd.or.axis[1][0] + fog->surface[1] * backEnd.or.axis[1][1] + fog->surface[2] * backEnd.or.axis[1][2];
		fogDepthVector[2] = fog->surface[0] * backEnd.or.axis[2][0] + fog->surface[1] * backEnd.or.axis[2][1] + fog->surface[2] * backEnd.or.axis[2][2];
		fogDepthVector[3] = -fog->surface[3] + DotProduct( backEnd.or.origin, fog->surface );

		eyeT = DotProduct( backEnd.or.viewOrigin, fogDepthVector ) + fogDepthVector[3];
	}
    else
		eyeT = 1;	// non-surface fog always has eye inside


	// see if the viewpoint is outside
	// this is needed for clipping distance even for constant fog

	if ( eyeT < 0 )
		eyeOutside = qtrue;
	else
		eyeOutside = qfalse;


	fogDistanceVector[3] += 1.0/512;

	// calculate density for each point
	for (i = 0, v = tess.xyz[0] ; i < tess.numVertexes ; i++, v += 4)
    {
		// calculate the length in fog
		s = DotProduct( v, fogDistanceVector ) + fogDistanceVector[3];
		t = DotProduct( v, fogDepthVector ) + fogDepthVector[3];

		// partially clipped fogs use the T axis		
		if ( eyeOutside )
        {
			if ( t < 1.0 ) {
				t = 1.0/32;	// point is outside, so no fogging
			} else {
				t = 1.0/32 + 30.0/32 * t / ( t - eyeT );	// cut the distance at the fog plane
			}
		}
        else
        {
			if ( t < 0 ) {
				t = 1.0/32;	// point is outside, so no fogging
			} else {
				t = 31.0/32;
			}
		}

		st[0] = s;
		st[1] = t;
		st += 2;
	}
}



/*
** RB_CalcEnvironmentTexCoords
*/
void RB_CalcEnvironmentTexCoords( float *st ) 
{
	vec3_t viewer, reflected;

	float* v = tess.xyz[0];
	float* normal = tess.normal[0];
    int i;
	for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2 ) 
	{
		VectorSubtract (backEnd.or.viewOrigin, v, viewer);
		FastVectorNormalize(viewer);

		float d = DotProduct (normal, viewer);

		reflected[0] = normal[0]*2*d - viewer[0];
		reflected[1] = normal[1]*2*d - viewer[1];
		reflected[2] = normal[2]*2*d - viewer[2];

		st[0] = 0.5 + reflected[1] * 0.5;
		st[1] = 0.5 - reflected[2] * 0.5;
	}
}

/*
** RB_CalcEnvironmentTexCoordsNew

	This one also is offset by origin and axis which makes it look better on moving
	objects and weapons. May be slow.

*/
void RB_CalcEnvironmentTexCoordsNew( float *st ) 
{

	int			i;
	vec3_t		viewer, reflected, where, what, why, who;
	float *v = tess.xyz[0];
	float *normal = tess.normal[0];

	for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2 ) 
	{

		VectorSubtract (backEnd.or.axis[0], v, what);
		VectorSubtract (backEnd.or.axis[1], v, why);
		VectorSubtract (backEnd.or.axis[2], v, who);

		VectorSubtract (backEnd.or.origin, v, where);
		VectorSubtract (backEnd.or.viewOrigin, v, viewer);

		FastVectorNormalize(viewer);
		FastVectorNormalize(where);
		FastVectorNormalize(what);
		FastVectorNormalize(why);
		FastVectorNormalize(who);

		float d = DotProduct (normal, viewer);
		//a = DotProduct (normal, where);

		if ( backEnd.currentEntity == &tr.worldEntity )
        {
    		reflected[0] = normal[0]*2*d - viewer[0];
	    	reflected[1] = normal[1]*2*d - viewer[1];
		    reflected[2] = normal[2]*2*d - viewer[2];
		}
	    else
		{
		    reflected[0] = normal[0]*2*d - viewer[0] - (where[0] * 5) + (what[0] * 4);
		    reflected[1] = normal[1]*2*d - viewer[1] - (where[1] * 5) + (why[1] * 4);
		    reflected[2] = normal[2]*2*d - viewer[2] - (where[2] * 5) + (who[2] * 4);
		}
		st[0] = 0.33 + reflected[1] * 0.33;
		st[1] = 0.33 - reflected[2] * 0.33;
	}
}


/*
** RB_CalcEnvironmentTexCoordsHW

	Hardware-native cubemapping (or sphere mapping if the former is unsupported)

	adapted from this tremulous patch by Odin 

	NOTE: THIS BREAKS OTHER TCMODS IN A STAGE

void RB_CalcEnvironmentTexCoordsHW() 
{
	qglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
	qglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
	qglTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
	qglEnable(GL_TEXTURE_GEN_S);
	qglEnable(GL_TEXTURE_GEN_T);
	qglEnable(GL_TEXTURE_GEN_R);
}
*/

/*
** RB_CalcEnvironmentTexCoordsJO
	from JediOutcast source
*/
void RB_CalcEnvironmentTexCoordsJO( float *st ) 
{
	int			i;
	vec3_t		viewer;
	float		d;

	float* v = tess.xyz[0];
	float* normal = tess.normal[0];

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
			FastVectorNormalize(viewer);

			d = DotProduct (normal, viewer);
			st[0] = normal[0]*d - 0.5*viewer[0];
			st[1] = normal[1]*d - 0.5*viewer[1];
		}
	}
}

/*
** RB_CalcEnvironmentTexCoordsR
	Inpsired by Revolution, reflect from the sun light position instead
*/

void RB_CalcEnvironmentTexCoordsR( float *st ) 
{
	int			i;
	float		*v, *normal;
	vec3_t		viewer, reflected, sunned;
	float		d;
	vec3_t		sundy;
	float		size;
	float		dist;
	vec3_t		vec1, vec2;

	v = tess.xyz[0];
	normal = tess.normal[0];

	dist = 	backEnd.viewParms.zFar / 1.75;		// div sqrt(3)
	size = dist * 0.4;

	VectorScale( tr.sunDirection, dist, sundy);
	PerpendicularVector( vec1, tr.sunDirection );
	CrossProduct( tr.sunDirection, vec1, vec2 );

	VectorScale( vec1, size, vec1 );
	VectorScale( vec2, size, vec2 );


	v = tess.xyz[0];
	normal = tess.normal[0];

	for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2 ) 
	{
		VectorSubtract (backEnd.or.viewOrigin, v, viewer);
		FastVectorNormalize(viewer);

		VectorSubtract (sundy, v, sunned);
		FastVectorNormalize(sunned);

		d = DotProduct (normal, viewer) + DotProduct (viewer, sunned);

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
void RB_CalcCelTexCoords( float *st ) 
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
		FastVectorNormalize(viewer);

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




void RB_CalcEnvironmentCelShadeTexCoords( float *st ) 
{
    int    i;
    vec3_t lightDir;

    float* normal = tess.normal[0];
	float* v = tess.xyz[0];

	// Calculate only once
//	VectorCopy( backEnd.currentEntity->lightDir, lightDir );
//	if ( backEnd.currentEntity == &tr.worldEntity )
//		VectorSubtract( lightOrigin, v, lightDir );
//	else
	VectorCopy( backEnd.currentEntity->lightDir, lightDir );
	FastVectorNormalize( lightDir );

    for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2 )
    {
		float d= DotProduct( normal, lightDir );

		st[0] = 0.5 + d * 0.5;
		st[1] = 0.5;
    }
}




void RB_CalcTurbulentTexCoords( const waveForm_t *wf, float *st )
{
	float now = ( wf->phase + tess.shaderTime * wf->frequency );
    int i;
	for ( i = 0; i < tess.numVertexes; i++, st += 2 )
	{
		float s = st[0];
		float t = st[1];

		st[0] = s + tr.sinTable[ ( ( int ) ( ( ( tess.xyz[i][0] + tess.xyz[i][2] )* 1.0/128 * 0.125 + now ) * FUNCTABLE_SIZE ) ) & ( FUNCTABLE_MASK ) ] * wf->amplitude;
		st[1] = t + tr.sinTable[ ( ( int ) ( ( tess.xyz[i][1] * 1.0/128 * 0.125 + now ) * FUNCTABLE_SIZE ) ) & ( FUNCTABLE_MASK ) ] * wf->amplitude;
	}
}



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

/*
** RB_CalcTransformTexCoords
*/
void RB_CalcTransformTexCoords( const texModInfo_t *tmi, float *st  )
{
	int i;

	for ( i = 0; i < tess.numVertexes; i++, st += 2 )
	{
		float s = st[0];
		float t = st[1];

		st[0] = s * tmi->matrix[0][0] + t * tmi->matrix[1][0] + tmi->translate[0];
		st[1] = s * tmi->matrix[0][1] + t * tmi->matrix[1][1] + tmi->translate[1];
	}
}

/*
** RB_CalcRotateTexCoords
*/
void RB_CalcRotateTexCoords( float degsPerSecond, float *st )
{
	float timeScale = tess.shaderTime;
	float degs;
	int index;
	float sinValue, cosValue;
	texModInfo_t tmi;

	degs = -degsPerSecond * timeScale;
	index = degs * ( FUNCTABLE_SIZE / 360.0f );

	sinValue = tr.sinTable[ index & FUNCTABLE_MASK ];
	cosValue = tr.sinTable[ ( index + FUNCTABLE_SIZE / 4 ) & FUNCTABLE_MASK ];

	tmi.matrix[0][0] = cosValue;
	tmi.matrix[1][0] = -sinValue;
	tmi.translate[0] = 0.5 - 0.5 * cosValue + 0.5 * sinValue;

	tmi.matrix[0][1] = sinValue;
	tmi.matrix[1][1] = cosValue;
	tmi.translate[1] = 0.5 - 0.5 * sinValue - 0.5 * cosValue;

	RB_CalcTransformTexCoords( &tmi, st );
}

/*  
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
*/


/*
** RB_CalcSpecularAlpha
**
** Calculates specular coefficient and places it in the alpha channel
*/

// This fixed version comes from ZEQ2Lite
void RB_CalcSpecularAlphaNew( unsigned char *alphas )
{
	vec3_t		viewer,  reflected;
	int			i, b;
	vec3_t		lightDir;

	float* v = tess.xyz[0];
	float* normal = tess.normal[0];
	int numVertexes = tess.numVertexes;

	alphas += 3;
    vec3_t lightOrigin = { -960, 1980, 96 };		// FIXME: track dynamically
	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4, alphas += 4)
    {
		if ( backEnd.currentEntity == &tr.worldEntity )
			VectorSubtract( lightOrigin, v, lightDir );	// old compatibility with maps that use it on some models
		else
			VectorCopy( backEnd.currentEntity->lightDir, lightDir );

		FastVectorNormalize( lightDir );

		// calculate the specular color
		float d = DotProduct (normal, lightDir);

		// we don't optimize for the d < 0 case since this tends to
		// cause visual artifacts such as faceted "snapping"
		reflected[0] = normal[0]*2*d - lightDir[0];
		reflected[1] = normal[1]*2*d - lightDir[1];
		reflected[2] = normal[2]*2*d - lightDir[2];

		VectorSubtract (backEnd.or.viewOrigin, v, viewer);
		float ilength = Q_rsqrt( DotProduct( viewer, viewer ) );
		float l = DotProduct (reflected, viewer);
		l *= ilength;

		if (l < 0)
			b = 0;
        else
        {
			b = l*l*l*l*255;
			if (b > 255)
            {
				b = 255;
			}
		}

		*alphas = b;
	}
}



/*
** RB_CalcDynamicColor
**
** MDave; Vertex color dynamic lighting for cel shading
*/
void RB_CalcDynamicColor( unsigned char *colors )
{
	int				i;
	vec4_t			dynamic;
	int				numVertexes;

	trRefEntity_t* ent = backEnd.currentEntity;

	VectorCopy( ent->dynamicLight, dynamic );

	float normalize = NormalizeColor( dynamic, dynamic );
	if ( normalize > 255 ) normalize = 255;
	VectorScale( dynamic, normalize, dynamic );
	dynamic[3] = 255;

	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++ )
    {
		colors[i*4+0] = dynamic[0];
		colors[i*4+1] = dynamic[1];
		colors[i*4+2] = dynamic[2];
		colors[i*4+3] = dynamic[3];
	}
}


// leilei celsperiment


void RB_CalcFlatAmbient( unsigned char *colors )
{
	int				i;
	vec3_t			ambientLight;
	trRefEntity_t* ent = backEnd.currentEntity;
	//ambientLightInt = ent->ambientLightInt;
	VectorCopy( ent->ambientLight, ambientLight );
	//VectorCopy( ent->directedLight, directedLight );


	//lightDir[0] = 0;
	//lightDir[1] = 0;
	//lightDir[2] = 1;

	float* v = tess.xyz[0];
	float* normal = tess.normal[0];
	int numVertexes = tess.numVertexes;
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

